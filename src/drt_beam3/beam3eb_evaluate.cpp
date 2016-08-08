/*!----------------------------------------------------------------------
\file beam3eb_evaluate.cpp

\brief three dimensional nonlinear torsionless rod based on a C1 curve

\level 2

\maintainer Christoph Meier
*-----------------------------------------------------------------------------------------------------------*/

#include "beam3eb.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_lib/drt_exporter.H"
#include "../drt_lib/drt_dserror.H"
#include "../drt_lib/drt_utils.H"
#include "../linalg/linalg_utils.H"
#include "../drt_lib/drt_timecurve.H"
#include "../drt_fem_general/drt_utils_fem_shapefunctions.H"
#include "../drt_mat/stvenantkirchhoff.H"
#include "../linalg/linalg_fixedsizematrix.H"
#include "../drt_fem_general/largerotations.H"
#include "../drt_fem_general/drt_utils_integration.H"
#include "../drt_inpar/inpar_structure.H"
#include "../drt_inpar/inpar_statmech.H"
#include <Epetra_CrsMatrix.h>
#include "../drt_lib/standardtypes_cpp.H"
#include "../drt_beamcontact/beam3contact_utils.H"
#include "../drt_structure_new/str_elements_paramsinterface.H"

#include "Sacado.hpp"
typedef Sacado::Fad::DFad<double> FAD;

/*-----------------------------------------------------------------------------------------------------------*
 |  evaluate the element (public)                                                                 meier 05/12|
 *----------------------------------------------------------------------------------------------------------*/
int DRT::ELEMENTS::Beam3eb::Evaluate(Teuchos::ParameterList& params,
                                     DRT::Discretization& discretization,
                                     std::vector<int>& lm,
                                     Epetra_SerialDenseMatrix& elemat1,
                                     Epetra_SerialDenseMatrix& elemat2,
                                     Epetra_SerialDenseVector& elevec1,
                                     Epetra_SerialDenseVector& elevec2,
                                     Epetra_SerialDenseVector& elevec3)
{
  SetParamsInterfacePtr(params);

  // start with "none"
  ELEMENTS::ActionType act = ELEMENTS::none;

  if (IsParamsInterface())
  {
   act = ParamsInterface().GetActionType();
  }
  else
  {
    // get the action required
    std::string action = params.get<std::string>("action","calc_none");
    if     (action == "calc_none")         dserror("No action supplied");
    else if (action=="calc_struct_linstiff")                               act = ELEMENTS::struct_calc_linstiff;
    else if (action=="calc_struct_nlnstiff")                               act = ELEMENTS::struct_calc_nlnstiff;
    else if (action=="calc_struct_internalforce")                          act = ELEMENTS::struct_calc_internalforce;
    else if (action=="calc_struct_linstiffmass")                           act = ELEMENTS::struct_calc_linstiffmass;
    else if (action=="calc_struct_nlnstiffmass")                           act = ELEMENTS::struct_calc_nlnstiffmass;
    else if (action=="calc_struct_nlnstifflmass")                          act = ELEMENTS::struct_calc_nlnstifflmass; //with lumped mass matrix
    else if (action=="calc_struct_stress")                                 act = ELEMENTS::struct_calc_stress;
    else if (action=="calc_struct_eleload")                                act = ELEMENTS::struct_calc_eleload;
    else if (action=="calc_struct_fsiload")                                act = ELEMENTS::struct_calc_fsiload;
    else if (action=="calc_struct_update_istep")                           act = ELEMENTS::struct_calc_update_istep;
    else if (action=="calc_struct_reset_istep")                            act = ELEMENTS::struct_calc_reset_istep;
    else if (action=="calc_struct_ptcstiff")                               act = ELEMENTS::struct_calc_ptcstiff;
    else if (action=="calc_struct_energy")                                 act = ELEMENTS::struct_calc_energy;
    else     dserror("Unknown type of action for Beam3eb");
  }

  std::string test = params.get<std::string>("action","calc_none");

  switch(act)
  {

    case ELEMENTS::struct_calc_ptcstiff:
    {
      EvaluatePTC<2>(params, elemat1);
    }
    break;

    case ELEMENTS::struct_calc_linstiff:
    {
      //only nonlinear case implemented!
      dserror("linear stiffness matrix called, but not implemented");
    }
    break;

    //nonlinear stiffness and mass matrix are calculated even if only nonlinear stiffness matrix is required
    case ELEMENTS::struct_calc_nlnstiffmass:
    case ELEMENTS::struct_calc_nlnstifflmass:
    case ELEMENTS::struct_calc_nlnstiff:
    case ELEMENTS::struct_calc_internalforce:
    {
      // need current global displacement and residual forces and get them from discretization
      // making use of the local-to-global map lm one can extract current displacement and residual values for each degree of freedom

      //Uncomment the following line to calculate the entire problem with arbitrary precision
      #ifdef PRECISION
      {
      HighPrecissionCalc();
      }
      #endif

      // get element displacements
      Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState("displacement");
      if (disp==Teuchos::null) dserror("Cannot get state vectors 'displacement'");
      std::vector<double> mydisp(lm.size());
      DRT::UTILS::ExtractMyValues(*disp,mydisp,lm);


      // get residual displacements
      Teuchos::RCP<const Epetra_Vector> res  = discretization.GetState("residual displacement");
      if (res==Teuchos::null) dserror("Cannot get state vectors 'residual displacement'");
      std::vector<double> myres(lm.size());
      DRT::UTILS::ExtractMyValues(*res,myres,lm);


      //Only in the dynamic case the velocities are needed.
      // get element velocities only if example is static in nature


      Teuchos::RCP<const Epetra_Vector> vel;
      std::vector<double> myvel(lm.size(),0.0);
      myvel.clear();
      const Teuchos::ParameterList& sdyn = DRT::Problem::Instance()->StructuralDynamicParams();

      if(DRT::INPUT::IntegralValue<INPAR::STR::DynamicType>(sdyn, "DYNAMICTYP")!=INPAR::STR::dyna_statics)
      {
        vel  = discretization.GetState("velocity");
        if (vel==Teuchos::null) dserror("Cannot get state vectors 'velocity'");
        DRT::UTILS::ExtractMyValues(*vel,myvel,lm);
      }


      if (act == ELEMENTS::struct_calc_nlnstiffmass)
      {
      eb_nlnstiffmass(params,myvel,mydisp,&elemat1,&elemat2,&elevec1);
      }
      else if (act == ELEMENTS::struct_calc_nlnstifflmass)
      {
          eb_nlnstiffmass(params,myvel,mydisp,&elemat1,&elemat2,&elevec1);
          lumpmass(&elemat2);
      }
      else if (act == ELEMENTS::struct_calc_nlnstiff)
      {
          eb_nlnstiffmass(params,myvel,mydisp,&elemat1,NULL,&elevec1);
      }
      else if (act == ELEMENTS::struct_calc_internalforce)
      {
          eb_nlnstiffmass(params,myvel,mydisp,NULL,NULL,&elevec1);
      }
    }
    break;

    case ELEMENTS::struct_calc_stress:
      dserror("No stress output implemented for beam3 elements");
    break;
    case ELEMENTS::struct_calc_update_istep:
      for (int i=0;i<3;i++)
      {
        t0_(i,0)=t_(i,0);
        t0_(i,1)=t_(i,1);
      }
    break;
    case ELEMENTS::struct_calc_reset_istep:
      //not necessary since no class variables are modified in predicting steps
    break;
    case ELEMENTS::struct_calc_energy:
      elevec1(0)=Eint_;
      //elevec1(1)=Ekin_;
      //elevec1(2)=Eint_axial_;
      //elevec1(3)=P_(1);
      //elevec1(4)=P_(2);
      //elevec1(5)=L_(0);
      //elevec1(6)=L_(1);
      //elevec1(7)=L_(2);
    break;

    case ELEMENTS::struct_calc_recover:
    {
      // do nothing here
      break;
    }

    default:
      dserror("Unknown type of action for Beam3eb %d", act);
     break;
  }//switch(act)

  return 0;

}  //DRT::ELEMENTS::Beam3eb::Evaluate

/*-----------------------------------------------------------------------------------------------------------*
 |  Integrate a Surface/Line Neumann boundary condition (public)                                  meier 05/12|
 *-----------------------------------------------------------------------------------------------------------*/

int DRT::ELEMENTS::Beam3eb::EvaluateNeumann(Teuchos::ParameterList& params,
                                            DRT::Discretization& discretization,
                                            DRT::Condition& condition,
                                            std::vector<int>& lm,
                                            Epetra_SerialDenseVector& elevec1,
                                            Epetra_SerialDenseMatrix* elemat1)
{
  SetParamsInterfacePtr(params);

  // get element displacements
  Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState("displacement new");
  if (disp==Teuchos::null) dserror("Cannot get state vector 'displacement new'");
  std::vector<double> mydisp(lm.size());
  DRT::UTILS::ExtractMyValues(*disp,mydisp,lm);

  #ifndef INEXTENSIBLE
    const int dofpn = 3*NODALDOFS;
  #else
    const int dofpn = 7;
  #endif

  const int nnodes = 2;

  // get element velocities only if it's not a static problem, otherwise a dynamics problem (UNCOMMENT IF NEEDED)
  const Teuchos::ParameterList& sdyn = DRT::Problem::Instance()->StructuralDynamicParams();
  if(DRT::INPUT::IntegralValue<INPAR::STR::DynamicType>(sdyn,"DYNAMICTYP")!=INPAR::STR::dyna_statics)
  {
    Teuchos::RCP<const Epetra_Vector> vel  = discretization.GetState("velocity");
    if (vel==Teuchos::null) dserror("Cannot get state vectors 'velocity'");
    std::vector<double> myvel(lm.size());
    DRT::UTILS::ExtractMyValues(*vel,myvel,lm);
  }
  // find out whether we will use a time curve
  bool usetime = true;
  double time = -1.0;
  if (this->IsParamsInterface())
    time = this->ParamsInterfacePtr()->GetTotalTime();
  else
    time = params.get("total time",-1.0);
  if (time<0.0) usetime = false;

  // find out whether we will use a time curve and get the factor
  const std::vector<int>* curve  = condition.Get<std::vector<int> >("curve");
  // amplitude of load curve at current time called
  std::vector<double> curvefac(6,1.0);

  for (int i=0; i<6; ++i)
  {
    int curvenum = -1;
    // number of the load curve related with a specific line Neumann condition called
    if (curve) curvenum = (*curve)[i];

    if (curvenum>=0 && usetime)
      curvefac[i] = DRT::Problem::Instance()->Curve(curvenum).f(time);
  }

  // get values and switches from the condition
  // onoff is related to the first 6 flags of a line Neumann condition in the input file;
  // value 1 for flag i says that condition is active for i-th degree of freedom
  const std::vector<int>* onoff = condition.Get<std::vector<int> >("onoff");
  // val is related to the 6 "val" fields after the onoff flags of the Neumann condition

  // in the input file; val gives the values of the force as a multiple of the prescribed load curve
  const std::vector<double>* val = condition.Get<std::vector<double> >("val");

  #ifndef BEAM3EBDISCRETELINENEUMANN
  // funct is related to the 6 "funct" fields after the val field of the Neumann condition
  // in the input file; funct gives the number of the function defined in the section FUNCT
  const std::vector<int>* functions = condition.Get<std::vector<int> >("funct");
  #endif

  //find out which node is correct
  const std::vector< int > * nodeids = condition.Nodes();

  //if a point neumann condition needs to be linearized
  if(condition.Type() == DRT::Condition::PointNeumannEB)
  {
    //find out local element number --> this is done since the first element of a neumann point condition is used for this function
    //in this case we do not know whether it is the left or the right node.
    int insert = -1;

    if((*nodeids)[0] == Nodes()[0]->Id())
      insert = 0;
    else if((*nodeids)[0] == Nodes()[1]->Id())
      insert = 1;

    if (insert == -1)
      dserror("\nNode could not be found on nodemap!\n");

    //add forces to Res_external according to (5.56). There is a factor (-1) needed, as fext is multiplied by (-1) in BACI
    for(int i = 0; i < 3 ; i++)
    {
      elevec1(insert*dofpn + i) += (*onoff)[i]*(*val)[i]*curvefac[i];
    }

    //matrix for current tangent, moment at node and crossproduct
    LINALG::Matrix<3,1> tangent;
    LINALG::Matrix<3,1> crossproduct;
    LINALG::Matrix<3,1> moment;
    LINALG::Matrix<3,3> spinmatrix;

    //clear all matrices
    tangent.Clear();
    crossproduct.Clear();
    moment.Clear();
    spinmatrix.Clear();

    //assemble current tangent and moment at node
    for (int dof = 3 ; dof < 6 ; dof++)
    {
      //get current tangent at nodes
      tangent(dof-3) = Tref_[insert](dof-3) + mydisp[insert*dofpn + dof];
      moment(dof-3) = (*onoff)[dof]*(*val)[dof]*curvefac[dof];
    }

    double abs_tangent = 0.0;

    //Res will be normalized with the length of the current tangent
    abs_tangent = tangent.Norm2();

    //computespin = S ( tangent ) using the spinmatrix in namespace largerotations
    LARGEROTATIONS::computespin(spinmatrix,tangent);

    //matrixoperation crossproduct = t x m
    for(int i=0; i<3; i++)
    {
      for(int j=0; j<3; j++)
      {
        crossproduct(i) += spinmatrix(i,j) * moment(j);
      }
    }

    //add moments to Res_external according to (5.56). There is a factor (-1) needed, as fext is multiplied by (-1) in BACI
    for(int i = 3; i < 6 ; i++)
    {
      #ifndef SIMPLECALC
        elevec1(insert*dofpn + i) -= crossproduct(i-3) / pow(abs_tangent,2.0);
      #else
        elevec1(insert*dofpn + i) -= crossproduct(i-3) *ScaleFactorLine;
      #endif
    }

    //assembly for stiffnessmatrix
    LINALG::Matrix<3,3> crossxtangent;

    crossxtangent.Clear();

    //perform matrix operation
    for(int i=0; i<3; i++)
    {
      for(int j=0; j<3; j++)
      {
        crossxtangent(i,j) = crossproduct(i) * tangent(j);
      }
    }

    spinmatrix.Clear();

    //spinmatrix = S ( m )
    LARGEROTATIONS::computespin(spinmatrix,moment);

    //add R_external to stiffness matrix
    //all parts have been evaluated at the boundaries which helps simplifying the matrices
    //In contrast to the Neumann part of the residual force here is NOT a factor of (-1) needed, as elemat1 is directly added to the stiffness matrix
    //without sign change.
    if(elemat1!=NULL)
    {
      for(int i = 3; i < 6 ; i++)
      {
        for(int j = 3; j < 6 ; j++)
        {
          #ifndef SIMPLECALC
            (*elemat1)(insert*dofpn + i, insert*dofpn + j) -= 2.0 * crossxtangent(i-3,j-3) / pow(abs_tangent,4.0);
            (*elemat1)(insert*dofpn + i, insert*dofpn + j) -= spinmatrix(i-3,j-3) / pow(abs_tangent,2.0);
          #else
            (*elemat1)(insert*dofpn + i, insert*dofpn + j) -= 2.0 * crossxtangent(i-3,j-3);
            (*elemat1)(insert*dofpn + i, insert*dofpn + j) -= spinmatrix(i-3,j-3);
          #endif
        }
      }
    }
  }
  //if a line neumann condition needs to be linearized
  else if(condition.Type() == DRT::Condition::LineNeumann)
  {
    #ifdef SIMPLECALC
      dserror("SIMPLECALC not implemented for LineNeumann conditions so far!!!");
    #endif

    #ifndef BEAM3EBDISCRETELINENEUMANN
      // gaussian points
      DRT::UTILS::IntegrationPoints1D gausspoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::mygaussruleeb);
    #endif

    LINALG::Matrix<1,NODALDOFS*nnodes> N_i;

    #ifndef BEAM3EBDISCRETELINENEUMANN
    //integration loops
    for (int numgp=0; numgp<gausspoints.nquad; ++numgp)
    {

      //integration points in parameter space and weights
      const double xi = gausspoints.qxg[numgp][0];
      const double wgt = gausspoints.qwgt[numgp];

      //Get DiscretizationType of beam element
      const DRT::Element::DiscretizationType distype = Shape();

      //Clear matrix for shape functions
      N_i.Clear();

      //evaluation of shape funcitons at Gauss points
      #if (NODALDOFS == 2)
      //Get hermite derivatives N'xi and N''xi (jacobi_*2.0 is length of the element)
      DRT::UTILS::shape_function_hermite_1D(N_i,xi,jacobi_*2.0,distype);
      //end--------------------------------------------------------
      #elif (NODALDOFS == 3)
      //specific-for----------------------------------Frenet Serret
      //Get hermite derivatives N'xi, N''xi and N'''xi
      DRT::UTILS::shape_function_hermite_1D_order5(N_i,xi,jacobi_*2.0,distype);
      //end--------------------------------------------------------
      #else
      dserror("Only the values NODALDOFS = 2 and NODALDOFS = 3 are valid!");
      #endif

      //position vector at the gauss point at reference configuration needed for function evaluation
      std::vector<double> X_ref(3,0.0);
      //calculate coordinates of corresponding Guass point in reference configuration
      for (int node=0;node<2;node++)
      {
        #if (NODALDOFS == 2)
        for (int dof=0;dof<3;dof++)
        {
          X_ref[dof]+=Nodes()[node]->X()[dof]*N_i(2*node)+Tref_[node](dof)*N_i(2*node + 1);
        }
        #elif (NODALDOFS ==3)
        for (int dof=0;dof<3;dof++)
        {
          X_ref[dof]+=Nodes()[node]->X()[dof]*N_i(3*node)+Tref_[node](dof)*N_i(3*node + 1) + Kref_[node](dof)*N_i(3*node + 2);
        }
        #else
        dserror("Only the values NODALDOFS = 2 and NODALDOFS = 3 are valid!");
        #endif
      }

      double fac=0.0;
      fac = wgt * jacobi_;

      // load vector ar
      double ar[6];

      // loop the dofs of a node
      for (int dof=0; dof<6; ++dof)
        ar[dof] = fac * (*onoff)[dof]*(*val)[dof]*curvefac[dof];
      double functionfac = 1.0;
      int functnum = -1;

      //Check if also moment line Neumann conditions are implemented accidentally and throw error
      for (int dof=3; dof<6; ++dof)
      {
        if (functions) functnum = (*functions)[dof];
        else functnum = -1;

        if (functnum>0)
        {
          dserror("Line Neumann conditions for distributed moments are not implemented for beam3eb so far! Only the function flag 1, 2 and 3 can be set!");
        }
      }

      //sum up load components
      for (int dof=0; dof<3; ++dof)
      {
        if (functions) functnum = (*functions)[dof];
        else functnum = -1;

        if (functnum>0)
        {
          // evaluate function at the position of the current node       --> dof here correct?
          functionfac = DRT::Problem::Instance()->Funct(functnum-1).Evaluate(dof, &X_ref[0], time, NULL);
        }
        else functionfac = 1.0;

        for (int node=0; node<2*NODALDOFS; ++node)
        {
          #ifndef INEXTENSIBLE
            elevec1[node*3 + dof] += N_i(node) *ar[dof] *functionfac;
          #else
            if(node<2)
              elevec1[node*3 + dof] += N_i(node) *ar[dof] *functionfac;
            else
              elevec1[node*3 + dof + 1] += N_i(node) *ar[dof] *functionfac;
          #endif
        }
      }
    } // for (int numgp=0; numgp<intpoints.nquad; ++numgp)
    #else
    //hack in order to realize a discrete point force at position xi=0.0
    {
      //integration points in parameter space and weights
      const double xi = BEAM3EBDISCRETELINENEUMANN;

      //Get DiscretizationType of beam element
      const DRT::Element::DiscretizationType distype = Shape();

      //Clear matrix for shape functions
      N_i.Clear();

      //evaluation of shape funcitons at Gauss points
      #if (NODALDOFS == 2)
      //Get hermite derivatives N'xi and N''xi (jacobi_*2.0 is length of the element)
      DRT::UTILS::shape_function_hermite_1D(N_i,xi,jacobi_*2.0,distype);
      //end--------------------------------------------------------
      #elif (NODALDOFS == 3)
      //specific-for----------------------------------Frenet Serret
      //Get hermite derivatives N'xi, N''xi and N'''xi
      DRT::UTILS::shape_function_hermite_1D_order5(N_i,xi,jacobi_*2.0,distype);
      //end--------------------------------------------------------
      #else
      dserror("Only the values NODALDOFS = 2 and NODALDOFS = 3 are valid!");
      #endif

      // load vector ar
      double ar[6];

      // loop the dofs of a node
      for (int dof=0; dof<6; ++dof)
        ar[dof] = (*onoff)[dof]*(*val)[dof]*curvefac[dof];

      for(int dof=0;dof<3;++dof)
      {
        if(ar[dof+3]!=0)
          dserror("No discrete moment loads in the elements interior implemented so far!");
      }

      //sum up load components
      for (int dof=0; dof<3; ++dof)
      {
        for (int node=0; node<2*NODALDOFS; ++node)
        {
          #ifndef INEXTENSIBLE
            elevec1[node*3 + dof] += N_i(node) *ar[dof];
          #else
            if(node<2)
              elevec1[node*3 + dof] += N_i(node) *ar[dof];
            else
              elevec1[node*3 + dof + 1] += N_i(node) *ar[dof];
          #endif
        }
      }
    }
  #endif
  }//else if(condition.Type() == DRT::Condition::LineNeumann)

  //elevec1.Print(std::cout);

  //Uncomment the next line if the implementation of the Neumann part of the analytical stiffness matrix should be checked by Forward Automatic Differentiation (FAD)
  //FADCheckNeumann(params, discretization, condition, lm, elevec1, elemat1);

  return (0);

}  //DRT::ELEMENTS::Beam3eb::EvaluateNeumann



/*------------------------------------------------------------------------------------------------------------*
 | nonlinear stiffness and mass matrix (private)                                                   meier 05/12|
 *-----------------------------------------------------------------------------------------------------------*/
