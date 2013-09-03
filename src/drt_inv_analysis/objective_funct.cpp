/*----------------------------------------------------------------------*/
/*!
 * \file objective_funct.cpp

<pre>
Maintainer: Sebastian Kehl
            kehl@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
</pre>
*/
/*----------------------------------------------------------------------*/


#include "objective_funct.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_io/io_control.H"
#include "Epetra_MultiVector.h"

#include "matpar_manager.H"


/*----------------------------------------------------------------------*/
/* standard constructor */
STR::INVANA::ObjectiveFunct::ObjectiveFunct(Teuchos::RCP<DRT::Discretization> discret,
                                            int steps,
                                            Teuchos::RCP<std::vector<double> > timesteps):
discret_(discret),
timesteps_(timesteps),
msteps_(steps)
{
  const Teuchos::ParameterList& invap = DRT::Problem::Instance()->StatInverseAnalysisParams();

  if (not discret_->Filled() || not discret_->HaveDofs())
    dserror("Discretisation is not complete or has no dofs!");
  else
    dofrowmap_ = discret_->DofRowMap();

  // initialize vectors
  mdisp_ = Teuchos::rcp(new Epetra_MultiVector(*dofrowmap_,msteps_,true));
  //mask_ = Teuchos::rcp(new Epetra_MultiVector(*dofrowmap_,msteps_,true));
  mask_ = Teuchos::rcp(new Epetra_Vector(*dofrowmap_,true));

  ReadMonitor(invap.get<std::string>("MONITORFILE"));

}

/*----------------------------------------------------------------------*/
/* read monitor file */
void STR::INVANA::ObjectiveFunct::ReadMonitor(std::string monitorfilename)
{
  int myrank = discret_->Comm().MyPID();

  int ndofs = 0;
  int nsteps = 0;
  int nnodes = 0;

  // list of node gids observed
  std::vector<int> nodes;
  // list of dofs on each node that are monitored
  std::vector<std::vector<int> > dofs;
  // measured displacement of the experiments (target value)
  Epetra_SerialDenseVector mcurve_;
  // time values where displacements are measured
  double timestep = 0.0;

  char* foundit = NULL;
  if (monitorfilename=="none.monitor") dserror("No monitor file provided");
  // insert path to monitor file if necessary
  if (monitorfilename[0]!='/')
  {
    std::string filename = DRT::Problem::Instance()->OutputControlFile()->InputFileName();
    std::string::size_type pos = filename.rfind('/');
    if (pos!=std::string::npos)
    {
      std::string path = filename.substr(0,pos+1);
      monitorfilename.insert(monitorfilename.begin(), path.begin(), path.end());
    }
  }

  FILE* file = fopen(monitorfilename.c_str(),"rb");
  if (file==NULL) dserror("Could not open monitor file %s",monitorfilename.c_str());

  char buffer[150000];
  fgets(buffer,150000,file);

  // read steps
  foundit = strstr(buffer,"steps"); foundit += strlen("steps");
  nsteps = strtol(foundit,&foundit,10);
  if ( nsteps > msteps_ )
    dserror("number of measured time steps greater than simulated time steps");

  // read nnodes
  foundit = strstr(buffer,"nnodes");
  foundit += strlen("nnodes");
  nnodes = strtol(foundit,&foundit,10);

  // read nodes
  nodes.resize(nnodes);
  dofs.resize(nnodes);
  for (int i=0; i<nnodes; ++i)
  {
    fgets(buffer,150000,file);
    foundit = buffer;
    nodes[i] = strtol(foundit,&foundit,10);
    int ndofs_act = strtol(foundit,&foundit,10);
    ndofs += ndofs_act;
    dofs[i].resize(ndofs,-1);
    DRT::Node* actnode = discret_->gNode(nodes[i]);
    std::vector<int> actdofs = discret_->Dof(actnode);

    if (!myrank) printf("Monitored node %d ndofs %d dofs ",nodes[i],(int)dofs[i].size());
    for (int j=0; j<ndofs; ++j)
    {
      //dofs[i][j] = strtol(foundit,&foundit,10);
      dofs[i][j] = actdofs[strtol(foundit,&foundit,10)];
      if (!myrank) printf("%d ",dofs[i][j]);
    }
    if (!myrank) printf("\n");
    ndofs = 0;
  }

  // read comment lines
    foundit = buffer;
    fgets(buffer,150000,file);
    while(strstr(buffer,"#"))
      fgets(buffer,150000,file);

    // read in the values for each node in dirs directions
    bool readnext = true;
    int count = 0;
    for (int i=0; i<msteps_; i++)
    {
      // read the next time step in the monitor file
      if (readnext)
        timestep = strtod(foundit,&foundit);

      //check whether this timestep was simulated:
      if ( (timestep-(*timesteps_)[i]) <= 1.0e-10 )
      {
        count += 1;
        readnext = true;
      }
      else
      {
        readnext = false;
        continue;
      }

      for (int j=0; j<nnodes; j++)
      {
        for (int k=0; k<(int)dofs[j].size(); k++)
        {
          mdisp_->ReplaceGlobalValue(dofs[j][k],i,strtod(foundit,&foundit));
          //assuming equal number of measured dofs for each time step
          //mask_ can be filled only once
          //mask_->ReplaceGlobalValue(dofs[j][k],i,1.0);
          mask_->ReplaceGlobalValue(dofs[j][k],0,1.0);
        }
      }

      fgets(buffer,150000,file);
      foundit = buffer;
    }

    // check whether reading was successful
    if ( nsteps != count )
      dserror("check your monitor file for consistency");

}

