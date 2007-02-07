/*!----------------------------------------------------------------------
\file shell8.cpp
\brief

<pre>
Maintainer: Michael Gee
            gee@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15239
</pre>

*----------------------------------------------------------------------*/
#ifdef D_SHELL8
#ifdef CCADISCRET
#ifdef TRILINOS_PACKAGE

// This is just here to get the c++ mpi header, otherwise it would
// use the c version included inside standardtypes.h
#ifdef PARALLEL
#include "mpi.h"
#endif
#include "shell8.H"
#include "drt_discret.H"
#include "drt_elementsurface.H"
#include "drt_elementline.H"
#include "drt_utils.H"
#include "drt_exporter.H"
#include "drt_dserror.H"
#include "linalg_utils.H"

extern "C" 
{
#include "../headers/standardtypes.h"
#include "../shell8/shell8.h"
}
#include "dstrc.H"

/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | vector of material laws                                              |
 | defined in global_control.c
 *----------------------------------------------------------------------*/
extern struct _MATERIAL  *mat;

/*----------------------------------------------------------------------*
 |  evaluate the element (public)                            mwgee 12/06|
 *----------------------------------------------------------------------*/
int DRT::Elements::Shell8::Evaluate(ParameterList& params, 
                                    DRT::Discretization&      discretization,
                                    vector<int>&              lm,
                                    Epetra_SerialDenseMatrix& elemat1,
                                    Epetra_SerialDenseMatrix& elemat2,
                                    Epetra_SerialDenseVector& elevec1,
                                    Epetra_SerialDenseVector& elevec2,
                                    Epetra_SerialDenseVector& elevec3)
{
  DSTraceHelper dst("Shell8::Evaluate");  
  DRT::Elements::Shell8::ActionType act = Shell8::none;
  
  // get the action required
  string action = params.get<string>("action","none");
  if (action == "none") dserror("No action supplied");
  else if (action=="calc_struct_linstiff")      act = Shell8::calc_struct_linstiff;
  else if (action=="calc_struct_nlnstiff")      act = Shell8::calc_struct_nlnstiff;
  else if (action=="calc_struct_internalforce") act = Shell8::calc_struct_internalforce;
  else if (action=="calc_struct_linstiffmass")  act = Shell8::calc_struct_linstiffmass;
  else if (action=="calc_struct_nlnstiffmass")  act = Shell8::calc_struct_nlnstiffmass;
  else if (action=="calc_struct_stress")        act = Shell8::calc_struct_stress;
  else if (action=="calc_struct_eleload")       act = Shell8::calc_struct_eleload;
  else if (action=="calc_struct_fsiload")       act = Shell8::calc_struct_fsiload;
  else dserror("Unknown type of action for Shell8");
  
  // get the material law
  MATERIAL* actmat = &(mat[material_-1]);
  
  switch(act)
  {
    case calc_struct_linstiff:
      dserror("Case not yet implemented");
    break;
    case calc_struct_nlnstiff:
      dserror("Case not yet implemented");
    break;
    case calc_struct_internalforce:
      dserror("Case not yet implemented");
    break;
    case calc_struct_linstiffmass:
      dserror("Case not yet implemented");
    break;
    case calc_struct_nlnstiffmass: // do mass, stiffness and internal forces
    {
      // need current displacement and residual forces
      RefCountPtr<const Epetra_Vector> disp = discretization.GetState("displacement");
      RefCountPtr<const Epetra_Vector> res  = discretization.GetState("residual displacement");
      if (disp==null || res==null) dserror("Cannot get state vectors 'displacement' and/or residual");
      vector<double> mydisp(lm.size());
      DRT::Utils::ExtractMyValues(*disp,mydisp,lm);
      vector<double> myres(lm.size());
      DRT::Utils::ExtractMyValues(*res,myres,lm);
      s8_nlnstiffmass(lm,mydisp,myres,&elemat1,&elemat2,&elevec1,actmat);
    }
    break;
    case calc_struct_stress:
      dserror("Case not yet implemented");
    break;
    case calc_struct_eleload:
      dserror("this method is not supposed to evaluate a load, use EvaluateNeumann(...)");
    break;
    case calc_struct_fsiload:
      dserror("Case not yet implemented");
    break;
    default:
      dserror("Unknown type of action for Shell8");
  }
  
  
  return 0;
}

extern "C"
{
  void dyn_facfromcurve(int actcurve,double T,double *fac);  
}

enum LoadType
{
  neum_none,
  neum_live,
  neum_orthopressure,
  neum_consthydro_z,
  neum_increhydro_z,
  neum_live_FSI,
  neum_opres_FSI
};
static void s8loadgaussianpoint(double eload[][MAXNOD_SHELL8], const double hhi,
                         double wgt, const double xjm[][3], 
                         const vector<double>& funct,
                         const Epetra_SerialDenseMatrix& deriv,
                         const int iel, 
                         const double xi,
                         const double yi,
                         const double zi,
                         const enum LoadType ltype,
                         const vector<int>& onoff,
                         const vector<double>& val,
                         const double curvefac,
                         const double time);
/*----------------------------------------------------------------------*
 |  Integrate a Surface Neumann boundary condition (public)  mwgee 01/07|
 *----------------------------------------------------------------------*/
int DRT::Elements::Shell8::EvaluateNeumann(ParameterList& params, 
                                           DRT::Discretization&      discretization,
                                           DRT::Condition&           condition,
                                           vector<int>&              lm,
                                           Epetra_SerialDenseVector& elevec1)
{
  RefCountPtr<const Epetra_Vector> disp = discretization.GetState("displacement");
  if (disp==null) dserror("Cannot get state vector 'displacement'");
  vector<double> mydisp(lm.size());
  DRT::Utils::ExtractMyValues(*disp,mydisp,lm);
  
  // find out whether we will use a time curve
  bool usetime = true;
  const double time = params.get("total time",-1.0);
  if (time<0.0) usetime = false;

  // no. of nodes on this surface
  const int iel = NumNode();

  // init gaussian points
  S8_DATA s8data;
  s8_integration_points(s8data);
  
  const int nir = ngp_[0]; 
  const int nis = ngp_[1]; 
  const int numdf = 6;
  vector<double>* thick = data_.GetVector<double>("thick");
  if (!thick) dserror("Cannot find vector of nodal thicknesses");
  Epetra_SerialDenseMatrix* a3ref = data_.GetMatrix("a3ref");
  if (!a3ref) dserror("Cannot find array of directors");
  double a3ref2[3][MAXNOD_SHELL8];
  for (int i=0; i<3; ++i)
    for (int j=0; j<iel; ++j)
      a3ref2[i][j] = (*a3ref)(i,j);
      
  vector<double> funct(iel);
  Epetra_SerialDenseMatrix deriv(2,iel);
  
  double a3r[3][MAXNOD_SHELL8];
  double a3c[3][MAXNOD_SHELL8];
  double a3cur[3][MAXNOD_SHELL8];
  double xrefe[3][MAXNOD_SHELL8];
  double xcure[3][MAXNOD_SHELL8];
  double xjm[3][3];
  double eload[6][MAXNOD_SHELL8];
  for (int i=0; i<numdf; ++i)
    for (int j=0; j<iel; ++j)
      eload[i][j] = 0.0;

  // update geometry
  for (int k=0; k<iel; ++k)
  {
    const double h2 = (*thick)[k];
    
    a3r[0][k] = (*a3ref)(0,k)*h2;
    a3r[1][k] = (*a3ref)(1,k)*h2;
    a3r[2][k] = (*a3ref)(2,k)*h2;
    
    xrefe[0][k] = Nodes()[k]->X()[0];
    xrefe[1][k] = Nodes()[k]->X()[1];
    xrefe[2][k] = Nodes()[k]->X()[2];
    
    xcure[0][k] = xrefe[0][k] + mydisp[k*numdf+0];
    xcure[1][k] = xrefe[1][k] + mydisp[k*numdf+1];
    xcure[2][k] = xrefe[2][k] + mydisp[k*numdf+2];
    
    a3c[0][k] = a3r[0][k] + mydisp[k*numdf+3];
    a3c[1][k] = a3r[1][k] + mydisp[k*numdf+4];
    a3c[2][k] = a3r[2][k] + mydisp[k*numdf+5];

    a3cur[0][k] = a3c[0][k] / h2;
    a3cur[1][k] = a3c[1][k] / h2;
    a3cur[2][k] = a3c[2][k] / h2;
  }

  // find out whether we will use a time curve and get the factor
  vector<int>* curve  = condition.GetVector<int>("curve");
  int curvenum = -1;
  if (curve) curvenum = (*curve)[0]; 
  double curvefac = 1.0;
  if (curvenum>=0 && usetime)
    dyn_facfromcurve(curvenum,time,&curvefac); 
  
  // get type of condition
  LoadType ltype;
  string* type = condition.GetString("type");
  if      (*type == "neum_live")          ltype = neum_live;
  else if (*type == "neum_live_FSI")      ltype = neum_live_FSI;
  else if (*type == "neum_orthopressure") ltype = neum_orthopressure;
  else if (*type == "neum_consthydro_z")  ltype = neum_consthydro_z;
  else if (*type == "neum_increhydro_z")  ltype = neum_increhydro_z;
  else dserror("Unknown type of SurfaceNeumann condition");
  
  // get values and switches from the condition
  vector<int>*    onoff = condition.GetVector<int>("onoff");
  vector<double>* val   = condition.GetVector<double>("val");
  
  // start integration
  double e3=0.0;
  for (int lr=0; lr<nir; ++lr) // loop r direction
  {
    // gaussian points and weights
    double e1   = s8data.xgpr[lr];
    double facr = s8data.wgtr[lr];
    for (int ls=0; ls<nis; ++ls) // loop s direction
    {
      double e2   = s8data.xgps[ls];
      double facs = s8data.wgts[ls];
      // shape functiosn and derivatives at gaussian point
      s8_shapefunctions(funct,deriv,e1,e2,iel,1);
      // element thickness at gaussian point
      double hhi = 0.0;
      for (int i=0; i<iel; ++i) hhi += funct[i] * (*thick)[i];
      // Jacobian matrix
      // in reference configuration
      double det,deta;
      if (ltype == neum_live)
        s8_jaco(funct,deriv,xrefe,xjm,*thick,a3ref2,e3,iel,&det,&deta);
      else
        s8_jaco(funct,deriv,xcure,xjm,*thick,a3cur,e3,iel,&det,&deta);
      // total weight at gaussian point
      double wgt = facr*facs;
      // coordinates of gaussian point (needed in some types)
      double xi,yi,zi;
      xi=yi=zi=0.0;
      if (ltype != neum_live)
        for (int i=0; i<iel; ++i)
        {
          xi += xcure[0][i]*funct[i];
          yi += xcure[1][i]*funct[i];
          zi += xcure[2][i]*funct[i];
        }
      // do load calculation at gaussian point
      s8loadgaussianpoint(eload,hhi,wgt,xjm,funct,deriv,iel,xi,yi,zi,ltype,
                            *onoff,*val,curvefac,time);
      } // for (int ls=0; ls<nis; ++ls)
    } // for (int lr=0; lr<nir; ++lr)
  
  // add eload to element vector
  for (int inode=0; inode<iel; ++inode)
    for (int dof=0; dof<6; ++dof)
      elevec1[inode*iel+dof] += eload[dof][inode];
  
  return 0;
}