void DRT::ELEMENTS::Beam3eb::eb_nlnstiffmass(Teuchos::ParameterList& params,
                                              std::vector<double>& vel,
                                              std::vector<double>& disp,
                                              Epetra_SerialDenseMatrix* stiffmatrix,
                                              Epetra_SerialDenseMatrix* massmatrix,
                                              Epetra_SerialDenseVector* force)
{

  //number of nodes fixed for these element
  const int nnode = 2;

  /*first displacement vector is modified for proper element evaluation in case of periodic boundary conditions; in case that
   *no periodic boundary conditions are to be applied the following code line may be ignored or deleted*/
  // Only for statmech problems
  if(params.isParameter("PERIODLENGTH"))
    NodeShift<nnode,3>(params,disp);

  Eint_=0.0;
  Eint_axial_=0.0;
  Ekin_=0.0;
  L_.Clear();
  P_.Clear();
  kappa_max_=0.0;
  epsilon_max_=0.0;

#ifdef SIMPLECALC
{
  //dimensions of freedom per node
  const int dofpn = 3*NODALDOFS;

  //number of nodes fixed for these element
  const int nnode = 2;

  //matrix for current positions and tangents
  std::vector<double> disp_totlag(nnode*dofpn,0.0);

  LINALG::Matrix<3,1> r_;
  LINALG::Matrix<3,1> r_x;
  LINALG::Matrix<3,1> r_xx;

  LINALG::Matrix<3,1> f1;
  LINALG::Matrix<3,1> f2;
  LINALG::Matrix<3,1> n1;

  double rxxrxx;
  double rxrx;
  double tension;

  LINALG::Matrix<dofpn*nnode,dofpn*nnode> NTildex;
  LINALG::Matrix<dofpn*nnode,dofpn*nnode> NTildexx;

  LINALG::Matrix<dofpn*nnode,1> NxTrx;
  LINALG::Matrix<dofpn*nnode,1> NxxTrxx;

  LINALG::Matrix<dofpn*nnode,dofpn*nnode> M2;
  LINALG::Matrix<dofpn*nnode,dofpn*nnode> NxTrxrxTNx;

  //Matrices for N_i,xi and N_i,xixi. 2*nnode due to hermite shapefunctions
  LINALG::Matrix<1,NODALDOFS*nnode> N_i;
  LINALG::Matrix<1,NODALDOFS*nnode> N_i_x;
  LINALG::Matrix<1,NODALDOFS*nnode> N_i_xx;

  LINALG::Matrix<3,nnode*dofpn> N;
  LINALG::Matrix<3,nnode*dofpn> N_x;
  LINALG::Matrix<3,nnode*dofpn> N_xx;

  //stiffness due to tension and bending
  LINALG::Matrix<nnode*dofpn,nnode*dofpn> R_tension;
  LINALG::Matrix<nnode*dofpn,nnode*dofpn> R_bending;

  //internal force due to tension and bending
  LINALG::Matrix<nnode*dofpn,1> Res_tension;
  LINALG::Matrix<nnode*dofpn,1> Res_bending;

  //some matrices necessary for ANS approach
  #ifdef ANS_BEAM3EB
    #if (NODALDOFS ==3)
    dserror("ANS_BEAM3EB approach so far only defined for third order Hermitian shape functions, set NODALDOFS=2!!!");
    #endif
  LINALG::Matrix<1,3> L_i;
  L_i.Clear();
  LINALG::Matrix<nnode*dofpn,1> Res_tension_ANS;
  Res_tension_ANS.Clear();
  LINALG::Matrix<nnode*dofpn,nnode*dofpn> R_tension_ANS;
  R_tension_ANS.Clear();
  double epsilon_ANS = 0.0;
  LINALG::Matrix<1,nnode*dofpn> lin_epsilon_ANS;
  lin_epsilon_ANS.Clear();
  #endif

  //first of all we get the material law
  Teuchos::RCP<const MAT::Material> currmat = Material();
  double ym = 0;
  //Uncomment the next line for the dynamic case: so far only the static case is implemented
  //double density = 0;

  //assignment of material parameters; only St.Venant material is accepted for this beam
  switch(currmat->MaterialType())
  {
    case INPAR::MAT::m_stvenant:// only linear elastic material supported
    {
      const MAT::StVenantKirchhoff* actmat = static_cast<const MAT::StVenantKirchhoff*>(currmat.get());
      ym = actmat->Youngs();
      //Uncomment the next line for the dynamic case: so far only the static case is implemented
      //density = actmat->Density();
    }
    break;
    default:
    dserror("unknown or improper type of material law");
    break;
  }

  //Get integrationpoints for exact integration
  DRT::UTILS::IntegrationPoints1D gausspoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::mygaussruleeb);

  //Get DiscretizationType of beam element
  const DRT::Element::DiscretizationType distype = Shape();

  //update displacement vector /d in thesis Meier d = [ r1 t1 r2 t2]
  for (int node = 0 ; node < nnode ; node++)
  {
    for (int dof = 0 ; dof < dofpn ; dof++)
    {
      if(dof < 3)
      {
        //position of nodes
        disp_totlag[node*dofpn + dof] = (Nodes()[node]->X()[dof] + disp[node*dofpn + dof])*ScaleFactorColumn;
      }
      else if(dof<6)
      {
        //tangent at nodes
        disp_totlag[node*dofpn + dof] = (Tref_[node](dof-3) + disp[node*dofpn + dof])*ScaleFactorColumn;
      }
      else if(dof>=6)
      {
        #if NODALDOFS ==3
        //curvatures at nodes
        disp_totlag[node*dofpn + dof] = (Kref_[node](dof-6) + disp[node*dofpn + dof])*ScaleFactorColumn;
        #endif
      }
    }
  } //for (int node = 0 ; node < nnode ; node++)

  //Calculate epsilon at collocation points
  #ifdef ANS_BEAM3EB
  LINALG::Matrix<3,1> epsilon_cp;
  epsilon_cp.Clear();
  LINALG::Matrix<3,3> tangent_cp;
  tangent_cp.Clear();
  LINALG::Matrix<3,NODALDOFS*6> lin_epsilon_cp;
  lin_epsilon_cp.Clear();

  N_i_x.Clear();
  DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,0.0,jacobi_*2.0,distype);
  for (int i=0;i<2*NODALDOFS;i++)
  {
    N_i_x(i)=N_i_x(i)/jacobi_;
  }

  for (int i=0;i<3;i++)
  {
    tangent_cp(i,0)=disp_totlag[i+3];
    tangent_cp(i,1)=disp_totlag[i+9];

    for (int j=0;j<2*NODALDOFS;j++)
    {
      tangent_cp(i,2)+=N_i_x(j)*disp_totlag[3*j+i];
    }
  }
  for (int i=0;i<3;i++)
  {
    for (int j=0;j<3;j++)
    {
      epsilon_cp(i)+=tangent_cp(j,i)*tangent_cp(j,i);
    }
    epsilon_cp(i)=pow(epsilon_cp(i),0.5)-1.0;
  }

  for (int k=0;k<3;k++)
  {
    N_i_x.Clear();

    switch (k)
    {
    case 0:
      DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,-1.0,jacobi_*2.0,distype);
      break;
    case 1:
      DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,1.0,jacobi_*2.0,distype);
      break;
    case 2:
      DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,0.0,jacobi_*2.0,distype);
      break;
    default:
      dserror("Index k should only run from 1 to 3 (three collocation points)!");
     break;
    }

    for (int i=0;i<2*NODALDOFS;i++)
    {
      N_i_x(i)=N_i_x(i)/jacobi_;
    }
    //loop over space dimensions
    for (int i=0;i<3;i++)
    { //loop over all shape functions
      for (int j=0;j<2*NODALDOFS;j++)
      { //loop over CPs
          lin_epsilon_cp(k,3*j + i)+=tangent_cp(i,k)*N_i_x(j)/(epsilon_cp(k)+1);
      }
    }
  }
  #endif

  //Loop through all GP and calculate their contribution to the internal forcevector and stiffnessmatrix
  for(int numgp=0; numgp < gausspoints.nquad; numgp++)
  {
    //all matrices and scalars are set to zero again!!!
    //factors for stiffness assembly

    r_.Clear();
    r_x.Clear();
    r_xx.Clear();

    f1.Clear();
    f2.Clear();
    n1.Clear();

    rxxrxx=0;
    rxrx=0;
    tension=0;

    NTildex.Clear();
    NTildexx.Clear();

    NxTrx.Clear();
    NxxTrxx.Clear();

    M2.Clear();
    NxTrxrxTNx.Clear();

    N_i.Clear();
    N_i_x.Clear();
    N_i_xx.Clear();

    N.Clear();
    N_x.Clear();
    N_xx.Clear();

    R_tension.Clear();
    R_bending.Clear();

    Res_tension.Clear();
    Res_bending.Clear();

    //Get location and weight of GP in parameter space
    const double xi = gausspoints.qxg[numgp][0];
    const double wgt = gausspoints.qwgt[numgp];

#if (NODALDOFS == 2)
    //Get hermite derivatives N'xi and N''xi (jacobi_*2.0 is length of the element)
    DRT::UTILS::shape_function_hermite_1D(N_i,xi,jacobi_*2.0,distype);
    DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,xi,jacobi_*2.0,distype);
    DRT::UTILS::shape_function_hermite_1D_deriv2(N_i_xx,xi,jacobi_*2.0,distype);
    //end--------------------------------------------------------
#elif (NODALDOFS == 3)
    //specific-for----------------------------------Frenet Serret
    //Get hermite derivatives N'xi, N''xi and N'''xi
    DRT::UTILS::shape_function_hermite_1D_order5(N_i,xi,jacobi_*2.0,distype);
    DRT::UTILS::shape_function_hermite_1D_order5_deriv1(N_i_x,xi,jacobi_*2.0,distype);
    DRT::UTILS::shape_function_hermite_1D_order5_deriv2(N_i_xx,xi,jacobi_*2.0,distype);
    //end--------------------------------------------------------
#else
    dserror("Only the values NODALDOFS = 2 and NODALDOFS = 3 are valid!");
#endif


    //calculate r' and r''
    for (int i=0 ; i < 3 ; i++)
    {
      for (int j=0; j<2*NODALDOFS; j++)
      {
        r_(i,0)+= N_i(j)*disp_totlag[3*j + i];
        r_x(i,0)+= N_i_x(j) * disp_totlag[3*j + i];
        r_xx(i,0)+= N_i_xx(j) * disp_totlag[3*j + i];
      }
    }

    for (int i=0; i<3; i++)
    {
      rxxrxx+=r_xx(i)*r_xx(i);
      rxrx+=r_x(i)*r_x(i);
    }

    tension = 1/jacobi_ - 1/pow(rxrx,0.5);

    for (int i=0; i<3; ++i)
    {
      for (int j=0; j<2*NODALDOFS; ++j)
      {
        N(i,i+3*j) += N_i(j);
        N_x(i,i+3*j) += N_i_x(j);
        N_xx(i,i+3*j) += N_i_xx(j);
        NxTrx(i+3*j)+=N_i_x(j)*r_x(i);
        NxxTrxx(i+3*j)+=N_i_xx(j)*r_xx(i);
      }
    }

    NTildex.MultiplyTN(N_x,N_x);
    NTildexx.MultiplyTN(N_xx,N_xx);

    for (int i= 0; i<nnode*dofpn; i++)
    {
      for (int j= 0; j<nnode*dofpn; j++)
      {
        M2(i,j)+= NxxTrxx(i)*NxTrx(j);
        NxTrxrxTNx(i,j)+= NxTrx(i)*NxTrx(j);
      }
    }

#ifdef ANS_BEAM3EB
    DRT::UTILS::shape_function_1D(L_i,xi,line3);
    epsilon_ANS = 0.0;
    lin_epsilon_ANS.Clear();
    for (int i=0;i<ANSVALUES;i++)
    {
      epsilon_ANS+=L_i(i)*epsilon_cp(i);
      for (int j=0;j<nnode*dofpn;j++)
      {
        lin_epsilon_ANS(j)+=L_i(i)*lin_epsilon_cp(i,j);
      }
    }

    Res_tension_ANS.Clear();
    R_tension_ANS.Clear();

    for (int i=0;i<nnode*dofpn;i++)
    {
      for (int j=0;j<nnode*dofpn;j++)
      {
        R_tension_ANS(i,j)+=NxTrx(i)*lin_epsilon_ANS(j)/jacobi_;
      }
    }
#endif

    //assemble internal stiffness matrix / R = d/(dd) Res in thesis Meier
    if (stiffmatrix != NULL)
    {

      //assemble parts from tension
      #ifndef ANS_BEAM3EB
      R_tension = NTildex;
      R_tension.Scale(tension);
      R_tension.Update(1.0 / pow(rxrx,1.5),NxTrxrxTNx,1.0);
      R_tension.Scale(ym * crosssec_ * wgt);
      #else
      //attention: in epsilon_ANS and lin_epsilon_ANS the corresponding jacobi factors are allready considered,
      //all the other jacobi factors due to differentiation and integration cancel out!!!
      R_tension_ANS.Update(epsilon_ANS/jacobi_,NTildex,1.0);
      R_tension_ANS.Scale(ym * crosssec_ * wgt);
      #endif

      //assemble parts from bending
      R_bending.Update(-rxxrxx/pow(jacobi_,2.0) ,NTildex,1.0);
      R_bending.Update(1.0,NTildexx,1.0);
      R_bending.UpdateT(- 2.0 / pow(jacobi_,2.0) , M2 , 1.0);

      R_bending.Scale(ym * Izz_ * wgt / pow(jacobi_,3));

      //shifting values from fixed size matrix to epetra matrix *stiffmatrix
      for(int i = 0; i < dofpn*nnode; i++)
      {
        for(int j = 0; j < dofpn*nnode; j++)
        {
          #ifndef ANS_BEAM3EB
          (*stiffmatrix)(i,j) += R_tension(i,j);
          #else
          (*stiffmatrix)(i,j) += R_tension_ANS(i,j);
          #endif
          (*stiffmatrix)(i,j) += R_bending(i,j);
        }
      } //for(int i = 0; i < dofpn*nnode; i++)
    }//if (stiffmatrix != NULL)

    for (int i= 0; i<3; i++)
    {
      f1(i)=-r_x(i)*rxxrxx;
      f2(i)=r_xx(i);
      n1(i)=r_x(i)*tension;
    }

    //assemble internal force vector f_internal / Res in thesis Meier
    if (force != NULL)
    {
      for (int i=0;i<3;i++)
      {
        for (int j=0;j<2*NODALDOFS;j++)
        {
          Res_bending(j*3 + i)+= N_i_x(j)*f1(i)/pow(jacobi_,5) + N_i_xx(j)*f2(i)/pow(jacobi_,3);
          #ifndef ANS_BEAM3EB
          Res_tension(j*3 + i)+= N_i_x(j)*n1(i);
          #endif
        }
      }
      #ifdef ANS_BEAM3EB
      Res_tension_ANS.Update(ym * crosssec_ * wgt*epsilon_ANS / jacobi_,NxTrx,1.0);
      #endif
      Res_bending.Scale(ym * Izz_ * wgt);
      Res_tension.Scale(ym * crosssec_ * wgt);

      //shifting values from fixed size vector to epetra vector *force
      for(int i = 0; i < dofpn*nnode; i++)
      {
        #ifndef ANS_BEAM3EB
        (*force)(i) += Res_tension(i);
        #else
        (*force)(i) += Res_tension_ANS(i);
        #endif
        (*force)(i) += Res_bending(i);
      }
    } //if (force != NULL)

    //assemble massmatrix if requested
    //calculating mass matrix (local version = global version)
    //note: the mass matrix currently implemented is just a dummy and should not yet be used
    if (massmatrix != NULL)
    {
      for (int i=0; i<6*nnode; i++)
        (*massmatrix)(i,i) = 1;

    }//if (massmatrix != NULL)
  } //for(int numgp=0; numgp < gausspoints.nquad; numgp++)

  //Uncomment the following line to print the elment stiffness matrix to matlab format
  /*
  const std::string fname = "stiffmatrixele.mtl";
  std::cout<<"Printing stiffmatrixele to file"<<std::endl;
  LINALG::PrintSerialDenseMatrixInMatlabFormat(fname,*stiffmatrix);
  */

//  //with the following lines the conditioning of the stiffness matrix should be improved: its not fully tested up to now!!!
//  bool precond = PreConditioning;
//  if (precond)
//  {
//    #if NODALDOFS ==3
//      dserror("Preconditioning only implemented for NODALDOFS ==2!!!");
//    #endif
//    double length = jacobi_*2.0;
//    double radius = std::pow(crosssec_/M_PI,0.5);
//    for (int zeile=0; zeile <2; zeile++)
//    {
//      for (int spalte=0; spalte<12; spalte++)
//      {
//        (*stiffmatrix)(6*zeile,spalte)=(*stiffmatrix)(6*zeile,spalte)*length;
//        (*stiffmatrix)(6*zeile+1,spalte)=(*stiffmatrix)(6*zeile+1,spalte)*pow(length,3.0)/pow(radius,2.0);
//        (*stiffmatrix)(6*zeile+2,spalte)=(*stiffmatrix)(6*zeile+2,spalte)*pow(length,3.0)/pow(radius,2.0);
//        (*stiffmatrix)(6*zeile+4,spalte)=(*stiffmatrix)(6*zeile+4,spalte)*pow(length,2.0)/pow(radius,2.0);
//        (*stiffmatrix)(6*zeile+5,spalte)=(*stiffmatrix)(6*zeile+5,spalte)*pow(length,2.0)/pow(radius,2.0);
//      }
//        (*force)(6*zeile)=(*force)(6*zeile)*length;
//        (*force)(6*zeile+1)=(*force)(6*zeile+1)*pow(length,3.0)/pow(radius,2.0);
//        (*force)(6*zeile+2)=(*force)(6*zeile+2)*pow(length,3.0)/pow(radius,2.0);
//        (*force)(6*zeile+4)=(*force)(6*zeile+4)*pow(length,2.0)/pow(radius,2.0);
//        (*force)(6*zeile+5)=(*force)(6*zeile+5)*pow(length,2.0)/pow(radius,2.0);
//    }
//  }
//
//  //with the following lines the conditioning of the stiffness matrix can be improved by multiplying the lines and columns with the factors
//  //ScaleFactorLine and ScaleFactorColumn
//  double Factor = ScaleFactorLine;
//  Factor = Factor * ScaleFactorColumn;
//
//  for (int zeile=0; zeile <nnode*dofpn; zeile++)
//  {
//    for (int spalte=0; spalte<nnode*dofpn; spalte++)
//    {
//      (*stiffmatrix)(zeile,spalte)=(*stiffmatrix)(zeile,spalte)*Factor;
//    }
//    (*force)(zeile)=(*force)(zeile)*ScaleFactorLine;
//  }
}
#else
{
   //dimensions of freedom per node
  const int dofpn = 3*NODALDOFS;

  #ifdef ORTHOPRESSURE
  const double time = params.get("total time",-1.0);
  double orthopressureload = 0.0;
    if(time > 1.0)
      orthopressureload = ORTHOPRESSURE * (time-1.0)/0.1;
    if(time > 1.1)
      orthopressureload = ORTHOPRESSURE;
  #endif

  //matrix for current positions and tangents
  std::vector<double> disp_totlag(nnode*dofpn, 0.0);

  #ifdef BEAM3EBAUTOMATICDIFF
    std::vector<FAD> disp_totlag_fad(nnode*dofpn, 0.0);
  #endif

  #ifdef INEXTENSIBLE
    std::vector<FAD> lm_fad(3, 0.0);
    LINALG::TMatrix<FAD,15,1> Res_inextensibility(true);
    LINALG::TMatrix<FAD,15,15> R_inextensibility(true);
  #endif

  LINALG::Matrix<3,1> r_;
  LINALG::Matrix<3,1> r_x;
  LINALG::Matrix<3,1> r_xx;

  LINALG::Matrix<3,1> f1;
  LINALG::Matrix<3,1> f2;
  LINALG::Matrix<3,1> n1;

  double rxrxx;
  double rxxrxx;
  double rxrx;
  double tension;

  #ifdef BEAM3EBAUTOMATICDIFF
    LINALG::TMatrix<FAD,3,1> rx_fad;
    LINALG::TMatrix<FAD,3,1> ortho_normal(true);
    FAD rxrx_fad;
  #endif

  LINALG::Matrix<dofpn*nnode,dofpn*nnode> NTilde;
  LINALG::Matrix<dofpn*nnode,dofpn*nnode> NTildex;
  LINALG::Matrix<dofpn*nnode,dofpn*nnode> NTildexx;

  LINALG::Matrix<dofpn*nnode,1> NxTrx;
  LINALG::Matrix<dofpn*nnode,1> NxTrxx;
  LINALG::Matrix<dofpn*nnode,1> NxxTrx;
  LINALG::Matrix<dofpn*nnode,1> NxxTrxx;

  LINALG::Matrix<dofpn*nnode,dofpn*nnode> M1;
  LINALG::Matrix<dofpn*nnode,dofpn*nnode> M2;
  LINALG::Matrix<dofpn*nnode,dofpn*nnode> M3;
  LINALG::Matrix<dofpn*nnode,dofpn*nnode> NxTrxrxTNx;

  //Matrices for N_i,xi and N_i,xixi. 2*nnode due to hermite shapefunctions
  LINALG::Matrix<1,NODALDOFS*nnode> N_i;
  LINALG::Matrix<1,NODALDOFS*nnode> N_i_x;
  LINALG::Matrix<1,NODALDOFS*nnode> N_i_xx;

  #ifdef BEAM3EBAUTOMATICDIFF
    LINALG::TMatrix<FAD,3,nnode*dofpn> N;
  #endif
  LINALG::Matrix<3,nnode*dofpn> N_x;
  LINALG::Matrix<3,nnode*dofpn> N_xx;

  //stiffness due to tension and bending
  LINALG::Matrix<nnode*dofpn,nnode*dofpn> R_tension;
  LINALG::Matrix<nnode*dofpn,nnode*dofpn> R_bending;
  LINALG::Matrix<nnode*dofpn,nnode*dofpn> R_orthopressure;

  //internal force due to tension and bending
  LINALG::Matrix<nnode*dofpn,1> Res_tension;
  LINALG::Matrix<nnode*dofpn,1> Res_bending;
  #ifdef BEAM3EBAUTOMATICDIFF
    LINALG::TMatrix<FAD,nnode*dofpn,1> Res_orthopressure;
  #endif

  //some matrices necessary for ANS approach
  #ifdef ANS_BEAM3EB
    #if (NODALDOFS ==3)
      dserror("ANS approach so far only defined for third order Hermitian shape functions, set NODALDOFS=2!!!");
    #endif
    LINALG::Matrix<1,3> L_i;
    L_i.Clear();
    LINALG::Matrix<nnode*dofpn,1> Res_tension_ANS;
    Res_tension_ANS.Clear();
    LINALG::Matrix<nnode*dofpn,nnode*dofpn> R_tension_ANS;
    R_tension_ANS.Clear();
    double epsilon_ANS = 0.0;
    LINALG::Matrix<1,nnode*dofpn> lin_epsilon_ANS(true);

    #ifdef BEAM3EBAUTOMATICDIFF
      LINALG::TMatrix<FAD,1,nnode*dofpn> lin_epsilon_ANS_fad(true);

      LINALG::TMatrix<FAD,nnode*dofpn,1> Res_tension_ANS_fad;
      Res_tension_ANS_fad.Clear();
      LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> R_tension_ANS_fad;
      R_tension_ANS_fad.Clear();
      FAD epsilon_ANS_fad = 0.0;
    #endif
  #endif

  //first of all we get the material law
  Teuchos::RCP<const MAT::Material> currmat = Material();
  double ym = 0;
  //Uncomment the next line for the dynamic case: so far only the static case is implemented
  double density = 0;

  //assignment of material parameters; only St.Venant material is accepted for this beam
  switch(currmat->MaterialType())
  {
    case INPAR::MAT::m_stvenant:// only linear elastic material supported
    {
      const MAT::StVenantKirchhoff* actmat = static_cast<const MAT::StVenantKirchhoff*>(currmat.get());
      ym = actmat->Youngs();
      //Uncomment the next line for the dynamic case: so far only the static case is implemented
      density = actmat->Density();
    }
    break;
    default:
    dserror("unknown or improper type of material law");
    break;
  }

  //Get integrationpoints for exact integration
  DRT::UTILS::IntegrationPoints1D gausspoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::mygaussruleeb);

  //Get DiscretizationType of beam element
  const DRT::Element::DiscretizationType distype = Shape();