/*----------------------------------------------------------------------*/
/* Evaluate value of the objective function */
double STR::INVANA::ObjectiveFunct::Evaluate(Teuchos::RCP<Epetra_MultiVector> disp,
                                           Teuchos::RCP<MatParManager> matman)
{
  double val;

  Epetra_SerialDenseVector normvec(disp->NumVectors());

  Epetra_MultiVector tmpvec = Epetra_MultiVector(*dofrowmap_,msteps_,true);

  // tmpvec = u_sim - u_meas;
  tmpvec.Update(1.0,*mdisp_,0.0);
  tmpvec.Multiply(1.0,*mask_,*disp,-1.0);
  // (u_sim - u_meas)^2 for every element in the vector
  tmpvec.Multiply(1.0,tmpvec,tmpvec,0.0);
  // sum over every vector in the multivector
  tmpvec.Norm1(normvec.Values());
  // sum every entry
  val = 0.5*normvec.Norm1();

  //std::cout << "mdisp: " << *mdisp_ << std::endl;
  //std::cout << "disp: " << *disp << std::endl;
  //std::cout << "mask: " << *mask_ << std::endl;

  //add Thikonov regularization on the parameter vector:
  normvec.Scale(0.0);
  Epetra_MultiVector tmpvec2 = Epetra_MultiVector(*matman->GetInitialGuess());
  std::cout << *matman->GetParams() << std::endl;
  std::cout << tmpvec2 << std::endl;
  tmpvec2.Update(1.0,*(matman->GetParams()),-1.0);
  std::cout << tmpvec2 << std::endl;
  tmpvec2.Multiply(1.0,tmpvec2,tmpvec2,0.0);
  std::cout << tmpvec2 << std::endl;
  tmpvec2.Norm1(normvec.Values());
  val += 0.5*(matman->GetRegWeight())*normvec.Norm1();

  return val;
}

/*----------------------------------------------------------------------*/
/* Evaluate the gradient of the objective function w.r.t the displacements */
void STR::INVANA::ObjectiveFunct::EvaluateGradient(Teuchos::RCP<Epetra_MultiVector> disp,
                                                   Teuchos::RCP<Epetra_MultiVector> gradient)
{
  Epetra_MultiVector tmpvec = Epetra_MultiVector(*dofrowmap_,msteps_,true);
  tmpvec.Update(1.0,*mdisp_,0.0);

  tmpvec.Multiply(1.0,*mask_,*disp,-1.0);
  gradient->Update(1.0,tmpvec,0.0);

  //std::cout << "adjoint rhs " << *gradient << std::endl;
  return;
}
