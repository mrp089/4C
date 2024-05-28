/*----------------------------------------------------------------------*/
/*! \file

\brief internal implementation of thermo boundary elements (ThermoBoundary)

\level 1

 */

/*----------------------------------------------------------------------*
 | headers                                                   dano 09/09 |
 *----------------------------------------------------------------------*/
#include "4C_thermo_ele_boundary_impl.hpp"

#include "4C_discretization_fem_general_extract_values.hpp"
#include "4C_discretization_fem_general_utils_boundary_integration.hpp"
#include "4C_discretization_fem_general_utils_fem_shapefunctions.hpp"
#include "4C_discretization_fem_general_utils_nurbs_shapefunctions.hpp"
#include "4C_discretization_geometry_position_array.hpp"
#include "4C_global_data.hpp"
#include "4C_inpar_thermo.hpp"
#include "4C_lib_discret.hpp"
#include "4C_nurbs_discret.hpp"
#include "4C_thermo_ele_action.hpp"
#include "4C_utils_function.hpp"
#include "4C_utils_function_of_time.hpp"

FOUR_C_NAMESPACE_OPEN


/*----------------------------------------------------------------------*
 |                                                           dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::TemperBoundaryImplInterface* DRT::ELEMENTS::TemperBoundaryImplInterface::Impl(
    const DRT::Element* ele)
{
  // we assume here, that numdofpernode is equal for every node within
  // the discretization and does not change during the computations
  const int numdofpernode = ele->NumDofPerNode(*(ele->Nodes()[0]));

  switch (ele->Shape())
  {
    case CORE::FE::CellType::quad4:
    {
      static TemperBoundaryImpl<CORE::FE::CellType::quad4>* cp4;
      if (cp4 == nullptr) cp4 = new TemperBoundaryImpl<CORE::FE::CellType::quad4>(numdofpernode);
      return cp4;
    }
    case CORE::FE::CellType::quad8:
    {
      static TemperBoundaryImpl<CORE::FE::CellType::quad8>* cp8;
      if (cp8 == nullptr) cp8 = new TemperBoundaryImpl<CORE::FE::CellType::quad8>(numdofpernode);
      return cp8;
    }
    case CORE::FE::CellType::quad9:
    {
      static TemperBoundaryImpl<CORE::FE::CellType::quad9>* cp9;
      if (cp9 == nullptr) cp9 = new TemperBoundaryImpl<CORE::FE::CellType::quad9>(numdofpernode);
      return cp9;
    }
    case CORE::FE::CellType::nurbs9:
    {
      static TemperBoundaryImpl<CORE::FE::CellType::nurbs9>* cpn9;
      if (cpn9 == nullptr) cpn9 = new TemperBoundaryImpl<CORE::FE::CellType::nurbs9>(numdofpernode);
      return cpn9;
    }
    case CORE::FE::CellType::tri3:
    {
      static TemperBoundaryImpl<CORE::FE::CellType::tri3>* cp3;
      if (cp3 == nullptr) cp3 = new TemperBoundaryImpl<CORE::FE::CellType::tri3>(numdofpernode);
      return cp3;
    }
    /*  case CORE::FE::CellType::tri6:
    {
      static TemperBoundaryImpl<CORE::FE::CellType::tri6>* cp6;
      if (cp6 == nullptr)
        cp6 = new TemperBoundaryImpl<CORE::FE::CellType::tri6>(numdofpernode);
      return cp6;
    }*/
    case CORE::FE::CellType::line2:
    {
      static TemperBoundaryImpl<CORE::FE::CellType::line2>* cl2;
      if (cl2 == nullptr) cl2 = new TemperBoundaryImpl<CORE::FE::CellType::line2>(numdofpernode);
      return cl2;
    } /*
     case CORE::FE::CellType::line3:
     {
       static TemperBoundaryImpl<CORE::FE::CellType::line3>* cl3;
       if (cl3 == nullptr)
         cl3 = new TemperBoundaryImpl<CORE::FE::CellType::line3>(numdofpernode);
       return cl3;
     }*/
    default:
      FOUR_C_THROW("Shape %d (%d nodes) not supported", ele->Shape(), ele->num_node());
      break;
  }
  return nullptr;
}  // Impl()


