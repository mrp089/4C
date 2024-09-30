/*--------------------------------------------------------------------------*/
/*! \file

\brief Solve FSI problem with sliding grids using a monolithic scheme
with condensed structure interface displacements

\level 2

*/
/*--------------------------------------------------------------------------*/

#include "4C_fsi_slidingmonolithic_structuresplit.hpp"

#include "4C_adapter_str_fsiwrapper.hpp"
#include "4C_ale_utils_mapextractor.hpp"
#include "4C_constraint_manager.hpp"
#include "4C_coupling_adapter.hpp"
#include "4C_coupling_adapter_converter.hpp"
#include "4C_coupling_adapter_mortar.hpp"
#include "4C_fluid_utils_mapextractor.hpp"
#include "4C_fsi_debugwriter.hpp"
#include "4C_fsi_statustest.hpp"
#include "4C_fsi_utils.hpp"
#include "4C_global_data.hpp"
#include "4C_inpar_fsi.hpp"
#include "4C_io.hpp"
#include "4C_io_control.hpp"
#include "4C_linalg_utils_sparse_algebra_create.hpp"
#include "4C_linalg_utils_sparse_algebra_math.hpp"
#include "4C_linear_solver_method_linalg.hpp"
#include "4C_structure_aux.hpp"

#include <Teuchos_StandardParameterEntryValidators.hpp>
#include <Teuchos_TimeMonitor.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
FSI::SlidingMonolithicStructureSplit::SlidingMonolithicStructureSplit(
    const Epetra_Comm& comm, const Teuchos::ParameterList& timeparams)
    : BlockMonolithic(comm, timeparams),
      comm_(comm),
      lambda_(Teuchos::null),
      lambdaold_(Teuchos::null),
      energysum_(0.0)
{
  // ---------------------------------------------------------------------------
  // FSI specific check of Dirichlet boundary conditions
  // ---------------------------------------------------------------------------
  // Create intersection of slave DOFs that hold a Dirichlet boundary condition
  // and are located at the FSI interface
  std::vector<Teuchos::RCP<const Epetra_Map>> intersectionmaps;
  intersectionmaps.push_back(structure_field()->get_dbc_map_extractor()->cond_map());
  intersectionmaps.push_back(structure_field()->interface()->fsi_cond_map());
  Teuchos::RCP<Epetra_Map> intersectionmap =
      Core::LinAlg::MultiMapExtractor::intersect_maps(intersectionmaps);

  // Check whether the intersection is empty
  if (intersectionmap->NumGlobalElements() != 0)
  {
    //    std::cout << "Slave interface nodes with Dirichlet boundary condition "
    //                 "(input file numbering):" << std::endl;
    //    for (int i=0; i < (int)fluid_field()->discretization()->NumMyRowNodes(); i++)
    //    {
    //      // get all nodes and add them
    //      int gid = structure_field()->discretization()->NodeRowMap()->GID(i);
    //
    //      // do only nodes that I have in my discretization
    //      if (!structure_field()->discretization()->NodeColMap()->MyGID(gid)) continue;
    //      Core::Nodes::Node* node = structure_field()->discretization()->gNode(gid);
    //      if (!node) FOUR_C_THROW("Cannot find node with gid %",gid);
    //
    //      std::vector<int> nodedofs = structure_field()->discretization()->Dof(node);
    //
    //      for (int j=0; j < (int)nodedofs.size(); j++)
    //      {
    //        for (int k=0; k < (int)intersectionmap->NumGlobalElements(); k++)
    //        {
    //          if (nodedofs[j] == intersectionmap->GID(k))
    //          {
    //            std::cout << gid+1 << std::endl;
    //            k = (int)intersectionmap->GID(k);
    //            j = (int)nodedofs.size();
    //          }
    //        }
    //      }
    //    }

    // It is not allowed, that slave DOFs at the interface hold a Dirichlet
    // boundary condition. Thus --> Error message

    // We do not have to care whether ALE interface DOFs carry DBCs in the
    // input file since they do not occur in the monolithic system and, hence,
    // do not cause a conflict.

    std::stringstream errormsg;
    errormsg << "  "
                "+---------------------------------------------------------------------------------"
                "------------+"
             << std::endl
             << "  |                DIRICHLET BOUNDARY CONDITIONS ON SLAVE SIDE OF FSI INTERFACE   "
                "              |"
             << std::endl
             << "  "
                "+---------------------------------------------------------------------------------"
                "------------+"
             << std::endl
             << "  | NOTE: The slave side of the interface is not allowed to carry Dirichlet "
                "boundary conditions.|"
             << std::endl
             << "  |                                                                               "
                "              |"
             << std::endl
             << "  | This is a structure split scheme. Hence, master and slave field are chosen as "
                "follows:      |"
             << std::endl
             << "  |     MASTER  = FLUID                                                           "
                "              |"
             << std::endl
             << "  |     SLAVE   = STRUCTURE                                                       "
                "              |"
             << std::endl
             << "  |                                                                               "
                "              |"
             << std::endl
             << "  | Dirichlet boundary conditions were detected on slave interface degrees of "
                "freedom. Please   |"
             << std::endl
             << "  | remove Dirichlet boundary conditions from the slave side of the FSI "
                "interface.              |"
             << std::endl
             << "  | Only the master side of the FSI interface is allowed to carry Dirichlet "
                "boundary conditions.|"
             << std::endl
             << "  "
                "+---------------------------------------------------------------------------------"
                "------------+"
             << std::endl;

    FOUR_C_THROW(errormsg.str());
  }
  // ---------------------------------------------------------------------------

  notsetup_ = true;

  coupsfm_ = Teuchos::rcp(new Coupling::Adapter::CouplingMortar(
      Global::Problem::instance()->n_dim(), Global::Problem::instance()->mortar_coupling_params(),
      Global::Problem::instance()->contact_dynamic_params(),
      Global::Problem::instance()->spatial_approximation_type()));
  fscoupfa_ = Teuchos::rcp(new Coupling::Adapter::Coupling());

  aigtransform_ = Teuchos::rcp(new Coupling::Adapter::MatrixColTransform);
  fmiitransform_ = Teuchos::rcp(new Coupling::Adapter::MatrixColTransform);
  fmgitransform_ = Teuchos::rcp(new Coupling::Adapter::MatrixColTransform);
  fsaigtransform_ = Teuchos::rcp(new Coupling::Adapter::MatrixColTransform);
  fsmgitransform_ = Teuchos::rcp(new Coupling::Adapter::MatrixColTransform);

  set_lambda();
  ddiinc_ = Teuchos::null;
  disiprev_ = Teuchos::null;
  disgprev_ = Teuchos::null;
  sgiprev_ = Teuchos::null;
  sggprev_ = Teuchos::null;

#ifdef FOUR_C_ENABLE_ASSERTIONS
  if (coupsfm_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'coupsfm_' failed.");
  }
  if (fscoupfa_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'fscoupfa_' failed.");
  }
  if (aigtransform_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'aigtransform_' failed.");
  }
  if (fmiitransform_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'fmiitransform_' failed.");
  }
  if (fmgitransform_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'fmgitransform_' failed.");
  }
  if (fsaigtransform_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'fsaigtransform_' failed.");
  }
  if (fsmgitransform_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'fsmgitransform_' failed.");
  }
  if (lambda_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'lambda_' failed.");
  }
  if (lambdaold_ == Teuchos::null)
  {
    FOUR_C_THROW("Allocation of 'lambdaold_' failed.");
  }
#endif

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::set_lambda()
{
  lambda_ = Teuchos::rcp(
      new Core::LinAlg::Vector<double>(*structure_field()->interface()->fsi_cond_map(), true));
  lambdaold_ = Teuchos::rcp(
      new Core::LinAlg::Vector<double>(*structure_field()->interface()->fsi_cond_map(), true));

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::setup_system()
{
  if (notsetup_)
  {
    const Teuchos::ParameterList& fsidyn = Global::Problem::instance()->fsi_dynamic_params();
    const Teuchos::ParameterList& fsimono = fsidyn.sublist("MONOLITHIC SOLVER");
    linearsolverstrategy_ =
        Teuchos::getIntegralValue<Inpar::FSI::LinearBlockSolver>(fsimono, "LINEARBLOCKSOLVER");

    aleproj_ = Teuchos::getIntegralValue<Inpar::FSI::SlideALEProj>(fsidyn, "SLIDEALEPROJ");

    set_default_parameters(fsidyn, nox_parameter_list());

    // we use non-matching meshes at the interface
    // mortar with: structure = slave, fluid = master

    const int ndim = Global::Problem::instance()->n_dim();

    // get coupling objects
    Coupling::Adapter::Coupling& icoupfa = interface_fluid_ale_coupling();

    /* structure to fluid
     * coupling condition at the fsi interface:
     * displacements (=number spatial dimensions) are coupled
     * e.g.: 3D: coupleddof = [1, 1, 1]
     */
    std::vector<int> coupleddof(ndim, 1);

    coupsfm_->setup(fluid_field()->discretization(), structure_field()->discretization(),
        ale_field()->write_access_discretization(), coupleddof, "FSICoupling", comm_,
        Global::Problem::instance()->function_manager(),
        Global::Problem::instance()->binning_strategy_params(),
        Global::Problem::instance()->discretization_map(),
        Global::Problem::instance()->output_control_file(),
        Global::Problem::instance()->spatial_approximation_type(), false);

    // fluid to ale at the interface
    icoupfa.setup_condition_coupling(*fluid_field()->discretization(),
        fluid_field()->interface()->fsi_cond_map(), *ale_field()->discretization(),
        ale_field()->interface()->fsi_cond_map(), "FSICoupling", ndim);

    // the fluid-ale coupling always matches
    const Epetra_Map* fluidnodemap = fluid_field()->discretization()->node_row_map();
    const Epetra_Map* alenodemap = ale_field()->discretization()->node_row_map();

    Coupling::Adapter::Coupling& coupfa = fluid_ale_coupling();

    coupfa.setup_coupling(*fluid_field()->discretization(), *ale_field()->discretization(),
        *fluidnodemap, *alenodemap, ndim);

    fluid_field()->set_mesh_map(coupfa.master_dof_map());

    // create combined map
    create_combined_dof_row_map();

    // Use normal matrix for fluid equations but build (splitted) mesh movement
    // linearization (if requested in the input file)
    fluid_field()->use_block_matrix(false);

    // Use splitted structure matrix
    structure_field()->use_block_matrix();

    // build ale system matrix in splitted system
    ale_field()->create_system_matrix(ale_field()->interface());

    aleresidual_ = Teuchos::rcp(
        new Core::LinAlg::Vector<double>(*fsi_ale_field()->fsi_interface()->other_map()));

    // -------------------------------------------------------------------------
    // Build the global Dirichlet map extractor
    setup_dbc_map_extractor();
    // -------------------------------------------------------------------------

    // enable debugging
    if (fsidyn.get<bool>("DEBUGOUTPUT"))
    {
      pcdbg_ = Teuchos::rcp(new UTILS::MonolithicDebugWriter(*this));
    }

    create_system_matrix();

    // set up sliding ale if necessary
    if (aleproj_ != Inpar::FSI::ALEprojection_none)
    {
      // mesh_init possibly modifies reference configuration of slave side --> recompute element
      // volume in initialize_elements()
      structure_field()->discretization()->fill_complete(false, true, true);
      // set up sliding ale utils
      slideale_ = Teuchos::rcp(new FSI::UTILS::SlideAleUtils(structure_field()->discretization(),
          fluid_field()->discretization(), *coupsfm_, false, aleproj_));

      iprojdisp_ =
          Teuchos::rcp(new Core::LinAlg::Vector<double>(*coupsfm_->master_dof_map(), true));
      iprojdispinc_ =
          Teuchos::rcp(new Core::LinAlg::Vector<double>(*coupsfm_->master_dof_map(), true));
    }
    notsetup_ = false;
  }
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::create_combined_dof_row_map()
{
  std::vector<Teuchos::RCP<const Epetra_Map>> vecSpaces;
  vecSpaces.push_back(structure_field()->interface()->other_map());
  vecSpaces.push_back(fluid_field()->dof_row_map());
  vecSpaces.push_back(fsi_ale_field()->fsi_interface()->other_map());

  if (vecSpaces[0]->NumGlobalElements() == 0)
    FOUR_C_THROW("No inner structural equations. Splitting not possible. Panic.");

  set_dof_row_maps(vecSpaces);

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::setup_dbc_map_extractor()
{
  /* Dirichlet maps for structure and fluid do not intersect with interface map.
   * ALE Dirichlet map might intersect with interface map, but ALE interface
   * DOFs are not part of the final system of equations. Hence, we just need the
   * intersection of inner ALE DOFs with Dirichlet ALE DOFs.
   */
  std::vector<Teuchos::RCP<const Epetra_Map>> aleintersectionmaps;
  aleintersectionmaps.push_back(ale_field()->get_dbc_map_extractor()->cond_map());
  aleintersectionmaps.push_back(fsi_ale_field()->fsi_interface()->other_map());
  Teuchos::RCP<Epetra_Map> aleintersectionmap =
      Core::LinAlg::MultiMapExtractor::intersect_maps(aleintersectionmaps);

  // Merge Dirichlet maps of structure, fluid and ALE to global FSI Dirichlet map
  std::vector<Teuchos::RCP<const Epetra_Map>> dbcmaps;
  dbcmaps.push_back(structure_field()->get_dbc_map_extractor()->cond_map());
  dbcmaps.push_back(fluid_field()->get_dbc_map_extractor()->cond_map());
  dbcmaps.push_back(aleintersectionmap);
  Teuchos::RCP<const Epetra_Map> dbcmap = Core::LinAlg::MultiMapExtractor::merge_maps(dbcmaps);

  // Finally, create the global FSI Dirichlet map extractor
  dbcmaps_ = Teuchos::rcp(new Core::LinAlg::MapExtractor(*dof_row_map(), dbcmap, true));
  if (dbcmaps_ == Teuchos::null) FOUR_C_THROW("Creation of FSI Dirichlet map extractor failed.");

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
Teuchos::RCP<Core::LinAlg::BlockSparseMatrixBase>
FSI::SlidingMonolithicStructureSplit::system_matrix() const
{
  return systemmatrix_;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::setup_rhs_residual(Core::LinAlg::Vector<double>& f)
{
  /* get time integration parameters of structure and fluid time integrators
   * to enable consistent time integration among the fields
   */
  const double stiparam = structure_field()->tim_int_param();
  const double ftiparam = fluid_field()->tim_int_param();

  // some scaling factors for fluid
  const double fluidscale = fluid_field()->residual_scaling();

  // get the Mortar matrix M
  const Teuchos::RCP<Core::LinAlg::SparseMatrix> mortarp = coupsfm_->get_mortar_matrix_p();

  // get single field residuals
  Teuchos::RCP<const Core::LinAlg::Vector<double>> sv =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(*structure_field()->rhs()));
  Teuchos::RCP<const Core::LinAlg::Vector<double>> fv =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(*fluid_field()->rhs()));
  Teuchos::RCP<const Core::LinAlg::Vector<double>> av =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(*ale_field()->rhs()));

  // extract only inner DOFs from structure (=slave) and ALE field
  Teuchos::RCP<const Core::LinAlg::Vector<double>> sov =
      structure_field()->interface()->extract_other_vector(sv);
  Teuchos::RCP<const Core::LinAlg::Vector<double>> aov =
      fsi_ale_field()->fsi_interface()->extract_other_vector(av);

  // add structure interface residual to fluid interface residual considering temporal scaling
  Teuchos::RCP<const Core::LinAlg::Vector<double>> scv =
      structure_field()->interface()->extract_fsi_cond_vector(sv);
  Teuchos::RCP<Core::LinAlg::Vector<double>> fcv =
      Core::LinAlg::create_vector(*fluid_field()->interface()->fsi_cond_map(), true);
  mortarp->multiply(true, *scv, *fcv);
  Teuchos::RCP<Core::LinAlg::Vector<double>> modfv =
      fluid_field()->interface()->insert_fsi_cond_vector(fcv);
  modfv->Update(1.0, *fv, (1.0 - ftiparam) / ((1.0 - stiparam) * fluidscale));

  // put the single field residuals together
  FSI::Monolithic::combine_field_vectors(f, sov, modfv, aov);

  // add additional ale residual
  extractor().add_vector(*aleresidual_, 2, f);

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::setup_rhs_lambda(Core::LinAlg::Vector<double>& f)
{
  if (lambdaold_ != Teuchos::null)
  {
    // get time integration parameters of structure and fluid time integrators
    // to enable consistent time integration among the fields
    const double stiparam = structure_field()->tim_int_param();
    const double ftiparam = fluid_field()->tim_int_param();

    // some scaling factors for fluid
    const double fluidscale = fluid_field()->residual_scaling();

    // get the Mortar matrix M
    Teuchos::RCP<const Core::LinAlg::SparseMatrix> mortarm = coupsfm_->get_mortar_matrix_m();

    // project Lagrange multiplier field onto the master interface DOFs and consider temporal
    // scaling
    Teuchos::RCP<Core::LinAlg::Vector<double>> lambda =
        Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarm->domain_map(), true));
    mortarm->multiply(true, *lambdaold_, *lambda);
    Teuchos::RCP<Core::LinAlg::Vector<double>> lambdafull =
        fluid_field()->interface()->insert_fsi_cond_vector(lambda);
    lambdafull->Scale((-ftiparam + (stiparam * (1.0 - ftiparam)) / (1.0 - stiparam)) / fluidscale);

    // add Lagrange multiplier
    extractor().add_vector(*lambdafull, 1, f);
  }

  return;
}


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::setup_rhs_firstiter(Core::LinAlg::Vector<double>& f)
{
  // get time integration parameters of structure and fluid time integrators
  // to enable consistent time integration among the fields
  const double stiparam = structure_field()->tim_int_param();
  const double ftiparam = fluid_field()->tim_int_param();

  // some scaling factors for fluid
  const double scale = fluid_field()->residual_scaling();

  // old interface velocity of fluid field
  const Teuchos::RCP<const Core::LinAlg::Vector<double>> fveln =
      fluid_field()->extract_interface_veln();

  // get the Mortar projection matrix P = D^{-1} * M
  const Teuchos::RCP<const Core::LinAlg::SparseMatrix> mortarp = coupsfm_->get_mortar_matrix_p();

  // get fluid shape derivatives matrix
  const Teuchos::RCP<const Core::LinAlg::BlockSparseMatrixBase> mmm =
      fluid_field()->shape_derivatives();

  // get structure matrix
  const Teuchos::RCP<const Core::LinAlg::BlockSparseMatrixBase> blocks =
      structure_field()->block_system_matrix();

  // get ale matrix
  const Teuchos::RCP<const Core::LinAlg::BlockSparseMatrixBase> blocka =
      ale_field()->block_system_matrix();

#ifdef FOUR_C_ENABLE_ASSERTIONS
  if (mortarp == Teuchos::null) FOUR_C_THROW("Expected Teuchos::rcp to mortar matrix P.");
  if (blocks == Teuchos::null) FOUR_C_THROW("Expected Teuchos::rcp to structure block matrix.");
  if (blocka == Teuchos::null) FOUR_C_THROW("Expected Teuchos::rcp to ALE block matrix.");
#endif

  // extract submatrices
  const Core::LinAlg::SparseMatrix& sig = blocks->matrix(0, 1);  // S_{I\Gamma}
  const Core::LinAlg::SparseMatrix& sgg = blocks->matrix(1, 1);  // S_{\Gamma\Gamma}
  const Core::LinAlg::SparseMatrix& aig = blocka->matrix(0, 1);  // A_{I\Gamma}

  // some often re-used vectors
  Teuchos::RCP<Core::LinAlg::Vector<double>> rhs =
      Teuchos::null;  // right hand side of single set of DOFs
  Teuchos::RCP<Core::LinAlg::Vector<double>> auxvec = Teuchos::null;  // just for convenience
  Teuchos::RCP<Core::LinAlg::Vector<double>> tmpvec = Teuchos::null;  // just for convenience

  /* Different contributions/terms to the rhs are separated by the following
   * comment line */
  // ---------- inner structure DOFs
  /* The following terms are added to the inner structure DOFs of right hand side:
   *
   * rhs_firstnewtonstep =
   *
   * (1)  - dt * S_{I \Gamma} * P * u^{n}_{\Gamma}
   *
   * (2)  + S_{I \Gamma} * \Delta d_{\Gamma,p}
   *
   * Remarks on all terms:
   * +  tau: time scaling factor for interface time integration
   *         (tau = 1/fluid_field()->TimeScaling())
   *
   */
  // ----------addressing term 1
  rhs = Teuchos::rcp(new Core::LinAlg::Vector<double>(sig.range_map(), true));
  auxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarp->range_map(), true));

  mortarp->Apply(*fveln, *auxvec);
  sig.Apply(*auxvec, *rhs);

  rhs->Scale(-dt());

  extractor().add_vector(*rhs, 0, f);
  // ----------end of term 1

  // ----------addressing term 2
  rhs = Teuchos::rcp(new Core::LinAlg::Vector<double>(sig.range_map(), true));

  sig.Apply(*ddgpred_, *rhs);

  extractor().add_vector(*rhs, 0, f);
  // ----------end of term 2
  // ----------end of inner structure DOFs

  // ---------- inner fluid DOFs
  /* The following terms are added to the inner fluid DOFs of right hand side:
   *
   * rhs_firstnewtonstep =
   *
   * (1)  - dt * F^{G}_{I \Gamma} * u^{n}_{\Gamma}
   *
   */
  // ----------addressing term 1
  if (mmm != Teuchos::null)
  {
    // extract F^{G}_{I \Gamma}
    const Core::LinAlg::SparseMatrix& fmig = mmm->matrix(0, 1);

    rhs = Teuchos::rcp(new Core::LinAlg::Vector<double>(fmig.range_map(), true));

    fmig.Apply(*fveln, *rhs);

    rhs->Scale(-dt());
    rhs = fsi_fluid_field()->fsi_interface()->insert_other_vector(rhs);

    extractor().add_vector(*rhs, 1, f);
  }
  // ----------end of term 1
  // ----------end of inner fluid DOFs

  // ---------- interface fluid DOFs
  /* The following terms are added to the interface fluid DOFs of right hand side:
   *
   * rhs_firstnewtonstep =
   *
   * (1)  - dt * F^{G}_{\Gamma\Gamma} * u^{n}_{\Gamma}
   *
   * (2)  - (1-ftiparam) / (1-stiparam) * dt * P^{T} * S_{\Gamma\Gamma} * P * u^{n}_{\Gamma}
   *
   * (3)  + (1-ftiparam) / (1-stiparam) * P^{T} * S_{\Gamma\Gamma} * \Delta d_{\Gamma,p}
   *
   * Remarks on all terms:
   * +  tau: time scaling factor for interface time integration (tau =
   * 1/fluid_field()->TimeScaling())
   *
   */
  // ----------addressing term 1
  if (mmm != Teuchos::null)
  {
    // extract F^{G}_{\Gamma\Gamma}
    const Core::LinAlg::SparseMatrix& fmgg = mmm->matrix(1, 1);

    rhs = Teuchos::rcp(new Core::LinAlg::Vector<double>(fmgg.range_map(), true));

    fmgg.Apply(*fveln, *rhs);

    rhs->Scale(-dt());
    rhs = fluid_field()->interface()->insert_fsi_cond_vector(rhs);

    extractor().add_vector(*rhs, 1, f);
  }
  // ----------end of term 1

  // ----------addressing term 2
  rhs = Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarp->domain_map(), true));
  auxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(sgg.range_map(), true));
  tmpvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarp->range_map(), true));

  mortarp->Apply(*fveln, *tmpvec);
  sgg.Apply(*tmpvec, *auxvec);
  mortarp->multiply(true, *auxvec, *rhs);

  rhs->Scale(-(1. - ftiparam) / (1. - stiparam) * dt() / scale);
  rhs = fluid_field()->interface()->insert_fsi_cond_vector(rhs);

  extractor().add_vector(*rhs, 1, f);
  // ----------end of term 2

  // ----------addressing term 3
  rhs = Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarp->domain_map(), true));
  auxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(sgg.range_map(), true));

  sgg.Apply(*ddgpred_, *auxvec);
  mortarp->multiply(true, *auxvec, *rhs);

  rhs->Scale((1. - ftiparam) / (1. - stiparam) / scale);
  rhs = fluid_field()->interface()->insert_fsi_cond_vector(rhs);

  extractor().add_vector(*rhs, 1, f);
  // ----------end of term 3
  // ----------end of interface fluid DOFs

  // ---------- inner ALE DOFs
  /* The following terms are added to the inner ALE DOFs of right hand side:
   *
   * rhs_firstnewtonstep =
   *
   * (1)  - dt * A_{I \Gamma} * u^{n}_{\Gamma}
   *
   */
  // ----------addressing term 1
  rhs = Teuchos::rcp(new Core::LinAlg::Vector<double>(aig.range_map(), true));

  aig.Apply(*fluid_to_ale_interface(fveln), *rhs);

  rhs->Scale(-dt());

  extractor().add_vector(*rhs, 2, f);
  // ----------end of term 1
  // ----------end of inner ALE DOFs

  // only if relative movement between ale and structure is possible
  if (aleproj_ != Inpar::FSI::ALEprojection_none)
  {
    rhs = Teuchos::rcp(new Core::LinAlg::Vector<double>(aig.row_map(), true));

    aig.Apply(*fluid_to_ale_interface(iprojdispinc_), *rhs);

    extractor().add_vector(*rhs, 2, f);
  }

  /* Reset quantities for previous iteration step since they still store values
   * from the last time step */
  ddiinc_ = Core::LinAlg::create_vector(*structure_field()->interface()->other_map(), true);
  disiprev_ = Teuchos::null;
  disgprev_ = Teuchos::null;
  sgicur_ = Teuchos::null;
  sggcur_ = Teuchos::null;

  return;
}


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::setup_system_matrix(
    Core::LinAlg::BlockSparseMatrixBase& mat)
{
  TEUCHOS_FUNC_TIME_MONITOR("FSI::SlidingMonolithicStructureSplit::setup_system_matrix");

  // get the Mortar projection matrix P = D^{-1} * M
  Teuchos::RCP<Core::LinAlg::SparseMatrix> mortarp = coupsfm_->get_mortar_matrix_p();

  // get single field block matrices
  const Teuchos::RCP<Core::LinAlg::BlockSparseMatrixBase> s =
      structure_field()->block_system_matrix();
  const Teuchos::RCP<Core::LinAlg::SparseMatrix> f = fluid_field()->system_matrix();
  const Teuchos::RCP<Core::LinAlg::BlockSparseMatrixBase> a = ale_field()->block_system_matrix();


#ifdef FOUR_C_ENABLE_ASSERTIONS
  // check whether allocation was successful
  if (mortarp == Teuchos::null) FOUR_C_THROW("Expected Teuchos::rcp to mortar matrix P.");
  if (s == Teuchos::null)
  {
    FOUR_C_THROW("expect structure block matrix");
  }
  if (f == Teuchos::null)
  {
    FOUR_C_THROW("expect fluid matrix");
  }
  if (a == Teuchos::null)
  {
    FOUR_C_THROW("expect ale block matrix");
  }

  // some checks whether maps for matrix-matrix-multiplication do really match
  if (!s->matrix(0, 1).domain_map().PointSameAs(mortarp->range_map()))
    FOUR_C_THROW("Maps do not match.");
  if (!s->matrix(1, 0).range_map().PointSameAs(mortarp->range_map()))
    FOUR_C_THROW("Maps do not match.");
  if (!s->matrix(1, 1).domain_map().PointSameAs(mortarp->range_map()))
    FOUR_C_THROW("Maps do not match.");
#endif

  // extract submatrices
  Core::LinAlg::SparseMatrix& aii = a->matrix(0, 0);
  const Core::LinAlg::SparseMatrix& aig = a->matrix(0, 1);

  // scaling factors for fluid
  const double scale = fluid_field()->residual_scaling();
  const double timescale = fluid_field()->time_scaling();

  // get time integration parameters of structure and fluid time integrators
  // to enable consistent time integration among the fields
  const double stiparam = structure_field()->tim_int_param();
  const double ftiparam = fluid_field()->tim_int_param();

  // Uncomplete fluid matrix to be able to deal with slightly defective
  // interface meshes.
  f->un_complete();

  // ---------------------------------------------------------------------------
  // BEGIN building the global 4x4 system matrix
  // ---------------------------------------------------------------------------
  // Contributions to blocks in system matrix are listed separately.
  // Block numbering in comments ranges from (1,1) to (4,4).

  mat.assign(0, 0, Core::LinAlg::View, s->matrix(0, 0));

  // ----------Addressing contribution to block (1,3)
  Teuchos::RCP<Core::LinAlg::SparseMatrix> sig =
      matrix_multiply(s->matrix(0, 1), false, *mortarp, false, false, false, true);
  Teuchos::RCP<Core::LinAlg::SparseMatrix> lsig =
      Teuchos::rcp(new Core::LinAlg::SparseMatrix(sig->row_map(), 81, false));

  lsig->add(*sig, false, 1. / timescale, 0.0);
  lsig->complete(f->domain_map(), sig->range_map());

  mat.assign(0, 1, Core::LinAlg::View, *lsig);

  // ----------Addressing contribution to block (3,1)
  Teuchos::RCP<Core::LinAlg::SparseMatrix> sgi =
      matrix_multiply(*mortarp, true, s->matrix(1, 0), false, false, false, true);
  Teuchos::RCP<Core::LinAlg::SparseMatrix> lsgi =
      Teuchos::rcp(new Core::LinAlg::SparseMatrix(f->row_map(), 81, false));

  lsgi->add(*sgi, false, (1. - ftiparam) / ((1. - stiparam) * scale), 0.0);
  lsgi->complete(sgi->domain_map(), f->range_map());

  mat.assign(1, 0, Core::LinAlg::View, *lsgi);

  // ----------Addressing contribution to block (3,3)
  Teuchos::RCP<Core::LinAlg::SparseMatrix> sgg =
      matrix_multiply(s->matrix(1, 1), false, *mortarp, false, false, false, true);
  sgg = matrix_multiply(*mortarp, true, *sgg, false, false, false, true);

  f->add(*sgg, false, (1. - ftiparam) / ((1. - stiparam) * scale * timescale), 1.0);
  mat.assign(1, 1, Core::LinAlg::View, *f);

  (*aigtransform_)(a->full_row_map(), a->full_col_map(), aig, 1. / timescale,
      Coupling::Adapter::CouplingSlaveConverter(interface_fluid_ale_coupling()), mat.matrix(2, 1));
  mat.assign(2, 2, Core::LinAlg::View, aii);

  /*--------------------------------------------------------------------------*/
  // add optional fluid linearization with respect to mesh motion block

  Teuchos::RCP<Core::LinAlg::BlockSparseMatrixBase> mmm = fluid_field()->shape_derivatives();
  if (mmm != Teuchos::null)
  {
    // extract submatrices
    const Core::LinAlg::SparseMatrix& fmii = mmm->matrix(0, 0);
    const Core::LinAlg::SparseMatrix& fmig = mmm->matrix(0, 1);
    const Core::LinAlg::SparseMatrix& fmgi = mmm->matrix(1, 0);
    const Core::LinAlg::SparseMatrix& fmgg = mmm->matrix(1, 1);

    // ----------Addressing contribution to block (3,3)
    mat.matrix(1, 1).add(fmgg, false, 1. / timescale, 1.0);

    // ----------Addressing contribution to block (2,3)
    mat.matrix(1, 1).add(fmig, false, 1. / timescale, 1.0);

    const Coupling::Adapter::Coupling& coupfa = fluid_ale_coupling();

    (*fmgitransform_)(mmm->full_row_map(), mmm->full_col_map(), fmgi, 1.,
        Coupling::Adapter::CouplingMasterConverter(coupfa), mat.matrix(1, 2), false, false);

    (*fmiitransform_)(mmm->full_row_map(), mmm->full_col_map(), fmii, 1.,
        Coupling::Adapter::CouplingMasterConverter(coupfa), mat.matrix(1, 2), false, true);
  }

  // done. make sure all blocks are filled.
  mat.complete();

  // Finally, take care of Dirichlet boundary conditions
  mat.apply_dirichlet(*(dbcmaps_->cond_map()), true);
  //
  // ---------------------------------------------------------------------------
  // END building the global system matrix
  // ---------------------------------------------------------------------------

  // store parts of structural matrix to know them in the next iteration as previous iteration
  // matrices
  sgiprev_ = sgicur_;
  sggprev_ = sggcur_;
  sgicur_ = Teuchos::rcp(new Core::LinAlg::SparseMatrix(s->matrix(1, 0)));
  sggcur_ = Teuchos::rcp(new Core::LinAlg::SparseMatrix(s->matrix(1, 1)));
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::update()
{
  lambdaold_->Update(1.0, *lambda_, 0.0);

  // update history variables for sliding ale
  if (aleproj_ != Inpar::FSI::ALEprojection_none)
  {
    iprojdisp_ = Teuchos::rcp(new Core::LinAlg::Vector<double>(*coupsfm_->master_dof_map(), true));
    Teuchos::RCP<Core::LinAlg::Vector<double>> idispale = ale_to_fluid_interface(
        ale_field()->interface()->extract_fsi_cond_vector(ale_field()->dispnp()));

    slideale_->remeshing(*structure_field(), fluid_field()->discretization(), idispale, iprojdisp_,
        *coupsfm_, get_comm());

    iprojdispinc_->Update(-1.0, *iprojdisp_, 1.0, *idispale, 0.0);

    slideale_->evaluate_mortar(
        structure_field()->extract_interface_dispnp(), iprojdisp_, *coupsfm_);
    slideale_->evaluate_fluid_mortar(idispale, iprojdisp_);

    Teuchos::RCP<Core::LinAlg::Vector<double>> temp =
        Teuchos::rcp(new Core::LinAlg::Vector<double>(*iprojdisp_));
    temp->ReplaceMap(idispale->Map());
    Teuchos::RCP<Core::LinAlg::Vector<double>> acx = fluid_to_ale_interface(temp);
    ale_field()->apply_interface_displacements(acx);
    fluid_field()->apply_mesh_displacement(ale_to_fluid(ale_field()->dispnp()));

    Teuchos::RCP<Core::LinAlg::Vector<double>> unew =
        slideale_->interpolate_fluid(fluid_field()->extract_interface_velnp());
    fluid_field()->apply_interface_velocities(unew);
  }

  // call update()-routine in base class to handle the single fields
  FSI::BlockMonolithic::update();
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::scale_system(
    Core::LinAlg::BlockSparseMatrixBase& mat, Core::LinAlg::Vector<double>& b)
{
  const Teuchos::ParameterList& fsidyn = Global::Problem::instance()->fsi_dynamic_params();
  const Teuchos::ParameterList& fsimono = fsidyn.sublist("MONOLITHIC SOLVER");
  const bool scaling_infnorm = fsimono.get<bool>("INFNORMSCALING");

  if (scaling_infnorm)
  {
    // The matrices are modified here. Do we have to change them back later on?

    // do scaling of structure rows
    Teuchos::RCP<Epetra_CrsMatrix> A = mat.matrix(0, 0).epetra_matrix();
    srowsum_ = Teuchos::rcp(new Core::LinAlg::Vector<double>(A->RowMap(), false));
    scolsum_ = Teuchos::rcp(new Core::LinAlg::Vector<double>(A->RowMap(), false));
    A->InvRowSums(*srowsum_->get_ptr_of_Epetra_Vector());
    A->InvColSums(*scolsum_->get_ptr_of_Epetra_Vector());
    if (A->LeftScale(*srowsum_) or A->RightScale(*scolsum_) or
        mat.matrix(0, 1).epetra_matrix()->LeftScale(*srowsum_) or
        mat.matrix(0, 2).epetra_matrix()->LeftScale(*srowsum_) or
        mat.matrix(1, 0).epetra_matrix()->RightScale(*scolsum_) or
        mat.matrix(2, 0).epetra_matrix()->RightScale(*scolsum_))
      FOUR_C_THROW("structure scaling failed");

    // do scaling of ale rows
    A = mat.matrix(2, 2).epetra_matrix();
    arowsum_ = Teuchos::rcp(new Core::LinAlg::Vector<double>(A->RowMap(), false));
    acolsum_ = Teuchos::rcp(new Core::LinAlg::Vector<double>(A->RowMap(), false));
    A->InvRowSums(*arowsum_->get_ptr_of_Epetra_Vector());
    A->InvColSums(*acolsum_->get_ptr_of_Epetra_Vector());
    if (A->LeftScale(*arowsum_) or A->RightScale(*acolsum_) or
        mat.matrix(2, 0).epetra_matrix()->LeftScale(*arowsum_) or
        mat.matrix(2, 1).epetra_matrix()->LeftScale(*arowsum_) or
        mat.matrix(0, 2).epetra_matrix()->RightScale(*acolsum_) or
        mat.matrix(1, 2).epetra_matrix()->RightScale(*acolsum_))
      FOUR_C_THROW("ale scaling failed");

    // do scaling of structure and ale rhs vectors
    Teuchos::RCP<Core::LinAlg::Vector<double>> sx = extractor().extract_vector(b, 0);
    Teuchos::RCP<Core::LinAlg::Vector<double>> ax = extractor().extract_vector(b, 2);

    if (sx->Multiply(1.0, *srowsum_, *sx, 0.0)) FOUR_C_THROW("structure scaling failed");
    if (ax->Multiply(1.0, *arowsum_, *ax, 0.0)) FOUR_C_THROW("ale scaling failed");

    extractor().insert_vector(*sx, 0, b);
    extractor().insert_vector(*ax, 2, b);
  }
}


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::unscale_solution(
    Core::LinAlg::BlockSparseMatrixBase& mat, Core::LinAlg::Vector<double>& x,
    Core::LinAlg::Vector<double>& b)
{
  const Teuchos::ParameterList& fsidyn = Global::Problem::instance()->fsi_dynamic_params();
  const Teuchos::ParameterList& fsimono = fsidyn.sublist("MONOLITHIC SOLVER");
  const bool scaling_infnorm = fsimono.get<bool>("INFNORMSCALING");

  if (scaling_infnorm)
  {
    Teuchos::RCP<Core::LinAlg::Vector<double>> sy = extractor().extract_vector(x, 0);
    Teuchos::RCP<Core::LinAlg::Vector<double>> ay = extractor().extract_vector(x, 2);

    if (sy->Multiply(1.0, *scolsum_, *sy, 0.0)) FOUR_C_THROW("structure scaling failed");
    if (ay->Multiply(1.0, *acolsum_, *ay, 0.0)) FOUR_C_THROW("ale scaling failed");

    extractor().insert_vector(*sy, 0, x);
    extractor().insert_vector(*ay, 2, x);

    Teuchos::RCP<Core::LinAlg::Vector<double>> sx = extractor().extract_vector(b, 0);
    Teuchos::RCP<Core::LinAlg::Vector<double>> ax = extractor().extract_vector(b, 2);

    if (sx->ReciprocalMultiply(1.0, *srowsum_, *sx, 0.0)) FOUR_C_THROW("structure scaling failed");
    if (ax->ReciprocalMultiply(1.0, *arowsum_, *ax, 0.0)) FOUR_C_THROW("ale scaling failed");

    extractor().insert_vector(*sx, 0, b);
    extractor().insert_vector(*ax, 2, b);

    Teuchos::RCP<Epetra_CrsMatrix> A = mat.matrix(0, 0).epetra_matrix();
    srowsum_->Reciprocal(*srowsum_);
    scolsum_->Reciprocal(*scolsum_);
    if (A->LeftScale(*srowsum_) or A->RightScale(*scolsum_) or
        mat.matrix(0, 1).epetra_matrix()->LeftScale(*srowsum_) or
        mat.matrix(0, 2).epetra_matrix()->LeftScale(*srowsum_) or
        mat.matrix(1, 0).epetra_matrix()->RightScale(*scolsum_) or
        mat.matrix(2, 0).epetra_matrix()->RightScale(*scolsum_))
      FOUR_C_THROW("structure scaling failed");

    A = mat.matrix(2, 2).epetra_matrix();
    arowsum_->Reciprocal(*arowsum_);
    acolsum_->Reciprocal(*acolsum_);
    if (A->LeftScale(*arowsum_) or A->RightScale(*acolsum_) or
        mat.matrix(2, 0).epetra_matrix()->LeftScale(*arowsum_) or
        mat.matrix(2, 1).epetra_matrix()->LeftScale(*arowsum_) or
        mat.matrix(0, 2).epetra_matrix()->RightScale(*acolsum_) or
        mat.matrix(1, 2).epetra_matrix()->RightScale(*acolsum_))
      FOUR_C_THROW("ale scaling failed");
  }

  // very simple hack just to see the linear solution

  Core::LinAlg::Vector<double> r(b.Map());
  mat.Apply(x, r);
  r.Update(1., b, 1.);

  Teuchos::RCP<Core::LinAlg::Vector<double>> sr = extractor().extract_vector(r, 0);
  Teuchos::RCP<Core::LinAlg::Vector<double>> fr = extractor().extract_vector(r, 1);
  Teuchos::RCP<Core::LinAlg::Vector<double>> ar = extractor().extract_vector(r, 2);

  // increment additional ale residual
  aleresidual_->Update(-1., *ar, 0.);

  std::ios_base::fmtflags flags = utils()->out().flags();

  double n, ns, nf, na;
  r.Norm2(&n);
  sr->Norm2(&ns);
  fr->Norm2(&nf);
  ar->Norm2(&na);
  utils()->out() << std::scientific << "\nlinear solver quality:\n"
                 << "L_2-norms:\n"
                 << "   |r|=" << n << "   |rs|=" << ns << "   |rf|=" << nf << "   |ra|=" << na
                 << "\n";
  r.NormInf(&n);
  sr->NormInf(&ns);
  fr->NormInf(&nf);
  ar->NormInf(&na);
  utils()->out() << "L_inf-norms:\n"
                 << "   |r|=" << n << "   |rs|=" << ns << "   |rf|=" << nf << "   |ra|=" << na
                 << "\n";

  utils()->out().flags(flags);
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
Teuchos::RCP<::NOX::StatusTest::Combo> FSI::SlidingMonolithicStructureSplit::create_status_test(
    Teuchos::ParameterList& nlParams, Teuchos::RCP<::NOX::Epetra::Group> grp)
{
  // ---------------------------------------------------------------------------
  // Setup the test framework
  // ---------------------------------------------------------------------------
  // Create the top-level test combo
  Teuchos::RCP<::NOX::StatusTest::Combo> combo =
      Teuchos::rcp(new ::NOX::StatusTest::Combo(::NOX::StatusTest::Combo::OR));

  // Create test combo for convergence of residuals and iterative increments
  Teuchos::RCP<::NOX::StatusTest::Combo> converged =
      Teuchos::rcp(new ::NOX::StatusTest::Combo(::NOX::StatusTest::Combo::AND));

  // Create some other plausibility tests
  Teuchos::RCP<::NOX::StatusTest::MaxIters> maxiters =
      Teuchos::rcp(new ::NOX::StatusTest::MaxIters(nlParams.get<int>("Max Iterations")));
  Teuchos::RCP<::NOX::StatusTest::FiniteValue> fv =
      Teuchos::rcp(new ::NOX::StatusTest::FiniteValue);

  // Add single tests to the top-level test combo
  combo->addStatusTest(fv);
  combo->addStatusTest(converged);
  combo->addStatusTest(maxiters);

  // Start filling the 'converged' combo here
  // require one solve
  converged->addStatusTest(Teuchos::rcp(new NOX::FSI::MinIters(1)));


  // ---------------------------------------------------------------------------
  // setup tests for structural displacement field
  // ---------------------------------------------------------------------------
  // create ::NOX::StatusTest::Combo for structural displacement field
  Teuchos::RCP<::NOX::StatusTest::Combo> structcombo =
      Teuchos::rcp(new ::NOX::StatusTest::Combo(::NOX::StatusTest::Combo::AND));

  // create Norm-objects for each norm that has to be tested
  Teuchos::RCP<NOX::FSI::PartialNormF> structureDisp_L2 = Teuchos::rcp(new NOX::FSI::PartialNormF(
      "DISPL residual", extractor(), 0, nlParams.get<double>("Tol dis res L2"),
      ::NOX::Abstract::Vector::TwoNorm, NOX::FSI::PartialNormF::Scaled));
  Teuchos::RCP<NOX::FSI::PartialNormF> structureDisp_inf = Teuchos::rcp(new NOX::FSI::PartialNormF(
      "DISPL residual", extractor(), 0, nlParams.get<double>("Tol dis res Inf"),
      ::NOX::Abstract::Vector::MaxNorm, NOX::FSI::PartialNormF::Unscaled));
  Teuchos::RCP<NOX::FSI::PartialNormUpdate> structureDispUpdate_L2 =
      Teuchos::rcp(new NOX::FSI::PartialNormUpdate("DISPL update", extractor(), 0,
          nlParams.get<double>("Tol dis inc L2"), ::NOX::Abstract::Vector::TwoNorm,
          NOX::FSI::PartialNormUpdate::Scaled));
  Teuchos::RCP<NOX::FSI::PartialNormUpdate> structureDispUpdate_inf =
      Teuchos::rcp(new NOX::FSI::PartialNormUpdate("DISPL update", extractor(), 0,
          nlParams.get<double>("Tol dis inc Inf"), ::NOX::Abstract::Vector::MaxNorm,
          NOX::FSI::PartialNormUpdate::Unscaled));

  // tests needed to adapt relative tolerance of the linear solver
  add_status_test(structureDisp_L2);

  // add norm-tests to structural displacement ::NOX::StatusTest::Combo
  structcombo->addStatusTest(structureDisp_L2);
  structcombo->addStatusTest(structureDisp_inf);
  structcombo->addStatusTest(structureDispUpdate_L2);
  structcombo->addStatusTest(structureDispUpdate_inf);

  // add structural displacement test combo to top-level test combo
  converged->addStatusTest(structcombo);
  // ---------- end of structural displacement field tests

  // ---------------------------------------------------------------------------
  // setup tests for interface
  // ---------------------------------------------------------------------------
  // build mapextractor
  std::vector<Teuchos::RCP<const Epetra_Map>> interface;
  interface.push_back(fluid_field()->interface()->fsi_cond_map());
  interface.push_back(Teuchos::null);
  Core::LinAlg::MultiMapExtractor interfaceextract(*dof_row_map(), interface);

  // create ::NOX::StatusTest::Combo for interface
  Teuchos::RCP<::NOX::StatusTest::Combo> interfacecombo =
      Teuchos::rcp(new ::NOX::StatusTest::Combo(::NOX::StatusTest::Combo::AND));

  // create Norm-objects for each norm that has to be tested
  Teuchos::RCP<NOX::FSI::PartialNormF> interfaceTest_L2 = Teuchos::rcp(new NOX::FSI::PartialNormF(
      "GAMMA residual", interfaceextract, 0, nlParams.get<double>("Tol fsi res L2"),
      ::NOX::Abstract::Vector::TwoNorm, NOX::FSI::PartialNormF::Scaled));
  Teuchos::RCP<NOX::FSI::PartialNormF> interfaceTest_inf = Teuchos::rcp(new NOX::FSI::PartialNormF(
      "GAMMA residual", interfaceextract, 0, nlParams.get<double>("Tol fsi res Inf"),
      ::NOX::Abstract::Vector::MaxNorm, NOX::FSI::PartialNormF::Unscaled));
  Teuchos::RCP<NOX::FSI::PartialNormUpdate> interfaceTestUpdate_L2 =
      Teuchos::rcp(new NOX::FSI::PartialNormUpdate("GAMMA update", interfaceextract, 0,
          nlParams.get<double>("Tol fsi inc L2"), ::NOX::Abstract::Vector::TwoNorm,
          NOX::FSI::PartialNormUpdate::Scaled));
  Teuchos::RCP<NOX::FSI::PartialNormUpdate> interfaceTestUpdate_inf =
      Teuchos::rcp(new NOX::FSI::PartialNormUpdate("GAMMA update", interfaceextract, 0,
          nlParams.get<double>("Tol fsi inc Inf"), ::NOX::Abstract::Vector::MaxNorm,
          NOX::FSI::PartialNormUpdate::Unscaled));

  // tests needed to adapt relative tolerance of the linear solver
  add_status_test(interfaceTest_L2);

  // add norm-tests to interface ::NOX::StatusTest::Combo
  interfacecombo->addStatusTest(interfaceTest_L2);
  interfacecombo->addStatusTest(interfaceTest_inf);
  interfacecombo->addStatusTest(interfaceTestUpdate_L2);
  interfacecombo->addStatusTest(interfaceTestUpdate_inf);

  // add interface test combo to top-level test combo
  converged->addStatusTest(interfacecombo);
  // ---------- end of interface tests

  // ---------------------------------------------------------------------------
  // setup tests for fluid velocity field
  // ---------------------------------------------------------------------------
  // build mapextractor
  std::vector<Teuchos::RCP<const Epetra_Map>> fluidvel;
  fluidvel.push_back(fluid_field()->inner_velocity_row_map());
  fluidvel.push_back(Teuchos::null);
  Core::LinAlg::MultiMapExtractor fluidvelextract(*dof_row_map(), fluidvel);

  // create ::NOX::StatusTest::Combo for fluid velocity field
  Teuchos::RCP<::NOX::StatusTest::Combo> fluidvelcombo =
      Teuchos::rcp(new ::NOX::StatusTest::Combo(::NOX::StatusTest::Combo::AND));

  // create Norm-objects for each norm that has to be tested
  Teuchos::RCP<NOX::FSI::PartialNormF> innerFluidVel_L2 = Teuchos::rcp(new NOX::FSI::PartialNormF(
      "VELOC residual", fluidvelextract, 0, nlParams.get<double>("Tol vel res L2"),
      ::NOX::Abstract::Vector::TwoNorm, NOX::FSI::PartialNormF::Scaled));
  Teuchos::RCP<NOX::FSI::PartialNormF> innerFluidVel_inf = Teuchos::rcp(new NOX::FSI::PartialNormF(
      "VELOC residual", fluidvelextract, 0, nlParams.get<double>("Tol vel res Inf"),
      ::NOX::Abstract::Vector::MaxNorm, NOX::FSI::PartialNormF::Unscaled));
  Teuchos::RCP<NOX::FSI::PartialNormUpdate> innerFluidVelUpdate_L2 =
      Teuchos::rcp(new NOX::FSI::PartialNormUpdate("VELOC update", fluidvelextract, 0,
          nlParams.get<double>("Tol vel inc L2"), ::NOX::Abstract::Vector::TwoNorm,
          NOX::FSI::PartialNormUpdate::Scaled));
  Teuchos::RCP<NOX::FSI::PartialNormUpdate> innerFluidVelUpdate_inf =
      Teuchos::rcp(new NOX::FSI::PartialNormUpdate("VELOC update", fluidvelextract, 0,
          nlParams.get<double>("Tol vel inc Inf"), ::NOX::Abstract::Vector::MaxNorm,
          NOX::FSI::PartialNormUpdate::Unscaled));

  // tests needed to adapt relative tolerance of the linear solver
  add_status_test(innerFluidVel_L2);

  // add norm-tests to fluid velocity ::NOX::StatusTest::Combo
  fluidvelcombo->addStatusTest(innerFluidVel_L2);
  fluidvelcombo->addStatusTest(innerFluidVel_inf);
  fluidvelcombo->addStatusTest(innerFluidVelUpdate_L2);
  fluidvelcombo->addStatusTest(innerFluidVelUpdate_inf);

  // add fluid velocity test combo to top-level test combo
  converged->addStatusTest(fluidvelcombo);
  // ---------- end of fluid velocity field tests

  // ---------------------------------------------------------------------------
  // setup tests for fluid pressure field
  // ---------------------------------------------------------------------------
  // build mapextractor
  std::vector<Teuchos::RCP<const Epetra_Map>> fluidpress;
  fluidpress.push_back(fluid_field()->pressure_row_map());
  fluidpress.push_back(Teuchos::null);
  Core::LinAlg::MultiMapExtractor fluidpressextract(*dof_row_map(), fluidpress);

  // create ::NOX::StatusTest::Combo for fluid pressure field
  Teuchos::RCP<::NOX::StatusTest::Combo> fluidpresscombo =
      Teuchos::rcp(new ::NOX::StatusTest::Combo(::NOX::StatusTest::Combo::AND));

  // create Norm-objects for each norm that has to be tested
  Teuchos::RCP<NOX::FSI::PartialNormF> fluidPress_L2 = Teuchos::rcp(new NOX::FSI::PartialNormF(
      "PRESS residual", fluidpressextract, 0, nlParams.get<double>("Tol pre res L2"),
      ::NOX::Abstract::Vector::TwoNorm, NOX::FSI::PartialNormF::Scaled));
  Teuchos::RCP<NOX::FSI::PartialNormF> fluidPress_inf = Teuchos::rcp(new NOX::FSI::PartialNormF(
      "PRESS residual", fluidpressextract, 0, nlParams.get<double>("Tol pre res Inf"),
      ::NOX::Abstract::Vector::MaxNorm, NOX::FSI::PartialNormF::Unscaled));
  Teuchos::RCP<NOX::FSI::PartialNormUpdate> fluidPressUpdate_L2 =
      Teuchos::rcp(new NOX::FSI::PartialNormUpdate("PRESS update", fluidpressextract, 0,
          nlParams.get<double>("Tol pre inc L2"), ::NOX::Abstract::Vector::TwoNorm,
          NOX::FSI::PartialNormUpdate::Scaled));
  Teuchos::RCP<NOX::FSI::PartialNormUpdate> fluidPressUpdate_inf =
      Teuchos::rcp(new NOX::FSI::PartialNormUpdate("PRESS update", fluidpressextract, 0,
          nlParams.get<double>("Tol pre inc Inf"), ::NOX::Abstract::Vector::MaxNorm,
          NOX::FSI::PartialNormUpdate::Unscaled));

  // tests needed to adapt relative tolerance of the linear solver
  add_status_test(fluidPress_L2);

  // add norm-tests to fluid pressure ::NOX::StatusTest::Combo
  fluidpresscombo->addStatusTest(fluidPress_L2);
  fluidpresscombo->addStatusTest(fluidPress_inf);
  fluidpresscombo->addStatusTest(fluidPressUpdate_L2);
  fluidpresscombo->addStatusTest(fluidPressUpdate_inf);

  // add fluid pressure test combo to top-level test combo
  converged->addStatusTest(fluidpresscombo);
  // ---------- end of fluid pressure field tests

  // Finally, return the test combo
  return combo;
}


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::extract_field_vectors(
    Teuchos::RCP<const Core::LinAlg::Vector<double>> x,
    Teuchos::RCP<const Core::LinAlg::Vector<double>>& sx,
    Teuchos::RCP<const Core::LinAlg::Vector<double>>& fx,
    Teuchos::RCP<const Core::LinAlg::Vector<double>>& ax)
{
  TEUCHOS_FUNC_TIME_MONITOR("FSI::SlidingMonolithicStructureSplit::extract_field_vectors");

#ifdef FOUR_C_ENABLE_ASSERTIONS
  if (ddgpred_ == Teuchos::null)
    FOUR_C_THROW("Vector 'ddgpred_' has not been initialized properly.");
#endif

  // get the Mortar projection matrix P = D^{-1} * M
  const Teuchos::RCP<const Core::LinAlg::SparseMatrix> mortarp = coupsfm_->get_mortar_matrix_p();

  // ---------------------------------------------------------------------------
  // process fluid unknowns
  // ---------------------------------------------------------------------------
  // extract fluid solution increment from NOX increment
  Teuchos::RCP<Core::LinAlg::Vector<double>> f = extractor().extract_vector(x, 1);
  fluid_field()->update_slave_dof(f);
  fx = f;

  // ---------------------------------------------------------------------------
  // process ale unknowns
  // ---------------------------------------------------------------------------
  // extract inner ALE solution increment from NOX increment
  Teuchos::RCP<const Core::LinAlg::Vector<double>> aox = extractor().extract_vector(x, 2);

  // convert fluid interface velocities into ALE interface displacements
  Teuchos::RCP<Core::LinAlg::Vector<double>> fcx =
      fluid_field()->interface()->extract_fsi_cond_vector(fx);
  fluid_field()->velocity_to_displacement(fcx);
  Teuchos::RCP<Core::LinAlg::Vector<double>> acx = fluid_to_ale_interface(fcx);

  // put inner and interface ALE solution increments together
  Teuchos::RCP<Core::LinAlg::Vector<double>> a =
      fsi_ale_field()->fsi_interface()->insert_other_vector(aox);
  ale_field()->interface()->insert_fsi_cond_vector(acx, a);
  ale_field()->update_slave_dof(a);
  ax = a;

  // ---------------------------------------------------------------------------
  // process structure unknowns
  // ---------------------------------------------------------------------------
  // extract inner structure solution increment from NOX increment
  Teuchos::RCP<const Core::LinAlg::Vector<double>> sox = extractor().extract_vector(x, 0);

  // convert ALE interface displacements to structure interface displacements
  Teuchos::RCP<Core::LinAlg::Vector<double>> scx =
      Core::LinAlg::create_vector(*structure_field()->interface()->fsi_cond_map());
  acx = ale_to_fluid_interface(acx);
  mortarp->Apply(*acx, *scx);
  scx->Update(-1.0, *ddgpred_, 1.0);

  // put inner and interface structure solution increments together
  Teuchos::RCP<Core::LinAlg::Vector<double>> s =
      structure_field()->interface()->insert_other_vector(sox);
  structure_field()->interface()->insert_fsi_cond_vector(scx, s);
  sx = s;

  // ---------------------------------------------------------------------------

  // Store field vectors to know them later on as previous quantities
  if (disiprev_ != Teuchos::null)
    ddiinc_->Update(1.0, *sox, -1.0, *disiprev_, 0.0);  // compute current iteration increment
  else
    ddiinc_ = Teuchos::rcp(new Core::LinAlg::Vector<double>(*sox));  // first iteration increment

  disiprev_ = sox;  // store current step increment

  if (velgprev_ != Teuchos::null)
    duginc_->Update(1.0, *fcx, -1.0, *velgprev_, 0.0);  // compute current iteration increment
  else
    duginc_ = Teuchos::rcp(new Core::LinAlg::Vector<double>(*fcx));  // first iteration increment

  velgprev_ = fcx;  // store current step increment
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::output()
{
  structure_field()->output();

  // output Lagrange multiplier
  output_lambda();

  fluid_field()->output();

  if (aleproj_ != Inpar::FSI::ALEprojection_none)
  {
    int uprestart = timeparams_.get<int>("RESTARTEVRY");
    if (uprestart != 0 && fluid_field()->step() % uprestart == 0)
    {
      fluid_field()->disc_writer()->write_vector("slideALE", iprojdisp_);
      fluid_field()->disc_writer()->write_vector("slideALEincr", iprojdispinc_);
      slideale_->output_restart(*fluid_field()->disc_writer());
    }
  }
  ale_field()->output();

  if (structure_field()->get_constraint_manager()->have_monitor())
  {
    structure_field()->get_constraint_manager()->compute_monitor_values(
        structure_field()->dispnp());
    if (comm_.MyPID() == 0) structure_field()->get_constraint_manager()->print_monitor_values();
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::output_lambda()
{
  /* 'lambda_' is only defined on the interface. So, insert 'lambda_' into

   * output or restart data.
   */
  Teuchos::RCP<Core::LinAlg::Vector<double>> lambdafull =
      structure_field()->interface()->insert_fsi_cond_vector(lambda_);
  const int uprestart = timeparams_.get<int>("RESTARTEVRY");
  const int upres = timeparams_.get<int>("RESULTSEVRY");
  if ((uprestart != 0 && fluid_field()->step() % uprestart == 0) ||
      fluid_field()->step() % upres == 0)
    structure_field()->disc_writer()->write_vector("fsilambda", lambdafull);
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::read_restart(int step)
{
  auto input_control_file = Global::Problem::instance()->input_control_file();

  // read Lagrange multiplier
  {
    Teuchos::RCP<Core::LinAlg::Vector<double>> lambdafull =
        Teuchos::rcp(new Core::LinAlg::Vector<double>(*structure_field()->dof_row_map(), true));
    Core::IO::DiscretizationReader reader = Core::IO::DiscretizationReader(
        structure_field()->discretization(), input_control_file, step);
    reader.read_vector(lambdafull, "fsilambda");
    lambdaold_ = structure_field()->interface()->extract_fsi_cond_vector(lambdafull);
    // Note: the above is normally enough. However, we can use the restart in order to periodically
    // repeat the fsi simulation (see AC-FS3I)
    lambda_ = structure_field()->interface()->extract_fsi_cond_vector(lambdafull);
  }

  structure_field()->read_restart(step);
  fluid_field()->read_restart(step);

  setup_system();

  if (aleproj_ != Inpar::FSI::ALEprojection_none)
  {
    Core::IO::DiscretizationReader reader =
        Core::IO::DiscretizationReader(fluid_field()->discretization(), input_control_file, step);
    reader.read_vector(iprojdisp_, "slideALE");
    reader.read_vector(iprojdispinc_, "slideALEincr");
    slideale_->read_restart(reader);
  }

  ale_field()->read_restart(step);

  set_time_step(fluid_field()->time(), fluid_field()->step());

  if (aleproj_ != Inpar::FSI::ALEprojection_none)
    slideale_->evaluate_mortar(structure_field()->extract_interface_dispn(), iprojdisp_, *coupsfm_);
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::recover_lagrange_multiplier()
{
  // get time integration parameter of structural time integrator
  // to enable consistent time integration among the fields
  const double stiparam = structure_field()->tim_int_param();

  // some scaling factors for fluid
  //  const double timescale = fluid_field()->TimeScaling();

  // get the Mortar projection matrix P = D^{-1} * M
  const Teuchos::RCP<Core::LinAlg::SparseMatrix> mortarp = coupsfm_->get_mortar_matrix_p();

  // get the inverted Mortar matrix D^{-1}
  const Teuchos::RCP<Core::LinAlg::SparseMatrix> mortardinv = coupsfm_->get_mortar_matrix_dinv();

#ifdef FOUR_C_ENABLE_ASSERTIONS
  if (mortarp == Teuchos::null) FOUR_C_THROW("Expected Teuchos::rcp to mortar matrix P.");
  if (mortardinv == Teuchos::null) FOUR_C_THROW("Expected Teuchos::rcp to mortar matrix D^{-1}.");
#endif

  // some often re-used vectors
  Teuchos::RCP<Core::LinAlg::Vector<double>> tmpvec =
      Teuchos::null;  // stores intermediate result of terms (3)-(8)
  Teuchos::RCP<Core::LinAlg::Vector<double>> auxvec = Teuchos::null;     // just for convenience
  Teuchos::RCP<Core::LinAlg::Vector<double>> auxauxvec = Teuchos::null;  // just for convenience

  /* Recovery of Lagrange multiplier lambda^{n+1} is done by the following
   * condensation expression:
   *
   * lambda^{n+1} =
   *
   * (1)  - stiparam / (1.-stiparam) * lambda^{n}
   *
   * (2)  + 1. / (1.-stiparam) * D^{-T} * tmpvec
   *
   * with tmpvec =
   *
   * (3)    r_{\Gamma}^{S,n+1}
   *
   * (4)  + S_{\Gamma I} * \Delta d_{I}^{S,n+1}
   *
   * (5)  + tau * S_{\Gamma\Gamma} * P * \Delta u_{\Gamma}^{F,n+1}
   *
   * (6)  + dt * S_{\Gamma\Gamma} * P * u_{\Gamma}^n]
   *
   * Remark on term (6):
   * Term (6) has to be considered only in the first Newton iteration.
   * Hence, it will usually not be computed since in general we need more
   * than one nonlinear iteration until convergence.
   *
   * Remarks on all terms:
   * +  Division by -(1.0 - stiparam) will be done in the end
   *    since this is common to all terms
   * +  tau: time scaling factor for interface time integration (tau =
   * 1/fluid_field()->TimeScaling())
   * +  neglecting terms (4)-(6) should not alter the results significantly
   *    since at the end of the time step the solution increments tend to zero.
   *
   *                                                 Matthias Mayr (10/2012)
   */

  // ---------Addressing term (1)
  lambda_->Update(-stiparam, *lambdaold_, 0.0);
  // ---------End of term (1)

  // ---------Addressing term (3)
  Teuchos::RCP<Core::LinAlg::Vector<double>> structureresidual =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(
          *structure_field()->interface()->extract_fsi_cond_vector(structure_field()->rhs())));
  structureresidual->Scale(-1.0);  // invert sign to obtain residual, not rhs
  tmpvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(*structureresidual));
  // ---------End of term (3)

  /* You might want to comment out terms (4) to (6) since they tend to
   * introduce oscillations in the Lagrange multiplier field for certain
   * material properties of the structure.
   *                                                    Matthias Mayr 11/2012
  // ---------Addressing term (4)
  auxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(sgiprev_->RangeMap(),true));
  sgiprev_->Apply(*ddiinc_,*auxvec);
  tmpvec->Update(1.0,*auxvec,1.0);
  // ---------End of term (4)

  // ---------Addressing term (5)
  auxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarp->RangeMap(),true));
  mortarp->Apply(*duginc_,*auxvec);
  auxauxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(sggprev_->RangeMap(),true));
  sggprev_->Apply(*auxvec,*auxauxvec);
  tmpvec->Update(1.0/timescale,*auxauxvec,1.0);
  // ---------End of term (5)

  // ---------Addressing term (6)
  if (firstcall_)
  {
    auxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarp->RangeMap(),true));
    mortarp->Apply(*fluid_field()->extract_interface_veln(),*auxvec);
    auxauxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(sggprev_->RangeMap(),true));
    sggprev_->Apply(*auxvec,*auxauxvec);
    tmpvec->Update(Dt(),*auxauxvec,1.0);
  }
  // ---------End of term (6)
   *
   */

  // ---------Addressing term (2)
  auxvec = Teuchos::rcp(new Core::LinAlg::Vector<double>(mortardinv->domain_map(), true));
  mortardinv->multiply(true, *tmpvec, *auxvec);
  lambda_->Update(1.0, *auxvec, 1.0);
  // ---------End of term (2)

  // finally, divide by -(1.-stiparam) which is common to all terms
  lambda_->Scale(1. / (1.0 - stiparam));

  /* Finally, the Lagrange multiplier lambda_ is recovered here. It has the
   * unit [N/m^2]. Actual nodal forces are obtained by multiplication with
   * mortar matrices M or D later on.
   */

  //  check_kinematic_constraint();
  //  check_dynamic_equilibrium();

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::calculate_interface_energy_increment()
{
  // get time integration parameters of structure and fluid time integrators
  // to enable consistent time integration among the fields
  const double stiparam = structure_field()->tim_int_param();
  const double ftiparam = fluid_field()->tim_int_param();

  // interface traction weighted by time integration factors
  Teuchos::RCP<Core::LinAlg::Vector<double>> tractionstructure =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(lambda_->Map(), true));
  tractionstructure->Update(stiparam - ftiparam, *lambdaold_, ftiparam - stiparam, *lambda_, 0.0);

  // displacement increment of this time step
  Teuchos::RCP<Core::LinAlg::Vector<double>> deltad =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(*structure_field()->dof_row_map(), true));
  deltad->Update(1.0, *structure_field()->dispnp(), -1.0, *structure_field()->dispn(), 0.0);

  // calculate the energy increment
  double energy = 0.0;
  tractionstructure->Dot(*structure_field()->interface()->extract_fsi_cond_vector(deltad), &energy);

  energysum_ += energy;

  write_interface_energy_file(energy, energysum_);

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::check_kinematic_constraint()
{
  // some scaling factors for fluid
  const double timescale = fluid_field()->time_scaling();

  // get the Mortar matrices D and M
  const Teuchos::RCP<Core::LinAlg::SparseMatrix> mortard = coupsfm_->get_mortar_matrix_d();
  const Teuchos::RCP<Core::LinAlg::SparseMatrix> mortarm = coupsfm_->get_mortar_matrix_m();

  // get interface displacements and velocities
  Teuchos::RCP<Core::LinAlg::Vector<double>> disnp = structure_field()->extract_interface_dispnp();
  Teuchos::RCP<Core::LinAlg::Vector<double>> disn = structure_field()->extract_interface_dispn();
  Teuchos::RCP<Core::LinAlg::Vector<double>> velnp = fluid_field()->extract_interface_velnp();
  Teuchos::RCP<Core::LinAlg::Vector<double>> veln = fluid_field()->extract_interface_veln();

  // prepare vectors for projected interface quantities
  Teuchos::RCP<Core::LinAlg::Vector<double>> disnpproj =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(mortard->range_map(), true));
  Teuchos::RCP<Core::LinAlg::Vector<double>> disnproj =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(mortard->range_map(), true));
  Teuchos::RCP<Core::LinAlg::Vector<double>> velnpproj =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarm->range_map(), true));
  Teuchos::RCP<Core::LinAlg::Vector<double>> velnproj =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarm->range_map(), true));

  // projection of interface displacements
  mortard->Apply(*disnp, *disnpproj);
  mortard->Apply(*disn, *disnproj);

  // projection of interface velocities
  mortarm->Apply(*velnp, *velnpproj);
  mortarm->Apply(*veln, *velnproj);

  // calculate violation of kinematic interface constraint
  Teuchos::RCP<Core::LinAlg::Vector<double>> violation =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(*disnpproj));
  violation->Update(-1.0, *disnproj, 1.0);
  violation->Update(-1.0 / timescale, *velnpproj, 1.0 / timescale, *velnproj, 1.0);
  violation->Update(-dt(), *velnproj, 1.0);

  // calculate some norms
  double violationl2 = 0.0;
  double violationinf = 0.0;
  violation->Norm2(&violationl2);
  violation->NormInf(&violationinf);

  // scale L2-Norm with length of vector
  violationl2 /= sqrt(violation->MyLength());

  // output to screen
  std::ios_base::fmtflags flags = utils()->out().flags();

  utils()->out() << std::scientific << "\nViolation of kinematic interface constraint:\n"
                 << "L_2-norm: " << violationl2 << "        L_inf-norm: " << violationinf << "\n";
  utils()->out().flags(flags);

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::check_dynamic_equilibrium()
{
  // get the Mortar matrices D and M
  const Teuchos::RCP<Core::LinAlg::SparseMatrix> mortard = coupsfm_->get_mortar_matrix_d();
  const Teuchos::RCP<Core::LinAlg::SparseMatrix> mortarm = coupsfm_->get_mortar_matrix_m();

#ifdef FOUR_C_ENABLE_ASSERTIONS
  if (mortarm == Teuchos::null) FOUR_C_THROW("Expected Teuchos::rcp to mortar matrix M.");
  if (mortard == Teuchos::null) FOUR_C_THROW("Expected Teuchos::rcp to mortar matrix D.");
#endif

  // auxiliary vectors
  Teuchos::RCP<Core::LinAlg::Vector<double>> tractionmaster =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(mortarm->domain_map(), true));
  Teuchos::RCP<Core::LinAlg::Vector<double>> tractionslave =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(mortard->domain_map(), true));

  // calculate forces on master and slave side
  mortarm->multiply(true, *lambda_, *tractionmaster);
  mortard->multiply(true, *lambda_, *tractionslave);

  // calculate violation of dynamic equilibrium
  Teuchos::RCP<Core::LinAlg::Vector<double>> violation =
      Teuchos::rcp(new Core::LinAlg::Vector<double>(*tractionmaster));
  violation->Update(-1.0, *tractionslave, 1.0);

  // calculate some norms
  double violationl2 = 0.0;
  double violationinf = 0.0;
  violation->Norm2(&violationl2);
  violation->NormInf(&violationinf);

  // scale L2-Norm with sqrt of length of interface vector
  violationl2 /= sqrt(structure_field()->interface()->fsi_cond_map()->NumGlobalElements());

  // output to screen
  std::ios_base::fmtflags flags = utils()->out().flags();

  utils()->out() << std::scientific << "\nViolation of dynamic interface equilibrium:\n"
                 << "L_2-norm: " << violationl2 << "        L_inf-norm: " << violationinf << "\n";
  utils()->out().flags(flags);

  return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::combine_field_vectors(Core::LinAlg::Vector<double>& v,
    Teuchos::RCP<const Core::LinAlg::Vector<double>> sv,
    Teuchos::RCP<const Core::LinAlg::Vector<double>> fv,
    Teuchos::RCP<const Core::LinAlg::Vector<double>> av,
    const bool slave_vectors_contain_interface_dofs)
{
  if (slave_vectors_contain_interface_dofs)
  {
    // extract inner DOFs from slave vectors
    Teuchos::RCP<Core::LinAlg::Vector<double>> sov =
        structure_field()->interface()->extract_other_vector(sv);
    Teuchos::RCP<Core::LinAlg::Vector<double>> aov =
        fsi_ale_field()->fsi_interface()->extract_other_vector(av);

    // put them together
    FSI::Monolithic::combine_field_vectors(v, sov, fv, aov);
  }
  else
    FSI::Monolithic::combine_field_vectors(v, sv, fv, av);
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
double FSI::SlidingMonolithicStructureSplit::select_dt_error_based() const
{
  // get time step size suggestions based on some error norms
  const double dtfl = get_ada_fl_dt();               // based on all fluid DOFs
  const double dtflfsi = get_ada_fl_fsi_dt();        // based on fluid FSI DOFs
  const double dtstrinner = get_ada_str_inner_dt();  // based on inner structural DOFs

  double dt = SlidingMonolithicStructureSplit::dt();

  // select time step size based on error estimation
  if (is_ada_structure() and is_ada_fluid())
    dt = std::min(std::min(dtfl, dtflfsi), dtstrinner);
  else if (is_ada_structure() and (not is_ada_fluid()))
    dt = dtstrinner;
  else if ((not is_ada_structure()) and is_ada_fluid())
    dt = std::min(dtfl, dtflfsi);
  else
  {
    // no change in time step size based on structure or fluid field error estimation
  }

  return dt;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
bool FSI::SlidingMonolithicStructureSplit::set_accepted() const
{
  // get error norms
  const double flnorm = get_ada_flnorm();               // based on all fluid DOFs
  const double flfsinorm = get_ada_fl_fs_inorm();       // based on fluid FSI DOFs
  const double strinnernorm = get_ada_str_innernorm();  // based on inner structural DOFs

  bool accepted = std::max(flnorm, flfsinorm) < errtolfl_ && strinnernorm < errtolstr_;

  // in case error estimation in the fluid field is turned off:
  if (not is_ada_fluid()) accepted = strinnernorm < errtolstr_;

  // in case error estimation in the structure field is turned off:
  if (not is_ada_structure()) accepted = std::max(flnorm, flfsinorm) < errtolfl_;

  // no error based time adaptivity
  if ((not is_ada_structure()) and (not is_ada_fluid())) accepted = true;

  return accepted;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void FSI::SlidingMonolithicStructureSplit::create_system_matrix()
{
  FSI::BlockMonolithic::create_system_matrix(systemmatrix_, true);
}

FOUR_C_NAMESPACE_CLOSE
