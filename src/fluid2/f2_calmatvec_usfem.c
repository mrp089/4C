/*!----------------------------------------------------------------------
\file
\brief evaluate 2D fluid coefficient matrix

<pre>
Maintainer: Christiane Foerster
            foerster@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/foerster/
            0711 - 685-6572
</pre>

------------------------------------------------------------------------*/
/*!
\addtogroup FLUID2
*//*! @{ (documentation module open)*/
#ifdef D_FLUID2
#include "../headers/standardtypes.h"
/*#include "fluid2_new_prototypes.h" */
#include "fluid2.h"
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | pointer to allocate dynamic variables if needed                      |
 | dedfined in global_control.c                                         |
 | ALLDYNA               *alldyn;                                       |
 *----------------------------------------------------------------------*/
extern ALLDYNA      *alldyn;
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | global variable GENPROB genprob is defined in global_control.c       |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;

static FLUID_DYNAMIC *fdyn;

/*!---------------------------------------------------------------------
\brief evaluate fluid coefficient matrix

<pre>                                                        chfoe 04/04

In this routine the Gauss point contributions to the elemental coefficient
matrix of a stabilised fluid2 element are calculated. The procedure is
based on the Rothe method of first integrating in time. Hence the
resulting terms include coefficients containing time integration variables
such as theta or delta t which are represented by 'timefac'.

The routine was completed to contain ALE-terms also.         chfoe 11/04

The stabilisation is based on the residuum:

R_M = u + timefac u * grad u - timefac * 2 nu div epsilon(u)
    + timefac grad p - rhsint

R_C = div u

The corresponding weighting operators are
L_M = v + timefac u_old * grad v + timefac v * grad u_old
    - timefac * 2 nu alpha div epsilon (v) + timefac beta grad q

L_C = div v

where alpha = -1
      beta  = -1
are sign regulating factors and rhsint differs for different time
These factores are worked in now and cannot be changed any more.

integration schemes:

One-step-Theta:
rhsint = u_old + Theta dt f + (1-Theta) acc_old

BDF2:

generalised alpha:


The stabilisation by means of the momentum residuum R_M is of the unusual
type:
   Galerkin parts MINUS sum over elements (stabilising parts)
The stabilisation by means of the continuity equation R_C is done in the
usual way:
   Galerkin parts PLUS sum over elements (stabilising parts)

The calculation proceeds as follows.
1) obtain single (linearised) operators of R_M, R_C, L_M and L_C
2) build Galerkin terms from them
3) build stabilising terms from them
4) build Galerkin and stabilising terms of RHS

NOTE: u_old represents the last iteration value. (The most recent one
      we've got!)

NOTE: Galerkin and stabilisation matrices are calculated within one
      routine.

NOTE: In order to increase the performance plenty of terms are concentrated
      and worked into each other. A lengthy version of the file is available
      from the author.


Notational remarks:

                   /              \
                  | u_x,x   u_x,y |
vderxy = grad u = |               |
                  | u_y,x   u_y,y |
                  \               /

           /                         \
          | u_x,xx   u_x,yy   u_x,xy |
vderxy2 = |                          |
          | u_y,xx   u_y,yy   u_y,xy |
          \                          /

for further comments see comment lines within code.


</pre>
\param **estif      DOUBLE        (o)   ele stiffness matrix
\param  *eforce     DOUBLE        (o)   ele force vector
\param  *velint     DOUBLE        (i)   vel at INT point
\param  *histvec    DOUBLE        (i)   rhs at INT point
\param  *gridvint   DOUBLE        (i)   gridvel at INT point
\param **vderxy     DOUBLE        (i)   global vel derivatives
\param  *vderxy2    DOUBLE        (i)   2nd global vel derivatives
\param  *funct      DOUBLE        (i)   nat. shape funcs
\param **derxy      DOUBLE        (i)   global coord. deriv.
\param **derxy2     DOUBLE        (i)   2nd global coord. deriv.
\param  *edeadng    DOUBLE        (i)   dead load at time n+1
\param   fac 	    DOUBLE        (i)   weighting factor
\param   visc       DOUBLE        (i)   fluid viscosity
\param   iel	    INT           (i)   number of nodes of act. ele
\param  *hasext     INT           (i)   flag, if element has volume load
\param   isale      INT           (i)   flag, if ALE or EULER
\return void

------------------------------------------------------------------------*/
void f2_calmat( DOUBLE **estif,
		DOUBLE  *eforce,
		DOUBLE  *velint,
		DOUBLE   histvec[2],
		DOUBLE   gridvint[2],
		DOUBLE **vderxy,
                DOUBLE **vderxy2,
                DOUBLE   gradp[2],
		DOUBLE  *funct,
		DOUBLE **derxy,
		DOUBLE **derxy2,
                DOUBLE  *edeadng,
		DOUBLE   fac,    
		DOUBLE   visc,
		INT      iel,
                INT     *hasext,
                INT      isale
              )
{
INT     i, j, ri, ci;
DOUBLE  timefac;    /* One-step-Theta: timefac = theta*dt
                       BDF2:           timefac = 2/3 * dt               */
DOUBLE  dt;         /* time step size*/
DOUBLE  aux;
DOUBLE  auxmat[2][2];
DOUBLE  tau_M, tau_C;             /* stabilisation parameter            */
DOUBLE  tau_Mp;             /* stabilisation parameter            */
DOUBLE  viscs2[2][2*MAXNOD]; /* viscous term incluiding 2nd derivatives */
DOUBLE  viscous[2][2][2*MAXNOD];   /* viscous term partially integrated */
DOUBLE  conv_c[MAXNOD];    /* linearisation of convect, convective part */
DOUBLE  conv_g[MAXNOD];          /* linearisation of convect, grid part */
DOUBLE  conv_r[2][2*MAXNOD]; /* linearisation of convect, reactive part */
DOUBLE  div[2*MAXNOD];             /* divergence of u or v              */
DOUBLE  ugradv[MAXNOD][2*MAXNOD];  /* linearisation of u * grad v       */
DOUBLE  conv_old[2]; /* convective term evalaluated with old velocities */
DOUBLE  visc_old[2]; /* viscous term evaluated with old velocities      */
DOUBLE  rhsint[2];   /* total right hand side terms at int.-point       */

DOUBLE  time2nue, timetauM, timetauMp, ttimetauM, ttimetauMp, timefacfac;

/*----------------------------------------------------------------------*/
#ifdef DEBUG
dstrc_enter("f2_calmat");
#endif
/*========================== initialisation ============================*/
fdyn = alldyn[genprob.numff].fdyn;

tau_M  = fdyn->tau[0]*fac;
tau_Mp = fdyn->tau[0]*fac;
tau_C  = fdyn->tau[2]*fac;

timefac = fdyn->thsl;
dt      = fdyn->dta;

/* integration factors and koefficients of single terms */
time2nue  = timefac * 2.0 * visc;
timetauM   = timefac * tau_M;
timetauMp  = timefac * tau_Mp;

ttimetauM  = timefac * timetauM;
ttimetauMp = timefac * timetauMp;
timefacfac = timefac * fac;


/*------------------------- evaluate rhs vector at integration point ---*/
if (*hasext)
{
   rhsint[0] = timefac * edeadng[0] + histvec[0];
   rhsint[1] = timefac * edeadng[1] + histvec[1];
}
else
{
   rhsint[0] = histvec[0];
   rhsint[1] = histvec[1];
}

/*----------------- get numerical representation of single operators ---*/

/* Convective term  u_old * grad u_old: */
conv_old[0] = vderxy[0][0] * velint[0] + vderxy[0][1] * velint[1];
conv_old[1] = vderxy[1][0] * velint[0] + vderxy[1][1] * velint[1];

/* Viscous term  div epsilon(u_old) */
visc_old[0] = 0.5 * (2.0*vderxy2[0][0] + vderxy2[0][1] + vderxy2[1][2]);
visc_old[1] = 0.5 * (2.0*vderxy2[1][1] + vderxy2[1][0] + vderxy2[0][2]);


for (i=0; i<iel; i++) /* loop over nodes of element */
{
   /* Reactive term  u:  funct */
   /* linearise convective term */

   /*--- convective part u_old * grad (funct) --------------------------*/
   /* u_old_x * N,x  +  u_old_y * N,y   with  N .. form function matrix */
   conv_c[i] = derxy[0][i] * velint[0] + derxy[1][i] * velint[1] ;

   /*--- convective grid part u_G * grad (funct) -----------------------*/
   /* u_old_x * N,x  +  u_old_y * N,y   with  N .. form function matrix */
   if(isale)
   {
     conv_g[i] = - derxy[0][i] * gridvint[0] - derxy[1][i] * gridvint[1];
   }

   /*--- reactive part funct * grad (u_old) ----------------------------*/
   /* /                          \
      |  u_old_x,x   u_old_x,y   |
      |                          | * N   with  N .. form function matrix
      |  u_old_y,x   u_old_y,y   |
      \                         /                                       */
   conv_r[0][2*i]   = vderxy[0][0]*funct[i];
   conv_r[0][2*i+1] = vderxy[0][1]*funct[i];
   conv_r[1][2*i]   = vderxy[1][0]*funct[i];
   conv_r[1][2*i+1] = vderxy[1][1]*funct[i];

   /*--- viscous term  - grad * epsilon(u): ----------------------------*/
   /*   /                              \
      1 |  2 N_x,xx + N_x,yy + N_y,xy  |    with N_x .. x-line of N
    - - |                              |         N_y .. y-line of N
      2 |  N_y,xx + N_x,yx + 2 N_y,yy  |
        \                             /                                 */
   viscs2[0][2*i]   = - 0.5 * ( 2.0 * derxy2[0][i] + derxy2[1][i] );
   viscs2[0][2*i+1] = - 0.5 * ( derxy2[2][i] );
   viscs2[1][2*i]   = - 0.5 * ( derxy2[2][i] );
   viscs2[1][2*i+1] = - 0.5 * ( derxy2[0][i] + 2.0 * derxy2[1][i] );

   /*--- viscous term (after integr. by parts) -------------------------*/
   /*   /                            \
      1 |  2 N_x,x    N_x,y + N_y,x  |    with N_x .. x-line of N
      - |                            |         N_y .. y-line of N
      2 |  N_y,x + N_x,y    2 N_y,y  |
        \                           /                                   */
   viscous[0][0][2*i]   = derxy[0][i];
   viscous[0][0][2*i+1] = 0.0;                /* 1st index:             */
   viscous[0][1][2*i]   = 0.5 * derxy[1][i];  /*   line of epsilon      */
   viscous[0][1][2*i+1] = 0.5 * derxy[0][i];  /* 2nd index:             */
   viscous[1][0][2*i]   = 0.5 * derxy[1][i];  /*   column of epsilon    */
   viscous[1][0][2*i+1] = 0.5 * derxy[0][i];  /* 3rd index:             */
   viscous[1][1][2*i+1] = derxy[1][i];        /*   elemental vel dof    */
   viscous[1][1][2*i]   = 0.0;

   /* pressure gradient term derxy, funct without or with integration   *
    * by parts, respectively                                            */

   /*--- divergence u term ---------------------------------------------*/
   div[2*i]   = derxy[0][i];
   div[2*i+1] = derxy[1][i];

   /*--- ugradv-Term ---------------------------------------------------*/
   /*
     /                                                          \
     |  N1*N1,x  N1*N1,y  N2*N1,x  N2*N1,y  N3*N1,x ...       . |
     |                                                          |
     |  N1*N2,x  N1*N2,y  N2*N2,x  N2*N2,y  N3*N2,x ...       . |
     |                                                          |
     |  N1*N3,x  N1*N3,y  N2*N3,x  N2*N3,y  N3*N3,x ...       . |
     |                                           .              |
     |  . . .                                        .          |
     |                                                  Ni*Ni,y |
     \                                                          /       */
   /* remark: vgradu = ugradv^T */
   for (j=0; j<iel; j++)
   {
      ugradv[i][2*j]   = derxy[0][i] * funct[j];
      ugradv[i][2*j+1] = derxy[1][i] * funct[j];
   }

}

/*--------------------------------- now build single stiffness terms ---*/
for (ri=0; ri<iel; ri++)      /* row index */
{
   for (ci=0; ci<iel; ci++)   /* column index */
   {
      /************** integrate element coefficient matrix **************/
/*===================== GALERKIN part of the matrix ====================*/

      /* a concentration of the following terms: */
      /* 'mass matrix' (u,v) */
      /* N_c (u_old * grad u, v) */
      /* N_r (u * grad u_old, v) */
      aux = funct[ri] * ( funct[ci]*fac + timefacfac*conv_c[ci] );
      estif[ri*3][ci*3]     += funct[ri] * conv_r[0][2*ci] * timefacfac + aux;
      estif[ri*3][ci*3+1]   += funct[ri] * conv_r[0][2*ci+1] * timefacfac;
      estif[ri*3+1][ci*3]   += funct[ri] * conv_r[1][2*ci] * timefacfac;
      estif[ri*3+1][ci*3+1] += funct[ri] * conv_r[1][2*ci+1] * timefacfac + aux;
      /* ALE: N_c (-u_G * grad u, v) */
      if(isale)
      {
         aux = timefacfac * funct[ri] * conv_g[ci];
         estif[ri*3][ci*3]     += aux;
         estif[ri*3+1][ci*3+1] += aux;
      }

      /* K (2 * nu * epsilon(u), epsilon(v)) */
      auxmat[0][0] = viscous[0][0][ri*2]   * viscous[0][0][ci*2]
                   + viscous[0][1][ri*2]   * viscous[1][0][ci*2]
                   + viscous[1][0][ri*2]   * viscous[0][1][ci*2]
                   + viscous[1][1][ri*2]   * viscous[1][1][ci*2];
      auxmat[0][1] = viscous[0][0][ri*2]   * viscous[0][0][ci*2+1]
                   + viscous[0][1][ri*2]   * viscous[1][0][ci*2+1]
                   + viscous[1][0][ri*2]   * viscous[0][1][ci*2+1]
                   + viscous[1][1][ri*2]   * viscous[1][1][ci*2+1];
      auxmat[1][0] = viscous[0][0][ri*2+1] * viscous[0][0][ci*2]
                   + viscous[0][1][ri*2+1] * viscous[1][0][ci*2]
                   + viscous[1][0][ri*2+1] * viscous[0][1][ci*2]
                   + viscous[1][1][ri*2+1] * viscous[1][1][ci*2];
      auxmat[1][1] = viscous[0][0][ri*2+1] * viscous[0][0][ci*2+1]
                   + viscous[0][1][ri*2+1] * viscous[1][0][ci*2+1]
                   + viscous[1][0][ri*2+1] * viscous[0][1][ci*2+1]
                   + viscous[1][1][ri*2+1] * viscous[1][1][ci*2+1];
      aux = time2nue * fac;
      estif[ri*3][ci*3]     += auxmat[0][0]*aux;
      estif[ri*3][ci*3+1]   += auxmat[0][1]*aux;
      estif[ri*3+1][ci*3]   += auxmat[1][0]*aux;
      estif[ri*3+1][ci*3+1] += auxmat[1][1]*aux;
      /* G (- div v, p) */
      estif[ri*3][ci*3+2]   -= timefacfac * derxy[0][ri] * funct[ci];
      estif[ri*3+1][ci*3+2] -= timefacfac * derxy[1][ri] * funct[ci];
      /* G^T ( div u, q) */
      estif[ri*3+2][ci*3]   += timefacfac * funct[ri] * derxy[0][ci];
      estif[ri*3+2][ci*3+1] += timefacfac * funct[ri] * derxy[1][ci];

/*=================== Stabilisation part of the matrix =================*/


      /*--- CONVECTIVE stabilisation ---*/
      /* a concentration of the following two terms: */
      /* tau_M*timefac*(u, u_old * grad v) */
      /* -tau_M*timefac*timefac*(u_old * grad u, u_old * grad v) */
      aux = conv_c[ri] * (timetauM * funct[ci] + ttimetauM * conv_c[ci]);
      estif[ri*3][ci*3]     += aux;
      estif[ri*3+1][ci*3+1] += aux;
      /* ALE: -tau_M*timefac*timefac*(-u_G * grad u, u_old * grad v) */
      if(isale)
      {
         aux = ttimetauM * conv_c[ri] * conv_g[ci];
         estif[ri*3][ci*3]     += aux;
         estif[ri*3+1][ci*3+1] += aux;
      }
      /* a concentration of the following two terms: */
      /* -tau_M*timefac*timefac*(u * grad u_old, u_old * grad v) */
      /* tau_M*timefac*timefac*2*nu*(div epsilon(u), u_old * grad v) */
      aux = timetauM * time2nue;
      estif[ri*3][ci*3]     += conv_c[ri] * ( conv_r[0][2*ci]*ttimetauM
                                             +viscs2[0][2*ci]*aux );
      estif[ri*3][ci*3+1]   += conv_c[ri] * ( conv_r[0][2*ci+1]*ttimetauM
                                             +viscs2[0][2*ci+1]*aux );
      estif[ri*3+1][ci*3]   += conv_c[ri] * ( conv_r[1][2*ci]*ttimetauM
                                             +viscs2[1][2*ci]*aux );
      estif[ri*3+1][ci*3+1] += conv_c[ri] * ( conv_r[1][2*ci+1]*ttimetauM
                                             +viscs2[1][2*ci+1]*aux );
      /* -tau_M*timefac*timefac*(grad p, u_old * grad v) */
      estif[ri*3][ci*3+2]   += conv_c[ri] * derxy[0][ci] * ttimetauM;
      estif[ri*3+1][ci*3+2] += conv_c[ri] * derxy[1][ci] * ttimetauM;

      /*--- ALE only: CONVECTIVE GRID stabilisation ---*/
      if(isale)
      {
         /* a concentration of the following terms: */
         /* -tau_M*timefac*(u, -u_G * grad v) */
         /* -tau_M*timefac*timefac*(u_old * grad u, -u_G * grad v) */
         /* -tau_M*timefac*timefac*(-u_G * grad u, -u_G * grad v) */
         aux = conv_g[ri] *
              (ttimetauM*(conv_c[ci]+conv_g[ci]) + timetauM*funct[ci]);
         estif[ri*3][ci*3]     += aux;
         estif[ri*3+1][ci*3+1] += aux;
         /* a concentration of the following two terms: */
         /* -tau_M*timefac*timefac*(u * grad u_old, -u_G * grad v) */
         /* tau_M*timefac*timefac*2*nu*(div epsilon(u), -u_G * grad v) */
         aux = timetauM * time2nue;
         estif[ri*3][ci*3]     += conv_g[ri] * ( conv_r[0][2*ci]*ttimetauM
                                                +viscs2[0][2*ci] * aux );
         estif[ri*3][ci*3+1]   += conv_g[ri] * ( conv_r[0][2*ci+1]*ttimetauM
                                                +viscs2[0][2*ci+1] * aux );
         estif[ri*3+1][ci*3]   += conv_g[ri] * ( conv_r[1][2*ci]*ttimetauM
                                                +viscs2[1][2*ci] * aux );
         estif[ri*3+1][ci*3+1] += conv_g[ri] * ( conv_r[1][2*ci+1]*ttimetauM
                                                +viscs2[1][2*ci+1] * aux );
         /* -tau_M*timefac*timefac*(grad p, -u_G * grad v) */
         estif[ri*3][ci*3+2]   += conv_g[ri] * derxy[0][ci] * ttimetauM;
         estif[ri*3+1][ci*3+2] += conv_g[ri] * derxy[1][ci] * ttimetauM;
      }

      /*--- DIFFUSION part of stabilisation ---*/
      /* a concentration of the following two terms: */
      /* tau_M*timefac*2*nu*(u, div epsilon(v)) */
      /* tau_M*timefac*timefac*2*nu*(u_old * grad u, div epsilon(v)) */
      aux = time2nue * ( funct[ci]*tau_Mp + conv_c[ci] * timetauMp );
      estif[ri*3][ci*3]     += viscs2[0][2*ri] * aux;
      estif[ri*3][ci*3+1]   += viscs2[1][2*ri] * aux;
      estif[ri*3+1][ci*3]   += viscs2[0][2*ri+1] * aux;
      estif[ri*3+1][ci*3+1] += viscs2[1][2*ri+1] * aux;
      /*ALE: tau_M*timefac*timefac*2*nu*(-u_G * grad u, div epsilon(v)) */
      if(isale)
      {
         aux = timetauMp * time2nue * conv_g[ci];
         estif[ri*3][ci*3]     += viscs2[0][2*ri] * aux;
         estif[ri*3][ci*3+1]   += viscs2[1][2*ri] * aux;
         estif[ri*3+1][ci*3]   += viscs2[0][2*ri+1] * aux;
         estif[ri*3+1][ci*3+1] += viscs2[1][2*ri+1] * aux;
      }
      /* tau_M*timefac*timefac*2*nu*(u * grad u_old, div epsilon(v)) */
      aux = timetauMp * time2nue;
      estif[ri*3][ci*3]     += (viscs2[0][2*ri]   * conv_r[0][2*ci]
                               +viscs2[1][2*ri]   * conv_r[1][2*ci]) * aux;
      estif[ri*3+1][ci*3]   += (viscs2[0][2*ri+1] * conv_r[0][2*ci]
                               +viscs2[1][2*ri+1] * conv_r[1][2*ci]) * aux;
      estif[ri*3][ci*3+1]   += (viscs2[0][2*ri]   * conv_r[0][2*ci+1]
                               +viscs2[1][2*ri]   * conv_r[1][2*ci+1]) * aux;
      estif[ri*3+1][ci*3+1] += (viscs2[0][2*ri+1] * conv_r[0][2*ci+1]
                               +viscs2[1][2*ri+1] * conv_r[1][2*ci+1]) * aux;
      /* -tau_M*timefac*timefac*4*nu^2(div epsilon(u), div epsilon(v)) */
      aux = time2nue * time2nue * tau_Mp; 
      estif[ri*3][ci*3]     += (viscs2[0][2*ri]   * viscs2[0][2*ci]
                               +viscs2[1][2*ri]   * viscs2[1][2*ci]) * aux;
      estif[ri*3+1][ci*3]   += (viscs2[0][2*ri+1] * viscs2[0][2*ci]
                               +viscs2[1][2*ri+1] * viscs2[1][2*ci]) * aux;
      estif[ri*3][ci*3+1]   += (viscs2[0][2*ri]   * viscs2[0][2*ci+1]
                               +viscs2[1][2*ri]   * viscs2[1][2*ci+1]) * aux;
      estif[ri*3+1][ci*3+1] += (viscs2[0][2*ri+1] * viscs2[0][2*ci+1]
                               +viscs2[1][2*ri+1] * viscs2[1][2*ci+1]) * aux;
      /* tau_M*timefac*timefac*2*nu*(grad p, div epsilon(v)) */
      aux = time2nue * timetauMp;
      estif[ri*3][ci*3+2]   += (viscs2[0][2*ri] * derxy[0][ci]
                               +viscs2[1][2*ri] * derxy[1][ci]) * aux;
      estif[ri*3+1][ci*3+2] += (viscs2[0][2*ri+1] * derxy[0][ci]
                               +viscs2[1][2*ri+1] * derxy[1][ci]) * aux;

      /*--- PRESSURE part of stabilisation ---*/
      /* a concentration of the following terms: */
      /* -tau_M*timefac*(u, grad q) */
      /* -tau_M*timefac*timefac*(u_old * grad u, grad q) */
      estif[ri*3+2][ci*3]   += derxy[0][ri] * ( funct[ci]*timetauMp
                                               +conv_c[ci]*ttimetauMp );
      estif[ri*3+2][ci*3+1] += derxy[1][ri] * ( funct[ci]*timetauMp
                                               +conv_c[ci]*ttimetauMp );
      /*ALE -tau_M*timefac*timefac*(-u_G * grad u, grad q) */
      if(isale)
      {
         estif[ri*3+2][ci*3]   += derxy[0][ri] * conv_g[ci] * ttimetauMp;
         estif[ri*3+2][ci*3+1] += derxy[1][ri] * conv_g[ci] * ttimetauMp;
      }
      /* -tau_M*timefac*timefac*(u * grad u_old, grad q) */
      estif[ri*3+2][ci*3]   += (derxy[0][ri] * conv_r[0][2*ci]
                               +derxy[1][ri] * conv_r[1][2*ci]) * ttimetauMp;
      estif[ri*3+2][ci*3+1] += (derxy[0][ri] * conv_r[0][2*ci+1]
                               +derxy[1][ri] * conv_r[1][2*ci+1]) * ttimetauMp;
      /* tau_M*timefac*timefac*2*nu*(div epsilon(u), grad q) */
      aux = timetauMp * time2nue;
      estif[ri*3+2][ci*3]   += (derxy[0][ri] * viscs2[0][2*ci]
                               +derxy[1][ri] * viscs2[1][2*ci]) * aux;
      estif[ri*3+2][ci*3+1] += (derxy[0][ri] * viscs2[0][2*ci+1]
                               +derxy[1][ri] * viscs2[1][2*ci+1]) * aux;
      /* -tau_M*timefac*timefac*(grad p, grad q) */
      estif[ri*3+2][ci*3+2] += (derxy[0][ri] * derxy[0][ci]
                               +derxy[1][ri] * derxy[1][ci]) * timefac * timetauMp;

      /*--- R(u_old) * L_conv STABILISATION ---*/
      /* a concentration of the following terms: */
      /* -tau_M*timefac*(u_old, u * grad v) */
      /* -tau_M*timefac*timefac*(u_old * grad u_old, u * grad v) */
      /* tau_M*timefac*timefac*2*nu*(div epsilon(u_old), u * grad v) */
      /* -tau_M*timefac*timefac*(grad p_old, u * grad v) */
      /*--- linear part of RHS stabilisation (goes into matrix) ---*/
      /* tau_M*timefac*(rhsint, u * grad v) */
      aux = - timetauM * time2nue;
      estif[ri*3][ci*3]     += ( (velint[0]-rhsint[0]) * timetauM
                                +(conv_old[0] + gradp[0]) * ttimetauM
                                + visc_old[0] * aux ) * ugradv[ri][2*ci];
      estif[ri*3][ci*3+1]   += ( (velint[0]-rhsint[0]) * timetauM
                                +(conv_old[0] + gradp[0]) * ttimetauM
                                + visc_old[0] * aux ) * ugradv[ri][2*ci+1];
      estif[ri*3+1][ci*3]   += ( (velint[1]-rhsint[1]) * timetauM
                                +(conv_old[1] + gradp[1]) * ttimetauM
                                + visc_old[1] * aux ) * ugradv[ri][2*ci];
      estif[ri*3+1][ci*3+1] += ( (velint[1]-rhsint[1]) * timetauM
                                +(conv_old[1] + gradp[1]) * ttimetauM
                                + visc_old[1] * aux ) * ugradv[ri][2*ci+1];

      /*--- CONTINUITY equation stabilisation ---*/
      /* tau_C*timefac*timefac*(div u, div v) */
      aux = timefac * timefac * tau_C;
      estif[ri*3][ci*3]     += div[ri*2] * div[ci*2] * aux;
      estif[ri*3][ci*3+1]   += div[ri*2] * div[ci*2+1] * aux;
      estif[ri*3+1][ci*3]   += div[ri*2+1] * div[ci*2] * aux;
      estif[ri*3+1][ci*3+1] += div[ri*2+1] * div[ci*2+1] * aux;
   }  /* end column loop (ci) */

   /**************** integrate element force vector *********************/
   /*================== Galerkin part of the RHS =======================*/
   /*--- 'Original' RHS, concentrated ---*/
   /* (rhsint, v) */
   /*--- from Nonlinearity of Galerkin stiffness ---*/
   /* timefac*(u_old * grad u_old, v) */
   eforce[ri*3]   += funct[ri] * ( rhsint[0]*fac + conv_old[0]*timefacfac);
   eforce[ri*3+1] += funct[ri] * ( rhsint[1]*fac + conv_old[1]*timefacfac);

   /*================ Stabilisation part of the RHS ====================*/
   /*--- 'Original' RHS ---*/
   /* tau_M*timefac*2*nu*(rhsint, div epsilon(v)) */
   aux = time2nue * tau_Mp;
   eforce[ri*3]   += (rhsint[0] * viscs2[0][2*ri]
                     +rhsint[1] * viscs2[1][2*ri]) * aux;
   eforce[ri*3+1] += (rhsint[0] * viscs2[0][2*ri+1]
                     +rhsint[1] * viscs2[1][2*ri+1]) * aux;
   /* -tau_M*timefac*(rhsint, grad q) */
   eforce[ri*3+2] += (rhsint[0] * derxy[0][ri]
                     +rhsint[1] * derxy[1][ri]) * timetauMp;
   /* -tau_M*timefac*(rhsint, -u_G * grad v) */
   if(isale)
   {
      eforce[ri*3]   += rhsint[0] * conv_g[ri] * timetauM;
      eforce[ri*3+1] += rhsint[1] * conv_g[ri] * timetauM;
   }
   /*--- Terms resulting from stiffness linearisation ---*/
   /* a concentration of the following: */
   /* -tau_M*timefac*(u_old, u_old * grad v) */
   /* tau_M*timefac*timefac*2*nu*(div epsilon(u_old), u_old * grad v) */
   /* -tau_M*timefac*timefac*(grad p_old, u_old * grad v) */
   aux = - timetauM * time2nue;
   eforce[ri*3]   += conv_c[ri] * ( velint[0]*timetauM
                                   +visc_old[0]*aux
                                   +gradp[0]*ttimetauM );
   eforce[ri*3+1] += conv_c[ri] * ( velint[1]*timetauM
                                   +visc_old[1]*aux
                                   +gradp[1]*ttimetauM );
   /* -tau_M*2*timefac*timefac*(u_old * grad u_old, u_old * grad v) */
   aux = ttimetauM * 2.0;
   eforce[ri*3]   += conv_old[0] * conv_c[ri] * aux;
   eforce[ri*3+1] += conv_old[1] * conv_c[ri] * aux;
   /* ALE: -tau_M*timefac*timefac*(u_old * grad u_old, u_old * grad v) */
   if(isale)
   {
      eforce[ri*3]   += conv_old[0] * conv_g[ri] * ttimetauM;
      eforce[ri*3+1] += conv_old[1] * conv_g[ri] * ttimetauM;
   }
   /* tau_M*timefac*timefac*2*nu*(u_old * grad u_old, div epsilon(v)) */
   aux = timetauMp * time2nue;
   eforce[ri*3]   += (conv_old[0] * viscs2[0][2*ri]
                     +conv_old[1] * viscs2[1][2*ri]) * aux;
   eforce[ri*3+1] += (conv_old[0] * viscs2[0][2*ri+1]
                     +conv_old[1] * viscs2[1][2*ri+1]) * aux;
   /* -tau_M*timefac*timefac*(u_old * grad u_old, grad q) */
   eforce[ri*3+2] += (conv_old[0] * derxy[0][ri]
                     +conv_old[1] * derxy[1][ri]) * ttimetauMp;
}     /* end row loop (ri) */


/*----------------------------------------------------------------------*/
#ifdef DEBUG
dstrc_exit();
#endif
}


