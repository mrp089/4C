/*!----------------------------------------------------------------------
\file
\brief evaluate galerkin part of stiffness matrix

------------------------------------------------------------------------*/
#ifdef D_FLUID3 
#include "../headers/standardtypes.h"
#include "fluid3_prototypes.h"
/*!---------------------------------------------------------------------                                         
\brief evaluate galerkin part of Kvv

<pre>                                                         genk 05/02

In this routine the galerkin part of matrix Kvv is calculated:

    /
   |  2 * nue * eps(v) : eps(u)   d_omega
  /

    /
   |  v * u_old * grad(u)     d_omega
  /

    /
   |  v * u * grad(u_old)     d_omega
  /

see also dissertation of W.A. Wall chapter 4.4 'Navier-Stokes Loeser'
  
NOTE: there's only one elestif  			    
      --> Kvv is stored in estif[0..(3*iel-1)][0..(3*iel-1)]
      
</pre>
\param  *dynvar    FLUID_DYN_CALC  (i)
\param **estif     DOUBLE	   (i/o)  ele stiffness matrix
\param  *velint    DOUBLE	   (i)    vel at INT point
\param **vderxy    DOUBLE	   (i)    global vel derivatives
\param  *funct     DOUBLE	   (i)    nat. shape funcs
\param **derxy     DOUBLE	   (i)    global coord. deriv.
\param   fac 	   DOUBLE	   (i)    weighting factor	      
\param   visc      DOUBLE	   (i)    fluid viscosity	     
\param   iel	   INT  	   (i)	  number of nodes of act. ele
\return void                                                                       

------------------------------------------------------------------------*/
void f3_calkvv(
                FLUID_DYN_CALC  *dynvar,
		DOUBLE         **estif,
		DOUBLE          *velint,
		DOUBLE         **vderxy,
		DOUBLE          *funct,
		DOUBLE         **derxy,
		DOUBLE           fac,
		DOUBLE           visc,
		INT              iel		 
              )
{
/*----------------------------------------------------------------------*
 | NOTATION:                                                            |
 |   irow - row number in element matrix                                |
 |   icol - column number in element matrix                             |
 |   irn  - row node: number of node considered for matrix-row          |
 |   icn  - column node: number of node considered for matrix column    |  
/*----------------------------------------------------------------------*/
INT     irow, icol,irn,icn;
DOUBLE  c,aux;

#ifdef DEBUG 
dstrc_enter("f3_calkvv");
#endif	

c=fac*visc;

/*----------------------------------------------------------------------*
   Calculate full Galerkin part of matrix K:
    /
   |  2 * nue * eps(v) : eps(u)   d_omega
  /
 *----------------------------------------------------------------------*/
 
icol=0;
for (icn=0;icn<iel;icn++)
{
   irow=0;
   for (irn=0;irn<iel;irn++)
   {
      aux =   derxy[0][irn]*derxy[0][icn] \
            + derxy[1][irn]*derxy[1][icn] \
	    + derxy[2][irn]*derxy[2][icn] ;

      estif[irow][icol]     += c*(aux+derxy[0][irn]*derxy[0][icn]); 	     
      estif[irow+1][icol]   += c*(    derxy[0][irn]*derxy[1][icn]);
      estif[irow+2][icol]   += c*(    derxy[0][irn]*derxy[2][icn]);

      estif[irow][icol+1]   += c*(    derxy[1][irn]*derxy[0][icn]);
      estif[irow+1][icol+1] += c*(aux+derxy[1][irn]*derxy[1][icn]);
      estif[irow+2][icol+1] += c*(    derxy[1][irn]*derxy[2][icn]);

      estif[irow][icol+2]   += c*(    derxy[2][irn]*derxy[0][icn]);
      estif[irow+1][icol+2] += c*(    derxy[2][irn]*derxy[1][icn]);
      estif[irow+2][icol+2] += c*(aux+derxy[2][irn]*derxy[2][icn]);
      irow += 3;
   } /* end of loop over irn */
   icol += 3;
} /* end of loop over icn */

/*----------------------------------------------------------------------*
   Calculate full Galerkin part of matrix Nc(u):
    /
   |  v * u_old * grad(u)     d_omega
  /
 *----------------------------------------------------------------------*/

if(dynvar->nic != 0) /* evaluate for Newton- and fixed-point-like-iteration */
{
   icol=0;
   for (icn=0;icn<iel;icn++)
   {
      irow=0;  
      for (irn=0;irn<iel;irn++)
      {
         aux = (velint[0]*derxy[0][icn] + velint[1]*derxy[1][icn] \
              + velint[2]*derxy[2][icn])*funct[irn]*fac;
	 estif[irow][icol]     += aux;
         estif[irow+1][icol+1] += aux;
         estif[irow+2][icol+2] += aux;
         irow += 3;	 
      } /* end of loop over irn */
      icol += 3;
   } /* end of loop over icn */
} /* endif (dynvar->nic != 0) */

/*----------------------------------------------------------------------*
   Calculate full Galerkin part of matrix Nr(u):
    /
   |  v * u * grad(u_old)     d_omega
  /
 *----------------------------------------------------------------------*/

if (dynvar->nir != 0) /* evaluate for Newton iteraton */
{
   icol=0;
   for (icn=0;icn<iel;icn++)
   {
      irow=0;  
      for (irn=0;irn<iel;irn++)
      {
         aux = funct[irn]*funct[icn]*fac;
	 estif[irow][icol]     += aux*vderxy[0][0];
	 estif[irow+1][icol]   += aux*vderxy[1][0];
	 estif[irow+2][icol]   += aux*vderxy[2][0];

	 estif[irow][icol+1]   += aux*vderxy[0][1];
	 estif[irow+1][icol+1] += aux*vderxy[1][1];
	 estif[irow+2][icol+1] += aux*vderxy[2][1];

	 estif[irow][icol+2]   += aux*vderxy[0][2];
	 estif[irow+1][icol+2] += aux*vderxy[1][2];
	 estif[irow+2][icol+2] += aux*vderxy[2][2];
	 irow += 3;
      } /* end of loop over irn */
      icol += 3;
   } /* end of loop over icn */
} /* endif (dynvar->nir != 0) */

/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();

#endif
return;
} /* end of f3_calkvv */	

