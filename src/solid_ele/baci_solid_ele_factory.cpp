/*! \file

\brief Factory of solid elements

\level 1
*/

#include "baci_solid_ele_factory.H"

#include "baci_lib_element.H"
#include "baci_solid_ele_calc.H"
#include "baci_solid_ele_calc_eas.H"
#include "baci_solid_ele_calc_fbar.H"
#include "baci_solid_ele_calc_mulf.H"
#include "baci_utils_exceptions.H"

#include <memory>

namespace
{
  template <DRT::Element::DiscretizationType distype>
  DRT::ELEMENTS::SolidCalcVariant CreateFBarSolidCalculationInterface(
      INPAR::STR::KinemType kinem_type)
  {
    if (kinem_type != INPAR::STR::KinemType::kinem_nonlinearTotLag)
    {
      dserror("FBAR only usable for KINEM nonlinear (you are using %s).", kinem_type);
    }

    if constexpr (distype == DRT::Element::DiscretizationType::hex8 ||
                  distype == DRT::Element::DiscretizationType::pyramid5)
    {
      return DRT::ELEMENTS::SolidEleCalcFbar<distype>();
    }

    dserror("FBAR is only implemented for hex8 and pyramid5 elements.");
    return {};
  }

  template <DRT::Element::DiscretizationType distype>
  DRT::ELEMENTS::SolidCalcVariant CreateMulfSolidCalculationInterface(
      INPAR::STR::KinemType kinem_type)
  {
    if (kinem_type != INPAR::STR::KinemType::kinem_nonlinearTotLag)
    {
      dserror("MULF only usable for KINEM nonlinear (you are using %s).", kinem_type);
    }
    return DRT::ELEMENTS::SolidEleCalcMulf<distype>();
  }
}  // namespace

DRT::ELEMENTS::SolidCalcVariant DRT::ELEMENTS::CreateSolidCalculationInterface(
    const DRT::Element& ele, const std::set<INPAR::STR::EleTech>& eletech,
    INPAR::STR::KinemType kinem_type, STR::ELEMENTS::EasType eastype)
{
  switch (ele.Shape())
  {
    case DRT::Element::DiscretizationType::hex8:
      return CreateSolidCalculationInterface<DRT::Element::DiscretizationType::hex8>(
          ele, eletech, kinem_type, eastype);
      break;
    case DRT::Element::DiscretizationType::hex27:
      return CreateSolidCalculationInterface<DRT::Element::DiscretizationType::hex27>(
          ele, eletech, kinem_type, eastype);
      break;
    case DRT::Element::DiscretizationType::hex20:
      return CreateSolidCalculationInterface<DRT::Element::DiscretizationType::hex20>(
          ele, eletech, kinem_type, eastype);
      break;
    case DRT::Element::DiscretizationType::hex18:
      return CreateSolidCalculationInterface<DRT::Element::DiscretizationType::hex18>(
          ele, eletech, kinem_type, eastype);
      break;
    case DRT::Element::DiscretizationType::pyramid5:
      return CreateSolidCalculationInterface<DRT::Element::DiscretizationType::pyramid5>(
          ele, eletech, kinem_type, eastype);
      break;
    case DRT::Element::DiscretizationType::wedge6:
      return CreateSolidCalculationInterface<DRT::Element::DiscretizationType::wedge6>(
          ele, eletech, kinem_type, eastype);
      break;
    case DRT::Element::DiscretizationType::tet4:
      return CreateSolidCalculationInterface<DRT::Element::DiscretizationType::tet4>(
          ele, eletech, kinem_type, eastype);
      break;
    case DRT::Element::DiscretizationType::tet10:
      return CreateSolidCalculationInterface<DRT::Element::DiscretizationType::tet10>(
          ele, eletech, kinem_type, eastype);
      break;
    default:
      dserror("unknown distype provided");
      break;
  }
  return {};
}

template <DRT::Element::DiscretizationType distype>
DRT::ELEMENTS::SolidCalcVariant DRT::ELEMENTS::CreateSolidCalculationInterface(
    const DRT::Element& ele, const std::set<INPAR::STR::EleTech>& eletech,
    INPAR::STR::KinemType kinem_type, STR::ELEMENTS::EasType eastype)
{
  // here we go into the different cases for element technology
  switch (eletech.size())
  {
    // no element technology
    case 0:
      return DRT::ELEMENTS::SolidEleCalc<distype>();
      break;
    // simple: just one element technology
    case 1:
      switch (*eletech.begin())
      {
        case INPAR::STR::EleTech::eas:
          if constexpr (distype != DRT::Element::DiscretizationType::hex8)
          {
            dserror("EAS is only implemented for hex8 elements.");
          }
          else
          {
            switch (eastype)
            {
              case ::STR::ELEMENTS::EasType::eastype_h8_9:
                return DRT::ELEMENTS::SolidEleCalcEas<distype,
                    ::STR::ELEMENTS::EasType::eastype_h8_9>();
              case ::STR::ELEMENTS::EasType::eastype_h8_21:
                return DRT::ELEMENTS::SolidEleCalcEas<distype,
                    ::STR::ELEMENTS::EasType::eastype_h8_21>();
              default:
                dserror("EAS type %d is not implemented %d.", (int)eastype);
            }
          }
        case INPAR::STR::EleTech::fbar:
          return CreateFBarSolidCalculationInterface<distype>(kinem_type);
        case INPAR::STR::EleTech::ps_mulf:
          return CreateMulfSolidCalculationInterface<distype>(kinem_type);
        default:
          dserror("unknown element technology");
      }
    // combination of element technologies
    default:
    {
      dserror("unknown combination of element technologies.");
    }
  }
  return {};
}