/*----------------------------------------------------------------------*
 |  evaluate the element (private)                            mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_nlnstiffmass(vector<int>&              lm, 
                                            vector<double>&           disp, 
                                            vector<double>&           residual,
                                            Epetra_SerialDenseMatrix* stiffmatrix,
                                            Epetra_SerialDenseMatrix* massmatrix,
                                            Epetra_SerialDenseVector* force,
                                            struct _MATERIAL*         material)
{
  DSTraceHelper dst("Shell8::s8_nlnstiffmass");  

  const int numnode = NumNode();
  const int numdf   = 6;
  int       ngauss  = 0;
  const int nd      = numnode*numdf;

  // general arrays
  vector<double>           funct(numnode);
  Epetra_SerialDenseMatrix deriv(2,numnode);
  Epetra_SerialDenseMatrix bop(12,nd);
  Epetra_SerialDenseVector intforce(nd);
  double D[12][12];    // mid surface material tensor
  double stress[6];
  double strain[6];
  double stress_r[12]; // mid surface stress resultants
  ARRAY C_a;
  double** C = (double**)amdef("C",&C_a,6 ,6,"DA");
  double a3r[3][MAXNOD_SHELL8];
  double a3c[3][MAXNOD_SHELL8];
  double xrefe[3][MAXNOD_SHELL8];
  double xcure[3][MAXNOD_SHELL8];
  double akovr[3][3];
  double akonr[3][3];
  double amkovr[3][3];
  double amkonr[3][3];
  double a3kvpr[3][2];
  double akovc[3][3];
  double akonc[3][3];
  double amkovc[3][3];
  double amkonc[3][3];
  double a3kvpc[3][2];
  double detr = 0.0;
  double detc = 0.0;
  double h[3];
  double da;
  double gkovr[3][3];
  double gkonr[3][3];
  double gmkovr[3][3];
  double gmkonr[3][3];
  double gkovc[3][3];
  double gkonc[3][3];
  double gmkovc[3][3];
  double gmkonc[3][3];
  
  // for ans
  int ansq=0;
  int nsansq=0;
  const int nsansmax = 6;
  double xr1[nsansmax];
  double xs1[nsansmax];
  double xr2[nsansmax];
  double xs2[nsansmax];
  double frq[nsansmax];
  double fsq[nsansmax];
  
  vector<double> funct1q[nsansmax];
  vector<double> funct2q[nsansmax];
  Epetra_SerialDenseMatrix deriv1q[nsansmax];
  Epetra_SerialDenseMatrix deriv2q[nsansmax];
  
  double akovr1q[nsansmax][3][3];
  double akonr1q[nsansmax][3][3];
  double amkovr1q[nsansmax][3][3];
  double amkonr1q[nsansmax][3][3];
  double a3kvpr1q[nsansmax][3][2];

  double akovc1q[nsansmax][3][3];
  double akonc1q[nsansmax][3][3];
  double amkovc1q[nsansmax][3][3];
  double amkonc1q[nsansmax][3][3];
  double a3kvpc1q[nsansmax][3][2];

  double akovr2q[nsansmax][3][3];
  double akonr2q[nsansmax][3][3];
  double amkovr2q[nsansmax][3][3];
  double amkonr2q[nsansmax][3][3];
  double a3kvpr2q[nsansmax][3][2];

  double akovc2q[nsansmax][3][3];
  double akonc2q[nsansmax][3][3];
  double amkovc2q[nsansmax][3][3];
  double amkonc2q[nsansmax][3][3];
  double a3kvpc2q[nsansmax][3][2];
  
  // for eas
  Epetra_SerialDenseMatrix P;          
  Epetra_SerialDenseMatrix transP;
  Epetra_SerialDenseMatrix T;
  Epetra_SerialDenseMatrix Lt;
  Epetra_SerialDenseMatrix Dtild;
  Epetra_SerialDenseMatrix Dtildinv;
  vector<double>           Rtild(0);
  vector<double>           epsh(12);  // transformed eas strains
  double akovr0[3][3];
  double akonr0[3][3];
  double amkovr0[3][3];
  double amkonr0[3][3];
  double detr0;
  double akovc0[3][3];
  double akonc0[3][3];
  double amkovc0[3][3];
  double amkonc0[3][3];
  double detc0;
  vector<double>*           alfa        = NULL;
  Epetra_SerialDenseMatrix* oldDtildinv = NULL;
  Epetra_SerialDenseMatrix* oldLt       = NULL;
  vector<double>*           oldRtild    = NULL;


  // gaussian points
  S8_DATA s8data;
  s8_integration_points(s8data);
  
  vector<double>* thick = NULL;
  thick = data_.GetVector<double>("thick");
  if (!thick) dserror("Cannot find nodal thicknesses");
  
  // eas
  if (nhyb_)
  {
    // init to zero
    P.Shape(12,nhyb_);
    transP.Shape(12,nhyb_);
    T.Shape(12,12);
    Lt.Shape(nhyb_,nd);
    Dtild.Shape(nhyb_,nhyb_);
    Dtildinv.Shape(nhyb_,nhyb_);
    Rtild.resize(nhyb_); for (int i=0; i<nhyb_; ++i) Rtild[i] = 0.0;
    
    // access history stuff stored in element
    alfa        = data_.GetVector<double>("alfa");
    oldDtildinv = data_.GetMatrix("Dtildinv");
    oldLt       = data_.GetMatrix("Lt");
    oldRtild    = data_.GetVector<double>("Rtild");
    if (!alfa || !oldDtildinv || !oldLt || !oldRtild) dserror("Missing data");
    /*---------------- make multiplication eashelp = oldLt * disp[kstep] */
    vector<double> eashelp(nhyb_);
    s8_YpluseqAx(eashelp,*oldLt,disp,1.0,true);
    /*---------------------------------------- add old Rtilde to eashelp */
    for (int i=0; i<nhyb_; ++i) eashelp[i] += (*oldRtild)[i];
    /*----------------- make multiplication alfa -= olDtildinv * eashelp */
    s8_YpluseqAx(*alfa,*oldDtildinv,eashelp,-1.0,false);
  } // if (nhyb_)

  // ------------------------------------ check calculation of mass matrix
  int imass=0;
  double density=0.0;
  if (massmatrix)
  {
    imass=1;
    s8_getdensity(material,&density);
  }
  
  /*----------------------------------------------- integrationsparameter */
  const int nir = ngp_[0];
  const int nis = ngp_[1];
  const int nit = ngp_[2];
  const int iel = numnode;
  const double condfac = sdc_;
  Epetra_SerialDenseMatrix* a3ref = data_.GetMatrix("a3ref");
  if (!a3ref) dserror("Cannot get data a3ref");
  
  /*----------------------------------------------------- geometry update */
  for (int k=0; k<iel; ++k)
  {
    const double h2 = (*thick)[k]*condfac/2.0;
    
    a3r[0][k] = (*a3ref)(0,k)*h2;
    a3r[1][k] = (*a3ref)(1,k)*h2;
    a3r[2][k] = (*a3ref)(2,k)*h2;
    
    xrefe[0][k] = Nodes()[k]->X()[0];
    xrefe[1][k] = Nodes()[k]->X()[1];
    xrefe[2][k] = Nodes()[k]->X()[2];
    
    xcure[0][k] = xrefe[0][k] + disp[k*numdf+0];
    xcure[1][k] = xrefe[1][k] + disp[k*numdf+1];
    xcure[2][k] = xrefe[2][k] + disp[k*numdf+2];
    
    a3c[0][k] = a3r[0][k] + disp[k*numdf+3];
    a3c[1][k] = a3r[1][k] + disp[k*numdf+4];
    a3c[2][k] = a3r[2][k] + disp[k*numdf+5];
  }

  /*============ metric and shape functions at collocation points (ANS=1) */
  /*----------------------------------------------------- 4-noded element */
  if (ans_==1 || ans_==3)
  {
    ansq=1;
    nsansq=0;
    if (iel==4) nsansq=2;
    if (iel==9) nsansq=6;
    for (int i=0; i<nsansq; ++i)
    {
      funct1q[i].resize(iel);
      funct2q[i].resize(iel);
      deriv1q[i].Shape(2,iel);
      deriv2q[i].Shape(2,iel);
    }
    s8_ans_colloquationpoints(nsansq,iel,ans_,
                              xr1,xs1,xr2,xs2,
                              funct1q,deriv1q,funct2q,deriv2q,
                              xrefe,a3r,xcure,a3c,
                              akovr1q,akonr1q,amkovr1q,amkonr1q,a3kvpr1q,
                              akovc1q,akonc1q,amkovc1q,amkonc1q,a3kvpc1q,
                              akovr2q,akonr2q,amkovr2q,amkonr2q,a3kvpr2q,
                              akovc2q,akonc2q,amkovc2q,amkonc2q,a3kvpc2q,
                              &detr,&detc);
  }
  
  /*=============================== metric of element mid point (for eas) */
  if (nhyb_)
  {
    s8_shapefunctions(funct,deriv,0.,0.,iel,1);
    s8tmtr(xrefe,a3r,0.0,akovr0,akonr0,amkovr0,amkonr0,&detr0,
           funct,deriv,iel,condfac,0);
    s8tmtr(xcure,a3c,0.0,akovc0,akonc0,amkovc0,amkonc0,&detc0,
              funct,deriv,iel,condfac,0);
  }
  
  /*=================================================== integration loops */
  for (int lr=0; lr<nir; ++lr)
  {
    /*================================== gaussian point and weight at it */
    const double e1   = s8data.xgpr[lr];
    const double facr = s8data.wgtr[lr];
    for (int ls=0; ls<nis; ++ls)
    {
      const double e2   = s8data.xgps[ls];
      const double facs = s8data.wgts[ls];
      /*-------------------- shape functions at gp e1,e2 on mid surface */
      s8_shapefunctions(funct,deriv,e1,e2,iel,1);
      /*----------------------------- shape functions for querschub-ans */
      if (ansq==1) s8_ansqshapefunctions(frq,fsq,e1,e2,iel,nsansq);
      /*-------- init mid surface material tensor and stress resultants */
      for (int i=0; i<12; ++i) 
      {
        stress_r[i] = 0.;
        for (int j=0; j<12; ++j) D[i][j] = 0.;
      }
      /*------------------------------------ init mass matrix variables */
      double facv  = 0.;
      double facw  = 0.;
      double facvw = 0.;
      /*------------------------------------- metrics at gaussian point */
      s8tvmr(xrefe,a3r,akovr,akonr,amkovr,amkonr,&detr,funct,deriv,iel,a3kvpr,0);
      s8tvmr(xcure,a3c,akovc,akonc,amkovc,amkonc,&detc,funct,deriv,iel,a3kvpc,0);
      /*------------------------- make h as cross product in ref config.
                                       to get area da on shell mid surf */
      h[0] = akovr[1][0]*akovr[2][1] - akovr[2][0]*akovr[1][1];
      h[1] = akovr[2][0]*akovr[0][1] - akovr[0][0]*akovr[2][1];
      h[2] = akovr[0][0]*akovr[1][1] - akovr[1][0]*akovr[0][1];
      /*------------------------------------- make director unit lenght
                                        and get midsurf area da from it */
      s8unvc(&da,h,3);
       /*--------------------------------------- make eas if switched on */
       if (nhyb_)
       {
         /*------------------- make shape functions for incomp. strains */
         s8eas(nhyb_,e1,e2,iel,eas_,P);
         /*-------------------- transform basis of Eij to Gausian point */
         s8transeas(P,transP,T,akovr,akonr0,detr,detr0,nhyb_);
         /*------------------------ transform strains to Gaussian point */
         s8_YpluseqAx(epsh,transP,*alfa,1.0,true);
       }
       /*------------------------ make B-operator for compatible strains */
       s8tvbo(e1,e2,bop,funct,deriv,iel,numdf,akovc,a3kvpc,nsansq);
       /*-------------------------------------- modifications due to ans */
       if (ansq) s8ansbbarq(bop,frq,fsq,funct1q,funct2q,deriv1q,deriv2q,
                            akovc1q,akovc2q,a3kvpc1q,a3kvpc2q,iel,numdf,nsansq);
       /*============================== loop GP in thickness direction t */
       for (int lt=0; lt<nit; ++lt)
       {
         /*---------------------------- gaussian point and weight at it */
         const double e3   = s8data.xgpt[lt];
         double       fact = s8data.wgtt[lt];
         /*-------------------- basis vectors and metrics at shell body */
         s8tmtr(xrefe,a3r,e3,gkovr,gkonr,gmkovr,gmkonr,&detr,funct,deriv,iel,condfac,0);
         s8tmtr(xcure,a3c,e3,gkovc,gkonc,gmkovc,gmkonc,&detc,funct,deriv,iel,condfac,0);
         /*--------------------------------- metric at gp in shell body */
         if (!ansq)
           s8tvhe(gmkovr,gmkovc,gmkonr,gmkonc,gkovr,gkovc,&detr,&detc,
                  amkovc,amkovr,akovc,akovr,a3kvpc,a3kvpr,e3,condfac);
         /*- modifications to metric of shell body due to querschub-ans */
         else;
           s8anstvheq(gmkovr,gmkovc,gmkonr,gmkonc,gkovr,gkovc,amkovc,amkovr,
                      akovc,akovr,a3kvpc,a3kvpr,&detr,&detc,
                      amkovr1q,amkovc1q,akovr1q,akovc1q,a3kvpr1q,a3kvpc1q,
                      amkovr2q,amkovc2q,akovr2q,akovc2q,a3kvpr2q,a3kvpc2q,
                      frq,fsq,e3,nsansq,iel,condfac);
         /*----------- calc shell shifter and put it in the weight fact */
         const double xnu = (1.0/condfac)*(detr/da);
         fact *= xnu;
         /*----------------------- change to current metrics due to eas */
         if (nhyb_) s8vthv(gmkovc,gmkonc,epsh,&detc,e3,condfac);
         /*------------------------------------------ call material law */
         s8tmat(material,stress,strain,C,gmkovc,gmkonc,gmkovr,gmkonr,
                gkovc,gkonc,gkovr,gkonr,detc,detr,e3,0,ngauss);
         /*---------------- do thickness integration of material tensor */
         s8tvma(D,C,stress,stress_r,e3,fact,condfac);
         /*-------------------------- mass matrix thickness integration */
         if (imass)
         {
           facv  += s8data.wgtt[lt] * detr;
           facw  += s8data.wgtt[lt] * detr * e3 * e3;
           facvw += s8data.wgtt[lt] * detr * e3;
         }
         /*-------------------------------------------------------------*/
       } // for (int lt=0; lt<nit; ++lt)
       /*------------ product of all weights and jacobian of mid surface */
       double weight = facr*facs*da;
       /*----------------------------------- elastic stiffness matrix ke */
       s8BtDB(*stiffmatrix,bop,D,iel,numdf,weight);
       /*--------------------------------- geometric stiffness matrix kg */
       if (!ansq) s8tvkg(*stiffmatrix,stress_r,funct,deriv,numdf,iel,weight,e1,e2); 
       else       s8anstvkg(*stiffmatrix,stress_r,funct,deriv,numdf,iel,weight,e1,e2,
                            frq,fsq,funct1q,funct2q,deriv1q,deriv2q,ansq,nsansq);
       /*-------------------------------- calculation of internal forces */
       if (force) s8intforce(intforce,stress_r,bop,iel,numdf,12,weight);
       /*------------- mass matrix : gaussian point on shell mid surface */
       if (imass)
       {
         double fac = facr * facs * density;
         facv  *= fac;
         facw  *= fac;
         facvw *= fac;
         s8tmas(funct,*thick,*massmatrix,iel,numdf,facv,facw,facvw);
       }
       /*----------------------------------- integration of eas matrices */
       if (nhyb_)
       {
         /*==========================================================*/
         /*  Ltrans(nhyb,nd) = Mtrans(nhyb,12) * D(12,12) * B(12,nd) */
         /*==========================================================*/
         /*-------------------------------------------------- DB=D*B */
         Epetra_SerialDenseMatrix workeas(12,nd); // bug in old version (nhyb,nd)
         s8matmatdense(workeas,D,bop,12,12,nd,0,0.0);
         /*----------------------------------- Ltransposed = Mt * DB */
         s8mattrnmatdense(Lt,transP,workeas,nhyb_,12,nd,1,weight);
         /*==========================================================*/
         /*  Dtilde(nhyb,nhyb) = Mtrans(nhyb,12) * D(12,12) * M(12,nhyb) */
         /*==========================================================*/
         /*-------------------------------------------------DM = D*M */
         workeas.Shape(12,nhyb_);
         s8matmatdense(workeas,D,transP,12,12,nhyb_,0,0.0);
         /*------------------------------------------ Dtilde = Mt*DM */
         s8mattrnmatdense(Dtild,transP,workeas,nhyb_,12,nhyb_,1,weight);
         /*==========================================================*/
         /*  Rtilde(nhyb) = Mtrans(nhyb,12) * Forces(12)             */
         /*==========================================================*/
         /*---------------------- eas part of internal forces Rtilde */
         s8mattrnvecdense(Rtild,transP,stress_r,nhyb_,12,1,weight);
       }
       ngauss++;
    } // for (int ls=0; ls<nis; ++ls)
  } // for (int lr=0; lr<nir; ++lr)
  /*----------------- make modifications to stiffness matrices due to eas */
  if (nhyb_)
  {
    /*------------------------------------ make inverse of matrix Dtilde */
    Epetra_SerialDenseMatrix Dtildinv(Dtild);
    LINALG::SymmetricInverse(Dtildinv,nhyb_);
    /*===================================================================*/
    /* estif(nd,nd) = estif(nd,nd) - Ltrans(nhyb,nd) * Dtilde^-1(nhyb,nhyb) * L(nd,nhyb) */
    /*===================================================================*/
    Epetra_SerialDenseMatrix workeas(nd,nhyb_);
    /*------------------------------------------- make Ltrans * Dtildinv */
    s8mattrnmatdense(workeas,Lt,Dtildinv,nd,nhyb_,nhyb_,0,0.0);
    /*---------------------------------- make estif -= Lt * Dtildinv * L */
    s8matmatdense(*stiffmatrix,workeas,Lt,nd,nhyb_,nd,1,-1.0);
    /*===================================================================*/
    /* R(nd) = R(nd) - Ltrans(nhyb,nd) * Dtilde^-1(nhyb,nhyb) * Rtilde(nhyb) */
    /*===================================================================*/
    /*--------------------------- make intforce -= Lt * Dtildinv * Rtild */
    s8_YpluseqAx(intforce,workeas,Rtild,-1.0,false);
    /*------------------------------------------ put Dtildinv to storage */
    /*------------------------------------------------ put Lt to storage */
    /*-------------------------------------------- put Rtilde to storage */
    for (int i=0; i<nhyb_; ++i)
    {
      for (int j=0; j<nhyb_; ++j) (*oldDtildinv)(i,j) = Dtildinv(i,j);
      for (int j=0; j<nd; ++j)    (*oldLt)(i,j)       = Lt(i,j);
                                  (*oldRtild)[i]      = Rtild[i];
    }
  } // if (nhyb_)
  /*- add internal forces to global vector, if a global vector was passed */
  /*                                                      to this routine */
  if (force)
    for (int i=0; i<nd; ++i) (*force)[i] += intforce[i];
  //------------------------------------------------ delete the only ARRAY
  amdel(&C_a);
  /*------------------------------------- make estif absolute symmetric */
  for (int i=0; i<nd; ++i)
    for (int j=i+1; j<nd; ++j)
    {
      const double average = 0.5*( (*stiffmatrix)(i,j) + (*stiffmatrix)(j,i) );
      (*stiffmatrix)(i,j) = average;
      (*stiffmatrix)(j,i) = average;
    }
  
#if 0  
printf("Element id %d\n",Id());
for (int i=0; i<nd; ++i)
{
  for (int j=0; j<nd; ++j)
    printf("  %15.10e  ",(*stiffmatrix)(i,j));
  printf("\n");
}
printf("internal forces\n");
for (int i=0; i<nd; ++i) 
  printf("%15.10e\n",(*force)[i]);
fflush(stdout);
exit(0);
#endif  
  
  return;
} // DRT::Elements::Shell8::s8_nlnstiffmass


/*----------------------------------------------------------------------*
 | calculation of gaussian points mass with update of displacements     |
 |                                                 (private) 12/06 mgee |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8tmas(
              const vector<double>& funct, const vector<double>& thick,
              Epetra_SerialDenseMatrix& emass, const int iel, const int numdf,
              const double facv, const double facw, const double facvw)
{
  DSTraceHelper dst("Shell8::s8tmas");
/*----------------------------------------------------------------------*/
/*---------------------------- half element thickness at gaussian point */
  double he=0.0;
  for (int i=0; i<iel; ++i) he+= thick[i]*funct[i];
  he /=2.0;
  const double hehe=he*he;
/*---------------------------------------------------- make mass matrix */
  for (int i=0; i<iel; ++i)
    for (int j=0; j<iel; ++j)
    {
      const double helpf = funct[i] * funct[j];
      double       help  = facv * helpf;
      for (int k=0; k<3; ++k)
        emass(j*numdf+k,i*numdf+k) += help;
      
      help = facw * helpf * hehe;
      for (int k=3; k<6; ++k)
        emass(j*numdf+k,i*numdf+k) += help;
        
      if (abs(facvw)>1.0e-14)
      {
        help = facvw * helpf * he;
        emass(j*numdf+3,i*numdf+0) += help;
        emass(j*numdf+4,i*numdf+1) += help;
        emass(j*numdf+5,i*numdf+2) += help;
        emass(j*numdf+0,i*numdf+3) += help;
        emass(j*numdf+1,i*numdf+4) += help;
        emass(j*numdf+2,i*numdf+5) += help;
      }
    }
/*----------------------------------------------------------------------*/
  return;
}




/*----------------------------------------------------------------------*
 | make internal forces                                                 |
 |                                                 (private) 12/06 mgee |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8intforce(Epetra_SerialDenseVector& intforce, const double stress_r[],
                  const Epetra_SerialDenseMatrix& bop, const int iel, 
                  const int numdf, const int nstress_r, const double weight)
{
  DSTraceHelper dst("Shell8::s8intforce");
/*----------------------------------------------------------------------*/
  const int nd = iel*numdf;
  /*
  make intforce[nd] = transposed(bop[nstress_r][nd]) * stress_r[nstress_r]
  */
  for (int i=0; i<nd; ++i)
  {
    double sum=0.0;
    for (int k=0; k<nstress_r; ++k) sum += bop(k,i)*stress_r[k];
    intforce[i] += sum*weight;
  }
/*----------------------------------------------------------------------*/
  return;
}


/*----------------------------------------------------------------------*
 | geometric stiffness matrix kg  with ans                              |
 |                                                 (private) 12/06 mgee |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8anstvkg(
                 Epetra_SerialDenseMatrix& estif, double stress_r[], 
                 const vector<double>& funct, const Epetra_SerialDenseMatrix& deriv,
                 const int numdf, const int iel, const double weight,
                 const double e1, const double e2,
                 const double frq[], const double fsq[], 
                 const vector<double> funct1q[], const vector<double> funct2q[],
                 const Epetra_SerialDenseMatrix deriv1q[], const Epetra_SerialDenseMatrix deriv2q[],
                 const int ansq, const int nsansq)
{
  DSTraceHelper dst("Shell8::s8anstvkg");
/*----------------------------------------------------------------------*/
  const double sn11 = stress_r[0];
  const double sn21 = stress_r[1];
  const double sn31 = stress_r[2];
  const double sn22 = stress_r[3];
  const double sn32 = stress_r[4];
  const double sn33 = stress_r[5];
  const double sm11 = stress_r[6];
  const double sm21 = stress_r[7];
  const double sm31 = stress_r[8];
  const double sm22 = stress_r[9];
  const double sm32 = stress_r[10];
