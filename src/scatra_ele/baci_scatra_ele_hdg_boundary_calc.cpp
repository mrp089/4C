/*----------------------------------------------------------------------*/
/*! \file

\brief Routines for ScaTraHDG boundary elements

\level 3

*/
/*--------------------------------------------------------------------------*/


#include "baci_scatra_ele_hdg_boundary_calc.H"

#include "baci_lib_node.H"
#include "baci_lib_utils_parameter_list.H"
#include "baci_scatra_ele_action.H"
#include "baci_scatra_ele_hdg.H"


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::ScaTraHDGBoundaryImplInterface* DRT::ELEMENTS::ScaTraHDGBoundaryImplInterface::Impl(
    const DRT::Element* ele)
{
  switch (ele->Shape())
  {
    case DRT::Element::quad4:
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::quad4>::Instance();
    }
    case DRT::Element::quad8:
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::quad8>::Instance();
    }
    case DRT::Element::quad9:
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::quad9>::Instance();
    }
    case DRT::Element::tri3:
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::tri3>::Instance();
    }
    case DRT::Element::tri6:
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::tri6>::Instance();
    }
    case DRT::Element::line2:
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::line2>::Instance();
    }
    case DRT::Element::line3:
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::line3>::Instance();
    }
    case DRT::Element::nurbs2:  // 1D nurbs boundary element
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::nurbs2>::Instance();
    }
    case DRT::Element::nurbs3:  // 1D nurbs boundary element
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::nurbs3>::Instance();
    }
    case DRT::Element::nurbs4:  // 2D nurbs boundary element
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::nurbs4>::Instance();
    }
    case DRT::Element::nurbs9:  // 2D nurbs boundary element
    {
      return ScaTraHDGBoundaryImpl<DRT::Element::nurbs9>::Instance();
    }
    default:
      dserror(
          "Element shape %d (%d nodes) not activated. Just do it.", ele->Shape(), ele->NumNode());
      break;
  }
  return nullptr;
}

template <DRT::Element::DiscretizationType distype>
DRT::ELEMENTS::ScaTraHDGBoundaryImpl<distype>*
DRT::ELEMENTS::ScaTraHDGBoundaryImpl<distype>::Instance(CORE::UTILS::SingletonAction action)
{
  static auto singleton_owner = CORE::UTILS::MakeSingletonOwner(
      []()
      {
        return std::unique_ptr<DRT::ELEMENTS::ScaTraHDGBoundaryImpl<distype>>(
            new DRT::ELEMENTS::ScaTraHDGBoundaryImpl<distype>());
      });

  return singleton_owner.Instance(action);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
DRT::ELEMENTS::ScaTraHDGBoundaryImpl<distype>::ScaTraHDGBoundaryImpl()
    : xyze_(true),
      funct_(true),
      deriv_(true),
      unitnormal_(true),
      velint_(true),
      drs_(0.0),
      fac_(0.0)
{
  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <DRT::Element::DiscretizationType distype>
int DRT::ELEMENTS::ScaTraHDGBoundaryImpl<distype>::EvaluateNeumann(
    DRT::ELEMENTS::ScaTraHDGBoundary* ele, Teuchos::ParameterList& params,
    DRT::Discretization& discretization, DRT::Element::LocationArray& la,
    CORE::LINALG::SerialDenseMatrix& elemat1_epetra,
    CORE::LINALG::SerialDenseVector& elevec1_epetra)
{
  CORE::LINALG::SerialDenseVector dummy_vec2, dummy_vec3;
  CORE::LINALG::SerialDenseMatrix dummy_mat2;

  DRT::UTILS::AddEnumClassToParameterList<SCATRA::Action>(
      "action", SCATRA::Action::project_neumann_field, params);

  const int* nodeids = ele->NodeIds();

  DRT::Element* parent = ele->ParentElement();
  Teuchos::RCP<DRT::FaceElement>* faces = parent->Faces();
  bool same = false;
  for (int i = 0; i < parent->NumFace(); ++i)
  {
    const int* nodeidsfaces = faces[i]->NodeIds();

    if (faces[i]->NumNode() != ele->NumNode()) break;

    for (int j = 0; j < ele->NumNode(); ++j)
    {
      if (nodeidsfaces[j] == nodeids[j])
        same = true;
      else
      {
        same = false;
        break;
      }
    }
    if (same == true)
    {
      // i is the number we were searching for!!!!
      params.set<int>("face", i);
      ele->ParentElement()->Evaluate(params, discretization, la, elemat1_epetra, dummy_mat2,
          elevec1_epetra, dummy_vec2, dummy_vec3);
      // break;
    }
  }
  if (same == false && (faces[0]->NumNode() != ele->NumNode()))
    dserror("Neumann boundary condition implemented only for surface elements");

  return 0;
}
