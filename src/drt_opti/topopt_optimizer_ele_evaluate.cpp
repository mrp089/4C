/*!------------------------------------------------------------------------------------------------*
\file topopt_optimizer_ele_evaluate.cpp

\brief evaluate interface of the topopt element

<pre>
Maintainer: Martin Winklmaier
            winklmaier@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15241
</pre>
 *------------------------------------------------------------------------------------------------*/


#include "topopt_optimizer_ele.H"
#include "topopt_optimizer_ele_impl.H"
#include "topopt_optimizer_ele_parameter.H"


/*---------------------------------------------------------------------*
|  converts a string into an action for this element                   |
*----------------------------------------------------------------------*/
DRT::ELEMENTS::TopOpt::ActionType DRT::ELEMENTS::TopOpt::convertStringToActionType(
    const string& action
) const
{
  dsassert(action != "none", "No action supplied");

  DRT::ELEMENTS::TopOpt::ActionType act = TopOpt::none;
  if (action == "set_general_optimization_parameter")
    act = TopOpt::set_general_optimization_parameter;
  else if (action == "compute_values")
    act = TopOpt::compute_values;
  else if (action == "compute_gradients")
    act = TopOpt::compute_gradients;
  else
    dserror("(%s) Unknown type of action for Fluid",action.c_str());

  return act;
}


/*----------------------------------------------------------------------*
 |  evaluate the element (public)                              gjb 01/09|
 *----------------------------------------------------------------------*/
int DRT::ELEMENTS::TopOpt::Evaluate(
    ParameterList&            params,
    DRT::Discretization&      optidis,
    vector<int>&              lm,
    Epetra_SerialDenseMatrix& elemat1,
    Epetra_SerialDenseMatrix& elemat2,
    Epetra_SerialDenseVector& elevec1,
    Epetra_SerialDenseVector& elevec2,
    Epetra_SerialDenseVector& elevec3)
{
  // get the action required
  const string action = params.get<string>("action","none");
  const DRT::ELEMENTS::TopOpt::ActionType act = convertStringToActionType(action);

  // get material
  Teuchos::RCP<MAT::Material> mat = Material();

  switch (act)
  {
  case set_general_optimization_parameter:
  {
    Teuchos::RCP<DRT::ELEMENTS::TopOptParam> optiparam = DRT::ELEMENTS::TopOptParam::Instance();
    optiparam->SetGeneralOptimizationParameter(params);
    break;
  }
  case compute_values:
  {
    return DRT::ELEMENTS::TopOptImplInterface::Impl(this)->EvaluateValues(
        this,
        params,
        optidis,
        mat,
        lm
    );
    break;
  }
  case compute_gradients:
  {
    return DRT::ELEMENTS::TopOptImplInterface::Impl(this)->EvaluateGradients(
        this,
        params,
        optidis,
        mat,
        lm,
        elevec1
    );
    break;
  }
  default:
    dserror("undefined action type when evaluating optimization element");
  }

  return 0; // algo will never be here, it returned before or gave a dserror
} //DRT::ELEMENTS::Transport::Evaluate


/*----------------------------------------------------------------------*
 |  do nothing (public)                                        gjb 01/09|
 |                                                                      |
 |  The function is just a dummy. For the transport elements, the       |
 |  integration of the volume Neumann (body forces) loads takes place   |
 |  in the element. We need it there for the stabilization terms!       |
 *----------------------------------------------------------------------*/
int DRT::ELEMENTS::TopOpt::EvaluateNeumann(ParameterList& params,
    DRT::Discretization&      discretization,
    DRT::Condition&           condition,
    vector<int>&              lm,
    Epetra_SerialDenseVector& elevec1,
    Epetra_SerialDenseMatrix* elemat1)
{
  return 0;
}





/*---------------------------------------------------------------------*
|  converts a string into an action for this element                   |
*----------------------------------------------------------------------*/
DRT::ELEMENTS::TopOptBoundary::ActionType DRT::ELEMENTS::TopOptBoundary::convertStringToActionType(
    const string& action
) const
{
  dsassert(action != "none", "No action supplied");

  DRT::ELEMENTS::TopOptBoundary::ActionType act = TopOptBoundary::none;
  if (action == "set_general_optimization_parameter")
    act = TopOptBoundary::set_general_optimization_parameter;
  else if (action == "compute_values")
    act = TopOptBoundary::compute_values;
  else if (action == "compute_gradients")
    act = TopOptBoundary::compute_gradients;
  else
    dserror("(%s) Unknown type of action for Fluid",action.c_str());

  return act;
}


/*----------------------------------------------------------------------*
 |  evaluate the element (public)                             gjb 01/09 |
 *----------------------------------------------------------------------*/
int DRT::ELEMENTS::TopOptBoundary::Evaluate(
    ParameterList&            params,
    DRT::Discretization&      discretization,
    vector<int>&              lm,
    Epetra_SerialDenseMatrix& elemat1,
    Epetra_SerialDenseMatrix& elemat2,
    Epetra_SerialDenseVector& elevec1,
    Epetra_SerialDenseVector& elevec2,
    Epetra_SerialDenseVector& elevec3)
{
 return 0;
}


/*----------------------------------------------------------------------*
 |  Integrate a Surface/Line Neumann boundary condition       gjb 01/09 |
 *----------------------------------------------------------------------*/
int DRT::ELEMENTS::TopOptBoundary::EvaluateNeumann(
    ParameterList&            params,
    DRT::Discretization&      optidis,
    DRT::Condition&           condition,
    vector<int>&              lm,
    Epetra_SerialDenseVector& elevec1,
    Epetra_SerialDenseMatrix* elemat1)
{
  // get the action required
  const string action = params.get<string>("action","none");
  const DRT::ELEMENTS::TopOptBoundary::ActionType act = convertStringToActionType(action);

  // get material
  Teuchos::RCP<MAT::Material> mat = Material();

  switch (act)
  {
  case set_general_optimization_parameter:
  {
    Teuchos::RCP<DRT::ELEMENTS::TopOptParam> optiparam = DRT::ELEMENTS::TopOptParam::Instance();
    optiparam->SetGeneralOptimizationParameter(params);
  }
  case compute_values:
  {
    return DRT::ELEMENTS::TopOptBoundaryImplInterface::Impl(this)->EvaluateBoundaryValues(
        this,
        params,
        optidis,
        mat,
        lm
    );
    break;
  }
  case compute_gradients:
  {
    return DRT::ELEMENTS::TopOptBoundaryImplInterface::Impl(this)->EvaluateBoundaryGradients(
        this,
        params,
        optidis,
        mat,
        lm,
        elevec1
    );
    break;
  }
  default:
    dserror("undefined action type when evaluating optimization element");
  }

  return 0; // algo will never be here, it returned before or gave a dserror
}