/*----------------------------------------------------------------------*/
  for (int inode=0; inode<iel; ++inode)
  {
    for (int jnode=0; jnode<=inode; ++jnode)
    {
      const double pi = funct[inode];
      const double pj = funct[jnode];
      
      const double d11 = deriv(0,inode) * deriv(0,jnode);
      const double d12 = deriv(0,inode) * deriv(1,jnode);
      const double d21 = deriv(1,inode) * deriv(0,jnode);
      const double d22 = deriv(1,inode) * deriv(1,jnode);
      
      const double xn = (sn11*d11 + sn21*(d12+d21) + sn22*d22) * weight;
      const double xm = (sm11*d11 + sm21*(d12+d21) + sm22*d22) * weight;

      double pd1ij,pd1ji,pd2ij,pd2ji,yu,yo;
      /*----------------------------------------- no ans for querschub */
      if (!ansq)
      {
        pd1ij = deriv(0,inode) * pj;
        pd1ji = deriv(0,jnode) * pi;
        pd2ij = deriv(1,inode) * pj;
        pd2ji = deriv(1,jnode) * pi;
        yu = (sn31*pd1ji + sn32*pd2ji) * weight;
        yo = (sn31*pd1ij + sn32*pd2ij) * weight;
      }
      /*----------------------------------------- ans for querschub */
      else
      {
        yu=0.0;
        yo=0.0;
        for (int i=0; i<nsansq; ++i)
        {
          pd1ij = deriv1q[i](0,inode) * funct1q[i][jnode] * frq[i];
          pd1ji = deriv1q[i](0,jnode) * funct1q[i][inode] * frq[i];
          pd2ij = deriv2q[i](1,inode) * funct2q[i][jnode] * fsq[i];
          pd2ji = deriv2q[i](1,jnode) * funct2q[i][inode] * fsq[i];
          yu += (sn31*pd1ji + sn32*pd2ji) * weight;
          yo += (sn31*pd1ij + sn32*pd2ij) * weight;
        } 
      }
      /*---------------- linear part of querschub is always unmodified */
      pd1ij = deriv(0,inode) * pj;
      pd1ji = deriv(0,jnode) * pi;
      pd2ij = deriv(1,inode) * pj;
      pd2ji = deriv(1,jnode) * pi;
      const double yy = (sm31*(pd1ij+pd1ji) + sm32*(pd2ij+pd2ji)) * weight;
      const double z  = pi*pj*sn33*weight;
      
      estif(inode*numdf+0,jnode*numdf+0) += xn;
      estif(inode*numdf+1,jnode*numdf+1) += xn;
      estif(inode*numdf+2,jnode*numdf+2) += xn;
      
      estif(inode*numdf+3,jnode*numdf+0) += (xm+yu);
      estif(inode*numdf+4,jnode*numdf+1) += (xm+yu);
      estif(inode*numdf+5,jnode*numdf+2) += (xm+yu);
      
      estif(inode*numdf+0,jnode*numdf+3) += (xm+yo);
      estif(inode*numdf+1,jnode*numdf+4) += (xm+yo);
      estif(inode*numdf+2,jnode*numdf+5) += (xm+yo);
      
      estif(inode*numdf+3,jnode*numdf+3) += (yy+z);
      estif(inode*numdf+4,jnode*numdf+4) += (yy+z);
      estif(inode*numdf+5,jnode*numdf+5) += (yy+z);
      
      if (inode!=jnode)
      {
         estif(jnode*numdf+0,inode*numdf+0) += xn;
         estif(jnode*numdf+1,inode*numdf+1) += xn;
         estif(jnode*numdf+2,inode*numdf+2) += xn;
      
         estif(jnode*numdf+0,inode*numdf+3) += (xm+yu);
         estif(jnode*numdf+1,inode*numdf+4) += (xm+yu);
         estif(jnode*numdf+2,inode*numdf+5) += (xm+yu);
      
         estif(jnode*numdf+3,inode*numdf+0) += (xm+yo);
         estif(jnode*numdf+4,inode*numdf+1) += (xm+yo);
         estif(jnode*numdf+5,inode*numdf+2) += (xm+yo);
      
         estif(jnode*numdf+3,inode*numdf+3) += (yy+z);
         estif(jnode*numdf+4,inode*numdf+4) += (yy+z);
         estif(jnode*numdf+5,inode*numdf+5) += (yy+z);
      }
    } // for (int jnode=0; jnode<=inode; ++jnode)
  } // for (int inode=0; inode<iel; ++inode)



/*----------------------------------------------------------------------*/
  return;
}


/*----------------------------------------------------------------------*
 | geometric stiffness matrix kg                                        |
 |                                                 (private) 12/06 mgee |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8tvkg(
              Epetra_SerialDenseMatrix& estif, double stress_r[], 
              const vector<double>& funct, const Epetra_SerialDenseMatrix& deriv,
              const int numdf, const int iel, const double weight,
              const double e1, const double e2)
{
  DSTraceHelper dst("Shell8::s8tvkg");
/*----------------------------------------------------------------------*/
  const double sn11 = stress_r[0];
  const double sn21 = stress_r[1];
  const double sn31 = stress_r[2];
  const double sn22 = stress_r[3];
  const double sn32 = stress_r[4];
  const double sn33 = stress_r[5];
  const double sm11 = stress_r[6];
  const double sm21 = stress_r[7];
  const double sm31 = stress_r[8];
  const double sm22 = stress_r[9];
  const double sm32 = stress_r[10];
  for (int inode=0; inode<iel; ++inode)
  {
    for (int jnode=0; jnode<=inode; ++jnode)
    {
      const double pi = funct[inode];
      const double pj = funct[jnode];
      
      const double d11 = deriv(0,inode) * deriv(0,jnode);
      const double d12 = deriv(0,inode) * deriv(1,jnode);
      const double d21 = deriv(1,inode) * deriv(0,jnode);
      const double d22 = deriv(1,inode) * deriv(1,jnode);
      
      const double pd1ij = deriv(0,inode) * pj;
      const double pd1ji = deriv(0,jnode) * pi;
      const double pd2ij = deriv(1,inode) * pj;
      const double pd2ji = deriv(1,jnode) * pi;

      const double xn = (sn11*d11 + sn21*(d12+d21) + sn22*d22) * weight;
      const double xm = (sm11*d11 + sm21*(d12+d21) + sm22*d22) * weight;
      const double yu = (sn31*pd1ji + sn32*pd2ji) * weight;
      const double yo = (sn31*pd1ij + sn32*pd2ij) * weight;
      const double yy = (sm31*(pd1ij+pd1ji) + sm32*(pd2ij+pd2ji)) * weight;
      const double z  = pi*pj*sn33*weight;
      
      estif(inode*numdf+0,jnode*numdf+0) += xn;
      estif(inode*numdf+1,jnode*numdf+1) += xn;
      estif(inode*numdf+2,jnode*numdf+2) += xn;
      
      estif(inode*numdf+3,jnode*numdf+0) += (xm+yu);
      estif(inode*numdf+4,jnode*numdf+1) += (xm+yu);
      estif(inode*numdf+5,jnode*numdf+2) += (xm+yu);
      
      estif(inode*numdf+0,jnode*numdf+3) += (xm+yo);
      estif(inode*numdf+1,jnode*numdf+4) += (xm+yo);
      estif(inode*numdf+2,jnode*numdf+5) += (xm+yo);
      
      estif(inode*numdf+3,jnode*numdf+3) += (yy+z);
      estif(inode*numdf+4,jnode*numdf+4) += (yy+z);
      estif(inode*numdf+5,jnode*numdf+5) += (yy+z);
      
      if (inode!=jnode)
      {
         estif(jnode*numdf+0,inode*numdf+0) += xn;
         estif(jnode*numdf+1,inode*numdf+1) += xn;
         estif(jnode*numdf+2,inode*numdf+2) += xn;
      
         estif(jnode*numdf+0,inode*numdf+3) += (xm+yu);
         estif(jnode*numdf+1,inode*numdf+4) += (xm+yu);
         estif(jnode*numdf+2,inode*numdf+5) += (xm+yu);
      
         estif(jnode*numdf+3,inode*numdf+0) += (xm+yo);
         estif(jnode*numdf+4,inode*numdf+1) += (xm+yo);
         estif(jnode*numdf+5,inode*numdf+2) += (xm+yo);
      
         estif(jnode*numdf+3,inode*numdf+3) += (yy+z);
         estif(jnode*numdf+4,inode*numdf+4) += (yy+z);
         estif(jnode*numdf+5,inode*numdf+5) += (yy+z);
      }
    } // for (int jnode=0; jnode<=inode; ++jnode)
  } // for (int inode=0; inode<iel; ++inode)
/*----------------------------------------------------------------------*/
  return;
}


/*----------------------------------------------------------------------*
 | integrate material law and stresses in thickness direction of shell  |
 |                                                 (private) 12/06 mgee |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8tvma(
              double D[][12], double** C, double stress[], double stress_r[], 
              const double e3, const double fact, const double condfac)
{
  DSTraceHelper dst("Shell8::s8tvma");
/*----------------------------------------------------------------------*/
  const double zeta = e3/condfac;
  for (int i=0; i<6; ++i)
  {
    const int i6 = i+6;
    const double stress_fact = stress[i]*fact;
    stress_r[i]  += stress_fact;
    stress_r[i6] += stress_fact * zeta;
    for (int j=0; j<6; ++j)
    {
      const int j6 = j+6;
      const double C_fact = C[i][j]*fact;
      D[i][j]   += C_fact;
      D[i6][j]  += C_fact*zeta;
      D[i6][j6] += C_fact*zeta*zeta;
    }
  }
/*-------------------------------------------------------- symmetrize D */
for (int i=0; i<12; i++)
{
   for (int j=i+1; j<12; j++)
   {
      D[i][j]=D[j][i];
   }
}
/*----------------------------------------------------------------------*/
  return;
}

/*----------------------------------------------------------------------*
 | calculate Ke += Bt * D * B                                           |
 |                                                 (private) 12/06 mgee |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8BtDB(Epetra_SerialDenseMatrix& estif, 
                                   const Epetra_SerialDenseMatrix& bop,
                                   const double D[][12], 
                                   const int iel, 
                                   const int numdf, 
                                   const double weight)
{
  DSTraceHelper dst("Shell8::s8BtDB");
/*----------------------------------------------------------------------*/
  const int dim = iel*numdf;
/*------------------------------------ make multiplication work = D * B */
  double work[12][dim];
  for (int i=0; i<12; ++i)
    for (int j=0; j<dim; ++j)
    {
      double sum = 0.0;
      for (int k=0; k<12; ++k) sum += D[i][k]*bop(k,j);
      work[i][j] = sum;
    }
/*--------------------------- make multiplication estif += bop^t * work */    
  for (int i=0; i<dim; ++i)
    for (int j=0; j<dim; ++j)
    {
      double sum = 0.0;
      for (int k=0; k<12; ++k) sum += bop(k,i) * work[k][j];
      estif(i,j) += sum*weight;
    }
/*----------------------------------------------------------------------*/
  return;
}


/*----------------------------------------------------------------------*
 | call material laws  (private)                             mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8tmat(
              struct _MATERIAL* material, 
              double stress[], double strain[], double** C,
              double gmkovc[][3], double gmkonc[][3], 
              double gmkovr[][3], double gmkonr[][3],
              double gkovc[][3], double gkonc[][3],
              double gkovr[][3], double gkonr[][3], 
              const double detc, const double detr, 
              const double e3, const int option, 
              const int ngauss)
{
  DSTraceHelper dst("Shell8::s8tmat");
/*----------------------------------------------------------------------*/
  // make strains
  strain[0] = 0.5*(gmkovc[0][0] - gmkovr[0][0]);
  strain[1] = 0.5*(gmkovc[0][1] - gmkovr[0][1]);
  strain[2] = 0.5*(gmkovc[0][2] - gmkovr[0][2]);
  strain[3] = 0.5*(gmkovc[1][1] - gmkovr[1][1]);
  strain[4] = 0.5*(gmkovc[1][2] - gmkovr[1][2]);
  strain[5] = 0.5*(gmkovc[2][2] - gmkovr[2][2]);
/*------------------------------------------------ switch material type */
  switch (material->mattyp)
  {
    case m_stvenant:/*------------------------ st.venant-kirchhoff-material */
    {
      ARRAY tmp;
      double** gmkonrtmp = (double**)amdef("tmp",&tmp,3,3,"DA");
      for (int i=0; i<3; ++i)
      for (int j=0; j<3; ++j) gmkonrtmp[i][j] = gmkonr[i][j];
      s8_mat_linel(mat->m.stvenant,gmkonrtmp,C);
      s8_mat_stress1(stress,strain,C);
      amdel(&tmp);
    }
    break;
    case m_neohooke:/*------------------------------ kompressible neo-hooke */
    {
      ARRAY tmp1;
      ARRAY tmp2;
      double** gmkonrtmp = (double**)amdef("tmp",&tmp1,3,3,"DA");
      double** gmkonctmp = (double**)amdef("tmp",&tmp2,3,3,"DA");
      for (int i=0; i<3; ++i)
      for (int j=0; j<3; ++j) 
      {
        gmkonrtmp[i][j] = gmkonr[i][j];
        gmkonctmp[i][j] = gmkonc[i][j];
      }
      s8_mat_neohooke(material->m.neohooke,stress,C,gmkonrtmp,gmkonctmp,detr,detc);
      amdel(&tmp1);
      amdel(&tmp2);
    }
    break;
    case m_compogden:/*--------------------------------- kompressible ogden */
    {
      ARRAY tmp1;
      ARRAY tmp2;
      double** gkonrtmp = (double**)amdef("tmp",&tmp1,3,3,"DA");
      double** gmkovctmp = (double**)amdef("tmp",&tmp2,3,3,"DA");
      for (int i=0; i<3; ++i)
      for (int j=0; j<3; ++j) 
      {
        gkonrtmp[i][j] = gkonr[i][j];
        gmkovctmp[i][j] = gmkovc[i][j];
      }
      /*----------------------------- call compressible ogden material law */
      double C4[3][3][3][3];
      /*         Ogden hyperelasticity without deviatoric-volumetric split */
      // s8_mat_ogden_coupled(mat->m.compogden,stress,C4,gkonrtmp,gmkovctmp);
      /*            Ogden hyperelasticity with deviatoric-volumetric split */
      s8_mat_ogden_uncoupled2(mat->m.compogden,stress,C4,gkonrtmp,gmkovctmp);
      /* PK2 stresses are cartesian ->  return stresses to curvilinear bases */
      s8_kon_cacu(stress,gkonrtmp);
      /*---------------- C4 is cartesian -> return C4 to curvilinear bases */
      s8_4kon_cacu(C4,gkonrtmp);
      /*---------------------- sort material tangent from tensor to matrix */
      s8_c4_to_C2(C4,C);
      /*-------------------------------------------------------------------*/
      amdel(&tmp1);
      amdel(&tmp2);
    }
    break;
    case m_viscohyper:/*-------------------------viscous kompressible ogden */
      dserror("viscous kompressible ogden in shell8 not ported to DRT");
    break;
    default:
      dserror("Ilegal typ of material for element shell8");
    break;
  } // switch (material->mattyp)


/*----------------------------------------------------------------------*/
  return;
}


/*----------------------------------------------------------------------*
 |   (private)                                               mwgee 12/06|
 *----------------------------------------------------------------------*/
/*
C.......................................................................
C!    GEAENDERTE METRIK DES VERF. SCHALENRAUMS INFOLGE ENHANCED STRAIN .
C.......................................................................
*/
void DRT::Elements::Shell8::s8vthv(double gmkovc[][3], double gmkonc[][3], 
                                   const vector<double>& epsh,
                                   double* detc, const double e3, const double condfac)
{
  DSTraceHelper dst("Shell8::s8vthv");
/*----------------------------------------------------------------------*/
  const double zeta = e3/condfac;
/*----------------------------------------------------------------------*/
  gmkovc[0][0] = gmkovc[0][0] + 2.0 * (epsh[0]+zeta*epsh[6]);
  gmkovc[1][0] = gmkovc[1][0] +       (epsh[1]+zeta*epsh[7]);
  gmkovc[2][0] = gmkovc[2][0] +       (epsh[2]+zeta*epsh[8]);
  gmkovc[1][1] = gmkovc[1][1] + 2.0 * (epsh[3]+zeta*epsh[9]);
  gmkovc[2][1] = gmkovc[2][1] +       (epsh[4]+zeta*epsh[10]);
  gmkovc[2][2] = gmkovc[2][2] + 2.0 * (epsh[5]+zeta*epsh[11]);
  gmkovc[0][2] = gmkovc[2][0];
  gmkovc[1][2] = gmkovc[2][1];
  gmkovc[0][1] = gmkovc[1][0];
/*----------------------------------------------------------------------*/
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) gmkonc[i][j] = gmkovc[i][j];
  double det_dummy;
  s8inv3(gmkonc,&det_dummy);
  if (det_dummy<=0.0) det_dummy=-det_dummy;
  *detc = sqrt(det_dummy);
/*----------------------------------------------------------------------*/
  return;
}


