/*----------------------------------------------------------------------*/
/*! \file

\brief A service method allowing the application of initial conditions
       for nurbs discretisations.


\level 2
*/
/*----------------------------------------------------------------------*/
#include "4C_nurbs_discret_apply_nurbs_initial_condition.hpp"

#include "4C_fem_discretization.hpp"
#include "4C_fem_general_utils_integration.hpp"
#include "4C_fem_general_utils_nurbs_shapefunctions.hpp"
#include "4C_linalg_utils_sparse_algebra_assemble.hpp"
#include "4C_linalg_utils_sparse_algebra_create.hpp"
#include "4C_linear_solver_method_linalg.hpp"
#include "4C_nurbs_discret.hpp"
#include "4C_utils_function.hpp"

FOUR_C_NAMESPACE_OPEN

namespace
{
  /*----------------------------------------------------------------------*/
  /*!
  \brief A service method allowing the application of initial conditions
         for nurbs discretisations.

  This method provides the following:

  Given an initial distribution u_0(x) of initial values (for example by a
  spatial function), we compute control point values u_cp such that they
  minimize the following least-squares problem:

                        ||                                   || 2
                    || +----+                            ||
                    ||  \                                ||
               min  ||   +    N   (x) * u     -  u   (x) ||
                    ||  /      cp       - cp     - 0     ||
               u    || +----+                            ||
               - cp ||   cp                              ||


  This is equivalent to the solution of the following linear system:


           /                         \               /                         \
          |    /                      |             |    /                      |
   +----+ |   |                       |             |   |                       |
    \     |   |                       |    dim      |   |            dim        |
     +    |   | N   (x) * N   (x) dx  | * u     =   |   | N   (x) * u   (x) dx  |
    /     |   |  cp        cp         |    cp       |   |  cp        0          |
   +----+ |   |    j         i        |      j      |   |    j                  |
     cp   |  /                        |             |  /                        |
       j   \                         /               \                         /

          |                           |             |                           |
          +---------------------------+             +---------------------------+

                 M(assmatrix)                                   rhs



  \param dis         (i) the discretisation
  \param solver      (i) a solver object for the least-squares problem
  \param startfuncno (i) the number of the startfunction defining
                         the initial field (i.e. u_0(x))
  \param initialvals (o) the initial field on output (i.e. u_cp)

  \date 04/09
  */

