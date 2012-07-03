/*!----------------------------------------------------------------------
\file so3_poro_evaluate.cpp
\brief

<pre>
   Maintainer: Anh-Tu Vuong
               vuong@lnm.mw.tum.de
               http://www.lnm.mw.tum.de
               089 - 289-15264
</pre>

*----------------------------------------------------------------------*/

#include "so3_poro.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_lib/drt_dserror.H"
#include "../drt_lib/drt_timecurve.H"
#include "../linalg/linalg_utils.H"
#include "../linalg/linalg_serialdensevector.H"
#include "Epetra_SerialDenseSolver.h"
#include "../drt_mat/robinson.H"
#include "../drt_mat/micromaterial.H"
#include <iterator>

#include "../drt_mat/fluidporo.H"
#include "../drt_mat/structporo.H"
#include "../drt_inpar/inpar_structure.H"
#include "../drt_fem_general/drt_utils_fem_shapefunctions.H"
#include "../drt_fem_general/drt_utils_gder2.H"
#include "../drt_lib/drt_globalproblem.H"

//#include "Sacado.hpp"

/*----------------------------------------------------------------------*
 |  preevaluate the element (public)                                       |
 *----------------------------------------------------------------------*/
template<class so3_ele, DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::So3_Poro<so3_ele,distype>::PreEvaluate(ParameterList& params,
                                        DRT::Discretization&      discretization,
                                        DRT::Element::LocationArray& la)
{
    return;
}

/*----------------------------------------------------------------------*
 |  evaluate the element (public)                                       |
 *----------------------------------------------------------------------*/
template<class so3_ele, DRT::Element::DiscretizationType distype>
int DRT::ELEMENTS::So3_Poro< so3_ele, distype>::Evaluate(ParameterList& params,
                                    DRT::Discretization&      discretization,
                                    DRT::Element::LocationArray& la,
                                    Epetra_SerialDenseMatrix& elemat1_epetra,
                                    Epetra_SerialDenseMatrix& elemat2_epetra,
                                    Epetra_SerialDenseVector& elevec1_epetra,
                                    Epetra_SerialDenseVector& elevec2_epetra,
                                    Epetra_SerialDenseVector& elevec3_epetra)
{
  // start with "none"
  typename So3_Poro::ActionType act = So3_Poro::none;

  // get the required action
  string action = params.get<string>("action","none");
  if (action == "none") dserror("No action supplied");
  else if (action=="calc_struct_multidofsetcoupling")   act = So3_Poro::calc_struct_multidofsetcoupling;
  // what should the element do
  switch(act)
  {
  //==================================================================================
  // coupling terms in force-vector and stiffness matrix
  case So3_Poro::calc_struct_multidofsetcoupling:
  {
    MyEvaluate(params,
                      discretization,
                      la,
                      elemat1_epetra,
                      elemat2_epetra,
                      elevec1_epetra,
                      elevec2_epetra,
                      elevec3_epetra);
  }
  break;
  //==================================================================================
  default:
  {
    //in some cases we need to write/change some data before evaluating
    PreEvaluate(params,
                      discretization,
                      la);

    so3_ele::Evaluate(params,
                      discretization,
                      la[0].lm_,
                      elemat1_epetra,
                      elemat2_epetra,
                      elevec1_epetra,
                      elevec2_epetra,
                      elevec3_epetra);

    MyEvaluate(params,
                      discretization,
                      la,
                      elemat1_epetra,
                      elemat2_epetra,
                      elevec1_epetra,
                      elevec2_epetra,
                      elevec3_epetra);
  }
  } // action

  return 0;
}

/*----------------------------------------------------------------------*
 |  evaluate the element (public)                                       |
 *----------------------------------------------------------------------*/
template<class so3_ele, DRT::Element::DiscretizationType distype>
int DRT::ELEMENTS::So3_Poro<so3_ele,distype>::MyEvaluate(ParameterList& params,
                                    DRT::Discretization&      discretization,
                                    DRT::Element::LocationArray& la,
                                    Epetra_SerialDenseMatrix& elemat1_epetra,
                                    Epetra_SerialDenseMatrix& elemat2_epetra,
                                    Epetra_SerialDenseVector& elevec1_epetra,
                                    Epetra_SerialDenseVector& elevec2_epetra,
                                    Epetra_SerialDenseVector& elevec3_epetra)
{

  // start with "none"
  ActionType act = none;

  // get the required action
  string action = params.get<string>("action","none");
  if (action == "none") dserror("No action supplied");
  else if (action=="calc_struct_update_istep")          act = calc_struct_update_istep;
  else if (action=="calc_struct_internalforce")         act = calc_struct_internalforce;
  else if (action=="calc_struct_nlnstiff")              act = calc_struct_nlnstiff;
  else if (action=="calc_struct_nlnstiffmass")          act = calc_struct_nlnstiffmass;
  else if (action=="calc_struct_multidofsetcoupling")   act = calc_struct_multidofsetcoupling;
  //else if (action=="postprocess_stress")                act = postprocess_stress;
  else dserror("Unknown type of action for So3_Poro: %s",action.c_str());
  // what should the element do
  switch(act)
  {
  //==================================================================================
  // nonlinear stiffness, damping and internal force vector for poroelasticity
  case calc_struct_nlnstiff:
  {
    // stiffness
    LINALG::Matrix<numdof_,numdof_> elemat1(elemat1_epetra.A(),true);
    //damping
    LINALG::Matrix<numdof_,numdof_> elemat2(elemat2_epetra.A(),true);
    // internal force vector
    LINALG::Matrix<numdof_,1> elevec1(elevec1_epetra.A(),true);
    LINALG::Matrix<numdof_,1> elevec2(elevec2_epetra.A(),true);
    // elemat2,elevec2+3 are not used anyway

    // need current displacement, velocities and residual forces
    Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState(0,"displacement");
    Teuchos::RCP<const Epetra_Vector> res = discretization.GetState(0,"residual displacement");

    if (disp==null )
      dserror("calc_struct_nlnstiff: Cannot get state vector 'displacement' ");
    // build the location vector only for the structure field
    vector<int> lm = la[0].lm_;

    vector<double> mydisp((lm).size());
    DRT::UTILS::ExtractMyValues(*disp,mydisp,lm); // global, local, lm

    vector<double> myres((lm).size());
    DRT::UTILS::ExtractMyValues(*res,myres,lm);

    LINALG::Matrix<numdof_,numdof_>* matptr = NULL;
    if (elemat1.IsInitialized()) matptr = &elemat1;

    enum INPAR::STR::DampKind damping = params.get<enum INPAR::STR::DampKind>("damping",INPAR::STR::damp_none);
    LINALG::Matrix<numdof_,numdof_>* matptr2 = NULL;
    if (elemat2.IsInitialized() and (damping==INPAR::STR::damp_material) ) matptr2 = &elemat2;

    // need current fluid state,
    // call the fluid discretization: fluid equates 2nd dofset
    // disassemble velocities and pressures

    vector<double> myvel((lm).size(),0.0);

    LINALG::Matrix<numdim_,numnod_> myfluidvel(true);
    LINALG::Matrix<numnod_,1> myepreaf(true);

    if(la.Size()>1)
    {
      //  dofs per node of second dofset
      const int numdofpernode = NumDofPerNode(1,*(so3_ele::Nodes()[0]));

      if (la[1].Size() != numnod_*numdofpernode)
        dserror("calc_struct_nlnstiff: Location vector length for velocities does not match!");

      if (discretization.HasState(0,"velocity"))
      {
        Teuchos::RCP<const Epetra_Vector> vel = discretization.GetState(0,"velocity");
        if (vel==null )
          dserror("calc_struct_nlnstiff: Cannot get state vector 'velocity' ");
        DRT::UTILS::ExtractMyValues(*vel,myvel,lm);
      }

      if (discretization.HasState(1,"fluidvel"))
      {
        // check if you can get the velocity state
        Teuchos::RCP<const Epetra_Vector> velnp
          = discretization.GetState(1,"fluidvel");
        //if there are no velocities or pressures
        if (velnp==Teuchos::null)
        {
          dserror("calc_struct_nlnstiff: Cannot get state vector 'fluidvel' ");
        }
        else
        {
          // extract local values of the global vectors
          std::vector<double> mymatrix(la[1].lm_.size());
          DRT::UTILS::ExtractMyValues(*velnp,mymatrix,la[1].lm_);

          for (int inode=0; inode<numnod_; ++inode) // number of nodes
          {
            for(int idim=0; idim<numdim_; ++idim) // number of dimensions
            {
              (myfluidvel)(idim,inode) = mymatrix[idim+(inode*numdofpernode)];
            } // end for(idim)

            (myepreaf)(inode,0) = mymatrix[numdim_+(inode*numdofpernode)];
          }
        }
      }

      //calculate tangent stiffness matrix
      nlnstiff_poroelast(lm,mydisp,myvel,myfluidvel,myepreaf,matptr,matptr2,&elevec1,//NULL,//NULL,NULL,
          params,
          INPAR::STR::stress_none,INPAR::STR::strain_none);
    }
  }
  break;

  //==================================================================================
  // nonlinear stiffness, mass matrix and internal force vector for poroelasticity
  case calc_struct_nlnstiffmass:
  {

    // stiffness
    LINALG::Matrix<numdof_,numdof_> elemat1(elemat1_epetra.A(),true);
    // mass
    LINALG::Matrix<numdof_,numdof_> elemat2(elemat2_epetra.A(),true);
    // internal force vector
    LINALG::Matrix<numdof_,1> elevec1(elevec1_epetra.A(),true);
    LINALG::Matrix<numdof_,1> elevec2(elevec2_epetra.A(),true);
    // elemat2,elevec2+3 are not used anyway

    // need current displacement, velocities and residual forces
    Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState(0,"displacement");
    Teuchos::RCP<const Epetra_Vector> res = discretization.GetState(0,"residual displacement");

    if (disp==null )
      dserror("calc_struct_nlnstiffmass: Cannot get state vector 'displacement' ");

    // build the location vector only for the structure field
    vector<int> lm = la[0].lm_;

    vector<double> mydisp((lm).size());
    DRT::UTILS::ExtractMyValues(*disp,mydisp,lm); // global, local, lm

    vector<double> myres((lm).size());
    DRT::UTILS::ExtractMyValues(*res,myres,lm);

    LINALG::Matrix<numdof_,numdof_>* matptr = NULL;
    if (elemat1.IsInitialized()) matptr = &elemat1;

    //get structure material
    MAT::StructPoro* structmat = static_cast<MAT::StructPoro*>((Material()).get());
    if(structmat->MaterialType() != INPAR::MAT::m_structporo)
      dserror("calc_struct_nlnstiffmass: invalid structure material for poroelasticity");

    const double initporosity = structmat->Initporosity();
    if(initporosity<0.0)
      dserror("calc_struct_nlnstiffmass: invalid initial porosity!");

    elemat2.Scale(1-initporosity);

    // need current fluid state,
    // call the fluid discretization: fluid equates 2nd dofset
    // disassemble velocities and pressures

    //  dof per node
    const int numdofpernode_ = NumDofPerNode(1,*(Nodes()[0]));

    vector<double> myvel((lm).size(),0.0);

    LINALG::Matrix<numdim_,numnod_> myfluidvel(true);
    LINALG::Matrix<numnod_,1> myepreaf(true);

    if(la.Size()>1)
    {
      if (la[1].Size() != numnod_*numdofpernode_)
        dserror("calc_struct_nlnstiffmass: Location vector length for velocities does not match!");

      Teuchos::RCP<const Epetra_Vector> vel = discretization.GetState(0,"velocity");
      if (vel==null )
        dserror("calc_struct_nlnstiffmass: Cannot get state vector 'velocity' ");
      DRT::UTILS::ExtractMyValues(*vel,myvel,lm);

      // check if you can get the velocity state
      Teuchos::RCP<const Epetra_Vector> velnp
        = discretization.GetState(1,"fluidvel");
      //if there are no velocities or pressures, set them to zero
      if (velnp==Teuchos::null)
      {
        //dserror("calc_struct_nlnstiffmass: Cannot get state vector 'fluidvel' ");
        //cout<<"WARNING: No fluid velocity found. Set to zero"<<endl;
        for (int inode=0; inode<numnod_; ++inode) // number of nodes
        {
          for(int idim=0; idim<numdim_; ++idim) // number of dimensions
          {
            (myfluidvel)(idim,inode) = 0.0;
          } // end for(idim)
        }
      }
      else
      {
        // extract local values of the global vectors
        std::vector<double> mymatrix(la[1].lm_.size());
        DRT::UTILS::ExtractMyValues(*velnp,mymatrix,la[1].lm_);

        for (int inode=0; inode<numnod_; ++inode) // number of nodes
        {
          for(int idim=0; idim<numdim_; ++idim) // number of dimensions
          {
            (myfluidvel)(idim,inode) = mymatrix[idim+(inode*numdofpernode_)];
          } // end for(idim)

          (myepreaf)(inode,0) = mymatrix[numdim_+(inode*numdofpernode_)];
        }
      }

      nlnstiff_poroelast(lm,mydisp,myvel,myfluidvel,myepreaf,matptr,NULL,&elevec1,//NULL,//NULL,NULL,
          params,
        INPAR::STR::stress_none,INPAR::STR::strain_none);
    }

  }
  break;

  //==================================================================================
  // coupling terms in force-vector and stiffness matrix for poroelasticity
  case calc_struct_multidofsetcoupling:
  {
    // stiffness
    LINALG::Matrix<numdof_,(numdim_+1)*numnod_> elemat1(elemat1_epetra.A(),true);
    //LINALG::Matrix<numdof_,(numdim_+1)*numnod_> elemat2(elemat2_epetra.A(),true);

    // internal force vector
    //LINALG::Matrix<numdof_,1> elevec1(elevec1_epetra.A(),true);
    //LINALG::Matrix<numdof_,1> elevec2(elevec2_epetra.A(),true);

    // elemat2,elevec2+3 are not used anyway

    // need current displacement, velocities and residual forces
    Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState(0,"displacement");

    if (disp==null )
      dserror("calc_struct_multidofsetcoupling: Cannot get state vector 'displacement' ");

    // build the location vector only for the structure field
    vector<int> lm = la[0].lm_;

    vector<double> mydisp((lm).size());
    DRT::UTILS::ExtractMyValues(*disp,mydisp,lm); // global, local, lm

    LINALG::Matrix<numdof_,(numdim_+1)*numnod_>* matptr = NULL;
    if (elemat1.IsInitialized()) matptr = &elemat1;

    // need current fluid state,
    // call the fluid discretization: fluid equates 2nd dofset
    // disassemble velocities and pressures
    if (discretization.HasState(1,"fluidvel"))
    {
      //  dof per node of fluid field
      const int numdofpernode_ = NumDofPerNode(1,*(Nodes()[0]));

      Teuchos::RCP<const Epetra_Vector> vel = discretization.GetState(0,"velocity");
      if (vel==null )
        dserror("calc_struct_multidofsetcoupling: Cannot get state vector 'velocity' ");
      vector<double> myvel((lm).size());
      DRT::UTILS::ExtractMyValues(*vel,myvel,lm);

      LINALG::Matrix<numdim_,numnod_> myvelnp(true);
      LINALG::Matrix<numnod_,1> myepreaf(true);

      // check if you can get the velocity state
      Teuchos::RCP<const Epetra_Vector> velnp
        = discretization.GetState(1,"fluidvel");
      if (velnp==Teuchos::null)
        dserror("Cannot get state vector 'fluidvel'");

      dsassert(la[1].Size() == numnod_*numdofpernode_,
              "Location vector length for fluid velocities and pressures does not match!");

      // extract the current velocitites and pressures of the global vectors
      std::vector<double> mymatrix(la[1].lm_.size());
      DRT::UTILS::ExtractMyValues(*velnp,mymatrix,la[1].lm_);

      for (int inode=0; inode<numnod_; ++inode) // number of nodes
      {
        for(int idim=0; idim<numdim_; ++idim) // number of dimensions
        {
          (myvelnp)(idim,inode) = mymatrix[idim+(inode*numdofpernode_)];
        } // end for(idim)

        (myepreaf)(inode,0) = mymatrix[numdim_+(inode*numdofpernode_)];
      }

      coupling_poroelast(lm,mydisp,myvel,myvelnp,myepreaf,matptr,//NULL,
          NULL,NULL,params);
    }

  }
  break;

  //==================================================================================
  // nonlinear stiffness and internal force vector for poroelasticity
  case calc_struct_internalforce:
  {
    // stiffness
    LINALG::Matrix<numdof_,numdof_> elemat1(elemat1_epetra.A(),true);
    LINALG::Matrix<numdof_,numdof_> elemat2(elemat2_epetra.A(),true);
    // internal force vector
    LINALG::Matrix<numdof_,1> elevec1(elevec1_epetra.A(),true);
    LINALG::Matrix<numdof_,1> elevec2(elevec2_epetra.A(),true);
    // elemat2,elevec2+3 are not used anyway

    // need current displacement, velocities and residual forces
    Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState(0,"displacement");
    Teuchos::RCP<const Epetra_Vector> res = discretization.GetState(0,"residual displacement");

    if (disp==null )
      dserror("Cannot get state vector 'displacement' ");

    // build the location vector only for the structure field
    vector<int> lm = la[0].lm_;

    vector<double> mydisp((lm).size());
    DRT::UTILS::ExtractMyValues(*disp,mydisp,lm); // global, local, lm

    vector<double> myres((lm).size());
    DRT::UTILS::ExtractMyValues(*res,myres,lm);

    // need current fluid state,
    // call the fluid discretization: fluid equates 2nd dofset
    // disassemble velocities and pressures

    if (discretization.HasState(1,"fluidvel"))
    {
      //  dof per node of second dofset
      const int numdofpernode_ = NumDofPerNode(1,*(Nodes()[0]));

      Teuchos::RCP<const Epetra_Vector> vel = discretization.GetState(0,"velocity");
      if (vel==null )
        dserror("Cannot get state vector 'velocity' ");
      vector<double> myvel((lm).size());
      DRT::UTILS::ExtractMyValues(*vel,myvel,lm);

      LINALG::Matrix<numdim_,numnod_> myfluidvel(true);
      LINALG::Matrix<numnod_,1> myepreaf(true);

      if (la[1].Size() != numnod_*numdofpernode_)
        dserror("Location vector length for velocities does not match!");

      // check if you can get the velocity state
      Teuchos::RCP<const Epetra_Vector> velnp
        = discretization.GetState(1,"fluidvel");

      if (velnp==Teuchos::null)
      {
        dserror("Cannot get state vector 'fluidvel' ");
      }
      else
      {
        // extract local values of the global vectors
        std::vector<double> mymatrix(la[1].lm_.size());
        DRT::UTILS::ExtractMyValues(*velnp,mymatrix,la[1].lm_);
        for (int inode=0; inode<numnod_; ++inode) // number of nodes
        {
          for(int idim=0; idim<numdim_; ++idim) // number of dimensions
          {
            (myfluidvel)(idim,inode) = mymatrix[idim+(inode*numdofpernode_)];
          } // end for(idim)

          (myepreaf)(inode,0) = mymatrix[numdim_+(inode*numdofpernode_)];
        }
      }

      nlnstiff_poroelast(lm,mydisp,myvel,myfluidvel,myepreaf,NULL,NULL,&elevec1,//NULL,//NULL,NULL,
          params,
          INPAR::STR::stress_none,INPAR::STR::strain_none);
    }
  }
  break;

  //==================================================================================
  case calc_struct_update_istep:
  {
    // Update of history for visco material if they exist
    RefCountPtr<MAT::Material> mat = Material();
    if (mat->MaterialType() == INPAR::MAT::m_struct_multiscale)
    {
      MAT::MicroMaterial* micro = static_cast <MAT::MicroMaterial*>(mat.get());
      micro->Update();
    }
    // incremental update of internal variables/history
    if (mat->MaterialType() == INPAR::MAT::m_vp_robinson)
    {
      MAT::Robinson* robinson = static_cast<MAT::Robinson*>(mat.get());
      robinson->Update();
    }
  }
  break;

  //==================================================================================
  default:
  dserror("Unknown type of action for So3_poro");
  } // action
  return 0;
}