/*----------------------------------------------------------------------*
 |   (private)                                               mwgee 12/06|
 | modifications to metrics of shell body due to ans for querschub      |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8anstvheq(
                  double gmkovr[][3], double gmkovc[][3], double gmkonr[][3], double gmkonc[][3],
                  double gkovr[][3], double gkovc[][3], double amkovc[][3], double amkovr[][3],
                  double akovc[][3], double akovr[][3], double a3kvpc[][2], double a3kvpr[][2],
                  double* detr, double* detc,
                  double amkovr1q[][3][3], double amkovc1q[][3][3],
                  double akovr1q[][3][3], double akovc1q[][3][3],
                  double a3kvpr1q[][3][2], double a3kvpc1q[][3][2],
                  double amkovr2q[][3][3], double amkovc2q[][3][3],
                  double akovr2q[][3][3], double akovc2q[][3][3],
                  double a3kvpr2q[][3][2], double a3kvpc2q[][3][2],
                  double frq[], double fsq[], const double e3, 
                  const int nansq, const int iel, const double condfac)
{
  DSTraceHelper dst("Shell8::s8anstvheq");
  
  double b11c=0.0;
  double b12c=0.0;
  double b21c=0.0;
  double b22c=0.0;
  double b31c=0.0;
  double b32c=0.0;
  double b11r=0.0;
  double b12r=0.0;
  double b21r=0.0;
  double b22r=0.0;
  double b31r=0.0;
  double b32r=0.0;
  const double zeta = e3/condfac;
  
/*----------------------------------------------------------------------*/
  for (int i=0; i<3; ++i)
  {
    b11c += akovc[i][0]*a3kvpc[i][0];
    b12c += akovc[i][0]*a3kvpc[i][1];
    b21c += akovc[i][1]*a3kvpc[i][0];
    b22c += akovc[i][1]*a3kvpc[i][1];
    b31c += akovc[i][2]*a3kvpc[i][0];
    b32c += akovc[i][2]*a3kvpc[i][1];
  
    b11r += akovr[i][0]*a3kvpr[i][0];
    b12r += akovr[i][0]*a3kvpr[i][1];
    b21r += akovr[i][1]*a3kvpr[i][0];
    b22r += akovr[i][1]*a3kvpr[i][1];
    b31r += akovr[i][2]*a3kvpr[i][0];
    b32r += akovr[i][2]*a3kvpr[i][1];
  }
/*----------------------------------------------------------------------*/
  gmkovc[0][0] = gmkovr[0][0] + (amkovc[0][0]-amkovr[0][0]) + zeta * 2.0 * (b11c-b11r);
  gmkovc[1][1] = gmkovr[1][1] + (amkovc[1][1]-amkovr[1][1]) + zeta * 2.0 * (b22c-b22r);
  gmkovc[2][2] = gmkovr[2][2] + (amkovc[2][2]-amkovr[2][2]);
  gmkovc[0][1] = gmkovr[0][1] + (amkovc[0][1]-amkovr[0][1]) + zeta * (b21c+b12c-b21r-b12r);
  gmkovc[0][2] = gmkovr[0][2]                               + zeta * (b31c-b31r);
  gmkovc[1][2] = gmkovr[1][2]                               + zeta * (b32c-b32r);
  gmkovc[2][0] = gmkovc[0][2];
  gmkovc[2][1] = gmkovc[1][2];
  gmkovc[1][0] = gmkovc[0][1];
/*----------------------------------------------------------------------*/
  for (int i=0; i<nansq; i++)
  {
     gmkovc[0][2] += (amkovc1q[i][0][2]-amkovr1q[i][0][2]) * frq[i];
     gmkovc[1][2] += (amkovc2q[i][1][2]-amkovr2q[i][1][2]) * fsq[i];
  }
  gmkovc[2][0] = gmkovc[0][2];
  gmkovc[2][1] = gmkovc[1][2];
/*----------------------------------------------------------------------*/
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) gmkonr[i][j] = gmkovr[i][j];
  double det_dummy;
  s8inv3(gmkonr,&det_dummy);
  if (det_dummy<=0.0) det_dummy=1.0e-08;
  *detr = sqrt(det_dummy);
  
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) gmkonc[i][j] = gmkovc[i][j];
  s8inv3(gmkonc,&det_dummy);
  if (det_dummy<=0.0) det_dummy=1.0e-08;
  *detc = sqrt(det_dummy);
/*----------------------------------------------------------------------*/
  return;
}


/*----------------------------------------------------------------------*
 |  calculates metrics (geom. nonlinear)                  m.gee 12/06   |
 *----------------------------------------------------------------------*/
/*
C.......................................................................
C!    METRIK DES SCHALENRAUMS
c!    gmkovc wird hier erneut berechnet, um die Vernachlaessigung der
c!    in e3 quadratischen Anteile zu beruecksichtigen, d.h.
c!    gmkovc_ij ungleich gkovc_i*gkovc_j                                         .
C.......................................................................
*/
void DRT::Elements::Shell8::s8tvhe(
              double gmkovr[][3], double gmkovc[][3], 
              double gmkonr[][3], double gmkonc[][3],
              double gkovr[][3], double gkovc[][3],
              double* detr, double* detc,
              double amkovc[][3], double amkovr[][3],
              double akovc[][3], double akovr[][3],
              double a3kvpc[][2], double a3kvpr[][2],
              const double e3, const double condfac)
{
  DSTraceHelper dst("Shell8::s8tvhe");

  double b11c=0.0;
  double b12c=0.0;
  double b21c=0.0;
  double b22c=0.0;
  double b31c=0.0;
  double b32c=0.0;
  double b11r=0.0;
  double b12r=0.0;
  double b21r=0.0;
  double b22r=0.0;
  double b31r=0.0;
  double b32r=0.0;
  const double zeta = e3/condfac;
  
  for (int i=0; i<3; ++i)
  {
    b11c += akovc[i][0]*a3kvpc[i][0];
    b12c += akovc[i][0]*a3kvpc[i][1];
    b21c += akovc[i][1]*a3kvpc[i][0];
    b22c += akovc[i][1]*a3kvpc[i][1];
    b31c += akovc[i][2]*a3kvpc[i][0];
    b32c += akovc[i][2]*a3kvpc[i][1];

    b11r += akovr[i][0]*a3kvpr[i][0];
    b12r += akovr[i][0]*a3kvpr[i][1];
    b21r += akovr[i][1]*a3kvpr[i][0];
    b22r += akovr[i][1]*a3kvpr[i][1];
    b31r += akovr[i][2]*a3kvpr[i][0];
    b32r += akovr[i][2]*a3kvpr[i][1];
  }

  gmkovc[0][0] = gmkovr[0][0] + (amkovc[0][0]-amkovr[0][0]) + zeta*2.0*(b11c-b11r);
  gmkovc[1][1] = gmkovr[1][1] + (amkovc[1][1]-amkovr[1][1]) + zeta*2.0*(b22c-b22r);
  gmkovc[2][2] = gmkovr[2][2] + (amkovc[2][2]-amkovr[2][2]);
  gmkovc[0][1] = gmkovr[0][1] + (amkovc[0][1]-amkovr[0][1]) + zeta*(b21c+b12c-b21r-b12r);
  gmkovc[0][2] = gmkovr[0][2] + (amkovc[0][2]-amkovr[0][2]) + zeta*(b31c-b31r);
  gmkovc[1][2] = gmkovr[1][2] + (amkovc[1][2]-amkovr[1][2]) + zeta*(b32c-b32r);
  gmkovc[2][0] = gmkovc[0][2];
  gmkovc[2][1] = gmkovc[1][2];
  gmkovc[1][0] = gmkovc[0][1];

  double detdummy;
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) gmkonr[i][j] = gmkovr[i][j];
  s8inv3(gmkonr,&detdummy);
  if (detdummy<=0.0) detdummy=1.0e-08;
  *detr = sqrt(detdummy);
  
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) gmkonc[i][j] = gmkovc[i][j];
  s8inv3(gmkonc,&detdummy);
  if (detdummy<=0.0) detdummy=1.0e-08;
  *detc = sqrt(detdummy);

  return;
}


/*----------------------------------------------------------------------*
 |  B-Operator ans modification (private)                    mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8ansbbarq(
     Epetra_SerialDenseMatrix& bop, 
     const double frq[], const double fsq[],
     const vector<double> funct1q[], const vector<double> funct2q[],
     const Epetra_SerialDenseMatrix deriv1q[], const Epetra_SerialDenseMatrix deriv2q[],
     const double akovc1q[][3][3], const double akovc2q[][3][3],
     const double a3kvpc1q[][3][2], const double a3kvpc2q[][3][2],
     const int& iel, const int& numdf, const int& nsansq)
{
  DSTraceHelper dst("Shell8::s8ansbbarq");
  
  for (int inode=0; inode<iel; ++inode)
  {
    int node_start = inode*numdf;
    
    bop(2,node_start+0)= 0.0;
    bop(2,node_start+1)= 0.0;
    bop(2,node_start+2)= 0.0;
    bop(2,node_start+3)= 0.0;
    bop(2,node_start+4)= 0.0;
    bop(2,node_start+5)= 0.0;

    bop(4,node_start+0)= 0.0;
    bop(4,node_start+1)= 0.0;
    bop(4,node_start+2)= 0.0;
    bop(4,node_start+3)= 0.0;
    bop(4,node_start+4)= 0.0;
    bop(4,node_start+5)= 0.0;
    
    for (int isamp=0; isamp<nsansq; ++isamp)
    {
      const double a1x1 = akovc1q[isamp][0][0];
      const double a1y1 = akovc1q[isamp][1][0];
      const double a1z1 = akovc1q[isamp][2][0];
      const double a3x1 = akovc1q[isamp][0][2];
      const double a3y1 = akovc1q[isamp][1][2];
      const double a3z1 = akovc1q[isamp][2][2];

      const double a2x2 = akovc2q[isamp][0][1];
      const double a2y2 = akovc2q[isamp][1][1];
      const double a2z2 = akovc2q[isamp][2][1];
      const double a3x2 = akovc2q[isamp][0][2];
      const double a3y2 = akovc2q[isamp][1][2];
      const double a3z2 = akovc2q[isamp][2][2];

      //const double a31x1 = a3kvpc1q[isamp][0][0];
      //const double a31y1 = a3kvpc1q[isamp][1][0];
      //const double a31z1 = a3kvpc1q[isamp][2][0];

      //const double a32x2 = a3kvpc2q[isamp][0][1];
      //const double a32y2 = a3kvpc2q[isamp][1][1];
      //const double a32z2 = a3kvpc2q[isamp][2][1];

      const double p1k = funct1q[isamp][inode];
      const double p2k = funct2q[isamp][inode];

      const double pk1 = deriv1q[isamp](0,inode);
      const double pk2 = deriv2q[isamp](1,inode);

      const double fris = frq[isamp];
      const double fsis = fsq[isamp];
/*--------------------------------------------------E13(CONST)-------- */
      bop(2,node_start+0)+= pk1*a3x1*fris;
      bop(2,node_start+1)+= pk1*a3y1*fris;
      bop(2,node_start+2)+= pk1*a3z1*fris;
      bop(2,node_start+3)+= p1k*a1x1*fris;
      bop(2,node_start+4)+= p1k*a1y1*fris;
      bop(2,node_start+5)+= p1k*a1z1*fris;
/*--------------------------------------------------E23(CONST)-------- */
      bop(4,node_start+0)+= pk2*a3x2*fsis;
      bop(4,node_start+1)+= pk2*a3y2*fsis;
      bop(4,node_start+2)+= pk2*a3z2*fsis;
      bop(4,node_start+3)+= p2k*a2x2*fsis;
      bop(4,node_start+4)+= p2k*a2y2*fsis;
      bop(4,node_start+5)+= p2k*a2z2*fsis;
    } // for (int isamp=0; isamp<nsansq; ++isamp)
  } // for (int inode=0; inode<iel; ++inode)
  return;
}



/*----------------------------------------------------------------------*
 |  B-Operator for compatible strains (private)              mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8tvbo(const double e1, const double e2,
                                   Epetra_SerialDenseMatrix& bop,
                                   const vector<double>& funct,
                                   const Epetra_SerialDenseMatrix& deriv,
                                   const int iel,
                                   const int numdf,
                                   const double akov[][3],
                                   const double a3kvp[][2],
                                   const int nsansq)
{
  DSTraceHelper dst("Shell8::s8tvbo");
  /*----------------------------------------------------------------------*/
  const double a1x=akov[0][0];
  const double a1y=akov[1][0];
  const double a1z=akov[2][0];
  const double a2x=akov[0][1];
  const double a2y=akov[1][1];
  const double a2z=akov[2][1];
  const double a3x=akov[0][2];
  const double a3y=akov[1][2];
  const double a3z=akov[2][2];
  const double a31x=a3kvp[0][0];
  const double a31y=a3kvp[1][0];
  const double a31z=a3kvp[2][0];
  const double a32x=a3kvp[0][1];
  const double a32y=a3kvp[1][1];
  const double a32z=a3kvp[2][1];
  /*----------------------------------------------------------------------*/
  for (int inode=0; inode<iel; ++inode)
  {
    const double pk = funct[inode];
    const double pk1 = deriv(0,inode);
    const double pk2 = deriv(1,inode);
    
    const int node_start = inode*numdf;
    
   bop(0,node_start+0)= pk1*a1x;
   bop(0,node_start+1)= pk1*a1y;
   bop(0,node_start+2)= pk1*a1z;
   bop(0,node_start+3)= 0.0;
   bop(0,node_start+4)= 0.0;
   bop(0,node_start+5)= 0.0;

   bop(1,node_start+0)= pk2*a1x + pk1*a2x;
   bop(1,node_start+1)= pk2*a1y + pk1*a2y;
   bop(1,node_start+2)= pk2*a1z + pk1*a2z;
   bop(1,node_start+3)= 0.0;
   bop(1,node_start+4)= 0.0;
   bop(1,node_start+5)= 0.0;
    
   if (!nsansq)
   {
     bop(2,node_start+0)= pk1*a3x;
     bop(2,node_start+1)= pk1*a3y;
     bop(2,node_start+2)= pk1*a3z;
     bop(2,node_start+3)= pk *a1x;
     bop(2,node_start+4)= pk *a1y;
     bop(2,node_start+5)= pk *a1z;
   }
   
   bop(3,node_start+0)= pk2*a2x;
   bop(3,node_start+1)= pk2*a2y;
   bop(3,node_start+2)= pk2*a2z;
   bop(3,node_start+3)= 0.0;
   bop(3,node_start+4)= 0.0;
   bop(3,node_start+5)= 0.0;
   
   if (!nsansq)
   {
     bop(4,node_start+0)= pk2*a3x;
     bop(4,node_start+1)= pk2*a3y;
     bop(4,node_start+2)= pk2*a3z;
     bop(4,node_start+3)= pk *a2x;
     bop(4,node_start+4)= pk *a2y;
     bop(4,node_start+5)= pk *a2z;
   }
   
   bop(5,node_start+0)= 0.0;
   bop(5,node_start+1)= 0.0;
   bop(5,node_start+2)= 0.0;
   bop(5,node_start+3)= pk *a3x;
   bop(5,node_start+4)= pk *a3y;
   bop(5,node_start+5)= pk *a3z;

   bop(6,node_start+0)= pk1*a31x;
   bop(6,node_start+1)= pk1*a31y;
   bop(6,node_start+2)= pk1*a31z;
   bop(6,node_start+3)= pk1*a1x;
   bop(6,node_start+4)= pk1*a1y;
   bop(6,node_start+5)= pk1*a1z;

   bop(7,node_start+0)= pk1*a32x + pk2*a31x;
   bop(7,node_start+1)= pk1*a32y + pk2*a31y;
   bop(7,node_start+2)= pk1*a32z + pk2*a31z;
   bop(7,node_start+3)= pk1* a2x + pk2* a1x;
   bop(7,node_start+4)= pk1* a2y + pk2* a1y;
   bop(7,node_start+5)= pk1* a2z + pk2* a1z;

   bop(8,node_start+0)= 0.0;
   bop(8,node_start+1)= 0.0;
   bop(8,node_start+2)= 0.0;
   bop(8,node_start+3)= pk *a31x + pk1*a3x;
   bop(8,node_start+4)= pk *a31y + pk1*a3y;
   bop(8,node_start+5)= pk *a31z + pk1*a3z;

   bop(9,node_start+0)= pk2*a32x;
   bop(9,node_start+1)= pk2*a32y;
   bop(9,node_start+2)= pk2*a32z;
   bop(9,node_start+3)= pk2*a2x;
   bop(9,node_start+4)= pk2*a2y;
   bop(9,node_start+5)= pk2*a2z;

   bop(10,node_start+0)= 0.0;
   bop(10,node_start+1)= 0.0;
   bop(10,node_start+2)= 0.0;
   bop(10,node_start+3)= pk *a32x + pk2*a3x;
   bop(10,node_start+4)= pk *a32y + pk2*a3y;
   bop(10,node_start+5)= pk *a32z + pk2*a3z;

   bop(11,node_start+0)= 0.0;
   bop(11,node_start+1)= 0.0;
   bop(11,node_start+2)= 0.0;
   bop(11,node_start+3)= 0.0;
   bop(11,node_start+4)= 0.0;
   bop(11,node_start+5)= 0.0;
  } // for (int inode=0; inode<iel; ++inode)
  return;
}