  void apply_nurbs_initial_condition_solve(Core::FE::Discretization& dis,
      Core::LinAlg::Solver& solver, const Core::UTILS::FunctionOfSpaceTime& start_function,
      Teuchos::RCP<Epetra_Vector> initialvals)
  {
    // try to cast dis to a nurbs discretisation --- if possible, proceed
    // with setting initial conditions. Otherwise return.
    Discret::Nurbs::NurbsDiscretization* nurbsdis =
        dynamic_cast<Discret::Nurbs::NurbsDiscretization*>(&dis);

    if (nurbsdis == nullptr)
    {
      return;
    }

    // get the knotvector from nurbs discretisation
    Teuchos::RCP<Discret::Nurbs::Knotvector> knots = nurbsdis->GetKnotVector();

    // get the processor ID from the communicator
    const int myrank = dis.Comm().MyPID();

    if (myrank == 0)
    {
      printf("\n");
      printf("Setting up least-squares Nurbs approximation of initial field (discretization %s)\n",
          dis.Name().c_str());
    }

    // -------------------------------------------------------------------
    // get a vector layout from the discretization to construct matching
    // vectors and matrices
    //                 local <-> global dof numbering
    // -------------------------------------------------------------------
    const Epetra_Map* dofrowmap = dis.dof_row_map();

    // -------------------------------------------------------------------
    // create empty mass matrix
    // -------------------------------------------------------------------
    Teuchos::RCP<Core::LinAlg::SparseMatrix> massmatrix =
        Teuchos::rcp(new Core::LinAlg::SparseMatrix(*dofrowmap, 108, false, true));

    // -------------------------------------------------------------------
    // create empty right hand side vector
    // -------------------------------------------------------------------
    Teuchos::RCP<Epetra_Vector> rhs = Core::LinAlg::CreateVector(*dofrowmap, true);

    // -------------------------------------------------------------------
    // call elements to calculate massmatrix and righthandside
    // -------------------------------------------------------------------
    {
      // call elements and assemble
      if (!nurbsdis->Filled()) FOUR_C_THROW("fill_complete() was not called");
      if (!nurbsdis->HaveDofs()) FOUR_C_THROW("assign_degrees_of_freedom() was not called");

      // see what we have for input
      bool assemblemat = massmatrix != Teuchos::null;
      bool assemblevec = rhs != Teuchos::null;

      // define element matrices and vectors
      Core::LinAlg::SerialDenseMatrix elemass;
      Core::LinAlg::SerialDenseVector elerhs;

      std::vector<int> lm;
      std::vector<int> lmowner;
      std::vector<int> lmstride;

      // loop over column elements
      const int numcolele = nurbsdis->NumMyColElements();

      int every = numcolele / 58;
      // prevent division by zero when dividing by every later on
      if (every < 1) every = 1;

      for (int i = 0; i < numcolele; ++i)
      {
        // first 4C progress bar
        if (myrank == 0 && i % every == 0)
        {
          printf(".");
          fflush(nullptr);
        }

        Core::Elements::Element* actele = nurbsdis->lColElement(i);

        // get element location vector, dirichlet flags and ownerships
        lm.clear();
        lmowner.clear();
        actele->LocationVector(*nurbsdis, lm, lmowner, lmstride);

        // get dimension of element matrices and vectors
        // Reshape element matrices and vectors and init to zero
        const int eledim = (int)lm.size();

        if (assemblemat)
        {
          if (elemass.numRows() != eledim or elemass.numCols() != eledim)
            elemass.shape(eledim, eledim);
          else
            elemass.putScalar(0.0);
        }
        if (assemblevec)
        {
          if (elerhs.length() != eledim)
            elerhs.size(eledim);
          else
            elerhs.putScalar(0.0);
        }

        {
          int spacedim = -1;

          const Core::FE::CellType distype = actele->Shape();
          switch (distype)
          {
            case Core::FE::CellType::nurbs4:
            case Core::FE::CellType::nurbs9:
            {
              spacedim = 2;
              break;
            }
            case Core::FE::CellType::nurbs8:
            case Core::FE::CellType::nurbs27:
            {
              spacedim = 3;
              break;
            }
            default:
              FOUR_C_THROW("this method is designed for usage with NurbsDiscretization only");
              break;
          }

          // set element data
          const int iel = actele->num_node();

          // dofblocks (spacedim or spacedim+1 for fluid problems)
          const int dofblock = eledim / iel;

          // get node coordinates of element
          Core::LinAlg::SerialDenseMatrix xyze(spacedim, iel);
          Core::Nodes::Node** nodes = actele->Nodes();
          for (int inode = 0; inode < iel; inode++)
          {
            const auto& x = nodes[inode]->X();
            for (int dim = 0; dim < spacedim; ++dim)
            {
              xyze(dim, inode) = x[dim];
            }
          }

          // aquire weights from nodes
          Core::LinAlg::SerialDenseVector weights(iel);

          for (int inode = 0; inode < iel; ++inode)
          {
            Discret::Nurbs::ControlPoint* cp =
                dynamic_cast<Discret::Nurbs::ControlPoint*>(nodes[inode]);

            weights(inode) = cp->W();
          }

          // access elements knot span
          std::vector<Core::LinAlg::SerialDenseVector> eleknots(spacedim);

          bool zero_size = false;
          zero_size = knots->GetEleKnots(eleknots, actele->Id());

          // nothing to be done for a zero sized element
          if (zero_size)
          {
            continue;
          }

          Core::LinAlg::SerialDenseVector funct(iel);
          Core::LinAlg::SerialDenseMatrix xjm(spacedim, spacedim);
          Core::LinAlg::SerialDenseMatrix deriv(spacedim, iel);
          Core::LinAlg::SerialDenseVector gp(spacedim);
          Core::LinAlg::SerialDenseVector position(
              3);  // always three-dimensional coordinates for function evaluation!
          Core::LinAlg::SerialDenseVector initialval(dofblock);

          // depending on the spatial dimension, we need a different
          // integration scheme
          switch (spacedim)
          {
            case 2:
            {
              // gaussian points
              const Core::FE::IntegrationPoints2D intpoints(Core::FE::GaussRule2D::quad_9point);

              for (int iquad = 0; iquad < intpoints.nquad; ++iquad)
              {
                // set gauss point coordinates
                for (int rr = 0; rr < spacedim; ++rr)
                {
                  gp(rr) = intpoints.qxg[iquad][rr];
                }

                Core::FE::Nurbs::nurbs_get_2D_funct_deriv(
                    funct, deriv, gp, eleknots, weights, distype);

                // get transposed Jacobian matrix and determinant
                //
                //        +-            -+ T      +-            -+
                //        | dx   dx   dx |        | dx   dy   dz |
                //        | --   --   -- |        | --   --   -- |
                //        | dr   ds   dt |        | dr   dr   dr |
                //        |              |        |              |
                //        | dy   dy   dy |        | dx   dy   dz |
                //        | --   --   -- |   =    | --   --   -- |
                //        | dr   ds   dt |        | ds   ds   ds |
                //        |              |        |              |
                //        | dz   dz   dz |        | dx   dy   dz |
                //        | --   --   -- |        | --   --   -- |
                //        | dr   ds   dt |        | dt   dt   dt |
                //        +-            -+        +-            -+
                //
                // The Jacobian is computed using the formula
                //
                //            +-----
                //   dx_j(r)   \      dN_k(r)
                //   -------  = +     ------- * (x_j)_k
                //    dr_i     /       dr_i       |
                //            +-----    |         |
                //            node k    |         |
                //                  derivative    |
                //                   of shape     |
                //                   function     |
                //                           component of
                //                          node coordinate
                //
                for (int rr = 0; rr < spacedim; ++rr)
                {
                  for (int mm = 0; mm < spacedim; ++mm)
                  {
                    xjm(rr, mm) = deriv(rr, 0) * xyze(mm, 0);
                    for (int nn = 1; nn < iel; ++nn)
                    {
                      xjm(rr, mm) += deriv(rr, nn) * xyze(mm, nn);
                    }
                  }
                }

                // The determinant is computed using Sarrus's rule
                const double det = xjm(0, 0) * xjm(1, 1) - xjm(0, 1) * xjm(1, 0);

                // get real physical coordinates of integration point
                /*
                //              +-----
                //               \
                //    pos (x) =   +      N (x) * x
                //               /        j       j
                //              +-----
                //              node j
                */
                for (int rr = 0; rr < spacedim; ++rr)
                {
                  position(rr) = funct(0) * xyze(rr, 0);
                  for (int mm = 1; mm < iel; ++mm)
                  {
                    position(rr) += funct(mm) * xyze(rr, mm);
                  }
                }
                // if spacedim < 3, ensure we define a valid z-coordinate!
                for (int rr = spacedim; rr < 3; ++rr) position(rr) = 0.0;

                for (int rr = 0; rr < dofblock; ++rr)
                {
                  // important: position has to have always three components!!
                  initialval(rr) = start_function.Evaluate(position.values(), 0.0, rr);
                }


                // check for degenerated elements
                if (det < 1E-16)
                {
                  FOUR_C_THROW("GLOBAL ELEMENT NO.%i\nZERO OR NEGATIVE JACOBIAN DETERMINANT: %f",
                      actele->Id(), det);
                }

                // set total integration factor
                double fac = intpoints.qwgt[iquad] * det;

                for (int vi = 0; vi < iel; ++vi)  // loop rows  (test functions)
                {
                  const int fvi = dofblock * vi;

                  for (int ui = 0; ui < iel; ++ui)  // loop columns  (test functions)
                  {
                    const int fui = dofblock * ui;

                    const double diag = fac * funct(ui) * funct(vi);

                    for (int rr = 0; rr < dofblock; ++rr)
                    {
                      elemass(fvi + rr, fui + rr) += diag;
                    }
                  }
                  for (int rr = 0; rr < dofblock; ++rr)
                  {
                    elerhs(fvi + rr) += fac * funct(vi) * initialval(rr);
                  }
                }
              }  // end gaussloop
              break;
            }
            case 3:
            {
              // gaussian points
              const Core::FE::IntegrationPoints3D intpoints(Core::FE::GaussRule3D::hex_27point);

              for (int iquad = 0; iquad < intpoints.nquad; ++iquad)
              {
                // set gauss point coordinates
                for (int rr = 0; rr < spacedim; ++rr)
                {
                  gp(rr) = intpoints.qxg[iquad][rr];
                }

                Core::FE::Nurbs::nurbs_get_3D_funct_deriv(
                    funct, deriv, gp, eleknots, weights, distype);

                // get transposed Jacobian matrix and determinant
                //
                //        +-            -+ T      +-            -+
                //        | dx   dx   dx |        | dx   dy   dz |
                //        | --   --   -- |        | --   --   -- |
                //        | dr   ds   dt |        | dr   dr   dr |
                //        |              |        |              |
                //        | dy   dy   dy |        | dx   dy   dz |
                //        | --   --   -- |   =    | --   --   -- |
                //        | dr   ds   dt |        | ds   ds   ds |
                //        |              |        |              |
                //        | dz   dz   dz |        | dx   dy   dz |
                //        | --   --   -- |        | --   --   -- |
                //        | dr   ds   dt |        | dt   dt   dt |
                //        +-            -+        +-            -+
                //
                // The Jacobian is computed using the formula
                //
                //            +-----
                //   dx_j(r)   \      dN_k(r)
                //   -------  = +     ------- * (x_j)_k
                //    dr_i     /       dr_i       |
                //            +-----    |         |
                //            node k    |         |
                //                  derivative    |
                //                   of shape     |
                //                   function     |
                //                           component of
                //                          node coordinate
                //
                for (int rr = 0; rr < spacedim; ++rr)
                {
                  for (int mm = 0; mm < spacedim; ++mm)
                  {
                    xjm(rr, mm) = deriv(rr, 0) * xyze(mm, 0);
                    for (int nn = 1; nn < iel; ++nn)
                    {
                      xjm(rr, mm) += deriv(rr, nn) * xyze(mm, nn);
                    }
                  }
                }

                // The determinant is computed using Sarrus's rule
                const double det =
                    xjm(0, 0) * xjm(1, 1) * xjm(2, 2) + xjm(2, 0) * xjm(0, 1) * xjm(1, 2) +
                    xjm(0, 2) * (xjm(1, 0) * xjm(2, 1) - xjm(2, 0) * xjm(1, 1)) -
                    xjm(2, 1) * xjm(0, 0) * xjm(1, 2) - xjm(0, 1) * xjm(1, 0) * xjm(2, 2);


                // get real physical coordinates of integration point
                /*
                //              +-----
                //               \
                //    pos (x) =   +      N (x) * x
                //               /        j       j
                //              +-----
                //              node j
                */
                for (int rr = 0; rr < spacedim; ++rr)
                {
                  position(rr) = funct(0) * xyze(rr, 0);
                  for (int mm = 1; mm < iel; ++mm)
                  {
                    position(rr) += funct(mm) * xyze(rr, mm);
                  }
                }
                // if spacedim < 3, ensure we define a valid z-coordinate!
                for (int rr = spacedim; rr < 3; ++rr) position(rr) = 0.0;

                for (int rr = 0; rr < dofblock; ++rr)
                {
                  // important: position has to have always three components!!
                  initialval(rr) = start_function.Evaluate(position.values(), 0.0, rr);
                }

                // check for degenerated elements
                if (det < 0.0)
                {
                  FOUR_C_THROW(
                      "GLOBAL ELEMENT NO.%i\nNEGATIVE JACOBIAN DETERMINANT: %f", actele->Id(), det);
                }

                // set total integration factor
                double fac = intpoints.qwgt[iquad] * det;

                for (int vi = 0; vi < iel; ++vi)  // loop rows  (test functions)
                {
                  const int fvi = dofblock * vi;

                  for (int ui = 0; ui < iel; ++ui)  // loop columns  (test functions)
                  {
                    const int fui = dofblock * ui;

                    const double diag = fac * funct(ui) * funct(vi);

                    for (int rr = 0; rr < dofblock; ++rr)
                    {
                      elemass(fvi + rr, fui + rr) += diag;
                    }
                  }
                  for (int rr = 0; rr < dofblock; ++rr)
                  {
                    elerhs(fvi + rr) += fac * funct(vi) * initialval(rr);
                  }
                }
              }  // end gaussloop
              break;
            }
            default:
              FOUR_C_THROW(
                  "expecting two or three-dimensional problems to set the initial conditions\n");
              break;
          }
        }

        int eid = actele->Id();
        if (assemblemat) massmatrix->Assemble(eid, elemass, lm, lmowner);
        if (assemblevec) Core::LinAlg::Assemble(*rhs, elerhs, lm, lmowner);
      }  // for (int i=0; i<numcolele; ++i)
    }

    if (myrank == 0)
    {
      printf("\n");
      printf("\n");
      printf("Solving least-squares problem: ");
    }

    // -------------------------------------------------------------------
    // finalize the system matrix
    // -------------------------------------------------------------------
    massmatrix->Complete();

    // -------------------------------------------------------------------
    // solve system
    // -------------------------------------------------------------------

    // always refactor and reset the matrix before a single new solver call

    initialvals->PutScalar(0.0);
    Core::LinAlg::SolverParams solver_params;
    solver_params.refactor = true;
    solver_params.reset = true;
    solver.Solve(massmatrix->EpetraOperator(), initialvals, rhs, solver_params);

    // perform resets for solver and matrix
    solver.Reset();
    massmatrix->Reset();

    if (myrank == 0)
    {
      printf("Initial field set.\n\n");
    }

    return;
  }
}  // namespace