/*----------------------------------------------------------------------*
 |                                                           dano 09/09 |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
DRT::ELEMENTS::TemperBoundaryImpl<distype>::TemperBoundaryImpl(int numdofpernode)
    : numdofpernode_(numdofpernode),
      xyze_(true),
      xsi_(true),
      funct_(true),
      deriv_(true),
      derxy_(true),
      normal_(true),
      fac_(0.0),
      normalfac_(1.0)
{
  return;
}  // TemperBoundaryImpl()


/*----------------------------------------------------------------------*
 |                                                           dano 09/09 |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
int DRT::ELEMENTS::TemperBoundaryImpl<distype>::Evaluate(const DRT::ELEMENTS::ThermoBoundary* ele,
    Teuchos::ParameterList& params, const DRT::Discretization& discretization,
    const DRT::Element::LocationArray& la, CORE::LINALG::SerialDenseMatrix& elemat1_epetra,
    CORE::LINALG::SerialDenseMatrix& elemat2_epetra,
    CORE::LINALG::SerialDenseVector& elevec1_epetra,
    CORE::LINALG::SerialDenseVector& elevec2_epetra,
    CORE::LINALG::SerialDenseVector& elevec3_epetra)
{
  // what actions are available
  // ( action=="calc_thermo_fextconvection" )
  // ( action=="calc_thermo_fextconvection_coupltang" )
  // ( action=="calc_normal_vectors" )
  // ( action=="ba_integrate_shape_functions" )

  // prepare nurbs
  prepare_nurbs_eval(ele, discretization);

  // First, do the things that are needed for all actions:
  // get the material (of the parent element)
  DRT::Element* genericparent = ele->parent_element();
  // make sure the static cast below is really valid
  FOUR_C_ASSERT(dynamic_cast<DRT::ELEMENTS::Thermo*>(genericparent) != nullptr,
      "Parent element is no fluid element");
  DRT::ELEMENTS::Thermo* parentele = static_cast<DRT::ELEMENTS::Thermo*>(genericparent);
  Teuchos::RCP<CORE::MAT::Material> mat = parentele->Material();

  // Now, check for the action parameter
  const THR::BoundaryAction action = CORE::UTILS::GetAsEnum<THR::BoundaryAction>(params, "action");
  if (action == THR::calc_normal_vectors)
  {
    // access the global vector
    const Teuchos::RCP<Epetra_MultiVector> normals =
        params.get<Teuchos::RCP<Epetra_MultiVector>>("normal vectors", Teuchos::null);
    if (normals == Teuchos::null) FOUR_C_THROW("Could not access vector 'normal vectors'");

    // get node coordinates (we have a nsd_+1 dimensional domain!)
    CORE::GEO::fillInitialPositionArray<distype, nsd_ + 1, CORE::LINALG::Matrix<nsd_ + 1, nen_>>(
        ele, xyze_);

    // determine constant normal to this element
    get_const_normal(normal_, xyze_);

    // loop over the boundary element nodes
    for (int j = 0; j < nen_; j++)
    {
      const int nodegid = (ele->Nodes()[j])->Id();
      if (normals->Map().MyGID(nodegid))
      {  // OK, the node belongs to this processor

        // scaling to a unit vector is performed on the global level after
        // assembly of nodal contributions since we have no reliable information
        // about the number of boundary elements adjacent to a node
        for (int dim = 0; dim < (nsd_ + 1); dim++)
        {
          normals->SumIntoGlobalValue(nodegid, dim, normal_(dim));
        }
      }
      // else: the node belongs to another processor; the ghosted
      //       element will contribute the right value on that proc
    }
  }

  else if (action == THR::ba_integrate_shape_functions)
  {
    // NOTE: add area value only for elements which are NOT ghosted!
    const bool addarea = (ele->Owner() == discretization.Comm().MyPID());
    integrate_shape_functions(ele, params, elevec1_epetra, addarea);
  }

  // surface heat transfer boundary condition q^_c = h (T - T_infty)
  else if (action == THR::calc_thermo_fextconvection)
  {
    // get node coordinates ( (nsd_+1): domain, nsd_: boundary )
    CORE::GEO::fillInitialPositionArray<distype, nsd_ + 1, CORE::LINALG::Matrix<nsd_ + 1, nen_>>(
        ele, xyze_);

    // set views, here we assemble on the boundary dofs only!
    CORE::LINALG::Matrix<nen_, nen_> etang(elemat1_epetra.values(), true);  // view only!
    CORE::LINALG::Matrix<nen_, 1> efext(elevec1_epetra.values(), true);     // view only!

    // get current condition
    Teuchos::RCP<CORE::Conditions::Condition> cond =
        params.get<Teuchos::RCP<CORE::Conditions::Condition>>("condition");
    if (cond == Teuchos::null) FOUR_C_THROW("Cannot access condition 'ThermoConvections'");

    // access parameters of the condition
    const std::string* tempstate = &cond->parameters().Get<std::string>("temperature state");
    double coeff = cond->parameters().Get<double>("coeff");
    const int curvenum = cond->parameters().Get<int>("funct");
    const double time = params.get<double>("total time");

    // get surrounding temperature T_infty from input file
    double surtemp = cond->parameters().Get<double>("surtemp");
    // increase the surrounding temperature T_infty step by step
    // can be scaled with a time curve, get time curve number from input file
    const int surtempcurvenum = cond->parameters().Get<int>("surtempfunct");

    // find out whether we shall use a time curve for q^_c and get the factor
    double curvefac = 1.0;
    if (curvenum >= 0)
    {
      curvefac =
          GLOBAL::Problem::Instance()->FunctionById<CORE::UTILS::FunctionOfTime>(curvenum).Evaluate(
              time);
    }
    // multiply heat convection coefficient with the timecurve factor
    coeff *= curvefac;

    // we can increase or decrease the surrounding (fluid) temperature T_oo
    // enabling for instance a load cycle due to combustion of a fluid
    double surtempcurvefac = 1.0;
    // find out whether we shall use a time curve for T_oo and get the factor
    if (surtempcurvenum >= 0)
    {
      surtempcurvefac = GLOBAL::Problem::Instance()
                            ->FunctionById<CORE::UTILS::FunctionOfTime>(surtempcurvenum)
                            .Evaluate(time);
    }
    // complete surrounding temperatures T_oo: multiply with the timecurve factor
    surtemp *= surtempcurvefac;

    // use current temperature T_{n+1} of current time step for heat convection
    // boundary condition
    if (*tempstate == "Tempnp")
    {
      // disassemble temperature
      if (discretization.HasState("temperature"))
      {
        // get actual values of temperature from global location vector
        std::vector<double> mytempnp((la[0].lm_).size());
        Teuchos::RCP<const Epetra_Vector> tempnp = discretization.GetState("temperature");
        if (tempnp == Teuchos::null) FOUR_C_THROW("Cannot get state vector 'tempnp'");

        CORE::FE::ExtractMyValues(*tempnp, mytempnp, la[0].lm_);
        // build the element temperature
        CORE::LINALG::Matrix<nen_, 1> etemp(mytempnp.data(), true);  // view only!
        etemp_.Update(etemp);                                        // copy
      }  // discretization.HasState("temperature")
      else
        FOUR_C_THROW("No old temperature T_n+1 available");
    }
    // use temperature T_n of last known time step t_n
    else if (*tempstate == "Tempn")
    {
      if (discretization.HasState("old temperature"))
      {
        // get actual values of temperature from global location vector
        std::vector<double> mytempn((la[0].lm_).size());
        Teuchos::RCP<const Epetra_Vector> tempn = discretization.GetState("old temperature");
        if (tempn == Teuchos::null) FOUR_C_THROW("Cannot get state vector 'tempn'");

        CORE::FE::ExtractMyValues(*tempn, mytempn, la[0].lm_);
        // build the element temperature
        CORE::LINALG::Matrix<nen_, 1> etemp(mytempn.data(), true);  // view only!
        etemp_.Update(etemp);                                       // copy
      }  // discretization.HasState("old temperature")
      else
        FOUR_C_THROW("No old temperature T_n available");
    }
    else
      FOUR_C_THROW("Unknown type of convection boundary condition");

#ifdef THRASOUTPUT
    if (ele->Id() == 0)
    {
      std::cout << "ele Id= " << ele->Id() << std::endl;
      // print all parameters read from the current condition
      std::cout << "type of boundary condition  = " << *tempstate << std::endl;
      std::cout << "heat convection coefficient = " << coeff << std::endl;
      std::cout << "surrounding temperature     = " << surtemp << std::endl;
      std::cout << "time curve                  = " << curvenum << std::endl;
      std::cout << "total time                  = " << time << std::endl;
    }
#endif

    // get kinematic type from parent element
    INPAR::STR::KinemType kintype = parentele->kintype_;

    // ------------------------------------------------------ default
    // ------------ purely thermal / geometrically linear TSI problem
    if (kintype == INPAR::STR::KinemType::linear)  // geo_linear
    {
      // and now check if there is a convection heat transfer boundary condition
      calculate_convection_fint_cond(ele,  // current boundary element
          &etang,                          // element-matrix
          &efext,                          // element-rhs
          coeff, surtemp, *tempstate);
    }  // geo_linear

    // initialise the vectors
    // Evaluate() is called the first time in ThermoBaseAlgorithm: at this stage
    // the coupling field is not yet known. Pass coupling vectors filled with zeros
    // the size of the vectors is the length of the location vector*nsd_
    std::vector<double> mydisp(((la[0].lm_).size()) * nsd_, 0.0);

    // -------------------------- geometrically nonlinear TSI problem

    // if it's a TSI problem with displacementcoupling_ --> go on here!
    if ((kintype == INPAR::STR::KinemType::nonlinearTotLag) and (la.Size() > 1))  // geo_nonlinear
    {
      // set views, here we assemble on the boundary dofs only!
      CORE::LINALG::Matrix<nen_, (nsd_ + 1) * nen_> etangcoupl(
          elemat2_epetra.values(), true);  // view only!

      // and now get the current displacements/velocities
      if (discretization.HasState(1, "displacement"))
      {
        // get the displacements
        Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState(1, "displacement");
        if (disp == Teuchos::null) FOUR_C_THROW("Cannot get state vectors 'displacement'");
        // extract the displacements
        CORE::FE::ExtractMyValues(*disp, mydisp, la[1].lm_);

        // and now check if there is a convection heat transfer boundary condition
        calculate_nln_convection_fint_cond(ele,  // current boundary element
            mydisp,
            &etang,   // element-matrix k_TT
            nullptr,  // coupling matrix k_Td
            &efext,   // element-rhs
            coeff, surtemp, *tempstate);

      }  // disp!=0
    }    // (la.Size() > 1) and (kintype == INPAR::STR::KinemType::nonlinearTotLag)

    // BUILD EFFECTIVE TANGENT AND RESIDUAL ACC TO TIME INTEGRATOR
    // check the time integrator
    const INPAR::THR::DynamicType timint = CORE::UTILS::GetAsEnum<INPAR::THR::DynamicType>(
        params, "time integrator", INPAR::THR::dyna_undefined);
    switch (timint)
    {
      case INPAR::THR::dyna_statics:
      {
        if (*tempstate == "Tempn")
          FOUR_C_THROW("Old temperature T_n is not allowed with static time integrator");
        // continue
        break;
      }
      case INPAR::THR::dyna_onesteptheta:
      {
        // Note: efext is scaled with theta in thrtimint_ost.cpp. Because the
        // convective boundary condition is nonlinear and produces a term in the
        // tangent, consider the factor theta here, too
        const double theta = params.get<double>("theta");
        // combined tangent and conductivity matrix to one global matrix
        etang.Scale(theta);
        break;
      }
      case INPAR::THR::dyna_genalpha:
      {
        const double alphaf = params.get<double>("alphaf");
        // combined tangent and conductivity matrix to one global matrix
        etang.Scale(alphaf);
        break;
      }
      case INPAR::THR::dyna_undefined:
      default:
      {
        FOUR_C_THROW("Don't know what to do...");
        break;
      }
    }  // end of switch(timint)
  }    // calc_thermo_fextconvection

  // only contribution in case of geometrically nonlinear analysis
  // evaluate coupling matrix k_Td for surface heat transfer boundary condition
  // q^_c da = h (T - T_infty) da with da(u)
  // --> k_Td: d(da(u))/du
  else if (action == THR::calc_thermo_fextconvection_coupltang)
  {
    // -------------------------- geometrically nonlinear TSI problem

    // get kinematic type from parent element
    INPAR::STR::KinemType kintype = parentele->kintype_;

    // initialise the vectors
    // Evaluate() is called the first time in ThermoBaseAlgorithm: at this stage
    // the coupling field is not yet known. Pass coupling vectors filled with zeros
    // the size of the vectors is the length of the location vector*nsd_
    std::vector<double> mydisp(((la[0].lm_).size()) * nsd_, 0.0);

    // -------------------------- geometrically nonlinear TSI problem

    // if it's a TSI problem with displacementcoupling_ --> go on here!
    if ((kintype == INPAR::STR::KinemType::nonlinearTotLag) and (la.Size() > 1))  // geo_nonlinear
    {
      // and now get the current displacements/velocities
      if (discretization.HasState(1, "displacement"))
      {
        // get node coordinates (nsd_+1: domain, nsd_: boundary)
        CORE::GEO::fillInitialPositionArray<distype, nsd_ + 1,
            CORE::LINALG::Matrix<nsd_ + 1, nen_>>(ele, xyze_);

        // set views, here we assemble on the boundary dofs only!
        CORE::LINALG::Matrix<nen_, (nsd_ + 1) * nen_> etangcoupl(
            elemat1_epetra.values(), true);  // view only!

        // get current condition
        Teuchos::RCP<CORE::Conditions::Condition> cond =
            params.get<Teuchos::RCP<CORE::Conditions::Condition>>("condition");
        if (cond == Teuchos::null) FOUR_C_THROW("Cannot access condition 'ThermoConvections'");

        // access parameters of the condition
        const std::string* tempstate = &cond->parameters().Get<std::string>("temperature state");
        double coeff = cond->parameters().Get<double>("coeff");
        const int curvenum = cond->parameters().Get<int>("funct");
        const double time = params.get<double>("total time");

        // get surrounding temperature T_infty from input file
        double surtemp = cond->parameters().Get<double>("surtemp");
        // increase the surrounding temperature T_infty step by step
        // can be scaled with a time curve, get time curve number from input file
        const int surtempcurvenum = cond->parameters().Get<int>("surtempfunct");

        // find out whether we shall use a time curve for q^_c and get the factor
        double curvefac = 1.0;
        if (curvenum >= 0)
        {
          curvefac = GLOBAL::Problem::Instance()
                         ->FunctionById<CORE::UTILS::FunctionOfTime>(curvenum)
                         .Evaluate(time);
        }
        // multiply heat convection coefficient with the timecurve factor
        coeff *= curvefac;

        // we can increase or decrease the surrounding (fluid) temperature T_oo
        // enabling for instance a load cycle due to combustion of a fluid
        double surtempcurvefac = 1.0;
        // find out whether we shall use a time curve for T_oo and get the factor
        if (surtempcurvenum >= 0)
        {
          surtempcurvefac = GLOBAL::Problem::Instance()
                                ->FunctionById<CORE::UTILS::FunctionOfTime>(surtempcurvenum)
                                .Evaluate(time);
        }
        // complete surrounding temperatures T_oo: multiply with the timecurve factor
        surtemp *= surtempcurvefac;

        // use current temperature T_{n+1} of current time step for heat convection
        // boundary condition
        if (*tempstate == "Tempnp")
        {
          // disassemble temperature
          if (discretization.HasState("temperature"))
          {
            // get actual values of temperature from global location vector
            std::vector<double> mytempnp((la[0].lm_).size());
            Teuchos::RCP<const Epetra_Vector> tempnp = discretization.GetState("temperature");
            if (tempnp == Teuchos::null) FOUR_C_THROW("Cannot get state vector 'tempnp'");

            CORE::FE::ExtractMyValues(*tempnp, mytempnp, la[0].lm_);
            // build the element temperature
            CORE::LINALG::Matrix<nen_, 1> etemp(mytempnp.data(), true);  // view only!
            etemp_.Update(etemp);                                        // copy
          }  // discretization.HasState("temperature")
          else
            FOUR_C_THROW("No old temperature T_n+1 available");
        }  // Tempnp
        // use temperature T_n of last known time step t_n
        else if (*tempstate == "Tempn")
        {
          if (discretization.HasState("old temperature"))
          {
            // get actual values of temperature from global location vector
            std::vector<double> mytempn((la[0].lm_).size());
            Teuchos::RCP<const Epetra_Vector> tempn = discretization.GetState("old temperature");
            if (tempn == Teuchos::null) FOUR_C_THROW("Cannot get state vector 'tempn'");

            CORE::FE::ExtractMyValues(*tempn, mytempn, la[0].lm_);
            // build the element temperature
            CORE::LINALG::Matrix<nen_, 1> etemp(mytempn.data(), true);  // view only!
            etemp_.Update(etemp);                                       // copy
          }  // discretization.HasState("old temperature")
          else
            FOUR_C_THROW("No old temperature T_n available");
        }  // Tempn
        else
          FOUR_C_THROW("Unknown type of convection boundary condition");

#ifdef THRASOUTPUT
        if (ele->Id() == 0)
        {
          std::cout << "ele Id= " << ele->Id() << std::endl;
          // print all parameters read from the current condition
          std::cout << "type of boundary condition  = " << *tempstate << std::endl;
          std::cout << "heat convection coefficient = " << coeff << std::endl;
          std::cout << "surrounding temperature     = " << surtemp << std::endl;
          std::cout << "time curve                  = " << curvenum << std::endl;
          std::cout << "total time                  = " << time << std::endl;
        }
#endif  // THRASOUTPUT

        // get the displacements
        Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState(1, "displacement");
        if (disp == Teuchos::null) FOUR_C_THROW("Cannot get state vectors 'displacement'");
        // extract the displacements
        CORE::FE::ExtractMyValues(*disp, mydisp, la[1].lm_);

        // and now check if there is a convection heat transfer boundary condition
        calculate_nln_convection_fint_cond(ele,  // current boundary element
            mydisp,
            nullptr,      // element-matrix k_TT
            &etangcoupl,  // coupling matrix k_Td
            nullptr,      // element-rhs
            coeff, surtemp, *tempstate);

        // BUILD EFFECTIVE TANGENT AND RESIDUAL ACC TO TIME INTEGRATOR
        // check the time integrator
        const INPAR::THR::DynamicType timint = CORE::UTILS::GetAsEnum<INPAR::THR::DynamicType>(
            params, "time integrator", INPAR::THR::dyna_undefined);
        switch (timint)
        {
          case INPAR::THR::dyna_statics:
          {
            if (*tempstate == "Tempn")
              FOUR_C_THROW("Old temperature T_n is not allowed with static time integrator");
            // continue
            break;
          }
          case INPAR::THR::dyna_onesteptheta:
          {
            // Note: efext is scaled with theta in thrtimint_ost.cpp. Because the
            // convective boundary condition is nonlinear and produces a term in the
            // tangent, consider the factor theta here, too
            const double theta = params.get<double>("theta");
            // combined tangent and conductivity matrix to one global matrix
            etangcoupl.Scale(theta);
            break;
          }
          case INPAR::THR::dyna_genalpha:
          {
            // Note: efext is scaled with theta in thrtimint_ost.cpp. Because the
            // convective boundary condition is nonlinear and produces a term in the
            // tangent, consider the factor theta here, too
            const double alphaf = params.get<double>("alphaf");
            // combined tangent and conductivity matrix to one global matrix
            etangcoupl.Scale(alphaf);
            break;
          }
          case INPAR::THR::dyna_undefined:
          default:
          {
            FOUR_C_THROW("Don't know what to do...");
            break;
          }
        }  // end of switch(timint)

      }  // disp!=0
    }    // if ( (kintype == INPAR::STR::KinemType::nonlinearTotLag) and (la.Size()>1) )
  }      // calc_thermo_fextconvection_coupltang

  else
    FOUR_C_THROW("Unknown type of action for Temperature Implementation: %s",
        THR::BoundaryActionToString(action).c_str());

  return 0;
}  // Evaluate()


/*----------------------------------------------------------------------*
 | integrate a Surface/Line Neumann boundary condition        gjb 01/09 |
 | i.e. calculate q^ = q . n over surface da                            |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
int DRT::ELEMENTS::TemperBoundaryImpl<distype>::evaluate_neumann(const DRT::Element* ele,
    Teuchos::ParameterList& params, const DRT::Discretization& discretization,
    const CORE::Conditions::Condition& condition, const std::vector<int>& lm,
    CORE::LINALG::SerialDenseVector& elevec1)
{
  // prepare nurbs
  prepare_nurbs_eval(ele, discretization);

  // get node coordinates (we have a nsd_+1 dimensional domain!)
  CORE::GEO::fillInitialPositionArray<distype, nsd_ + 1, CORE::LINALG::Matrix<nsd_ + 1, nen_>>(
      ele, xyze_);

  // integration points and weights
  CORE::FE::IntPointsAndWeights<nsd_> intpoints(THR::DisTypeToOptGaussRule<distype>::rule);

  // find out whether we will use a time curve
  const double time = params.get("total time", -1.0);

  // find out whether we will use a time curve and get the factor

  // get values, switches and spatial functions from the condition
  // (assumed to be constant on element boundary)
  const auto* onoff = &condition.parameters().Get<std::vector<int>>("onoff");
  const auto* val = &condition.parameters().Get<std::vector<double>>("val");
  const auto* func = &condition.parameters().Get<std::vector<int>>("funct");

  // integration loop
  for (int iquad = 0; iquad < intpoints.IP().nquad; ++iquad)
  {
    // output of function: fac_ =  detJ * w(gp)
    eval_shape_func_and_int_fac(intpoints, iquad, ele->Id());

    // factor given by spatial function
    double functfac = 1.0;
    // determine global coordinates of current Gauss point

    const int nsd_vol_ele = nsd_ + 1;
    CORE::LINALG::Matrix<nsd_vol_ele, 1> coordgp;  // coordinate has always to be given in 3D!
    coordgp.MultiplyNN(xyze_, funct_);

    int functnum = -1;
    const double* coordgpref = &coordgp(0);

    for (int dof = 0; dof < numdofpernode_; dof++)
    {
      if ((*onoff)[dof])  // is this dof activated?
      {
        // factor given by spatial function
        if (func) functnum = (*func)[dof];
        {
          if (functnum > 0)
          {
            // evaluate function at current gauss point
            functfac = GLOBAL::Problem::Instance()
                           ->FunctionById<CORE::UTILS::FunctionOfSpaceTime>(functnum - 1)
                           .Evaluate(coordgpref, time, dof);
          }
          else
            functfac = 1.0;
        }

        // q * detJ * w(gp) * spatial_fac * timecurve_fac
        // val = q; fac_ = detJ * w(gp) * timecurve; funcfac =  spatial_fac
        const double val_fac_funct_curve_fac = (*val)[dof] * fac_ * functfac;

        for (int node = 0; node < nen_; ++node)
        {
          // fext  = fext +  N^T  * q * detJ * w(gp) * spatial_fac * timecurve_fac
          // with scalar-valued q
          elevec1[node * numdofpernode_ + dof] += funct_(node) * val_fac_funct_curve_fac;
        }
      }  // if ((*onoff)[dof])
    }
  }  // end of loop over integration points

  return 0;
}  // evaluate_neumann()


/*----------------------------------------------------------------------*
 | evaluate a convective thermo boundary condition          dano 12/10 |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
void DRT::ELEMENTS::TemperBoundaryImpl<distype>::calculate_convection_fint_cond(
    const DRT::Element* ele, CORE::LINALG::Matrix<nen_, nen_>* econd,
    CORE::LINALG::Matrix<nen_, 1>* efext, const double coeff, const double surtemp,
    const std::string& tempstate)
{
  // ------------------------------- integration loop for one element

  // integrations points and weights
  CORE::FE::IntPointsAndWeights<nsd_> intpoints(THR::DisTypeToOptGaussRule<distype>::rule);
  if (intpoints.IP().nquad != nquad_) FOUR_C_THROW("Trouble with number of Gauss points");

  // ----------------------------------------- loop over Gauss Points
  for (int iquad = 0; iquad < intpoints.IP().nquad; iquad++)
  {
    // output of function: fac_ = detJ * w(gp)
    eval_shape_func_and_int_fac(intpoints, iquad, ele->Id());
    // fac_ = Gauss weight * det(J) is calculated in eval_shape_func_and_int_fac()

    // ------------right-hand-side
    // q . n = h ( T - T_sur )

    // multiply fac_ * coeff
    // --> must be insert in balance equation as positive term,
    // but fext is included as negative --> scale with (-1)
    double coefffac_ = fac_ * coeff;

    // get the current temperature
    // Theta = Ntemp = N . T
    // caution: funct_ implemented as (4,1)--> use transposed in code for
    // theoretic part
    // funct_ describes a 2D area, for hex8: 4 nodes
    // (1x1)= (1x4)(4x1) = (nen_*numdofpernode_ x 1)^T(nen_*numdofpernode_ x 1)
    CORE::LINALG::Matrix<1, 1> Ntemp(false);
    Ntemp.MultiplyTN(funct_, etemp_);

    // substract the surface temperature: Ntemp -=  T_surf
    CORE::LINALG::Matrix<1, 1> Tsurf(true);
    for (int i = 0; i < 1; ++i)
    {
      Tsurf(i) = (surtemp);
    }
    // in the following Ntemp describes the temperature difference and not only
    // the scalar-valued temperature
    Ntemp.Update(-1.0, Tsurf, 1.0);

    // ---------------------------------------------- right-hand-side
    if (efext != nullptr)
    {
      // fext^e = fext^e - N^T . coeff . (N . T - T_oo) . detJ * w(gp)
      // in energy balance: q_c positive, but fext = r^ + q^ + q^_c
      // we want to define q^_c = - q . n
      // q^_cr = k . Grad T = h (T - T_oo), vgl. Farhat(1992)
      efext->Multiply(coefffac_, funct_, Ntemp, 1.0);
    }  // efext != nullptr

    // ------------------------------------------------------ tangent
    // if current temperature T_{n+1} is considered in boundary condition
    // consider additional term in thermal tangent K_TT
    if (tempstate == "Tempnp")
    {
      // if the boundary condition shall be dependent on the current temperature
      // solution T_n+1 --> linearisation must be considered
      // ---------------------matrix
      if (econd != nullptr)
      {
        // k_TT^e = k_TT^e + (N^T . coeff . N) * detJ * w(gp)
        econd->MultiplyNT((-1.0) * coefffac_, funct_, funct_, 1.0);
      }  // econd != nullptr

    }  // Tempnp

  }  // ---------------------------------- end loop over Gauss Points

  return;

}  // calculate_convection_fint_cond()


/*----------------------------------------------------------------------*
 | evaluate a convective thermo boundary condition          dano 11/12 |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
void DRT::ELEMENTS::TemperBoundaryImpl<distype>::calculate_nln_convection_fint_cond(
    const DRT::Element* ele,
    const std::vector<double>& disp,  // current displacements
    CORE::LINALG::Matrix<nen_, nen_>* econd,
    CORE::LINALG::Matrix<nen_, (nsd_ + 1) * nen_>* etangcoupl, CORE::LINALG::Matrix<nen_, 1>* efext,
    const double coeff, const double surtemp, const std::string& tempstate)
{
  // update element geometry
  // get node coordinates of full dimensions, i.e. nsd_+1
  CORE::LINALG::Matrix<nen_, (nsd_ + 1)> xcurr;  // current  coord. of boundary element

  for (int i = 0; i < nen_; ++i)
  {
    // material/referential coordinates
    // (dimensions of dof as parent element) (8x3) = (nen_xnsd_)
    // xyze_ = xrefe

    // the current coordinates
    xcurr(i, 0) = xyze_(0, i) + disp[i * (nsd_ + 1) + 0];
    xcurr(i, 1) = xyze_(1, i) + disp[i * (nsd_ + 1) + 1];
    xcurr(i, 2) = xyze_(2, i) + disp[i * (nsd_ + 1) + 2];
  }

  // ------------------------------- integration loop for one element

  // integrations points and weights for 2D, i.e. dim of boundary element
  CORE::FE::IntPointsAndWeights<nsd_> intpoints(THR::DisTypeToOptGaussRule<distype>::rule);
  if (intpoints.IP().nquad != nquad_) FOUR_C_THROW("Trouble with number of Gauss points");

  // set up matrices and parameters needed for the evaluation of current
  // interfacial area and its derivatives w.r.t. the displacements

  // overall number of surface dofs
  // --> (nsd_+1)*nen_ == ndof== numdofperelement
  // with dimension of parent element nsd_+1
  int numdofperelement = (nsd_ + 1) * nen_;

  // interfacial area, i.e. current element area
  double A = 0.0;
  // first partial derivatives VECTOR
  CORE::LINALG::Matrix<(nsd_ + 1) * nen_, 1> Adiff(true);  // (12x1)

  // ----------------------------------------- loop over Gauss Points
  // with ngp = intpoints.IP().nquad
  for (int iquad = 0; iquad < intpoints.IP().nquad; iquad++)
  {
    // output of function: fac_ = detJ * w(gp)
    // allocate vector for shape functions (funct_) and matrix for derivatives
    // (deriv_) for boundary element
    eval_shape_func_and_int_fac(intpoints, iquad, ele->Id());
    // fac_ = Gauss weight * det(J) is calculated in eval_shape_func_and_int_fac()

    // calculate the current normal vector normal and the area
    CORE::LINALG::Matrix<nsd_ + 1, 1> normal;  // (3x1)
    double detA = 0.0;
    surface_integration(detA, normal, xcurr);
    // the total surface corresponds to the sum over all GPs.
    // fext and k_ii is calculated by assembling over all nodes
    // --> multiply the corresponding sub-area to the terms
    A = detA * intpoints.IP().qwgt[iquad];  // here is the current area included

    // initialise the matrices
    CORE::LINALG::Matrix<(nsd_ + 1), (nsd_ + 1) * nen_> ddet(true);  // (3x12)
    CORE::LINALG::Matrix<((nsd_ + 1) * (nsd_ + 1) * nen_), (nsd_ + 1) * nen_> ddet2(
        true);                                                        // (3*2*4x2*4)=(24x8)
    CORE::LINALG::Matrix<((nsd_ + 1) * nen_), 1> jacobi_deriv(true);  // (3*4x1)=(12x1)

    // with derxy_ (2x4) --> (nsd_xnen_)
    // derxy_== (LENA) dxyzdrs.Multiply('N','N',1.0,deriv,x,0.0);
    // compute global derivatives
    // (nsd_x(nsd_+1)) = (nsdxnen_) . (nen_x(nsd_+1))
    // (2x3) = (2x4) . (4x3)
    CORE::LINALG::Matrix<nsd_, (nsd_ + 1)> dxyzdrs(false);  // (2x3)
    dxyzdrs.Multiply(deriv_, xcurr);

    // derivation of minor determiants of the Jacobian with respect to the
    // displacements
    // loop over surface nodes
    for (int i = 0; i < nen_; ++i)  // nen_=4, i:1--3
    {
      // deriv == deriv_: (2x8), dxyzdrs: (2x3)
      // r,s variable and t=1
      // ddet (3x12)
      ddet(0, 3 * i) = 0.;
      ddet(0, 3 * i + 1) = deriv_(0, i) * dxyzdrs(1, 2) - deriv_(1, i) * dxyzdrs(0, 2);
      ddet(0, 3 * i + 2) = deriv_(1, i) * dxyzdrs(0, 1) - deriv_(0, i) * dxyzdrs(1, 1);

      ddet(1, 3 * i) = deriv_(1, i) * dxyzdrs(0, 2) - deriv_(0, i) * dxyzdrs(1, 2);
      ddet(1, 3 * i + 1) = 0.;
      ddet(1, 3 * i + 2) = deriv_(0, i) * dxyzdrs(1, 0) - deriv_(1, i) * dxyzdrs(0, 0);

      ddet(2, 3 * i) = deriv_(0, i) * dxyzdrs(1, 1) - deriv_(1, i) * dxyzdrs(0, 1);
      ddet(2, 3 * i + 1) = deriv_(1, i) * dxyzdrs(0, 0) - deriv_(0, i) * dxyzdrs(1, 0);
      ddet(2, 3 * i + 2) = 0.;

      jacobi_deriv(i * 3, 0) =
          1 / detA * (normal(2, 0) * ddet(2, 3 * i) + normal(1, 0) * ddet(1, 3 * i));
      jacobi_deriv(i * 3 + 1, 0) =
          1 / detA * (normal(2, 0) * ddet(2, 3 * i + 1) + normal(0, 0) * ddet(0, 3 * i + 1));
      jacobi_deriv(i * 3 + 2, 0) =
          1 / detA * (normal(0, 0) * ddet(0, 3 * i + 2) + normal(1, 0) * ddet(1, 3 * i + 2));
    }

    // --- calculation of first derivatives of current interfacial
    // ---------------------area with respect to the displacements
    for (int i = 0; i < numdofperelement; ++i)  // 3*4 = 12
    {
      Adiff(i, 0) += jacobi_deriv(i) * intpoints.IP().qwgt[iquad];
    }

    // ------------right-hand-side
    // AK: q . n da = q^_c da = h ( T - T_sur ) da
    // RK: q . n da = h ( T - T_sur ) da = =: Q^_c dA
    //      da  = J sqrt(N^T . C^{-1} . N) dA
    // here we use an alternative approach for the implementation
    // do not map the term to RK--> coordinate space,
    // BUT: directly form AK --> coordinate space

    // multiply fac_ . coeff
    // --> must be insert in balance equation as positive term,
    // but fext is included as negative --> scale with (-1)
    // compared to linear case here we use the mapping from coordinate space to AK
    // --> for jacobi determinant use A instead of fac_ which is build with
    // material coordinates
    double coeffA = A * coeff;

    // get the current temperature
    // Theta = Ntemp = N . T
    // caution: funct_ implemented as (4,1)--> use transposed in code for
    // theoretic part
    // funct_ describes a 2D area, for hex8: 4 nodes
    // (1x1)= (1x4)(4x1) = (nen_*numdofpernode_ x 1)^T(nen_*numdofpernode_ x 1)
    CORE::LINALG::Matrix<1, 1> Ntemp(false);
    Ntemp.MultiplyTN(funct_, etemp_);

    // T - T_surf
    CORE::LINALG::Matrix<1, 1> Tsurf(false);
    Tsurf(0, 0) = (surtemp);

    // Ntemp -= T_surf
    // in the following Ntemp describes the temperature difference and not only
    // the scalar-valued temperature
    Ntemp.Update(-1.0, Tsurf, 1.0);

    // ---------------------------------------------- right-hand-side
    if (efext != nullptr)
    {
      // efext = efext - N^T . coeff . ( N . T - T_surf) . sqrt( Normal^T . C^{-1} . Normal ) . detJ
      // * w(gp)

      // in energy balance: q_c positive, but fext = r + q^ + q_c^
      // we want to define q_c^ = - q . n
      // q_c^ = k . Grad T = h (T - T_oo)
      efext->Multiply(coeffA, funct_, Ntemp, 1.0);
    }  // efext != nullptr

    // ------------------------------------------------------ tangent
    // if current temperature T_{n+1} is considered in boundary condition
    // consider additional term in thermal tangent K_TT
    if (tempstate == "Tempnp")
    {
      // if the boundary condition shall be dependent on the current temperature
      // solution T_n+1 --> linearisation must be considered
      // ---------------------matrix
      if (econd != nullptr)
      {
        // ke = ke + (N^T . coeff . N) . detJ . w(gp)
        // (nsd_xnsd_) = (4x4)
        econd->MultiplyNT((-1.0) * coeffA, funct_, funct_, 1.0);
      }  // etang != nullptr

    }  // Tempnp

    // ke_Td = ke_Td + N^T . coeff . h . (N . T - T_oo) * dA/dd * detJ * w(gp)
    // (4x12)         (4X1)                  (1x1)        (1X12)
    // ke_Td = ke_Td + N^T . coeff . h . (N . T - T_oo) * Adiff * detJ * w(gp)
    if (etangcoupl != nullptr)
    {
      // multiply fac_ * coeff
      // --> must be insert in balance equation as positive term,
      // but fext is included as negative --> scale with (-1)

      CORE::LINALG::Matrix<nen_, 1> NNtemp;
      NNtemp.Multiply(funct_, Ntemp);
      etangcoupl->MultiplyNT((-1.0) * coeffA, NNtemp, Adiff, 1.0);
    }

  }  // ---------------------------------- end loop over Gauss Points

  return;

}  // calculate_nln_convection_fint_cond()


/*----------------------------------------------------------------------*
 | evaluate shape functions and int. factor at int. point     gjb 01/09 |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
void DRT::ELEMENTS::TemperBoundaryImpl<distype>::eval_shape_func_and_int_fac(
    const CORE::FE::IntPointsAndWeights<nsd_>& intpoints,  // integration points
    const int& iquad,                                      // id of current Gauss point
    const int& eleid                                       // the element id
)
{
  // coordinates of the current (Gauss) integration point (xsi_)
  const double* gpcoord = (intpoints.IP().qxg)[iquad];
  for (int idim = 0; idim < nsd_; idim++)
  {
    xsi_(idim) = gpcoord[idim];
  }

  // shape functions and their first derivatives
  // deriv_ == deriv(LENA), dxydrs (LENA)
  if (myknots_.size() == 0)
  {
    CORE::FE::shape_function<distype>(xsi_, funct_);
    CORE::FE::shape_function_deriv1<distype>(xsi_, deriv_);  // nsd_ x nen_
  }
  else
    CORE::FE::NURBS::nurbs_get_2D_funct_deriv(funct_, deriv_, xsi_, myknots_, weights_, distype);

  // the metric tensor and the area of an infinitesimal surface/line element
  // initialise the determinant: drs = srqt( det(metrictensor_) )
  double drs(0.0);
  CORE::FE::ComputeMetricTensorForBoundaryEle<distype>(xyze_, deriv_,
      metrictensor_,  // metrictensor between material coordinates xyze_ and coordinate space xi_i
      drs);

  // set the integration factor = GP_weight * sqrt ( det(metrictensor_) )
  fac_ = intpoints.IP().qwgt[iquad] * drs * normalfac_;

  // say goodbye
  return;
}  // eval_shape_func_and_int_fac()


/*----------------------------------------------------------------------*
 | get constant normal                                        gjb 01/09 |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
void DRT::ELEMENTS::TemperBoundaryImpl<distype>::get_const_normal(
    CORE::LINALG::Matrix<nsd_ + 1, 1>& normal,
    const CORE::LINALG::Matrix<nsd_ + 1, nen_>& xyze  // node coordinates
) const
{
  // determine normal to this element

  // be aware: nsd_ corresponds to dimension of the boundary not of the simulation
  // simulation is 3D --> boundary is a surface, i.e. nsd_ = 2
  //               2D -->                             nsd_ = 1
  switch (nsd_)
  {
    case 2:
    {
      CORE::LINALG::Matrix<3, 1> dist1(true), dist2(true);
      for (int i = 0; i < 3; i++)
      {
        dist1(i) = xyze(i, 1) - xyze(i, 0);
        dist2(i) = xyze(i, 2) - xyze(i, 0);
      }

      normal(0) = dist1(1) * dist2(2) - dist1(2) * dist2(1);
      normal(1) = dist1(2) * dist2(0) - dist1(0) * dist2(2);
      normal(2) = dist1(0) * dist2(1) - dist1(1) * dist2(0);
    }
    break;
    case 1:
    {
      // quad4: surface is described via 2 nodes N1(x1,y1), N2(x2,y2) in (x,y)
      // n = [n1, n2]^T
      // n1 = x2-x1, n2 = y2-y1
      // --> outward-pointing normal: scale with (-1.0)
      normal(0) = xyze(1, 1) - xyze(1, 0);
      normal(1) = (-1.0) * (xyze(0, 1) - xyze(0, 0));
    }
    break;
    default:
      FOUR_C_THROW("Illegal number of space dimensions: %d", nsd_);
      break;
  }  // switch(nsd)

  // length of normal to this element
  const double length = normal.Norm2();
  // outward-pointing normal of length 1.0
  normal.Scale(1 / length);

  return;
}  // get_const_normal()


/*----------------------------------------------------------------------*
 | integrate shapefunctions over surface (private)            gjb 02/09 |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
void DRT::ELEMENTS::TemperBoundaryImpl<distype>::integrate_shape_functions(const DRT::Element* ele,
    Teuchos::ParameterList& params, CORE::LINALG::SerialDenseVector& elevec1, const bool addarea)
{
  // access boundary area variable with its actual value
  double boundaryint = params.get<double>("boundaryint");

  // get node coordinates (we have a nsd_+1 dimensional domain!)
  CORE::GEO::fillInitialPositionArray<distype, nsd_ + 1, CORE::LINALG::Matrix<nsd_ + 1, nen_>>(
      ele, xyze_);

  // integrations points and weights
  CORE::FE::IntPointsAndWeights<nsd_> intpoints(THR::DisTypeToOptGaussRule<distype>::rule);

  // loop over integration points
  for (int iquad = 0; iquad < intpoints.IP().nquad; iquad++)
  {
    eval_shape_func_and_int_fac(intpoints, iquad, ele->Id());

    // compute integral of shape functions
    for (int node = 0; node < nen_; ++node)
    {
      for (int k = 0; k < numdofpernode_; k++)
      {
        elevec1[node * numdofpernode_ + k] += funct_(node) * fac_;
      }
    }

    if (addarea)
    {
      // area calculation
      boundaryint += fac_;
    }

  }  // loop over integration points

  // add contribution to the global value
  params.set<double>("boundaryint", boundaryint);

  return;

}  // integrate_shape_function()


/*----------------------------------------------------------------------*
 | evaluate sqrt of determinant of metric at gp (private)    dano 12/12 |
 *----------------------------------------------------------------------*/