#ifndef INEXTENSIBLE
  //update displacement vector /d in thesis Meier d = [ r1 t1 r2 t2]
  for (int node = 0 ; node < nnode ; node++)
  {
    for (int dof = 0 ; dof < dofpn ; dof++)
    {
      if(dof < 3)
      {
        //position of nodes
        disp_totlag[node*dofpn + dof] = (Nodes()[node]->X()[dof] + disp[node*dofpn + dof]);
      }
      else if(dof<6)
      {
        //tangent at nodes
        disp_totlag[node*dofpn + dof] = (Tref_[node](dof-3) + disp[node*dofpn + dof]);
      }
      else if(dof>=6)
      {
        #if NODALDOFS ==3
        //curvatures at nodes
        disp_totlag[node*dofpn + dof] = (Kref_[node](dof-6) + disp[node*dofpn + dof])*ScaleFactorColumn;
        #endif
      }
    }
  } //for (int node = 0 ; node < nnode ; node++)
#else
  //update displacement vector /d in thesis Meier d = [ r1 t1 r2 t2]
  for (int node = 0 ; node < 2 ; node++)
  {
    for (int dof = 0 ; dof < 6 ; dof++)
    {
      if(dof < 3)
      {
        //position of nodes
        disp_totlag[node*6 + dof] = (Nodes()[node]->X()[dof] + disp[node*7 + dof]);
      }
      else if(dof<6)
      {
        //tangent at nodes
        disp_totlag[node*6 + dof] = (Tref_[node](dof-3) + disp[node*7 + dof]);
      }
    }
  } //for (int node = 0 ; node < nnode ; node++)
#endif

  #ifndef INEXTENSIBLE
    #ifdef BEAM3EBAUTOMATICDIFF
      for (int dof=0;dof<nnode*dofpn;dof++)
      {
        disp_totlag_fad[dof]=disp_totlag[dof];
        disp_totlag_fad[dof].diff(dof,nnode*dofpn);
      }
    #endif
  #else
    for (int dof=0;dof<nnode*dofpn;dof++)
    {
      disp_totlag_fad[dof]=disp_totlag[dof];
      disp_totlag_fad[dof].diff(dof,15);
    }
    lm_fad[0]=disp[6];
    lm_fad[0].diff(12,15);
    lm_fad[1]=disp[13];
    lm_fad[1].diff(13,15);
    lm_fad[2]=disp[14];
    lm_fad[2].diff(14,15);
  #endif

  //Set class variables
  for (int i=0;i<3;i++)
  {
    t_(i,0)=disp_totlag[3+i];
    t_(i,1)=disp_totlag[9+i];
  }

  double tangentnorm1 = sqrt(disp_totlag[3]*disp_totlag[3]+disp_totlag[4]*disp_totlag[4]+disp_totlag[5]*disp_totlag[5]);
  double tangentnorm2 = sqrt(disp_totlag[9]*disp_totlag[9]+disp_totlag[10]*disp_totlag[10]+disp_totlag[11]*disp_totlag[11]);

  if (tangentnorm1 <1.0e-12 or tangentnorm2 <1.0e-12)
    dserror("Tangent of norm zero --> deformation to large!!!");

  //Calculate epsilon at collocation points
  #ifdef ANS_BEAM3EB
  LINALG::Matrix<3,1> epsilon_cp(true);
  LINALG::Matrix<3,3> tangent_cp(true);
  LINALG::Matrix<3,NODALDOFS*6> lin_epsilon_cp(true);

  #ifdef BEAM3EBAUTOMATICDIFF
    LINALG::TMatrix<FAD,3,1> epsilon_cp_fad(true);
    LINALG::TMatrix<FAD,3,3> tangent_cp_fad(true);
    LINALG::TMatrix<FAD,3,NODALDOFS*6> lin_epsilon_cp_fad(true);
  #endif

  N_i_x.Clear();
  DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,0.0,jacobi_*2.0,distype);
  for (int i=0;i<2*NODALDOFS;i++)
  {
    N_i_x(i)=N_i_x(i)/jacobi_;
  }

  for (int i=0;i<3;i++)
  {
    tangent_cp(i,0)=disp_totlag[i+3];
    tangent_cp(i,1)=disp_totlag[i+9];

    for (int j=0;j<2*NODALDOFS;j++)
    {
      tangent_cp(i,2)+=N_i_x(j)*disp_totlag[3*j+i];
    }

    #ifdef BEAM3EBAUTOMATICDIFF
      tangent_cp_fad(i,0)=disp_totlag_fad[i+3];
      tangent_cp_fad(i,1)=disp_totlag_fad[i+9];
      for (int j=0;j<2*NODALDOFS;j++)
      {
        tangent_cp_fad(i,2)+=N_i_x(j)*disp_totlag_fad[3*j+i];
      }
    #endif
  }
  for (int i=0;i<3;i++)
  {
    for (int j=0;j<3;j++)
    {
      epsilon_cp(i)+=tangent_cp(j,i)*tangent_cp(j,i);
    }
    epsilon_cp(i)=pow(epsilon_cp(i),0.5)-1.0;
  }

  #ifdef BEAM3EBAUTOMATICDIFF
    for (int i=0;i<3;i++)
    {
      for (int j=0;j<3;j++)
      {
        epsilon_cp_fad(i)+=tangent_cp_fad(j,i)*tangent_cp_fad(j,i);
      }
      epsilon_cp_fad(i)=pow(epsilon_cp_fad(i),0.5)-1.0;
    }
  #endif

  for (int k=0;k<3;k++)
  {
    N_i_x.Clear();

    switch (k)
    {
    case 0:
      DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,-1.0,jacobi_*2.0,distype);
      break;
    case 1:
      DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,1.0,jacobi_*2.0,distype);
      break;
    case 2:
      DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,0.0,jacobi_*2.0,distype);
      break;
    default:
      dserror("Index k should only run from 1 to 3 (three collocation points)!");
     break;
    }

    for (int i=0;i<2*NODALDOFS;i++)
    {
      N_i_x(i)=N_i_x(i)/jacobi_;
    }
    //loop over space dimensions
    for (int i=0;i<3;i++)
    { //loop over all shape functions
      for (int j=0;j<2*NODALDOFS;j++)
      { //loop over CPs
        lin_epsilon_cp(k,3*j + i)+=tangent_cp(i,k)*N_i_x(j)/(epsilon_cp(k)+1);
      }
    }

    #ifdef BEAM3EBAUTOMATICDIFF
      //loop over space dimensions
      for (int i=0;i<3;i++)
      { //loop over all shape functions
        for (int j=0;j<2*NODALDOFS;j++)
        { //loop over CPs
          lin_epsilon_cp_fad(k,3*j + i)+=tangent_cp_fad(i,k)*N_i_x(j)/(epsilon_cp_fad(k)+1);
        }
      }
    #endif
  }
  #endif

  #if defined(INEXTENSIBLE)
    for(int i=0;i<2;i++)
    {
      for(int j=0;j<6;j++)
      {
        for(int k=0;k<3;k++)
        {
          Res_inextensibility(j+7*i)+=INEXTENSIBLE*ym*crosssec_*lm_fad[k]*lin_epsilon_cp_fad(k,j+6*i);
        }
      }
    }
    Res_inextensibility(6)+=INEXTENSIBLE*ym*crosssec_*epsilon_cp_fad(0);
    Res_inextensibility(13)+=INEXTENSIBLE*ym*crosssec_*epsilon_cp_fad(1);
    Res_inextensibility(14)+=INEXTENSIBLE*ym*crosssec_*epsilon_cp_fad(2);

    for(int i=0; i<15;i++)
    {
      for(int j=0;j<6;j++)
      {
        R_inextensibility(i,j)=Res_inextensibility(i).dx(j);
        R_inextensibility(i,7+j)=Res_inextensibility(i).dx(6+j);
      }
      R_inextensibility(i,6)=Res_inextensibility(i).dx(12);
      R_inextensibility(i,13)=Res_inextensibility(i).dx(13);
      R_inextensibility(i,14)=Res_inextensibility(i).dx(14);
    }
    #ifdef SWITCHINEXTENSIBLEON
      if (force != NULL)
      {
        //shifting values from fixed size vector to epetra vector *force
        for(int i = 0; i < 15; i++)
        {
          (*force)(i) += Res_inextensibility(i).val();
        }
      } //if (force != NULL)

      //assemble internal stiffness matrix / R = d/(dd) Res in thesis Meier
      if (stiffmatrix != NULL)
      {
        for(int i = 0; i < 15; i++)
        {
          for(int j = 0; j < 15; j++)
          {
            (*stiffmatrix)(i,j) += R_inextensibility(i,j).val();
          }
        } //for(int i = 0; i < dofpn*nnode; i++)
      }//if (stiffmatrix != NULL)
    #else
      (*stiffmatrix)(6,6) += 1.0;
      (*stiffmatrix)(13,13) += 1.0;
      (*stiffmatrix)(14,14) += 1.0;
    #endif
  #endif

  //Loop through all GP and calculate their contribution to the internal forcevector and stiffnessmatrix
  for(int numgp=0; numgp < gausspoints.nquad; numgp++)
  {
    //all matrices and scalars are set to zero again!!!
    //factors for stiffness assembly

    r_.Clear();
    r_x.Clear();
    r_xx.Clear();

    f1.Clear();
    f2.Clear();
    n1.Clear();

    rxrxx=0;
    rxxrxx=0;
    rxrx=0;
    tension=0;

    #ifdef BEAM3EBAUTOMATICDIFF
      rx_fad.Clear();
      rxrx_fad=0.0;
      N.Clear();
    #endif

    NTilde.Clear();
    NTildex.Clear();
    NTildexx.Clear();

    NxTrx.Clear();
    NxTrxx.Clear();
    NxxTrx.Clear();
    NxxTrxx.Clear();

    M1.Clear();
    M2.Clear();
    M3.Clear();
    NxTrxrxTNx.Clear();

    N_i.Clear();
    N_i_x.Clear();
    N_i_xx.Clear();

    N_x.Clear();
    N_xx.Clear();

    R_tension.Clear();
    R_bending.Clear();

    Res_tension.Clear();
    Res_bending.Clear();

    //Get location and weight of GP in parameter space
    const double xi = gausspoints.qxg[numgp][0];
    const double wgt = gausspoints.qwgt[numgp];

#if (NODALDOFS == 2)
    //Get hermite derivatives N'xi and N''xi (jacobi_*2.0 is length of the element)
    DRT::UTILS::shape_function_hermite_1D(N_i,xi,jacobi_*2.0,distype);
    DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,xi,jacobi_*2.0,distype);
    DRT::UTILS::shape_function_hermite_1D_deriv2(N_i_xx,xi,jacobi_*2.0,distype);
    //end--------------------------------------------------------
#elif (NODALDOFS == 3)
    //specific-for----------------------------------Frenet Serret
    //Get hermite derivatives N'xi, N''xi and N'''xi
    DRT::UTILS::shape_function_hermite_1D_order5_deriv1(N_i_x,xi,jacobi_*2.0,distype);
    DRT::UTILS::shape_function_hermite_1D_order5_deriv2(N_i_xx,xi,jacobi_*2.0,distype);
    //end--------------------------------------------------------
#else
    dserror("Only the values NODALDOFS = 2 and NODALDOFS = 3 are valid!");
#endif

    //calculate r' and r''
    for (int i=0 ; i < 3 ; i++)
    {
      for (int j=0; j<nnode*NODALDOFS; j++)
      {
        r_(i,0)+= N_i(j)*disp_totlag[3*j + i];
        r_x(i,0)+= N_i_x(j) * disp_totlag[3*j + i];
        r_xx(i,0)+= N_i_xx(j) * disp_totlag[3*j + i];
      }
    }

    #ifdef BEAM3EBAUTOMATICDIFF
      //calculate r' and r''
      for (int i=0 ; i < 3 ; i++)
      {
        for (int j=0; j<nnode*NODALDOFS; j++)
        {
          rx_fad(i,0)+= N_i_x(j) * disp_totlag_fad[3*j + i];
        }
      }
    #endif

    for (int i= 0; i<3; i++)
    {
      rxrxx+=r_x(i)*r_xx(i);
      rxxrxx+=r_xx(i)*r_xx(i);
      rxrx+=r_x(i)*r_x(i);
    }

    #ifdef BEAM3EBAUTOMATICDIFF
      for (int i= 0; i<3; i++)
      {
        rxrx_fad+=rx_fad(i)*rx_fad(i);
      }
    #endif

    tension = 1/jacobi_ - 1/pow(rxrx,0.5);

    for (int i=0; i<3; ++i)
    {
      for (int j=0; j<nnode*NODALDOFS; ++j)
      {
        N_x(i,i+3*j) += N_i_x(j);
        N_xx(i,i+3*j) += N_i_xx(j);
        NxTrx(i+3*j)+=N_i_x(j)*r_x(i);
        NxTrxx(i+3*j)+=N_i_x(j)*r_xx(i);
        NxxTrx(i+3*j)+=N_i_xx(j)*r_x(i);
        NxxTrxx(i+3*j)+=N_i_xx(j)*r_xx(i);
      }
    }

    #ifdef BEAM3EBAUTOMATICDIFF
      for (int i=0; i<3; ++i)
      {
        for (int j=0; j<nnode*NODALDOFS; ++j)
        {
          N(i,i+3*j) += N_i(j);
        }
      }
    #endif

    #ifdef ORTHOPRESSURE
      ortho_normal(0)=rx_fad(1,0);
      ortho_normal(1)=-rx_fad(0,0);
      ortho_normal(2)=0.0;
      if (FADUTILS::CastToDouble(FADUTILS::VectorNorm<3>(ortho_normal))>1.0e-12)
        ortho_normal.Scale(1.0/(FADUTILS::VectorNorm<3>(ortho_normal)));

      Res_orthopressure.Clear();
      R_orthopressure.Clear();
      Res_orthopressure.MultiplyTN(N,ortho_normal);
      Res_orthopressure.Scale(orthopressureload* wgt *jacobi_);
      for (int i= 0; i<nnode*dofpn; i++)
      {
        for (int j= 0; j<nnode*dofpn; j++)
        {
          R_orthopressure(i,j)=Res_orthopressure(i).dx(j);
        }
      }
    #endif

    NTilde.MultiplyTN(N_x,N_xx);
    NTildex.MultiplyTN(N_x,N_x);
    NTildexx.MultiplyTN(N_xx,N_xx);

    for (int i= 0; i<nnode*dofpn; i++)
    {
      for (int j= 0; j<nnode*dofpn; j++)
      {
        M1(i,j)+= NxTrx(i)*(NxxTrx(j)+NxTrxx(j));
        M2(i,j)+= NxxTrxx(i)*NxTrx(j);
        M3(i,j)+= (NxTrxx(i)+NxxTrx(i))*(NxTrxx(j)+NxxTrx(j));
        NxTrxrxTNx(i,j)+= NxTrx(i)*NxTrx(j);
      }
    }

    //calculate quantities necessary for ANS approach
    #ifdef ANS_BEAM3EB
      DRT::UTILS::shape_function_1D(L_i,xi,line3);
      epsilon_ANS = 0.0;
      lin_epsilon_ANS.Clear();
      for (int i=0;i<ANSVALUES;i++)
      {
        epsilon_ANS+=L_i(i)*epsilon_cp(i);
        for (int j=0;j<nnode*dofpn;j++)
        {
          lin_epsilon_ANS(j)+=L_i(i)*lin_epsilon_cp(i,j);
        }
      }

      #ifdef BEAM3EBAUTOMATICDIFF
        epsilon_ANS_fad = 0.0;
        lin_epsilon_ANS_fad.Clear();
        for (int i=0;i<ANSVALUES;i++)
        {
          epsilon_ANS_fad+=L_i(i)*epsilon_cp_fad(i);
          for (int j=0;j<nnode*dofpn;j++)
          {
            lin_epsilon_ANS_fad(j)+=L_i(i)*lin_epsilon_cp_fad(i,j);
          }
        }

        Res_tension_ANS_fad.Clear();
        R_tension_ANS_fad.Clear();

        #ifndef CONSISTENTANSBEAM3EB
          for (int i=0;i<nnode*dofpn;i++)
          {
            for (int k=0;k<3;k++)
            {
              Res_tension_ANS_fad(i)+=N_x(k,i)*rx_fad(k)/pow(rxrx_fad,0.5)*ym * crosssec_ * wgt*epsilon_ANS_fad;
            }
          }
        #else
          for (int i=0;i<nnode*dofpn;i++)
          {
            Res_tension_ANS_fad(i)+=lin_epsilon_ANS_fad(i)*jacobi_*ym * crosssec_ * wgt*epsilon_ANS_fad;
          }
        #endif
        for (int i=0;i<nnode*dofpn;i++)
        {
          for (int j=0;j<nnode*dofpn;j++)
          {
            R_tension_ANS_fad(i,j)=Res_tension_ANS_fad(i).dx(j);
          }
        }
      #endif
      Res_tension_ANS.Clear();
      R_tension_ANS.Clear();
    #endif

    //assemble internal stiffness matrix / R = d/(dd) Res in thesis Meier
    if (stiffmatrix != NULL)
    {
      //assemble parts from tension
      #ifndef ANS_BEAM3EB
      R_tension = NTildex;
      R_tension.Scale(tension);
      R_tension.Update(1.0 / pow(rxrx,1.5),NxTrxrxTNx,1.0);
      R_tension.Scale(ym * crosssec_ * wgt);
      #else
        #ifndef CONSISTENTANSBEAM3EB
          //attention: in epsilon_ANS and lin_epsilon_ANS the corresponding jacobi factors are allready considered,
          //all the other jacobi factors due to differentiation and integration cancel out!!!
          for (int i=0;i<nnode*dofpn;i++)
          {
            for (int j=0;j<nnode*dofpn;j++)
            {
              R_tension_ANS(i,j)+=NxTrx(i)*lin_epsilon_ANS(j)/pow(rxrx,0.5);
            }
          }
          R_tension_ANS.Update(-epsilon_ANS / pow(rxrx,1.5),NxTrxrxTNx,1.0);
          R_tension_ANS.Update(epsilon_ANS / pow(rxrx,0.5),NTildex,1.0);
          R_tension_ANS.Scale(ym * crosssec_ * wgt);
        #else
          //since CONSISTENTANSBEAM3EB can so far only be calculated via FAD, R_tension_ANS has to be replaced by R_tension_ANS_fad
          for (int i=0;i<nnode*dofpn;i++)
          {
            for (int j=0;j<nnode*dofpn;j++)
            {
              R_tension_ANS(i,j)=R_tension_ANS_fad(i,j).val();
            }
          }
        #endif
      #endif

      //assemble parts from bending
      R_bending = NTildex;
      R_bending.Scale(2.0 * pow(rxrxx,2.0) / pow(rxrx,3.0));
      R_bending.Update(-rxxrxx/pow(rxrx,2.0),NTildex,1.0);
      R_bending.Update(-rxrxx/pow(rxrx,2.0),NTilde,1.0);
      R_bending.UpdateT(-rxrxx/pow(rxrx,2.0),NTilde,1.0);
      R_bending.Update(1.0/rxrx,NTildexx,1.0);
      R_bending.Update(-12.0 * pow(rxrxx,2.0)/pow(rxrx,4.0),NxTrxrxTNx,1.0);
      R_bending.Update(4.0 * rxrxx / pow(rxrx,3.0) , M1 , 1.0);
      R_bending.UpdateT(4.0 * rxrxx / pow(rxrx,3.0) , M1 , 1.0);
      R_bending.Update(4.0 * rxxrxx / pow(rxrx,3.0) , NxTrxrxTNx, 1.0);
      R_bending.Update(- 2.0 / pow(rxrx,2.0) , M2 , 1.0);
      R_bending.UpdateT(- 2.0 / pow(rxrx,2.0) , M2 , 1.0);
      R_bending.Update(- 1.0 / pow(rxrx,2.0) , M3 , 1.0);

      R_bending.Scale(ym * Izz_ * wgt / jacobi_);

      #ifndef INEXTENSIBLE
        //shifting values from fixed size matrix to epetra matrix *stiffmatrix
        for(int i = 0; i < dofpn*nnode; i++)
        {
          for(int j = 0; j < dofpn*nnode; j++)
          {
            #ifndef ANS_BEAM3EB
            (*stiffmatrix)(i,j) += R_tension(i,j);
            #else
            (*stiffmatrix)(i,j) += R_tension_ANS(i,j);
            #endif
            (*stiffmatrix)(i,j) += R_bending(i,j);
            #ifdef ORTHOPRESSURE
            (*stiffmatrix)(i,j) += R_orthopressure(i,j);
            #endif
          }
        } //for(int i = 0; i < dofpn*nnode; i++)
      #else
        //shifting values from fixed size matrix to epetra matrix *stiffmatrix
        int i1=0;
        int j1=0;
        for(int i = 0; i < 12; i++)
        {
          if(i<6)
            i1=i;
          else
            i1=i+1;

          for(int j = 0; j < 12; j++)
          {
            if(j<6)
              j1=j;
            else
              j1=j+1;

            (*stiffmatrix)(i1,j1) += INEXTENSIBLE*R_tension_ANS(i,j);
            (*stiffmatrix)(i1,j1) += R_bending(i,j);
          }
        } //for(int i = 0; i < dofpn*nnode; i++)
      #endif
    }//if (stiffmatrix != NULL)

    for (int i= 0; i<3; i++)
    {
      f1(i)=2*r_x(i)*pow(rxrxx,2.0)/pow(rxrx,3.0)-(r_x(i)*rxxrxx+r_xx(i)*rxrxx)/pow(rxrx,2.0);
      f2(i)=r_xx(i)/rxrx-r_x(i)*rxrxx/pow(rxrx,2.0);
      n1(i)=r_x(i)*tension;
    }
    //assemble internal force vector f_internal / Res in thesis Meier
    if (force != NULL)
    {
      for (int i=0;i<3;i++)
      {
        for (int j=0;j<nnode*NODALDOFS;j++)
        {
          Res_bending(j*3 + i)+= N_i_x(j)*f1(i) + N_i_xx(j)*f2(i);
          #ifndef ANS_BEAM3EB
          Res_tension(j*3 + i)+= N_i_x(j)*n1(i);
          #endif
        }
      }
      #ifdef ANS_BEAM3EB
        #ifndef CONSISTENTANSBEAM3EB
          //attention: in epsilon_ANS and lin_epsilon_ANS the corresponding jacobi factors are allready considered,
          //all the other jacobi factors due to differentiation and integration cancel out!!!
          Res_tension_ANS.Update(ym * crosssec_ * wgt*epsilon_ANS / pow(rxrx,0.5),NxTrx,0.0);
        #else
          //since CONSISTENTANSBEAM3EB can so far only be calculated via FAD, Rrd_tension_ANS has to be replaced by Rrd_tension_ANS_fad
          for (int i=0;i<nnode*dofpn;i++)
          {
            Res_tension_ANS(i)=Res_tension_ANS_fad(i).val();
          }
        #endif
      #endif

      Res_bending.Scale(ym * Izz_ * wgt / jacobi_);
      Res_tension.Scale(ym * crosssec_ * wgt);

      #ifndef INEXTENSIBLE
        //shifting values from fixed size vector to epetra vector *force
        for(int i = 0; i < dofpn*nnode; i++)
        {
            #ifndef ANS_BEAM3EB
            (*force)(i) += Res_tension(i);
            #else
            (*force)(i) += Res_tension_ANS(i);
            #endif
            (*force)(i) += Res_bending(i) ;
            #ifdef ORTHOPRESSURE
            (*force)(i) += Res_orthopressure(i).val();
            #endif
        }
      #else
        int i1=0;
        //shifting values from fixed size vector to epetra vector *force
        for(int i = 0; i < dofpn*nnode; i++)
        {
          if(i<6)
            i1=i;
          else
            i1=i+1;

          (*force)(i1) += INEXTENSIBLE*Res_tension_ANS(i);
          (*force)(i1) += Res_bending(i);
        }
      #endif
    } //if (force != NULL)

    #ifdef ANS_BEAM3EB

      double kappa_quad = (rxxrxx/rxrx-pow(rxrxx,2)/pow(rxrx,2))/pow(jacobi_,2);
  //    if(kappa_quad>0)
  //      std::cout << std::setprecision(16) << "kappa: " << sqrt(kappa_quad) << std::endl;

      if(kappa_quad<0)
        kappa_quad=-kappa_quad;

      Eint_+=0.5*wgt*jacobi_*ym * crosssec_ * pow(epsilon_ANS,2);
      Eint_axial_+=0.5*wgt*jacobi_*ym * crosssec_ * pow(epsilon_ANS,2);
      Eint_+=0.5*wgt*jacobi_*ym *Izz_ * kappa_quad;

      //determine maximal curvature
      if(sqrt(kappa_quad)>kappa_max_)
        kappa_max_=sqrt(kappa_quad);

      double epsilon_norm=sqrt(pow(epsilon_ANS,2));

      //determine maximal axial tension
      if(epsilon_norm>epsilon_max_)
        epsilon_max_=epsilon_norm;
    #endif
  } //for(int numgp=0; numgp < gausspoints.nquad; numgp++)

  std::vector<double > myvel(12,0.0);

  #ifndef INEXTENSIBLE
    for(int i=0;i<12;i++)
    {
      myvel[i]=vel[i];
    }
  #else
    for(int i=0;i<6;i++)
    {
      myvel[i]=vel[i];
      myvel[i+6]=vel[i+7];
    }
  #endif

  LINALG::Matrix<3,nnode*dofpn> N_mass;
  //Loop through all GP and calculate their contribution to the mass matrix
  for(int numgp=0; numgp < gausspoints.nquad; numgp++)
  {
    LINALG::Matrix<3,1> r_t(true);
    LINALG::Matrix<3,1> r(true);

    N_i.Clear();
    N_mass.Clear();
    NTilde.Clear();

    //Get location and weight of GP in parameter space
    const double xi = gausspoints.qxg[numgp][0];
    const double wgt = gausspoints.qwgt[numgp];

    #if (NODALDOFS == 2)
      //Get hermite derivatives N'xi and N''xi (jacobi_*2.0 is length of the element)
      DRT::UTILS::shape_function_hermite_1D(N_i,xi,jacobi_*2.0,distype);
      //end--------------------------------------------------------
    #elif (NODALDOFS == 3)
      dserror("massmatrix only implemented for the case NODALDOFS == 2!!!");
    #else
      dserror("Only the values NODALDOFS = 2 and NODALDOFS = 3 are valid!");
    #endif

    for (int i=0; i<3; ++i)
    {
      for (int j=0; j<nnode*NODALDOFS; ++j)
      {
        N_mass(i,i+3*j) += N_i(j);
      }
    }
    for (int i=0 ; i < 3 ; i++)
    {
      for (int j=0; j<nnode*NODALDOFS; j++)
      {
        r_t(i)+= N_i(j)*myvel[3*j + i];
      }
    }
    //calculate r' and r''
    for (int i=0 ; i < 3 ; i++)
    {
      for (int j=0; j<nnode*NODALDOFS; j++)
      {
        r(i,0)+= N_i(j)*disp_totlag[3*j + i];
      }
    }
    NTilde.MultiplyTN(N_mass,N_mass);

    if (massmatrix != NULL)
    {
    #ifndef INEXTENSIBLE
      for (int i=0; i<6*nnode; i++)
        for (int j=0; j<6*nnode; j++)
        {
          (*massmatrix)(i,j) += density*crosssec_*wgt*jacobi_*NTilde(i,j);
        }
    #else
      int i1=0;
      int j1=0;
      for (int i=0; i<6*nnode; i++)
      {
        if(i<6)
          i1=i;
        else
          i1=i+1;

        for (int j=0; j<6*nnode; j++)
        {
          if(j<6)
            j1=j;
          else
            j1=j+1;

          (*massmatrix)(i1,j1) += density*crosssec_*wgt*jacobi_*NTilde(i,j);
        }
      }
    #endif
    }//if (massmatrix != NULL)

    Ekin_+=0.5*wgt*jacobi_*density*crosssec_*pow(r_t.Norm2(),2.0);

    LINALG::Matrix<3,1> dL(true);
    LINALG::Matrix<3,3> S_r(true);
    LARGEROTATIONS::computespin(S_r,r);
    dL.Multiply(S_r,r_t);
    dL.Scale(density*crosssec_);
    for (int i=0;i<3;i++)
    {
      L_(i)+=wgt*jacobi_*dL(i);
      P_(i)+=wgt*jacobi_*density*crosssec_*r_t(i);
    }

  }//for(int numgp=0; numgp < gausspoints.nquad; numgp++)

  //Uncomment the following line to print the elment stiffness matrix to matlab format
  /*
  const std::string fname = "stiffmatrixele.mtl";
  std::cout<<"Printing stiffmatrixele to file"<<std::endl;
  LINALG::PrintSerialDenseMatrixInMatlabFormat(fname,*stiffmatrix);
  */
}
#endif