/*----------------------------------------------------------------------*
 |  evaluate only the poroelasticity fraction for the element (private) |
 *----------------------------------------------------------------------*/
template<class so3_ele, DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::So3_Poro<so3_ele,distype>::nlnstiff_poroelast(
    vector<int>& lm, // location matrix
    vector<double>& disp, // current displacements
    vector<double>& vel, // current velocities
    // vector<double>&           residual,       // current residual displ
    LINALG::Matrix<numdim_, numnod_> & evelnp,
    LINALG::Matrix<numnod_, 1> & epreaf,
    LINALG::Matrix<numdof_, numdof_>* stiffmatrix, // element stiffness matrix
    LINALG::Matrix<numdof_, numdof_>* reamatrix, // element reactive matrix
    LINALG::Matrix<numdof_, 1>* force, // element internal force vector
 //   LINALG::Matrix<numdof_, 1>* forcerea, // element reactive force vector
 //   LINALG::Matrix<numgpt_, numstr_>* elestress, // stresses at GP
 //   LINALG::Matrix<numgpt_, numstr_>* elestrain, // strains at GP
    ParameterList& params, // algorithmic parameters e.g. time
    const INPAR::STR::StressType iostress, // stress output option
    const INPAR::STR::StrainType iostrain) // strain output option
{
  // get global id of the structure element
  int id = Id();
  //access fluid discretization
  RCP<DRT::Discretization> fluiddis = null;
  fluiddis = DRT::Problem::Instance()->GetDis("fluid");
  //get corresponding fluid element (it has the same global ID as the structure element)
  DRT::Element* fluidele = fluiddis->gElement(id);
  if (fluidele == NULL)
    dserror("Fluid element %i not on local processor", id);

  //get fluid material
  MAT::FluidPoro* fluidmat = static_cast<MAT::FluidPoro*>((fluidele->Material()).get());
  if(fluidmat->MaterialType() != INPAR::MAT::m_fluidporo)
    dserror("invalid fluid material for poroelasticity");

  //get structure material
  MAT::StructPoro* structmat = static_cast<MAT::StructPoro*>((Material()).get());
  if(structmat->MaterialType() != INPAR::MAT::m_structporo)
    dserror("invalid structure material for poroelasticity");

  double reacoeff = fluidmat->ComputeReactionCoeff();
  double dt = params.get<double>("delta time");

  // update element geometry
  LINALG::Matrix<numdim_,numnod_> xrefe; // material coord. of element
  LINALG::Matrix<numdim_,numnod_> xcurr; // current  coord. of element
  LINALG::Matrix<numnod_,numdim_> xdisp;

  DRT::Node** nodes = Nodes();
  for (int i=0; i<numnod_; ++i)
  {
    const double* x = nodes[i]->X();
    xrefe(0,i) = x[0];
    xrefe(1,i) = x[1];
    xrefe(2,i) = x[2];

    xcurr(0,i) = xrefe(0,i) + disp[i*noddof_+0];
    xcurr(1,i) = xrefe(1,i) + disp[i*noddof_+1];
    xcurr(2,i) = xrefe(2,i) + disp[i*noddof_+2];
  }

  LINALG::Matrix<numdof_,1> nodaldisp;
  for (int i=0; i<numdof_; ++i)
  {
    nodaldisp(i,0) = disp[i];
  }

  LINALG::Matrix<numdof_,1> nodalvel;
  for (int i=0; i<numdof_; ++i)
  {
    nodalvel(i,0) = vel[i];
  }

  //vector of porosity at gp (for output only)
  std::vector<double> porosity_gp(numgpt_,0.0);

  std::vector<LINALG::Matrix<numdim_,1> > gradporosity_gp (numgpt_);
  for(int i = 0 ; i < numgpt_ ; i++)
  {
    gradporosity_gp.at(i)(0) = 0.0;
    gradporosity_gp.at(i)(1) = 0.0;
    gradporosity_gp.at(i)(2) = 0.0;
  }

  //******************* FAD ************************

/*
   // sacado data type replaces "double"
   typedef Sacado::Fad::DFad<double> FAD;  // for first derivs
   // sacado data type replaces "double" (for first+second derivs)
   typedef Sacado::Fad::DFad<Sacado::Fad::DFad<double> > FADFAD;

   vector<FAD> fad_disp(numdof_);
   for (int i=0; i<numdof_; ++i)
   {
   fad_disp[i] = disp[i];
   fad_disp[i].diff(i,numdof_);   // variables to differentiate for
   }

   LINALG::TMatrix<FAD,numdim_,numnod_>  fad_xrefe(false);
   LINALG::TMatrix<FAD,numdim_,numnod_> fad_xcurr(false);

   for (int i=0; i<numnod_; ++i)
   {
   const double* x = nodes[i]->X();

   fad_xrefe(0,i) = x[0];
   fad_xrefe(1,i) = x[1];
   fad_xrefe(2,i) = x[2];

   fad_xcurr(0,i) = fad_xrefe(0,i) + fad_disp[i*noddof_+0];
   fad_xcurr(1,i) = fad_xrefe(1,i) + fad_disp[i*noddof_+1];
   fad_xcurr(2,i) = fad_xrefe(2,i) + fad_disp[i*noddof_+2];
   }

   LINALG::TMatrix<FAD,numdof_,1> fad_nodaldisp(false);
   LINALG::TMatrix<FAD,numnod_,1> fad_epreaf(false);
   for(int i=0; i<numnod_ ; i++)
   {
   fad_epreaf(i) = epreaf(i);
   // fad_epreaf(i).diff(numdof_+i,numdof_+numnod_);
   }

   for (int i=0; i<numdof_; ++i)
   fad_nodaldisp(i,0) = fad_disp[i];
*/
  //******************** FAD ***********************

  /* =========================================================================*/
  /* ================================================= Loop over Gauss Points */
  /* =========================================================================*/
  LINALG::Matrix<numdim_,numnod_> N_XYZ;
  LINALG::Matrix<6,numnod_> N_XYZ2;
  // build deformation gradient wrt to material configuration
  // in case of prestressing, build defgrd wrt to last stored configuration
  // CAUTION: defgrd(true): filled with zeros!
  LINALG::Matrix<numdim_,numdim_> defgrd(true);
  LINALG::Matrix<numnod_,1> shapefct;
  LINALG::Matrix<numdim_,numnod_> deriv ;
  LINALG::Matrix<6,numnod_> deriv2;

  for (int gp=0; gp<numgpt_; ++gp)
  {
    LINALG::Matrix<numdim_,numdim_> invJ = invJ_[gp];

    DRT::UTILS::shape_function<distype>(xsi_[gp],shapefct);
    DRT::UTILS::shape_function_deriv1<distype>(xsi_[gp],deriv);

    /* get the inverse of the Jacobian matrix which looks like:
     **            [ X_,r  Y_,r  Z_,r ]^-1
     **     J^-1 = [ X_,s  Y_,s  Z_,s ]
     **            [ X_,t  Y_,t  Z_,t ]
     */

    // compute derivatives N_XYZ at gp w.r.t. material coordinates
    // by N_XYZ = J^-1 * N_rst
    N_XYZ.Multiply(invJ_[gp],deriv); // (6.21)
    double detJ = detJ_[gp]; // (6.22)

    if( ishigherorder_ )
    {
      // transposed jacobian "dX/ds"
      LINALG::Matrix<numdim_,numdim_> xjm0;
      xjm0.MultiplyNT(deriv,xrefe);

      // get the second derivatives of standard element at current GP w.r.t. rst
      DRT::UTILS::shape_function_deriv2<distype>(xsi_[gp],deriv2);
      // get the second derivatives of standard element at current GP w.r.t. XYZ
      DRT::UTILS::gder2<distype>(xjm0,N_XYZ,deriv2,xrefe,N_XYZ2);
    }
    else
    {
      deriv2.Clear();
      N_XYZ2.Clear();
    }

    // get Jacobian matrix and determinant w.r.t. spatial configuration
    //! transposed jacobian "dx/ds"
    LINALG::Matrix<numdim_,numdim_> xjm;
    //! inverse of transposed jacobian "ds/dx"
    LINALG::Matrix<numdim_,numdim_> xji;
    xjm.MultiplyNT(deriv,xcurr);
    const double det = xji.Invert(xjm);

    // determinant of deformationgradient: det F = det ( d x / d X ) = det (dx/ds) * ( det(dX/ds) )^-1
    const double J = det/detJ;

    //----------------------------------------------------
    // pressure at integration point
    double press = shapefct.Dot(epreaf);

    // pressure gradient at integration point
    LINALG::Matrix<numdim_,1> Gradp;
    Gradp.Multiply(N_XYZ,epreaf);

    // fluid velocity at integration point
    LINALG::Matrix<numdim_,1> fvelint;
    fvelint.Multiply(evelnp,shapefct);

    // material fluid velocity gradient at integration point
    LINALG::Matrix<numdim_,numdim_>              fvelder;
    fvelder.MultiplyNT(evelnp,N_XYZ);

    // structure displacement and velocity at integration point
    LINALG::Matrix<numdim_,1> dispint(true);
    LINALG::Matrix<numdim_,1> velint(true);

    for(int i=0; i<numnod_; i++)
    for(int j=0; j<numdim_; j++)
    {
      dispint(j) += nodaldisp(i*numdim_+j) * shapefct(i);
      velint(j) += nodalvel(i*numdim_+j) * shapefct(i);
    }

    // (material) deformation gradient F = d xcurr / d xrefe = xcurr * N_XYZ^T
    defgrd.MultiplyNT(xcurr,N_XYZ); //  (6.17)

    /* non-linear B-operator (may so be called, meaning
     ** of B-operator is not so sharp in the non-linear realm) *
     ** B = F . Bl *
     **
     **      [ ... | F_11*N_{,1}^k  F_21*N_{,1}^k  F_31*N_{,1}^k | ... ]
     **      [ ... | F_12*N_{,2}^k  F_22*N_{,2}^k  F_32*N_{,2}^k | ... ]
     **      [ ... | F_13*N_{,3}^k  F_23*N_{,3}^k  F_33*N_{,3}^k | ... ]
     ** B =  [ ~~~   ~~~~~~~~~~~~~  ~~~~~~~~~~~~~  ~~~~~~~~~~~~~   ~~~ ]
     **      [       F_11*N_{,2}^k+F_12*N_{,1}^k                       ]
     **      [ ... |          F_21*N_{,2}^k+F_22*N_{,1}^k        | ... ]
     **      [                       F_31*N_{,2}^k+F_32*N_{,1}^k       ]
     **      [                                                         ]
     **      [       F_12*N_{,3}^k+F_13*N_{,2}^k                       ]
     **      [ ... |          F_22*N_{,3}^k+F_23*N_{,2}^k        | ... ]
     **      [                       F_32*N_{,3}^k+F_33*N_{,2}^k       ]
     **      [                                                         ]
     **      [       F_13*N_{,1}^k+F_11*N_{,3}^k                       ]
     **      [ ... |          F_23*N_{,1}^k+F_21*N_{,3}^k        | ... ]
     **      [                       F_33*N_{,1}^k+F_31*N_{,3}^k       ]
     */
    LINALG::Matrix<numstr_,numdof_> bop;
    for (int i=0; i<numnod_; ++i)
    {
      bop(0,noddof_*i+0) = defgrd(0,0)*N_XYZ(0,i);
      bop(0,noddof_*i+1) = defgrd(1,0)*N_XYZ(0,i);
      bop(0,noddof_*i+2) = defgrd(2,0)*N_XYZ(0,i);
      bop(1,noddof_*i+0) = defgrd(0,1)*N_XYZ(1,i);
      bop(1,noddof_*i+1) = defgrd(1,1)*N_XYZ(1,i);
      bop(1,noddof_*i+2) = defgrd(2,1)*N_XYZ(1,i);
      bop(2,noddof_*i+0) = defgrd(0,2)*N_XYZ(2,i);
      bop(2,noddof_*i+1) = defgrd(1,2)*N_XYZ(2,i);
      bop(2,noddof_*i+2) = defgrd(2,2)*N_XYZ(2,i);
      /* ~~~ */
      bop(3,noddof_*i+0) = defgrd(0,0)*N_XYZ(1,i) + defgrd(0,1)*N_XYZ(0,i);
      bop(3,noddof_*i+1) = defgrd(1,0)*N_XYZ(1,i) + defgrd(1,1)*N_XYZ(0,i);
      bop(3,noddof_*i+2) = defgrd(2,0)*N_XYZ(1,i) + defgrd(2,1)*N_XYZ(0,i);
      bop(4,noddof_*i+0) = defgrd(0,1)*N_XYZ(2,i) + defgrd(0,2)*N_XYZ(1,i);
      bop(4,noddof_*i+1) = defgrd(1,1)*N_XYZ(2,i) + defgrd(1,2)*N_XYZ(1,i);
      bop(4,noddof_*i+2) = defgrd(2,1)*N_XYZ(2,i) + defgrd(2,2)*N_XYZ(1,i);
      bop(5,noddof_*i+0) = defgrd(0,2)*N_XYZ(0,i) + defgrd(0,0)*N_XYZ(2,i);
      bop(5,noddof_*i+1) = defgrd(1,2)*N_XYZ(0,i) + defgrd(1,0)*N_XYZ(2,i);
      bop(5,noddof_*i+2) = defgrd(2,2)*N_XYZ(0,i) + defgrd(2,0)*N_XYZ(2,i);
    }

    // Right Cauchy-Green tensor = F^T * F
    LINALG::Matrix<numdim_,numdim_> cauchygreen;
    cauchygreen.MultiplyTN(defgrd,defgrd);

     // Green-Lagrange strains matrix E = 0.5 * (Cauchygreen - Identity)
     // GL strain vector glstrain={E11,E22,E33,2*E12,2*E23,2*E31}
     Epetra_SerialDenseVector glstrain_epetra(numstr_);
     LINALG::Matrix<numstr_,1> glstrain(glstrain_epetra.A(),true);
     glstrain(0) = 0.5 * (cauchygreen(0,0) - 1.0);
     glstrain(1) = 0.5 * (cauchygreen(1,1) - 1.0);
     glstrain(2) = 0.5 * (cauchygreen(2,2) - 1.0);
     glstrain(3) = cauchygreen(0,1);
     glstrain(4) = cauchygreen(1,2);
     glstrain(5) = cauchygreen(2,0);

    // inverse Right Cauchy-Green tensor
    LINALG::Matrix<numdim_,numdim_> C_inv(false);
    C_inv.Invert(cauchygreen);

    /*
    double b00 = cauchygreen(0,0);
    double b01 = cauchygreen(0,1);
    double b02 = cauchygreen(0,2);
    double b10 = cauchygreen(1,0);
    double b11 = cauchygreen(1,1);
    double b12 = cauchygreen(1,2);
    double b20 = cauchygreen(2,0);
    double b21 = cauchygreen(2,1);
    double b22 = cauchygreen(2,2);
    C_inv(0,0) =   b11*b22 - b21*b12;
    C_inv(1,0) = - b10*b22 + b20*b12;
    C_inv(2,0) =   b10*b21 - b20*b11;
    C_inv(0,1) = - b01*b22 + b21*b02;
    C_inv(1,1) =   b00*b22 - b20*b02;
    C_inv(2,1) = - b00*b21 + b20*b01;
    C_inv(0,2) =   b01*b12 - b11*b02;
    C_inv(1,2) = - b00*b12 + b10*b02;
    C_inv(2,2) =   b00*b11 - b10*b01;
    double det_C = b00*C_inv(0,0)+b01*C_inv(1,0)+b02*C_inv(2,0);
    C_inv.Scale(1./det_C);
    */

    //inverse Right Cauchy-Green tensor as vector
    LINALG::Matrix<numstr_,1> C_inv_vec(true);
    C_inv_vec(0) = C_inv(0,0);
    C_inv_vec(1) = C_inv(1,1);
    C_inv_vec(2) = C_inv(2,2);
    C_inv_vec(3) = C_inv(0,1);
    C_inv_vec(4) = C_inv(1,2);
    C_inv_vec(5) = C_inv(2,0);

    // inverse deformation gradient F^-1
    LINALG::Matrix<numdim_,numdim_> defgrd_inv(false);
    defgrd_inv.Invert(defgrd);

    /*
    double b00_F = defgrd(0,0);
    double b01_F = defgrd(0,1);
    double b02_F = defgrd(0,2);
    double b10_F = defgrd(1,0);
    double b11_F = defgrd(1,1);
    double b12_F = defgrd(1,2);
    double b20_F = defgrd(2,0);
    double b21_F = defgrd(2,1);
    double b22_F = defgrd(2,2);
    defgrd_inv(0,0) =   b11_F*b22_F - b21_F*b12_F;
    defgrd_inv(1,0) = - b10_F*b22_F + b20_F*b12_F;
    defgrd_inv(2,0) =   b10_F*b21_F - b20_F*b11_F;
    defgrd_inv(0,1) = - b01_F*b22_F + b21_F*b02_F;
    defgrd_inv(1,1) =   b00_F*b22_F - b20_F*b02_F;
    defgrd_inv(2,1) = - b00_F*b21_F + b20_F*b01_F;
    defgrd_inv(0,2) =   b01_F*b12_F - b11_F*b02_F;
    defgrd_inv(1,2) = - b00_F*b12_F + b10_F*b02_F;
    defgrd_inv(2,2) =   b00_F*b11_F - b10_F*b01_F;
    double det_F = b00_F*defgrd_inv(0,0)+b01_F*defgrd_inv(1,0)+b02_F*defgrd_inv(2,0);
    defgrd_inv.Scale(1./det_F);
    */

    //------------------------------------ build F^-1 as vector 9x1
    LINALG::Matrix<9,1> defgrd_inv_vec;
    defgrd_inv_vec(0)=defgrd_inv(0,0);
    defgrd_inv_vec(1)=defgrd_inv(0,1);
    defgrd_inv_vec(2)=defgrd_inv(0,2);
    defgrd_inv_vec(3)=defgrd_inv(1,0);
    defgrd_inv_vec(4)=defgrd_inv(1,1);
    defgrd_inv_vec(5)=defgrd_inv(1,2);
    defgrd_inv_vec(6)=defgrd_inv(2,0);
    defgrd_inv_vec(7)=defgrd_inv(2,1);
    defgrd_inv_vec(8)=defgrd_inv(2,2);

    //------------------------------------ build F^-T as vector 9x1
    LINALG::Matrix<9,1> defgrd_IT_vec;
    defgrd_IT_vec(0)=defgrd_inv(0,0);
    defgrd_IT_vec(1)=defgrd_inv(1,0);
    defgrd_IT_vec(2)=defgrd_inv(2,0);
    defgrd_IT_vec(3)=defgrd_inv(0,1);
    defgrd_IT_vec(4)=defgrd_inv(1,1);
    defgrd_IT_vec(5)=defgrd_inv(2,1);
    defgrd_IT_vec(6)=defgrd_inv(0,2);
    defgrd_IT_vec(7)=defgrd_inv(1,2);
    defgrd_IT_vec(8)=defgrd_inv(2,2);

    //--------------------------- build N_X operator (wrt material config)
    LINALG::Matrix<9,numdof_> N_X(true); // set to zero
    for (int i=0; i<numnod_; ++i)
    {
      N_X(0,3*i+0) = N_XYZ(0,i);
      N_X(1,3*i+1) = N_XYZ(0,i);
      N_X(2,3*i+2) = N_XYZ(0,i);

      N_X(3,3*i+0) = N_XYZ(1,i);
      N_X(4,3*i+1) = N_XYZ(1,i);
      N_X(5,3*i+2) = N_XYZ(1,i);

      N_X(6,3*i+0) = N_XYZ(2,i);
      N_X(7,3*i+1) = N_XYZ(2,i);
      N_X(8,3*i+2) = N_XYZ(2,i);
    }

    LINALG::Matrix<numdim_*numdim_,numdim_> F_X(true);
    for(int i=0; i<numdim_; i++)
    {
      for(int n=0; n<numnod_; n++)
      {
        // second derivatives w.r.t. XYZ are orderd as followed: deriv2(N,XX ; N,YY ; N,ZZ ; N,XY ; N,XZ ; N,YZ)
        F_X(i*numdim_+0, 0) += N_XYZ2(0,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+1, 0) += N_XYZ2(3,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+2, 0) += N_XYZ2(4,n)*nodaldisp(n*numdim_+i);

        F_X(i*numdim_+0, 1) += N_XYZ2(3,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+1, 1) += N_XYZ2(1,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+2, 1) += N_XYZ2(5,n)*nodaldisp(n*numdim_+i);

        F_X(i*numdim_+0, 2) += N_XYZ2(4,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+1, 2) += N_XYZ2(5,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+2, 2) += N_XYZ2(2,n)*nodaldisp(n*numdim_+i);

      }
    }

    //--------------------------- compute dJ/dX = dJ/dF : dF/dX = JF^-T : dF/dX at gausspoint
    //--------------material gradient of jacobi determinant J: GradJ = dJ/dX= dJ/dF : dF/dX = J * F^-T : dF/dX
    LINALG::Matrix<1,numdim_> GradJ;
    GradJ.MultiplyTN(J, defgrd_IT_vec, F_X);

    //------linearization of jacobi determinant detF=J w.r.t. strucuture displacement   dJ/d(us) = dJ/dF : dF/dus = J * F^-T * N,X
    LINALG::Matrix<1,numdof_> dJ_dus;
    dJ_dus.MultiplyTN(J,defgrd_inv_vec,N_X);

    //------linearization of material gradient of jacobi determinant GradJ  w.r.t. strucuture displacement d(GradJ)/d(us)
    //---------------------d(GradJ)/dus =  dJ/dus * F^-T . : dF/dX + J * dF^-T/dus : dF/dX + J * F^-T : N_X_X

    //dF^-T/dus
    LINALG::Matrix<numdim_*numdim_,numdof_> dFinvTdus(true);
    for (int i=0; i<numdim_; i++)
      for (int n =0; n<numnod_; n++)
        for(int j=0; j<numdim_; j++)
        {
          const int gid = numdim_ * n +j;
          for (int k=0; k<numdim_; k++)
            for(int l=0; l<numdim_; l++)
              dFinvTdus(i*numdim_+l, gid) += -defgrd_inv(l,j) * N_XYZ(k,n) * defgrd_inv(k,i);
        }

    //dF^-T/dus : dF/dX = - (F^-1 . dN/dX . u_s . F^-1)^T : dF/dX
    /*
    LINALG::Matrix<numdim_,numdof_> dFinvdus_dFdX(true);
    for (int i=0; i<numdim_; i++)
      for (int n =0; n<numnod_; n++)
        for(int j=0; j<numdim_; j++)
        {
          const int gid = numdim_ * n +j;
          for (int k=0; k<numdim_; k++)
            for(int l=0; l<numdim_; l++)
              for(int p=0; p<numdim_; p++)
              {
                dFinvdus_dFdX(p, gid) += -defgrd_inv(l,j) * N_XYZ(k,n) * defgrd_inv(k,i) * F_X(i*numdim_+l,p);
              }
        }
    */

    //dF^-T/dus : dF/dX = - (F^-1 . dN/dX . u_s . F^-1)^T : dF/dX
    LINALG::Matrix<numdim_,numdof_> dFinvdus_dFdX(true);
    for (int i=0; i<numdim_; i++)
      for (int n =0; n<numnod_; n++)
        for(int j=0; j<numdim_; j++)
        {
          const int gid = numdim_ * n +j;
          for(int l=0; l<numdim_; l++)
            for(int p=0; p<numdim_; p++)
            {
              dFinvdus_dFdX(p, gid) += dFinvTdus(i*numdim_+l, gid) * F_X(i*numdim_+l,p);
            }
        }

    /*
    for(int i=0; i<numdim_;i++)
      for(int n=0; n<numdof_;n++)
        if(abs( dFinvdus_dFdX_alt(i,n)-dFinvdus_dFdX(i,n) )< 1.0e-8 ){}
        else dserror("dFinvdus_dFdX_alt check failed");
    cout<<"dFinvdus_dFdX_alt check ok"<<endl;
    */

    //F^-T : N_X_X
    LINALG::Matrix<numdim_,numdof_> Finv_N_XYZ2(true);

    for (int n =0; n<numnod_; n++)
    {
      //! second derivatives  are ordered as followed: (N,xx ; N,yy ; N,zz ; N,xy ; N,xz ; N,yz)
      int n_dim = n*numdim_;
      Finv_N_XYZ2(0, n_dim + 0) += defgrd_inv(0,0) * N_XYZ2(0,n) + defgrd_inv(1,0) * N_XYZ2(3,n)+ defgrd_inv(2,0) * N_XYZ2(4,n);
      Finv_N_XYZ2(0, n_dim + 1) += defgrd_inv(0,1) * N_XYZ2(0,n) + defgrd_inv(1,1) * N_XYZ2(3,n)+ defgrd_inv(2,1) * N_XYZ2(4,n);
      Finv_N_XYZ2(0, n_dim + 2) += defgrd_inv(0,2) * N_XYZ2(0,n) + defgrd_inv(1,2) * N_XYZ2(3,n)+ defgrd_inv(2,2) * N_XYZ2(4,n);

      Finv_N_XYZ2(1, n_dim + 0) += defgrd_inv(0,0) * N_XYZ2(3,n) + defgrd_inv(1,0) * N_XYZ2(1,n)+ defgrd_inv(2,0) * N_XYZ2(5,n);
      Finv_N_XYZ2(1, n_dim + 1) += defgrd_inv(0,1) * N_XYZ2(3,n) + defgrd_inv(1,1) * N_XYZ2(1,n)+ defgrd_inv(2,1) * N_XYZ2(5,n);
      Finv_N_XYZ2(1, n_dim + 2) += defgrd_inv(0,2) * N_XYZ2(3,n) + defgrd_inv(1,2) * N_XYZ2(1,n)+ defgrd_inv(2,2) * N_XYZ2(5,n);

      Finv_N_XYZ2(2, n_dim + 0) += defgrd_inv(0,0) * N_XYZ2(4,n) + defgrd_inv(1,0) * N_XYZ2(5,n)+ defgrd_inv(2,0) * N_XYZ2(2,n);
      Finv_N_XYZ2(2, n_dim + 1) += defgrd_inv(0,1) * N_XYZ2(4,n) + defgrd_inv(1,1) * N_XYZ2(5,n)+ defgrd_inv(2,1) * N_XYZ2(2,n);
      Finv_N_XYZ2(2, n_dim + 2) += defgrd_inv(0,2) * N_XYZ2(4,n) + defgrd_inv(1,2) * N_XYZ2(5,n)+ defgrd_inv(2,2) * N_XYZ2(2,n);
    }

    LINALG::Matrix<1,numdim_> temp2;
    temp2.MultiplyTN( defgrd_IT_vec, F_X);

    LINALG::Matrix<numdim_,numdof_> dgradJ_dus(true);
    dgradJ_dus.MultiplyTN(temp2,dJ_dus);

    dgradJ_dus.Update(J,dFinvdus_dFdX,1.0);

    dgradJ_dus.Update(J,Finv_N_XYZ2,1.0);

    //--------------------------------------------------------------------

    double dphi_dp=0.0;
    double dphi_dJ=0.0;
    double dphi_dJdp=0.0;
    double dphi_dJJ=0.0;
    double dphi_dpp=0.0;
    double porosity=0.0;

    structmat->ComputePorosity(press, J, gp,porosity,dphi_dp,dphi_dJ,dphi_dJdp,dphi_dJJ,dphi_dpp);

    porosity_gp[gp] = porosity;

    //linearization of porosity w.r.t structure displacement d\phi/d(us) = d\phi/dJ*dJ/d(us)
    LINALG::Matrix<1,numdof_> dphi_dus;
    dphi_dus.Update( dphi_dJ , dJ_dus );

    //material porosity gradient Grad(phi) = dphi/dp * Grad(p) + dphi/dJ * Grad(J)
    LINALG::Matrix<numdim_,1> grad_porosity;
    for (int idim=0; idim<numdim_; ++idim)
    {
      grad_porosity(idim)=dphi_dp*Gradp(idim)+dphi_dJ*GradJ(idim);
    }

    //linearization of material porosity gradient w.r.t structure displacement
    // d ( Grad(\phi) ) / du_s = d\phi / (dJ du_s) * dJ /dX+ d\phi / dJ * dJ / (dX*du_s) + d\phi / (dp*du_s) * dp /dX
    LINALG::Matrix<numdim_,numdof_> dgradphi_dus;
    dgradphi_dus.MultiplyTN(dphi_dJJ, GradJ ,dJ_dus);
    dgradphi_dus.Update(dphi_dJ, dgradJ_dus, 1.0);
    dgradphi_dus.Multiply(dphi_dJdp, Gradp, dJ_dus, 1.0);

    //double *tmp = new double[numdim_];
    //for (int idim=0; idim<numdim_; ++idim)
    //{
    //  tmp[idim]=grad_porosity(idim);
    //}
    gradporosity_gp[gp] = grad_porosity;    // trial urrecha.

    //*****************************************************************************************
    /*
     // return gp strains (only in case of stress/strain output)
     switch (iostrain)
     {
     case INPAR::STR::strain_gl:
     {
     if (elestrain == NULL) dserror("strain data not available");
     for (int i = 0; i < 3; ++i)
     (*elestrain)(gp,i) = glstrain(i);
     for (int i = 3; i < 6; ++i)
     (*elestrain)(gp,i) = 0.5 * glstrain(i);
     }
     break;
     case INPAR::STR::strain_ea:
     {
     if (elestrain == NULL) dserror("strain data not available");
     // rewriting Green-Lagrange strains in matrix format
     LINALG::Matrix<numdim_,numdim_> gl;
     gl(0,0) = glstrain(0);
     gl(0,1) = 0.5*glstrain(3);
     gl(0,2) = 0.5*glstrain(5);
     gl(1,0) = gl(0,1);
     gl(1,1) = glstrain(1);
     gl(1,2) = 0.5*glstrain(4);
     gl(2,0) = gl(0,2);
     gl(2,1) = gl(1,2);
     gl(2,2) = glstrain(2);

     LINALG::Matrix<numdim_,numdim_> temp;
     LINALG::Matrix<numdim_,numdim_> euler_almansi;
     temp.Multiply(gl,defgrd_inv);
     euler_almansi.MultiplyTN(defgrd_inv,temp);

     (*elestrain)(gp,0) = euler_almansi(0,0);
     (*elestrain)(gp,1) = euler_almansi(1,1);
     (*elestrain)(gp,2) = euler_almansi(2,2);
     (*elestrain)(gp,3) = euler_almansi(0,1);
     (*elestrain)(gp,4) = euler_almansi(1,2);
     (*elestrain)(gp,5) = euler_almansi(0,2);
     }
     break;
     case INPAR::STR::strain_none:
     break;
     default:
     dserror("requested strain type not available");
     }*/

    /* call material law cccccccccccccccccccccccccccccccccccccccccccccccccccccc
     ** Here all possible material laws need to be incorporated,
     ** the stress vector, a C-matrix, and a density must be retrieved,
     ** every necessary data must be passed.
     */
    /*
     double density = 0.0;
     LINALG::Matrix<numstr_,numstr_> cmat(true);
     LINALG::Matrix<numstr_,1> stress(true);
     soh8_mat_sel(&stress,&cmat,&density,&glstrain,&defgrd,gp,params);
     // end of call material law ccccccccccccccccccccccccccccccccccccccccccccccc
     */

    /*
     // return gp stresses
     switch (iostress)
     {
     case INPAR::STR::stress_2pk:
     {
     if (elestress == NULL) dserror("stress data not available");
     for (int i = 0; i < numstr_; ++i)
     (*elestress)(gp,i) = stress(i);
     }
     break;
     case INPAR::STR::stress_cauchy:
     {
     if (elestress == NULL) dserror("stress data not available");
     const double detF = defgrd.Determinant();

     LINALG::Matrix<3,3> pkstress;
     pkstress(0,0) = stress(0);
     pkstress(0,1) = stress(3);
     pkstress(0,2) = stress(5);
     pkstress(1,0) = pkstress(0,1);
     pkstress(1,1) = stress(1);
     pkstress(1,2) = stress(4);
     pkstress(2,0) = pkstress(0,2);
     pkstress(2,1) = pkstress(1,2);
     pkstress(2,2) = stress(2);

     LINALG::Matrix<3,3> temp;
     LINALG::Matrix<3,3> cauchystress;
     temp.Multiply(1.0/detF,defgrd,pkstress,0.0);
     cauchystress.MultiplyNT(temp,defgrd);

     (*elestress)(gp,0) = cauchystress(0,0);
     (*elestress)(gp,1) = cauchystress(1,1);
     (*elestress)(gp,2) = cauchystress(2,2);
     (*elestress)(gp,3) = cauchystress(0,1);
     (*elestress)(gp,4) = cauchystress(1,2);
     (*elestress)(gp,5) = cauchystress(0,2);
     }
     break;
     case INPAR::STR::stress_none:
     break;
     default:
     dserror("requested stress type not available");
     }
     */

    //F^-T * Grad\phi
    LINALG::Matrix<numdim_,1> Finvgradphi;
    Finvgradphi.MultiplyTN(defgrd_inv, grad_porosity);

    //F^-T * d(Grad\phi)/d(u_s)
    LINALG::Matrix<numdim_,numdof_> Finvdgradphidus;
    Finvdgradphidus.MultiplyTN(defgrd_inv, dgradphi_dus);

    /*
    //dF^-T/du_s * Grad(\phi) = - (F^-1 . dN/dX . u_s . F^-1)^T * Grad(\phi)
    LINALG::Matrix<numdim_,numdof_> dFinvdus_gradphi(true);
    for (int i=0; i<numdim_; i++)
      for (int n =0; n<numnod_; n++)
        for(int j=0; j<numdim_; j++)
        {
          const int gid = numdim_ * n +j;
          for (int k=0; k<numdim_; k++)
            for(int l=0; l<numdim_; l++)
              dFinvdus_gradphi(i, gid) += -defgrd_inv(l,j) * N_XYZ(k,n) * defgrd_inv(k,i) * grad_porosity(l);
        }
    */

    //dF^-T/du_s * Grad(\phi) = - (F^-1 . dN/dX . u_s . F^-1)^T * Grad(\phi)
    LINALG::Matrix<numdim_,numdof_> dFinvdus_gradphi(true);
    for (int i=0; i<numdim_; i++)
      for (int n =0; n<numnod_; n++)
        for(int j=0; j<numdim_; j++)
        {
          const int gid = numdim_ * n +j;
          for(int l=0; l<numdim_; l++)
            dFinvdus_gradphi(i, gid) += dFinvTdus(i*numdim_+l, gid)  * grad_porosity(l);
        }

    /*
    for(int i=0; i<numdim_;i++)
      for(int n=0; n<numdof_;n++)
        if(abs( dFinvdus_gradphi_alt(i,n)-dFinvdus_gradphi(i,n) )< 1.0e-30 ){}
        else dserror("dFinvdus_gradphi_alt check failed");
    cout<<"dFinvdus_gradphi_alt check ok"<<endl;
    cout<<dFinvdus_gradphi_alt<<endl;
    */

    LINALG::Matrix<numstr_,numdof_> dCinv_dus (true);
    for (int n=0; n<numnod_; ++n)
      for (int k=0; k<numdim_; ++k)
      {
        const int gid = n*numdim_+k;
        for (int i=0; i<numdim_; ++i)
        {
          dCinv_dus(0,gid) += -2*C_inv(0,i)*N_XYZ(i,n)*defgrd_inv(0,k);
          dCinv_dus(1,gid) += -2*C_inv(1,i)*N_XYZ(i,n)*defgrd_inv(1,k);
          dCinv_dus(2,gid) += -2*C_inv(2,i)*N_XYZ(i,n)*defgrd_inv(2,k);
          /* ~~~ */
          dCinv_dus(3,gid) += -C_inv(0,i)*N_XYZ(i,n)*defgrd_inv(1,k)-defgrd_inv(0,k)*N_XYZ(i,n)*C_inv(1,i);
          dCinv_dus(4,gid) += -C_inv(1,i)*N_XYZ(i,n)*defgrd_inv(2,k)-defgrd_inv(1,k)*N_XYZ(i,n)*C_inv(2,i);
          dCinv_dus(5,gid) += -C_inv(2,i)*N_XYZ(i,n)*defgrd_inv(0,k)-defgrd_inv(2,k)*N_XYZ(i,n)*C_inv(0,i);
        }
      }

    //******************* FAD ************************

    /*
     LINALG::TMatrix<FAD,numnod_,1>  fad_shapefct;
     LINALG::TMatrix<FAD,numdim_,numnod_> fad_N_XYZ;
     LINALG::TMatrix<FAD,6,numnod_> fad_N_XYZ2;
     for (int j=0; j<numnod_; ++j)
     {
     fad_shapefct(j)=shapefct(j);
     for (int i=0; i<numdim_; ++i)
     fad_N_XYZ(i,j) = N_XYZ(i,j);
     for (int i=0; i<6 ; i++)
     fad_N_XYZ2(i,j) = N_XYZ2(i,j);
     }

     FAD fad_press = fad_shapefct.Dot(fad_epreaf);
     LINALG::TMatrix<FAD,numdim_,1> fad_Gradp;
     fad_Gradp.Multiply(fad_N_XYZ,fad_epreaf);

     LINALG::TMatrix<FAD,numdim_,numdim_>  fad_fvelder;
     for (int i=0; i<numdim_; ++i)
       for (int j=0; j<numdim_; ++j)
         fad_fvelder(i,j) = fvelder(i,j);

     // compute F
     LINALG::TMatrix<FAD,numdim_,numdim_> fad_defgrd;
     fad_defgrd.MultiplyNT(fad_xcurr,fad_N_XYZ);
     FAD fad_J = Determinant3x3(fad_defgrd);

     LINALG::TMatrix<FAD,numdim_,numdim_>    fad_defgrd_inv;
     fad_defgrd_inv = fad_defgrd;
     Inverse3x3(fad_defgrd_inv);

     LINALG::TMatrix<FAD,numdim_,numdim_>    fad_defgrd_inv2;
     for (int i=0; i<numdim_; ++i)
       for (int j=0; j<numdim_; ++j)
         fad_defgrd_inv2(i,j) = defgrd_inv(i,j);

     LINALG::TMatrix<FAD,numdim_,numdim_> fad_cauchygreen;
     fad_cauchygreen.MultiplyTN(fad_defgrd,fad_defgrd);

     LINALG::TMatrix<FAD,numdim_,numdim_>    fad_C_inv;
     fad_C_inv = fad_cauchygreen;
     Inverse3x3(fad_C_inv);

     LINALG::TMatrix<FAD,6,1> fad_C_inv_vec(false);
     fad_C_inv_vec(0) = fad_C_inv(0,0);
     fad_C_inv_vec(1) = fad_C_inv(1,1);
     fad_C_inv_vec(2) = fad_C_inv(2,2);
     fad_C_inv_vec(3) = fad_C_inv(0,1);
     fad_C_inv_vec(4) = fad_C_inv(1,2);
     fad_C_inv_vec(5) = fad_C_inv(2,0);

     LINALG::TMatrix<FAD,numdim_,numdim_>    fad_C_inv2;
     for (int i=0; i<numdim_; ++i)
       for (int j=0; j<numdim_; ++j)
         fad_C_inv2(i,j) = C_inv(i,j);


     LINALG::TMatrix<FAD,9,1> fad_defgrd_IT_vec;
     fad_defgrd_IT_vec(0)=fad_defgrd_inv(0,0);
     fad_defgrd_IT_vec(1)=fad_defgrd_inv(1,0);
     fad_defgrd_IT_vec(2)=fad_defgrd_inv(2,0);
     fad_defgrd_IT_vec(3)=fad_defgrd_inv(0,1);
     fad_defgrd_IT_vec(4)=fad_defgrd_inv(1,1);
     fad_defgrd_IT_vec(5)=fad_defgrd_inv(2,1);
     fad_defgrd_IT_vec(6)=fad_defgrd_inv(0,2);
     fad_defgrd_IT_vec(7)=fad_defgrd_inv(1,2);
     fad_defgrd_IT_vec(8)=fad_defgrd_inv(2,2);

     LINALG::TMatrix<FAD,numdim_*numdim_,numdim_> fad_F_X(true);
     for(int i=0; i<numdim_; i++)
     {
     for(int n=0; n<numnod_; n++)
     {
     fad_F_X(i*numdim_+0, 0) +=   fad_N_XYZ2(0,n)*fad_nodaldisp(n*numdim_+i);
     fad_F_X(i*numdim_+1, 0) +=   fad_N_XYZ2(3,n)*fad_nodaldisp(n*numdim_+i);
     fad_F_X(i*numdim_+2, 0) +=   fad_N_XYZ2(4,n)*fad_nodaldisp(n*numdim_+i);

     fad_F_X(i*numdim_+0, 1) +=   fad_N_XYZ2(3,n)*fad_nodaldisp(n*numdim_+i) ;
     fad_F_X(i*numdim_+1, 1) +=   fad_N_XYZ2(1,n)*fad_nodaldisp(n*numdim_+i) ;
     fad_F_X(i*numdim_+2, 1) +=   fad_N_XYZ2(5,n)*fad_nodaldisp(n*numdim_+i) ;

     fad_F_X(i*numdim_+0, 2) +=   fad_N_XYZ2(4,n)*fad_nodaldisp(n*numdim_+i) ;
     fad_F_X(i*numdim_+1, 2) +=   fad_N_XYZ2(5,n)*fad_nodaldisp(n*numdim_+i) ;
     fad_F_X(i*numdim_+2, 2) +=   fad_N_XYZ2(2,n)*fad_nodaldisp(n*numdim_+i) ;
     }
     }

     LINALG::TMatrix<FAD,1,numdim_> fad_GradJ;
     fad_GradJ.MultiplyTN(fad_J, fad_defgrd_IT_vec, fad_F_X);

     double initporosity  = structmat->Initporosity();
     double bulkmodulus = structmat->Bulkmodulus();
     double penalty = structmat->Penaltyparameter();

     FAD fad_a     = ( bulkmodulus/(1-initporosity) + fad_press - penalty/initporosity ) * fad_J;
     FAD fad_b     = -fad_a + bulkmodulus + penalty;
     FAD fad_c   = (fad_b/fad_a) * (fad_b/fad_a) + 4*penalty/fad_a;
     FAD fad_d     = sqrt(fad_c)*fad_a;

     FAD fad_sign = 1.0;

     FAD fad_test = 1 / (2 * fad_a) * (-fad_b + fad_d);
     if (fad_test >= 1.0 or fad_test < 0.0)
     {
       fad_sign = -1.0;
       fad_d = fad_sign * fad_d;
     }

     FAD fad_porosity = 1/(2*fad_a)*(-fad_b+fad_d);

     FAD fad_d_p   =  fad_J * (-fad_b+2*penalty)/fad_d;
     FAD fad_d_J   =  fad_a/fad_J * ( -fad_b + 2*penalty ) / fad_d;

     FAD fad_dphi_dp=  - fad_J * fad_porosity/fad_a + (fad_J+fad_d_p)/(2*fad_a);
     FAD fad_dphi_dJ=  -fad_porosity/fad_J+ 1/(2*fad_J) + fad_d_J / (2*fad_a);

     LINALG::TMatrix<FAD,1,numdim_>             fad_grad_porosity;
     //      fad_grad_porosity.Update(fad_dphi_dp,fad_Gradp,fad_dphi_dJ,fad_GradJ);
     for (int idim=0; idim<numdim_; ++idim)
     {
     fad_grad_porosity(idim)=fad_dphi_dp*fad_Gradp(idim)+fad_dphi_dJ*fad_GradJ(idim);
     }

     //double visc = fluidmat->Viscosity();
     LINALG::TMatrix<FAD,numdim_,numdim_> fad_CinvFvel;
     LINALG::TMatrix<FAD,numdim_,numdim_> fad_tmp;
     LINALG::TMatrix<FAD,numstr_,1> fad_fstress;
     fad_CinvFvel.Multiply(fad_C_inv,fad_fvelder);
     fad_tmp.MultiplyNT(fad_CinvFvel,fad_defgrd_inv);
     LINALG::TMatrix<FAD,numdim_,numdim_> fad_tmp2(fad_tmp);
     fad_tmp.UpdateT(1.0,fad_tmp2,1.0);

     fad_fstress(0) = fad_tmp(0,0);
     fad_fstress(1) = fad_tmp(1,1);
     fad_fstress(2) = fad_tmp(2,2);
     fad_fstress(3) = fad_tmp(0,1);
     fad_fstress(4) = fad_tmp(1,2);
     fad_fstress(5) = fad_tmp(2,0);

     for (int i=0; i<numdof_; i++)
     {
     if( abs(dJ_dus(i)-fad_J.dx(i)) > 1e-15)
     {
     cout<<"dJdus("<<i<<"): "<<dJ_dus(i)<<endl;
     cout<<"fad_J.dx("<<i<<"): "<<fad_J.dx(i)<<endl;
     cout<<"error: "<<abs(dJ_dus(i)-fad_J.dx(i))<<endl;
     dserror("check dJdus failed!");
     }
     }
     cout<<"dJdus check done and ok"<<endl;

     for (int i=0; i<numdof_; i++)
     for (int j=0; j<numdim_*numdim_; j++)
     {
     if( abs(dFinvTdus(j,i)-fad_defgrd_IT_vec(j).dx(i)) > 1e-15)
     {
     cout<<"dFinvTdus("<<i<<"): "<<dFinvTdus(j,i)<<endl;
     cout<<"fad_defgrd_IT_vec.dx("<<i<<"): "<<fad_defgrd_IT_vec(j).dx(i)<<endl;
     cout<<"error: "<<abs(dFinvTdus(j,i)-fad_defgrd_IT_vec(j).dx(i))<<endl;
     dserror("check dFinvTdus failed!, ");
     }
     }
     cout<<"dFinvdus check done and ok"<<endl;

     for (int i=0; i<numdof_; i++)
     for (int j=0; j<numdim_; j++)
     {
     if( abs(dgradJ_dus(j,i)-fad_GradJ(j).dx(i)) > 1e-15)
     {
     cout<<"dgradJ_dus("<<i<<"): "<<dgradJ_dus(j,i)<<endl;
     cout<<"fad_GradJ.dx("<<i<<"): "<<fad_GradJ(j).dx(i)<<endl;
     cout<<"GradJ:"<<endl<<GradJ<<endl;
     cout<<"fad_GradJ:"<<endl<<fad_GradJ<<endl;
     cout<<"error: "<<abs(dgradJ_dus(j,i)-fad_GradJ(j).dx(i))<<endl;
     dserror("check dgradJ_dus failed!");
     }
     }
     cout<<"dgradJ_dus check done and ok"<<endl;

     for (int i=0; i<numdof_; i++)
     if( abs(dphi_dus(i)-fad_porosity.dx(i)) > 1e-15)
     {
     cout<<"dphi_dus("<<i<<"): "<<dphi_dus(i)<<endl;
     cout<<"fad_porosity.dx("<<i<<"): "<<fad_porosity.dx(i)<<endl;
     cout<<"dphi_dus:"<<endl<<dphi_dus<<endl;
     cout<<"fad_porosity:"<<endl<<fad_porosity<<endl;
     cout<<"error: "<<abs(dphi_dus(i)-fad_porosity.dx(i))<<endl;
     dserror("check dphi_dus failed!");
     }
     cout<<"dphi_dus check done and ok"<<endl;

     for (int i=0; i<numdof_; i++)
     for (int j=0; j<numdim_; j++)
     {
     if( abs(dgradphi_dus(j,i)-fad_grad_porosity(j).dx(i)) > 1e-15)
     {
     cout<<"dgradphi_dus("<<i<<"): "<<dgradphi_dus(j,i)<<endl;
     cout<<"fad_grad_porosity.dx("<<i<<"): "<<fad_grad_porosity(j).dx(i)<<endl;
     cout<<"dgradphi_dus:"<<endl<<dgradphi_dus<<endl;
     cout<<"fad_grad_porosity:"<<endl<<fad_grad_porosity<<endl;
     cout<<"error: "<<abs(dgradphi_dus(j,i)-fad_grad_porosity(j).dx(i))<<endl;
     dserror("check dgradphi_dus failed!");
     }
     }
     cout<<"dgradphi_dus check done and ok"<<endl;

     for (int i=0; i<numdof_; i++)
     for (int j=0; j<6; j++)
     {
     if( abs(dCinv_dus(j,i)-fad_C_inv_vec(j).dx(i)) > 1e-15)
     {
     cout<<"dCinv_dus("<<i<<"): "<<dCinv_dus(j,i)<<endl;
     cout<<"fad_C_inv.dx("<<i<<"): "<<fad_C_inv_vec(j).dx(i)<<endl;
     cout<<"dCinv_dus:"<<endl<<dCinv_dus<<endl;
     cout<<"fad_C_inv:"<<endl<<fad_C_inv_vec<<endl;
     cout<<"error: "<<abs(dCinv_dus(j,i)-fad_C_inv_vec(j).dx(i))<<endl;
     dserror("check dCinv_dus failed!");
     }
     }
     cout<<"dCinv_dus check done and ok"<<endl;
     */

    //******************* FAD ************************

    //B^T . C^-1
    LINALG::Matrix<numdof_,1> cinvb(true);
    cinvb.MultiplyTN(bop,C_inv_vec);

    //--------------------------------------------------------

    // **********************evaluate stiffness matrix and force vector+++++++++++++++++++++++++
    double detJ_w = detJ*intpoints_.Weight(gp);//gpweights[gp];
    LINALG::Matrix<numdof_,numdof_> estiff_stat(true);
    LINALG::Matrix<numdof_,numdof_> erea_u(true);
    LINALG::Matrix<numdof_,numdof_> erea_v(true);
    LINALG::Matrix<numdof_,1> erea_force(true);
    LINALG::Matrix<numdof_,1> ecoupl_force_p(true);
    LINALG::Matrix<numdof_,1> ecoupl_force_v(true);

    if (force != NULL or stiffmatrix != NULL or reamatrix != NULL )
    {
      for (int k=0; k<numnod_; k++)
      {
        const int fk = numdim_*k;
        const double fac = detJ_w* shapefct(k);
        const double v = fac * reacoeff * porosity * porosity* J;

        for(int j=0; j<numdim_; j++)
        {

          /*-------structure- fluid velocity coupling:  RHS
           "dracy-terms"
           - reacoeff * J *  phi^2 *  v^f
           */
          ecoupl_force_v(fk+j) += -v * fvelint(j);

          /* "reactive dracy-terms"
           reacoeff * J *  phi^2 *  v^s
           */
          erea_force(fk+j) += v * velint(j);

          /*-------structure- fluid pressure coupling: RHS
           *                        "porosity gradient terms"
           J *  F^-T * Grad(phi) * p
           */
          ecoupl_force_p(fk+j) += fac * J * Finvgradphi(j) * press;

          for(int i=0; i<numnod_; i++)
          {
            const int fi = numdim_*i;

            /* additional "reactive darcy-term"
             detJ * w(gp) * ( J * reacoeff * phi^2  ) * D(v_s)
             */
            erea_v(fk+j,fi+j) += v * shapefct(i);

            for (int l=0; l<numdim_; l++)
            {
              /* additional "porosity gradient term" + "darcy-term"
               +  detJ * w(gp) * p * ( J * F^-T * d(Grad(phi))/d(us) + dJ/d(us) * F^-T * Grad(phi) + J * d(F^-T)/d(us) *Grad(phi) ) * D(us)
               - detJ * w(gp) * (  dJ/d(us) * v^f * reacoeff * phi^2 + 2* J * reacoeff * phi * d(phi)/d(us) * v^f ) * D(us)
               */
              estiff_stat(fk+j,fi+l) += fac * ( J * Finvdgradphidus(j,fi+l) * press + press * dJ_dus(fi+l) * Finvgradphi(j)
                  + press * J * dFinvdus_gradphi(j, fi+l)
                  - reacoeff * porosity * ( porosity * dJ_dus(fi+l) + 2 * J * dphi_dus(fi+l) ) * fvelint(j)
              )
              ;

              /* additional "reactive darcy-term"
               detJ * w(gp) * (  dJ/d(us) * vs * reacoeff * phi^2 + 2* J * reacoeff * phi * d(phi)/d(us) * vs ) * D(us)
               */
              erea_u(fk+j,fi+l) += fac * reacoeff * porosity * velint(j) * ( porosity * dJ_dus(fi+l) + 2 * J * dphi_dus(fi+l) );
            }
          }
        }
      }
    }

    LINALG::Matrix<numstr_,1> fstress(true);
    if(fluidmat->Type() == "Darcy-Brinkman")
    {
      double visc = fluidmat->Viscosity();
      LINALG::Matrix<numdim_,numdim_> CinvFvel;
      LINALG::Matrix<numdim_,numdim_> tmp;
      CinvFvel.Multiply(C_inv,fvelder);
      tmp.MultiplyNT(CinvFvel,defgrd_inv);
      LINALG::Matrix<numdim_,numdim_> tmp2(tmp);
      tmp.UpdateT(1.0,tmp2,1.0);

      fstress(0) = tmp(0,0);
      fstress(1) = tmp(1,1);
      fstress(2) = tmp(2,2);
      fstress(3) = tmp(0,1);
      fstress(4) = tmp(1,2);
      fstress(5) = tmp(2,0);

      fstress.Scale(detJ_w * visc * J * porosity);

      //B^T . C^-1
      LINALG::Matrix<numdof_,1> fstressb(true);
      fstressb.MultiplyTN(bop,fstress);

      if (force != NULL )
      {
        force->Update(1.0,fstressb,1.0);
      }

      //evaluate viscous terms (for darcy-brinkman flow only)
      if (stiffmatrix != NULL)
      {
        LINALG::Matrix<numdim_,numdim_> tmp4;
        tmp4.MultiplyNT(fvelder,defgrd_inv);

        double fac = detJ_w * visc;

        LINALG::Matrix<numstr_,numdof_> fstress_dus (true);
        for (int n=0; n<numnod_; ++n)
          for (int k=0; k<numdim_; ++k)
          {
            const int gid = n*numdim_+k;

            fstress_dus(0,gid) += 2*( dCinv_dus(0,gid)*tmp4(0,0) + dCinv_dus(3,gid)*tmp4(1,0) + dCinv_dus(5,gid)*tmp4(2,0) );
            fstress_dus(1,gid) += 2*( dCinv_dus(3,gid)*tmp4(0,1) + dCinv_dus(1,gid)*tmp4(1,1) + dCinv_dus(4,gid)*tmp4(2,1) );
            fstress_dus(2,gid) += 2*( dCinv_dus(5,gid)*tmp4(0,2) + dCinv_dus(4,gid)*tmp4(1,2) + dCinv_dus(2,gid)*tmp4(2,2) );
            /* ~~~ */
            fstress_dus(3,gid) += + dCinv_dus(0,gid)*tmp4(0,1) + dCinv_dus(3,gid)*tmp4(1,1) + dCinv_dus(5,gid)*tmp4(2,1)
                                  + dCinv_dus(3,gid)*tmp4(0,0) + dCinv_dus(1,gid)*tmp4(1,0) + dCinv_dus(4,gid)*tmp4(2,0);
            fstress_dus(4,gid) += + dCinv_dus(3,gid)*tmp4(0,2) + dCinv_dus(1,gid)*tmp4(1,2) + dCinv_dus(4,gid)*tmp4(2,2)
                                  + dCinv_dus(5,gid)*tmp4(0,1) + dCinv_dus(4,gid)*tmp4(1,1) + dCinv_dus(2,gid)*tmp4(2,1);
            fstress_dus(5,gid) += + dCinv_dus(5,gid)*tmp4(0,0) + dCinv_dus(4,gid)*tmp4(1,0) + dCinv_dus(2,gid)*tmp4(2,0)
                                  + dCinv_dus(0,gid)*tmp4(0,2) + dCinv_dus(3,gid)*tmp4(1,2) + dCinv_dus(5,gid)*tmp4(2,2);

            for(int j=0; j<numdim_; j++)
            {
              fstress_dus(0,gid) += 2*CinvFvel(0,j) * dFinvTdus(j*numdim_  ,gid);
              fstress_dus(1,gid) += 2*CinvFvel(1,j) * dFinvTdus(j*numdim_+1,gid);
              fstress_dus(2,gid) += 2*CinvFvel(2,j) * dFinvTdus(j*numdim_+2,gid);
              /* ~~~ */
              fstress_dus(3,gid) += + CinvFvel(0,j) * dFinvTdus(j*numdim_+1,gid)
                                    + CinvFvel(1,j) * dFinvTdus(j*numdim_  ,gid);
              fstress_dus(4,gid) += + CinvFvel(1,j) * dFinvTdus(j*numdim_+2,gid)
                                    + CinvFvel(2,j) * dFinvTdus(j*numdim_+1,gid);
              fstress_dus(5,gid) += + CinvFvel(2,j) * dFinvTdus(j*numdim_  ,gid)
                                    + CinvFvel(0,j) * dFinvTdus(j*numdim_+2,gid);
            }
          }

        /*
        for (int i=0; i<numdof_; i++)
        for (int j=0; j<6; j++)
        {
        if( abs(fstress_dus(j,i)-fad_fstress(j).dx(i)) > 1e-15)
        {
        cout<<"fstress_dus("<<j<<", "<<i<<"): "<<fstress_dus(j,i)<<endl;
        cout<<"fstress.dx("<<j<<", "<<i<<"): "<<fad_fstress(j).dx(i)<<endl;
        cout<<"fstress_dus:"<<endl<<fstress_dus<<endl;
        cout<<"fad_fstress:"<<endl<<fad_fstress<<endl;
        cout<<"dFinvdus:"<<endl<<dFinvTdus<<endl;
        cout<<"CinvFvel:"<<endl<<CinvFvel<<endl;
        cout<<"fad_CinvFvel:"<<endl<<fad_CinvFvel<<endl;
        dserror("check fstress failed!");
        }
        }
        cout<<"fstress check done and ok"<<endl;
        */

        LINALG::Matrix<numdof_,numdof_> tmp1;
        LINALG::Matrix<numdof_,numdof_> tmp2;
        LINALG::Matrix<numdof_,numdof_> tmp3;

        tmp1.Multiply(fac*porosity,fstressb,dJ_dus);
        tmp2.Multiply(fac*J,fstressb,dphi_dus);
        tmp3.MultiplyTN(detJ_w * visc * J * porosity,bop,fstress_dus);

        // additional viscous fluid stress- stiffness term (B^T . fstress . dJ/d(us) * porosity * detJ * w(gp))
        stiffmatrix->Update(1.0,tmp1,1.0);

        // additional fluid stress- stiffness term (B^T .  d\phi/d(us) . fstress  * J * w(gp))
        stiffmatrix->Update(1.0,tmp2,1.0);

        // additional fluid stress- stiffness term (B^T .  phi . dfstress/d(us)  * J * w(gp))
        stiffmatrix->Update(1.0,tmp3,1.0);
      }
    }

    double fac1 = -detJ_w * (1-porosity) * press;
    double fac2= fac1 * J;
    double fac3= detJ_w * press * J;
    //LINALG::Matrix<numstr_,1> stress(true);
   // stress.Update();

    // update internal force vector
    if (force != NULL )
    {
      // additional fluid stress- stiffness term RHS -(B^T .  (1-phi) . C^-1  * J * p^f * detJ * w(gp))
      force->Update(fac2,cinvb,1.0);

      //stationary pressure coupling part of RHS
      // "porosity gradient terms": detJ * w(gp) * J *  F^-T * Grad(phi) * p
      force->Update(1.0,ecoupl_force_p,1.0);

      //stationary velocity coupling part of RHS
      //additional "reactive darcy-term":  - detJ * w(gp) * reacoeff * J *  phi^2 *  v^f
      force->Update(1.0,ecoupl_force_v,1.0);

      //additional "reactive term" RHS  detJ * w(gp) * ( J * reacoeff * phi^2 * v_s)
      force->Update(1.0,erea_force,1.0);

    }  // if (force != NULL )

    if ( reamatrix != NULL )
    {
      /* additional "reactive darcy-term"
       detJ * w(gp) * ( J * reacoeff * phi^2  ) * D(v_s)
       */
      reamatrix->Update(1.0,erea_v,1.0);
    }

    // update stiffness matrix
    if (stiffmatrix != NULL)
    {
      LINALG::Matrix<numdof_,numdof_> tmp1;
      LINALG::Matrix<numdof_,numdof_> tmp2;
      LINALG::Matrix<numdof_,numdof_> tmp3;

      tmp1.Multiply(fac1,cinvb,dJ_dus);
      tmp2.MultiplyTN(fac2,bop,dCinv_dus);
      tmp3.Multiply(fac3,cinvb,dphi_dus);

      // additional fluid stress- stiffness term -(B^T . C^-1 . dJ/d(us) * (1-\phi) * p^f * detJ * w(gp))
      stiffmatrix->Update(1.0,tmp1,1.0);

      // additional fluid stress- stiffness term -(B^T .  dC^-1/d(us) * J * (1-\phi) * p^f * detJ * w(gp))
      stiffmatrix->Update(1.0,tmp2,1.0);

      // additional fluid stress- stiffness term (B^T .  d\phi/d(us) . C^-1  * J * p^f * detJ * w(gp))
      stiffmatrix->Update(1.0,tmp3,1.0);

      /* additional "porosity gradient term" + "darcy-term"
       -  detJ * w(gp) * p *( J * F^-T * d(grad(phi))/d(us) + dJ/d(us) * F^-T * grad(phi) + J * d(F^-T)/d(us) *grad(phi) ) * D(us)
       + detJ * w(gp) * ( 2 * reacoeff * phi * d(phi)/d(us) * J * v^f ) * D(us)
       */
      stiffmatrix->Update(1.0,estiff_stat,1.0);

      /* additional "reactive darcy-term"
       detJ * w(gp) * (  dJ/d(us) * vs * reacoeff * phi^2 + 2* J * reacoeff * phi * d(phi)/d(us) * vs ) * D(us)
       */
      stiffmatrix->Update(1.0,erea_u,1.0);

      // integrate `geometric' stiffness matrix and add to keu *****************
      LINALG::Matrix<6,1> sfac(C_inv_vec); // auxiliary integrated stress
      sfac.Update(detJ_w,fstress,fac1); // detJ*w(gp)*[S11,S22,S33,S12=S21,S23=S32,S13=S31]
      vector<double> SmB_L(3); // intermediate Sm.B_L
      // kgeo += (B_L^T . sigma . B_L) * detJ * w(gp)  with B_L = Ni,Xj see NiliFEM-Skript
      for (int inod=0; inod<numnod_; ++inod)
      {
        SmB_L[0] = sfac(0) * N_XYZ(0, inod) + sfac(3) * N_XYZ(1, inod)
        + sfac(5) * N_XYZ(2, inod);
        SmB_L[1] = sfac(3) * N_XYZ(0, inod) + sfac(1) * N_XYZ(1, inod)
        + sfac(4) * N_XYZ(2, inod);
        SmB_L[2] = sfac(5) * N_XYZ(0, inod) + sfac(4) * N_XYZ(1, inod)
        + sfac(2) * N_XYZ(2, inod);
        for (int jnod=0; jnod<numnod_; ++jnod)
        {
          double bopstrbop = 0.0; // intermediate value
          for (int idim=0; idim<numdim_; ++idim)
          bopstrbop += N_XYZ(idim, jnod) * SmB_L[idim];
          (*stiffmatrix)(3*inod+0,3*jnod+0) += bopstrbop;
          (*stiffmatrix)(3*inod+1,3*jnod+1) += bopstrbop;
          (*stiffmatrix)(3*inod+2,3*jnod+2) += bopstrbop;
        }
      } // end of integrate `geometric' stiffness******************************

      //if the reaction part is not supposed to be computed separately, we add it to the stiffness
      //(this is not the best way to do it, but it only happens once during initialization)
      if ( reamatrix == NULL )
      {
        stiffmatrix->Update(1.0/dt,erea_v,1.0);
      }
    }

    /* =========================================================================*/
  }/* ==================================================== end of Loop over GP */
  /* =========================================================================*/

  //write porosities at GP into material (for output only)
  //structmat->SetPorosityAtGP(porosity_gp);
  structmat->SetGradPorosityAtGP(gradporosity_gp);

  return;
}  // nlnstiff_poroelast()