/*!---------------------------------------------------------------------                                         
\brief evaluate galerkin part of Kvp

<pre>                                                         genk 05/02

In this routine the galerkin part of matrix Kvp is calculated:

    /
   |  - div(v) * p     d_omega
  /

    /
   | - q * div(u)      d_omega
  / 

see also dissertation of W.A. Wall chapter 4.4 'Navier-Stokes Loeser'
  
NOTE: there's only one elestif  				    
      --> Kvp is stored in estif[(0..(3*iel-1)][(4*iel)..(4*iel-1)] 
      --> Kpv is stored in estif[((3*iel)..(4*iel-1)][0..(3*iel-1)] 
      
</pre>
\param **estif     DOUBLE	   (i/o)  ele stiffness matrix
\param  *funct     DOUBLE	   (i)    nat. shape funcs
\param **derxy     DOUBLE	   (i)    global coord. deriv.
\param   fac       DOUBLE	   (i)    weighting factor
\param   iel       INT  	   (i)    number of nodes of act. ele
\return void                                                                       

------------------------------------------------------------------------*/
void f3_calkvp(
		DOUBLE         **estif,
		DOUBLE          *funct,
		DOUBLE         **derxy,
		DOUBLE           fac,
		INT              iel                
              )
{
/*----------------------------------------------------------------------*
 | NOTATION:                                                            |
 |   irow - row number in element matrix                                |
 |   icol - column number in element matrix                             |
 |   irn  - row node: number of node considered for matrix-row          |
 |   ird  - row dim.: number of spatial dimension at row node           |  
 |   posc - since there's only one full element stiffness matrix the    |
 |          column number has to be changed!                            |
/*----------------------------------------------------------------------*/
INT     irow, icol,irn,ird;  
INT     posc;
DOUBLE  aux;

#ifdef DEBUG 
dstrc_enter("f3_calkvp");
#endif		


/*----------------------------------------------------------------------*
   Calculate full Galerkin part of matrix Kvp:
    /
   |  - div(v) * p     d_omega
  /
  
   and matrix Kpv: 
    /
   | - q * div(u)      d_omega
  /      
 *----------------------------------------------------------------------*/

for (icol=0;icol<iel;icol++)
{
  irow=-1;
  posc = icol + 3*iel;
  for (irn=0;irn<iel;irn++)
  {
     for(ird=0;ird<3;ird++)
     {      
	aux = funct[icol]*derxy[ird][irn]*fac;
	irow++;
	estif[irow][posc] -= aux;
	estif[posc][irow] -= aux;		
     } /* end of loop over ird */
  } /* end of loop over irn */
} /* end of loop over icol */

/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif

return;
} /* end of f3_calkvp */

/*!---------------------------------------------------------------------                                         
\brief evaluate galerkin part of Mvv

<pre>                                                         genk 05/02

In this routine the galerkin part of matrix Mvv is calculated:

    /
   |  v * u    d_omega
  /

see also dissertation of W.A. Wall chapter 4.4 'Navier-Stokes Loeser'
  
NOTE: there's only one elestif  			     
      --> Mvv is stored in estif[0..(3*iel-1)][0..(3*iel-1)] 
      
</pre>
\param **emass     DOUBLE	   (i/o)  ele mass matrix
\param  *funct     DOUBLE	   (i)    nat. shape funcs
\param   fac       DOUBLE	   (i)    weighting factor
\param   iel       INT   	   (i)    number of nodes of act. ele
\return void                                                                       

------------------------------------------------------------------------*/
void f3_calmvv(
		DOUBLE         **estif,
		DOUBLE          *funct,
		DOUBLE           fac,
		INT              iel  
              )
{
/*----------------------------------------------------------------------*
 | NOTATION:                                                            |
 |   irow - row number in element matrix                                |
 |   icol - column number in element matrix                             |
 |   irn  - row node: number of node considered for matrix-row          |
 |   icn  - column node: number of node considered for matrix column    |  
/*----------------------------------------------------------------------*/
INT     irow, icol,irn,icn;  
INT     nvdfe;             /* number of velocity dofs of actual element */
DOUBLE  aux;

#ifdef DEBUG 
dstrc_enter("f3_calmvv");
#endif		

nvdfe = NUM_F3_VELDOF*iel;

/*----------------------------------------------------------------------*
   Calculate full Galerkin part of matrix Mvv:
    /
   |  v * u    d_omega
  /
 *----------------------------------------------------------------------*/

icn=-1;
for(icol=0;icol<nvdfe;icol+=3)
{
   icn++;
   irn=-1;
   for(irow=0;irow<nvdfe;irow+=3)
   {
      irn++;
      aux = funct[icn]*funct[irn]*fac;
      estif[irow][icol]     += aux;
      estif[irow+1][icol+1] += aux;
      estif[irow+2][icol+2] += aux;
   }
} 
 
#ifdef DEBUG 
dstrc_exit();
#endif

/*----------------------------------------------------------------------*/
return;
} /* end of f3_calmvv */

#endif