/*----------------------------------------------------------------------*
 |  transform the eas-strains from midpoint to gausspoint (private) mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8transeas(
                  Epetra_SerialDenseMatrix& P, Epetra_SerialDenseMatrix& transP,
                  Epetra_SerialDenseMatrix& T, const double akovr[][3],
                  const double akonr0[][3], const double detr, const double detr0,
                  const int nhyb)
{
  DSTraceHelper dst("Shell8::s8transeas");
  
  const double two=2.0;
  double t11,t12,t13,t21,t22,t23,t31,t32,t33;
  const double factor = detr0/detr;;
  
  /*--------------------------- components of the transformation matrix T */
  t11=0.0;
  t12=0.0;
  t13=0.0;
  t21=0.0;
  t22=0.0;
  t23=0.0;
  t31=0.0;
  t32=0.0;
  t33=1.0;
  for (int i=0; i<3; ++i)
  {
   t11 += akovr[i][0]*akonr0[i][0];
   t12 += akovr[i][0]*akonr0[i][1];
/*   t13 += akovr[i][0]*akonr0[i][2]; */
   t21 += akovr[i][1]*akonr0[i][0];
   t22 += akovr[i][1]*akonr0[i][1];
/*   t23 += akovr[i][1]*akonr0[i][2]; */
/*   t31 += akovr[i][2]*akonr0[i][0]; */
/*   t32 += akovr[i][2]*akonr0[i][1]; */
/*   t33 += akovr[i][2]*akonr0[i][2]; */
  }

  T(0,0) = factor*t11*t11;
  T(1,0) = factor*two*t11*t21;
  T(2,0) = factor*two*t11*t31;
  T(3,0) = factor*t21*t21;
  T(4,0) = factor*two*t21*t31;
  T(5,0) = factor*t31*t31;
  
  T(0,1) = factor*t11*t12;
  T(1,1) = factor*(t11*t22+t21*t12);
  T(2,1) = factor*(t11*t32+t31*t12);
  T(3,1) = factor*t21*t22;
  T(4,1) = factor*(t21*t32+t31*t22);
  T(5,1) = factor*t31*t32;
  
  T(0,2) = factor*t11*t13;
  T(1,2) = factor*(t11*t23+t21*t13);
  T(2,2) = factor*(t11*t33+t31*t13);
  T(3,2) = factor*t21*t23;
  T(4,2) = factor*(t21*t33+t31*t23);
  T(5,2) = factor*t31*t33;
  
  T(0,3) = factor*t12*t12;
  T(1,3) = factor*two*t12*t22;
  T(2,3) = factor*two*t12*t32;
  T(3,3) = factor*t22*t22;
  T(4,3) = factor*two*t22*t32;
  T(5,3) = factor*t32*t32;

  T(0,4) = factor*t12*t13          ;
  T(1,4) = factor*(t12*t23+t22*t13);
  T(2,4) = factor*(t12*t33+t32*t13);
  T(3,4) = factor*t22*t23          ;
  T(4,4) = factor*(t22*t33+t32*t23);
  T(5,4) = factor*t32*t33          ;
  
  T(0,5) = factor*t13*t13    ;
  T(1,5) = factor*two*t13*t23;
  T(2,5) = factor*two*t13*t33;
  T(3,5) = factor*t23*t23    ;
  T(4,5) = factor*two*t23*t33;
  T(5,5) = factor*t33*t33    ;
  
  T(6,6)  = factor*t11*t11     ;
  T(7,6)  = factor*two*t11*t21 ;
  T(8,6)  = factor*two*t11*t31 ;
  T(9,6)  = factor*t21*t21    ;
  T(10,6) = factor*two*t21*t31;
  T(11,6) = factor*t31*t31    ;
  
  T(6,7)  = factor*t11*t12           ;
  T(7,7)  = factor*(t11*t22+t21*t12) ;
  T(8,7)  = factor*(t11*t32+t31*t12) ;
  T(9,7)  = factor*t21*t22          ;
  T(10,7) = factor*(t21*t32+t31*t22);
  T(11,7) = factor*t31*t32          ;
  
  T(6,8)  = factor*t11*t13           ;
  T(7,8)  = factor*(t11*t23+t21*t13) ;
  T(8,8)  = factor*(t11*t33+t31*t13) ;
  T(9,8)  = factor*t21*t23          ;
  T(10,8) = factor*(t21*t33+t31*t23);
  T(11,8) = factor*t31*t33          ;
  
  T(6,9)  = factor*t12*t12     ;
  T(7,9)  = factor*two*t12*t22 ;
  T(8,9)  = factor*two*t12*t32 ;
  T(9,9)  = factor*t22*t22    ;
  T(10,9) = factor*two*t22*t32;
  T(11,9) = factor*t32*t32    ;
  
  T(6,10)  = factor*t12*t13           ;
  T(7,10)  = factor*(t12*t23+t22*t13) ;
  T(8,10)  = factor*(t12*t33+t32*t13) ;
  T(9,10)  = factor*t22*t23          ;
  T(10,10) = factor*(t22*t33+t32*t23);
  T(11,10) = factor*t32*t33          ;
  
  T(6,11)  = factor*t13*t13     ;
  T(7,11)  = factor*two*t13*t23 ;
  T(8,11)  = factor*two*t13*t33 ;
  T(9,11)  = factor*t23*t23    ;
  T(10,11) = factor*two*t23*t33;
  T(11,11) = factor*t33*t33    ;
  
  for (int i=0; i<6; ++i)
    for (int j=0; j<6; ++j)
    {
      T(i,j+6) = 0.0;
      T(i+6,j) = 0.0;
    }
    
  /*-------------------------------------------- multiply transP = T*P */
  s8matmatdense(transP,T,P,12,12,nhyb,0,1.);
  
  return;
}



/*----------------------------------------------------------------------*
 |  do eas (private)                                         mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8eas(const int nhyb, const double e1, const double e2, 
                                  const int iel, const int* eas, 
                                  Epetra_SerialDenseMatrix& P)
{
  DSTraceHelper dst("Shell8::s8eas");  

  int place_P=0;
  
  const int nrr=0;
  const int nss=3;
  const int nrs=1;
  const int mrr=6;
  const int mss=9;
  const int mrs=7;
  const int qr=2;
  const int qs=4;
  const int sr=8;
  const int ss=10;
  const int st=11;
  
  const double e1e2     = e1*e2;
  const double e1e1     = e1*e1;
  const double e2e2     = e2*e2;
  const double e1e1e2   = e1*e1e2;
  const double e1e2e2   = e1e2*e2;
  const double e1e1e2e2 = e1e2*e1e2;
  
  if (iel>4) /*------------------------------------- nine node element */
  {
/*----------------------------------------------------------------------
      MEMBRAN: E11,E12,E22 KONSTANT
  ----------------------------------------------------------------------*/
  switch(eas[0])
  {
    case 0:
    break;
    case 7:
      P(nrr,place_P)   = e2-3.0*e1e1e2;
      P(nrr,place_P+1) = e2e2-3.0*e1e1e2e2;
      P(nss,place_P+2) = e1-3.0*e1e2e2;
      P(nss,place_P+3) = e1e1-3.0*e1e1e2e2;
      P(nrs,place_P+4) = e2-3.0*e1e1e2;
      P(nrs,place_P+5) = e1-3.0*e1e2e2;
      P(nrs,place_P+6) = 1.0-3.0*(e1e1+e2e2)+9.0*e1e1e2e2;
      place_P+=7;
    break;
    case 9:
      P(nrr,place_P)   = 1.0-3.0*e1e1;;
      P(nrr,place_P+1) = e2-3.0*e1e1e2;;
      P(nss,place_P+2) = 1.0-3.0*e2e2;
      P(nss,place_P+3) = e1-3.0*e1e2e2;
      P(nrs,place_P+4) = 1.0-3.0*e1e1e2;
      P(nrs,place_P+5) = 1.0-3.0*e1e2e2;
      P(nrs,place_P+6) = e2-3.0*e1e1e2;
      P(nrs,place_P+7) = e1-3.0*e1e2e2;
      P(nrs,place_P+8) = 1.0-3.0*(e1e1+e2e2)+9.0*e1e1e2e2;
      place_P+=9;
    break;
    case 11:
      P(nrr,place_P)    = 1.0-3.0*e1e1;
      P(nrr,place_P+1)  = e2-3.0*e1e1e2;
      P(nrr,place_P+2)  = e2e2-3.0*e1e1e2e2;
      P(nss,place_P+3)  = 1.0-3.0*e2e2;
      P(nss,place_P+4)  = e1-3.0*e1e2e2;
      P(nss,place_P+5)  = e1e1-3.0*e1e1e2e2;
      P(nrs,place_P+6)  = 1.0-3.0*e1e1;
      P(nrs,place_P+7)  = 1.0-3.0*e2e2;
      P(nrs,place_P+8)  = e2-3.0*e1e1e2;
      P(nrs,place_P+9)  = e1-3.0*e1e2e2;
      P(nrs,place_P+10) = 1.0-3.0*(e1e1+e2e2)+9.0*e1e1e2e2;
      place_P+=11;
    break;
    default:
      dserror("eas: MEMBRAN: E11,E12,E22 KONSTANT other then 0,7,9,11");
    break;
  }
/*----------------------------------------------------------------------
      BIEGUNG: E11,E12,E22 LINEAR
  ----------------------------------------------------------------------*/
  switch (eas[1])
  {
    case 0:
    break;
    case 9:
      P(mrr,place_P)   = 1.0-3.0*e1e1;
      P(mrr,place_P+1) = e2-3.0*e1e1e2;
      P(mss,place_P+2) = 1.0-3.0*e2e2;
      P(mss,place_P+3) = e1-3.0*e1e2e2;
      P(mrs,place_P+4) = 1.0-3.0*e1e1;
      P(mrs,place_P+5) = 1.0-3.0*e2e2;
      P(mrs,place_P+6) = e2-3.0*e1e1e2;
      P(mrs,place_P+7) = e1-3.0*e1e2e2;
      P(mrs,place_P+8) = 1.0-3.0*(e1e1+e2e2)+9.0*e1e1e2e2;
      place_P+=9;
    break;
    case 11:
      P(mrr,place_P)    = 1.0-3.0*e1e1;
      P(mrr,place_P+1)  = e2-3.0*e1e1e2;
      P(mrr,place_P+2)  = e2e2-3.0*e1e1e2e2;
      P(mss,place_P+3)  = 1.0-3.0*e2e2;
      P(mss,place_P+4)  = e1-3.0*e1e2e2;
      P(mss,place_P+5)  = e1e1-3.0*e1e1e2e2;
      P(mrs,place_P+6)  = 1.0-3.0*e1e1;
      P(mrs,place_P+7)  = 1.0-3.0*e2e2;
      P(mrs,place_P+8)  = e2-3.0*e1e1e2;
      P(mrs,place_P+9)  = e1-3.0*e1e2e2;
      P(mrs,place_P+10) = 1.0-3.0*(e1e1+e2e2)+9.0*e1e1e2e2;
      place_P+=11;
    break;
    default:
      dserror("eas: BIEGUNG: E11,E12,E22 LINEAR other then 0,9,11");
    break;
  }  
/*----------------------------------------------------------------------
      DICKENRICHTUNG: E33 LINEAR (--> 7P - FORMULIERUNG)
  ----------------------------------------------------------------------*/
  switch(eas[2])
  {
    case 0:
    break;
    case 1:
      P(st,place_P) = 1.0;
      place_P+=1;
    break;
    case 3:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      place_P+=3;
    break;
    case 4:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      P(st,place_P+3) = e1e2;
      place_P+=4;
    break;
    case 6:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      P(st,place_P+3) = e1e2;
      P(st,place_P+4) = e1e1;
      P(st,place_P+5) = e2e2;
      place_P+=6;
    break;
    case 8:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      P(st,place_P+3) = e1e2;
      P(st,place_P+4) = e1e1;
      P(st,place_P+5) = e2e2;
      P(st,place_P+6) = e1e1e2;
      P(st,place_P+7) = e1e2e2;
      place_P+=8;
    break;
    case 9:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      P(st,place_P+3) = e1e2;
      P(st,place_P+4) = 1.0-3.0*e1e1;
      P(st,place_P+5) = 1.0-3.0*e2e2;
      P(st,place_P+6) = e1e1e2;
      P(st,place_P+7) = e1e2e2;
      P(st,place_P+8) = 1.0-9.0*e1e1e2e2;
      place_P+=9;
    break;
    default:
      dserror("eas: DICKENRICHTUNG: E33 LINEAR other then 0,1,3,4,5,8,9");
    break;
  }  
/*----------------------------------------------------------------------
      QUERSCHUB: E13,E23 KONSTANT
  ----------------------------------------------------------------------*/
  switch (eas[3])
  {
    case 0:
    break;
    case 2:
      P(qr,place_P)   = e2-3.0*e1e1e2;
      P(qs,place_P+1) = e1-3.0*e1e2e2;
      place_P+=2;
    break;
    case 4:
      P(qr,place_P)   = 1.0-3.0*e1e1;
      P(qr,place_P+1) = e2-3.0*e1e1e2;
      P(qs,place_P+2) = 1.0-3.0*e2e2;
      P(qs,place_P+3) = e1-3.0*e1e2e2;
      place_P+=4;
    break;
    case 6:
      P(qr,place_P)   = 1.0-3.0*e1e1;
      P(qr,place_P+1) = e2-3.0*e1e1e2;
      P(qr,place_P+2) = e2e2-3.0*e1e1e2e2;
      P(qs,place_P+3) = 1.0-3.0*e2e2;
      P(qs,place_P+4) = e1-3.0*e1e2e2;
      P(qs,place_P+5) = e1e1-3.0*e1e1e2e2;
      place_P+=6;
    break;
    default:
      dserror("eas: QUERSCHUB: E13,E23 KONSTANT other then 0,2,4,6");
    break;
  }  
/*----------------------------------------------------------------------
      QUERSCHUB: E13,E23 LINEAR
  ----------------------------------------------------------------------*/
  switch (eas[4])
  {
    case 0:
    break;
    case 2:
      P(sr,place_P)   = e1e1;
      P(ss,place_P+1) = e2e2;
      place_P+=2;
    break;
    case 4:
      P(sr,place_P)   = e1e1;
      P(sr,place_P+1) = e1e1e2e2;
      P(ss,place_P+2) = e2e2;
      P(ss,place_P+3) = e1e1e2e2;
      place_P+=4;
    break;
    case 6:
      P(sr,place_P)   = e1e1;
      P(sr,place_P+1) = e1e1e2;
      P(sr,place_P+2) = e1e1e2e2;
      P(ss,place_P+3) = e2e2;
      P(ss,place_P+4) = e1e2e2;
      P(ss,place_P+5) = e1e1e2e2;
      place_P+=6;
    break;
    default:
      dserror("eas: QUERSCHUB: E13,E23 LINEAR other then 0,2,4,6");
    break;
  }  

  } // if (iel>4)
