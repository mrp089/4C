// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_config.hpp"

#include "4C_solid_3D_ele_properties.hpp"

#include "4C_comm_parobject.hpp"
#include "4C_solid_scatra_3D_ele_factory.hpp"

FOUR_C_NAMESPACE_OPEN

void Discret::Elements::add_to_pack(Core::Communication::PackBuffer& data,
    const Discret::Elements::SolidElementProperties& properties)
{
  add_to_pack(data, properties.kintype);
  add_to_pack(data, properties.element_technology);
  add_to_pack(data, properties.prestress_technology);
}

void Discret::Elements::extract_from_pack(Core::Communication::UnpackBuffer& buffer,
    Discret::Elements::SolidElementProperties& properties)
{
  extract_from_pack(buffer, properties.kintype);
  extract_from_pack(buffer, properties.element_technology);
  extract_from_pack(buffer, properties.prestress_technology);
}

FOUR_C_NAMESPACE_CLOSE
