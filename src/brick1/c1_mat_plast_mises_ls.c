/*!----------------------------------------------------------------------
\file
\brief contains the routine ' ' which calclate displacement
       derivatives for a 3D hex element

*----------------------------------------------------------------------*/
#ifdef D_BRICK1

#include "../headers/standardtypes.h"
#include "brick1.h"
#include "brick1_prototypes.h"

/*! 
\addtogroup BRICK1 
*//*! @{ (documentation module open)*/

/*!----------------------------------------------------------------------
\brief routines for mises plasticity with respect to large strains 

<pre>                                                              al 06/02
This routine ... for an 3D-hex-element.

</pre>
\param **sigy  DOUBLE  (i)   input value

\warning There is nothing special to this routine
\return void                                               
\sa calling: ---; called by: c1_cint()

/*----------------------------------------------------------------------*
 |                                                        al    9/01    |
 |   evaluation of the elastic constitutive matrix (large strains)      |
 *----------------------------------------------------------------------*/
void c1mate(
            double detf,   /*   */
            double rmu,    /*   */
            double rk,     /*   */
            double bmu,    /*   */
            double sig2,   /*   */
            double *devn,  /*   */
            double **d)    /*   */
{
/*----------------------------------------------------------------------*/
int    i, j;
double a, b, c, f, e, fac;
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("c1mate");
#endif
/*----------------------------------------------------------------------*/
  a = 2./3.*bmu;
  b =-2./3.*sig2;
  c = detf*rk*(1.+log(detf));
  f = detf*rk*(1.-log(detf));
  e= -detf*rk*log(detf);

  for (i=0; i<6; i++) {
    for (j=0; j<6; j++) {
      d[i][j] = 0.;
  }}

  d[0][0] = f + 2.*a + b*2.*devn[0];
  d[1][1] = f + 2.*a + b*2.*devn[1];
  d[2][2] = f + 2.*a + b*2.*devn[2];
  d[3][3] = e + bmu;
  d[4][4] = e + bmu;
  d[5][5] = e + bmu; 

  d[0][1] = c - a + b*(devn[0]+devn[1]);
  d[0][2] = c - a + b*(devn[0]+devn[2]);
  d[1][0] = c - a + b*(devn[0]+devn[1]);
  d[2][0] = c - a + b*(devn[0]+devn[2]);
  d[1][2] = c - a + b*(devn[1]+devn[2]);
  d[2][1] = c - a + b*(devn[1]+devn[2]);
  d[0][3] = b*devn[3];
  d[0][4] = b*devn[4];
  d[0][5] = b*devn[5];
  d[1][3] = b*devn[3];
  d[1][4] = b*devn[4];
  d[1][5] = b*devn[5];
  d[2][3] = b*devn[3];
  d[2][4] = b*devn[4];
  d[2][5] = b*devn[5];
  d[3][0] = b*devn[3];
  d[4][0] = b*devn[4];
  d[5][0] = b*devn[5];
  d[3][1] = b*devn[3];
  d[4][1] = b*devn[4];
  d[5][1] = b*devn[5];
  d[3][2] = b*devn[3];
  d[4][2] = b*devn[4];
  d[5][2] = b*devn[5];      
  
  fac=1.;
  for (i=0; i<6; i++) {
    for (j=0; j<6; j++) {
      d[i][j] = d[i][j]/fac;
  }}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif  
return; 
} /* end of c1mate */
/*----------------------------------------------------------------------*
 |                                                        al    9/01    |
 |   radial return for elements with mises material model               |
 |   topic : radial return (large def. model)                           |
 *----------------------------------------------------------------------*/
