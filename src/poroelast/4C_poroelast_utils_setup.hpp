/*----------------------------------------------------------------------*/
/*! \file

 \brief utility methods for poro

\level 2

 *----------------------------------------------------------------------*/


#ifndef FOUR_C_POROELAST_UTILS_SETUP_HPP
#define FOUR_C_POROELAST_UTILS_SETUP_HPP


#include "4C_config.hpp"

#include "4C_discretization_dofset_gidbased_wrapper.hpp"
#include "4C_discretization_dofset_predefineddofnumber.hpp"
#include "4C_discretization_fem_general_utils_createdis.hpp"
#include "4C_global_data.hpp"
#include "4C_poroelast_utils.hpp"
#include "4C_poroelast_utils_setup.hpp"

FOUR_C_NAMESPACE_OPEN

namespace POROELAST
{
  namespace UTILS
  {
    //! setup poro discretization,i.e. clone the structural discretization
    template <class PoroCloneStrategy>
    void SetupPoro(bool setmaterialpointers = true)
    {
      GLOBAL::Problem* problem = GLOBAL::Problem::Instance();

      // access the problem-specific parameter list
      const Teuchos::ParameterList& porodyn =
          GLOBAL::Problem::Instance()->poroelast_dynamic_params();
      const bool matchinggrid = CORE::UTILS::IntegralValue<bool>(porodyn, "MATCHINGGRID");

      // access the structure discretization, make sure it is filled
      Teuchos::RCP<DRT::Discretization> structdis;
      structdis = problem->GetDis("structure");
      // set degrees of freedom in the discretization
      if (!structdis->Filled() or !structdis->HaveDofs()) structdis->fill_complete();

      // access the fluid discretization
      Teuchos::RCP<DRT::Discretization> fluiddis;
      fluiddis = problem->GetDis("porofluid");
      if (!fluiddis->Filled()) fluiddis->fill_complete();

      // we use the structure discretization as layout for the fluid discretization
      if (structdis->NumGlobalNodes() == 0) FOUR_C_THROW("Structure discretization is empty!");

      // create fluid elements if the fluid discretization is empty
      if (fluiddis->NumGlobalNodes() == 0)
      {
        if (!matchinggrid)
        {
          FOUR_C_THROW(
              "MATCHINGGRID is set to 'no' in POROELASTICITY DYNAMIC section, but fluid "
              "discretization is empty!");
        }

        // create fluid discretization
        CORE::FE::CloneDiscretization<PoroCloneStrategy>(
            structdis, fluiddis, GLOBAL::Problem::Instance()->CloningMaterialMap());
        fluiddis->fill_complete();

        // set material pointers
        if (setmaterialpointers)
          POROELAST::UTILS::SetMaterialPointersMatchingGrid(structdis, fluiddis);

        // if one discretization is a subset of the other, they will differ in node number (and
        // element number) we assume matching grids for the overlapping part here
        const Epetra_Map* structnodecolmap = structdis->NodeColMap();
        const Epetra_Map* fluidnodecolmap = fluiddis->NodeColMap();

        const int numglobalstructnodes = structnodecolmap->NumGlobalElements();
        const int numglobalfluidnodes = fluidnodecolmap->NumGlobalElements();

        // the problem is two way coupled, thus each discretization must know the other
        // discretization

        /* When coupling porous media with a pure structure we will have two discretizations
         * of different size. In this case we need a special dofset, which can handle submeshes.
         */
        if (numglobalstructnodes != numglobalfluidnodes)
        {
          Teuchos::RCP<CORE::Dofsets::DofSetGIDBasedWrapper> structsubdofset = Teuchos::rcp(
              new CORE::Dofsets::DofSetGIDBasedWrapper(structdis, structdis->GetDofSetProxy()));
          Teuchos::RCP<CORE::Dofsets::DofSetGIDBasedWrapper> fluidsubdofset = Teuchos::rcp(
              new CORE::Dofsets::DofSetGIDBasedWrapper(fluiddis, fluiddis->GetDofSetProxy()));

          // check if fluid_field has 2 discretizations, so that coupling is possible
          if (fluiddis->AddDofSet(structsubdofset) != 1)
            FOUR_C_THROW("unexpected dof sets in fluid field");
          if (structdis->AddDofSet(fluidsubdofset) != 1)
            FOUR_C_THROW("unexpected dof sets in structure field");
        }
        else
        {
          // build a proxy of the structure discretization for the fluid field
          Teuchos::RCP<CORE::Dofsets::DofSetInterface> structdofset = structdis->GetDofSetProxy();
          // build a proxy of the fluid discretization for the structure field
          Teuchos::RCP<CORE::Dofsets::DofSetInterface> fluiddofset = fluiddis->GetDofSetProxy();

          // check if fluid_field has 2 discretizations, so that coupling is possible
          if (fluiddis->AddDofSet(structdofset) != 1)
            FOUR_C_THROW("unexpected dof sets in fluid field");
          if (structdis->AddDofSet(fluiddofset) != 1)
            FOUR_C_THROW("unexpected dof sets in structure field");
        }

        structdis->fill_complete();
        fluiddis->fill_complete();
      }
      else
      {
        if (matchinggrid)
        {
          FOUR_C_THROW(
              "MATCHINGGRID is set to 'yes' in POROELASTICITY DYNAMIC section, but fluid "
              "discretization is not empty!");
        }

        // first call fill_complete for single discretizations.
        // This way the physical dofs are numbered successively
        structdis->fill_complete();
        fluiddis->fill_complete();

        // build auxiliary dofsets, i.e. pseudo dofs on each discretization
        const int ndofpernode_fluid = GLOBAL::Problem::Instance()->NDim() + 1;
        const int ndofperelement_fluid = 0;
        const int ndofpernode_struct = GLOBAL::Problem::Instance()->NDim();
        const int ndofperelement_struct = 0;

        Teuchos::RCP<CORE::Dofsets::DofSetInterface> dofsetaux;
        dofsetaux = Teuchos::rcp(new CORE::Dofsets::DofSetPredefinedDoFNumber(
            ndofpernode_fluid, ndofperelement_fluid, 0, true));
        if (structdis->AddDofSet(dofsetaux) != 1)
          FOUR_C_THROW("unexpected dof sets in structure field");
        dofsetaux = Teuchos::rcp(new CORE::Dofsets::DofSetPredefinedDoFNumber(
            ndofpernode_struct, ndofperelement_struct, 0, true));
        if (fluiddis->AddDofSet(dofsetaux) != 1) FOUR_C_THROW("unexpected dof sets in fluid field");

        // call assign_degrees_of_freedom also for auxiliary dofsets
        // note: the order of fill_complete() calls determines the gid numbering!
        // 1. structure dofs
        // 2. fluiddis dofs
        // 3. structure auxiliary dofs
        // 4. fluiddis auxiliary dofs
        structdis->fill_complete(true, false, false);
        fluiddis->fill_complete(true, false, false);
      }
    }
  }  // namespace UTILS
}  // namespace POROELAST

FOUR_C_NAMESPACE_CLOSE

#endif
