/*---------------------------------------------------------------------------*/
/*!
\file particle_wall_result_test.cpp

\brief particle wall result test for particle simulations

\level 3

\maintainer  Sebastian Fuchs
             fuchs@lnm.mw.tum.de
             http://www.lnm.mw.tum.de
             089 - 289 -15262

*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | headers                                                    sfuchs 03/2019 |
 *---------------------------------------------------------------------------*/
#include "particle_wall_result_test.H"

#include "../drt_particle_algorithm/particle_wall_interface.H"

#include "../drt_lib/drt_linedefinition.H"
#include "../drt_lib/drt_discret.H"

/*---------------------------------------------------------------------------*
 | constructor                                                sfuchs 03/2019 |
 *---------------------------------------------------------------------------*/
PARTICLEALGORITHM::WallResultTest::WallResultTest() : DRT::ResultTest("PARTICLEWALL")
{
  // empty constructor
}

/*---------------------------------------------------------------------------*
 | init wall result test                                      sfuchs 03/2019 |
 *---------------------------------------------------------------------------*/
void PARTICLEALGORITHM::WallResultTest::Init()
{
  // nothing to do
}

/*---------------------------------------------------------------------------*
 | setup wall result test                                     sfuchs 03/2019 |
 *---------------------------------------------------------------------------*/
void PARTICLEALGORITHM::WallResultTest::Setup(
    const std::shared_ptr<PARTICLEALGORITHM::WallHandlerInterface> particlewallinterface)
{
  // set interface to particle wall handler
  particlewallinterface_ = particlewallinterface;

  // get wall discretization
  walldiscretization_ = particlewallinterface_->GetWallDiscretization();
}

/*---------------------------------------------------------------------------*
 | test node value                                            sfuchs 03/2019 |
 *---------------------------------------------------------------------------*/
void PARTICLEALGORITHM::WallResultTest::TestNode(
    DRT::INPUT::LineDefinition& res, int& nerr, int& test_count)
{
  // extract and check discretization name
  std::string dis;
  res.ExtractString("DIS", dis);
  if (dis != walldiscretization_->Name()) return;

  // extract node id
  int node;
  res.ExtractInt("NODE", node);
  node -= 1;

  int havenode(walldiscretization_->HaveGlobalNode(node));
  int havenodeonanyproc(0);
  walldiscretization_->Comm().SumAll(&havenode, &havenodeonanyproc, 1);

  // safety check
  if (not havenodeonanyproc)
    dserror("node %d does not belong to discretization %s", node + 1,
        walldiscretization_->Name().c_str());


  if (walldiscretization_->HaveGlobalNode(node))
  {
    const DRT::Node* actnode = walldiscretization_->gNode(node);

    // node not owned on this processor
    if (actnode->Owner() != walldiscretization_->Comm().MyPID()) return;

    // extract test quantity
    std::string quantity;
    res.ExtractString("QUANTITY", quantity);

    // init actual result
    double actresult = 0.0;

    // position
    if (quantity == "posx" or quantity == "posy" or quantity == "posz")
    {
      // get wall displacements
      Teuchos::RCP<const Epetra_Vector> disnp = particlewallinterface_->GetDisnp();

      int idx = -1;
      if (quantity == "posx")
        idx = 0;
      else if (quantity == "posy")
        idx = 1;
      else if (quantity == "posz")
        idx = 2;

      if (idx >= 0)
      {
        actresult = actnode->X()[idx];

        if (disnp != Teuchos::null)
        {
          const Epetra_BlockMap& disnpmap = disnp->Map();
          int lid = disnpmap.LID(walldiscretization_->Dof(0, actnode, idx));
          if (lid < 0)
            dserror("You tried to test %s on nonexistent dof %d on node %d", quantity.c_str(), idx,
                actnode->Id());
          actresult += (*disnp)[lid];
        }
      }
    }
    // displacement
    else if (quantity == "dispx" or quantity == "dispy" or quantity == "dispz")
    {
      // get wall displacements
      Teuchos::RCP<const Epetra_Vector> disnp = particlewallinterface_->GetDisnp();
      if (disnp == Teuchos::null) return;

      int idx = -1;
      if (quantity == "dispx")
        idx = 0;
      else if (quantity == "dispy")
        idx = 1;
      else if (quantity == "dispz")
        idx = 2;

      if (idx >= 0)
      {
        const Epetra_BlockMap& disnpmap = disnp->Map();
        int lid = disnpmap.LID(walldiscretization_->Dof(0, actnode, idx));
        if (lid < 0)
          dserror("You tried to test %s on nonexistent dof %d on node %d", quantity.c_str(), idx,
              actnode->Id());
        actresult = (*disnp)[lid];
      }
    }
    else
      dserror("result check failed with unknown quantity '%s'!", quantity.c_str());

    // compare values
    const int err = CompareValues(actresult, "NODE", res);
    nerr += err;
    test_count++;
  }
}

/*---------------------------------------------------------------------------*
 | test special quantity                                      sfuchs 03/2019 |
 *---------------------------------------------------------------------------*/
void PARTICLEALGORITHM::WallResultTest::TestSpecial(
    DRT::INPUT::LineDefinition& res, int& nerr, int& test_count)
{
  // check results only for processor 0
  if (walldiscretization_->Comm().MyPID() != 0) return;

  // extract and check discretization name
  std::string dis;
  res.ExtractString("DIS", dis);
  if (dis != walldiscretization_->Name()) return;

  // extract test quantity
  std::string quantity;
  res.ExtractString("QUANTITY", quantity);

  // init actual result
  double actresult = 0.0;

  // number of total wall elements
  if (quantity == "nwalleles") actresult = walldiscretization_->NumGlobalElements();
  // number of total wall nodes
  else if (quantity == "nwallnodes")
    actresult = walldiscretization_->NumGlobalNodes();
  else
    dserror("result check failed with unknown quantity '%s'!", quantity.c_str());

  // compare values
  const int err = CompareValues(actresult, "SPECIAL", res);
  nerr += err;
  test_count++;
}