/*----------------------------------------------------------------------*/
/*!
   A service method allowing the application of initial conditions
   for nurbs discretisations. recommended version with allocation
   of a separate solver!
*/
/*----------------------------------------------------------------------*/
void Discret::Nurbs::apply_nurbs_initial_condition(Core::FE::Discretization& dis,
    const Teuchos::ParameterList& solverparams,
    const Core::UTILS::FunctionOfSpaceTime& start_function, Teuchos::RCP<Epetra_Vector> initialvals)
{
  // try to cast dis to a nurbs discretisation --- if possible, proceed
  // with setting initial conditions. Otherwise return.
  Discret::Nurbs::NurbsDiscretization* nurbsdis =
      dynamic_cast<Discret::Nurbs::NurbsDiscretization*>(&dis);

  if (nurbsdis == nullptr)
  {
    return;
  }

  // Owing to experience a very accurate solution has to be enforced here!
  // Thus, we allocate an own solver with VERY strict tolerance!
  Teuchos::ParameterList p(solverparams);
  const double origtol = p.get<double>("AZTOL");
  const double newtol = 1.0e-11;
  p.set("AZTOL", newtol);

  Teuchos::RCP<Core::LinAlg::Solver> lssolver =
      Teuchos::rcp(new Core::LinAlg::Solver(p, dis.Comm()));
  dis.compute_null_space_if_necessary(lssolver->Params());

  // get the processor ID from the communicator
  const int myrank = dis.Comm().MyPID();

  if (myrank == 0)
    std::cout << "\nSolver tolerance for least squares problem set to " << newtol
              << " (orig: " << origtol << ")";

  apply_nurbs_initial_condition_solve(dis, *lssolver, start_function, initialvals);
  return;
}

FOUR_C_NAMESPACE_CLOSE