/*--------------------------------------------------- four node element */
  else if (iel==4)
  {
/*----------------------------------------------------------------------
      MEMBRAN: E11,E12,E22 KONSTANT
  ----------------------------------------------------------------------*/
  switch (eas[0])
  {
    case 0:
    break;
    case 1:
      P(nss,place_P)=e2;
      place_P+=1;
    break;
    case 2:
      P(nrs,place_P)  =e1;
      P(nrs,place_P+1)=e2;
      place_P+=2;
    break;
    case 3:
      P(nrs,place_P)  =e1;
      P(nrs,place_P+1)=e2;
      P(nrs,place_P+2)=e1e2;
      place_P+=3;
    break;
    case 4:
      P(nrr,place_P)  =e1;
      P(nss,place_P+1)=e2;
      P(nrs,place_P+2)=e1;
      P(nrs,place_P+3)=e2;
      place_P+=4;
    break;
    case 5:
      P(nrr,place_P)  =e1;
      P(nss,place_P+1)=e2;
      P(nrs,place_P+2)=e1;
      P(nrs,place_P+3)=e2;
      P(nrs,place_P+4)=e1e2;
      place_P+=5;
    break;
    case 7:
      P(nrr,place_P)  =e1;
      P(nss,place_P+1)=e2;
      P(nrs,place_P+2)=e1;
      P(nrs,place_P+3)=e2;
      P(nrr,place_P+4)=e1e2;
      P(nss,place_P+5)=e1e2;
      P(nrs,place_P+6)=e1e2;
      place_P+=7;
    break;
    default:
      dserror("eas: MEMBRAN: E11,E12,E22 KONSTANT other then 0,1,2,3,4,5,7");      
    break;
  }
/*----------------------------------------------------------------------
      BIEGUNG: E11,E12,E22 LINEAR
  ----------------------------------------------------------------------*/
  switch (eas[1])
  {
    case 0:
    break;
    case 4:
      P(mrr,place_P)  =e1;
      P(mss,place_P+1)=e2;
      P(mrs,place_P+2)=e1;
      P(mrs,place_P+3)=e2;
      place_P+=4;
    break;
    case 5:
      P(mrr,place_P)  =e1;
      P(mss,place_P+1)=e2;
      P(mrs,place_P+2)=e1;
      P(mrs,place_P+3)=e2;
      P(mrs,place_P+4)=e1e2;
      place_P+=5;
    break;
    case 7:
      P(mrr,place_P)  =e1;
      P(mss,place_P+1)=e2;
      P(mrs,place_P+2)=e1;
      P(mrs,place_P+3)=e2;
      P(mrr,place_P+4)=e1e2;
      P(mss,place_P+5)=e1e2;
      P(mrs,place_P+6)=e1e2;
      place_P+=7;
    break;
    case 6:
      P(mrr,place_P)  =e1e1;
      P(mrr,place_P+1)=e1e1e2e2;
      P(mss,place_P+2)=e2e2;
      P(mss,place_P+3)=e1e1e2e2;
      P(mrs,place_P+4)=e1e1;
      P(mrs,place_P+5)=e2e2;
      place_P+=6;
    break;
    default:
      dserror("eas: BIEGUNG: E11,E12,E22 LINEAR other then 0,4,5,7,8");
    break;
  }
/*----------------------------------------------------------------------
      DICKENRICHTUNG: E33 LINEAR (--> 7P - FORMULIERUNG)
  ----------------------------------------------------------------------*/
  switch (eas[2])
  {
    case 0:
    break;
    case 1:
      P(st,place_P) = 1.0;
      place_P+=1;
    break;
    case 3:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      place_P+=3;
    break;
    case 4:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      P(st,place_P+3) = e1e2;
      place_P+=4;
    break;
    case 6:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      P(st,place_P+3) = e1e2;
      P(st,place_P+4) = e1e1;
      P(st,place_P+5) = e2e2;
      place_P+=6;
    break;
    case 8:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      P(st,place_P+3) = e1e2;
      P(st,place_P+4) = e1e1;
      P(st,place_P+5) = e2e2;
      P(st,place_P+6) = e1e1e2;
      P(st,place_P+7) = e1e2e2;
      place_P+=8;
    break;
    case 9:
      P(st,place_P)   = 1.0;
      P(st,place_P+1) = e1;
      P(st,place_P+2) = e2;
      P(st,place_P+3) = e1e2;
      P(st,place_P+4) = 1.0-3.0*e1e1;
      P(st,place_P+5) = 1.0-3.0*e2e2;
      P(st,place_P+6) = e1e1e2;
      P(st,place_P+7) = e1e2e2;
      P(st,place_P+8) = 1.0-9.0*e1e1e2e2;
      place_P+=9;
    break;
    default:
      dserror("eas: DICKENRICHTUNG: E33 LINEAR other then 0,3,4,6,8,9");
    break;
  }
/*----------------------------------------------------------------------
      QUERSCHUB: E13,E23 KONSTANT
  ----------------------------------------------------------------------*/
  switch (eas[3])
  {
    case 0:
    break;
    case 2:
      P(qr,place_P)  =e1;
      P(qs,place_P+1)=e2;
      place_P+=2;
    break;
    case 4:
      P(qr,place_P)  =e1;
      P(qr,place_P+1)=e1e2;
      P(qs,place_P+2)=e2;
      P(qs,place_P+3)=e1e2;
      place_P+=4;
    break;
    default:
      dserror("eas: QUERSCHUB: E13,E23 KONSTANT other then 0,2,4");
    break;
  }
/*----------------------------------------------------------------------
      QUERSCHUB: E13,E23 LINEAR
  ----------------------------------------------------------------------*/
  switch (eas[4])
  {
    case 0:
    break;
    case 2:
      P(sr,place_P)  =e1;
      P(ss,place_P+1)=e2;
      place_P+=2;
    break;
    case 4:
      P(sr,place_P)  =e1;
      P(sr,place_P+1)=e1e2;
      P(ss,place_P+2)=e2;
      P(ss,place_P+3)=e1e2;
      place_P+=4;
    break;
    default:
      dserror("eas: QUERSCHUB: E13,E23 LINEAR other then 0,2,4");
    break;
  }
  } // else if (iel==4)
  /*------------------------------------------------------------- default */
  else dserror("eas has 8,9 and 4 node elements only");

  if (place_P != nhyb) dserror("wrong parameter nhyb in EAS");

  return;
}

/*----------------------------------------------------------------------*
 |  do ans (private)                                         mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_ansqshapefunctions(
                             double frq[], double fsq[], const double r, 
                             const double s, const int iel, const int nsansq)
{
  DSTraceHelper dst("Shell8::s8_ansqshapefunctions");  
  if (iel==4)
  {
   frq[0] = 0.5 * (1.0 - s);
   frq[1] = 0.5 * (1.0 + s);
   fsq[0] = 0.5 * (1.0 - r);
   fsq[1] = 0.5 * (1.0 + r);
  }
  else if (iel==9)
  {
   const double rthreei = 1.0 / (sqrt(3.0));
   double pr[3],ps[3];
   double qr[2],qs[2];
   double rr[3],rs[3];

   pr[0] = -0.5 * s * (1.0-s);
   pr[1] =  (1.0-s) * (1.0+s);
   pr[2] =  0.5 * s * (1.0+s);

   qr[0] =  0.5 * (1.0-r/rthreei);
   qr[1] =  0.5 * (1.0+r/rthreei);

   rr[0] =  1.0/6.0 - 0.5 * s;
   rr[1] =  2.0/3.0;
   rr[2] =  1.0/6.0 + 0.5 * s;

   ps[0] = -0.5 * r * (1.0-r);
   ps[1] =  (1.0-r) * (1.0+r);
   ps[2] =  0.5 * r * (1.0+r);

   qs[0] =  0.5 * (1.0-s/rthreei);
   qs[1] =  0.5 * (1.0+s/rthreei);

   rs[0] =  1.0/6.0 - 0.5 * r;
   rs[1] =  2.0/3.0;
   rs[2] =  1.0/6.0 + 0.5 * r;

   frq[0] = pr[0] * qr[0];
   frq[1] = pr[1] * qr[0];
   frq[2] = pr[2] * qr[0];
   frq[3] = pr[0] * qr[1];
   frq[4] = pr[1] * qr[1];
   frq[5] = pr[2] * qr[1];

   fsq[0] = ps[0] * qs[0];
   fsq[1] = ps[1] * qs[0];
   fsq[2] = ps[2] * qs[0];
   fsq[3] = ps[0] * qs[1];
   fsq[4] = ps[1] * qs[1];
   fsq[5] = ps[2] * qs[1];
  }
  return;
}

/*----------------------------------------------------------------------*
 |  do ans (private)                                         mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_ans_colloquationpoints(
                                 const int nsansq, const int iel, const int ans,
                                 double xr1[], double xs1[], double xr2[], double xs2[],
                                 vector<double>                  funct1q[],
                                 Epetra_SerialDenseMatrix        deriv1q[],
                                 vector<double>                  funct2q[],
                                 Epetra_SerialDenseMatrix        deriv2q[],
                                 const double xrefe[][MAXNOD_SHELL8],
                                 const double a3r[][MAXNOD_SHELL8],
                                 const double xcure[][MAXNOD_SHELL8],
                                 const double a3c[][MAXNOD_SHELL8],
                                 double       akovr1q[][3][3],
                                 double       akonr1q[][3][3],
                                 double       amkovr1q[][3][3],
                                 double       amkonr1q[][3][3],
                                 double       a3kvpr1q[][3][2],
                                 double       akovc1q[][3][3],
                                 double       akonc1q[][3][3],
                                 double       amkovc1q[][3][3],
                                 double       amkonc1q[][3][3],
                                 double       a3kvpc1q[][3][2],
                                 double       akovr2q[][3][3],
                                 double       akonr2q[][3][3],
                                 double       amkovr2q[][3][3],
                                 double       amkonr2q[][3][3],
                                 double       a3kvpr2q[][3][2],
                                 double       akovc2q[][3][3],
                                 double       akonc2q[][3][3],
                                 double       amkovc2q[][3][3],
                                 double       amkonc2q[][3][3],
                                 double       a3kvpc2q[][3][2],
                                 double* detr, double* detc)
{
  DSTraceHelper dst("Shell8::s8_ans_colloquationpoints");  
  
  /*------------------------------- get coordinates of collocation points */
  s8_ans_colloquationcoords(xr1,xs1,xr2,xs2,iel,ans);
  
  for (int i=0; i<nsansq; ++i)
  {
    s8_shapefunctions(funct1q[i],deriv1q[i],xr1[i],xs1[i],iel,1);
    s8tvmr(xrefe,a3r,akovr1q[i],akonr1q[i],amkovr1q[i],amkonr1q[i],detr,
           funct1q[i],deriv1q[i],iel,a3kvpr1q[i],0);
    s8tvmr(xcure,a3c,akovc1q[i],akonc1q[i],amkovc1q[i],amkonc1q[i],detc,
           funct1q[i],deriv1q[i],iel,a3kvpc1q[i],0);
    
    s8_shapefunctions(funct2q[i],deriv2q[i],xr2[i],xs2[i],iel,1);
    s8tvmr(xrefe,a3r,akovr2q[i],akonr2q[i],amkovr2q[i],amkonr2q[i],detr,
           funct2q[i],deriv2q[i],iel,a3kvpr2q[i],0);
    s8tvmr(xcure,a3c,akovc2q[i],akonc2q[i],amkovc2q[i],amkonc2q[i],detc,
           funct2q[i],deriv2q[i],iel,a3kvpc2q[i],0);
  }
  
  return;
}

/*----------------------------------------------------------------------*
 |  do metric (private)                                      mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8tmtr(const double x[][MAXNOD_SHELL8], 
                                   const double a3[][MAXNOD_SHELL8],
                                   const double e3, 
                                   double gkov[][3],
                                   double gkon[][3],
                                   double gmkov[][3],
                                   double gmkon[][3],
                                   double* det,
                                   const vector<double>& funct,
                                   const Epetra_SerialDenseMatrix& deriv,
                                   const int iel,
                                   const double condfac,
                                   const int flag)
{
  DSTraceHelper dst("Shell8::s8tmtr");  

  /*---------------------------------------------------- sdc-conditioning */
  const double zeta = e3/condfac;
  /*------------------------------------ interpolation of kovariant g1,g2 */
  for (int ialpha=0; ialpha<2; ialpha++)
    for (int idim=0; idim<3; idim++)
    {
      gkov[idim][ialpha]=0.0;
      for (int inode=0; inode<iel; inode++)
        gkov[idim][ialpha] += deriv(ialpha,inode)*(x[idim][inode]+zeta*a3[idim][inode]);
    }
  /*------------------------------------------------- interpolation of g3 */
  for (int idim=0; idim<3; idim++)
  {
    gkov[idim][2]=0.0;
    for (int inode=0; inode<iel; inode++)
      gkov[idim][2] += funct[inode] * a3[idim][inode];
  }
  /*--------------- kontravariant basis vectors g1,g2,g3 (inverse of kov) */
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) gkon[i][j] = gkov[i][j];
  s8inv3(gkon,det);
  s8trans3(gkon);
  /*--------------------------------------------- kovariant metrik tensor */
  for (int i=0; i<3; i++)
    for (int j=i; j<3; j++)
    {
      gmkov[i][j]=0.0;
      for (int k=0; k<3; k++)
        gmkov[i][j] += gkov[k][i]*gkov[k][j];
    }
  gmkov[1][0] = gmkov[0][1];
  gmkov[2][0] = gmkov[0][2];
  gmkov[2][1] = gmkov[1][2];
  /*----------------------------------------- kontravariant metrik tensor */
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) gmkon[i][j] = gmkov[i][j];
  double dummy;
  s8inv3(gmkon,&dummy);

  return;
}

/*----------------------------------------------------------------------*
 |  do Jacobian (private)                                    mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_jaco(const vector<double>& funct,
                                    const Epetra_SerialDenseMatrix& deriv,
                                    const double x[][MAXNOD_SHELL8],
                                    double xjm[][3],
                                    const vector<double>& hte,
                                    const double a3ref[][MAXNOD_SHELL8],
                                    const double e3,
                                    const int iel,
                                    double* det,
                                    double* deta)
{
  DSTraceHelper dst("Shell8::s8_jaco");  
  double gkov[3][3];
  double gkon[3][3];
  double gmkov[3][3];
  double gmkon[3][3];
  s8tmtr(x,a3ref,e3,gkov,gkon,gmkov,gmkon,det,funct,deriv,iel,1.0,0);
  xjm[0][0]=gkov[0][0];
  xjm[0][1]=gkov[1][0];
  xjm[0][2]=gkov[2][0];
  xjm[1][0]=gkov[0][1];
  xjm[1][1]=gkov[1][1];
  xjm[1][2]=gkov[2][1];
  xjm[2][0]=gkov[0][2];
  xjm[2][1]=gkov[1][2];
  xjm[2][2]=gkov[2][2];
  const double x1r=xjm[0][0];
  const double x2r=xjm[0][1];
  const double x3r=xjm[0][2];
  const double x1s=xjm[1][0];
  const double x2s=xjm[1][1];
  const double x3s=xjm[1][2];

  *deta = DSQR(x1r*x2s - x2r*x1s) + DSQR(x3r*x1s - x3s*x1r) + DSQR(x2r*x3s - x3r*x2s);
  *deta = sqrt(*deta);
  if (*deta <= 1.0e-14) dserror("Element Area equal 0.0 or negativ detected");
  
  return;
}

  
/*----------------------------------------------------------------------*
 |  do metric (private)                                      mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8tvmr(const double x[][MAXNOD_SHELL8], 
                                   const double a3[][MAXNOD_SHELL8], 
                                   double akov[][3],
                                   double akon[][3],
                                   double amkov[][3],
                                   double amkon[][3],
                                   double* det,
                                   const vector<double>& funct,
                                   const Epetra_SerialDenseMatrix& deriv,
                                   const int iel,
                                   double a3kvp[][2],
                                   const int flag)
{
  DSTraceHelper dst("Shell8::s8tvmr");  

  /*------------------------------------ interpolation of kovariant a1,a2 */
  for (int ialpha=0; ialpha<2; ialpha++)
  {
     for (int idim=0; idim<3; idim++)
     {
        akov[idim][ialpha]=0.0;
        for (int inode=0; inode<iel; inode++)
           akov[idim][ialpha] +=
              deriv(ialpha,inode) * x[idim][inode];
     }
  }
  /*------------------------------------------------- interpolation of a3 */
  for (int idim=0; idim<3; idim++)
  {
     akov[idim][2]=0.0;
     for (int inode=0; inode<iel; inode++)
        akov[idim][2] += funct[inode] * a3[idim][inode];
  }
  /*--------------- kontravariant basis vectors g1,g2,g3 (inverse of kov) */
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) akon[i][j] = akov[i][j];
  s8inv3(akon,det);
  s8trans3(akon);
  /*--------------------------------------------- kovariant metrik tensor */
  for (int i=0; i<3; i++)
    for (int j=i; j<3; j++)
    {
      amkov[i][j]=0.0;
      for (int k=0; k<3; k++)
        amkov[i][j] += akov[k][i]*akov[k][j];
    }
  amkov[1][0] = amkov[0][1];
  amkov[2][0] = amkov[0][2];
  amkov[2][1] = amkov[1][2];
  /*----------------------------------------- kontravariant metrik tensor */
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) amkon[i][j] = amkov[i][j];
  double dummy;
  s8inv3(amkon,&dummy);
  /*------------------------------------------- partial derivatives of a3 */
  for (int ialpha=0; ialpha<2; ialpha++)
    for (int idim=0; idim<3; idim++)
    {
      a3kvp[idim][ialpha]=0.0;
      for (int inode=0; inode<iel; inode++)
         a3kvp[idim][ialpha] += deriv(ialpha,inode)*a3[idim][inode];
    }

  return;
}
  
/*----------------------------------------------------------------------*
 |  do ans (private)                                         mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_ans_colloquationcoords(double xqr1[], double xqs1[], 
                                                      double xqr2[], double xqs2[],
                                                      const int iel, const int ans)
{
  DSTraceHelper dst("Shell8::s8_ans_colloquationcoords");  
  if (ans==1) // ans fuer querschublocking
  {
    if (iel==4)
    {
      xqr1[0] =  0.0;     xqs1[0] = -1.0; /* point ( 0.0/-1.0) */
      xqr1[1] =  0.0;     xqs1[1] =  1.0; /* point ( 0.0/ 1.0) */

      xqr2[0] = -1.0;     xqs2[0] =  0.0; /* point (-1.0/ 0.0) */
      xqr2[1] =  1.0;     xqs2[1] =  0.0; /* point ( 1.0/ 0.0) */
    }
    else if (iel==9)
    {
      const double rthreei = 1.0 / (sqrt(3.0));
      xqr1[0] = -rthreei; xqs1[0] = -1.0;
      xqr1[1] = -rthreei; xqs1[1] =  0.0;
      xqr1[2] = -rthreei; xqs1[2] =  1.0;
      xqr1[3] =  rthreei; xqs1[3] = -1.0;
      xqr1[4] =  rthreei; xqs1[4] =  0.0;
      xqr1[5] =  rthreei; xqs1[5] =  1.0;

      xqr2[0] = -1.0;     xqs2[0] = -rthreei;
      xqr2[1] = 0.0;     xqs2[1] =  -rthreei;
      xqr2[2] =  1.0;     xqs2[2] = -rthreei;
      xqr2[3] = -1.0;     xqs2[3] =  rthreei;
      xqr2[4] =  0.0;     xqs2[4] =  rthreei;
      xqr2[5] =  1.0;     xqs2[5] =  rthreei;
    }
  }
  
  
  return;
}