//Uncomment the next line if the implementation of the analytical stiffness matrix should be checked by Forward Automatic Differentiation (FAD)
//FADCheckStiffMatrix(disp, stiffmatrix, force);

// in statistical mechanics simulations, a deletion influenced by the values of the internal force vector might occur
if(params.get<std::string>("internalforces","no")=="yes" && force != NULL)
internalforces_ = *force;
/*the following function call applied statistical forces and damping matrix according to the fluctuation dissipation theorem;
* it is dedicated to the application of beam3 elements in the frame of statistical mechanics problems; for these problems a
* special vector has to be passed to the element packed in the params parameter list; in case that the control routine calling
* the element does not attach this special vector to params the following method is just doing nothing, which means that for
* any ordinary problem of structural mechanics it may be ignored*/
// Get if normal dynamics problem or statmech problem

const Teuchos::ParameterList& sdyn = DRT::Problem::Instance()->StructuralDynamicParams();
if(DRT::INPUT::IntegralValue<INPAR::STR::DynamicType>(sdyn,"DYNAMICTYP")==INPAR::STR::dyna_statmech)
  {
    #ifdef INEXTENSIBLE
      dserror("INEXTENSIBLE formulation not possible for statmech so far. Adapt vector vel -> myvel like above!");
    #endif
    CalcBrownian<nnode,3,6,4>(params,vel,disp,stiffmatrix,force);
  }

  return;

} // DRT::ELEMENTS::Beam3eb::eb_nlnstiffmass.

/*-----------------------------------------------------------------------------------------------------------*
 | Evaluate PTC damping (public)                                                              mukherjee 12/13|
 *----------------------------------------------------------------------------------------------------------*/
template<int nnode>
void DRT::ELEMENTS::Beam3eb::EvaluatePTC(Teuchos::ParameterList& params,
                                      Epetra_SerialDenseMatrix& elemat1)
{
  if(nnode > 2)
    dserror("PTC implemented for 2-noded elements only");
  for (int node=0; node<nnode; node++)
  {
    LINALG::Matrix<3,1> t0(true);
    LINALG::Matrix<3,1> t(true);
    for (int i=0;i<3;i++)
    {
      t0(i)=t0_(i,node);
      t(i)=t_(i,node);
    }
    t0.Scale(1.0/t0.Norm2());
    t.Scale(1.0/t.Norm2());

    double tTt0 = 0.0;
    for (int i=0;i<3;i++)
    {
      tTt0 += t0(i)*t(i);
    }

    #ifdef BEAM3EBROTPTC
      //variant1: PTC for tangential degrees of freedom; the Lobatto integration weight is 0.5 for 2-noded elements
      for(int k=0; k<3; k++)
      {
        elemat1(node*6+3+k,node*6+3+k) += tTt0*params.get<double>("crotptc",0.0)*0.5*jacobi_;
      }

      for (int k=0; k<3; k++)
      {
        for (int l=0; l<3; l++)
        {
          elemat1(node*6+3+k,node*6+3+l) += params.get<double>("crotptc",0.0)*0.5*jacobi_*t(k)*t0(l);
        }
      }
    #else
      //variant2: PTC for tangential degrees of freedom; the Lobatto integration weight is 0.5 for 2-noded elements
      for(int k=0; k<3; k++)
      {
        elemat1(node*6+3+k,node*6+3+k) += tTt0*params.get<double>("crotptc",0.0)*0.5*jacobi_;
      }
    #endif

    //PTC for translational degrees of freedom; the Lobatto integration weight is 0.5 for 2-noded elements
    for(int k=0; k<3; k++)
    {
      elemat1(node*6+k,node*6+k) += params.get<double>("ctransptc",0.0)*0.5*jacobi_;
    }
  }

  return;
} //DRT::ELEMENTS::Beam3r::EvaluatePTC

/*------------------------------------------------------------------------------------------------------------*
 | lump mass matrix            (private)                                                   meier 05/12|
 *------------------------------------------------------------------------------------------------------------*/
void DRT::ELEMENTS::Beam3eb::lumpmass(Epetra_SerialDenseMatrix* emass)
{
  std::cout << "\n\nWarning: Massmatrix not implemented yet!";
}

/*-----------------------------------------------------------------------------------------------------------*
 | computes damping coefficients per lengthand stores them in a matrix in the following order: damping of    |
 | translation parallel to filament axis, damping of translation orthogonal to filament axis, damping of     |
 | rotation around filament axis                                             (private)       Mukherjee  10/13|
 *----------------------------------------------------------------------------------------------------------*/
inline void DRT::ELEMENTS::Beam3eb::MyDampingConstants(Teuchos::ParameterList& params,LINALG::Matrix<3,1>& gamma)
{
  //translational damping coefficients according to Howard, p. 107, table 6.2;
  gamma(0) = 2*PI*params.get<double>("ETA",0.0);
  gamma(1) = 4*PI*params.get<double>("ETA",0.0);

  /*damping coefficient of rigid straight rod spinning around its own axis according to Howard, p. 107, table 6.2;
   *as this coefficient is very small for thin rods it is increased artificially by a factor for numerical convencience*/
//  double rsquare = std::pow((4*Iyy_/PI),0.5);
//  double artificial = 4000;//50;  20000//50 not bad for standard Actin3D_10.dat files; for 40 elements also 1 seems to work really well; for large networks 4000 seems good (artificial contribution then still just ~0.1 % of nodal moments)
//  gamma(2) = 4*PI*params.get<double>("ETA",0.0)*rsquare*artificial;

  //in case of an isotropic friction model the same damping coefficients are applied parallel to the polymer axis as perpendicular to it
  if(DRT::INPUT::get<INPAR::STATMECH::FrictionModel>(params,"FRICTION_MODEL") == INPAR::STATMECH::frictionmodel_isotropicconsistent || DRT::INPUT::get<INPAR::STATMECH::FrictionModel>(params,"FRICTION_MODEL") == INPAR::STATMECH::frictionmodel_isotropiclumped)
    gamma(0) = gamma(1);

}

/*-----------------------------------------------------------------------------------------------------------*
 |computes the number of different random numbers required in each time step for generation of stochastic    |
 |forces;                                                                    (public)       Mukherjee   10/13|
 *----------------------------------------------------------------------------------------------------------*/
int DRT::ELEMENTS::Beam3eb::HowManyRandomNumbersINeed()
{
  //get Gauss points and weights for evaluation of damping matrix
  DRT::UTILS::IntegrationPoints1D gausspoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::mygaussruleeb);
  /*at each Gauss point one needs as many random numbers as randomly excited degrees of freedom, i.e. three
   *random numbers for the translational degrees of freedom and one random number for the rotation around the element axis*/
  #ifdef CONSTSTOCHFORCE
    return (3);
  #else
    return (4*gausspoints.nquad);
  #endif
}

/*-----------------------------------------------------------------------------------------------------------*
 |computes velocity of background fluid and gradient of that velocity at a certain evaluation point in       |
 |the physical space                                                         (private)      Mukherjee   10/13|
 *----------------------------------------------------------------------------------------------------------*/
template<int ndim> //number of dimensions of embedding space
void DRT::ELEMENTS::Beam3eb::MyBackgroundVelocity(Teuchos::ParameterList& params,  //!<parameter list
                                                const LINALG::Matrix<ndim,1>& evaluationpoint,  //!<point at which background velocity and its gradient has to be computed
                                                LINALG::Matrix<ndim,1>& velbackground,  //!< velocity of background fluid
                                                LINALG::Matrix<ndim,ndim>& velbackgroundgrad) //!<gradient of velocity of background fluid
{
  /*note: this function is not yet a general one, but always assumes a shear flow, where the velocity of the
   * background fluid is always directed in direction params.get<int>("DBCDISPDIR",0) and orthogonal to z-axis.
   * In 3D the velocity increases linearly in z and equals zero for z = 0.
   * In 2D the velocity increases linearly in y and equals zero for y = 0. */

  //velocity at upper boundary of domain
  double uppervel = 0.0;

  //default values for background velocity and its gradient
  velbackground.PutScalar(0);
  velbackgroundgrad.PutScalar(0);

  double time = params.get<double>("total time",0.0);
  double starttime = params.get<double>("STARTTIMEACT",0.0);
  double dt = params.get<double>("delta time");
  double shearamplitude = params.get<double> ("SHEARAMPLITUDE", 0.0);
  int curvenumber = params.get<int> ("CURVENUMBER", -1)-1;
  int dbcdispdir = params.get<int> ("DBCDISPDIR", -1)-1;

  Teuchos::RCP<std::vector<double> > defvalues = Teuchos::rcp(new std::vector<double>(3,0.0));
  Teuchos::RCP<std::vector<double> > periodlength = params.get("PERIODLENGTH", defvalues);
  INPAR::STATMECH::DBCType dbctype = params.get<INPAR::STATMECH::DBCType>("DBCTYPE", INPAR::STATMECH::dbctype_std);
  bool shearflow = false;
  if(dbctype==INPAR::STATMECH::dbctype_shearfixed ||
     dbctype==INPAR::STATMECH::dbctype_shearfixeddel ||
     dbctype==INPAR::STATMECH::dbctype_sheartrans ||
     dbctype==INPAR::STATMECH::dbctype_affineshear||
     dbctype==INPAR::STATMECH::dbctype_affinesheardel)
    shearflow = true;

  //oscillations start only at params.get<double>("STARTTIMEACT",0.0)
  if(periodlength->at(0) > 0.0)
    if(shearflow && time>starttime && fabs(time-starttime)>dt/1e4 && curvenumber >=  0 && dbcdispdir >= 0 )
    {
      uppervel = shearamplitude * (DRT::Problem::Instance()->Curve(curvenumber).FctDer(time,1))[1];

      //compute background velocity
      velbackground(dbcdispdir) = (evaluationpoint(ndim-1) / periodlength->at(ndim-1)) * uppervel;

      //compute gradient of background velocity
      velbackgroundgrad(dbcdispdir,ndim-1) = uppervel / periodlength->at(ndim-1);
    }
}


/*-----------------------------------------------------------------------------------------------------------*
 | computes translational damping forces and stiffness (private)                            Mukherjee   10/13|
 *----------------------------------------------------------------------------------------------------------*/
