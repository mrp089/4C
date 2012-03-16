/*----------------------------------------------------------------------*/
/*!
\file stru_resulttest.cpp

\brief tesing of structure calculation results

<pre>
Maintainer: Alexander Popp
            popp@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15238
</pre>
*/
/*----------------------------------------------------------------------*/

#ifdef PARALLEL
#include <mpi.h>
#endif

#include <string>
#include "stru_resulttest.H"
#include "strtimint.H"
#include "../drt_lib/drt_linedefinition.H"
#include "../drt_lib/drt_discret.H"


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
StruResultTest::StruResultTest(Teuchos::RCP<DRT::Discretization> strudis_in,
                               Teuchos::RCP<Epetra_Vector> dis,
                               Teuchos::RCP<Epetra_Vector> vel,
                               Teuchos::RCP<Epetra_Vector> acc)
{
  strudisc_ = strudis_in;
  dis_ = dis;
  vel_ = vel;
  acc_ = acc;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
StruResultTest::StruResultTest(STR::TimInt& tintegrator)
{
  dis_ = tintegrator.Dis();
  vel_ = tintegrator.Vel();
  acc_ = tintegrator.Acc();
  strudisc_ = tintegrator.Discretization();
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void StruResultTest::TestNode(DRT::INPUT::LineDefinition& res, int& nerr, int& test_count)
{
  int dis;
  res.ExtractInt("DIS",dis);
  if (dis != 1)
    dserror("fix me: only one structure discretization supported for testing");

  /* this implementation does not allow testing of stresses
   */
  int node;
  res.ExtractInt("NODE",node);
  node -= 1;
  if (strudisc_->HaveGlobalNode(node))
  {
    const DRT::Node* actnode = strudisc_->gNode(node);

    // Strange! It seems we might actually have a global node around
    // even if it does not belong to us. But here we are just
    // interested in our nodes!
    if (actnode->Owner() != strudisc_->Comm().MyPID())
      return;

    // verbose output
    //cout << "TESTING STRUCTURE RESULTS with StruResultTest::TestNode(..)" << endl;

    string position;
    res.ExtractString("POSITION",position);
    bool unknownpos = true;  // make sure the result value string can be handled
    double result = 0.0;  // will hold the actual result of run

    // test displacements or pressure
    if (dis_ != null)
    {
      const Epetra_BlockMap& disnpmap = dis_->Map();
      if (position=="dispx")
      {
        unknownpos = false;
        result = (*dis_)[disnpmap.LID(strudisc_->Dof(0,actnode,0))];
      }
      else if (position=="dispy")
      {
        unknownpos = false;
        result = (*dis_)[disnpmap.LID(strudisc_->Dof(0,actnode,1))];
      }
      else if (position=="dispz")
      {
        unknownpos = false;
        result = (*dis_)[disnpmap.LID(strudisc_->Dof(0,actnode,2))];
      }
      else if (position=="press")
      {
        unknownpos = false;
        result = (*dis_)[disnpmap.LID(strudisc_->Dof(0,actnode,3))];
      }
    }

    // test velocities
    if (vel_ != null)
    {
      const Epetra_BlockMap& velnpmap = vel_->Map();

      if (position=="velx")
      {
        unknownpos = false;
        result = (*vel_)[velnpmap.LID(strudisc_->Dof(0,actnode,0))];
      }
      else if (position=="vely")
      {
        unknownpos = false;
        result = (*vel_)[velnpmap.LID(strudisc_->Dof(0,actnode,1))];
      }
      else if (position=="velz")
      {
        unknownpos = false;
        result = (*vel_)[velnpmap.LID(strudisc_->Dof(0,actnode,2))];
      }
    }

    // test accelerations
    if (acc_ != null)
    {
      const Epetra_BlockMap& accnpmap = acc_->Map();

      if (position=="accx")
      {
        unknownpos = false;
        result = (*acc_)[accnpmap.LID(strudisc_->Dof(0,actnode,0))];
      }
      else if (position=="accy")
      {
        unknownpos = false;
        result = (*acc_)[accnpmap.LID(strudisc_->Dof(0,actnode,1))];
      }
      else if (position=="accz")
      {
        unknownpos = false;
        result = (*acc_)[accnpmap.LID(strudisc_->Dof(0,actnode,2))];
      }
    }

    // catch position strings, which are not handled by structure result test
    if (unknownpos)
      dserror("position '%s' not supported in structure testing", position.c_str());

    // compare values
    const int err = CompareValues(result, res);
    nerr += err;
    test_count++;

    // verbose output
    cout.precision(16);
    cout << "RESULT "  << test_count
         << " IS " << std::scientific << result
         << " AND " << ((err==0) ? "OKAY" : "INCORRECT")
         << endl;
  }
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
bool StruResultTest::Match(DRT::INPUT::LineDefinition& res)
{
  return res.HaveNamed("STRUCTURE");
}