/*----------------------------------------------------------------------*
 |  evaluate only the poroelasticity fraction for the element (private) |
 *----------------------------------------------------------------------*/
template<class so3_ele, DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::So3_Poro<so3_ele,distype>::coupling_poroelast(vector<int>& lm, // location matrix
    vector<double>& disp, // current displacements
    vector<double>& vel, // current velocities
    //  vector<double>&           residual,       // current residual displ
    LINALG::Matrix<numdim_, numnod_> & evelnp, //current fluid velocity
    LINALG::Matrix<numnod_, 1> & epreaf, //current fluid pressure
    LINALG::Matrix<numdof_, (numdim_ + 1) * numnod_>* stiffmatrix, // element stiffness matrix
    LINALG::Matrix<numdof_, (numdim_ + 1) * numnod_>* reamatrix, // element reactive matrix
    LINALG::Matrix<numdof_, 1>* force, // element internal force vector
 //   LINALG::Matrix<numdof_, 1>* forcerea, // element reactive force vector
    ParameterList& params) // algorithmic parameters e.g. time
{
  //=============================get parameters
  // get global id of the structure element
  int id = Id();
  //access fluid discretization
  RCP<DRT::Discretization> fluiddis = null;
  fluiddis = DRT::Problem::Instance()->GetDis("fluid");
  //get corresponding fluid element
  DRT::Element* fluidele = fluiddis->gElement(id);
  if (fluidele == NULL)
    dserror("Fluid element %i not on local processor", id);

  //get fluid material
  const MAT::FluidPoro* fluidmat = static_cast<const MAT::FluidPoro*>((fluidele->Material()).get());
  if(fluidmat->MaterialType() != INPAR::MAT::m_fluidporo)
    dserror("invalid fluid material for poroelasticity");

  //get structure material
  const MAT::StructPoro* structmat = static_cast<const MAT::StructPoro*>((Material()).get());
  if(structmat->MaterialType() != INPAR::MAT::m_structporo)
    dserror("invalid structure material for poroelasticity");

  const double reacoeff = fluidmat->ComputeReactionCoeff();
  //const double bulkmodulus = structmat->Bulkmodulus();
  //const double penalty = structmat->Penaltyparameter();
  //const double initporosity = structmat->Initporosity();
  double theta = params.get<double>("theta");
  //double dt   = params.get<double>("delta time");

  //=======================================================================

  // update element geometry
  LINALG::Matrix<numdim_,numnod_> xrefe; // material coord. of element
  LINALG::Matrix<numdim_,numnod_> xcurr; // current  coord. of element
  LINALG::Matrix<numnod_,numdim_> xdisp;

  DRT::Node** nodes = Nodes();
  for (int i=0; i<numnod_; ++i)
  {
    const double* x = nodes[i]->X();
    xrefe(0,i) = x[0];
    xrefe(1,i) = x[1];
    xrefe(2,i) = x[2];

    xcurr(0,i) = xrefe(0,i) + disp[i*noddof_+0];
    xcurr(1,i) = xrefe(1,i) + disp[i*noddof_+1];
    xcurr(2,i) = xrefe(2,i) + disp[i*noddof_+2];
  }

  LINALG::Matrix<numdof_,1> nodaldisp;
  for (int i=0; i<numdof_; ++i)
  {
    nodaldisp(i,0) = disp[i];
  }

  LINALG::Matrix<numdof_,1> nodalvel;
  for (int i=0; i<numdof_; ++i)
  {
    nodalvel(i,0) = vel[i];
  }

  LINALG::Matrix<numdof_,(numdim_+1)*numnod_> ecoupl(true);
  LINALG::Matrix<numdof_,numnod_> ecoupl_p(true);
  LINALG::Matrix<numdof_,numdof_> ecoupl_v(true);

  /* =========================================================================*/
  /* ================================================= Loop over Gauss Points */
  /* =========================================================================*/
  LINALG::Matrix<numdim_,numnod_> N_XYZ;      //  first derivatives at gausspoint w.r.t. X, Y,Z
  LINALG::Matrix<6,numnod_> N_XYZ2;     //  second derivatives at gausspoint w.r.t. X, Y,Z
  // build deformation gradient wrt to material configuration
  // in case of prestressing, build defgrd wrt to last stored configuration
  // CAUTION: defgrd(true): filled with zeros!
  LINALG::Matrix<numdim_,numdim_> defgrd(true); //  deformation gradiant evaluated at gauss point
  LINALG::Matrix<numnod_,1> shapefct;           //  shape functions evalulated at gauss point
  LINALG::Matrix<numdim_,numnod_> deriv(true);  //  first derivatives at gausspoint w.r.t. r,s,t
  LINALG::Matrix<6,numnod_> deriv2;       //  second derivatives at gausspoint w.r.t. r,s,t
  //LINALG::Matrix<numdim_,1> xsi;
  for (int gp=0; gp<numgpt_; ++gp)
  {
    DRT::UTILS::shape_function<distype>(xsi_[gp],shapefct);
    DRT::UTILS::shape_function_deriv1<distype>(xsi_[gp],deriv);

    /* get the inverse of the Jacobian matrix which looks like:
     **            [ X_,r  Y_,r  Z_,r ]^-1
     **     J^-1 = [ X_,s  Y_,s  Z_,s ]
     **            [ X_,t  Y_,t  Z_,t ]
     */
    // compute derivatives N_XYZ at gp w.r.t. material coordinates
    // by N_XYZ = J^-1 * N_rst
    N_XYZ.Multiply(invJ_[gp],deriv); // (6.21)
    double detJ = detJ_[gp]; // (6.22)

    if( ishigherorder_ )
    {
      // transposed jacobian "dX/ds"
      LINALG::Matrix<numdim_,numdim_> xjm0;
      xjm0.MultiplyNT(deriv,xrefe);

      // get the second derivatives of standard element at current GP w.r.t. rst
      DRT::UTILS::shape_function_deriv2<distype>(xsi_[gp],deriv2);
      // get the second derivatives of standard element at current GP w.r.t. xyz
      DRT::UTILS::gder2<distype>(xjm0,N_XYZ,deriv2,xrefe,N_XYZ2);
    }
    else
    {
      deriv2.Clear();
      N_XYZ2.Clear();
    }

    // get Jacobian matrix and determinant w.r.t. spatial configuration
    //! transposed jacobian "dx/ds"
    LINALG::Matrix<numdim_,numdim_> xjm;
    //! inverse of transposed jacobian "ds/dx"
    LINALG::Matrix<numdim_,numdim_> xji;
    xjm.MultiplyNT(deriv,xcurr);
    const double det = xji.Invert(xjm);

    // determinant of deformationgradient det F = det ( d x / d X ) = det (dx/ds) * ( det(dX/ds) )^-1
    const double J = det/detJ;

    // (material) deformation gradient F = d xcurr / d xrefe = xcurr * N_XYZ^T
    defgrd.MultiplyNT(xcurr,N_XYZ); //  (6.17)

    /* non-linear B-operator (may so be called, meaning
     ** of B-operator is not so sharp in the non-linear realm) *
     ** B = F . Bl *
     **
     **      [ ... | F_11*N_{,1}^k  F_21*N_{,1}^k  F_31*N_{,1}^k | ... ]
     **      [ ... | F_12*N_{,2}^k  F_22*N_{,2}^k  F_32*N_{,2}^k | ... ]
     **      [ ... | F_13*N_{,3}^k  F_23*N_{,3}^k  F_33*N_{,3}^k | ... ]
     ** B =  [ ~~~   ~~~~~~~~~~~~~  ~~~~~~~~~~~~~  ~~~~~~~~~~~~~   ~~~ ]
     **      [       F_11*N_{,2}^k+F_12*N_{,1}^k                       ]
     **      [ ... |          F_21*N_{,2}^k+F_22*N_{,1}^k        | ... ]
     **      [                       F_31*N_{,2}^k+F_32*N_{,1}^k       ]
     **      [                                                         ]
     **      [       F_12*N_{,3}^k+F_13*N_{,2}^k                       ]
     **      [ ... |          F_22*N_{,3}^k+F_23*N_{,2}^k        | ... ]
     **      [                       F_32*N_{,3}^k+F_33*N_{,2}^k       ]
     **      [                                                         ]
     **      [       F_13*N_{,1}^k+F_11*N_{,3}^k                       ]
     **      [ ... |          F_23*N_{,1}^k+F_21*N_{,3}^k        | ... ]
     **      [                       F_33*N_{,1}^k+F_31*N_{,3}^k       ]
     */
    LINALG::Matrix<numstr_,numdof_> bop;
    for (int i=0; i<numnod_; ++i)
    {
      bop(0,noddof_*i+0) = defgrd(0,0)*N_XYZ(0,i);
      bop(0,noddof_*i+1) = defgrd(1,0)*N_XYZ(0,i);
      bop(0,noddof_*i+2) = defgrd(2,0)*N_XYZ(0,i);
      bop(1,noddof_*i+0) = defgrd(0,1)*N_XYZ(1,i);
      bop(1,noddof_*i+1) = defgrd(1,1)*N_XYZ(1,i);
      bop(1,noddof_*i+2) = defgrd(2,1)*N_XYZ(1,i);
      bop(2,noddof_*i+0) = defgrd(0,2)*N_XYZ(2,i);
      bop(2,noddof_*i+1) = defgrd(1,2)*N_XYZ(2,i);
      bop(2,noddof_*i+2) = defgrd(2,2)*N_XYZ(2,i);
      /* ~~~ */
      bop(3,noddof_*i+0) = defgrd(0,0)*N_XYZ(1,i) + defgrd(0,1)*N_XYZ(0,i);
      bop(3,noddof_*i+1) = defgrd(1,0)*N_XYZ(1,i) + defgrd(1,1)*N_XYZ(0,i);
      bop(3,noddof_*i+2) = defgrd(2,0)*N_XYZ(1,i) + defgrd(2,1)*N_XYZ(0,i);
      bop(4,noddof_*i+0) = defgrd(0,1)*N_XYZ(2,i) + defgrd(0,2)*N_XYZ(1,i);
      bop(4,noddof_*i+1) = defgrd(1,1)*N_XYZ(2,i) + defgrd(1,2)*N_XYZ(1,i);
      bop(4,noddof_*i+2) = defgrd(2,1)*N_XYZ(2,i) + defgrd(2,2)*N_XYZ(1,i);
      bop(5,noddof_*i+0) = defgrd(0,2)*N_XYZ(0,i) + defgrd(0,0)*N_XYZ(2,i);
      bop(5,noddof_*i+1) = defgrd(1,2)*N_XYZ(0,i) + defgrd(1,0)*N_XYZ(2,i);
      bop(5,noddof_*i+2) = defgrd(2,2)*N_XYZ(0,i) + defgrd(2,0)*N_XYZ(2,i);
    }

    // -----------------Right Cauchy-Green tensor = F^T * F
    LINALG::Matrix<numdim_,numdim_> cauchygreen;
    cauchygreen.MultiplyTN(defgrd,defgrd);

    // ------------------Green-Lagrange strains matrix E = 0.5 * (Cauchygreen - Identity)
    // GL strain vector glstrain={E11,E22,E33,2*E12,2*E23,2*E31}
    Epetra_SerialDenseVector glstrain_epetra(numstr_);
    LINALG::Matrix<numstr_,1> glstrain(glstrain_epetra.A(),true);
    glstrain(0) = 0.5 * (cauchygreen(0,0) - 1.0);
    glstrain(1) = 0.5 * (cauchygreen(1,1) - 1.0);
    glstrain(2) = 0.5 * (cauchygreen(2,2) - 1.0);
    glstrain(3) = cauchygreen(0,1);
    glstrain(4) = cauchygreen(1,2);
    glstrain(5) = cauchygreen(2,0);

    //------------------ inverse Right Cauchy-Green tensor
    LINALG::Matrix<numdim_,numdim_> C_inv(false);
    C_inv.Invert(cauchygreen);

    //-----------inverse Right Cauchy-Green tensor as vector
    LINALG::Matrix<6,1> C_inv_vec(true);
    C_inv_vec(0) = C_inv(0,0);
    C_inv_vec(1) = C_inv(1,1);
    C_inv_vec(2) = C_inv(2,2);
    C_inv_vec(3) = C_inv(0,1);
    C_inv_vec(4) = C_inv(1,2);
    C_inv_vec(5) = C_inv(2,0);

    //---------------- get pressure at integration point
    double press = shapefct.Dot(epreaf);

    //------------------ get material pressure gradient at integration point
    LINALG::Matrix<numdim_,1> Gradp;
    Gradp.Multiply(N_XYZ,epreaf);

    //--------------------- get fluid velocity at integration point
    LINALG::Matrix<numdim_,1> fvelint;
    fvelint.Multiply(evelnp,shapefct);

    // material fluid velocity gradient at integration point
    LINALG::Matrix<numdim_,numdim_>              fvelder;
    fvelder.MultiplyNT(evelnp,N_XYZ);

    //! ----------------structure displacement and velocity at integration point
    LINALG::Matrix<numdim_,1> dispint(true);
    LINALG::Matrix<numdim_,1> velint(true);
    for(int i=0; i<numnod_; i++)
    for(int j=0; j<numdim_; j++)
    {
      dispint(j) += nodaldisp(i*numdim_+j) * shapefct(i);
      velint(j) += nodalvel(i*numdim_+j) * shapefct(i);
    }

    // inverse deformation gradient F^-1
    LINALG::Matrix<numdim_,numdim_> defgrd_inv(false);
    defgrd_inv.Invert(defgrd);

    //------------------------------------ build F^-1 as vector 9x1
    LINALG::Matrix<9,1> defgrd_inv_vec;
    defgrd_inv_vec(0)=defgrd_inv(0,0);
    defgrd_inv_vec(1)=defgrd_inv(0,1);
    defgrd_inv_vec(2)=defgrd_inv(0,2);
    defgrd_inv_vec(3)=defgrd_inv(1,0);
    defgrd_inv_vec(4)=defgrd_inv(1,1);
    defgrd_inv_vec(5)=defgrd_inv(1,2);
    defgrd_inv_vec(6)=defgrd_inv(2,0);
    defgrd_inv_vec(7)=defgrd_inv(2,1);
    defgrd_inv_vec(8)=defgrd_inv(2,2);

    //------------------------------------ build F^-T as vector 9x1
    LINALG::Matrix<9,1> defgrd_IT_vec;
    defgrd_IT_vec(0)=defgrd_inv(0,0);
    defgrd_IT_vec(1)=defgrd_inv(1,0);
    defgrd_IT_vec(2)=defgrd_inv(2,0);
    defgrd_IT_vec(3)=defgrd_inv(0,1);
    defgrd_IT_vec(4)=defgrd_inv(1,1);
    defgrd_IT_vec(5)=defgrd_inv(2,1);
    defgrd_IT_vec(6)=defgrd_inv(0,2);
    defgrd_IT_vec(7)=defgrd_inv(1,2);
    defgrd_IT_vec(8)=defgrd_inv(2,2);

    //--------------------------- build N_x operator (wrt material config)
    LINALG::Matrix<9,numdof_> N_X(true); // set to zero
    for (int i=0; i<numnod_; ++i)
    {
      N_X(0,3*i+0) = N_XYZ(0,i);
      N_X(1,3*i+1) = N_XYZ(0,i);
      N_X(2,3*i+2) = N_XYZ(0,i);

      N_X(3,3*i+0) = N_XYZ(1,i);
      N_X(4,3*i+1) = N_XYZ(1,i);
      N_X(5,3*i+2) = N_XYZ(1,i);

      N_X(6,3*i+0) = N_XYZ(2,i);
      N_X(7,3*i+1) = N_XYZ(2,i);
      N_X(8,3*i+2) = N_XYZ(2,i);
    }

    LINALG::Matrix<numdim_*numdim_,numdim_> F_X(true);
    for(int i=0; i<numdim_; i++)
    {
      for(int n=0; n<numnod_; n++)
      {
        F_X(i*numdim_+0, 0) += N_XYZ2(0,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+1, 0) += N_XYZ2(3,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+2, 0) += N_XYZ2(4,n)*nodaldisp(n*numdim_+i);

        F_X(i*numdim_+0, 1) += N_XYZ2(3,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+1, 1) += N_XYZ2(1,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+2, 1) += N_XYZ2(5,n)*nodaldisp(n*numdim_+i);

        F_X(i*numdim_+0, 2) += N_XYZ2(4,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+1, 2) += N_XYZ2(5,n)*nodaldisp(n*numdim_+i);
        F_X(i*numdim_+2, 2) += N_XYZ2(2,n)*nodaldisp(n*numdim_+i);
      }
    }

    //--------------material gradient of jacobi determinant J: GradJ = dJ/dX= dJ/dF : dF/dX = J * F^-T : dF/dX
    LINALG::Matrix<1,numdim_> GradJ;
    GradJ.MultiplyTN(J, defgrd_IT_vec, F_X);

    //**************************************************+auxilary variables for computing the porosity and linearization
    double dphi_dp=0.0;
    double dphi_dJ=0.0;
    double dphi_dJdp=0.0;
    double dphi_dJJ=0.0;
    double dphi_dpp=0.0;
    double porosity=0.0;

    structmat->ComputePorosity(press, J, gp,porosity,dphi_dp,dphi_dJ,dphi_dJdp,dphi_dJJ,dphi_dpp);

    //-----------material porosity gradient
    LINALG::Matrix<1,numdim_> grad_porosity;
    for (int idim=0; idim<numdim_; ++idim)
    {
      grad_porosity(idim)=dphi_dp*Gradp(idim)+dphi_dJ*GradJ(idim);
    }

    //----------------linearization of material porosity gradient w.r.t fluid pressure
    // d(Grad(phi))/dp = d^2(phi)/(dJ*dp) * GradJ * N + d^2(phi)/(dp)^2 * Gradp * N + d(phi)/dp * N,X
    LINALG::Matrix<numdim_,numnod_> dgradphi_dp;
    dgradphi_dp.MultiplyTT(dphi_dJdp,GradJ,shapefct);
    dgradphi_dp.MultiplyNT(dphi_dpp,Gradp,shapefct,1.0);
    dgradphi_dp.Update(dphi_dp,N_XYZ,1.0);

    //******************* FAD ************************
    /*
     // sacado data type replaces "double"
     typedef Sacado::Fad::DFad<double> FAD;  // for first derivs
     // sacado data type replaces "double" (for first+second derivs)
     typedef Sacado::Fad::DFad<Sacado::Fad::DFad<double> > FADFAD;


     LINALG::TMatrix<FAD,numnod_,1>  fad_shapefct;
     LINALG::TMatrix<FAD,numdim_,numnod_> fad_N_XYZ(false);
     LINALG::TMatrix<FAD,6,numnod_> fad_N_XYZ2(false);
     for (int j=0; j<numnod_; ++j)
     {
     fad_shapefct(j)=shapefct(j);
     for (int i=0; i<numdim_; ++i)
     fad_N_XYZ(i,j) = N_XYZ(i,j);
     for (int i=0; i<6 ; i++)
     fad_N_XYZ2(i,j) = N_XYZ2(i,j);
     }

     LINALG::TMatrix<FAD,numdof_,1> fad_nodaldisp(false);
     LINALG::TMatrix<FAD,numnod_,1> fad_epreaf(false);
     for(int i=0; i<numnod_ ; i++)
     {
     fad_epreaf(i) = epreaf(i);
     fad_epreaf(i).diff(i,numnod_);
     }

     FAD fad_press = fad_shapefct.Dot(fad_epreaf);
     LINALG::TMatrix<FAD,numdim_,1> fad_Gradp;
     fad_Gradp.Multiply(fad_N_XYZ,fad_epreaf);

     LINALG::TMatrix<FAD,1,numdim_> fad_GradJ;
     for(int i=0; i<numdim_; i++)
     fad_GradJ(i)=GradJ(i);

     double initporosity  = structmat->Initporosity();
     double bulkmodulus = structmat->Bulkmodulus();
     double penalty = structmat->Penaltyparameter();

     FAD fad_a     = ( bulkmodulus/(1-initporosity) + fad_press - penalty/initporosity ) * J;
     FAD fad_b     = -fad_a + bulkmodulus + penalty;
     FAD fad_c   = (fad_b/fad_a) * (fad_b/fad_a) + 4*penalty/fad_a;
     FAD fad_d     = sqrt(fad_c)*fad_a;

     FAD fad_sign = 1.0;

     FAD fad_test = 1 / (2 * fad_a) * (-fad_b + fad_d);
     if (fad_test >= 1.0 or fad_test < 0.0)
     {
       fad_sign = -1.0;
       fad_d = fad_sign * fad_d;
     }

     FAD fad_porosity = 1/(2*fad_a)*(-fad_b+fad_d);

     FAD fad_d_p   =  J * (-fad_b+2*penalty)/fad_d;
     FAD fad_d_J   =  fad_a/J * ( -fad_b + 2*penalty ) / fad_d;

     FAD fad_dphi_dp=  - J * fad_porosity/fad_a + (J+fad_d_p)/(2*fad_a);
     FAD fad_dphi_dJ=  -fad_porosity/J+ 1/(2*J) + fad_d_J / (2*fad_a);

     LINALG::TMatrix<FAD,1,numdim_>             fad_grad_porosity;
     for (int idim=0; idim<numdim_; ++idim)
     {
     fad_grad_porosity(idim)=fad_dphi_dp*fad_Gradp(idim)+fad_dphi_dJ*fad_GradJ(idim);
     }


     for (int i=0; i<numnod_; i++)
     for (int j=0; j<numdim_; j++)
     {
     if( (dgradphi_dp(j,i)-fad_grad_porosity(j).dx(i)) > 1e-15)
     {
     cout<<"dgradphi_dp("<<i<<"): "<<dgradphi_dp(j,i)<<endl;
     cout<<"fad_grad_porosity.dx("<<i<<"): "<<fad_grad_porosity(j).dx(i)<<endl;
     cout<<"dgradphi_dp:"<<endl<<dgradphi_dp<<endl;
     cout<<"fad_grad_porosity:"<<endl<<fad_grad_porosity<<endl;
     dserror("check dgradphi_dus failed!");
     }
     }
     cout<<"dgradphi_dp check done and ok"<<endl;
     */
    //******************* FAD ************************

    //*****************************************************************************************

    /* call material law cccccccccccccccccccccccccccccccccccccccccccccccccccccc
     ** Here all possible material laws need to be incorporated,
     ** the stress vector, a C-matrix, and a density must be retrieved,
     ** every necessary data must be passed.
     */
    /*
     double density = 0.0;
     LINALG::Matrix<numstr_,numstr_> cmat(true);
     LINALG::Matrix<numstr_,1> stress(true);
     soh8_mat_sel(&stress,&cmat,&density,&glstrain,&defgrd,gp,params);
     // end of call material law ccccccccccccccccccccccccccccccccccccccccccccccc
     */

    // **********************evaluate stiffness matrix and force vector+++++++++++++++++++++++++
    double detJ_w = detJ*intpoints_.Weight(gp);//gpweights[gp];

    //B^T . C^-1
    LINALG::Matrix<numdof_,1> cinvb(true);
    cinvb.MultiplyTN(bop,C_inv_vec);

    //F^-T * grad\phi
    LINALG::Matrix<numdim_,1> Finvgradphi;
    Finvgradphi.MultiplyTT(defgrd_inv, grad_porosity);

    //F^-T * dgrad\phi/dp
    LINALG::Matrix<numdim_,numnod_> Finvgradphidp;
    Finvgradphidp.MultiplyTN(defgrd_inv, dgradphi_dp);

    if (force != NULL or stiffmatrix != NULL or reamatrix != NULL)
    {
      for (int i=0; i<numnod_; i++)
      {
        const int fi = numdim_*i;
        const double fac = detJ_w* shapefct(i);

        for(int j=0; j<numdim_; j++)
        {
          for(int k=0; k<numnod_; k++)
          {
            const int fk = numdim_*k;

            /*-------structure- fluid pressure coupling: "stress terms" + "porosity gradient terms"
             -B^T . ( (1-phi)*J*C^-1 - d(phi)/(dp)*p*J*C^-1 ) * Dp
             + J * F^-T * Grad(phi) * Dp + J * F^-T * d(Grad((phi))/(dp) * p * Dp
             */
            ecoupl_p(fi+j, k) += detJ_w * cinvb(fi+j) * ( -(1-porosity)+ dphi_dp * press ) * J * shapefct(k)
                                + fac * J * ( Finvgradphi(j) * shapefct(k) + Finvgradphidp(j,k) * press );

            /*-------structure- fluid pressure coupling:  "dracy-terms" + "reactive darcy-terms"
             - 2 * reacoeff * J * v^f * phi * d(phi)/dp  Dp
             + 2 * reacoeff * J * v^s * phi * d(phi)/dp  Dp
             */
            const double tmp = fac * reacoeff * J * 2 * porosity * dphi_dp * shapefct(k);
            ecoupl_p(fi+j, k) += -tmp * fvelint(j);

            ecoupl_p(fi+j, k) += tmp * velint(j);

            /*-------structure- fluid velocity coupling:  "darcy-terms"
             -reacoeff * J *  phi^2 *  Dv^f
             */
            ecoupl_v(fi+j, fk+j) += -fac * reacoeff * J * porosity * porosity * shapefct(k);
          }
        }
      }
      if(fluidmat->Type() == "Darcy-Brinkman")
      {
        LINALG::Matrix<numstr_,1> fstress;

        double visc = fluidmat->Viscosity();
        LINALG::Matrix<numdim_,numdim_> CinvFvel;
        LINALG::Matrix<numdim_,numdim_> tmp;
        CinvFvel.Multiply(C_inv,fvelder);
        tmp.MultiplyNT(CinvFvel,defgrd_inv);
        LINALG::Matrix<numdim_,numdim_> tmp2(tmp);
        tmp.UpdateT(1.0,tmp2,1.0);

        fstress(0) = tmp(0,0);
        fstress(1) = tmp(1,1);
        fstress(2) = tmp(2,2);
        fstress(3) = tmp(0,1);
        fstress(4) = tmp(1,2);
        fstress(5) = tmp(2,0);

       // fstress.Scale(detJ_w * visc * J * porosity);

        //B^T . \sigma
        LINALG::Matrix<numdof_,1> fstressb;
        fstressb.MultiplyTN(bop,fstress);
        LINALG::Matrix<numdim_,numnod_> N_XYZ_Finv;
        N_XYZ_Finv.Multiply(defgrd_inv,N_XYZ);
        LINALG::Matrix<numdim_,numnod_> N_XYZ_FinvT;
        N_XYZ_FinvT.MultiplyTN(defgrd_inv,N_XYZ);

        for (int i=0; i<numnod_; i++)
        {
          const int fi = numdim_*i;
          //const double fac = detJ_w* shapefct(i);

          for(int j=0; j<numdim_; j++)
          {
            for(int k=0; k<numnod_; k++)
            {
              const int fk = numdim_*k;

              /*-------structure- fluid pressure coupling: "darcy-brinkman stress terms"
               B^T . ( \mu*J*- d(phi)/(dp) * fstress ) * Dp
               */
              ecoupl_p(fi+j, k) += detJ_w * fstressb(fi+j) * dphi_dp * visc * J * shapefct(k);
              for(int l=0; l<numdim_; l++)
              {
                /*-------structure- fluid velocity coupling: "darcy-brinkman stress terms"
                 B^T . ( \mu*J*- phi * dfstress/dv^f ) * Dp
                 */
                ecoupl_v(fi+j, fk+l) += detJ_w * visc * J * porosity * cinvb(fi+j) * ( N_XYZ_Finv(l,k)+N_XYZ_FinvT(l,k) );
              }
            }
          }
        }
      }//darcy-brinkman
    }
    /* =========================================================================*/
  }/* ==================================================== end of Loop over GP */
  /* =========================================================================*/

  //if (force != NULL )
  //{
    //all rhs terms are added in nlnstiff_poroelast
  //}

  if (stiffmatrix != NULL or reamatrix != NULL)
  {
    // add structure displacement - fluid velocity part to matrix
    for (int ui=0; ui<numnod_; ++ui)
    {
      const int dim_ui = numdim_*ui;

      for (int jdim=0; jdim < numdim_;++jdim)
      {
        const int dim_ui_jdim = dim_ui+jdim;

        for (int vi=0; vi<numnod_; ++vi)
        {
          const int numdof_vi = (numdim_+1)*vi;
          const int dim_vi = numdim_*vi;

          for (int idim=0; idim <numdim_; ++idim)
          {
            ecoupl(dim_ui_jdim , numdof_vi+idim) += ecoupl_v(dim_ui_jdim , dim_vi+idim);
          }
        }
      }
    }

    // add structure displacement - fluid pressure part to matrix
    for (int ui=0; ui<numnod_; ++ui)
    {
      const int dim_ui = numdim_*ui;

      for (int jdim=0; jdim < numdim_;++jdim)
      {
        const int dim_ui_jdim = dim_ui+jdim;

        for (int vi=0; vi<numnod_; ++vi)
        {
          ecoupl( dim_ui_jdim , (numdim_+1)*vi+numdim_ ) += ecoupl_p( dim_ui_jdim , vi);
        }
      }
    }
  }

  //if ( reamatrix != NULL )
  //{
    //reamatrix->Update(1.0,erea,1.0);
  //}

  if (stiffmatrix != NULL)
  {
    // build tangent coupling matrix : effective dynamic stiffness coupling matrix
    //    K_{Teffdyn} = 1/dt C
    //                + theta K_{T}
    stiffmatrix->Update(theta,ecoupl,1.0);
  }

  return;

}  // coupling_poroelast()


