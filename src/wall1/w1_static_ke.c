#include "../headers/standardtypes.h"
#include "wall1.h"
#include "wall1_calc.h"
/*----------------------------------------------------------------------*
 | integration of linear stiffness ke for wall1 element      al 9/01    |
 *----------------------------------------------------------------------*/
void w1static_ke(ELEMENT   *ele, 
                    W1_DATA   *data, 
                    MATERIAL  *mat,
                    ARRAY     *estif_global, 
                    int        init)
{
int                 i,j,k;            /* some loopers */
int                 nir,nis;          /* num GP in r/s/t direction */
int                 lr, ls;           /* loopers over GP */
int                 iel;              /* numnp to this element */
int                 nd;
const int           numdf =2;
const int           numeps=3;

double              fac;
double              e1,e2,e3;         /*GP-coords*/
double              facr,facs,fact;   /* weights at GP */
double              xnu;              /* value of shell shifter */
double              weight;

static ARRAY    D_a;      /* material tensor */     
static double **D;         
static ARRAY    funct_a;  /* shape functions */    
static double  *funct;     
static ARRAY    deriv_a;  /* derivatives of shape functions */   
static double **deriv;     
static ARRAY    xjm_a;    /* jacobian matrix */     
static double **xjm;         
static ARRAY    bop_a;    /* B-operator */   
static double **bop;       
static double **estif;    /* element stiffness matrix ke */

double det;

/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("w1static_ke");
#endif
/*------------------------------------------------- some working arrays */
if (init==1)
{
funct     = amdef("funct"  ,&funct_a,MAXNOD_WALL1,1 ,"DV");       
deriv     = amdef("deriv"  ,&deriv_a,2,MAXNOD_WALL1 ,"DA");       
D         = amdef("D"      ,&D_a   ,6,6             ,"DA");           
xjm       = amdef("xjm"    ,&xjm_a ,numdf,numdf     ,"DA");           

bop       = amdef("bop"  ,&bop_a ,numeps,(numdf*MAXNOD_WALL1),"DA");           
goto end;
}
/*------------------------------------------- integration parameters ---*/
w1intg(ele,data,1);
/*-------------- some of the fields have to be reinitialized to zero ---*/
amzero(estif_global);
estif     = estif_global->a.da;
/*------------------------------------------- integration parameters ---*/
nir     = ele->e.w1->nGP[0];
nis     = ele->e.w1->nGP[1];
iel     = ele->numnp;
nd      = numdf * iel;
/*================================================ integration loops ===*/
for (lr=0; lr<nir; lr++)
{
   /*=============================== gaussian point and weight at it ===*/
   e1   = data->xgrr[lr];
   facr = data->wgtr[lr];
   for (ls=0; ls<nis; ls++)
   {
      /*============================ gaussian point and weight at it ===*/
      e2   = data->xgss[ls];
      facs = data->wgts[ls];
      /*------------------------- shape functions and their derivatives */
      w1_funct_deriv(funct,deriv,e1,e2,ele->distyp,1);
      /*------------------------------------ compute jacobian matrix ---*/       
      w1_jaco (funct,deriv,xjm,&det,ele,iel);                         
      fac = facr * facs * det; 
      /*--------------------------------------- calculate operator B ---*/
      amzero(&bop_a);
      w1_bop(bop,deriv,xjm,det,iel);
      /*------------------------------------------ call material law ---*/
      w1_mat_linel(mat->m.lin_el,D);
      /*-------------------------------- elastic stiffness matrix ke ---*/
      w1_keku(estif,bop,D,fac,nd,numeps);
   }/*============================================= end of loop over ls */ 
}/*================================================ end of loop over lr */
end:
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return; 
} /* end of w1static_ke */
/*----------------------------------------------------------------------*/
