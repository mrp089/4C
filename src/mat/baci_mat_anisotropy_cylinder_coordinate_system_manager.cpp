/*----------------------------------------------------------------------*/
/*! \file

\brief Implementation of a cylinder coordinate system manager

\level 3


*/
/*----------------------------------------------------------------------*/

#include "baci_mat_anisotropy_cylinder_coordinate_system_manager.hpp"

#include "baci_comm_parobject.hpp"
#include "baci_io_linedefinition.hpp"
#include "baci_mat_anisotropy.hpp"
#include "baci_mat_anisotropy_utils.hpp"
#include "baci_utils_exceptions.hpp"

FOUR_C_NAMESPACE_OPEN

MAT::CylinderCoordinateSystemManager::CylinderCoordinateSystemManager() = default;

void MAT::CylinderCoordinateSystemManager::Pack(CORE::COMM::PackBuffer& data) const
{
  CORE::COMM::ParObject::AddtoPack(data, radial_);
  CORE::COMM::ParObject::AddtoPack(data, axial_);
  CORE::COMM::ParObject::AddtoPack(data, circumferential_);
  CORE::COMM::ParObject::AddtoPack(data, static_cast<int>(isDefined_));
}

void MAT::CylinderCoordinateSystemManager::Unpack(
    const std::vector<char>& data, std::vector<char>::size_type& position)
{
  CORE::COMM::ParObject::ExtractfromPack(position, data, radial_);
  CORE::COMM::ParObject::ExtractfromPack(position, data, axial_);
  CORE::COMM::ParObject::ExtractfromPack(position, data, circumferential_);
  isDefined_ = static_cast<bool>(CORE::COMM::ParObject::ExtractInt(position, data));
}

void MAT::CylinderCoordinateSystemManager::ReadFromElementLineDefinition(
    INPUT::LineDefinition* linedef)
{
  if (linedef->HaveNamed("RAD") and linedef->HaveNamed("AXI") and linedef->HaveNamed("CIR"))
  {
    ReadAnisotropyFiber(linedef, "RAD", radial_);
    ReadAnisotropyFiber(linedef, "AXI", axial_);
    ReadAnisotropyFiber(linedef, "CIR", circumferential_);
    isDefined_ = true;
  }
}

void MAT::CylinderCoordinateSystemManager::EvaluateLocalCoordinateSystem(
    CORE::LINALG::Matrix<3, 3>& cosy) const
{
  for (int i = 0; i < 3; ++i)
  {
    cosy(i, 0) = GetRad()(i);
    cosy(i, 1) = GetAxi()(i);
    cosy(i, 2) = GetCir()(i);
  }
}

const MAT::CylinderCoordinateSystemManager& MAT::Anisotropy::GetElementCylinderCoordinateSystem()
    const
{
  return elementCylinderCoordinateSystemManager_.value();
}

const MAT::CylinderCoordinateSystemManager& MAT::Anisotropy::GetGPCylinderCoordinateSystem(
    const int gp) const
{
  return gpCylinderCoordinateSystemManagers_[gp];
}
FOUR_C_NAMESPACE_CLOSE