template <CORE::FE::CellType distype>
void DRT::ELEMENTS::TemperBoundaryImpl<distype>::surface_integration(
    double& detA, CORE::LINALG::Matrix<nsd_ + 1, 1>& normal,
    const CORE::LINALG::Matrix<nen_, nsd_ + 1>& xcurr  // current coordinates of nodes
)
{
  // determine normal to this element
  CORE::LINALG::Matrix<nsd_, (nsd_ + 1)> dxyzdrs(false);
  dxyzdrs.MultiplyNN(deriv_, xcurr);

  /* compute covariant metric tensor G for surface element
  **                        | g11   g12 |
  **                    G = |           |
  **                        | g12   g22 |
  ** where (o denotes the inner product, xyz a vector)
  **
  **       dXYZ   dXYZ          dXYZ   dXYZ          dXYZ   dXYZ
  ** g11 = ---- o ----    g12 = ---- o ----    g22 = ---- o ----
  **        dr     dr            dr     ds            ds     ds
  */

  CORE::LINALG::Matrix<(nsd_ + 1), nen_> xcurr_T(false);
  xcurr_T.UpdateT(xcurr);

  // the metric tensor and the area of an infinitesimal surface/line element
  // compute dXYZ / drs is included in ComputeMetricTensorForBoundaryEle
  // dxyzdrs = deriv . xyze
  // dxyzdrs.MultiplyNT(1.0,deriv,xyze,0.0) = (LENA)dxyzdrs
  // be careful: normal
  CORE::FE::ComputeMetricTensorForBoundaryEle<distype>(xcurr_T, deriv_,
      metrictensor_,  // metric tensor between coordinate space and AK
      detA
      // normalvector==nullptr // we don't need the unit normal vector, but the
  );

  // be aware: nsd_ corresponds to dimension of the boundary not of the simulation
  // simulation is 3D --> boundary is a surface, i.e. nsd_ = 2
  //               2D -->                             nsd_ = 1
  switch (nsd_)
  {
    case 2:
    {
      normal(0) = dxyzdrs(0, 1) * dxyzdrs(1, 2) - dxyzdrs(0, 2) * dxyzdrs(1, 1);
      normal(1) = dxyzdrs(0, 2) * dxyzdrs(1, 0) - dxyzdrs(0, 0) * dxyzdrs(1, 2);
      normal(2) = dxyzdrs(0, 0) * dxyzdrs(1, 1) - dxyzdrs(0, 1) * dxyzdrs(1, 0);
    }
    break;
    case 1:
    {
      // quad4: surface is described via 2 nodes N1(x1,y1), N2(x2,y2) in (x,y)
      // n = [n1, n2]^T
      // n1 = x2-x1, n2 = y2-y1
      normal(0) = dxyzdrs(0, 1);
      normal(1) = -dxyzdrs(0, 0);
    }
    break;
    default:
      FOUR_C_THROW("Illegal number of space dimensions: %d", nsd_);
      break;
  }  // switch(nsd)

  return;
}

