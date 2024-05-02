/*----------------------------------------------------------------------*/
/*! \file
\brief Bundle holds all read-in materials of a #GLOBAL::Problem

\level 1

*/

/*----------------------------------------------------------------------*/
/* macros */


/*----------------------------------------------------------------------*/
/* headers */
#include "4C_mat_par_bundle.hpp"

#include "4C_mat_material_factory.hpp"
#include "4C_matelast_summand.hpp"
#include "4C_material_base.hpp"

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*/
MAT::PAR::Bundle::Bundle() : materialreadfromproblem_(0) {}


/*----------------------------------------------------------------------*/
void MAT::PAR::Bundle::Insert(int matid, Teuchos::RCP<CORE::MAT::PAR::Material> mat)
{
  matmap_.insert(std::pair<int, Teuchos::RCP<CORE::MAT::PAR::Material>>(matid, mat));
}

/*----------------------------------------------------------------------*/
int MAT::PAR::Bundle::Find(const int id) const
{
  if (matmap_.find(id) == matmap_.end())
    return -1;
  else
    return matmap_.find(id)->first;
}

/*----------------------------------------------------------------------*/
void MAT::PAR::Bundle::MakeParameters()
{
  for (std::map<int, Teuchos::RCP<CORE::MAT::PAR::Material>>::iterator m = matmap_.begin();
       m != matmap_.end(); ++m)
  {
    int matid = m->first;

    // 1st try
    {
      // indirectly add quick access parameter members
      Teuchos::RCP<CORE::MAT::Material> mat = MAT::Factory(matid);
      // check if allocation was successful
      Teuchos::RCP<CORE::MAT::PAR::Material> matpar = m->second;
      if (matpar->Parameter() != nullptr) continue;
    }

    // 2nd try
    {
      // indirectly add quick access parameter members
      Teuchos::RCP<MAT::ELASTIC::Summand> mat = MAT::ELASTIC::Summand::Factory(matid);
      // check if allocation was successful
      Teuchos::RCP<CORE::MAT::PAR::Material> matpar = m->second;
      if (matpar->Parameter() != nullptr) continue;
    }

    // trials failed
    FOUR_C_THROW("Allocation of quick access parameters failed for material MAT %d", matid);
  }
}

/*----------------------------------------------------------------------*/
Teuchos::RCP<CORE::MAT::PAR::Material> MAT::PAR::Bundle::ById(const int num) const
{
  std::map<int, Teuchos::RCP<CORE::MAT::PAR::Material>>::const_iterator m = matmap_.find(num);

  if (matmap_.size() == 0) FOUR_C_THROW("No materials available, num=%d", num);

  if (m == matmap_.end())
    FOUR_C_THROW("Material 'MAT %d' could not be found", num);
  else
    return m->second;

  // catch up
  return Teuchos::null;
}

/*----------------------------------------------------------------------*/
int MAT::PAR::Bundle::FirstIdByType(const CORE::Materials::MaterialType type) const
{
  std::map<int, Teuchos::RCP<CORE::MAT::PAR::Material>>::const_iterator m;

  int id = -1;
  for (m = matmap_.begin(); m != matmap_.end(); ++m)
  {
    if (m->second->Type() == type)
    {
      id = m->first;
      break;
    }
  }

  return id;
}

FOUR_C_NAMESPACE_CLOSE