template<int nnode, int ndim, int dof> //number of nodes, number of dimensions of embedding space, number of degrees of freedom per node
inline void DRT::ELEMENTS::Beam3eb::MyTranslationalDamping(Teuchos::ParameterList& params,  //!<parameter list
                                                  const std::vector<double>& vel,  //!< vector containing first order time derivative of nodal positions and nodal tangents of an element
                                                  const std::vector<double>& disp, //!< vector containing change in nodal positions and nodal tangents of an element w.r.t. inital config.
                                                  Epetra_SerialDenseMatrix* stiffmatrix,  //!< element stiffness matrix
                                                  Epetra_SerialDenseVector* force)//!< element internal force vector
{

  //get time step size
  double dt = params.get<double>("delta time",0.0);

  //velocity and gradient of background velocity field
  LINALG::Matrix<ndim,1> velbackground;
  LINALG::Matrix<ndim,ndim> velbackgroundgrad;

  //evaluation point in physical space corresponding to a certain Gauss point in parameter space
  LINALG::Matrix<ndim,1> evaluationpoint;

  //damping coefficients for translational and rotational degrees of freedom
  LINALG::Matrix<ndim,1> gamma(true);
  MyDampingConstants(params,gamma);

  //Get DiscretizationType of beam element
  const DRT::Element::DiscretizationType distype = Shape();

  // get Gauss points and weights
  DRT::UTILS::IntegrationPoints1D gausspoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::mygaussruleeb);

  //matrix to store hermite shape functions and their derivatives evaluated at a certain Gauss point
  LINALG::Matrix<1,nnode*NODALDOFS> N_i;
  LINALG::Matrix<1,nnode*NODALDOFS> N_i_x;


  for(int gp=0; gp < gausspoints.nquad; gp++)
  {
    //Get hermite shape functions at Gauss points in parametric space (jacobi_*2.0 is length of the element)
    DRT::UTILS::shape_function_hermite_1D(N_i,gausspoints.qxg[gp][0],jacobi_*2.0,distype);

    //Get derivatives of hermite shape functions at Gauss points in parametric space (jacobi_*2.0 is length of the element)
    DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,gausspoints.qxg[gp][0],jacobi_*2.0,distype);

    //compute point in physical space corresponding to Gauss point
    evaluationpoint.PutScalar(0);

    //update displacement vector /d in thesis Meier d = [ r1 t1 r2 t2]

    //matrix for current positions and tangents
    std::vector<double> disp_totlag(nnode*dof, 0.0);
    for (int node = 0 ; node < nnode ; node++)
    {
      for (int dof_count = 0 ; dof_count < dof ; dof_count++)
      {
        if(dof_count < 3)
        {
          //position of nodes
          disp_totlag[node*dof + dof_count] = (Nodes()[node]->X()[dof_count] + disp[node*dof + dof_count]);
        }
        else if(dof_count<6)
        {
          //tangent at nodes
          disp_totlag[node*dof + dof_count] = (Tref_[node](dof_count-3) + disp[node*dof + dof_count]);
        }
        else if(dof_count>=6)
        {
          #if NODALDOFS ==3
          //curvatures at nodes
          disp_totlag[node*dof + dof_count] = (Kref_[node](dof_count-6) + disp[node*dof + dof_count]);
          #endif
        }
      }
    } //for (int node = 0 ; node < nnode ; node++)

    //loop over all line nodes
    for(int i=0; i<nnode*NODALDOFS; i++)
    {
      //loop over all dimensions
      for(int j=0; j<ndim; j++)
         {
           evaluationpoint(j) += N_i(i)* disp_totlag[3*i+j];
         }
    }


    //compute velocity and gradient of background flow field at evaluation point
    MyBackgroundVelocity<3>(params,evaluationpoint,velbackground,velbackgroundgrad);


    //compute tangent vector t_{\par} at current Gauss point
    LINALG::Matrix<ndim,1> r_x(true);
    for(int i=0; i<nnode*NODALDOFS; i++)
    {
      for(int k=0; k<ndim; k++)
      {
        r_x(k) += N_i_x(i)* disp_totlag[3*i+k] / jacobi_;
      }
    }


    //compute velocity vector at this Gauss point
    LINALG::Matrix<ndim,1> velgp(true);
    for(int i=0; i<nnode*NODALDOFS; i++)
    {
      for(int l=0; l<ndim; l++)
      {
        velgp(l) += N_i(i)*vel[3*i+l];
      }
    }

    /* currently we are neglecting the contribution from the gradient of background velocity
     * i.e. dv/dx. Please uncomment this part if the gradient needs to be taken in account
      */
    //compute matrix product (t_{\par} \otimes t_{\par}) \cdot velbackgroundgrad
    LINALG::Matrix<ndim,ndim> tpartparvelbackgroundgrad(true);
    for(int i=0; i<ndim; i++)
      for(int j=0; j<ndim; j++)
        for(int k=0; k<ndim; k++)
          tpartparvelbackgroundgrad(i,j) += r_x(i)*r_x(k)*velbackgroundgrad(k,j);


    //loop over all line nodes. Number shape functions for beam3eb is 4 in contrast to 2, in case of beam3r elements.
    // Therefore max(i && j)= 4 i.e. 2*nnode
    for(int i=0; i<2*nnode; i++)
    {
      //loop over rows of matrix t_{\par} \otimes t_{\par}
      for(int k=0; k<ndim; k++)
      {
        //loop over columns of matrix t_{\par} \otimes t_{\par}
        for(int l=0; l<ndim; l++)
        {
          if(force != NULL)
            (*force)(i*3+k)+= N_i(i)*jacobi_*gausspoints.qwgt[gp]*( (k==l)*gamma(1) + (gamma(0) - gamma(1))*r_x(k)*r_x(l) ) *(velgp(l)- velbackground(l));
          if(stiffmatrix != NULL)
            //loop over all column nodes
            for (int j=0; j<2*nnode; j++)
            {
              (*stiffmatrix)(i*3+k,j*3+l) += gausspoints.qwgt[gp]*N_i(i)*N_i(j)*jacobi_*(  (k==l)*gamma(1) + (gamma(0) - gamma(1))*r_x(k)*r_x(l) ) / dt;
              /* currently we are neglecting the contribution from the gradient of background velocity
               * i.e. dv/dx. Please uncomment this part if the gradient needs to be taken in account */
              (*stiffmatrix)(i*3+k,j*3+l) -= gausspoints.qwgt[gp]*N_i(i)*N_i(j)*jacobi_*( velbackgroundgrad(k,l)*gamma(1) + (gamma(0) - gamma(1))*tpartparvelbackgroundgrad(k,l) ) ;
              (*stiffmatrix)(i*3+k,j*3+k) += gausspoints.qwgt[gp]*N_i(i)*N_i_x(j)* (gamma(0) - gamma(1))*r_x(l)*(velgp(l) - velbackground(l));
              (*stiffmatrix)(i*3+k,j*3+l) += gausspoints.qwgt[gp]*N_i(i)*N_i_x(j)* (gamma(0) - gamma(1))*r_x(k)*(velgp(l) - velbackground(l));
            }
        }
      }
    }
  }
  return;
}//DRT::ELEMENTS::Beam3eb::MyTranslationalDamping(.)

/*-----------------------------------------------------------------------------------------------------------*
 | computes stochastic forces and resulting stiffness (public)                              mukherjee   10/13|
 *----------------------------------------------------------------------------------------------------------*/
template<int nnode, int ndim, int dof, int randompergauss> //number of nodes, number of dimensions of embedding space, number of degrees of freedom per node, number of random numbers required per Gauss point
inline void DRT::ELEMENTS::Beam3eb::MyStochasticForces(Teuchos::ParameterList& params,  //!<parameter list
                                              const std::vector<double>& vel,  //!< element velocity vector
                                              const std::vector<double>& disp, //!<element disp vector
                                              Epetra_SerialDenseMatrix* stiffmatrix,  //!< element stiffness matrix
                                              Epetra_SerialDenseVector* force)//!< element internal force vector
{
  //damping coefficients for three translational and one rotatinal degree of freedom
  LINALG::Matrix<3,1> gamma(true);
  MyDampingConstants(params,gamma);

  //Get DiscretizationType of beam element
  const DRT::Element::DiscretizationType distype = Shape();

  //get Gauss points and weights for evaluation of damping matrix
  DRT::UTILS::IntegrationPoints1D gausspoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::mygaussruleeb);


  //matrix to store hermite shape functions and their derivatives evaluated at a certain Gauss point
  LINALG::Matrix<1,nnode*NODALDOFS> N_i;
  LINALG::Matrix<1,nnode*NODALDOFS> N_i_x;

  /*get pointer at Epetra multivector in parameter list linking to random numbers for stochastic forces with zero mean
   * and standard deviation (2*kT / dt)^0.5; note carefully: a space between the two subsequal ">" signs is mandatory
   * for the C++ parser in order to avoid confusion with ">>" for streams*/
  Teuchos::RCP<Epetra_MultiVector> randomnumbers = params.get<  Teuchos::RCP<Epetra_MultiVector> >("RandomNumbers",Teuchos::null);

  for(int gp=0; gp < gausspoints.nquad; gp++)
  {
    //Get hermite shape functions at Gauss points in parametric space (jacobi_*2.0 is length of the element)
    DRT::UTILS::shape_function_hermite_1D(N_i,gausspoints.qxg[gp][0],jacobi_*2.0,distype);

    //Get derivatives of hermite shape functions at Gauss points in parametric space (jacobi_*2.0 is length of the element)
    DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,gausspoints.qxg[gp][0],jacobi_*2.0,distype);

    //matrix for current positions and tangents
    std::vector<double> disp_totlag(nnode*dof, 0.0);
    for (int node = 0 ; node < nnode ; node++)
    {
      for (int dof_count = 0 ; dof_count < dof ; dof_count++)
      {
        if(dof_count < 3)
        {
          //position of nodes
          disp_totlag[node*dof + dof_count] = (Nodes()[node]->X()[dof_count] + disp[node*dof + dof_count]);
        }
        else if(dof_count<6)
        {
          //tangent at nodes
          disp_totlag[node*dof + dof_count] = (Tref_[node](dof_count-3) + disp[node*dof + dof_count]);
        }
        else if(dof_count>=6)
        {
          #if NODALDOFS ==3
          //curvatures at nodes
          disp_totlag[node*dof + dof_count] = (Kref_[node](dof_count-6) + disp[node*dof + dof_count]);
          #endif
        }
      }
    } //for (int node = 0 ; node < nnode ; node++)


    //compute tangent vector t_{\par} at current Gauss point
    LINALG::Matrix<ndim,1> r_x(true);
    for(int i=0; i<nnode*NODALDOFS; i++)
    {
      for(int k=0; k<ndim; k++)
      {
        r_x(k) += N_i_x(i)* disp_totlag[3*i+k] / jacobi_;
      }
    }

    //loop over all line nodes. Number shape functions for beam3eb is 4 in contrast to 2, in case of beam3r elements.
    // Therefore max(i && j)= 4 i.e. 2*nnode
    for(int i=0; i<2*nnode; i++)
    {
      //loop dimensions with respect to lines
      for(int k=0; k<ndim; k++)
      {
        //loop dimensions with respect to columns
        for(int l=0; l<ndim; l++)
        {
          if(force != NULL)
          {
            #ifndef CONSTSTOCHFORCE
              (*force)(i*3+k) -= N_i(i)*(sqrt(gamma(1))*(k==l) + (sqrt(gamma(0)) - sqrt(gamma(1)))*r_x(k)*r_x(l))*(*randomnumbers)[gp*randompergauss+l][LID()]*sqrt(jacobi_*gausspoints.qwgt[gp]);
            #else
              (*force)(i*3+k) -= N_i(i)*(sqrt(gamma(1))*(k==l) + (sqrt(gamma(0)) - sqrt(gamma(1)))*r_x(k)*r_x(l))*(*randomnumbers)[l][LID()]*sqrt(jacobi_*gausspoints.qwgt[gp]);
            #endif
          }

          if(stiffmatrix != NULL)
          {
            //loop over all column nodes
            for (int j=0; j<2*nnode; j++)
            {
              #ifndef CONSTSTOCHFORCE
                (*stiffmatrix)(i*3+k,j*3+k) -= N_i(i)*N_i_x(j)*r_x(l)*(*randomnumbers)[gp*randompergauss+l][LID()]*sqrt(gausspoints.qwgt[gp]/ jacobi_)*(sqrt(gamma(0)) - sqrt(gamma(1)));
                (*stiffmatrix)(i*3+k,j*3+l) -= N_i(i)*N_i_x(j)*r_x(k)*(*randomnumbers)[gp*randompergauss+l][LID()]*sqrt(gausspoints.qwgt[gp]/ jacobi_)*(sqrt(gamma(0)) - sqrt(gamma(1)));
              #else
                            (*stiffmatrix)(i*3+k,j*3+k) -= N_i(i)*N_i_x(j)*r_x(l)*(*randomnumbers)[l][LID()]*sqrt(gausspoints.qwgt[gp]/ jacobi_)*(sqrt(gamma(0)) - sqrt(gamma(1)));
                (*stiffmatrix)(i*3+k,j*3+l) -= N_i(i)*N_i_x(j)*r_x(k)*(*randomnumbers)[l][LID()]*sqrt(gausspoints.qwgt[gp]/ jacobi_)*(sqrt(gamma(0)) - sqrt(gamma(1)));
              #endif
            }
          }
        }
      }
    }
  }

  return;
}//DRT::ELEMENTS::Beam3eb::MyStochasticForces(.)


/*-----------------------------------------------------------------------------------------------------------*
 | Assemble stochastic and viscous forces and respective stiffness according to fluctuation dissipation      |
 | theorem                                                                           (public) Mukherjee 10/13|
 *----------------------------------------------------------------------------------------------------------*/
template<int nnode, int ndim, int dof, int randompergauss> //number of nodes, number of dimensions of embedding space, number of degrees of freedom per node, number of random numbers required per Gauss point
inline void DRT::ELEMENTS::Beam3eb::CalcBrownian(Teuchos::ParameterList& params,
                                              const std::vector<double>&       vel,  //!< element velocity vector
                                              const std::vector<double>&       disp, //!< element displacement vector
                                              Epetra_SerialDenseMatrix* stiffmatrix,  //!< element stiffness matrix
                                              Epetra_SerialDenseVector* force)        //!< element internal force vector
{
  //if no random numbers for generation of stochastic forces are passed to the element no Brownian dynamics calculations are conducted
  if( params.get<  Teuchos::RCP<Epetra_MultiVector> >("RandomNumbers",Teuchos::null) == Teuchos::null)
    return;

  //Evaluation of force vectors and stiffness matrices

  //add stiffness and forces due to translational damping effects
  MyTranslationalDamping<nnode,ndim,dof>(params,vel,disp,stiffmatrix,force);

  //add stochastic forces and (if required) resulting stiffness
  MyStochasticForces<nnode,ndim,dof,randompergauss>(params,vel,disp,stiffmatrix,force);

return;

}//DRT::ELEMENTS::Beam3eb::CalcBrownian(.)


/*-----------------------------------------------------------------------------------------------------------*
 | shifts nodes so that proper evaluation is possible even in case of periodic boundary conditions; if two   |
 | nodes within one element are separated by a periodic boundary, one of them is shifted such that the final |
 | distance in R^3 is the same as the initial distance in the periodic space; the shift affects computation  |
 | on element level within that very iteration step, only (no change in global variables performed)          |                                 |
 |                                                                                   (public) Mukherjee 10/13|
 *----------------------------------------------------------------------------------------------------------*/
template<int nnode, int ndim> //number of nodes, number of dimensions
inline void DRT::ELEMENTS::Beam3eb::NodeShift(Teuchos::ParameterList& params,  //!<parameter list
                                              std::vector<double>& disp) //!<element disp vector
{
  /*get number of degrees of freedom per node; note: the following function assumes the same number of degrees
   *of freedom for each element node*/
  int numdof = NumDofPerNode(*(Nodes()[0]));

  double time = params.get<double>("total time",0.0);
  double starttime = params.get<double>("STARTTIMEACT",0.0);
  double dt = params.get<double>("delta time");
  double shearamplitude = params.get<double> ("SHEARAMPLITUDE", 0.0);
  int curvenumber = params.get<int> ("CURVENUMBER", -1)-1;
  int dbcdispdir = params.get<int> ("DBCDISPDIR", -1)-1;

  Teuchos::RCP<std::vector<double> > defvalues = Teuchos::rcp(new std::vector<double>(3,0.0));
  Teuchos::RCP<std::vector<double> > periodlength = params.get("PERIODLENGTH", defvalues);
  INPAR::STATMECH::DBCType dbctype = params.get<INPAR::STATMECH::DBCType>("DBCTYPE", INPAR::STATMECH::dbctype_std);
  bool shearflow = false;
  if(dbctype==INPAR::STATMECH::dbctype_shearfixed || dbctype==INPAR::STATMECH::dbctype_sheartrans || dbctype==INPAR::STATMECH::dbctype_affineshear)
    shearflow = true;

  /*only if periodic boundary conditions are in use, i.e. params.get<double>("PeriodLength",0.0) > 0.0, this
   * method has to change the displacement variables*/
  if(periodlength->at(0) > 0.0)
    //loop through all nodes except for the first node which remains fixed as reference node
    for(int i=1;i<nnode;i++)
    {
      for(int dof= ndim - 1; dof > -1; dof--)
      {
        /*if the distance in some coordinate direction between some node and the first node becomes smaller by adding or subtracting
         * the period length, the respective node has obviously been shifted due to periodic boundary conditions and should be shifted
         * back for evaluation of element matrices and vectors; this way of detecting shifted nodes works as long as the element length
         * is smaller than half the periodic length*/
        if( fabs( (Nodes()[i]->X()[dof]+disp[numdof*i+dof]) + periodlength->at(dof) - (Nodes()[0]->X()[dof]+disp[numdof*0+dof]) ) < fabs( (Nodes()[i]->X()[dof]+disp[numdof*i+dof]) - (Nodes()[0]->X()[dof]+disp[numdof*0+dof]) ) )
        {
          disp[numdof*i+dof] += periodlength->at(dof);

          /*the upper domain surface orthogonal to the z-direction may be subject to shear Dirichlet boundary condition; the lower surface
           *may be fixed by DBC. To avoid problmes when nodes exit the domain through the upper z-surface and reenter through the lower
           *z-surface, the shear has to be substracted from nodal coordinates in that case */
          if(shearflow && dof == 2 && curvenumber >=  0 && time>starttime && fabs(time-starttime)>dt/1e4)
            disp[numdof*i+dbcdispdir] += shearamplitude*DRT::Problem::Instance()->Curve(curvenumber).f(time);
        }

        if( fabs( (Nodes()[i]->X()[dof]+disp[numdof*i+dof]) - periodlength->at(dof) - (Nodes()[0]->X()[dof]+disp[numdof*0+dof]) ) < fabs( (Nodes()[i]->X()[dof]+disp[numdof*i+dof]) - (Nodes()[0]->X()[dof]+disp[numdof*0+dof]) ) )
        {
          disp[numdof*i+dof] -= periodlength->at(dof);

          /*the upper domain surface orthogonal to the z-direction may be subject to shear Dirichlet boundary condition; the lower surface
           *may be fixed by DBC. To avoid problmes when nodes exit the domain through the lower z-surface and reenter through the upper
           *z-surface, the shear has to be added to nodal coordinates in that case */
          if(shearflow && dof == 2 && curvenumber >=  0 && time>starttime && fabs(time-starttime)>dt/1e4 )
            disp[numdof*i+dbcdispdir] -= shearamplitude*DRT::Problem::Instance()->Curve(curvenumber).f(time);
        }
      }
    }

return;

}//DRT::ELEMENTS::Beam3eb::NodeShift

/*----------------------------------------------------------------------------------------------------------*
 | Get position vector at xi for given nodal displacements                                       meier 02/14|
 *----------------------------------------------------------------------------------------------------------*/
LINALG::Matrix<3,1> DRT::ELEMENTS::Beam3eb::GetPos(double& xi, LINALG::Matrix<12,1>& disp_totlag) const
{
  LINALG::Matrix<3,1> r(true);
  LINALG::Matrix<4,1> N_i(true);

  const DRT::Element::DiscretizationType distype = Shape();
  DRT::UTILS::shape_function_hermite_1D(N_i,xi,jacobi_*2.0,distype);

  for (int n=0;n<4;n++)
  {
    for (int i=0;i<3;i++)
    {
      r(i)+=N_i(n)*disp_totlag(3*n+i);
    }
  }

  return (r);
}

//***************************************************************************************
//Methods for FAD Check
//***************************************************************************************