template <CORE::FE::CellType distype>
void DRT::ELEMENTS::TemperBoundaryImpl<distype>::prepare_nurbs_eval(
    const DRT::Element* ele,                   // the element whose matrix is calculated
    const DRT::Discretization& discretization  // current discretisation
)
{
  if (ele->Shape() != CORE::FE::CellType::nurbs9)
  {
    myknots_.resize(0);
    return;
  }

  // get nurbs specific infos
  // cast to nurbs discretization
  const auto* nurbsdis = dynamic_cast<const DRT::NURBS::NurbsDiscretization*>(&(discretization));
  if (nurbsdis == nullptr) FOUR_C_THROW("So_nurbs27 appeared in non-nurbs discretisation\n");

  std::vector<CORE::LINALG::SerialDenseVector> parentknots(3);
  myknots_.resize(2);

  const auto* faceele = dynamic_cast<const DRT::FaceElement*>(ele);
  (*nurbsdis).GetKnotVector()->get_boundary_ele_and_parent_knots(parentknots, myknots_, normalfac_,
      faceele->ParentMasterElement()->Id(), faceele->FaceMasterNumber());

  // get weights from cp's
  for (int inode = 0; inode < nen_; inode++)
    weights_(inode) = dynamic_cast<const DRT::NURBS::ControlPoint*>(ele->Nodes()[inode])->W();
}
/*----------------------------------------------------------------------*/

FOUR_C_NAMESPACE_CLOSE