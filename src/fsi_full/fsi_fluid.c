/*!----------------------------------------------------------------------
\file
\brief implicit and semi-implicit time integration algorithm for fsi

<pre>
Maintainer: Steffen Genkinger
            genkinger@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/genkinger/
            0711 - 685-6127
</pre>

------------------------------------------------------------------------*/
/*!
\addtogroup FSI
*//*! @{ (documentation module open)*/
#ifdef D_FSI
#include "../headers/standardtypes.h"
#include "../solver/solver.h"
#include "../fluid_full/fluid_prototypes.h"
#include "fsi_prototypes.h"
#include "../io/io.h"
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | global variable GENPROB genprob is defined in global_control.c       |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;
/*----------------------------------------------------------------------*
 | global variable *solv, vector of lenght numfld of structures SOLVAR  |
 | defined in solver_control.c                                          |
 |                                                                      |
 |                                                       m.gee 11/00    |
 *----------------------------------------------------------------------*/
extern struct _SOLVAR  *solv;
/*!----------------------------------------------------------------------
\brief one proc's info about his partition

<pre>                                                         m.gee 8/00
-the partition of one proc (all discretizations)
-the type is in partition.h
</pre>

*----------------------------------------------------------------------*/
extern struct _PARTITION  *partition;
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | structure of flags to control output                                 |
 | defined in out_global.c                                              |
 *----------------------------------------------------------------------*/
extern struct _IO_FLAGS     ioflags;
/*!----------------------------------------------------------------------
\brief ranks and communicators

<pre>                                                         m.gee 8/00
This structure struct _PAR par; is defined in main_ccarat.c
and the type is in partition.h
</pre>

*----------------------------------------------------------------------*/
extern struct _PAR   par;
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | pointer to allocate dynamic variables if needed                      |
 | dedfined in global_control.c                                         |
 | ALLDYNA               *alldyn;                                       |
 *----------------------------------------------------------------------*/
extern ALLDYNA      *alldyn;
/*----------------------------------------------------------------------*
 |                                                       m.gee 02/02    |
 | number of load curves numcurve                                       |
 | vector of structures of curves                                       |
 | defined in input_curves.c                                            |
 | INT                   numcurve;                                      |
 | struct _CURVE      *curve;                                           |
 *----------------------------------------------------------------------*/
extern INT            numcurve;
extern struct _CURVE *curve;
/*----------------------------------------------------------------------*
 | enum _CALC_ACTION                                      m.gee 1/02    |
 | command passed from control routine to the element level             |
 | to tell element routines what to do                                  |
 | defined globally in global_calelm.c                                  |
 *----------------------------------------------------------------------*/
extern enum _CALC_ACTION calc_action[MAXFIELD];

/*!----------------------------------------------------------------------
\brief positions of physical values in node arrays

<pre>                                                        chfoe 11/04

This structure contains the positions of the various fluid solutions 
within the nodal array of sol_increment.a.da[ipos][dim].

extern variable defined in fluid_service.c
</pre>

------------------------------------------------------------------------*/
extern struct _FLUID_POSITION ipos;