void DRT::ELEMENTS::Beam3eb::FADCheckStiffMatrix(std::vector<double>& disp,
                                                 Epetra_SerialDenseMatrix* stiffmatrix,
                                                 Epetra_SerialDenseVector* force)
{
#if NODALDOFS == 3
    dserror("FADCheck are not implemented for the case NODALDOFS = 3!!!");
#endif
  #ifdef SIMPLECALC
  {
    //see also so_nstet_nodalstrain.cpp, so_nstet.H, autodiff.cpp and autodiff.H
    //FAD calculated stiff matrix for validation purposes
    Epetra_SerialDenseMatrix stiffmatrix_check;
    LINALG::TMatrix<FAD,12,1> force_check;

    //reshape stiffmatrix_check
    stiffmatrix_check.Shape(12,12);

    //dimensions of freedom per node
    const int dofpn = 6;

    //number of nodes fixed for these element
    const int nnode = 2;

    //matrix for current positions and tangents
    std::vector<FAD> disp_totlag(nnode*dofpn,0.0);

    //abbreviated matrices for clearness
    LINALG::TMatrix<FAD,dofpn*nnode,dofpn*nnode> NTilde;
    LINALG::TMatrix<FAD,dofpn*nnode,dofpn*nnode> NTilde_x;
    LINALG::TMatrix<FAD,dofpn*nnode,dofpn*nnode> NTilde_xx;
    LINALG::TMatrix<FAD,dofpn*nnode,dofpn*nnode> NTilde_aux;

    //matrices helping to assemble above
    LINALG::TMatrix<FAD,3,nnode*dofpn> N_x;
    LINALG::TMatrix<FAD,3,nnode*dofpn> N_xx;

    //Matrices for N_i,xi and N_i,xixi. 2*nnode due to hermite shapefunctions
    LINALG::TMatrix<FAD,1,2*nnode> N_i_x;
    LINALG::TMatrix<FAD,1,2*nnode> N_i_xx;

    //stiffness due to tension and bending
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> R_tension;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> R_bending;

    //internal force due to tension and bending
    LINALG::TMatrix<FAD,nnode*dofpn,1> Res_tension;
    LINALG::TMatrix<FAD,nnode*dofpn,1> Res_bending;

    //algebraic operations
    LINALG::TMatrix<FAD,nnode*dofpn,1> NTilded;
    LINALG::TMatrix<FAD,nnode*dofpn,1> NTilde_xd;
    LINALG::TMatrix<FAD,nnode*dofpn,1> NTilde_xxd;
    LINALG::TMatrix<FAD,nnode*dofpn,1> NTilde_auxd;

    LINALG::TMatrix<FAD,1,nnode*dofpn> dTNTilde_x;
    LINALG::TMatrix<FAD,1,nnode*dofpn> dTNTilde_xx;
    LINALG::TMatrix<FAD,1,nnode*dofpn> dTNTilde_aux;

    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_xddTNTilde_x;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_xddTNTilde_aux;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_auxddTNTilde_x;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_xxddTNTilde_x;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_xddTNTilde_xx;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_auxddTNTilde_aux;

    //first of all we get the material law
    Teuchos::RCP<const MAT::Material> currmat = Material();
    double ym = 0;
    //Uncomment the next line for the dynamic case: so far only the static case is implemented
    //double density = 0;

    //assignment of material parameters; only St.Venant material is accepted for this beam
    switch(currmat->MaterialType())
    {
      case INPAR::MAT::m_stvenant:// only linear elastic material supported
      {
        const MAT::StVenantKirchhoff* actmat = static_cast<const MAT::StVenantKirchhoff*>(currmat.get());
        ym = actmat->Youngs();
        //Uncomment the next line for the dynamic case: so far only the static case is implemented
        //density = actmat->Density();
      }
      break;
      default:
      dserror("unknown or improper type of material law");
      break;
    }

    //Get integrationpoints for exact integration
    DRT::UTILS::IntegrationPoints1D gausspoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::mygaussruleeb);

    //Get DiscretizationType of beam element
    const DRT::Element::DiscretizationType distype = Shape();

    //update displacement vector /d in thesis Meier d = [ r1 t1 r2 t2]
    for (int node = 0 ; node < nnode ; node++)
    {
      for (int dof = 0 ; dof < dofpn ; dof++)
      {
        if(dof < 3)
        {
          //position of nodes
          disp_totlag[node*dofpn + dof] = Nodes()[node]->X()[dof] + disp[node*dofpn + dof];
          disp_totlag[node*dofpn + dof].diff(node*dofpn + dof,nnode*dofpn);
        }
        else if(dof>=3)
        {
          //tangent at nodes
          disp_totlag[node*dofpn + dof] = Tref_[node](dof-3) + disp[node*dofpn + dof];
          disp_totlag[node*dofpn + dof].diff(node*dofpn + dof,nnode*dofpn);
        }
      }
    } //for (int node = 0 ; node < nnode ; node++)

    //Loop through all GP and calculate their contribution to the internal forcevector and stiffnessmatrix
    for(int numgp=0; numgp < gausspoints.nquad; numgp++)
    {
      //all matrices and scalars are set to zero again!!!
      //factors for stiffness assembly
      FAD dTNTilded  = 0.0;
      FAD dTNTilde_xd = 0.0;
      FAD dTNTilde_xxd = 0.0;

      //initialize all matrices
      NTilde.Clear();
      NTilde_x.Clear();
      NTilde_xx.Clear();
      NTilde_aux.Clear();

      N_x.Clear();
      N_xx.Clear();

      R_tension.Clear();
      R_bending.Clear();

      Res_tension.Clear();
      Res_bending.Clear();

      N_i_x.Clear();
      N_i_xx.Clear();

      NTilded.Clear();
      NTilde_xd.Clear();
      NTilde_xxd.Clear();
      NTilde_auxd.Clear();

      dTNTilde_x.Clear();
      dTNTilde_xx.Clear();
      dTNTilde_aux.Clear();

      NTilde_xddTNTilde_x.Clear();
      NTilde_xddTNTilde_aux.Clear();
      NTilde_auxddTNTilde_x.Clear();
      NTilde_xxddTNTilde_x.Clear();
      NTilde_xddTNTilde_xx.Clear();
      NTilde_auxddTNTilde_aux.Clear();

      //Get location and weight of GP in parameter space
      const double xi = gausspoints.qxg[numgp][0];
      const double wgt = gausspoints.qwgt[numgp];

      //Get hermite derivatives N'xi and N''xi (jacobi_*2.0 is length of the element)
      DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,xi,jacobi_*2.0,distype);
      DRT::UTILS::shape_function_hermite_1D_deriv2(N_i_xx,xi,jacobi_*2.0,distype);

      //assemble test and trial functions
      for (int r=0; r<3; ++r)
      {
        for (int d=0; d<4; ++d)
        {
          //include jacobi factor due to coordinate transformation from local in global system
          N_x(r,r+3*d) = N_i_x(d)/jacobi_;
          N_xx(r,r+3*d) = N_i_xx(d)/pow(jacobi_,2.0);
        } //for (int d=0; d<4; ++d)
      } //for (int r=0; r<3; ++r)

      //create matrices to help assemble the stiffness matrix and internal force vector:: NTilde_x = N'^T * N'; NTilde_xx = N''^T * N''; NTilde = N'^T * N''
      NTilde_x.MultiplyTN(N_x,N_x);
      NTilde_xx.MultiplyTN(N_xx,N_xx);
      NTilde.MultiplyTN(N_x,N_xx);

      //NTilde_aux = N + N^T
      NTilde_aux = NTilde;
      NTilde_aux.UpdateT(1.0, NTilde,1.0);

      //calculate factors
      //row
      for (int i=0 ; i < dofpn*nnode ; i++)
      {
        //column
        for (int j=0 ; j < dofpn*nnode ; j++)
        {
          NTilded(i)     += NTilde(i,j)*disp_totlag[j];
          NTilde_xd(i)    += NTilde_x(i,j)*disp_totlag[j];
          NTilde_xxd(i)    += NTilde_xx(i,j)*disp_totlag[j];
          NTilde_auxd(i) += NTilde_aux(i,j)*disp_totlag[j];

          dTNTilde_x(i)    += disp_totlag[j]*NTilde_x(j,i);
          dTNTilde_xx(i)    += disp_totlag[j]*NTilde_xx(j,i);
          dTNTilde_aux(i) += disp_totlag[j]*NTilde_aux(j,i);
        } //for (int j=0 ; j < dofpn*nnode ; j++)

        dTNTilded  += disp_totlag[i] * NTilded(i);
        dTNTilde_xd += disp_totlag[i] * NTilde_xd(i);
        dTNTilde_xxd += disp_totlag[i] * NTilde_xxd(i);
      } //for (int i=0 ; i < dofpn*nnode ; i++)

      //calculate factors
      //row
      for (int i=0 ; i < dofpn*nnode ; i++)
      {
        //column
        for (int j=0 ; j < dofpn*nnode ; j++)
        {
          NTilde_xddTNTilde_x(j,i)       = NTilde_xd(j)*dTNTilde_x(i);
          NTilde_xddTNTilde_aux(j,i)    = NTilde_xd(j)*dTNTilde_aux(i);
          NTilde_auxddTNTilde_x(j,i)    = NTilde_auxd(j)*dTNTilde_x(i);
          NTilde_xxddTNTilde_x(j,i)       = NTilde_xxd(j)*dTNTilde_x(i);
          NTilde_xddTNTilde_xx(j,i)       = NTilde_xd(j)*dTNTilde_xx(i);
          NTilde_auxddTNTilde_aux(j,i) = NTilde_auxd(j)*dTNTilde_aux(i);
        } //for (int j=0 ; j < dofpn*nnode ; j++)
      } //for (int i=0 ; i < dofpn*nnode ; i++)

      //assemble internal stiffness matrix / R = d/(dd) Res in thesis Meier
      //assemble parts from tension
      R_tension = NTilde_x;
      R_tension.Scale(1.0 - 1.0/pow(dTNTilde_xd,0.5));
      R_tension.Update(1.0 / pow(dTNTilde_xd,1.5),NTilde_xddTNTilde_x,1.0);

      R_tension.Scale(ym * crosssec_ * jacobi_ * wgt);

      //assemble parts from bending
      R_bending.Update(-dTNTilde_xxd,NTilde_x,1.0);
      R_bending.Update(1.0,NTilde_xx,1.0);
      R_bending.Update(- 2.0 , NTilde_xddTNTilde_xx , 1.0);

      R_bending.Scale(ym * Izz_ * wgt * jacobi_);

      //assemble internal force vector f_internal / Res in thesis Meier
      //assemble parts from tension
      Res_tension = NTilde_xd;
      Res_tension.Scale(1.0 - 1.0 /pow(dTNTilde_xd,0.5));

      Res_tension.Scale(ym * crosssec_ * jacobi_ * wgt);

      //assemble parts from bending
      Res_bending.Update(-dTNTilde_xxd,NTilde_xd,1.0);
      Res_bending.Update(1.0 ,NTilde_xxd,1.0);
      Res_bending.Scale(ym * Izz_ * jacobi_ * wgt);

      //shifting values from fixed size vector to epetra vector *force
      for(int i = 0; i < dofpn*nnode; i++)
      {
          force_check(i,0) += Res_tension(i) ;
          force_check(i,0) += Res_bending(i) ;
      }

      //shifting values from fixed size matrix to epetra matrix *stiffmatrix
      for(int i = 0; i < dofpn*nnode; i++)
      {
        for(int j = 0; j < dofpn*nnode; j++)
        {
          stiffmatrix_check(i,j) = force_check(i,0).dx(j) ;
        }
      } //for(int i = 0; i < dofpn*nnode; i++)
    } //for(int numgp=0; numgp < gausspoints.nquad; numgp++)

    Epetra_SerialDenseMatrix stiff_relerr;
    stiff_relerr.Shape(12,12);

    for(int line=0; line<12; line++)
    {
      for(int col=0; col<12; col++)
      {
        stiff_relerr(line,col)= fabs(  (    pow(stiffmatrix_check(line,col),2) - pow( (*stiffmatrix)(line,col),2 )    )/(  (  (*stiffmatrix)(line,col) + stiffmatrix_check(line,col)  ) * (*stiffmatrix)(line,col)  )  );

        //suppressing small entries whose effect is only confusing and NaN entires (which arise due to zero entries)
        //if ( fabs( stiff_relerr(line,col) ) < h_rel*50 || isnan( stiff_relerr(line,col)) || elemat1(line,col) == 0) //isnan = is not a number
        if ( fabs( stiff_relerr(line,col) ) < 1.0e-15 || isnan( stiff_relerr(line,col)) || (*stiffmatrix)(line,col) == 0) //isnan = is not a number
          stiff_relerr(line,col) = 0;
      } //for(int col=0; col<3*nnode; col++)
    } //for(int line=0; line<3*nnode; line++)


    std::cout<<"\n\n original stiffness matrix: "<< std::endl;
    for(int i = 0; i< 12; i++)
    {
      for(int j = 0; j< 12; j++)
      {
        std::cout << std::setw(9) << std::setprecision(4) << std::scientific << (*stiffmatrix)(i,j);
      }
      std::cout<<std::endl;
    }

    std::cout<<"\n\n analytical stiffness matrix: "<< std::endl;
    for(int i = 0; i< 12; i++)
    {
      for(int j = 0; j< 12; j++)
      {
        std::cout << std::setw(9) << std::setprecision(4) << std::scientific << (stiffmatrix_check)(i,j);
      }
      std::cout<<std::endl;
    }

    //std::cout<<"\n\n FAD stiffness matrix"<< stiffmatrix_check;
    std::cout<<"\n\n rel error of stiffness matrix"<< stiff_relerr;
    std::cout<<"Force_FAD: "<< force_check << std::endl;
    std::cout<<"Force_original: "<< *force << std::endl;
  }
  #else
  {
    //see also so_nstet_nodalstrain.cpp, so_nstet.H, autodiff.cpp and autodiff.H
    //FAD calculated stiff matrix for validation purposes
    Epetra_SerialDenseMatrix stiffmatrix_check;
    LINALG::TMatrix<FAD,12,1> force_check;

    //reshape stiffmatrix_check
    stiffmatrix_check.Shape(12,12);

    //dimensions of freedom per node
    const int dofpn = 6;

    //number of nodes fixed for these element
    const int nnode = 2;

    //matrix for current positions and tangents
    std::vector<FAD> disp_totlag(nnode*dofpn,0.0);

    //abbreviated matrices for clearness
    LINALG::TMatrix<FAD,dofpn*nnode,dofpn*nnode> NTilde;
    LINALG::TMatrix<FAD,dofpn*nnode,dofpn*nnode> NTilde_x;
    LINALG::TMatrix<FAD,dofpn*nnode,dofpn*nnode> NTilde_xx;
    LINALG::TMatrix<FAD,dofpn*nnode,dofpn*nnode> NTilde_aux;

    //matrices helping to assemble above
    LINALG::TMatrix<FAD,3,nnode*dofpn> N_x;
    LINALG::TMatrix<FAD,3,nnode*dofpn> N_xx;

    //Matrices for N_i,xi and N_i,xixi. 2*nnode due to hermite shapefunctions
    LINALG::TMatrix<FAD,1,2*nnode> N_i_x;
    LINALG::TMatrix<FAD,1,2*nnode> N_i_xx;

    //stiffness due to tension and bending
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> R_tension;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> R_bending;

    //internal force due to tension and bending
    LINALG::TMatrix<FAD,nnode*dofpn,1> Res_tension;
    LINALG::TMatrix<FAD,nnode*dofpn,1> Res_bending;

    //algebraic operations
    LINALG::TMatrix<FAD,nnode*dofpn,1> NTilded;
    LINALG::TMatrix<FAD,nnode*dofpn,1> NTilde_xd;
    LINALG::TMatrix<FAD,nnode*dofpn,1> NTilde_xxd;
    LINALG::TMatrix<FAD,nnode*dofpn,1> NTilde_auxd;

    LINALG::TMatrix<FAD,1,nnode*dofpn> dTNTilde_x;
    LINALG::TMatrix<FAD,1,nnode*dofpn> dTNTilde_xx;
    LINALG::TMatrix<FAD,1,nnode*dofpn> dTNTilde_aux;

    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_xddTNTilde_x;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_xddTNTilde_aux;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_auxddTNTilde_x;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_xxddTNTilde_x;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_xddTNTilde_xx;
    LINALG::TMatrix<FAD,nnode*dofpn,nnode*dofpn> NTilde_auxddTNTilde_aux;

    //first of all we get the material law
    Teuchos::RCP<const MAT::Material> currmat = Material();
    double ym = 0;
    //Uncomment the next line for the dynamic case: so far only the static case is implemented
    //double density = 0;

    //assignment of material parameters; only St.Venant material is accepted for this beam
    switch(currmat->MaterialType())
    {
      case INPAR::MAT::m_stvenant:// only linear elastic material supported
      {
        const MAT::StVenantKirchhoff* actmat = static_cast<const MAT::StVenantKirchhoff*>(currmat.get());
        ym = actmat->Youngs();
        //Uncomment the next line for the dynamic case: so far only the static case is implemented
        //density = actmat->Density();
      }
      break;
      default:
      dserror("unknown or improper type of material law");
      break;
    }

    //Get integrationpoints for exact integration
    DRT::UTILS::IntegrationPoints1D gausspoints = DRT::UTILS::IntegrationPoints1D(DRT::UTILS::mygaussruleeb);

    //Get DiscretizationType of beam element
    const DRT::Element::DiscretizationType distype = Shape();

    //update displacement vector /d in thesis Meier d = [ r1 t1 r2 t2]
    for (int node = 0 ; node < nnode ; node++)
    {
      for (int dof = 0 ; dof < dofpn ; dof++)
      {
        if(dof < 3)
        {
          //position of nodes
          disp_totlag[node*dofpn + dof] = Nodes()[node]->X()[dof] + disp[node*dofpn + dof];
          disp_totlag[node*dofpn + dof].diff(node*dofpn + dof,nnode*dofpn);
        }
        else if(dof>=3)
        {
          //tangent at nodes
          disp_totlag[node*dofpn + dof] = Tref_[node](dof-3) + disp[node*dofpn + dof];
          disp_totlag[node*dofpn + dof].diff(node*dofpn + dof,nnode*dofpn);
        }
      }
    } //for (int node = 0 ; node < nnode ; node++)

    //Loop through all GP and calculate their contribution to the internal forcevector and stiffnessmatrix
    for(int numgp=0; numgp < gausspoints.nquad; numgp++)
    {
      //all matrices and scalars are set to zero again!!!
      //factors for stiffness assembly
      FAD dTNTilded  = 0.0;
      FAD dTNTilde_xd = 0.0;
      FAD dTNTilde_xxd = 0.0;

      //initialize all matrices
      NTilde.Clear();
      NTilde_x.Clear();
      NTilde_xx.Clear();
      NTilde_aux.Clear();

      N_x.Clear();
      N_xx.Clear();

      R_tension.Clear();
      R_bending.Clear();

      Res_tension.Clear();
      Res_bending.Clear();

      N_i_x.Clear();
      N_i_xx.Clear();

      NTilded.Clear();
      NTilde_xd.Clear();
      NTilde_xxd.Clear();
      NTilde_auxd.Clear();

      dTNTilde_x.Clear();
      dTNTilde_xx.Clear();
      dTNTilde_aux.Clear();

      NTilde_xddTNTilde_x.Clear();
      NTilde_xddTNTilde_aux.Clear();
      NTilde_auxddTNTilde_x.Clear();
      NTilde_xxddTNTilde_x.Clear();
      NTilde_xddTNTilde_xx.Clear();
      NTilde_auxddTNTilde_aux.Clear();

      //Get location and weight of GP in parameter space
      const double xi = gausspoints.qxg[numgp][0];
      const double wgt = gausspoints.qwgt[numgp];

      //Get hermite derivatives N'xi and N''xi (jacobi_*2.0 is length of the element)
      DRT::UTILS::shape_function_hermite_1D_deriv1(N_i_x,xi,jacobi_*2.0,distype);
      DRT::UTILS::shape_function_hermite_1D_deriv2(N_i_xx,xi,jacobi_*2.0,distype);

      //assemble test and trial functions
      for (int r=0; r<3; ++r)
      {
        for (int d=0; d<4; ++d)
        {
          //include jacobi factor due to coordinate transformation from local in global system
          N_x(r,r+3*d) = N_i_x(d)/jacobi_;
          N_xx(r,r+3*d) = N_i_xx(d)/pow(jacobi_,2.0);
        } //for (int d=0; d<4; ++d)
      } //for (int r=0; r<3; ++r)

      //create matrices to help assemble the stiffness matrix and internal force vector:: NTilde_x = N'^T * N'; NTilde_xx = N''^T * N''; NTilde = N'^T * N''
      NTilde_x.MultiplyTN(N_x,N_x);
      NTilde_xx.MultiplyTN(N_xx,N_xx);
      NTilde.MultiplyTN(N_x,N_xx);

      //NTilde_aux = N + N^T
      NTilde_aux = NTilde;
      NTilde_aux.UpdateT(1.0, NTilde,1.0);

      //calculate factors
      //row
      for (int i=0 ; i < dofpn*nnode ; i++)
      {
        //column
        for (int j=0 ; j < dofpn*nnode ; j++)
        {
          NTilded(i)     += NTilde(i,j)*disp_totlag[j];
          NTilde_xd(i)    += NTilde_x(i,j)*disp_totlag[j];
          NTilde_xxd(i)    += NTilde_xx(i,j)*disp_totlag[j];
          NTilde_auxd(i) += NTilde_aux(i,j)*disp_totlag[j];

          dTNTilde_x(i)    += disp_totlag[j]*NTilde_x(j,i);
          dTNTilde_xx(i)    += disp_totlag[j]*NTilde_xx(j,i);
          dTNTilde_aux(i) += disp_totlag[j]*NTilde_aux(j,i);
        } //for (int j=0 ; j < dofpn*nnode ; j++)

        dTNTilded  += disp_totlag[i] * NTilded(i);
        dTNTilde_xd += disp_totlag[i] * NTilde_xd(i);
        dTNTilde_xxd += disp_totlag[i] * NTilde_xxd(i);
      } //for (int i=0 ; i < dofpn*nnode ; i++)

      //calculate factors
      //row
      for (int i=0 ; i < dofpn*nnode ; i++)
      {
        //column
        for (int j=0 ; j < dofpn*nnode ; j++)
        {
          NTilde_xddTNTilde_x(j,i)       = NTilde_xd(j)*dTNTilde_x(i);
          NTilde_xddTNTilde_aux(j,i)    = NTilde_xd(j)*dTNTilde_aux(i);
          NTilde_auxddTNTilde_x(j,i)    = NTilde_auxd(j)*dTNTilde_x(i);
          NTilde_xxddTNTilde_x(j,i)       = NTilde_xxd(j)*dTNTilde_x(i);
          NTilde_xddTNTilde_xx(j,i)       = NTilde_xd(j)*dTNTilde_xx(i);
          NTilde_auxddTNTilde_aux(j,i) = NTilde_auxd(j)*dTNTilde_aux(i);
        } //for (int j=0 ; j < dofpn*nnode ; j++)
      } //for (int i=0 ; i < dofpn*nnode ; i++)

      //assemble internal stiffness matrix / R = d/(dd) Res in thesis Meier
      //assemble parts from tension
      R_tension = NTilde_x;
      R_tension.Scale(1.0 - 1.0/pow(dTNTilde_xd,0.5));
      R_tension.Update(1.0 / pow(dTNTilde_xd,1.5),NTilde_xddTNTilde_x,1.0);

      R_tension.Scale(ym * crosssec_ * jacobi_ * wgt);

      //assemble parts from bending
      R_bending = NTilde_x;
      R_bending.Scale(2.0 * pow(dTNTilded,2.0) / pow(dTNTilde_xd,3.0));
      R_bending.Update(-dTNTilde_xxd/pow(dTNTilde_xd,2.0),NTilde_x,1.0);
      R_bending.Update(-dTNTilded/pow(dTNTilde_xd,2.0),NTilde_aux,1.0);
      R_bending.Update(1.0/dTNTilde_xd,NTilde_xx,1.0);
      R_bending.Update(-12.0 * pow(dTNTilded,2.0)/pow(dTNTilde_xd,4.0),NTilde_xddTNTilde_x,1.0);
      R_bending.Update(4.0 * dTNTilded / pow(dTNTilde_xd,3.0) , NTilde_xddTNTilde_aux , 1.0);
      R_bending.Update(4.0 * dTNTilded / pow(dTNTilde_xd,3.0) , NTilde_auxddTNTilde_x , 1.0);
      R_bending.Update(4.0 * dTNTilde_xxd / pow(dTNTilde_xd,3.0) , NTilde_xddTNTilde_x , 1.0);
      R_bending.Update(- 2.0 / pow(dTNTilde_xd,2.0) , NTilde_xxddTNTilde_x , 1.0);
      R_bending.Update(- 2.0 / pow(dTNTilde_xd,2.0) , NTilde_xddTNTilde_xx , 1.0);
      R_bending.Update(- 1.0 / pow(dTNTilde_xd,2.0) , NTilde_auxddTNTilde_aux , 1.0);

      R_bending.Scale(ym * Izz_ * jacobi_ * wgt);

      //assemble internal force vector f_internal / Res in thesis Meier
      //assemble parts from tension
      Res_tension = NTilde_xd;
      Res_tension.Scale(1.0 - 1.0 /pow(dTNTilde_xd,0.5));

      Res_tension.Scale(ym * crosssec_ * jacobi_ * wgt);

      //assemble parts from bending
      Res_bending = NTilde_xd;
      Res_bending.Scale(2.0 * pow(dTNTilded,2.0)/pow(dTNTilde_xd,3.0));
      Res_bending.Update(-dTNTilde_xxd / pow(dTNTilde_xd,2.0),NTilde_xd,1.0);
      Res_bending.Update(-dTNTilded / pow(dTNTilde_xd,2.0),NTilde_auxd,1.0);
      Res_bending.Update(1.0 / dTNTilde_xd,NTilde_xxd,1.0);

      Res_bending.Scale(ym * Izz_ * jacobi_ * wgt);

      std::cout << "Resbending: " << Res_bending << std::endl;
      std::cout << "Restension: " << Res_tension << std::endl;

      //shifting values from fixed size vector to epetra vector *force
      for(int i = 0; i < dofpn*nnode; i++)
      {
          force_check(i,0) += Res_tension(i) ;
          force_check(i,0) += Res_bending(i) ;
      }

      //shifting values from fixed size matrix to epetra matrix *stiffmatrix
      for(int i = 0; i < dofpn*nnode; i++)
      {
        for(int j = 0; j < dofpn*nnode; j++)
        {
          stiffmatrix_check(i,j) = force_check(i,0).dx(j) ;
        }
      } //for(int i = 0; i < dofpn*nnode; i++)
    } //for(int numgp=0; numgp < gausspoints.nquad; numgp++)

    Epetra_SerialDenseMatrix stiff_relerr;
    stiff_relerr.Shape(12,12);

    for(int line=0; line<12; line++)
    {
      for(int col=0; col<12; col++)
      {
        stiff_relerr(line,col)= fabs(  (    pow(stiffmatrix_check(line,col),2) - pow( (*stiffmatrix)(line,col),2 )    )/(  (  (*stiffmatrix)(line,col) + stiffmatrix_check(line,col)  ) * (*stiffmatrix)(line,col)  )  );

        //suppressing small entries whose effect is only confusing and NaN entires (which arise due to zero entries)
        //if ( std::abs( stiff_relerr(line,col) ) < h_rel*50 || std::isnan( stiff_relerr(line,col)) || elemat1(line,col) == 0) //isnan = is not a number
        if ( std::abs( stiff_relerr(line,col) ) < 1.0e-15 || std::isnan( stiff_relerr(line,col)) || (*stiffmatrix)(line,col) == 0) //isnan = is not a number
          stiff_relerr(line,col) = 0;
      } //for(int col=0; col<3*nnode; col++)
    } //for(int line=0; line<3*nnode; line++)


    std::cout<<"\n\n original stiffness matrix: "<< std::endl;
    for(int i = 0; i< 12; i++)
    {
      for(int j = 0; j< 12; j++)
      {
        std::cout << std::setw(9) << std::setprecision(4) << std::scientific << (*stiffmatrix)(i,j);
      }
      std::cout<<std::endl;
    }

    std::cout<<"\n\n analytical stiffness matrix: "<< std::endl;
    for(int i = 0; i< 12; i++)
    {
      for(int j = 0; j< 12; j++)
      {
        std::cout << std::setw(9) << std::setprecision(4) << std::scientific << (stiffmatrix_check)(i,j);
      }
      std::cout<<std::endl;
    }

    std::cout<<"\n\n FAD stiffness matrix"<< stiffmatrix_check;
    std::cout<<"\n\n rel error of stiffness matrix"<< stiff_relerr;
    std::cout<<"Force: "<< force_check << std::endl;
  }
  #endif
}

