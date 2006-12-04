/*!----------------------------------------------------------------------
\file
\brief

<pre>
Maintainer: Michael Gee
            gee@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/gee/
            0711 - 685-6572
</pre>

*----------------------------------------------------------------------*/
#include "../headers/standardtypes.h"
#include "../solver/solver.h"
#include "../fluid_full/fluid_prototypes.h"
#include "../fsi_full/fsi_prototypes.h"
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | global variable GENPROB genprob is defined in global_control.c       |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | pointer to allocate dynamic variables if needed                      |
 | dedfined in global_control.c                                         |
 | ALLDYNA               *alldyn;                                       |
 *----------------------------------------------------------------------*/
extern ALLDYNA      *alldyn;
/*----------------------------------------------------------------------*
 |  routine to control dynamic execution                 m.gee 5/01     |
 *----------------------------------------------------------------------*/
void caldyn()
{
#ifdef DEBUG
dstrc_enter("caldyn");
#endif
/*------------------------------ switch into different time integrators */
/*   switch (alldyn[0].sdyn->Typ) */
/*   { */
/*     /\* central differences time integration *\/ */
/*     case centr_diff: */
/*       dyn_nln_stru_expl(); */
/*     /\* generalized alfa time integration *\/ */
/*     case gen_alfa: */
/*       dyn_nln_structural(); */
/*     /\* Generalized Energy-Momentum time integration *\/ */
/*     case Gen_EMM: */
/* #ifdef GEMM */
/*       dyn_nln_gemm(); */
/* #else */
/*       dserror("GEMM not supported"); */
/* #endif */
/*   } */


if (alldyn[0].sdyn->Typ == centr_diff) {
  /* central difference time integration */
  dyn_nln_stru_expl();
}
else if (alldyn[0].sdyn->Typ == gen_alfa) {
  /* generalized alfa time integration */
#ifndef CCADISCRET
  dyn_nln_structural();
#else
  dyn_nlnstructural_drt();
#endif
}
else if (alldyn[0].sdyn->Typ == Gen_EMM) {
#ifdef GEMM
  dyn_nln_gemm(); /* Generalized Energy-Momentum time integration */
#else
  dserror("GEMM not supported");
#endif
}

/*----------------------------------------------------------------------*/
#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of caldyn */