/*!---------------------------------------------------------------------
\brief Gauss point contributions for integration of boundary forces

<pre>                                                         chfoe 03/05

This routine evaluates the Gauss point vaulues of the residual vector
of one element taking stabilisation effects into account. Only the
residual of the momentum equation R_M is calculated.

R_M = u + timefac u * grad u - timefac * 2 nu div epsilon(u)
    + timefac grad p - rhsint

The residual contains stabilisation of the type

Sum_over_k (R_M, tau L_M)_k with

L_M = v + timefac u_old * grad v + timefac v * grad u_old
    - timefac * 2 nu alpha div epsilon (v) + timefac beta grad q

where alpha = -1
      beta  = -1

timefac depends on the time integration scheme:

One-step theta:

timefac = theta * dt

BDF2:

timefac = 2/3 * dt

NOTE: this works perfectly only when the fluid is solved via usfem

</pre>
\param  *eforce	   DOUBLE    (o)    element force vector (residual)
\param  *velint    DOUBLE    (i)    (converged) vel. at int.-point
\param   histvec   DOUBLE    (i)    histroy data
\param **vderxy    DOUBLE    (i)    velocity gradient at int.-point
\param **vderxy2   DOUBLE    (i)    second vel. derivatives at int.-point
\param  *funct     DOUBLE    (i)    natural shape functions
\param **derxy     DOUBLE    (i)    shape function derivatives
\param **derxy2    DOUBLE    (i)    second shape funct. derivs
\param  *edeadng   DOUBLE    (i)    body forces
\param  *press     DOUBLE    (i)    pressure at Gauss point
\param   gradp[2]  DOUBLE    (i)    pressure gradient at GP
\param   fac       DOUBLE    (i)    integration factor
\param   visc      DOUBLE    (i)    fluid viscosity
\param   iel       INT       (i)    number of elemental nodes
\param  *hasext    INT       (i)    flag, if there is body force
\param   is_ale    INT       (i)    flag, if it's ale or Euler
\return void

------------------------------------------------------------------------*/
void f2_calresvec(  DOUBLE  *eforce,
                    DOUBLE  *velint,
                    DOUBLE   histvec[2],
                    DOUBLE **vderxy,
                    DOUBLE **vderxy2,
                    DOUBLE  *funct,
                    DOUBLE **derxy,
		    DOUBLE **derxy2,
                    DOUBLE  *edeadng,
                    DOUBLE   aleconv[2],
                    DOUBLE  *press,
                    DOUBLE   gradp[2],
                    DOUBLE   fac,
                    DOUBLE   visc,
                    INT      iel,
                    INT     *hasext,
                    INT      is_ale
              )
{
INT     i, ri;
DOUBLE  timefac;    /* One-step-Theta: timefac = theta*dt
                       BDF2:           timefac = 2/3 * dt               */
DOUBLE  invtime;    /* 1 / timefac                                      */
DOUBLE  dt;         /* time step size*/
DOUBLE  tau_M, tau_C;             /* stabilisation parameter            */
DOUBLE  tau_Mp;             /* stabilisation parameter            */
DOUBLE  viscous[2][2][2*MAXNOD];   /* viscous term partially integrated */
DOUBLE  eps_u[2][2]; /* the strain rate                                 */
DOUBLE  rhsint[2];   /* total right hand side terms at int.-point       */
DOUBLE  conv_c[MAXNOD];    /* linearisation of convect, convective part */
DOUBLE  viscs2[2][2*MAXNOD]; /* viscous term incluiding 2nd derivatives */
DOUBLE  visc2[2];            /* viscous term evaluated                  */
DOUBLE  resid[2];
DOUBLE  twovisc;


/*----------------------------------------------------------------------*/
#ifdef DEBUG
dstrc_enter("f2_calresvec");
#endif
/*========================== initialisation ============================*/
fdyn = alldyn[genprob.numff].fdyn;


tau_M  = fdyn->tau[0]*fac;
tau_Mp = fdyn->tau[0]*fac;
tau_C  = fdyn->tau[2]*fac;

timefac = fdyn->thsl;
dt      = fdyn->dta;
invtime = 1.0 / timefac;

twovisc = 2.0 * visc;

/*--------------------- evaluate rhs vector at integration point ... ---*/
/*---------------- ... including actual velocity and convective term ---*/
if(is_ale)
{
   if (*hasext)
   {
      rhsint[0] = timefac * ( edeadng[0]
                  - vderxy[0][0] * aleconv[0] - vderxy[0][1] * aleconv[1] )
                  + histvec[0] - velint[0];
      rhsint[1] = timefac * ( edeadng[1]
                  - vderxy[1][0] * aleconv[0] - vderxy[1][1] * aleconv[1] )
                  + histvec[1] - velint[1];
   }
   else
   {
      rhsint[0] = histvec[0] - velint[0] - timefac *
                 ( vderxy[0][0] * aleconv[0] + vderxy[0][1] * aleconv[1] );
      rhsint[1] = histvec[1] - velint[1] - timefac *
                 ( vderxy[1][0] * aleconv[0] + vderxy[1][1] * aleconv[1] );
   }
}
else /* pure Euler element */
{
   if (*hasext)
   {
      rhsint[0] = timefac * ( edeadng[0]
                  - vderxy[0][0] * velint[0] - vderxy[0][1] * velint[1] )
                  + histvec[0] - velint[0];
      rhsint[1] = timefac * ( edeadng[1]
                  - vderxy[1][0] * velint[0] - vderxy[1][1] * velint[1] )
                  + histvec[1] - velint[1];
   }
   else
   {
      rhsint[0] = histvec[0] - velint[0] - timefac *
                 ( vderxy[0][0] * velint[0] + vderxy[0][1] * velint[1] );
      rhsint[1] = histvec[1] - velint[1] - timefac *
                 ( vderxy[1][0] * velint[0] + vderxy[1][1] * velint[1] );
   }
}

/*--- viscous term (after integr. by parts) -------------------------*/
/*   /                            \
   1 |  2 u_x,x    u_x,y + u_y,x  |
   - |                            |
   2 |  u_y,x + u_x,y    2 u_y,y  |
     \                           /                                   */
eps_u[0][0] = vderxy[0][0];
eps_u[0][1] = 0.5 * ( vderxy[0][1] + vderxy[1][0] );
eps_u[1][0] = eps_u[0][1];
eps_u[1][1] = vderxy[1][1];

/*--- viscous term (without integr. by parts) --------------------------*/
/* Viscous term  div epsilon(u_old) */
visc2[0] = 0.5 * (2.0*vderxy2[0][0] + vderxy2[0][1] + vderxy2[1][2]);
visc2[1] = 0.5 * (2.0*vderxy2[1][1] + vderxy2[1][0] + vderxy2[0][2]);


resid[0] = rhsint[0] + timefac * (twovisc * visc2[0] - gradp[0]);
resid[1] = rhsint[1] + timefac * (twovisc * visc2[1] - gradp[1]);

/*------ get partially integrated terms ---*/
for (i=0; i<iel; i++) /* loop over nodes of element */
{
   /*--- viscous term (after integr. by parts) -------------------------*/
   /*   /                            \
      1 |  2 N_x,x    N_x,y + N_y,x  |    with N_x .. x-line of N
      - |                            |         N_y .. y-line of N
      2 |  N_y,x + N_x,y    2 N_y,y  |
        \                           /                                   */
   viscous[0][0][2*i]   = derxy[0][i];
   viscous[0][0][2*i+1] = 0.0;                /* 1st index:             */
   viscous[0][1][2*i]   = 0.5 * derxy[1][i];  /*   line of epsilon      */
   viscous[0][1][2*i+1] = 0.5 * derxy[0][i];  /* 2nd index:             */
   viscous[1][0][2*i]   = 0.5 * derxy[1][i];  /*   column of epsilon    */
   viscous[1][0][2*i+1] = 0.5 * derxy[0][i];  /* 3rd index:             */
   viscous[1][1][2*i]   = 0.0;
   viscous[1][1][2*i+1] = derxy[1][i];        /*   elemental vel dof    */

   /*================= build stabilisation operatior ===================*/
   /*--- convective part u_old * grad (funct) --------------------------*/
   /* u_old_x * N,x  +  u_old_y * N,y   with  N .. form function matrix */
   if (is_ale)
      conv_c[i] = derxy[0][i] * aleconv[0] + derxy[1][i] * aleconv[1] ;
   else
      conv_c[i] = derxy[0][i] * velint[0] + derxy[1][i] * velint[1] ;

   /*--- viscous term  - grad * epsilon(u): ----------------------------*/
   /*   /                              \
      1 |  2 N_x,xx + N_x,yy + N_y,xy  |    with N_x .. x-line of N
    - - |                              |         N_y .. y-line of N
      2 |  N_y,xx + N_x,yx + 2 N_y,yy  |
        \                             /                                 */
   viscs2[0][2*i]   = - 0.5 * ( 2.0 * derxy2[0][i] + derxy2[1][i] );
   viscs2[0][2*i+1] = - 0.5 * ( derxy2[2][i] );
   viscs2[1][2*i]   = - 0.5 * ( derxy2[2][i] );
   viscs2[1][2*i+1] = - 0.5 * ( derxy2[0][i] + 2.0 * derxy2[1][i] );

}

/*---------------------------------- now build single residual terms ---*/
for (ri=0; ri<iel; ri++)      /* row index */
{
   /*************** integrate element residuum vector *******************/
   /* simple parts, which are not partially integrated */
   eforce[ri*3]   += funct[ri] * rhsint[0] * invtime * fac;
   eforce[ri*3+1] += funct[ri] * rhsint[1] * invtime * fac;

   /* viscous forces integrated by parts */
   eforce[ri*3]   -= ( viscous[0][0][ri*2] * eps_u[0][0] 
                      +viscous[0][1][ri*2] * eps_u[0][1]  
                      +viscous[1][0][ri*2] * eps_u[1][0]  
                      +viscous[1][1][ri*2] * eps_u[1][1] ) * twovisc * fac;
   eforce[ri*3+1] -= ( viscous[0][0][ri*2+1] * eps_u[0][0] 
                      +viscous[0][1][ri*2+1] * eps_u[0][1]  
                      +viscous[1][0][ri*2+1] * eps_u[1][0]  
                      +viscous[1][1][ri*2+1] * eps_u[1][1] ) * twovisc * fac;

   /* pressure forces integrated by parts*/
   eforce[ri*3]   += *press * derxy[0][ri] * fac;
   eforce[ri*3+1] += *press * derxy[1][ri] * fac;
   
   /* stabilisation part - impulse stabilisation */
   eforce[ri*3]   += tau_M * conv_c[ri] * resid[0];
   eforce[ri*3+1] += tau_M * conv_c[ri] * resid[1];

   eforce[ri*3]   += tau_M * twovisc * ( viscs2[0][ri*2] * resid[0]
                                        +viscs2[1][ri*2] * resid[1] );
   eforce[ri*3+1] += tau_M * twovisc * ( viscs2[0][ri*2+1] * resid[0]
                                                  +viscs2[1][ri*2+1] * resid[1] );
   /* stabilisation part - continuity stabilistation */
   eforce[ri*3]   -= tau_C * timefac * (vderxy[0][0]+vderxy[1][1]) * derxy[0][ri];
   eforce[ri*3+1] -= tau_C * timefac * (vderxy[0][0]+vderxy[1][1]) * derxy[1][ri];


} /* end loop over rows */

/*----------------------------------------------------------------------*/
#ifdef DEBUG
dstrc_exit();
#endif
}

#endif
/*! @} (documentation module close)*/