void DRT::ELEMENTS::Beam3eb::FADCheckNeumann(Teuchos::ParameterList& params,
                                             DRT::Discretization& discretization,
                                             DRT::Condition& condition,
                                             std::vector<int>& lm,
                                             Epetra_SerialDenseVector& elevec1,
                                             Epetra_SerialDenseMatrix* elemat1)
{
#if NODALDOFS == 3
    dserror("FADChecks are not implemented for the case NODALDOFS = 3!!!");
#endif
  //FAD calculated stiff matrix for validation purposes
  Epetra_SerialDenseMatrix stiffmatrix_check;

  const int nnode=2;
  const int dofpn=6;

  LINALG::TMatrix<FAD,dofpn*nnode,1> force_check;

  //reshape stiffmatrix_check
  stiffmatrix_check.Shape((dofpn)*nnode,(dofpn)*nnode);

  for (int i=0; i<(dofpn)*nnode; i++)
  {
    for (int j=0; j<(dofpn)*nnode; j++)
    {
      stiffmatrix_check(i,j)=0;
    }
    force_check(i,0)=0;
  }

  //get element displacements
  Teuchos::RCP<const Epetra_Vector> disp = discretization.GetState("displacement new");
  if (disp==Teuchos::null) dserror("Cannot get state vector 'displacement new'");
  std::vector<double> mydisp(lm.size());
  DRT::UTILS::ExtractMyValues(*disp,mydisp,lm);

  //matrix for current positions and tangents
  std::vector<FAD> disp_totlag((dofpn)*nnode,0.0);

  for (int i=0; i<(dofpn)*nnode; i++)
  {
    disp_totlag[i]=mydisp[i];
    disp_totlag[i].diff(i,(dofpn)*nnode);
  }

  // find out whether we will use a time curve
  bool usetime = true;
  const double time = params.get("total time",-1.0);
  if (time<0.0) usetime = false;

  // find out whether we will use a time curve and get the factor
  const std::vector<int>* curve  = condition.Get<std::vector<int> >("curve");
  // amplitude of load curve at current time called
  std::vector<double> curvefac(6,1.0);

  for (int i=0; i<6; ++i)
  {
    int curvenum = -1;
    // number of the load curve related with a specific line Neumann condition called
    if (curve) curvenum = (*curve)[i];

    if (curvenum>=0 && usetime)
      curvefac[i] = DRT::Problem::Instance()->Curve(curvenum).f(time);
  }

  // get values and switches from the condition

  // onoff is related to the first 6 flags of a line Neumann condition in the input file;
  // value 1 for flag i says that condition is active for i-th degree of freedom
  const std::vector<int>* onoff = condition.Get<std::vector<int> >("onoff");
  // val is related to the 6 "val" fields after the onoff flags of the Neumann condition

  // in the input file; val gives the values of the force as a multiple of the prescribed load curve
  const std::vector<double>* val = condition.Get<std::vector<double> >("val");

  //find out which node is correct
  const std::vector< int > * nodeids = condition.Nodes();

  //if a point neumann condition needs to be linearized
  if(condition.Type() == DRT::Condition::PointNeumannEB)
  {
    //find out local node number --> this is done since the first element of a neumann point condition is used for this function
    //in this case we do not know whether it is the left or the right node.
    int insert = -1;

    if((*nodeids)[0] == Nodes()[0]->Id())
      insert = 0;
    else if((*nodeids)[0] == Nodes()[1]->Id())
      insert = 1;

    if (insert == -1)
      dserror("\nNode could not be found on nodemap!\n");

    //add forces to Res_external according to (5.56)
    for(int i = 0; i < 3 ; i++)
    {
      force_check(insert*(dofpn) + i) += (*onoff)[i]*(*val)[i]*curvefac[i];
    }

    //matrix for current tangent, moment at node and crossproduct
    LINALG::TMatrix<FAD,3,1> tangent;
    LINALG::TMatrix<FAD,3,1> crossproduct;
    LINALG::TMatrix<FAD,3,1> moment;
    LINALG::TMatrix<FAD,3,3> spinmatrix;

    //clear all matrices
    tangent.Clear();
    crossproduct.Clear();
    moment.Clear();
    spinmatrix.Clear();

    //assemble current tangent and moment at node
    for (int dof = 3 ; dof < 6 ; dof++)
    {
      //get current tangent at nodes
      tangent(dof-3) = Tref_[insert](dof-3) + disp_totlag[insert*(dofpn) + dof];
      moment(dof-3) = (*onoff)[dof]*(*val)[dof]*curvefac[dof];
    }

    FAD abs_tangent = 0.0;

    for (int i=0; i<3;  i++)
     {
      abs_tangent += std::pow(tangent(i,0),2);
     }
    abs_tangent=pow(abs_tangent,0.5);

    //computespin = S ( tangent ) using the spinmatrix in namespace largerotations
    LARGEROTATIONS::computespin<FAD>(spinmatrix,tangent);

    //matrixoperation crossproduct = r' x m and rxrxTNx = I/|r'|-r'r'T/|r'|^3
    for(int i=0; i<3; i++)
    {
      for(int j=0; j<3; j++)
      {
        crossproduct(i,0) += spinmatrix(i,j) * moment(j);
      }
    }

    //add moments to Res_external according to (5.56)
    for(int i = 3; i < 6 ; i++)
    {
      force_check(insert*(dofpn) + i) -= crossproduct(i-3,0) / pow(abs_tangent,2.0);
    }

    //assembly for stiffnessmatrix
    LINALG::TMatrix<FAD,3,3> crossxtangent;

    crossxtangent.Clear();

    //perform matrix operation
    for(int i=0; i<3; i++)
    {
      for(int j=0; j<3; j++)
      {
        crossxtangent(i,j) = crossproduct(i,0) * tangent(j);
      }
    }

    spinmatrix.Clear();

    //spinmatrix = S ( m )
    LARGEROTATIONS::computespin<FAD>(spinmatrix,moment);

    for(int i = 0; i < (dofpn)*nnode; i++)
    {
      for(int j = 0; j < (dofpn)*nnode; j++)
      {
        stiffmatrix_check(i,j) = -force_check(i).dx(j) ;
      }
    }
  }

  //if a line neumann condition needs to be linearized
  else if(condition.Type() == DRT::Condition::LineNeumann)
  {
  }

  Epetra_SerialDenseMatrix stiff_relerr;
  stiff_relerr.Shape((dofpn+1)*nnode,(dofpn+1)*nnode);

  for(int line=0; line<(dofpn)*nnode; line++)
  {
    for(int col=0; col<(dofpn)*nnode; col++)
    {
      stiff_relerr(line,col)= fabs((pow(stiffmatrix_check(line,col),2) - pow((*elemat1)(line,col),2))/(((*elemat1)(line,col) + stiffmatrix_check(line,col)) * (*elemat1)(line,col)));

      //suppressing small entries whose effect is only confusing and NaN entires (which arise due to zero entries)
      if ( std::abs( stiff_relerr(line,col) ) < 1.0e-10 || std::isnan( stiff_relerr(line,col)) || (*elemat1)(line,col) == 0) //isnan = is not a number
        stiff_relerr(line,col) = 0;
    } //for(int col=0; col<3*nnode; col++)
  } //for(int line=0; line<3*nnode; line++)

  Epetra_SerialDenseMatrix force_relerr;
  force_relerr.Shape((dofpn)*nnode,1);
  for (int line=0; line<(dofpn)*nnode; line++)
  {
    force_relerr(line,0)= fabs(pow(force_check(line,0).val(),2.0) - pow((elevec1)(line),2.0 ));
  }
  std::cout<<"\n\n Rel error stiffness matrix Neumann: "<< stiff_relerr << std::endl;

}

//***************************************************************************************
//End: Methods for FAD Check
//***************************************************************************************