/*----------------------------------------------------------------------*
 |  R[i][j] = A[i][k]*B[k][j] -----  R=A*B                   m.gee12/01 |
 | if init==0 result is inited to 0.0                                   |
 |        !=0 result is added  to R multiplied by factor                |
 |  R[i][j] += A[i][k]*B[k][j]*factor                                   |
 |                                                                      |
 | important: this routine works only with arrays that are dynamically  |
 |            allocated the way the AM routines do it                   |
 |            (you can use BLAS as well for this, but you have to care  |
               for the fact, that rows and columns are changed by BLAS )|
 |                                                                      |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8matmatdense(Epetra_SerialDenseMatrix& R, 
                                          const Epetra_SerialDenseMatrix& A,
                                          const Epetra_SerialDenseMatrix& B,
                                          const int ni,
                                          const int nk,
                                          const int nj,
                                          const int init,
                                          const double factor)
{
  if (!init)
  {
    for (int i=0; i<ni; ++i)
      for (int j=0; j<nj; ++j)
      {
        double sum=0.0;
        for (int k=0; k<nk; ++k) sum += A(i,k)*B(k,j);
        R(i,j) = sum;
      }
  }
  else
  {
    for (int i=0; i<ni; ++i)
      for (int j=0; j<nj; ++j)
      {
        double sum=0.0;
        for (int k=0; k<nk; ++k) sum += A(i,k)*B(k,j);
        R(i,j) += sum*factor;
      }
  }
  return;
}                    
/*----------------------------------------------------------------------*
 |  R[i][j] = A[i][k]*B[k][j] -----  R=A*B                   m.gee12/01 |
 | if init==0 result is inited to 0.0                                   |
 |        !=0 result is added  to R multiplied by factor                |
 |  R[i][j] += A[i][k]*B[k][j]*factor                                   |
 |                                                                      |
 | important: this routine works only with arrays that are dynamically  |
 |            allocated the way the AM routines do it                   |
 |            (you can use BLAS as well for this, but you have to care  |
               for the fact, that rows and columns are changed by BLAS )|
 |                                                                      |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8matmatdense(Epetra_SerialDenseMatrix& R, 
                                          const double A[][12],
                                          const Epetra_SerialDenseMatrix& B,
                                          const int ni,
                                          const int nk,
                                          const int nj,
                                          const int init,
                                          const double factor)
{
  if (!init)
  {
    for (int i=0; i<ni; ++i)
      for (int j=0; j<nj; ++j)
      {
        double sum=0.0;
        for (int k=0; k<nk; ++k) sum += A[i][k]*B(k,j);
        R(i,j) = sum;
      }
  }
  else
  {
    for (int i=0; i<ni; ++i)
      for (int j=0; j<nj; ++j)
      {
        double sum=0.0;
        for (int k=0; k<nk; ++k) sum += A[i][k]*B(k,j);
        R(i,j) += sum*factor;
      }
  }
  return;
}                    
/*----------------------------------------------------------------------*
 |  R[i][j] = A[k][i]*B[k][j] -----  R=A^t * B               m.gee 7/01 |
 | if init==0 result is inited to 0.0                                   |
 |        !=0 result is added  to R multiplied by factor                |
 |  R[i][j] += A[k][i]*B[k][j]*factor                                   |
 |                                                                      |
 | important: this routine works only with arrays that are dynamically  |
 |            allocated the way the AM routines do it                   |
 |            (you can use BLAS as well for this, but you have to care  |
               for the fact, that rows and columns are changed by BLAS )|
 |                                                                      |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8mattrnmatdense(Epetra_SerialDenseMatrix& R,
                                             const Epetra_SerialDenseMatrix& A,
                                             const Epetra_SerialDenseMatrix& B,
                                             const int ni,
                                             const int nk,
                                             const int nj,
                                             const int init,
                                             const double factor)
{
  if (!init)
  {
    for (int i=0; i<ni; ++i)
      for (int j=0; j<nj; ++j)
      {
        double sum=0.0;
        for (int k=0; k<nk; ++k) sum += A(k,i)*B(k,j);
        R(i,j) = sum;
      }
  }
  else
  {
    for (int i=0; i<ni; ++i)
      for (int j=0; j<nj; ++j)
      {
        double sum=0.0;
        for (int k=0; k<nk; ++k) sum += A(k,i)*B(k,j);
        R(i,j) += sum*factor;
      }
  }
  return;
}                    

/*----------------------------------------------------------------------*
 |  r(I) = A(K,I)*b(K) -----  r = A*b                        m.gee 6/01 |
 |  or                                                                  |
 |  r(I) += A(K,I)*b(K)*factor                                          |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8mattrnvecdense(
                        vector<double>& r,
                        const Epetra_SerialDenseMatrix& A, 
                        const double b[],
                        const int ni,
                        const int nk,
                        const int init,
                        const double factor)
{
  if (!init)
    for (int i=0; i<ni; ++i) r[i] = 0.0;
  for (int i=0; i<ni; ++i)
  {
    double sum=0.0;
    for (int k=0; k<nk; ++k) sum += A(k,i)*b[k];
    r[i] += sum*factor;
  }
  return;
}                    


/*----------------------------------------------------------------------*
 |  y(I) = A(I,K)*x(K)*factor -----  y = A*x*factor         m.gee 12/06 |
 |  or                                                                  |
 |  y(I) += A(I,K)*x(K)*factor                                          |
 | (private)                                                            |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_YpluseqAx(Epetra_SerialDenseVector& y,
                                         const Epetra_SerialDenseMatrix& A,
                                         const vector<double>& x, 
                                         const double factor, 
                                         const bool init)
{
  const int rdim = (int)y.Length();
  const int ddim = (int)x.size();
  if (A.M()<rdim || A.N()<ddim) dserror("Mismatch in dimensions");
  
  if (init)
    for (int i=0; i<rdim; ++i) y[i] = 0.0;
  for (int i=0; i<rdim; ++i)
  {
    double sum = 0.0;
    for (int k=0; k<ddim; ++k) sum += A(i,k)*x[k];
    y[i] += sum*factor;
  }
  return;
}                    

/*----------------------------------------------------------------------*
 |  y(I) = A(I,K)*x(K)*factor -----  y = A*x*factor         m.gee 12/06 |
 |  or                                                                  |
 |  y(I) += A(I,K)*x(K)*factor                                          |
 | (private)                                                            |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_YpluseqAx(vector<double>& y,
                                         const Epetra_SerialDenseMatrix& A,
                                         const vector<double>& x, 
                                         const double factor, 
                                         const bool init)
{
  const int rdim = (int)y.size();
  const int ddim = (int)x.size();
  if (A.M()<rdim || A.N()<ddim) dserror("Mismatch in dimensions");
  
  if (init)
    for (int i=0; i<rdim; ++i) y[i] = 0.0;
  for (int i=0; i<rdim; ++i)
  {
    double sum = 0.0;
    for (int k=0; k<ddim; ++k) sum += A(i,k)*x[k];
    y[i] += sum*factor;
  }
  return;
}                    


/*----------------------------------------------------------------------*
 |  calcs the inverse of an unsym 3x3 matrix and determinant m.gee12/06 |
 | (private)                                                            |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8inv3(double a[][3], double* det)
{
  const double b00 = a[0][0];
  const double b01 = a[0][1];
  const double b02 = a[0][2];
  const double b10 = a[1][0];
  const double b11 = a[1][1];
  const double b12 = a[1][2];
  const double b20 = a[2][0];
  const double b21 = a[2][1];
  const double b22 = a[2][2];
  
  a[0][0] =   b11*b22 - b21*b12;
  a[1][0] = - b10*b22 + b20*b12;
  a[2][0] =   b10*b21 - b20*b11;
  a[0][1] = - b01*b22 + b21*b02;
  a[1][1] =   b00*b22 - b20*b02;
  a[2][1] = - b00*b21 + b20*b01;
  a[0][2] =   b01*b12 - b11*b02;
  a[1][2] = - b00*b12 + b10*b02;
  a[2][2] =   b00*b11 - b10*b01;
  
  *det = b00*a[0][0]+b01*a[1][0]+b02*a[2][0];
  const double detinv = 1.0/(*det);
  
  for (int i=0; i<3; ++i)
  for (int j=0; j<3; ++j) a[i][j] *= detinv;
  
  return;
}                    

/*----------------------------------------------------------------------*
 |  calcs the inverse of an unsym 3x3 matrix and determinant m.gee12/06 |
 | (private)                                                            |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8trans3(double a[][3])
{
  for (int i=0; i<3; ++i)
    for (int j=i+1; j<3; ++j)
    {
      const double change = a[j][i];
      a[j][i] = a[i][j];
      a[i][j] = change;
    }
  return;
}                    

/*----------------------------------------------------------------------*
 |  make a vector unit lenght and return orig lenght         m.gee12/06 |
 | (private)                                                            |
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8unvc(double* enorm, double vec[], const int n)
{
  double skalar = 0.0;
  for (int i=0; i<n; ++i) skalar += vec[i]*vec[i];
  *enorm = sqrt(skalar);
  if (*enorm<1.e-13) dserror("Vector of lenght < EPS13 appeared");
  for (int i=0; i<n; ++i) vec[i] /= (*enorm);
  return;
}                    

/*----------------------------------------------------------------------*
 |  evaluate the element integration points (private)        mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_integration_points(struct _S8_DATA& data)
{
  DSTraceHelper dst("Shell8::s8_integration_points");  
  
  const int numnode = NumNode();

  const double invsqrtthree = 1./sqrt(3.);
  const double sqrtthreeinvfive = sqrt(3./5.);
  const double wgt  = 5.0/9.0;
  const double wgt0 = 8.0/9.0;
  
  switch(ngp_[2])/*---------------- thickness direction t */
  {
    case 2:
      data.xgpt[0] = -invsqrtthree;
      data.xgpt[1] = invsqrtthree;
      data.xgpt[2] = 0.0;
      data.wgtt[0] =  1.0;
      data.wgtt[1] =  1.0;
      data.wgtt[2] =  0.0;
    break;
    default:
      dserror("Unknown no. of gaussian points in thickness direction");
    break;
  }
  
  // quad elements
  if (numnode==4 || numnode==8 || numnode==9)
  {
    switch(ngp_[0]) // r direction
    {
      case 1:
        data.xgpr[0] = 0.0;
        data.xgpr[1] = 0.0;
        data.xgpr[2] = 0.0;
        data.wgtr[0] = 2.0;
        data.wgtr[1] = 0.0;
        data.wgtr[2] = 0.0;
      break;
      case 2:
        data.xgpr[0] = -invsqrtthree;
        data.xgpr[1] =  invsqrtthree;
        data.xgpr[2] =  0.0;
        data.wgtr[0] =  1.0;
        data.wgtr[1] =  1.0;
        data.wgtr[2] =  0.0;
      break;
      case 3:
        data.xgpr[0] = -sqrtthreeinvfive;
        data.xgpr[1] =  0.0;
        data.xgpr[2] =  sqrtthreeinvfive;
        data.wgtr[0] =  wgt;
        data.wgtr[1] =  wgt0;
        data.wgtr[2] =  wgt;
      break;
      default:
        dserror("Unknown no. of gaussian points in r-direction");
      break;
    } // switch(ngp_[0]) // r direction
    
    switch(ngp_[1]) // s direction
    {
      case 1:
        data.xgps[0] = 0.0;
        data.xgps[1] = 0.0;
        data.xgps[2] = 0.0;
        data.wgts[0] = 2.0;
        data.wgts[1] = 0.0;
        data.wgts[2] = 0.0;
      break;
      case 2:
        data.xgps[0] = -invsqrtthree;
        data.xgps[1] =  invsqrtthree;
        data.xgps[2] =  0.0;
        data.wgts[0] =  1.0;
        data.wgts[1] =  1.0;
        data.wgts[2] =  0.0;
      break;
      case 3:
        data.xgps[0] = -sqrtthreeinvfive;
        data.xgps[1] =  0.0;
        data.xgps[2] =  sqrtthreeinvfive;
        data.wgts[0] =  wgt;
        data.wgts[1] =  wgt0;
        data.wgts[2] =  wgt;
      break;
      default:
        dserror("Unknown no. of gaussian points in s-direction");
      break;
    } // switch(ngp_[0]) // s direction
    
  } // if (numnode==4 || numnode==8 || numnode==9)

  else if (numnode==3 || numnode==6) // triangle elements
  {
    switch(ngptri_)
    {
      case 1:
      {
        const double third = 1.0/3.0;
        data.xgpr[0] =  third;
        data.xgpr[1] =  0.0;
        data.xgpr[2] =  0.0;
        data.xgps[0] =  third;
        data.xgps[1] =  0.0;
        data.xgps[2] =  0.0;
        data.wgtr[0] =  0.5;
        data.wgtr[1] =  0.0;
        data.wgtr[2] =  0.0;
        data.wgts[0] =  0.5;
        data.wgts[1] =  0.0;
        data.wgts[2] =  0.0;
      }
      break;
      case 3:
      {
        const double wgt = 1.0/6.0;
        data.xgpr[0] =  0.5;
        data.xgpr[1] =  0.5;
        data.xgpr[2] =  0.0;
        data.xgps[0] =  0.0;
        data.xgps[1] =  0.5;
        data.xgps[2] =  0.5;
        data.wgtr[0] =  wgt;
        data.wgtr[1] =  wgt;
        data.wgtr[2] =  wgt;
        data.wgts[0] =  wgt;
        data.wgts[1] =  wgt;
        data.wgts[2] =  wgt;
      }
      break;
      default:
        dserror("Unknown no. of gaussian points for triangle");
      break;
    } 
  } // else if (numnode==3 || numnode==6)
  
  return;
}


/*----------------------------------------------------------------------*
 |  local coords of nodal point (private)                    mwgee 12/06|
 *----------------------------------------------------------------------*/
const double DRT::Elements::Shell8::s8_localcoordsofnode(const int node, const int flag,
                                                         const int numnode) const
{
  DSTraceHelper dst("Shell8::s8_localcoordsofnode");  
  const double node489[9][2] = {{1.0,1.0},{-1.0,1.0},{-1.0,-1.0},{1.0,-1.0},
                                {0.0,1.0},{-1.0,0.0},{0.0,-1.0},{1.0,0.0},{0.0,0.0}};
  
  switch (numnode)
  {
  case 4:
  case 8:
  case 9:
    return node489[node][flag];
  break;
  default:
    dserror("Unknown no. of nodal points to element");
  break;
  }
  
  
  return 0.0;
}


/*----------------------------------------------------------------------*
 |  shape functions and derivatives (private)                mwgee 12/06|
 *----------------------------------------------------------------------*/
void DRT::Elements::Shell8::s8_shapefunctions(
                             vector<double>& funct, 
                             Epetra_SerialDenseMatrix& deriv,
                             const double r, const double s, const int numnode, 
                             const int doderiv) const
{
  DSTraceHelper dst("Shell8::s8_shapefunctions");  
  
  const double q12 = 0.5;
  const double q14 = 0.25;
  const double rr = r*r;
  const double ss = s*s;
  const double rp = 1.0+r;
  const double rm = 1.0-r;
  const double sp = 1.0+s;
  const double sm = 1.0-s;
  const double r2 = 1.0-rr;
  const double s2 = 1.0-ss;
  
  switch(numnode)
  {
    case 4:
    {
      funct[0] = q14*rp*sp;
      funct[1] = q14*rm*sp;
      funct[2] = q14*rm*sm;
      funct[3] = q14*rp*sm;
      if (doderiv)
      {
        deriv(0,0)= q14*sp;
        deriv(0,1)=-q14*sp;
        deriv(0,2)=-q14*sm;
        deriv(0,3)= q14*sm;
        deriv(1,0)= q14*rp;
        deriv(1,1)= q14*rm;
        deriv(1,2)=-q14*rm;
        deriv(1,3)=-q14*rp;
      }
      return;
    }
    break;
    case 8:
    {
      funct[0] = -q14*(1-r)*(1-s)*(1+r+s);
      funct[1] = -q14*(1+r)*(1-s)*(1-r+s);
      funct[2] = -q14*(1+r)*(1+s)*(1-r-s);
      funct[3] = -q14*(1-r)*(1+s)*(1+r-s);
      funct[4] =  q12*(1-r*r)*(1-s);
      funct[5] =  q12*(1+r)*(1-s*s);
      funct[6] =  q12*(1-r*r)*(1+s);
      funct[7] =  q12*(1-r)*(1-s*s);
      if (doderiv)
      {
        deriv(0,0)=  q14*(1-s)*(2*r+s);
        deriv(0,1)=  q14*(1-s)*(2*r-s);
        deriv(0,2)=  q14*(1+s)*(2*r+s);
        deriv(0,3)=  q14*(1+s)*(2*r-s);
        deriv(0,4)= -r*(1-s);
        deriv(0,5)=  q12*(1-s*s);
        deriv(0,6)= -r*(1+s);
        deriv(0,7)= -q12*(1-s*s);
        deriv(1,0)=  q14*(1-r)*(r+2*s);
        deriv(1,1)=  q14*(1+r)*(-r+2*s);
        deriv(1,2)=  q14*(1+r)*(r+2*s);
        deriv(1,3)=  q14*(1-r)*(-r+2*s);
        deriv(1,4)= -q12*(1-r*r);
        deriv(1,5)= -s*(1+r);
        deriv(1,6)=  q12*(1-r*r);
        deriv(1,7)= -s*(1-r);
      }
      return;
    }
    break;
    case 9:
    {
      const double rh  = q12*r;
      const double sh  = q12*s;
      const double rs  = rh*sh;
      const double rhp = r+q12;
      const double rhm = r-q12;
      const double shp = s+q12;
      const double shm = s-q12;
      funct[0] = rs*rp*sp;
      funct[1] =-rs*rm*sp;
      funct[2] = rs*rm*sm;
      funct[3] =-rs*rp*sm;
      funct[4] = sh*sp*r2;
      funct[5] =-rh*rm*s2;
      funct[6] =-sh*sm*r2;
      funct[7] = rh*rp*s2;
      funct[8] = r2*s2;
      if (doderiv==1)
      {
        deriv(0,0)= rhp*sh*sp;
        deriv(0,1)= rhm*sh*sp;
        deriv(0,2)=-rhm*sh*sm;
        deriv(0,3)=-rhp*sh*sm;
        deriv(0,4)=-2.0*r*sh*sp;
        deriv(0,5)= rhm*s2;
        deriv(0,6)= 2.0*r*sh*sm;
        deriv(0,7)= rhp*s2;
        deriv(0,8)=-2.0*r*s2;
        deriv(1,0)= shp*rh*rp;
        deriv(1,1)=-shp*rh*rm;
        deriv(1,2)=-shm*rh*rm;
        deriv(1,3)= shm*rh*rp;
        deriv(1,4)= shp*r2;
        deriv(1,5)= 2.0*s*rh*rm;
        deriv(1,6)= shm*r2;
        deriv(1,7)=-2.0*s*rh*rp;
        deriv(1,8)=-2.0*s*r2;
      }
      return;
    }
    break;
    case 3:
    {
      funct[0]=1-r-s;
      funct[1]=r;
      funct[2]=s;
      if (doderiv==1)
      {
        deriv(0,0)=  -1.0;
        deriv(0,1)=  1.0;
        deriv(0,2)=  0.0;
        deriv(1,0)=  -1.0;
        deriv(1,1)=  0.0;
        deriv(1,2)=  1.0;
      }
    }
    break;
    case 6:
      funct[0]=(1-2*r-2*s)*(1-r-s);
      funct[1]=2*r*r-r;
      funct[2]=2*s*s-s;
      funct[3]=4*(r-r*r-r*s);
      funct[4]=4*r*s;
      funct[5]=4*(s-s*s-s*r);
      if (doderiv==1)
      {
        deriv(0,0)= -3.0+4.0*r+4.0*s;
        deriv(0,1)= 4.0*r-1.0;
        deriv(0,2)= 0.0;
        deriv(0,3)= 4.0*(1-2.0*r-s);
        deriv(0,4)= 4.0*s;
        deriv(0,5)= -4.0*s;
        deriv(1,0)= -3.0+4.0*r+4.0*s;
        deriv(1,1)= 0.0;
        deriv(1,2)= 4.0*s-1.0;
        deriv(1,3)= -4.0*r;
        deriv(1,4)= 4.0*r;
        deriv(1,5)= 4.0*(1.0-2.0*s-r);
      }
    break;
    default:
      dserror("Unknown no. of nodes %d to shell8 element",numnode);
    break;
  }
  return;
}




