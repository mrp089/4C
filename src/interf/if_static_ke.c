/*!-----------------------------------------------------------------------
\file
\brief contains the routine 'if_static_ke' which calculates the usual
stiffness matrix for a interface element

*-----------------------------------------------------------------------*/
#ifdef D_INTERF
#include "../headers/standardtypes.h"
#include "interf.h"
#include "interf_prototypes.h"

/*! 
\addtogroup INTERF
*//*! @{ (documentation module open)*/

/*!----------------------------------------------------------------------
\brief calculates usual stiffness matrix  

<pre>                                                              mn 05/03
This routine calculates usual stiffness matrix for small strains
formulation.

</pre>
\param **s       DOUBLE    (o)  blablabla 
\param   dl      DOUBLE    (i)  blablabal

\warning There is nothing special to this routine
\return void                                               
\sa calling:  ---; 
    caled by: interf();

*----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;
/*---------------------------------------------------------------------*/

void ifstatic_ke(ELEMENT       *ele, 
                 INTERF_DATA   *data, 
                 MATERIAL      *mat,
                 ARRAY         *estif_global,
                 ARRAY         *emass_global,
                 DOUBLE        *force,  /* global int forces (0-initialized in the corrector, not needed for predictor) */
                 INT            init)
{
const DOUBLE    q12 = ONE/TWO;
const DOUBLE    q14 = ONE/FOUR;
INT             ield;         /* numnp to this element     */
INT             iele;         /* numnp for gradient enhancement of wall     */
INT             numdfd;
INT             i,k,lr,a,b;      /* some loopers     */
DOUBLE          detd;            /* determinants of jacobian matrix  */
DOUBLE          facd;        /* integration factor  */
DOUBLE          facr;        /* weight at gaussian point                                  */
DOUBLE          e1;          /* xi-coordinate of gaussian point                          */
INT             nir;         /* number of gaussian points                             */
DOUBLE          Thick;       /* element thickness perpendicular to wall plane                    */
DOUBLE          width;       /* element thickness in wall plane                    */
DOUBLE          deltax,deltay;/* differences between coordinates */
DOUBLE          beta;        /* angle between xi-axis and X-Y system in [0,pi/4] */
DOUBLE          alpha;       /* angle between xi-axis and X-Y system in [0,..2 pi] */
DOUBLE          cod,sid;
DOUBLE          c_parabel,b_parabel; /* y = a + b*x + c*x^2  */
DOUBLE          help;        /* Zwischenwert */
INT             flag=0;      /* flag for distinction between differnet element orientation cases  */
INT             ip;
INT             istore = 0;  /* controls storing of new stresses to wa */
INT             newval = 0;  /* controls evaluation of new stresses    */
INT             imass  = 0;  /* imass=0 -> static, imass=1 -> dynamic    */


static ARRAY    xrefe_a;     /* coordinates of element nodes */     
static DOUBLE **xrefe;         
static ARRAY    D_a;         /* material tensor */     
static DOUBLE **D;         

static ARRAY    functd_a;     /* shape functions for [u]*/    
static DOUBLE  *functd;     
static ARRAY    bopd_a;       /* lets call it B-operator for nt-direction*/   
static DOUBLE **bopd;     

    
static ARRAY    Kdd_a;       /* element stiffness-Zwischenspeicher */   
static DOUBLE **Kdd;     
       
static DOUBLE **estif;       /* element stiffness matrix ke */

DOUBLE T[2];                 /* stress */
DOUBLE L[4];                 /* distance between corner nodes of the element */
DOUBLE x_mid[3];             /* x-coordinates on xi-axis */
DOUBLE y_mid[3];             /* y-coordinates on xi-axis */
DOUBLE fintd[16];            /* internal force Zwischenspeicher */
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("ifstatic_ke");
#endif
/*------------------------------------------------- some working arrays */
if (init==1)
{
xrefe     = amdef("xrefe"  ,&xrefe_a,  2,8, "DA");

functd    = amdef("functd" ,&functd_a ,3,1, "DV");       
bopd      = amdef("bopd"   ,&bopd_a   ,2,16,"DA");           

D         = amdef("D"      ,&D_a      ,2,2, "DA");           

Kdd       = amdef("Kdd"    ,&Kdd_a    ,(2*MAXNOD_WALL1),(2*MAXNOD_WALL1),"DA");           
goto end;
}
/*----------------------------------------------------------------------*/
/* uninit phase        (init=-1)                                        */
/*----------------------------------------------------------------------*/
else if (init==-1)
{
   amdel(&xrefe_a);   
   amdel(&functd_a);
   amdel(&D_a);
   amdel(&bopd_a);
   amdel(&Kdd_a);
   goto end;  
}
else if(init==2)
{
  istore = 1;
}
/*----------------------------------------------------------------------*/
/*----------- check orientation of element (which is my xi direction)---*/
ield     = ele->numnp;
numdfd   = 2* ield;
iele     = 4;
for (k=0; k<ield; k++)
{
 xrefe[0][k] = ele->node[k]->x[0];       /* coordinates in x-direction */
 xrefe[1][k] = ele->node[k]->x[1];       /* coordinates in y-direction */              
}
L[0] = sqrt( (xrefe[0][1] - xrefe[0][0]) * (xrefe[0][1] - xrefe[0][0])
     +       (xrefe[1][1] - xrefe[1][0]) * (xrefe[1][1] - xrefe[1][0]));
L[1] = sqrt( (xrefe[0][2] - xrefe[0][1]) * (xrefe[0][2] - xrefe[0][1])
     +       (xrefe[1][2] - xrefe[1][1]) * (xrefe[1][2] - xrefe[1][1]));
/*-------------------------------------------- integration parameters ---*/
ifintg(ele,data);

/*--------------------- coordinates of "nonexisting nodes" on xi-axes ---*/
switch(ele->distyp)
{
case quad4:

     if(L[0]>L[1])
     {
      x_mid[0]   = q12*(xrefe[0][0] + xrefe[0][3]);
      y_mid[0]   = q12*(xrefe[1][0] + xrefe[1][3]);
      x_mid[1]   = q12*(xrefe[0][1] + xrefe[0][2]);
      y_mid[1]   = q12*(xrefe[1][1] + xrefe[1][2]);
      flag = 1;
      width=L[1];
     }
     else if (L[1]>L[0])
     {
      x_mid[0]   = q12*(xrefe[0][0] + xrefe[0][1]);
      y_mid[0]   = q12*(xrefe[1][0] + xrefe[1][1]);
      x_mid[1]   = q12*(xrefe[0][2] + xrefe[0][3]);
      y_mid[1]   = q12*(xrefe[1][2] + xrefe[1][3]);
      flag = 2;
      width=L[0];
     }
     
break;
/*-----------------------------------------------------------------------*/
case quad8:

     if(L[0]>L[1])
     {
      x_mid[0]   = q12*(xrefe[0][0] + xrefe[0][3]);
      y_mid[0]   = q12*(xrefe[1][0] + xrefe[1][3]);
      x_mid[1]   = q12*(xrefe[0][1] + xrefe[0][2]);
      y_mid[1]   = q12*(xrefe[1][1] + xrefe[1][2]);
      x_mid[2]   = q12*(xrefe[0][4] + xrefe[0][6]);
      y_mid[2]   = q12*(xrefe[1][4] + xrefe[1][6]);
      flag = 1;
      width=L[1];
     }
     else if (L[1]>L[0])
     {
      x_mid[0]   = q12*(xrefe[0][0] + xrefe[0][1]);
      y_mid[0]   = q12*(xrefe[1][0] + xrefe[1][1]);
      x_mid[1]   = q12*(xrefe[0][2] + xrefe[0][3]);
      y_mid[1]   = q12*(xrefe[1][2] + xrefe[1][3]);
      x_mid[2]   = q12*(xrefe[0][5] + xrefe[0][7]);
      y_mid[2]   = q12*(xrefe[1][5] + xrefe[1][7]);
      flag = 2;
      width=L[0];
     }
     help      = (x_mid[0]-x_mid[1])/(x_mid[0]-x_mid[2]);
     c_parabel = (y_mid[0]-y_mid[1]-(y_mid[0]-y_mid[2])*help)/
                 (x_mid[0]*x_mid[0]-x_mid[1]*x_mid[1]-
                  (x_mid[0]*x_mid[0]-x_mid[2]*x_mid[2])*help);
     b_parabel = (y_mid[0]-y_mid[1]-c_parabel*(x_mid[0]*x_mid[0]-x_mid[1]*x_mid[1]))/
                 (x_mid[0]-x_mid[1]);
     
break;
}
Thick = ele->e.interf->thick;
nir   = ele->e.interf->nGP;

/*-------------------------------------------- reinitalization to zero---*/
amzero(estif_global);
estif     = estif_global->a.da;
/*------------------------------------------------ If Dynamic, Mass=0 ---*/
if (emass_global) 
{
   imass = 1;
   amzero(emass_global);
} 
/*=======================================================================*/
if(genprob.graderw<=0)
{
 ip = -1;
/*================================================= integration loop ===*/
 for (lr=0; lr<nir; lr++)
 {   
   ip++;
   /*================================ gaussian point and weight at it ===*/
   e1   = data->xgr[lr];
   facr = data->wgtr[lr];
   /*----------------------------------------------- ansatzfunctions ---*/
   if_funcderiv(e1,ele->distyp,x_mid,y_mid,b_parabel,c_parabel,functd,&cod,&sid,&detd);
   /*-------------------------------------------- integration factor ---*/
   facd  = facr * detd * Thick; 
    /*------------------------------------------ calculate operator B ---*/
   amzero(&bopd_a);
   if_bop(ele->distyp,bopd,functd,cod,sid,flag);
   /*--------------------------------------------- call material law ---*/
   if (imass == 1) 
   { 
     if_mat_dyn(ele,mat,bopd,D,T,ip,istore,newval);
   } 
   else 
   {
     if_mat(ele,mat,bopd,D,T,ip,istore,newval);
   }
   /*-------------------------------------------------------------------*/
   /*-------------------------------------------------------------------*/
   if(istore==0)
   {
     /*--------------------------------- element stiffness matrix ke ---*/
      if_ke(ield,flag,estif,bopd,D,facd);
     /*--------------------------------------- internal nodal forces ---*/        
      if (force)
      { 
        if_fint(ield,T,facd,bopd,force);
      }                    
    } 
 }/*================================================ end of loop over lr */
}
/*=======================================================================*/
if(genprob.graderw>0)
{
 amzero(&Kdd_a);
 for (a=0; a<16; a++)  fintd[a] = 0.0;
 ip = -1;
/*============================ integration loop for balance equation ===*/
 for (lr=0; lr<nir; lr++)
 {   
   ip++;
   /*=============================== gaussian point and weight at it ===*/
   e1   = data->xgr[lr];
   facr = data->wgtr[lr];
   /*----------------------------------------------- ansatzfunctions ---*/
   if_funcderiv(e1,ele->distyp,x_mid,y_mid,b_parabel,c_parabel,functd,&cod,&sid,&detd);
   /*-------------------------------------------- integration factor ---*/
   facd  = facr * detd * Thick; 
    /*----------------------------------------- calculate operator B ---*/
   amzero(&bopd_a);
   if_bop(ele->distyp,bopd,functd,cod,sid,flag);
   /*--------------------------------------------- call material law ---*/
   if (imass == 1) 
   { 
     if_mat_dyn(ele,mat,bopd,D,T,ip,istore,newval);
   } 
   else 
   {
     if_mat(ele,mat,bopd,D,T,ip,istore,newval);
   }
   /*-------------------------------------------------------------------*/
   /*-------------------------------------------------------------------*/
   if(istore==0)
   {
     /*--------------------------------- element stiffness matrix ke ---*/
      if_ke(ield,flag,Kdd,bopd,D,facd);
     /*--------------------------------------- internal nodal forces ---*/        
      if (force)
      { 
        if_fint(ield,T,facd,bopd,fintd);
      }                    
    } 
 }/*================================================ end of loop over lr */
 if(istore==0)
 {
   if_permstiff(estif,Kdd,iele,ield);
   if(force)
   {
     if_permforce(force,fintd,iele,ield);
   }
 }
}
/*=======================================================================*/


/*----------------------------------------------------------------------*/
end:
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return; 
} /* end of ifstatic_ke */


/*----------------------------------------------------------------------*/
#endif /*D_INTERF*/
/*! @} (documentation module close)*/