/*!---------------------------------------------------------------------
\brief implicit and semi-implicit algorithms for
       multifield fluid problems

<pre>                                                         genk 09/02

this functions solves the fluid within a multifield problem in an
ALE-framework. The mesh velocity is determined based on the displacements
of the mesh (fsi_ale()).

</pre>

\param *fsidyn   FSI_DYNAMIC                                 (i)
\param *actfield FIELD            actual field               (i)
\param  mctrl    INT              evaluation flag            (i)
\param  numff    INT              number of fluid field      (i)
\return void
\warning up to now only the One-Step-Theta scheme combined with a
fixed-point-like iteration scheme is tested!

------------------------------------------------------------------------*/
void fsi_fluid(
		       FIELD          *actfield,
		       INT             mctrl
	      )
{
static INT             stresspro;          /* flag for stress projection       */
static INT             numff;              /* actual number of fluid field     */
static INT             itnum;              /* counter for nonlinear iteration  */
INT                    i;		   /* counters			       */
static INT             numeq;              /* number of equations on this proc */
static INT             numeq_total;        /* total number of equations        */
INT                    init;               /* flag for solver_control call     */
INT                    leftspace;        /* flag used to circulate other flags */
static INT             actsysarray=0;      /* number of actual sysarray        */
static INT             outstep;            /* counter for output control       */
static INT             pssstep;            /* counter for output control       */
static INT             restartstep;        /* counter for restart control      */
static INT             nfrastep;           /* number of steps for fractional-
                                              step-theta procedure             */
static INT             restart;
INT                    converged=0;        /* convergence flag                 */

static INT             actpos;             /* actual position in sol. history  */
DOUBLE                 vrat,prat;
DOUBLE                 grat;               /* convergence ratios               */
DOUBLE                 t1,ts,te;           /*				       */
static DOUBLE          tes=ZERO;           /*				       */
static DOUBLE          tss=ZERO;           /*				       */
static SOLVAR         *actsolv;            /* pointer to active sol. structure */
static PARTITION      *actpart;            /* pointer to active partition      */
static INTRA          *actintra;           /* pointer to active intra-communic.*/
static CALC_ACTION    *action;             /* pointer to the cal_action enum   */

static ARRAY           frhs_a;
static DOUBLE         *frhs;               /* iteration - RHS  		       */
static ARRAY           time_a;             /* stored time                      */
static ARRAY           totarea_a;
static DOUBLE         *totarea;
static CONTAINER       container;          /* variables for calelm             */
static FLUID_STRESS    str;
static FLUID_DYNAMIC  *fdyn;               /* fluid dynamic variables   */
static FSI_DYNAMIC    *fsidyn;             /* fsi dynamic variables     */

#ifdef BINIO
static BIN_OUT_FIELD out_context;
#endif

#ifdef DEBUG
dstrc_enter("fsi_fluid");
#endif

switch (mctrl)
{
/*======================================================================*
 |                      I N I T I A L I S A T I O N                     |
 *======================================================================*/
case 1:
numff  = genprob.numff;
fdyn   = alldyn[numff].fdyn;
fsidyn = alldyn[genprob.numaf+1].fsidyn;

fdyn->dt=fsidyn->dt;
fdyn->maxtime=fsidyn->maxtime;
fdyn->nstep=fsidyn->nstep;
grat=ZERO;

stresspro = fdyn->stresspro;

/*------------------------------------------ initialiase some counters */
outstep=0;
pssstep=0;
restartstep=0;
/*--------------------------------------------------- set some pointers */
/*---------------------------- only valid for single field problem !!!! */
actsolv     = &(solv[numff]);
actpart     = &(partition[numff]);
action      = &(calc_action[numff]);
restart     = genprob.restart;
container.fieldtyp = actfield->fieldtyp;
container.actndis  = 0;
container.turbu    = fdyn->turbu;
if (genprob.probtyp==prb_fsi) str = str_fsicoupling;
fdyn->acttime=ZERO;

if (fdyn->freesurf==5)
   fdyn->hf_stab=0;

/*---------------- if we are not parallel, we have to allocate an alibi *
  ---------------------------------------- intra-communicator structure */
#ifdef PARALLEL
actintra    = &(par.intra[numff]);
#else
actintra    = (INTRA*)CCACALLOC(1,sizeof(INTRA));
if (!actintra) dserror("Allocation of INTRA failed");
actintra->intra_fieldtyp = fluid;
actintra->intra_rank     = 0;
actintra->intra_nprocs   = 1;
#endif
/*- there are only procs allowed in here, that belong to the fluid -----*/
/* intracommunicator (in case of nonlinear fluid. dyn., this should be all)*/
if (actintra->intra_fieldtyp != fluid) break;

/*------------------------------- init the dist sparse matrices to zero */
solserv_zero_mat(
                 actintra,
                 &(actsolv->sysarray[actsysarray]),
                 &(actsolv->sysarray_typ[actsysarray])
                );

/*---------------------------- get global and local number of equations */
solserv_getmatdims(&(actsolv->sysarray[actsysarray]),
                   actsolv->sysarray_typ[actsysarray],
                   &numeq,
                   &numeq_total);

/*--------------------------------------- allocate 1 dist. vector 'rhs' */
actsolv->nrhs = 1;
solserv_create_vec(&(actsolv->rhs),actsolv->nrhs,numeq_total,numeq,"DV");
solserv_zero_vec(&(actsolv->rhs[0]));

/*---------------------------------- allocate dist. solution vectors ---*/
if (stresspro) actsolv->nsol= 2;/* one more solvec needed for stresspro */
else actsolv->nsol= 1;
solserv_create_vec(&(actsolv->sol),actsolv->nsol,numeq_total,numeq,"DV");
for (i=0; i<actsolv->nsol; i++)
   solserv_zero_vec(&(actsolv->sol[i]));

/*---------------  allocate one redundant vector frhs of full lenght ---*/
/*   this is used by the element routines to assemble the  Iteration RHS*/
frhs = amdef("frhs",&frhs_a,numeq_total,1,"DV");

/*--------------------------- allocate one vector for storing the time */
amdef("time",&time_a,1000,1,"DV");

/*--------------------------- allocate one vector for storing the area */
if (fdyn->checkarea>0)
{
   totarea = amdef("area",&totarea_a,fdyn->itemax,1,"DV");
   amzero(&totarea_a);
}

/*------------------------- init lift&drag calculation real FSI-problem */
if (fdyn->liftdrag==ld_stress)
{
  str         = str_liftdrag;
  if(genprob.numfld==3) fluid_liftdrag(0,NULL,NULL,NULL,NULL,NULL,NULL);
}
if (fdyn->liftdrag==ld_nodeforce)
  fluid_liftdrag(-1,action,&container,actfield,
                 actsolv,actpart,actintra);

/*--------------------------------------------- initialise fluid field */
if (restart > 0)
{
   if (fdyn->init>0)
      dserror("Initial field either by restart, or by function or from file ...\n");
   else
   {
      fdyn->resstep=genprob.restart;
      fdyn->init=2;
   }
}
fluid_init_pos_ale();
if(fdyn->iop == 4) ipos.numsol = 9;
else               ipos.numsol = 7;
if(stresspro) ipos.numsol++; /* stress projection and ...*/
if(fsidyn->ifsi == 6)        /* ... steepest descent relaxation ... */
   ipos.numsol++;            /* ... each need one more solution field. */
fluid_init(actpart,actintra,actfield,0,action,&container,ipos.numsol,str);
actpos=0;         /* ... each need one more solution field. */

/*-------------------------------------- init the dirichlet-conditions */
fluid_initdirich(actfield);

/*---------------------------------- initialize solver on all matrices */
/*
NOTE: solver init phase has to be called with each matrix one wants to
      solve with. Solver init phase has to be called with all matrices
      one wants to do matrix-vector products and matrix scalar products.
      This is not needed by all solver libraries, but the solver-init
      phase is cheap in computation (can be costly in memory)
*/
/*--------------------------------------------------- initialize solver */
init=1;
solver_control(actsolv, actintra,
               &(actsolv->sysarray_typ[actsysarray]),
               &(actsolv->sysarray[actsysarray]),
               &(actsolv->sol[0]),
               &(actsolv->rhs[0]),
               init);

/*------------------------------------- init the assembly for stiffness */
init_assembly(actpart,actsolv,actintra,actfield,actsysarray,0);

/*---------------------------------- allocate fluid integration data ---*/
alldyn[numff].fdyn->data = (FLUID_DATA*)CCACALLOC(1,sizeof(FLUID_DATA));

/*------------------------------- init the element calculating routines */
*action = calc_fluid_init;
calinit(actfield,actpart,action,&container);

/*-------------------------------------------- initialise energey check */
if (fsidyn->ichecke>0) fsi_dyneint(NULL,2);

/*------------------------------------------------ output to the screen */
if (par.myrank==0) printf("\n\n");
#ifdef PARALLEL
MPI_Barrier(actintra->MPI_INTRA_COMM);
#endif
for (i=0;i<par.nprocs;i++)
if (par.myrank==i)
printf("PROC  %3d | FIELD FLUID     | number of equations      : %10d \n",
        par.myrank,numeq);
#ifdef PARALLEL
MPI_Barrier(actintra->MPI_INTRA_COMM);
#endif
if (par.myrank==0)
printf("          | FIELD FLUID     | total number of equations: %10d \n",numeq_total);
if (par.myrank==0) printf("\n\n");

/*--------------------------------- initialise height function solution */
if (fdyn->freesurf==3)
fluid_heightfunc(1,&grat,actfield,actpart,actintra,action,
                 &container,converged);

/*-------------------------------- calculate curvature at the beginning */
if (fdyn->surftens!=0)
{
   fluid_tcons();
   *action = calc_fluid_curvature;
   fluid_curvature(actfield,actpart,actintra,action);
}

/*--------------------------------------------- calculate nodal normals */
fluid_cal_normal(actfield,1,action);

/*------------------------------------------------- define local co-sys */
fluid_locsys(actfield,fdyn);

/*------------------------- predictor for free surface at the beginning */
if (fdyn->freesurf>0)
fluid_updfscoor(actfield, fdyn, fdyn->dt, -1);

/*---------------------------------------------------------- monitoring */
if (ioflags.monitor==1)
{
   out_monitor(actfield,numff,ZERO,1);
   monitoring(actfield,numff,actpos,fdyn->acttime);
}
/*------------------------------------------------ init area monitoring */
if (fdyn->checkarea>0) out_area(totarea_a,fdyn->acttime,0,1);

/*-------------------------------------- print out initial data to .out */
out_sol(actfield,actpart,actintra,fdyn->step,actpos);

/*---------- calculate time independent constants for time algorithm ---*/
fluid_cons();

#ifdef D_MORTAR
/* redefine the size of sol_mf from 2 to 3, the third field is necessary*/
/* to store the nodal forces due to fsi */
  solserv_sol_zero(actfield, 0, node_array_sol_mf, 3);
#endif

#ifdef BINIO
  /* initialize binary output
   * It's important to do this only after all the node arrays are set
   * up because their sizes are used to allocate internal memory. */
  init_bin_out_field(&out_context,
                     &(actsolv->sysarray_typ[actsysarray]),
                     &(actsolv->sysarray[actsysarray]),
                     actfield, actpart, actintra, 0);
#endif

break;
/*======================================================================*
 |                     S O L U T I O N    P H A S E                     |
 *======================================================================*/
/* nodal solution history fluid field:                                  *
 * sol[0][j]           ... initial data 				*
 * sol[1...actpos][j]  ... solution for visualisation (real pressure)	*
 * sol_increment[flag][j] ... solution value needed further             *
 * sol_mf[0][j]        ... solution at time (n+1)			*
 * sol_mf[1][j]        ... nodal stresses at FS-interface at time (n+1) *
 * in mortar cases only:                                                *
 * sol_mf[2][j]        ... nodal forces at FS-interface at time (n+1)   *
 * sol_increment flags:                                                 *
 *  velnm  ...  nodal solution at time (n-1)                            *
 *  veln   ...  nodal solution at time (n)                              *
 *  velnp  ...  nodal solution at time (n+1)                            *
 *  accnm  ...  nodal acceleration at time (n-1)                        *
 *  accn   ...  nodal acceleration at time (n)                          *
 *  hist   ...  linear combination of history values needed for rhs     *
 *  gridv  ...  nodal grid velocity within actual time step             *
 *  convn  ...  nodal convective velocity at time (n)                   *
 *  convnp ...  nodal convective velocity at time (n+1)                 *
 *======================================================================*/
case 2:

grat=ZERO;
/*- there are only procs allowed in here, that belong to the fluid -----*/
/* intracommunicator (in case of nonlinear fluid. dyn., this should be all)*/
if (actintra->intra_fieldtyp != fluid) break;

/*------------------------------------------ check (starting) algorithm */
if (fdyn->step<=(fdyn->nums+1))
   fluid_startproc(&nfrastep,0);

/*------------------------------ calculate constants for time algorithm */
fluid_tcons();

/*------------------------------------------------ output to the screen */
if (par.myrank==0) 
{
  if (fdyn->iop == 4) printf("Solving FLUID by One-Step-Theta ...\n");
  else if (fdyn->iop == 7) printf("Solving FLUID by BDF2 ...\n");
  else dserror("wrong time integration scheme");
}
/*--------------------------------------------------------- ALE-PHASE I */
if (fsidyn->iale==1)
{
   /*--------------------------------------------- get the gridvelocity */
   fsi_alecp(actfield,fdyn->dta,fdyn->numdf,1);
   /*---------------------------------------------- change element flag */
   fdyn->ishape=1;
   /*------------------ calculate ALE-convective velocities at time (n) */
   fsi_aleconv(actfield,fdyn->numdf,ipos.convn,ipos.veln);
}
else  dserror("ALE field by function not implemented yet!\n");

/*--------------------- set dirichlet boundary conditions for  timestep */
fluid_setdirich(actfield,ipos.velnp);

/*------------------------------------ prepare time rhs in mass form ---*/
fluid_prep_rhs(actfield);

/*--------------------------- start time step for fluid on the screen---*/
if (fdyn->itnorm!=fncc_no && par.myrank==0)
{
   if (fdyn->freesurf>1)
   {
   printf("------------------------------------------------------------------------------- \n");
   printf("|- step/max -|-  tol     [norm] -|- vel. error -|- pre. error -|-  fs error  -|\n");
   }
   else
   {
   printf("---------------------------------------------------------------- \n");
   printf("|- step/max -|-  tol     [norm] -|- vel. error -|- pre. error -|\n");
   }
}
itnum=1;
/*======================================================================*
 |               N O N L I N E A R   I T E R A T I O N                  |
 *======================================================================*/
nonlniter:
fdyn->itnum=itnum;
/*------------------------- calculate constants for nonlinear iteration */
if(fdyn->freesurf) fluid_icons(itnum);

/*-------------------------------------------------------- ALE-PHASE II */
if (fsidyn->iale==1)
{
   /*------------------ for implicit free surface we have to update the
     ------------------------------- grid velocity during the iteration */
   if (fdyn->freesurf>1 && itnum>1)
   {
      fsi_alecp(actfield,fdyn->dta,fdyn->numdf,fdyn->freesurf);
     /*-------------------------------------------- change element flag */
      fdyn->ishape=1;
   }
   /*--------------- calculate ale-convective velocities at  time (n+1) */
   fsi_aleconv(actfield,fdyn->numdf,ipos.convnp,ipos.velnp);
}
else  dserror("ALE field by function not implemented yet!\n");

/*--------------------------------- calculate curvature at free surface */
if (fdyn->surftens!=0)
{
   *action = calc_fluid_curvature;
   fluid_curvature(actfield,actpart,actintra,action);
}

/*---------------------------- intitialise global matrix and global rhs */
solserv_zero_vec(&(actsolv->rhs[0]));
solserv_zero_mat(actintra,&(actsolv->sysarray[actsysarray]),
                 &(actsolv->sysarray_typ[actsysarray]));

/*--------------------------------------------------- initialise rhs ---*/
amzero(&frhs_a);

/*-------------- form incremental matrices, residual and element forces */
*action = calc_fluid;
t1=ds_cputime();
container.dvec         = NULL;
container.frhs         = frhs;
container.global_numeq = numeq_total;
container.nii          = fdyn->nii;
container.kstep        = 0;
container.fieldtyp     = actfield->fieldtyp;
container.is_relax     = 0;
calelm(actfield,actsolv,actpart,actintra,actsysarray,-1,
       &container,action);
te=ds_cputime()-t1;
tes+=te;

/*--------------------------------------------------------- add rhs: ---*/
assemble_vec(actintra,
             &(actsolv->sysarray_typ[actsysarray]),
             &(actsolv->sysarray[actsysarray]),
             &(actsolv->rhs[0]),
             frhs,
             1.0
             );

/*-------------------------------------------------------- solve system */
init=0;
t1=ds_cputime();
solver_control(actsolv, actintra,
               &(actsolv->sysarray_typ[actsysarray]),
               &(actsolv->sysarray[actsysarray]),
               &(actsolv->sol[0]),
               &(actsolv->rhs[0]),
               init);
ts=ds_cputime()-t1;
tss+=ts;

/*-- set flags for stability parameter evaluation and convergence check */
fdyn->ishape=0;

/*--- return solution to the nodes and calculate the convergence ratios */
fluid_result_incre(actfield, 0,actintra,&(actsolv->sol[0]),ipos.velnp,
                     &(actsolv->sysarray[actsysarray]),
                     &(actsolv->sysarray_typ[actsysarray]),
		     &vrat,&prat,&grat);

/*--------------------------------------------- do stress projection ---*/
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
if (stresspro)
{
   /*---------------------- intitialise global matrix and global rhs ---*/
   solserv_zero_vec(&(actsolv->rhs[0]));
   solserv_zero_mat(actintra,&(actsolv->sysarray[actsysarray]),
                 &(actsolv->sysarray_typ[actsysarray]));
   /*----------------------------------------------------- empty rhs ---*/
   amzero(&frhs_a);

   /*-------- form incremental matrices, residual and element forces ---*/
   *action = calc_fluid_stressprojection;
   t1=ds_cputime();
   container.dvec         = NULL;
   container.frhs         = frhs;
   container.global_numeq = numeq_total;
   container.nii          = 0;
   container.kstep        = 0;
   container.fieldtyp     = actfield->fieldtyp;
   container.is_relax     = 0;
   calelm(actfield,actsolv,actpart,actintra,actsysarray,-1,
          &container,action);
   te=ds_cputime()-t1;
   tes+=te;

   /* add rhs: */
   assemble_vec(actintra,
                &(actsolv->sysarray_typ[actsysarray]),
                &(actsolv->sysarray[actsysarray]),
                &(actsolv->rhs[0]),
                frhs,
                1.0
                );

   /*-------------------------------------------------- solve system ---*/
   init=0;
   t1=ds_cputime();
   solver_control(actsolv, actintra,
                  &(actsolv->sysarray_typ[actsysarray]),
                  &(actsolv->sysarray[actsysarray]),
                  &(actsolv->sol[1]),
                  &(actsolv->rhs[0]),
                  init);
   ts=ds_cputime()-t1;
   tss+=ts;

   /*-------------- return solution to the nodes to increment vector ---*/
   solserv_result_incre(actfield,
		        actintra,
		        &(actsolv->sol[1]),
		        ipos.stresspro,
		        &(actsolv->sysarray[actsysarray]),
		        &(actsolv->sysarray_typ[actsysarray]),
		        0);
}
/*+++++++++++++++++++++++++ end stress projection ++++++++++++++++++++++*/

/*---------------------------------------------------- store total area */
if (fdyn->checkarea>0)
{
   dsassert(totarea_a.fdim>=itnum,"cannot store totarea!\n");
   totarea[itnum-1] = fdyn->totarea;
}

/*------------------------------------- solve heightfunction seperately */
if (fdyn->freesurf==3)
fluid_heightfunc(2,&grat,actfield,actpart,actintra,action,
                 &container,converged);

/*---------------------------------- update coordinates at free surface */
if (fdyn->freesurf>1)
fluid_updfscoor(actfield, fdyn, fdyn->dta, 1);

/*---------- based on the new position calculate normal at free surface */
if (itnum==1) fluid_cal_normal(actfield,0,action);


/*----------------------------------------- iteration convergence check */
converged = fluid_convcheck(vrat,prat,grat,itnum,te,ts);

/*--------------------- check if nonlinear iteration has to be finished */
if (converged==0)
{
   itnum++;
   goto nonlniter;
}
/*----------------------------------------------------------------------*
 | -->  end of nonlinear iteration                                      |
 *----------------------------------------------------------------------*/

/*-------------------------------------------------- steady state check */
/*  no steady state check for fsi-problems!!!                           */
/*-------------------------------------- output of area to monitor file */
if (fdyn->checkarea>0) out_area(totarea_a,fdyn->acttime,itnum,0);

/*------------------------- calculate stresses transferred to structure */
if (fsidyn->ifsi>0)
{
   *action = calc_fluid_stress;
   container.nii= 0; 
   container.str=str;
   container.is_relax = 0;
   calelm(actfield,actsolv,actpart,actintra,actsysarray,-1,
          &container,action);

   /* since stresses are stored locally at the element it's necassary to
      reduce them to all procs! ----------------------------------------*/
   dsassert(actsolv->parttyp==cut_elements,
   "Stress reduction for 'cut_nodes' not possible\n");
   fluid_reducestress(actintra,actpart,actfield,fdyn->numdf,str);
   /*----------------------------------------- store stresses in sol_mf */
   solserv_sol_zero(actfield,0,node_array_sol_mf,1);
   fsi_fluidstress_result(actfield,fdyn->numdf);
}


#ifdef D_MORTAR
if(fsidyn->coupmethod == 0) /* mortar method */
{
  /*------- redefine the size of sol_mf from 2 to 3, the third field is */
  /*-------------------- necessary to store the nodal forces due to fsi */
  solserv_sol_zero(actfield, 0, node_array_sol_mf, 3);
}
#endif


if (fsidyn->ifsi>=4)
break;

/*======================================================================*
 |                       F I N A L I S I N G                            |
 *======================================================================*/
case 3:

/*----------------------------------------------- lift&drag computation */
if (fdyn->liftdrag>0)
{
   *action = calc_fluid_liftdrag;
   container.str=str_liftdrag;
   fluid_liftdrag(genprob.numfld,action,&container,actfield,
                  actsolv,actpart,actintra);
}

/*get_pressure(actfield);
if (fabs(fdyn->acttime-1.0) < EPS12) write_coords(actfield);*/

/*---------------------------------------------- update acceleration ---*/
if (fdyn->iop==4) /* for step theta */
{
  /*------------------------ evaluate acceleration in this time step ---*
   *-------------------------------- depending on integration method ---*/
  if (fdyn->step == 1)
  { /* do just a linear interpolation within the first timestep */
    solserv_sol_zero(actfield,0,node_array_sol_increment,ipos.accn);
    solserv_sol_add(actfield,0,node_array_sol_increment,
                               node_array_sol_increment,
                               ipos.velnp,ipos.accn, 1.0/fdyn->dta);
    solserv_sol_add(actfield,0,node_array_sol_increment,
                               node_array_sol_increment,
                               ipos.veln,ipos.accn, -1.0/fdyn->dta);
    solserv_sol_copy(actfield,0,node_array_sol_increment,
                                node_array_sol_increment,ipos.accn,ipos.accnm);
  }
  else
  {
    /* previous acceleration becomes (n-1)-acceleration of next step    */
    leftspace = ipos.accnm;
    ipos.accnm = ipos.accn;
    ipos.accn  = leftspace;
    fluid_acceleration(actfield,fdyn->iop);
  }
}

/*-------------------------------------- make predictor at free surface */
if (fdyn->freesurf>0)
   fluid_updfscoor(actfield, fdyn, fdyn->dta, 0);

/*--------- based on the predictor calculate new normal at free surface */
fluid_cal_normal(actfield,2,action);

/*-------------------------- shift position of old velocity solution ---*/
leftspace = ipos.velnm;
ipos.velnm = ipos.veln;

/*--------------------- shift position of previous velocity solution ---*/
ipos.veln = ipos.velnp;

/*--------- set place for new solution to be solved in the next step ---*/
ipos.velnp = leftspace;
/*---------- it is however necessary to have the newest solution ...
                                             ... still on ipos.velnp ---*/
solserv_sol_copy(actfield,0,node_array_sol_increment,
                            node_array_sol_increment,ipos.veln,ipos.velnp);

/*---------------------- for multifield fluid problems with freesurface */
/*----- copy solution from sol_increment[ipos.veln][j] to sol_mf[0][j] -*/
/* check this for FSI with free surface!!! */
if (fdyn->freesurf>0) solserv_sol_copy(actfield,0,node_array_sol_increment,
                                                  node_array_sol_mf,
                                                  ipos.velnp,0);

/*---------------------------------------------- finalise this timestep */
outstep++;
pssstep++;
restartstep++;

if (pssstep==fsidyn->uppss && ioflags.fluid_vis==1)
{
   pssstep=0;
   /*--------------------------------------------- store time in time_a */
   if (actpos >= time_a.fdim)
      amredef(&(time_a),time_a.fdim+100,1,"DV");
   time_a.a.dv[actpos] = fdyn->acttime;
   actpos++;
}

/*-----copy solution from sol_increment[ipos.veln][j] to sol[actpos][j]
           and transform kinematic to real pressure --------------------*/
solserv_sol_copy(actfield,0,node_array_sol_increment,
                            node_array_sol,ipos.velnp,actpos);
fluid_transpres(actfield,0,0,actpos,fdyn->numdf-1,0);

if (outstep==fdyn->upout && ioflags.output_out==1 && ioflags.fluid_sol==1)
{
   outstep=0;
   out_sol(actfield,actpart,actintra,fdyn->step,actpos);
}

/*------------------------------------------- write restart to pss file */
if (restartstep==fsidyn->uprestart)
{
   restartstep=0;
#ifdef BINIO
   restart_write_bin_fluiddyn(&out_context, fdyn);
#else
   restart_write_fluiddyn(fdyn,actfield,actpart,actintra,action,&container);
#endif
}

/*---------------------------------------------------------- monitoring */
if (ioflags.monitor==1)
monitoring(actfield,numff,actpos,fdyn->acttime);

fsidyn->actpos = actpos;

/*--------------------------------------------------------------------- */
break;

/*======================================================================*
 |     S O L U T I O N    F O R    S T E E P E S T    D E S C E N T     |
 |                        E V A L U A T I O N                           |
 *======================================================================*/
/* nodal solution history fluid field:                                  *
 * sol[0][j]           ... initial data 				*
 * sol[1...actpos][j]  ... solution for visualisation (real pressure)	*
 * sol_increment[0][j] ... solution at time (n-1)			*
 * sol_increment[1][j] ... solution at time (n) 			*
 * sol_increment[2][j] ... solution at time (n+g)			*
 * sol_increment[3][j] ... solution at time (n+1)			*
 * sol_increment[4][i] ... grid velocity time (n) -> (n+1) #		*
 * sol_increment[5][i] ... convective velocity at time (n)		*
 * sol_increment[6][i] ... convective velocity at time (n+1) #	        *
 * sol_increment[7][i] ... fluid solution for relax.-param. steep. desc.*
 * #: these vectors also used for steepest descent calculation		*
 * sol_mf[0][j]        ... solution at time (n+1)			*
 * sol_mf[1][j]        ... nodal stresses at FS-interface at time (n+1) *
 *======================================================================*/
case 6:

dsassert(fsidyn->ifsi = 6,
         "No auxiliary fluid solution within this coupling scheme");

/*- there are only procs allowed in here, that belong to the fluid -----*/
/* intracommunicator (in case of nonlinear fluid. dyn., this should be all)*/
if (actintra->intra_fieldtyp != fluid) break;

/*------------------------------ calculate constants for time algorithm */
fluid_tcons();

/*------------------------------------------------ output to the screen */
if (par.myrank==0) printf("          - Solving FLUID ...\n");
/*--------------------------------------------------------- ALE-PHASE I */
if (fsidyn->iale!=0)
{
  /*----------------------------------------------- change element flag */
  fdyn->ishape=1;
  /*------------------- calculate ALE-convective velocities at time (n) */
  fsi_aleconv(actfield,fdyn->numdf,ipos.convnp,ipos.velnp);
}

/*----------------------------------- set dirichlet boundary conditions */
fluid_setdirich_sd(actfield);

/*------------------------- calculate constants for nonlinear iteration */
/*
nir <->  EVALUATION OF NONLINEAR LHS N-REACTION
nil <->  EVALUATION OF LUMPED MASS MATRIX (Mvv-lumped)
nii <->  EVALUATION OF "ITERATION - RHS"
nis <->  STATIONARY CASE (NO TIMEDEPENDENT TERMS) */
fdyn->nir = 0;
fdyn->nil = 0;
fdyn->nii = 0;
fdyn->nis = 0;

/*--------------------------------- calculate curvature at free surface */
if (fdyn->surftens!=0)
{
   dserror("steepest descent method with free surface not yet implemented.");
   *action = calc_fluid_curvature;
   fluid_curvature(actfield,actpart,actintra,action);
}
/*---------------------------- intitialise global matrix and global rhs */
solserv_zero_vec(&(actsolv->rhs[0]));
solserv_zero_mat(actintra,&(actsolv->sysarray[actsysarray]),
                 &(actsolv->sysarray_typ[actsysarray]));

/*------------------------------------------- initialise iterations-rhs */
amzero(&frhs_a);

/*-------------- form incremental matrices, residual and element forces */
*action = calc_fluid;
t1=ds_cputime();
container.dvec         = NULL;
container.frhs         = frhs;
container.global_numeq = numeq_total;
container.nii          = fdyn->nii;
container.kstep        = 0;
container.fieldtyp     = actfield->fieldtyp;
container.is_relax     = 1;
calelm(actfield,actsolv,actpart,actintra,actsysarray,-1,
       &container,action);
te=ds_cputime()-t1;
tes+=te;

/*------------------------------------------------  build rhs vector ---*/
assemble_vec(actintra,
             &(actsolv->sysarray_typ[actsysarray]),
             &(actsolv->sysarray[actsysarray]),
             &(actsolv->rhs[0]),
             frhs,
             1.0
             );

/*-------------------------------------------------------- solve system */
init=0;
t1=ds_cputime();
solver_control(actsolv, actintra,
               &(actsolv->sysarray_typ[actsysarray]),
               &(actsolv->sysarray[actsysarray]),
               &(actsolv->sol[0]),
               &(actsolv->rhs[0]),
               init);
ts=ds_cputime()-t1;
tss+=ts;

/*-- set flags for stability parameter evaluation and convergence check */
fdyn->ishape=0;

/*----------------- return solution to the nodes to increment vector ---*/
solserv_result_incre(
		     actfield,
		     actintra,
		     &(actsolv->sol[actsysarray]),
		     ipos.relax,
		     &(actsolv->sysarray[actsysarray]),
		     &(actsolv->sysarray_typ[actsysarray]),
		     0);

/*------------------------- calculate stresses transferred to structure */
if (fsidyn->ifsi>0)
{
   *action = calc_fluid_stress;
   container.nii= 0;
   container.str=str;
   container.is_relax = 1;
   calelm(actfield,actsolv,actpart,actintra,actsysarray,-1,
          &container,action);

   /* since stresses are stored locally at the element it's necassary to
      reduce them to all procs! ----------------------------------------*/
   dsassert(actsolv->parttyp==cut_elements,"Stress reduction for 'cut_nodes' not possible\n");
   fluid_reducestress(actintra,actpart,actfield,fdyn->numdf,str);

   /*----------------------------------------- store stresses in sol_mf */
   solserv_sol_zero(actfield,0,node_array_sol_mf,1);
   fsi_fluidstress_result(actfield,fdyn->numdf);
}

break;

/*======================================================================*
                            Binary Output
 *======================================================================*/
case 98:
#ifdef BINIO
  if (ioflags.output_bin==1 && ioflags.fluid_sol==1)
  {
    out_results(&out_context, fdyn->acttime, fdyn->step, actpos, OUTPUT_VELOCITY);
    out_results(&out_context, fdyn->acttime, fdyn->step, actpos, OUTPUT_PRESSURE);
  }
#endif
  break;

/*======================================================================*
 |                C L E A N I N G   U P   P H A S E                     |
 *======================================================================*/
case 99:
/*- there are only procs allowed in here, that belong to the fluid -----*/
/* intracommunicator (in case of nonlinear fluid. dyn., this should be all)*/
if (actintra->intra_fieldtyp != fluid) break;
if (pssstep==0) actpos--;

/*------------------------------------- print out solution to .out file */
if (outstep!=0 && ioflags.output_out==1 && ioflags.fluid_sol==1)
out_sol(actfield,actpart,actintra,fdyn->step,actpos);

/*------------------------------------ print out solution to 0.pss file */
if (ioflags.fluid_vis==1)
{
   if (pssstep!=0)
   {
      /*------------------------------------------ store time in time_a */
      if (actpos >= time_a.fdim)
      amredef(&(time_a),time_a.fdim+1000,1,"DV");
      time_a.a.dv[actpos] = fdyn->acttime;
   }
   if (par.myrank==0) visual_writepss(actfield,actpos+1,&time_a);
}

/*---------------------------------- print total CPU-time to the screen */
#ifdef PARALLEL
MPI_Barrier(actintra->MPI_INTRA_COMM);
#endif
for (i=0;i<par.nprocs;i++)
{
#ifdef PARALLEL
MPI_Barrier(actintra->MPI_INTRA_COMM);
#endif
if (par.myrank==i)
{
printf("\n");
printf("PROC  %3d | FIELD FLUID     | total time element for calculations: %10.3E \n",
        par.myrank,tes);
printf("PROC  %3d | FIELD FLUID     | total time for solver              : %10.3E \n",
        par.myrank,tss);
}
}
#ifdef PARALLEL
MPI_Barrier(actintra->MPI_INTRA_COMM);
#endif

/*------------------------------------------------------------- tidy up */
amdel(&frhs_a);
amdel(&time_a);
if (fdyn->checkarea>0) amdel(&totarea_a);
solserv_del_vec(&(actsolv->rhs),actsolv->nrhs);
solserv_del_vec(&(actsolv->sol),actsolv->nsol);

#ifdef BINIO
destroy_bin_out_field(&out_context);
#endif

#ifndef PARALLEL
CCAFREE(actintra);
#endif
break;
default:
   dserror("Parameter out of range: mctrl \n");
} /* end switch (mctrl) */

#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of fluid_isi */

#endif

/*! @} (documentation module close)*/
