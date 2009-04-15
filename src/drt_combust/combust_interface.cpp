/*!-----------------------------------------------------------------------------------------------*
 \file combust_interface.cpp

 \brief interface handle that transports the intersection related things around for combustion problems

  detailed description in header file combust_interface.H

<pre>
Maintainer: Florian Henke
            henke@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15265
</pre>
 *------------------------------------------------------------------------------------------------*/
#ifdef CCADISCRET

#include "combust_interface.H"

#include "../drt_lib/standardtypes_cpp.H"
#include <Teuchos_TimeMonitor.hpp>
#include <Teuchos_StandardParameterEntryValidators.hpp>
#include "../drt_lib/drt_globalproblem.H"
 #include "../drt_lib/drt_utils.H"

// #include "../drt_io/io_gmsh.H"
// #include "../drt_io/io_gmsh_xfem_extension.H"
// #include "../drt_geometry/integrationcell.H"


/*------------------------------------------------------------------------------------------------*
 | constructor                                                                         henke 10/08 | 
 *------------------------------------------------------------------------------------------------*/
COMBUST::InterfaceHandleCombust::InterfaceHandleCombust(
    const Teuchos::RCP<DRT::Discretization> fluiddis,
    const Teuchos::RCP<const DRT::Discretization> gfuncdis,
    const Teuchos::RCP<const COMBUST::FlameFront> flamefront
    ) : InterfaceHandle(fluiddis),
        gfuncdis_(gfuncdis),
        flamefront_(flamefront)
{
  if (fluiddis->Comm().MyPID() == 0)
    std::cout << "Construct InterfaceHandleCombust" << std::endl;

/* Ich muss erstmal schauen, ob die DomainIntCell für meine Zwecke sinnvoll ist. Falls nicht, steht
 * das ganze InterfaceHanlde in Frage. Es könnte auch alles in die FlameFront integriert werden.
 * 
 * henke 03/09 */

  std::cout << "Hier passiert absolut nichts" << std::endl;
  // Dinge, die hier passieren müssen, sind in diesen Funktionen zu finden:
  // computeIntersection
  // computePLC
  // computeCDT

  if (fluiddis->Comm().MyPID() == 0)
    std::cout << "Construct InterfaceHandleCombust done" << std::endl;

  // Kläre was mit diesen Bäumen passieren soll!
  //  octTreenp_ = rcp( new GEO::SearchTree(5));
  //  octTreenp_->initializeTree(AABB, elementsByLabel_, GEO::TreeType(GEO::OCTTREE));
  //  octTreen_ = rcp( new GEO::SearchTree(5));
  //  octTreen_->initializeTree(AABB, elementsByLabel_, GEO::TreeType(GEO::OCTTREE));
}
/*------------------------------------------------------------------------------------------------*
 | destructor                                                                         henke 10/08 | 
 *------------------------------------------------------------------------------------------------*/
COMBUST::InterfaceHandleCombust::~InterfaceHandleCombust()
{
  return;
}

//! implement this function if needed for combustion!
void COMBUST::InterfaceHandleCombust::toGmsh(const int step) const
{
  dserror ("not implemented");
  return;
}

void COMBUST::InterfaceHandleCombust::UpdateInterfaceHandle()
{
  // das ist die Funktion, die die Integrationszellen tatsächlich bauen soll.
  dserror("UpdateInterfaceHandle not ready yet!");
}

//! implement this function if needed for combustion!
int COMBUST::InterfaceHandleCombust::PositionWithinConditionNP(const LINALG::Matrix<3,1>& x_in) const
{
  dserror("not implemented");
  return 0;
}

//! implement this function if needed for combustion!
int COMBUST::InterfaceHandleCombust::PositionWithinConditionN(const LINALG::Matrix<3,1>& x_in) const
{
  dserror("not implemented");
  return 0;
}

//! implement this function if needed for combustion!
int COMBUST::InterfaceHandleCombust::PositionWithinConditionNP(const LINALG::Matrix<3,1>&     x_in,
                              GEO::NearestObject&  nearestobject) const
{
  dserror("not implemented");
  return 0;
}

//! implement this function if needed for combustion!
int COMBUST::InterfaceHandleCombust::PositionWithinConditionN(const LINALG::Matrix<3,1>&     x_in,
                             GEO::NearestObject&  nearestobject) const
{
  dserror("not implemented");
  return 0;
}

#endif // #ifdef CCADISCRET