template<class so3_ele, DRT::Element::DiscretizationType distype>
void DRT::ELEMENTS::So3_Poro<so3_ele,distype>::InitJacobianMapping()
{
  //const static vector<LINALG::Matrix<numdim_,numnod_> > derivs;// = soh8_derivs();
  LINALG::Matrix<numdim_,numnod_> deriv ;
  LINALG::Matrix<numnod_,numdim_> xrefe;
  for (int i=0; i<numnod_; ++i)
  {
    Node** nodes=Nodes();
    if(!nodes) dserror("Nodes() returned null pointer");
    xrefe(i,0) = Nodes()[i]->X()[0];
    xrefe(i,1) = Nodes()[i]->X()[1];
    xrefe(i,2) = Nodes()[i]->X()[2];
  }
  invJ_.resize(numgpt_);
  detJ_.resize(numgpt_);
  xsi_.resize(numgpt_);

  for (int gp=0; gp<numgpt_; ++gp)
  {
    const double* gpcoord = intpoints_.Point(gp);
    for (int idim=0;idim<numdim_;idim++)
    {
       xsi_[gp](idim) = gpcoord[idim];
    }

    DRT::UTILS::shape_function_deriv1<distype>(xsi_[gp],deriv);

    //invJ_[gp].Shape(NUMDIM_SOH8,NUMDIM_SOH8);
    invJ_[gp].Multiply(deriv,xrefe);
    detJ_[gp] = invJ_[gp].Invert();
    if (detJ_[gp] <= 0.0) dserror("Element Jacobian mapping %10.5e <= 0.0",detJ_[gp]);
  }

  return;
}

#include "so3_poro_fwd.hpp"