//***************************************************************************************
//Methods for arbitrary precision calculation
//***************************************************************************************
#ifdef PRECISION

  void DRT::ELEMENTS::Beam3eb::eb_nlnstiffmassprec( LINALG::TMatrix<cl_F,12,1>* displocal,
                                                    LINALG::TMatrix<cl_F,12,12>* stifflocal,
                                                    LINALG::TMatrix<cl_F,12,1>* reslocal,
                                                    LINALG::TMatrix<cl_F,6,1>* xreflocal)
  {



#if NODALDOFS == 3
    dserror("High precision calculation is not implemented for the case NODALDOFS = 3!!!");
#endif

        std::vector<double> forcetest2(12);
        for (int i=0; i<12;i++)
        {
          forcetest2[i]=0.0;
        }

        //dimensions of freedom per node
        const int dofpn = 6;

        //number of nodes fixed for these element
        const int nnode = 2;

        //matrix for current positions and tangents
        std::vector<cl_F> disp_totlag(nnode*dofpn);

        LINALG::TMatrix<cl_F,3,1> r_;
        LINALG::TMatrix<cl_F,3,1> r_x;
        LINALG::TMatrix<cl_F,3,1> r_xx;

        LINALG::TMatrix<cl_F,3,1> f1;
        LINALG::TMatrix<cl_F,3,1> f2;
        LINALG::TMatrix<cl_F,3,1> n1;

        cl_F rxrxx;
        cl_F rxxrxx;
        cl_F rxrx;
        cl_F tension;

        LINALG::TMatrix<cl_F,dofpn*nnode,dofpn*nnode> NTilde;
        LINALG::TMatrix<cl_F,dofpn*nnode,dofpn*nnode> NTildex;
        LINALG::TMatrix<cl_F,dofpn*nnode,dofpn*nnode> NTildexx;

        LINALG::TMatrix<cl_F,dofpn*nnode,1> NxTrx;
        LINALG::TMatrix<cl_F,dofpn*nnode,1> NxTrxx;
        LINALG::TMatrix<cl_F,dofpn*nnode,1> NxxTrx;
        LINALG::TMatrix<cl_F,dofpn*nnode,1> NxxTrxx;

        LINALG::TMatrix<cl_F,dofpn*nnode,dofpn*nnode> M1;
        LINALG::TMatrix<cl_F,dofpn*nnode,dofpn*nnode> M2;
        LINALG::TMatrix<cl_F,dofpn*nnode,dofpn*nnode> M3;
        LINALG::TMatrix<cl_F,dofpn*nnode,dofpn*nnode> NxTrxrxTNx;

        //Matrices for N_i,xi and N_i,xixi. 2*nnode due to hermite shapefunctions
        LINALG::TMatrix<cl_F,1,2*nnode> N_i;
        LINALG::TMatrix<cl_F,1,2*nnode> N_i_x;
        LINALG::TMatrix<cl_F,1,2*nnode> N_i_xx;

        LINALG::TMatrix<cl_F,3,nnode*dofpn> N_x;
        LINALG::TMatrix<cl_F,3,nnode*dofpn> N_xx;

        //stiffness due to tension and bending
        LINALG::TMatrix<cl_F,nnode*dofpn,nnode*dofpn> R_tension;
        LINALG::TMatrix<cl_F,nnode*dofpn,nnode*dofpn> R_bending;

        //internal force due to tension and bending
        LINALG::TMatrix<cl_F,nnode*dofpn,1> Res_tension;
        LINALG::TMatrix<cl_F,nnode*dofpn,1> Res_bending;

        //clear disp_totlag vector before assembly
        disp_totlag.clear();

        //update displacement vector /d in thesis Meier d = [ r1 t1 r2 t2]
        for (int node = 0 ; node < nnode ; node++)
        {
          for (int dof = 0 ; dof < dofpn ; dof++)
          {
            if(dof < 3)
            {
              disp_totlag[node*dofpn + dof] = cl_float(0,float_format(40));
              disp_totlag[node*dofpn + dof] = (*xreflocal)(3*node + dof,0) + (*displocal)(node*dofpn + dof,0);
            }
            else if(dof>=3)
            {
              //tangent at nodes
              disp_totlag[node*dofpn + dof] = cl_float(0,float_format(40));
              disp_totlag[node*dofpn + dof] = Trefprec_(dof-3) + (*displocal)(node*dofpn + dof,0);
            }
          }
        } //for (int node = 0 ; node < nnode ; node++)

        //begin: gausspoints exact
        std::vector<cl_F> xivec(6);
        std::vector<cl_F> wgtvec(6);
        xivec[0]="-0.9324695142031520278123016_40";
        xivec[5]="0.9324695142031520278123016_40";
        xivec[1]="-0.6612093864662645136613996_40";
        xivec[4]="0.6612093864662645136613996_40";
        xivec[2]="-0.2386191860831969086305017_40";
        xivec[3]="0.2386191860831969086305017_40";
        wgtvec[0]="0.171324492379170345040296_40";
        wgtvec[5]="0.171324492379170345040296_40";
        wgtvec[1]="0.360761573048138607569834_40";
        wgtvec[4]="0.360761573048138607569834_40";
        wgtvec[2]="0.467913934572691047389870_40";
        wgtvec[3]="0.467913934572691047389870_40";
        //end: gausspoints exact

        //Loop through all GP and calculate their contribution to the internal forcevector and stiffnessmatrix
        for(int numgp=0; numgp < 6; numgp++)
        {
          //all matrices and scalars are set to zero again!!!
          //factors for stiffness assembly

          for (int i=0;i<3;i++)
          {
          r_(i)=cl_float(0,float_format(40));
          r_x(i)=cl_float(0,float_format(40));
          r_xx(i)=cl_float(0,float_format(40));
          f1(i)=cl_float(0,float_format(40));
          f2(i)=cl_float(0,float_format(40));
          n1(i)=cl_float(0,float_format(40));
            for (int j=0;j<12;j++)
            {
              N_x(i,j)=cl_float(0,float_format(40));
              N_xx(i,j)=cl_float(0,float_format(40));
            }
          }

          rxrxx=cl_float(0,float_format(40));
          rxxrxx=cl_float(0,float_format(40));
          rxrx=cl_float(0,float_format(40));
          tension=cl_float(0,float_format(40));

          for (int i=0;i<12;i++)
          {
            for (int j=0;j<12;j++)
            {
              NTilde(i,j)=cl_float(0,float_format(40));
              NTildex(i,j)=cl_float(0,float_format(40));
              NTildexx(i,j)=cl_float(0,float_format(40));
              M1(i,j)=cl_float(0,float_format(40));
              M2(i,j)=cl_float(0,float_format(40));
              M3(i,j)=cl_float(0,float_format(40));
              NxTrxrxTNx(i,j)=cl_float(0,float_format(40));
              R_tension(i,j)=cl_float(0,float_format(40));
              R_bending(i,j)=cl_float(0,float_format(40));
            }
            NxTrx(i)=cl_float(0,float_format(40));
            NxTrxx(i)=cl_float(0,float_format(40));
            NxxTrx(i)=cl_float(0,float_format(40));
            NxxTrxx(i)=cl_float(0,float_format(40));
            Res_tension(i)=cl_float(0,float_format(40));
            Res_bending(i)=cl_float(0,float_format(40));
          }

          for (int i=0;i<4;i++)
          {
            N_i(i)=cl_float(0,float_format(40));
            N_i_x(i)=cl_float(0,float_format(40));
            N_i_xx(i)=cl_float(0,float_format(40));
          }

          //Get location and weight of GP in parameter space
          cl_F xi=xivec[numgp];
          cl_F wgt=wgtvec[numgp];

          //Begin: shape fuction variante für arbitrary precision

           cl_F l = cl_float(2.0,float_format(40))*jacobiprec_;
           N_i_x(0)= cl_float(0.25,float_format(40))*(- cl_float(3.0,float_format(40)) + cl_float(3.0,float_format(40)) * cl_float(expt(xi,2.0),float_format(40)));
           N_i_x(1)= l / cl_float(8.0,float_format(40)) * (- cl_float(1.0,float_format(40)) - cl_float(2.0,float_format(40)) * xi + cl_float(3.0,float_format(40)) * cl_float(expt(xi,2.0),float_format(40)));
           N_i_x(2)= cl_float(0.25,float_format(40))*(cl_float(3.0,float_format(40)) - cl_float(3.0,float_format(40)) * cl_float(expt(xi,2.0),float_format(40)));
           N_i_x(3)= l / cl_float(8.0,float_format(40)) * (- cl_float(1.0,float_format(40)) + cl_float(2.0,float_format(40)) * xi + cl_float(3.0,float_format(40)) * cl_float(expt(xi,2.0),float_format(40)));

           N_i_xx(0)= cl_float(1.5,float_format(40)) * xi;
           N_i_xx(1)= l / cl_float(8.0,float_format(40)) * (- cl_float(2.0,float_format(40)) + cl_float(6.0,float_format(40)) * xi);
           N_i_xx(2)= - cl_float(1.5,float_format(40)) * xi;
           N_i_xx(3)= l / cl_float(8.0,float_format(40)) * (cl_float(2.0,float_format(40)) + cl_float(6.0,float_format(40)) * xi);

           //end: shape fuction variante für arbitrary precision


          //calculate r' and r''
          for (int i=0 ; i < 3 ; i++)
          {
            for (int j=0; j<4; j++)
            {
              r_x(i,0)+= N_i_x(j) * disp_totlag[3*j + i];
              r_xx(i,0)+= N_i_xx(j) * disp_totlag[3*j + i];
            }
          }

          for (int i= 0; i<3; i++)
          {
            rxrxx+=r_x(i)*r_xx(i);
            rxxrxx+=r_xx(i)*r_xx(i);
            rxrx+=r_x(i)*r_x(i);
          }

          tension = cl_float(1.0,float_format(40))/jacobiprec_ - cl_float(1.0,float_format(40))/sqrt(rxrx);

          for (int i=0; i<3; ++i)
          {
            for (int j=0; j<4; ++j)
            {
              N_x(i,i+3*j) += N_i_x(j);
              N_xx(i,i+3*j) += N_i_xx(j);
              NxTrx(i+3*j)+=N_i_x(j)*r_x(i);
              NxTrxx(i+3*j)+=N_i_x(j)*r_xx(i);
              NxxTrx(i+3*j)+=N_i_xx(j)*r_x(i);
              NxxTrxx(i+3*j)+=N_i_xx(j)*r_xx(i);
            }
          }

          NTilde.MultiplyTN(N_x,N_xx);
          NTildex.MultiplyTN(N_x,N_x);
          NTildexx.MultiplyTN(N_xx,N_xx);


          for (int i= 0; i<12; i++)
          {
            for (int j= 0; j<12; j++)
            {
              M1(i,j)+= NxTrx(i)*(NxxTrx(j)+NxTrxx(j));
              M2(i,j)+= NxxTrxx(i)*NxTrx(j);
              M3(i,j)+= (NxTrxx(i)+NxxTrx(i))*(NxTrxx(j)+NxxTrx(j));
              NxTrxrxTNx(i,j)+= NxTrx(i)*NxTrx(j);
            }
          }

          //assemble internal stiffness matrix / R = d/(dd) Res in thesis Meier
          if (stifflocal != NULL)
          {
            //assemble parts from tension
            R_tension = NTildex;
            R_tension.Scale(tension);
            R_tension.Update(cl_float(1.0,float_format(40)) / sqrt(expt(rxrx,3.0)),NxTrxrxTNx,cl_float(1.0,float_format(40)));

            R_tension.Scale(Eprec_ * crosssecprec_ * wgt);

            //assemble parts from bending
            R_bending = NTildex;
            R_bending.Scale(cl_float(cl_float(2.0,float_format(40)) * expt(rxrxx,2.0) / expt(rxrx,3.0),float_format(40)));
            R_bending.Update(-rxxrxx/expt(rxrx,2.0),NTildex,cl_float(1.0,float_format(40)));
            R_bending.Update(-rxrxx/expt(rxrx,2.0),NTilde,cl_float(1.0,float_format(40)));
            R_bending.UpdateT(-rxrxx/expt(rxrx,2.0),NTilde,cl_float(1.0,float_format(40)));
            R_bending.Update(cl_float(1.0,float_format(40))/rxrx,NTildexx,cl_float(1.0,float_format(40)));
            R_bending.Update(cl_float(-cl_float(12.0,float_format(40)) * expt(rxrxx,2.0)/expt(rxrx,4.0),float_format(40)),NxTrxrxTNx,cl_float(1.0,float_format(40)));
            R_bending.Update(cl_float(4.0,float_format(40)) * rxrxx / expt(rxrx,3.0) , M1 , cl_float(1.0,float_format(40)));
            R_bending.UpdateT(cl_float(4.0,float_format(40)) * rxrxx / expt(rxrx,3.0) , M1 , cl_float(1.0,float_format(40)));
            R_bending.Update(cl_float(4.0,float_format(40)) * rxxrxx / expt(rxrx,3.0) , NxTrxrxTNx, cl_float(1.0,float_format(40)));
            R_bending.Update(-cl_float(2.0,float_format(40)) / expt(rxrx,2.0) , M2 , cl_float(1.0,float_format(40)));
            R_bending.UpdateT(-cl_float(2.0,float_format(40)) / expt(rxrx,2.0) , M2 , cl_float(1.0,float_format(40)));
            R_bending.Update(-cl_float(1.0,float_format(40)) / expt(rxrx,2.0) , M3 , cl_float(1.0,float_format(40)));

            R_bending.Scale(Eprec_ * Izzprec_ * wgt / jacobiprec_);

            //shifting values from fixed size matrix to epetra matrix *stiffmatrix
            for(int i = 0; i < dofpn*nnode; i++)
            {
              for(int j = 0; j < dofpn*nnode; j++)
              {
                //cout << "Rbending: " << R_bending(i,j) << endl;
                (*stifflocal)(i,j) += R_tension(i,j);
                (*stifflocal)(i,j) += R_bending(i,j);
                //stifftest_(i,j)+= R_bending(i,j);
              }

            } //for(int i = 0; i < dofpn*nnode; i++)

          }//if (stiffmatrix != NULL)

          for (int i= 0; i<3; i++)
          {
            f1(i)=cl_float(2.0,float_format(40))*r_x(i)*expt(rxrxx,2.0)/expt(rxrx,3.0)-(r_x(i)*rxxrxx+r_xx(i)*rxrxx)/expt(rxrx,2.0);
            f2(i)=r_xx(i)/rxrx-r_x(i)*rxrxx/expt(rxrx,2.0);
            n1(i)=r_x(i)*tension;
          }


          //assemble internal force vector f_internal / Res in thesis Meier
          if (reslocal != NULL)
          {
            for (int i=0;i<3;i++)
            {
              for (int j=0;j<4;j++)
              {
                Res_bending(j*3 + i)+= N_i_x(j)*f1(i) + N_i_xx(j)*f2(i);
                Res_tension(j*3 + i)+= N_i_x(j)*n1(i);
              }
            }
            Res_bending.Scale(Eprec_ * Izzprec_ * wgt / jacobiprec_);
            Res_tension.Scale(Eprec_ * crosssecprec_ * wgt);

            //shifting values from fixed size vector to epetra vector *force
            for(int i = 0; i < dofpn*nnode; i++)
            {
                (*reslocal)(i) += Res_tension(i) ;
                (*reslocal)(i) += Res_bending(i) ;
                //restest_(i) += Res_bending(i);
            }
          } //if (force != NULL)

        } //for(int numgp=0; numgp < gausspoints.nquad; numgp++)

  /*    const std::string fname = "stiffmatrixele.mtl";
        cout<<"Printing stiffmatrixele to file"<<endl;
        LINALG::PrintSerialDenseMatrixInMatlabFormat(fname,*stiffmatrix);*/

    return;
  } // DRT::ELEMENTS::Beam3eb::eb_nlnstiffmass

  void DRT::ELEMENTS::Beam3eb::HighPrecissionCalc()
  {
#if NODALDOFS == 3
    dserror("High precision calculation is not implemented for the case NODALDOFS = 3!!!");
#endif
    //Uncomment the following line to avoid floating point underflow --> small numbers are then rounded to zero
    //cl_inhibit_floating_point_underflow = true;

    //Input Parameters
    const cl_F RESTOL = "1.0e-35_40";
    const cl_F DISPTOL = "1.0e-35_40";
    const cl_F TOLLINSOLV = "1.0e-50_40";
    const int numele = 32;
    const int NUMLOADSTEP = 250;
    cl_F balkenlaenge = "10.0_40";
    balkenradiusprec_ = "1.0_40";
    cl_F fext = "0.0_40";
    LINALG::TMatrix<cl_F,3,1> mextvec;
    mextvec(0)="0.0_40";
    mextvec(1)="0.0_40";
    mextvec(2)="0.0_40";
    //End: Input Parameters


    //Referenzgeometrieberechnung
    const int numnode = numele+1;
    cl_F elementlaenge = "0.0_40";
    elementlaenge = balkenlaenge / numele;
    jacobiprec_ = elementlaenge / cl_F("2.0_40");
    const cl_F PIPREC = "3.1415926535897932384626433832795028841971_40";
    crosssecprec_ = cl_float(expt(balkenradiusprec_,2.0) * PIPREC, float_format(40));
    Izzprec_ = cl_float(expt(balkenradiusprec_,4.0) * PIPREC / cl_F("4.0_40"),float_format(40));
    Eprec_= "1.0_40";
    cl_F mext = Izzprec_ * Eprec_ * cl_F("2.0_40") * PIPREC / balkenlaenge;
    mextvec(2)= mext;

    LINALG::TMatrix<cl_F,numnode*3,1> xrefglobal;

    for (int i=0;i<numnode;i++)
    {
      for (int j=0;j<3;j++)
      {
        xrefglobal(3*i+j,0)=cl_F("0.0_40");
        xrefglobal(3*i,0)=-balkenlaenge/2 + i*elementlaenge;
        Trefprec_(j,0)="0.0_40";
      }
    }
    Trefprec_(0,0)="1.0_40";
    //End: Referenzgeometrieberechnung


    //Globale Groeßen definieren
    cl_F resnorm="10.0_40";
    cl_F dispnorm="10.0_40";
    cl_F linsolverrornorm="10.0_40";
    LINALG::TMatrix<cl_F,numnode*6,numnode*6> stiffglobal;
    LINALG::TMatrix<cl_F,numnode*6,1> resglobal;
    LINALG::TMatrix<cl_F,numnode*6,1> dispglobal;
    LINALG::TMatrix<cl_F,numnode*6,1> deltadispglobal;
    LINALG::TMatrix<cl_F,numnode*6,1> fextglobal;

    for (int i=0;i<6*numnode;i++)
    {
      for (int j=0;j<6*numnode;j++)
      {
        stiffglobal(i,j)=cl_F("0.0_40");
      }
      resglobal(i,0)=cl_F("0.0_40");
      dispglobal(i,0)=cl_F("0.0_40");
      fextglobal(i,0)=cl_F("0.0_40");
    }
    //Ende: Globale Groeßen definieren


    //Loadsteps
    LINALG::TMatrix<cl_F,3,1>mextvecstep;
    cl_F fextstep="0.0_40";
    for (int i=0;i<3;i++)
    {
      mextvecstep(i)="0.0_40";
    }
    for (int lastschritt=0; lastschritt<NUMLOADSTEP; lastschritt++)
    {
      std::cout << "Lastschritt: " << lastschritt + 1 << std::endl;
      fextstep=fext*cl_float((lastschritt+1),float_format(40))/cl_float(NUMLOADSTEP,float_format(40));
      for (int j=0;j<3;j++)
      {
        mextvecstep(j)=mextvec(j)*cl_float((lastschritt+1),float_format(40))/cl_float(NUMLOADSTEP,float_format(40));
      }

      std::cout << "begin of Newton Iteration" << std::endl;
      int iter=0;
      resnorm="1.0_40";

      //Newton
      while (resnorm>"1.0e-50_40")
      {
        iter++;
        LINALG::TMatrix<cl_F,12,12> stifflocal;
        LINALG::TMatrix<cl_F,12,1> reslocal;
        LINALG::TMatrix<cl_F,12,1> displocal;
        LINALG::TMatrix<cl_F,6,1> xreflocal;

        //Normen nullen
        resnorm="0.0_40";
        dispnorm="0.0_40";
        linsolverrornorm="0.0_40";
        //end: Normen nullen

        for (int i=0;i<6*numnode;i++)
        {
          for (int j=0;j<6*numnode;j++)
          {
            stiffglobal(i,j)=cl_F("0.0_40");
          }
          resglobal(i,0)=cl_F("0.0_40");
        }

        //Evaluate all elements and assemble
        for (int ele=0;ele<numele;ele++)
        {

          for (int i=0;i<12;i++)
          {
            for (int j=0;j<12;j++)
            {
              stifflocal(i,j)=cl_F("0.0_40");
            }
            reslocal(i,0)=cl_F("0.0_40");
            displocal(i,0)=cl_F("0.0_40");
            xreflocal=cl_F("0.0_40");
          }

          for (int k=0;k<12;k++)
          {
            displocal(k,0)=dispglobal(ele*6 + k ,0);
          }

          for (int k=0;k<6;k++)
          {
            xreflocal(k,0)=xrefglobal(ele*3 + k ,0);
          }


          for (int i=0;i<12;i++)
          {
            for (int j=0;j<12;j++)
            {
              stifftest_(i,j) ="0.0_40";
            }
            restest_(i)="0.0_40";
          }

          eb_nlnstiffmassprec(&displocal, &stifflocal, &reslocal, &xreflocal);

          //Uncomment the following Code block to compare the high precision stiffness matrix with the standard precision stiffness matrix
          /*
          //Begin: Compare with old stiffness
          //Uncomment the following block to compare with the original stiffness calculation
          std::vector<double> testdisp(12);
          for (int i=0;i<12;i++)
          {
            testdisp[i]=double_approx(displocal(i,0));
          }
          for (int i=0;i<12;i++)
          {
            for (int j=0;j<12;j++)
            {
              elemat1(i,j) =0;
            }
            elevec1[i]=0;
          }
          eb_nlnstiffmass(params,testdisp,testdisp,&elemat1,NULL,&elevec1);
          //End: Compare with old stiffness
          cout << "resnew: " << endl;
          for (int i=0;i<12;i++)
          {
            cout << std::setprecision(15) << double_approx(restest_(i)) << endl;
          }

          cout << "resold: " << endl;
          for (int i=0;i<12;i++)
          {
            cout << std::setprecision(15) << elevec1[i] << endl;
          }

          cout << "stiffnew: " << endl;
          for (int i=0;i<12;i++)
          {
            for (int j=0;j<12;j++)
            {
              cout << std::setprecision(15) << double_approx(stifftest_(i,j)) << "  ";
            }
            cout << endl;
          }

          cout << "stiffold: " << endl;
          for (int i=0;i<12;i++)
          {
            for (int j=0;j<12;j++)
            {
              cout << std::setprecision(15) << elemat1(i,j) << "  ";
            }
            cout << endl;
          }

          LINALG::Matrix<12,12> stiff_error;
          LINALG::Matrix<12,1> res_error;
          for(int line=0; line<12; line++)
          {
            for(int col=0; col<12; col++)
            {
              if (stifftest_(line,col)<cl_F("1.0e-15_40"))
              {stiff_error(line,col)=fabs( double_approx(stifftest_(line,col)) - elemat1(line,col) );}
              else
              {stiff_error(line,col)= fabs( double_approx(stifftest_(line,col)) - elemat1(line,col) ) /  double_approx(stifftest_(line,col));}
              //{stiff_error(line,col)= cl_float(abs( ( expt(stifflocal(line,col),2.0) - expt(stiff_approx(line,col),2.0) )/ ( (stifflocal(line,col) + stiff_approx(line,col)) * stifflocal(line,col) )),float_format(40));}

              //suppressing small entries whose effect is only confusing and NaN entires (which arise due to zero entries)
              //if ( fabs( stiff_error(line,col) ) < 1.0e-15 ) //isnan = is not a number
              //stiff_error(line,col) = 0.0;
            } //for(int col=0; col<3*nnode; col++)
          } //for(int line=0; line<3*nnode; line++)

          for(int line=0; line<12; line++)
          {
              if (restest_(line)<cl_F("1.0e-15_40"))
              {res_error(line)=fabs( double_approx(restest_(line)) - elevec1(line) );}
              else
              {res_error(line)= fabs( double_approx(restest_(line)) - elevec1(line) ) /  double_approx(restest_(line));}
              //{stiff_error(line,col)= cl_float(abs( ( expt(stifflocal(line,col),2.0) - expt(stiff_approx(line,col),2.0) )/ ( (stifflocal(line,col) + stiff_approx(line,col)) * stifflocal(line,col) )),float_format(40));}

              //suppressing small entries whose effect is only confusing and NaN entires (which arise due to zero entries)
              if ( fabs( res_error(line) ) < 1.0e-15 ) //isnan = is not a number
              res_error(line) = 0.0;
          } //for(int line=0; line<3*nnode; line++)

          cout << "stifferror: " << endl;
          for (int i=0;i<12;i++)
          {
            for (int j=0;j<12;j++)
            {
              cout << std::setprecision(5) << std::setw(8) << stiff_error(i,j) << "  ";
            }
            cout << endl;
          }

          cout << "reserror: " << endl;
          for (int i=0;i<12;i++)
          {
            cout << std::setprecision(15) << res_error(i) << endl;
          }End: Compare with old stiffness
          */


          for (int i=0;i<12;i++)
          {
            for (int j=0;j<12;j++)
            {
              stiffglobal(ele*6 + i ,ele*6 + j)+=stifflocal(i,j);
            }
            resglobal(ele*6 + i ,0)+=reslocal(i,0);
          }

        }//End: Evaluate all elements and assemble


        //add fext
        for (int i=0;i<6*numnode;i++)
        {
          //forces:
          fextglobal(i)="0.0_40";
        }
        fextglobal(numnode*6 -1 -4,0)=fextstep;

        LINALG::TMatrix<cl_F,3,1> fextm;
        LINALG::TMatrix<cl_F,3,3> stiffextm;
        LINALG::TMatrix<cl_F,3,1> tangentdisp;
        for (int i=0;i<3;i++)
        {
          for (int j=0;j<3;j++)
          {
            stiffextm(i,j)="0.0_40";
          }
          fextm(i)="0.0_40";
          tangentdisp(i)=dispglobal(numnode*6-3+i);
        }

        EvaluateNeumannPrec(tangentdisp, mextvecstep, &fextm, &stiffextm);

        for (int i=0;i<3;i++)
        {
          //moments:
          fextglobal(numnode*6-3+i,0)+=fextm(i);
        }

        for (int i=0;i<6*numnode;i++)
        {
          //add fext to residual:
          resglobal(i,0)-=fextglobal(i,0);
          resglobal(i,0)= -resglobal(i,0);
        }

        for (int i=0;i<3;i++)
        {
          for (int j=0;j<3;j++)
          {
            stiffglobal(numnode*6 - 3 + i,numnode*6 - 3 + j)+=stiffextm(i,j);
          }
        }

        //end: add fext

        //apply dirichlet
        for (int j=0;j<3;j++)
        {
          for (int i=0;i<6*numnode;i++)
          {
            stiffglobal(j,i)=cl_F("0.0_40");
            stiffglobal(i,j)=cl_F("0.0_40");
          }
          resglobal(j,0)=cl_F("0.0_40");
          stiffglobal(j,j)=cl_F("1.0_40");
        }

        for (int j=4;j<6;j++)
        {
          for (int i=0;i<6*numnode;i++)
          {
            stiffglobal(j,i)=cl_F("0.0_40");
            stiffglobal(i,j)=cl_F("0.0_40");
          }
          resglobal(j,0)=cl_F("0.0_40");
          stiffglobal(j,j)=cl_F("1.0_40");
        }
        //end: apply dirichlet

        //linear solver
        LINALG::TMatrix<cl_F,numnode*6,numnode*6> stiffglobalsolv;
        LINALG::TMatrix<cl_F,numnode*6,1> resglobalsolv;

        for (int i=0;i<6*numnode;i++)
        {
          for (int j=0;j<6*numnode;j++)
          {
            stiffglobalsolv(i,j)=stiffglobal(i,j);
          }
          resglobalsolv(i,0)=resglobal(i,0);
        }

        //Obere Dreiecksmatrix erzeugen
        for (int k=1; k<numnode*6; k++)
        {
          for (int zeile=k; zeile<numnode*6; zeile++)
          {
            if (abs(stiffglobalsolv(zeile,k-1))<TOLLINSOLV)
            {
              stiffglobalsolv(zeile,k-1)=cl_F("0.0_40");
            }
            else
            {
              cl_F faktor= stiffglobalsolv(zeile,k-1);
              for (int spalte=k-1; spalte<numnode*6; spalte++)
              { //cout << "vorher k, zeile, spalte" << k << " , " << zeile << " , " << spalte << ": " << stiffglobalsolv(zeile, spalte) << endl;
                stiffglobalsolv(zeile,spalte)= -stiffglobalsolv(k-1,spalte)*faktor/stiffglobalsolv(k-1,k-1)+stiffglobalsolv(zeile, spalte);
                //cout << "nachher k, zeile, spalte" << k << " , " << zeile << " , " << spalte << ": " << stiffglobalsolv(zeile, spalte) << endl;
              }
              resglobalsolv(zeile,0)= -resglobalsolv(k-1,0)*faktor/stiffglobalsolv(k-1,k-1)+resglobalsolv(zeile, 0);
            }
          }
        }
        //End:Obere Dreiecksmatrix erzeugen


        //globales deltaDisplacement nullen
        for (int i=0;i<6*numnode;i++)
        {
          deltadispglobal(i,0)=cl_F("0.0_40");
        }
        //globales deltaDisplacement nullen


        //Rückwärtseliminierung
        for (int zeile=numnode*6-1;zeile>-1; zeile--)
        {
          deltadispglobal(zeile,0) = resglobalsolv(zeile,0);
          for (int spalte=zeile+1;spalte<numnode*6; spalte++)
          {
            deltadispglobal(zeile,0) -= deltadispglobal(spalte,0)*stiffglobalsolv(zeile,spalte);
          }
          deltadispglobal(zeile,0) = deltadispglobal(zeile,0)/stiffglobalsolv(zeile,zeile);
        }
        //End: Rückwärtseliminierung

        //Ermittlung des Fehlers
        LINALG::TMatrix<cl_F,numnode*6,1> disperror;
        for (int i=0; i<6*numnode;i++)
        {
          disperror(i,0)= cl_F("0.0_40");
          for (int j=0; j<6*numnode; j++)
          {
            disperror(i,0)+=stiffglobal(i,j)*deltadispglobal(j,0);
          }
          disperror(i,0)-=resglobal(i,0);
          //cout << "disperror " << i << ": " << disperror(i,0) << endl;
        }
        //End: Ermittlung des Fehlers
        //end: linear solver

        //Update Verschiebungszustand
        for (int i=0;i<numnode*6;i++)
        {
          dispglobal(i,0)+=deltadispglobal(i,0);
        }
        //End: Update Verschiebungszustand


        //Berechnung und Ausgabe der Normen
        for (int i=0; i<numnode*6;i++)
        {
          resnorm += resglobal(i,0)*resglobal(i,0);
          dispnorm += deltadispglobal(i,0)*deltadispglobal(i,0);
          linsolverrornorm += disperror(i,0)*disperror(i,0);
        }
        resnorm = sqrt(resnorm)/sqrt(cl_float(numnode*6,float_format(40)));
        dispnorm = sqrt(dispnorm)/sqrt(cl_float(numnode*6,float_format(40)));
        linsolverrornorm = sqrt(linsolverrornorm)/sqrt(cl_float(numnode*6,float_format(40)));
        std::cout << "iter: " << iter << "   resnorm: " << double_approx(resnorm) << "   dispnorm: " << double_approx(dispnorm) << "   linsolverrornorm: " << double_approx(linsolverrornorm) << std::endl;
        //End: Berechnung und Ausgabe der Normen

      }//End: Newton

      std::cout << "end of Newton Iteration" << std::endl;
      std::cout << "dispglobalx: " << dispglobal(6*numnode-6) << std::endl;
      std::cout << "dispglobaly: " << dispglobal(6*numnode-5) << std::endl;
      std::cout << "dispglobalz: " << dispglobal(6*numnode-4) << std::endl;

    }//End Load steps

    exit(0);

    return;
  }

  void DRT::ELEMENTS::Beam3eb::EvaluateNeumannPrec( LINALG::TMatrix<cl_F,3,1> tangentdisp,
                                                    LINALG::TMatrix<cl_F,3,1> mextvec,
                                                    LINALG::TMatrix<cl_F,3,1>* fextm,
                                                    LINALG::TMatrix<cl_F,3,3>* stiffextm)
  {

#if NODALDOFS == 3
    dserror("High precision calculation is not implemented for the case NODALDOFS = 3!!!");
#endif

    LINALG::TMatrix<cl_F,3,1> tangent;
    LINALG::TMatrix<cl_F,3,1> crossproduct;
    cl_F abs_tangent_quadr = "0.0_40";
    //assemble current tangent and moment at node
    for (int i = 0 ; i < 3 ; i++)
    {
      //get current tangent at nodes
      tangent(i) = Trefprec_(i) + tangentdisp(i);
      abs_tangent_quadr += expt(tangent(i),2.0);
    }

    //calculate crossproduct
    (*fextm)(0)=-(tangent(1)*mextvec(2)-tangent(2)*mextvec(1))/abs_tangent_quadr;
    (*fextm)(1)=-(tangent(2)*mextvec(0)-tangent(0)*mextvec(2))/abs_tangent_quadr;
    (*fextm)(2)=-(tangent(0)*mextvec(1)-tangent(1)*mextvec(0))/abs_tangent_quadr;

    //assembly for stiffnessmatrix
     LINALG::TMatrix<cl_F,3,3> crossxtangent;
     LINALG::TMatrix<cl_F,3,3> spinmatrix;

     //perform matrix operation
     for(int i=0; i<3; i++)
     {
       for(int j=0; j<3; j++)
       {
         crossxtangent(i,j) = -(*fextm)(i) * tangent(j);
         spinmatrix(i,j)="0.0_40";
       }
     }

     //Compute Spinmatrix
     spinmatrix(0,1) = -mextvec(2);
     spinmatrix(0,2) = mextvec(1);
     spinmatrix(1,0) = mextvec(2);
     spinmatrix(1,2) = -mextvec(0);
     spinmatrix(2,0) = -mextvec(1);
     spinmatrix(2,1) = mextvec(0);

     //add R_external to stiffness matrix
     //all parts have been evaluated at the boundaries which helps simplifying the matrices
     //In contrast to the Neumann part of the residual force here is NOT a factor of (-1) needed, as elemat1 is directly added to the stiffness matrix
     //without sign change.
     for(int i = 0; i < 3 ; i++)
     {
       for(int j = 0; j < 3 ; j++)
       {
         (*stiffextm)(i,j) -= cl_F("2.0_40") * crossxtangent(i,j) / abs_tangent_quadr;
         (*stiffextm)(i,j) -= spinmatrix(i,j) / abs_tangent_quadr;
       }
     }

    return;
  }


#endif
//***************************************************************************************
//End: Methods for arbitrary precision calculation
//***************************************************************************************