/*----------------------------------------------------------------------*
 |  calculate shell surface loads at gaussian point (private)mwgee 01/07|
 *----------------------------------------------------------------------*/
void s8loadgaussianpoint(double eload[][MAXNOD_SHELL8], const double hhi,
                         double wgt, const double xjm[][3], 
                         const vector<double>& funct,
                         const Epetra_SerialDenseMatrix& deriv,
                         const int iel, 
                         const double xi,
                         const double yi,
                         const double zi,
                         const enum LoadType ltype,
                         const vector<int>& onoff,
                         const vector<double>& val,
                         const double curvefac,
                         const double time)
{
  DSTraceHelper dst("s8loadgaussianpoint");

/*----------------------------------------------------------------------*/
/*------------------------------ evaluate components of angle of normal */
/*        xjm = J = (g1 g2 g3) siehe Dissertation Braun Kap. Grundlagen */
/*--------- the lenght of the vector ap (which is g3) is det(J) is |g3| */
  double ap[3];
  ap[0] = xjm[0][1]*xjm[1][2] - xjm[1][1]*xjm[0][2];
  ap[1] = xjm[0][2]*xjm[1][0] - xjm[1][2]*xjm[0][0];
  ap[2] = xjm[0][0]*xjm[1][1] - xjm[1][0]*xjm[0][1];
  double ar[3];
  switch(ltype)
  {
    case neum_live: // uniform load on reference configuration
    case neum_live_FSI:
    {
      double ar[3];
      ar[0]=ar[1]=ar[2]= sqrt( ap[0]*ap[0] + ap[1]*ap[1] + ap[2]*ap[2] );
      for (int i=0; i<3; ++i)
        ar[i] = ar[i] * wgt * onoff[i] * val[i] * curvefac;
      for (int i=0; i<iel; ++i)
        for (int j=0; j<3; ++j)
          eload[j][i] += funct[i]*ar[j];
    }
    break;
    // hydrostatic pressure dependent on z-coordinate of gaussian point
    case neum_consthydro_z: 
    {
      if (onoff[2] != 1) dserror("hydropressure must be on third dof");
      ar[0] = ap[0] * val[2] * wgt * curvefac;
      ar[1] = ap[1] * val[2] * wgt * curvefac;
      ar[2] = ap[2] * val[2] * wgt * curvefac;
      for (int i=0; i<iel; ++i)
        for (int j=0; j<3; ++j)
          eload[j][i] += funct[i]*ar[j];
    }
    break;
    // hydrostat pressure dep. on z-coord of gp increasing with time in height
    case neum_increhydro_z:
    {
      if (onoff[2] != 1) dserror("hydropressure must be on third dof");
      double height = time * 10.0;
      double pressure = 0.0;
      if (zi<=height)
        pressure = -val[2]*(height-zi);
      ar[0] = ap[0] * pressure * wgt;
      ar[1] = ap[1] * pressure * wgt;
      ar[2] = ap[2] * pressure * wgt;
      for (int i=0; i<iel; ++i)
        for (int j=0; j<3; ++j)
          eload[j][i] += funct[i]*ar[j];
    }
    break;
    // orthogonal pressure dep. on load curve only  
    case neum_orthopressure:
    case neum_opres_FSI:
    {
      if (onoff[2] != 1) dserror("orthopressure must be on third dof");
      double pressure = -val[2]*curvefac;
      ar[0] = ap[0] * pressure * wgt;
      ar[1] = ap[1] * pressure * wgt;
      ar[2] = ap[2] * pressure * wgt;
      for (int i=0; i<iel; ++i)
        for (int j=0; j<3; ++j)
          eload[j][i] += funct[i]*ar[j];
    }
    break;
    default:
      dserror("Unknown type of SurfaceNeumann load");
    break;
  }
  return;
}


//=======================================================================
//=======================================================================
//=======================================================================
//=======================================================================
static void s8_averagedirector(Epetra_SerialDenseMatrix& dir_list, 
                               const int numa3, double a3[]);

/*----------------------------------------------------------------------*
 |  init the element (public)                                mwgee 12/06|
 *----------------------------------------------------------------------*/
int DRT::Elements::Shell8Register::Initialize(DRT::Discretization& dis)
{
  DSTraceHelper dst("Shell8Register::Initialize");

  //-------------------- loop all my column elements and init directors at nodes
  for (int i=0; i<dis.NumMyColElements(); ++i)
  {
    if (dis.lColElement(i)->Type() != DRT::Element::element_shell8) continue;
    DRT::Elements::Shell8* actele = dynamic_cast<DRT::Elements::Shell8*>(dis.lColElement(i));
    if (!actele) dserror("cast to Shell8* failed");

    const int numnode = actele->NumNode();

    // create matrix a3ref
    Epetra_SerialDenseMatrix tmpmatrix(3,numnode);
    actele->data_.Add("a3ref",tmpmatrix);
    Epetra_SerialDenseMatrix* a3ref = actele->data_.GetMatrix("a3ref");
    
    // create vector thick
    vector<double> tmpvector(numnode);
    actele->data_.Add("thick",tmpvector);
    vector<double>* thick = actele->data_.GetVector<double>("thick");
    for (int i=0; i<numnode; ++i) (*thick)[i] = actele->thickness_;
    
    
    vector<double> funct(numnode);
    Epetra_SerialDenseMatrix deriv(2,numnode);
    
    for (int i=0; i<numnode; ++i)
    {
      double r = actele->s8_localcoordsofnode(i,0,numnode);
      double s = actele->s8_localcoordsofnode(i,1,numnode);
      actele->s8_shapefunctions(funct,deriv,r,s,numnode,1);
      double gkov[3][3];
      /*-------------------------------------------------------------- a1, a2 */
      for (int ialpha=0; ialpha<2; ialpha++)
      {
         for (int idim=0; idim<3; idim++)
         {
            gkov[idim][ialpha]=0.0;
            for (int inode=0; inode<numnode; inode++)
               gkov[idim][ialpha] +=
                 deriv(ialpha,inode) * actele->Nodes()[inode]->X()[idim];
         }
      }
      /*------------------------------------------------------------------ a3 */
      double a3[3];
      a3[0] = gkov[1][0]*gkov[2][1] - gkov[2][0]*gkov[1][1];
      a3[1] = gkov[2][0]*gkov[0][1] - gkov[0][0]*gkov[2][1];
      a3[2] = gkov[0][0]*gkov[1][1] - gkov[1][0]*gkov[0][1];
      double a3norm = a3[0]*a3[0] + a3[1]*a3[1] + a3[2]*a3[2];
      a3norm = 1./(sqrt(a3norm));
      a3[0] *= a3norm;
      a3[1] *= a3norm;
      a3[2] *= a3norm;
      for (int j=0; j<3; j++) (*a3ref)(j,i) = a3[j];
    } // for (int i=0; i<numnode; ++i)
    
    //------------------------------------------ allocate an array for forces
    {
    Epetra_SerialDenseMatrix forces;
    forces.Shape(18,actele->ngp_[0]*actele->ngp_[1]); // 18 forces on upto 9 gaussian points
    actele->data_.Add("forces",forces);
    }
    
    //--------------------------------------allocate space for material history
    MATERIAL* actmat = &(mat[actele->material_-1]);
    if (actmat->mattyp==m_viscohyper)/* material is viscohyperelastic */
    {
      dserror("viscohyperelastic material in shell8 not ported to DRT");
      const int nmaxw  = actmat->m.viscohyper->nmaxw;
      const int ngauss = actele->ngp_[0]*actele->ngp_[1]*actele->ngp_[2];
      const int size = ngauss*(nmaxw+1)*3*3;
      vector<double> his1(size);
      vector<double> his2(size);
      for (int i=0; i<size; ++i) his1[i] = his2[i] = 0.0;
      actele->data_.Add("mathis1",his1);     
      actele->data_.Add("mathis2",his2);     
    }
  } // for (int i=0; i<dis.NumMyColElements(); ++i)
  

  //------------------------------------ do directors at nodes Bischoff style
  map<int,vector<double> > a3map;
  Epetra_SerialDenseMatrix collaverdir(3,MAXELE);
  // loop my row nodes and build a3map
  for (int i=0; i<dis.NumMyRowNodes(); ++i)
  {
    DRT::Node* actnode = dis.lRowNode(i);
    int numa3=0;
    const int numele = actnode->NumElement();
    for (int j=0; j<numele; ++j)
    {
      DRT::Element* tmpele = actnode->Elements()[j];
      if (tmpele->Type()!=DRT::Element::element_shell8) continue;
      DRT::Elements::Shell8* actele = dynamic_cast<DRT::Elements::Shell8*>(tmpele);
      if (!actele) dserror("Element is not Shell8");
      for (int k=0; k<actele->NumNode(); ++k)
      {
        if (actele->Nodes()[k]==actnode)
        {
          Epetra_SerialDenseMatrix* a3ref = actele->data_.GetMatrix("a3ref");
          if (!a3ref) dserror("Cannot find a3ref");
          collaverdir(0,numa3) = (*a3ref)(0,k);
          collaverdir(1,numa3) = (*a3ref)(1,k);
          collaverdir(2,numa3) = (*a3ref)(2,k);
          ++numa3;
          if (numa3>MAXELE) dserror("MAXELE too small");
          break;
        }
      }
    }
    // no averaging if no. of elements to a node is one
    if (!numa3) dserror("No. of elements to a node is zero");
    if (numa3 == 1)
    {
      a3map[actnode->Id()].resize(3);
      for (int k=0; k<3; ++k) 
        a3map[actnode->Id()][k] = collaverdir(k,1);
    }
    else
    {
      // average director at node actnode
      double a3[3];
      s8_averagedirector(collaverdir,numa3,a3);
      a3map[actnode->Id()].resize(3);
      for (int k=0; k<3; ++k) 
        a3map[actnode->Id()][k] = a3[k];
    }    
  } // for (int i=0; i<NumMyRowNodes(); ++i)

  // export this map from nodal row map to nodal col map
  const Epetra_Map* noderowmap = dis.NodeRowMap();
  const Epetra_Map* nodecolmap = dis.NodeColMap();
  DRT::Exporter exporter(*noderowmap,*nodecolmap,dis.Comm());
  exporter.Export(a3map);
  
  // loop column nodes and put directors back into discretization
  for (int i=0; i<dis.NumMyColNodes(); ++i)
  {
    DRT::Node* actnode = dis.lColNode(i);
    map<int,vector<double> >::iterator curr = a3map.find(actnode->Id());
    if (curr==a3map.end()) dserror("Cannot find a3map entry");
    const int numele = actnode->NumElement();
    for (int j=0; j<numele; ++j)
    {
      DRT::Element* tmpele = actnode->Elements()[j];
      if (!tmpele) continue;
      if (tmpele->Type()!=DRT::Element::element_shell8) continue;
      DRT::Elements::Shell8* actele = dynamic_cast<DRT::Elements::Shell8*>(tmpele);
      if (!actele) dserror("Element is not Shell8");
      for (int k=0; k<actele->NumNode(); ++k)
      {
        if (actele->Nodes()[k]==actnode)
        {
          Epetra_SerialDenseMatrix* a3ref = actele->data_.GetMatrix("a3ref");
          if (!a3ref) dserror("Cannot find a3ref");
          (*a3ref)(0,k) = curr->second[0];
          (*a3ref)(1,k) = curr->second[1];
          (*a3ref)(2,k) = curr->second[2];
          break;
        }
      } // for (int k=0; k<actele->NumNode(); ++k)
    } // for (int j=0; j<numele; ++j)
  } // for (int i=0; i<dis.NumMyColNodes(); ++i)

  return 0;
}


/*----------------------------------------------------------------------*
 |  average director (public)                                mwgee 12/06|
 *----------------------------------------------------------------------*/
#define DSQR(a) ((a)*(a))
#define ABS(x)  ((x) <  0  ? (-x) : (x))
void s8_averagedirector(Epetra_SerialDenseMatrix& dir_list, const int numa3, double a3[])
{
  DSTraceHelper dst("Shell8Register::s8_averagedirector");

  double davn[3];
  double averdir[3];
  averdir[0] = dir_list(0,0);
  averdir[1] = dir_list(1,0);
  averdir[2] = dir_list(2,0);
  for (int i=1; i<numa3; ++i)
  {
    /*------------------------------ make cross product of two directors */
    double normal[3];
    normal[0] = averdir[1]*dir_list(2,i) - averdir[2]*dir_list(1,i);
    normal[1] = averdir[2]*dir_list(0,i) - averdir[0]*dir_list(2,i);
    normal[2] = averdir[0]*dir_list(1,i) - averdir[1]*dir_list(0,i);
    const double length = normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2];
    if (length<=1.e-12)
    {
      davn[0] = 0.5*(averdir[0]+dir_list(0,i));
      davn[1] = 0.5*(averdir[1]+dir_list(1,i));
      davn[2] = 0.5*(averdir[2]+dir_list(2,i));
    }
    else
    {
       const double denom =
                   (DSQR(dir_list(0,i))+DSQR(dir_list(2,i)))*DSQR(averdir[1])
                  +(-2.*dir_list(0,i)*averdir[0]*dir_list(1,i)-2.*dir_list(2,i)
                  *averdir[2]*dir_list(1,i))*averdir[1]+(DSQR(dir_list(2,i))
                  +DSQR(dir_list(1,i)))*DSQR(averdir[0])-2.*averdir[2]*averdir[0]
                  *dir_list(2,i)*dir_list(0,i)+(DSQR(dir_list(0,i))+DSQR(dir_list(1,i)))
                  *DSQR(averdir[2]);
       if (ABS(denom)<=1.e-13) dserror("Making of mod. directors failed");
       const double alpha = (averdir[2]*dir_list(2,i)-DSQR(dir_list(0,i))+averdir[0]*dir_list(0,i)
                   -DSQR(dir_list(1,i))+dir_list(1,i)*averdir[1]-DSQR(dir_list(2,i)))/denom; 

       davn[0] =-alpha*DSQR(averdir[1])*dir_list(0,i)+alpha*averdir[1]*averdir[0]*dir_list(1,i)
                +averdir[0]+alpha*averdir[2]*averdir[0]*dir_list(2,i)-alpha*DSQR(averdir[2])*dir_list(0,i);

       davn[1] =alpha*averdir[0]*averdir[1]*dir_list(0,i)+averdir[1]+alpha*averdir[2]*averdir[1]
                *dir_list(2,i)-alpha*DSQR(averdir[0])*dir_list(1,i)-alpha*DSQR(averdir[2])*dir_list(1,i);

       davn[2] =-alpha*DSQR(averdir[1])*dir_list(2,i)+alpha*averdir[1]*averdir[2]*dir_list(1,i)
                -alpha*DSQR(averdir[0])*dir_list(2,i)+alpha*averdir[0]*averdir[2]*dir_list(0,i)+averdir[2];
      
    }
    a3[0] = davn[0];
    a3[1] = davn[1];
    a3[2] = davn[2];
  } // for (int i=1; i<numa3; ++i)
  return;
}



#endif  // #ifdef TRILINOS_PACKAGE
#endif  // #ifdef CCADISCRET
#endif  // #ifdef D_SHELL8