void c1radg(
            double fhard,    /* hardening modulus                       */
            double uniax,    /* yield stresse                           */
            double bmu,      /*                        */
            double sig2,     /*                                         */
            double *dhard,   /* hardening modulus                       */
            double *dev,     /* elastic predicor projected onto yield   */
            double *epstn,   /* equivalent uniaxial plastic strain      */
            double *dlam)    /* increment of plastic multiplier         */
{
/*----------------------------------------------------------------------*/
int i;
int isoft1 = 0;
int imax = 30;
double epst, ro32, ro23, esig, f, dfdl, dum, mot, epsth;
double rnorm[6];
double tol = 1.0E-5;
double alpha = 0.;
double expo  = 0.;
double rlin  = 0.;
double rqua  = 0.;
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("c1radg");
#endif
/*---------------------------------------  non-linear hardening rule ---*/
/*  initialize variables */
  ro32  = sqrt(3./2.);
  ro23  = 1./ro32;
  *dlam = 0.;
  for (i=0;i<6;i++) rnorm[i]=dev[i]/sig2;
/*------- iteration scheme to obtain increment of plastic multiplier ---*/
  i=0;
/*------- iteration scheme to obtain increment of plastic multiplier ---*/
L500:
  ++i;
/* new plastic uniaxial deformation */

  epst = *epstn + ro23* *dlam;

/* new hardening rate and  new uniaxial yield stress */
  if(rqua!=0.) 
  {
    if(epst<=alpha){
      *dhard = uniax*(rlin -2.*rqua*epst);
      esig  = ro23*uniax*(1. + rlin*epst -rqua*epst*epst);
    }else{
      *dhard = uniax*(rlin -2.*rqua*epst)/expo;
      esig  = ro23*uniax*(fhard*expo + rlin*epst -rqua*epst*epst)/expo;
    }
  }
  else
  {
    if(uniax!=0.){
     *dhard = fhard + alpha*(expo*exp(-expo*epst));
     esig  = ro23*(uniax + fhard*epst + alpha*(1.-exp(-expo*epst)));
    }else{

     mot = expo-1.;
     fortranpow(&epst,&epsth,&mot);
     *dhard = fhard*expo*epsth;
     
     mot = expo;
     fortranpow(&epst,&epsth,&mot);
     esig  = ro23*( fhard*epsth );   
    }
  }

/*  apply von mises yield criteria */

  f= sig2 - esig -2.*bmu* *dlam;

/*  derivative of the yield criteria with respect to plastic increment */

  dfdl = -2.*bmu*(1.+*dhard/(3.*bmu));

/*  new plastic increment */

  *dlam = *dlam - f/dfdl;

/* check convergence */

  if (esig == 0.) goto L500;
  if ((dum = f / esig, fabs(dum)) > 1e-5) 
  {
      if (i > 30) {
          dserror("local iteration exceeds limit");
      }
      goto L500;
  }

/*----------------------------------------------------------------------*/
/* update plastic strain */

  *epstn += ro23* (*dlam);
  if(rqua!=0.) 
  {
      if(*epstn<=alpha){
        *dhard = uniax*(rlin -2.*rqua*(*epstn));
      }else{
        *dhard = uniax*(rlin -2.*rqua*(*epstn))/expo;
      }
  } else {
    if(uniax!=0.) {
      *dhard = fhard + alpha*(expo*exp(-expo*(*epstn)));
    }else{
      mot = expo-1.;
      fortranpow(epstn,&epsth,&mot);
      *dhard = fhard*expo*epsth;
    }
  }

/* new stress state */  
  for (i=0;i<6;i++) dev[i] -= 2.*bmu*(*dlam)*rnorm[i];
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of c1radg */
/*----------------------------------------------------------------------*
 |                                                        al    9/01    |
 |   forms the elasto-plastic consistent tangent material tensor        |
 |   (large strains)                                                    |
 *----------------------------------------------------------------------*/
void c1matpg(
             double dlam,    /* increment of plastic multiplier         */
             double detf,
             double rmu,
             double rk,
             double bmu,
             double sig2,
             double hard,
             double *devn,
             double **d)     /* material matrix to be calculated        */
{
/*----------------------------------------------------------------------*/
int i, j, k, l;
double a,b,c,f,e,b0,b1,b2,b3,trrn2,fac;
double rn2[6];
double drn2[6];
double aux[6][6];
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("c1matpg");
#endif
/*----------------------------------------------------------------------*/
  b0 = 1. + hard/(3.*bmu);
  b1 = 2.*bmu*dlam/sig2;
  b2 = (1. - 1./b0)*2.*sig2*dlam/(3.*bmu);
  b3 = 1./b0 - b1 + b2;

  a = 2./3.*bmu;
  b =-2./3.*sig2;
  c = detf*rk*(1.+log(detf));
  f = detf*rk*(1.-log(detf));
  e= -detf*rk*log(detf);

  for (i=0; i<6; i++) {
    for (j=0; j<6; j++) {
      d[i][j] = 0.;
  }}

  d[0][0] = f + (2.*a + b*2.*devn[0])*(1.-b1);
  d[1][1] = f + (2.*a + b*2.*devn[1])*(1.-b1);
  d[2][2] = f + (2.*a + b*2.*devn[2])*(1.-b1);
  d[3][3] = e + bmu*(1.-b1);
  d[4][4] = e + bmu*(1.-b1);
  d[5][5] = e + bmu*(1.-b1); 

  d[0][1] = c + ( - a + b*(devn[0]+devn[1]))*(1.-b1);
  d[0][2] = c + ( - a + b*(devn[0]+devn[2]))*(1.-b1);
  d[1][0] = c + ( - a + b*(devn[0]+devn[1]))*(1.-b1);
  d[2][0] = c + ( - a + b*(devn[0]+devn[2]))*(1.-b1);
  d[1][2] = c + ( - a + b*(devn[1]+devn[2]))*(1.-b1);
  d[2][1] = c + ( - a + b*(devn[1]+devn[2]))*(1.-b1); 
  d[0][3] = b*devn[3]*(1.-b1);
  d[0][4] = b*devn[4]*(1.-b1);
  d[0][5] = b*devn[5]*(1.-b1);
  d[1][3] = b*devn[3]*(1.-b1);
  d[1][4] = b*devn[4]*(1.-b1);
  d[1][5] = b*devn[5]*(1.-b1);
  d[2][3] = b*devn[3]*(1.-b1);
  d[2][4] = b*devn[4]*(1.-b1);
  d[2][5] = b*devn[5]*(1.-b1);
  d[3][0] = b*devn[3]*(1.-b1);
  d[4][0] = b*devn[4]*(1.-b1);
  d[5][0] = b*devn[5]*(1.-b1);
  d[3][1] = b*devn[3]*(1.-b1);
  d[4][1] = b*devn[4]*(1.-b1);
  d[5][1] = b*devn[5]*(1.-b1);
  d[3][2] = b*devn[3]*(1.-b1);
  d[4][2] = b*devn[4]*(1.-b1);
  d[5][2] = b*devn[5]*(1.-b1);  

  for (i=0; i<6; i++) {
    for (j=0; j<6; j++) {
      d[i][j] -= 2.*bmu*b3*devn[i]*devn[j];
  }}

/*  add term -b2*2*bmu*(n x devn(n2)) */

  rn2[0] = devn[0]*devn[0] + devn[3]*devn[3] + devn[5]*devn[5];
  rn2[1] = devn[3]*devn[3] + devn[1]*devn[1] + devn[4]*devn[4];
  rn2[2] = devn[4]*devn[4] + devn[5]*devn[5] + devn[2]*devn[2];
  rn2[3] = devn[0]*devn[3] + devn[3]*devn[1] + devn[4]*devn[5];
  rn2[4] = devn[1]*devn[4] + devn[3]*devn[5] + devn[4]*devn[2];
  rn2[5] = devn[3]*devn[4] + devn[0]*devn[5] + devn[5]*devn[2];

  trrn2 = (rn2[0]+rn2[1]+rn2[2])/3.;
  drn2[0] = rn2[0] - trrn2;
  drn2[1] = rn2[1] - trrn2;
  drn2[2] = rn2[2] - trrn2;
  drn2[3] =  rn2[3];
  drn2[4] =  rn2[4];
  drn2[5] =  rn2[5];         

  for (i=0; i<6; i++) {
    for (j=0; j<6; j++) {
      aux[i][j] = devn[i]*drn2[j];
  }}

  fac=1.;
  for (i=0; i<6; i++) {
    for (j=0; j<6; j++) {
      d[i][j] = d[i][j]/fac + 2.*sig2*(b1-1./b0)*(1./2.)*(aux[i][j]+aux[j][i])/fac;
  }}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif  
return; 
} /* end of c1matpg */
/*----------------------------------------------------------------------*
 |   push forward(deformations)/pull-back(stresses)       al    9/01    |
 *----------------------------------------------------------------------*/
void c1pushf(
            double *be,   
            double *bet,  
            double *fn
            )   
{
/*----------------------------------------------------------------------*/
int i;
int dim3 = 3;
double fb[9],bbe[9],fc[9],fa[9];
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("c1pushf");
#endif
/*----------------------------------------------------------------------*/

/* deformation gradient */
  /* fc[1,1] = */ fc[0] = fn[0];
  /* fc[2,2] = */ fc[4] = fn[1];
  /* fc[3,3] = */ fc[8] = fn[2];
  /* fc[2,1] = */ fc[1] = fn[3];
  /* fc[1,2] = */ fc[3] = fn[4];
  /* fc[3,2] = */ fc[5] = fn[5];
  /* fc[2,3] = */ fc[7] = fn[6];
  /* fc[3,1] = */ fc[2] = fn[7];
  /* fc[1,3] = */ fc[6] = fn[8];

  /* bbe[1,1] == */ bbe[0] = be[0];
  /* bbe[1,2] == */ bbe[3] = be[3];
  /* bbe[1,3] == */ bbe[6] = be[5];
  /* bbe[2,1] == */ bbe[1] = be[3];
  /* bbe[2,2] == */ bbe[4] = be[1];
  /* bbe[2,3] == */ bbe[7] = be[4];
  /* bbe[3,1] == */ bbe[2] = be[5];
  /* bbe[3,2] == */ bbe[5] = be[4];
  /* bbe[3,3] == */ bbe[8] = be[2];

   mxmab (fc,bbe,fb,&dim3,&dim3,&dim3);
   mxmabt(fb,&dim3,&dim3,&dim3,fc,fa);

  bet[0] = fa[0]; /* = fa[1,1] */
  bet[1] = fa[4]; /* = fa[2,2] */
  bet[2] = fa[8]; /* = fa[3,3] */
  bet[3] = fa[3]; /* = fa[1,2] */
  bet[4] = fa[7]; /* = fa[2,3] */
  bet[5] = fa[6]; /* = fa[1,3] */
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif  
return; 
} /* end of c1pushf */
/*----------------------------------------------------------------------*
 |                                                              al 9/01 |
 | topic :  calculate  e l a s t o p l a s t i c  stresses and          |
 |          stress increments  -  s t r e s s  p r o j e c t i o n      |
 |          for large deformation model                                 |
 |                                                                      |
 *----------------------------------------------------------------------*/
void c1elpag(
             double ym,      /* young's modulus              */
             double pv,      /* poisson's ratio              */
             double uniax,   /* yield stresse                */
             double fhard,   /* hardening modulus            */
             double *stress, /**/      
             double *sig,    /**/      
             double *fn,     /**/      
             double *fni,    /**/      
             double detf,    /**/      
             double **d,     /* material matrix              */
             double *epstn,  /**/
             int    *iupd,   /* controls storing of stresses */
             int    *yip)    /*    */
{
/*----------------------------------------------------------------------*/
int i,j,k;
double mot, dlam, rlin, rqua, rmu, rk, deth, trtau, sig2, ft;
double epstnh, expo, yld, bmu, fac, press, deti;
double sq23, alpha, sm, dhard, expoh;
double tol = 1.0E-10;
double sigf[6];
double tau[6];
double dev[6];
double rnorm[6];
double eps[9];
double faux[9];
#ifdef DEBUG 
dstrc_enter("c1elpag");
#endif
/*----------------------------------------------------------------------*/
  sq23 = sqrt(2./3.);
  *iupd = 0;
  dlam = 0.;
  rlin = 0.;
  rqua = 0.;
  mot = -1./3.;
  rmu = ym/(2.+2.*pv);
  rk  = ym/((1.-2.*pv)*3.);
/*-----------------------------------------------------------------------|
|     yip > 0  stresses are available from last update                   |
|         = 1  e l a s t i c                                             |
|         = 2  p l a s t i c                                             |
|     update flag must set to store change of parameter yip              |
|     no changes have been made on stress state                          |
|-----------------------------------------------------------------------*/
  if(*yip>0)
  {/*???*/
    for (i=0; i<9; i++)
    { /* faux = (detf)^(-1/3) * fn */
      fortranpow(&detf,&deth,&mot);
      faux[i] = deth * fn[i];
    }
    c1pushf (sig,sigf,faux);
    sm = (sigf[0]+sigf[1]+sigf[2])/3.;
    bmu=rmu*sm;
    dev[0] = rmu * (sigf[0] - sm);
    dev[1] = rmu * (sigf[1] - sm);
    dev[2] = rmu * (sigf[2] - sm);
    dev[3] = rmu * sigf[3];
    dev[4] = rmu * sigf[4];
    dev[5] = rmu * sigf[5];
    
    sig2 = ( dev[0]*dev[0] + dev[1]*dev[1] + dev[2]*dev[2])/2.
             + dev[3]*dev[3] + dev[4]*dev[4] + dev[5]*dev[5];
    sig2*=2.;
    sig2=sqrt(sig2);

    if(rqua!=0.) 
    {
        if(*epstn<=alpha){
          dhard = uniax*(rlin -2.*rqua*(*epstn));
        }else{
          dhard = uniax*(rlin -2.*rqua*(*epstn))/expo;
        }
    } else {
         if(uniax!=0.){
              dhard = fhard + alpha*(expo*exp(-expo*(*epstn)));
         }else{ 
              expoh = expo-1.;
              fortranpow(epstn,&epstnh,&expoh);
              dhard = fhard*expo*epstnh;
         }
    }
    
    for (i=0; i<6; i++)
    {
        if(sig2>0.){ 
          rnorm[i]=dev[i]/sig2;
        } else {
          rnorm[i]=dev[i];
        } 
    }

/*--------- yip = 2 ----------------------------------------------------*/
    if (*yip==1) 
    {
      c1mate (detf,rmu,rk,bmu,sig2,rnorm,d); 
/*--------- yip = 2 ----------------------------------------------------*/
    } else {    
      dlam = 0.;
      c1matpg (dlam,detf,rmu,rk,bmu,sig2,dhard,rnorm,d);
    }
    *yip=-*yip;
    fac=1.;
    press=detf* log(detf)*rk;

    stress[0]=(press + dev[0])/fac;
    stress[1]=(press + dev[1])/fac;
    stress[2]=(press + dev[2])/fac;      
    stress[3]=dev[3]/fac;
    stress[4]=dev[4]/fac;
    stress[5]=dev[5]/fac;

    *iupd=1;
   goto end;
  }/*???*/
/*-------------------------------------------------- yield condition ---*/
/*------ determine uniaxial yield stress (with non-linear hardening) ---*/
  if(rqua!=0.) 
  {
    if((*epstn)<=alpha){
      yld = sq23*uniax*(1. + rlin*(*epstn) -rqua*(*epstn)*(*epstn));
    }else{
      yld = sq23*uniax*(fhard*expo + rlin*(*epstn) -rqua*(*epstn)*(*epstn))/expo;
    }
  } 
  else
  {
    if(uniax!=0.) {
      yld = sq23*(uniax + fhard*(*epstn) + alpha*(1.-exp(-expo*(*epstn))));
    }else{
      fortranpow(epstn,&epstnh,&expo);
      yld = sq23*(fhard*epstnh);
    }
  } 

/*-----------------------------------------------------------------------|
|   for large strains:                                                   |
|   determine intermediate configuration for large strains               |
|     sig  = elastic cauchy-green strain tensor from last configuration  |
|     fn   = deformation gradient (inverse)                              |
|     tau  = trial elastic cauchy-grenn strain (interm conf isochoric)   |
|-----------------------------------------------------------------------*/
  for (i=0; i<9; i++)
  { /* faux = (detf)^(-1/3) * fn */
    fortranpow(&detf,&deth,&mot);
    faux[i] = deth * fn[i];
  }
    c1pushf (sig,tau,faux);
/*------ calculate elastic predictor  s(trial) = mu * dev[be(trial)] ---*/
  trtau = (tau[0]+tau[1]+tau[2])/3.;
  bmu=rmu*trtau;
  dev[0] = rmu * (tau[0] - trtau);
  dev[1] = rmu * (tau[1] - trtau);
  dev[2] = rmu * (tau[2] - trtau);
  dev[3] = rmu *  tau[3];
  dev[4] = rmu *  tau[4];
  dev[5] = rmu *  tau[5];
/*------------- check for plastic loading (von mises yield criteria) ---*/
  sig2 = ( dev[0]*dev[0] + dev[1]*dev[1] + dev[2]*dev[2])/2.
           + dev[3]*dev[3] + dev[4]*dev[4] + dev[5]*dev[5];
  sig2*=2.;
  sig2=sqrt(sig2);
  ft = sig2 - yld;
  for (i=0; i<6; i++)
  {
    if(sig2>0.){ 
      rnorm[i]=dev[i]/sig2;
    }else{
      rnorm[i]=dev[i];
    } 
  } 
/*-------------------------------------------------- yield condition ---*/
  if (ft<=tol) 
  {
/*------------- state of stress within yield surface - E L A S T I C ---*/
    *yip=1;
       c1mate (detf,rmu,rk,bmu,sig2,rnorm,d);
   } else {
/*------------ state of stress outside yield surface - P L A S T I C ---*/
    *yip = 2;
    bmu=rmu*trtau;
/*--- projection of devatoric stresses on yield surface ---*/
    c1radg (fhard, uniax,bmu,sig2,&dhard,dev,epstn,&dlam);
/*--- consistent tangent ---*/
      c1matpg (dlam,detf,rmu,rk,bmu,sig2,dhard,rnorm,d);
  }
/*--- adition of the elastic hydrostatic pressure (only to diagonal) ---*/
    fac=1.;

  press=detf* log(detf)*rk;

  stress[0]=(press + dev[0])/fac;
  stress[1]=(press + dev[1])/fac;
  stress[2]=(press + dev[2])/fac;      
  stress[3]=dev[3]/fac;
  stress[4]=dev[4]/fac;
  stress[5]=dev[5]/fac;
/*-------------------------------- update intermediate configuration ---*/
  for (i=0; i<6; i++) sig[i]= dev[i]/rmu;
  sig[0] += trtau;
  sig[1] += trtau;
  sig[2] += trtau;

/*--------------- pull back from link cauchy def. tensor (f inverse) ---*/
  deti=1./detf;
  for (i=0; i<9; i++)
  { /* faux = (detf)^(-1/3) * fn */
    fortranpow(&deti,&deth,&mot);
    faux[i] = deth * fni[i];
  }
    c1pushf (sig,tau,faux);

  sig[0] = tau[0];
  sig[1] = tau[1];
  sig[2] = tau[2];
  sig[3] = tau[3];
  sig[4] = tau[4];
  sig[5] = tau[5];
/*----------------------------------------------------------------------*/
end:
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of c1elpag */
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 |                                                              al 9/01 |
 | constitutive matrix - forces - plastic large strain - von Mises - 3D |
 *----------------------------------------------------------------------*/
void c1_mat_plast_mises_ls(
                        double ym,      /* young's modulus              */
                        double pv,      /* poisson's ratio              */
                        double alfat,   /* temperature expansion factor */
                        double uniax,   /* yield stresse                */
                        double fhard,   /* hardening modulus            */
                        ELEMENT   *ele, /* actual element               */
                        double **bop,   /* derivative operator          */
                        int ip,         /* integration point Id         */
                        double *stress, /*ele stress (-resultant) vector*/      
                        double **d,     /* material matrix              */
                        double  *disd,  /* displacement derivatives     */
                        double g[6][6], /* transformation matrix        */
                        double gi[6][6],/* inverse of g                 */
                        int istore,     /* controls storing of stresses */
                        int newval)     /* controls eval. of stresses   */
{
/*----------------------------------------------------------------------*/
int i,j,k;
int yip;
int isoft;
int iupd;
double e1, e2, e3, a1, b1, c1, sum, epstn, ft;
double sig[6];
double eps[9];
double strain[9];
double delsig[6];
double deleps[9];
double tau[6];
double tol = 1.0E-10;
double dlam;

double mot, deth;
double rmu, rk, fac, press, det, detf;
double aux[9];
double disd1[9];
double stress1[6];
double sigf[6];


double yld, sm, sx, sy, sz, sxy, sxz, syz, sig2, hard;
double expo  = 0.;
double alpha = 0.;
#ifdef DEBUG 
dstrc_enter("c1_mat_plast_mises_ls");
#endif
/*----------------------------------------------------------------------*/
  iupd=0;
/*----------------------------------------------------------------------*/
  mot = -1./3.;
  rmu = ym/(2.+2.*pv);
  rk  = ym/((1.-2.*pv)*3.);
/*----------------------------- get old values -> sig, eps,epstn,yip ---*/
  for (i=0; i<6; i++) sig[i] = ele->e.c1->elewa[0].ipwa[ip].sig[i];
  for (i=0; i<9; i++) eps[i] = ele->e.c1->elewa[0].ipwa[ip].eps[i];
  epstn  = ele->e.c1->elewa[0].ipwa[ip].epstn;
  yip    = ele->e.c1->elewa[0].ipwa[ip].yip;
/*----------------------------------------------------------------------*/
  det = ( eps[0]*eps[1]*eps[2] +
          eps[4]*eps[6]*eps[7] +
          eps[8]*eps[3]*eps[5] -
          eps[7]*eps[1]*eps[8] -
          eps[5]*eps[6]*eps[0] -
          eps[2]*eps[3]*eps[4] ); 

  for (i=0; i<9; i++)
  { /* aux = (det)^(-1/3) * eps */
    fortranpow(&det,&deth,&mot);
    aux[i] = deth * eps[i];
  }

  c1pushf (sig,sigf,aux);
  sm = (sig[0]+sig[1]+sig[2])/3.;
  stress1[0] = rmu * (sigf[0] - sm);
  stress1[1] = rmu * (sigf[1] - sm);
  stress1[2] = rmu * (sigf[2] - sm);
  stress1[3] = rmu * sigf[3];
  stress1[4] = rmu * sigf[4];
  stress1[5] = rmu * sigf[5];
 
  press=det* log(det)*rk;

  fac=1.;
  stress1[0]=(press + stress1[0])/fac;
  stress1[1]=(press + stress1[1])/fac;
  stress1[2]=(press + stress1[2])/fac ;     
/*-------------------------------------------- evaluate new stresses ---*/
  if(yip>0)
  {
    for (i=0; i<9; i++) disd[i]= eps[i];
    c1invf(disd,disd1,&detf);
    detf=1./detf;
  } else {
    disd[0] += 1.;
    disd[1] += 1.;
    disd[2] += 1.;
    c1invf(disd,disd1,&detf);
    detf=1./detf;
  }
/*----------------------------------------------------------------------*/
  if(newval==1)
  {
    c1elpag(ym, pv, uniax, fhard, stress, sig, disd, disd1, detf, d, &epstn, &iupd, &yip);    

  } else {
    iupd = 0;
    for (i=0; i<6; i++) stress[i] = stress1[i];
  }
/*----------------------------- put new values -> sig, eps,epstn,yip ---*/
end:
/*----------------------------------------------------------------------*/
/* disd1 ----- new def grad (inverse)         */
/* sig ---- new left cauchy-green def tensor  */
  if(istore==1 || iupd==1)
  {
    for (i=0; i<6; i++) ele->e.c1->elewa[0].ipwa[ip].sig[i] = sig[i];
    for (i=0; i<6; i++) ele->e.c1->elewa[0].ipwa[ip].eps[i] = disd[i];

    ele->e.c1->elewa[0].ipwa[ip].epstn = epstn;
    ele->e.c1->elewa[0].ipwa[ip].yip   = yip  ;
  }
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of c1_mat_plast_mises_ls */
/*----------------------------------------------------------------------*/
 #endif
/*! @} (documentation module close)*/
       
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
