#include "../headers/standardtypes.h"
#include "../headers/solution_mlpcg.h"
#include "../headers/solution.h"
#include "../shell8/shell8.h"
#include "../shell9/shell9.h"
#include "../wall1/wall1.h"
#include "../brick1/brick1.h"
#include "../fluid3/fluid3.h"
#include "../fluid3/fluid3_prototypes.h"
#include "../fluid2/fluid2.h"
#include "../fluid2/fluid2_prototypes.h"
#include "../ale3/ale3.h"
#include "../ale2/ale2.h"

/*----------------------------------------------------------------------*
 | enum _CALC_ACTION                                      m.gee 1/02    |
 | command passed from control routine to the element level             |
 | to tell element routines what to do                                  |
 | defined globally in global_calelm.c                                  |
 *----------------------------------------------------------------------*/
enum _CALC_ACTION calc_action[MAXFIELD];
/*----------------------------------------------------------------------*
 | global dense matrices for element routines             m.gee 7/01    |
 *----------------------------------------------------------------------*/
struct _ARRAY estif_global;    /* element stiffness matrix              */
struct _ARRAY emass_global;    /* element mass matrix                   */  
struct _ARRAY etforce_global;  /* element Time RHS                      */
struct _ARRAY eproforce_global;  /* element Time RHS                    */
struct _ARRAY eiforce_global;  /* element Iteration RHS                 */
struct _ARRAY edforce_global;  /* element dirichlet RHS                 */
struct _ARRAY intforce_global;
/*----------------------------------------------------------------------*
 |  routine to call elements                             m.gee 6/01     |
 *----------------------------------------------------------------------*/
void calelm(FIELD        *actfield,     /* active field */        
            SOLVAR       *actsolv,      /* active SOLVAR */
            PARTITION    *actpart,      /* my partition of this field */
            INTRA        *actintra,     /* my intra-communicator */
            INT           sysarray1,    /* number of first sparse system matrix */
            INT           sysarray2,    /* number of secnd system matrix, if present, else -1 */
            CONTAINER    *container,    /* contains variables defined in container.h */
            CALC_ACTION  *action)       /* calculation option passed to element routines */     
/*----------------------------------------------------------------------*/
{
INT               i,kk;
INT               hasdirich=0;      /* flag                             */
INT               hasext=0;         /* flag                             */
ELEMENT          *actele;
ELEMENT          *actele2;
SPARSE_TYP        sysarray1_typ;
SPARSE_TYP        sysarray2_typ;
ASSEMBLE_ACTION   assemble_action;

#ifdef DEBUG 
dstrc_enter("calelm");
#endif
/*----------------------------------------------------------------------*/
/*-------------- zero the parallel coupling exchange buffers if present */  
#ifdef PARALLEL 
/*------------------------ check the send & recv buffers from sysarray1 */
if (sysarray1 != -1)
{
   switch(actsolv->sysarray_typ[sysarray1])
   {
   case msr:
      if (actsolv->sysarray[sysarray1].msr->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].msr->couple_d_send);
      if (actsolv->sysarray[sysarray1].msr->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].msr->couple_d_recv);
   break;
   case parcsr:
      if (actsolv->sysarray[sysarray1].parcsr->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].parcsr->couple_d_send);
      if (actsolv->sysarray[sysarray1].parcsr->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].parcsr->couple_d_recv);
   break;
   case ucchb:
      if (actsolv->sysarray[sysarray1].ucchb->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].ucchb->couple_d_send);
      if (actsolv->sysarray[sysarray1].ucchb->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].ucchb->couple_d_recv);
   break;
   case dense:
      if (actsolv->sysarray[sysarray1].dense->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].dense->couple_d_send);
      if (actsolv->sysarray[sysarray1].dense->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].dense->couple_d_recv);
   break;
   case rc_ptr:
      if (actsolv->sysarray[sysarray1].rc_ptr->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].rc_ptr->couple_d_send);
      if (actsolv->sysarray[sysarray1].rc_ptr->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].rc_ptr->couple_d_recv);
   break;
   case ccf:
      if (actsolv->sysarray[sysarray1].ccf->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].ccf->couple_d_send);
      if (actsolv->sysarray[sysarray1].ccf->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].ccf->couple_d_recv);
   break;
   case skymatrix:
      if (actsolv->sysarray[sysarray1].sky->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].sky->couple_d_send);
      if (actsolv->sysarray[sysarray1].sky->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].sky->couple_d_recv);
   break;
   case spoolmatrix:
      if (actsolv->sysarray[sysarray1].spo->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].spo->couple_d_send);
      if (actsolv->sysarray[sysarray1].spo->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].spo->couple_d_recv);
   break;
   case oll:
      if (actsolv->sysarray[sysarray1].oll->couple_d_send)
         amzero(actsolv->sysarray[sysarray1].oll->couple_d_send);
      if (actsolv->sysarray[sysarray1].oll->couple_d_recv)
         amzero(actsolv->sysarray[sysarray1].oll->couple_d_recv);
   break;
   case bdcsr:;
   break;
   default:
      dserror("Unknown typ of system matrix");
   break;
   }
}
/*------------------------ check the send & recv buffers from sysarray2 */
if (sysarray2 != -1)
{
   switch(actsolv->sysarray_typ[sysarray2])
   {
   case msr:
      if (actsolv->sysarray[sysarray2].msr->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].msr->couple_d_send);
      if (actsolv->sysarray[sysarray2].msr->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].msr->couple_d_send);
   break;
   case parcsr:
      if (actsolv->sysarray[sysarray2].parcsr->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].parcsr->couple_d_send);
      if (actsolv->sysarray[sysarray2].parcsr->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].parcsr->couple_d_send);
   break;
   case ucchb:
      if (actsolv->sysarray[sysarray2].ucchb->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].ucchb->couple_d_send);
      if (actsolv->sysarray[sysarray2].ucchb->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].ucchb->couple_d_send);
   break;
   case dense:
      if (actsolv->sysarray[sysarray2].dense->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].dense->couple_d_send);
      if (actsolv->sysarray[sysarray2].dense->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].dense->couple_d_send);
   break;
   case rc_ptr:
      if (actsolv->sysarray[sysarray2].rc_ptr->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].rc_ptr->couple_d_send);
      if (actsolv->sysarray[sysarray2].rc_ptr->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].rc_ptr->couple_d_recv);
   break;
   case ccf:
      if (actsolv->sysarray[sysarray2].ccf->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].ccf->couple_d_send);
      if (actsolv->sysarray[sysarray2].ccf->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].ccf->couple_d_recv);
   break;
   case skymatrix:
      if (actsolv->sysarray[sysarray2].sky->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].sky->couple_d_send);
      if (actsolv->sysarray[sysarray2].sky->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].sky->couple_d_recv);
   break;
   case spoolmatrix:
      if (actsolv->sysarray[sysarray2].spo->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].spo->couple_d_send);
      if (actsolv->sysarray[sysarray2].spo->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].spo->couple_d_recv);
   break;
   case oll:
      if (actsolv->sysarray[sysarray2].oll->couple_d_send)
         amzero(actsolv->sysarray[sysarray2].oll->couple_d_send);
      if (actsolv->sysarray[sysarray2].oll->couple_d_recv)
         amzero(actsolv->sysarray[sysarray2].oll->couple_d_recv);
   break;
   case bdcsr:;
   break;
   default:
      dserror("Unknown typ of system matrix");
   break;
   }
}
#endif
/* =======================================================call elements */
/*---------------------------------------------- loop over all elements */
kk = container->actndis;
for (i=0; i<actpart->pdis[kk].numele; i++)
{
   /*------------------------------------ set pointer to active element */
   actele = actpart->pdis[kk].element[i];
   /* if present, init the element vectors intforce_global and dirich_global */
   if (container->dvec) amzero(&intforce_global);
   switch(actele->eltyp)/*======================= call element routines */
   {
   case el_shell8:
      container->handsize = 0;
      container->handles  = NULL;
      shell8(actfield,actpart,actintra,actele,
             &estif_global,&emass_global,&intforce_global,
             action,container);
   break;
   case el_shell9:
      container->handsize = 0;
      container->handles  = NULL;
      shell9(actfield,actpart,actintra,actele,
             &estif_global,&emass_global,&intforce_global,
             action,container);
   break;
   case el_brick1:
      brick1(actpart,actintra,actele,
             &estif_global,&emass_global,&intforce_global,
             action,container);
   break;
   case el_wall1:
      container->handsize = 0;
      container->handles  = NULL;
      wall1(actpart,actintra,actele,
            &estif_global,&emass_global,&intforce_global,
            action, container);
   break;
   case el_fluid2: 
#ifdef D_FLUID
      if(container->turbu==2 || container->turbu==3) actele2 = actpart->pdis[1].element[i];
      else                                           actele2 = NULL;
      fluid2(actpart,actintra,actele,actele2,
             &estif_global,&emass_global,
             &etforce_global,&eiforce_global,&edforce_global,
	       action,&hasdirich,&hasext,container);
#endif
   break;
   case el_fluid2_tu: 
      actele2 = actpart->pdis[0].element[i];
      fluid2_tu(actpart,actintra,actele,actele2,
                &estif_global,&emass_global,
                &etforce_global,&eiforce_global,&edforce_global,&eproforce_global,
	          action,&hasdirich,&hasext,container);
   break;
   case el_fluid3: 
      fluid3(actpart,actintra,actele,
             &estif_global,&emass_global,
             &etforce_global,&eiforce_global,&edforce_global,
	     action,&hasdirich,&hasext,container); 
   break;
   case el_ale3:
	ale3(actpart,actintra,actele,
        &estif_global,
        action,container);
   break;
   case el_ale2:
	ale2(actpart,actintra,actele,
        &estif_global,
        action,container);
   break;
   case el_none:
      dserror("Typ of element unknown");
   break;
   default:
      dserror("Typ of element unknown");
   }/* end of calling elements */


   switch(*action)/*=== call assembly dependent on calculation-flag */
   {
   case calc_struct_linstiff        : assemble_action = assemble_one_matrix; break;
   case calc_struct_nlnstiff        : assemble_action = assemble_one_matrix; break;
   case calc_struct_nlnstiffmass    : assemble_action = assemble_two_matrix; break;
   case calc_struct_linstifflmass   : assemble_action = assemble_one_matrix; break;
   case calc_struct_internalforce   : assemble_action = assemble_do_nothing; break;
   case calc_struct_eleload         : assemble_action = assemble_do_nothing; break;
   case calc_struct_fsiload         : assemble_action = assemble_do_nothing; break;
   case calc_struct_stress          : assemble_action = assemble_do_nothing; break;
   case calc_struct_ste             : assemble_action = assemble_do_nothing; break;
   case calc_struct_stm             : assemble_action = assemble_do_nothing; break;
   case calc_struct_def             : assemble_action = assemble_do_nothing; break;
   case calc_struct_stv             : assemble_action = assemble_do_nothing; break;
   case calc_struct_dee             : assemble_action = assemble_do_nothing; break;
   case calc_deriv_self_adj         : assemble_action = assemble_do_nothing; break;
   case calc_struct_dmc             : assemble_action = assemble_do_nothing; break;
   case update_struct_odens         : assemble_action = assemble_do_nothing; break;
   case calc_struct_update_istep    : assemble_action = assemble_do_nothing; break;
   case calc_struct_update_stepback : assemble_action = assemble_do_nothing; break;
   case calc_ale_stiff              : assemble_action = assemble_one_matrix; break;
   case calc_ale_rhs                : assemble_action = assemble_do_nothing; break;
   case calc_ale_stiff_nln          : assemble_action = assemble_one_matrix; break;
   case calc_ale_stiff_stress       : assemble_action = assemble_one_matrix; break;
   case calc_ale_stiff_step2        : assemble_action = assemble_one_matrix; break;
   case calc_ale_stiff_spring       : assemble_action = assemble_one_matrix; break;
   case calc_ale_stiff_laplace      : assemble_action = assemble_one_matrix; break;
   case calc_fluid                  : assemble_action = assemble_one_matrix; break;
   case calc_fluid_vort             : assemble_action = assemble_do_nothing; break;
   case calc_fluid_stress           : assemble_action = assemble_do_nothing; break;
   case calc_fluid_shearvelo        : assemble_action = assemble_do_nothing; break;
   default: dserror("Unknown type of assembly 1"); break;
   }
   /*--------------------------- assemble one or two system matrices */
   assemble(sysarray1,
            &estif_global,
            sysarray2,
            &emass_global,
            actpart,
            actsolv,
            actintra,
            actele,
            assemble_action,
            container);
   /*---------------------------- assemble the vector intforce_global */
   switch(container->fieldtyp)
   {
   case structure:
      /*------------------ assemble internal force or external forces */
      if (container->dvec)
      assemble_intforce(actele,&intforce_global,container,actintra);
      /*--- assemble the rhs vector of condensed dirichlet conditions */
      /* static case */
      if (container->dirich && container->isdyn==0)
      assemble_dirich(actele,&estif_global,container);
      /* dynamic case */
      if (container->dirich && container->isdyn==1)
      assemble_dirich_dyn(actele,&estif_global,&emass_global,container);
   break;
#ifdef D_FLUID
   case fluid:
      if (container->nif!=0)
      {
         container->dvec = container->ftimerhs;
         assemble_intforce(actele,&etforce_global,container,actintra);
      }
   /*-------------- assemble the vector eiforce_global to iteration rhs */
      if (container->nii+hasext!=0)
      {   
         container->dvec = container->fiterhs;
         assemble_intforce(actele,&eiforce_global,container,actintra); 
      }
   /*-------------- assemble the vector edforce_global to iteration rhs */
      if (hasdirich!=0)
      {
         container->dvec = container->fiterhs;
         assemble_intforce(actele,&edforce_global,container,actintra);
      }
      if (container->actndis==1 && (container->turbu==2 || container->turbu==3))
      {
         if (container->niturbu_pro!=0)
         {
          container->dvec = container->ftimerhs_pro;
          assemble_intforce(actele,&eproforce_global,container,actintra);
         }
         if (container->niturbu_n!=0)
         {
          container->dvec = container->ftimerhs;
          assemble_intforce(actele,&etforce_global,container,actintra);
         }
         container->dvec = container->fiterhs;
         assemble_intforce(actele,&eiforce_global,container,actintra);
      }   
      container->dvec=NULL;   
   break;
#endif   
#ifdef D_ALE
   case ale:
   if (container->dirich && container->isdyn == 1)
      {
         hasdirich = check_ale_dirich(actele);
	 if (hasdirich)
            ale_caldirich_increment(actele,container->dirich,
                          container->global_numeq,&estif_global,
			  container->pos); 
      }
   break;
#endif 
   default:
      dserror("fieldtyp unknown!");
   }
}/* end of loop over elements */
/*----------------------------------------------------------------------*/
/*                    in parallel coupled dofs have to be exchanged now */
/*             (if there are any inter-proc couplings, which is tested) */
/*----------------------------------------------------------------------*/
#ifdef PARALLEL 
switch(*action)
{
case calc_struct_linstiff        : assemble_action = assemble_one_exchange; break;
case calc_struct_nlnstiff        : assemble_action = assemble_one_exchange; break;
case calc_struct_internalforce   : assemble_action = assemble_do_nothing;   break;
case calc_struct_nlnstiffmass    : assemble_action = assemble_two_exchange; break;
case calc_struct_linstifflmass   : assemble_action = assemble_one_exchange; break;
case calc_struct_eleload         : assemble_action = assemble_do_nothing;   break;
case calc_struct_fsiload         : assemble_action = assemble_do_nothing;   break;
case calc_struct_stress          : assemble_action = assemble_do_nothing;   break;
case calc_struct_ste             : assemble_action = assemble_do_nothing;   break;
case calc_struct_stm             : assemble_action = assemble_do_nothing;   break;
case calc_struct_def             : assemble_action = assemble_do_nothing;   break;
case calc_struct_stv             : assemble_action = assemble_do_nothing;   break;
case calc_struct_dmc             : assemble_action = assemble_do_nothing;   break;
case update_struct_odens         : assemble_action = assemble_do_nothing;   break;
case calc_struct_dee             : assemble_action = assemble_do_nothing;   break;
case calc_deriv_self_adj         : assemble_action = assemble_do_nothing;   break;
case calc_struct_update_istep    : assemble_action = assemble_do_nothing;   break;
case calc_struct_update_stepback : assemble_action = assemble_do_nothing;   break;
case calc_ale_stiff              : assemble_action = assemble_one_exchange; break;
case calc_ale_stiff_nln          : assemble_action = assemble_one_exchange; break;
case calc_ale_stiff_stress       : assemble_action = assemble_one_exchange; break;
case calc_ale_stiff_step2        : assemble_action = assemble_one_exchange; break;
case calc_ale_stiff_spring       : assemble_action = assemble_one_exchange; break;
case calc_ale_stiff_laplace      : assemble_action = assemble_one_exchange; break;
case calc_ale_rhs                : assemble_action = assemble_do_nothing;   break;
case calc_fluid                  : assemble_action = assemble_one_exchange; break;
case calc_fluid_vort             : assemble_action = assemble_do_nothing;   break;
case calc_fluid_stress           : assemble_action = assemble_do_nothing;   break;
case calc_fluid_shearvelo        : assemble_action = assemble_do_nothing;   break;
default: dserror("Unknown type of assembly 2"); break;
}
/*------------------------------ exchange coupled dofs, if there are any */
assemble(sysarray1,
         NULL,
         sysarray2,
         NULL,
         actpart,
         actsolv,
         actintra,
         actele,
         assemble_action,
         container);
#endif
/*----------------------------------------------------------------------*/
/* 
   in the case of dynamically increasing sparse matrices (spooles) the 
   matrix has to be closed after assembly
*/
#ifdef D_CONTACT
switch(*action)
{
case calc_struct_linstiff        : assemble_action = assemble_close_1matrix; break;
case calc_struct_nlnstiff        : assemble_action = assemble_close_1matrix; break;
case calc_struct_internalforce   : assemble_action = assemble_do_nothing;   break;
case calc_struct_nlnstiffmass    : assemble_action = assemble_close_2matrix; break;
case calc_struct_linstifflmass   : assemble_action = assemble_close_1matrix; break;
case calc_struct_eleload         : assemble_action = assemble_do_nothing;    break;
case calc_struct_fsiload         : assemble_action = assemble_do_nothing;    break;
case calc_struct_stress          : assemble_action = assemble_do_nothing;    break;
case calc_struct_ste             : assemble_action = assemble_do_nothing;   break;
case calc_struct_stm             : assemble_action = assemble_do_nothing;   break;
case calc_struct_def             : assemble_action = assemble_do_nothing;   break;
case calc_struct_stv             : assemble_action = assemble_do_nothing;   break;
case calc_struct_dmc             : assemble_action = assemble_do_nothing;   break;
case update_struct_odens         : assemble_action = assemble_do_nothing;   break;
case calc_struct_dee             : assemble_action = assemble_do_nothing;   break;
case calc_deriv_self_adj         : assemble_action = assemble_do_nothing;   break;
case calc_struct_update_istep    : assemble_action = assemble_do_nothing;   break;
case calc_struct_update_stepback : assemble_action = assemble_do_nothing;   break;
case calc_ale_stiff              : assemble_action = assemble_close_1matrix; break;
case calc_ale_rhs                : assemble_action = assemble_do_nothing;    break;
case calc_ale_stiff_nln          : assemble_action = assemble_close_1matrix; break;
case calc_ale_stiff_stress       : assemble_action = assemble_close_1matrix; break;
case calc_ale_stiff_step2        : assemble_action = assemble_close_1matrix; break;
case calc_ale_stiff_spring       : assemble_action = assemble_close_1matrix; break;
case calc_ale_stiff_laplace      : assemble_action = assemble_one_matrix; break;
case calc_fluid                  : assemble_action = assemble_close_1matrix; break;
case calc_fluid_vort             : assemble_action = assemble_do_nothing;   break;
case calc_fluid_stress           : assemble_action = assemble_do_nothing;   break;
case calc_fluid_shearvelo        : assemble_action = assemble_do_nothing;   break;
default: dserror("Unknown type of assembly 3"); break;
}
assemble(sysarray1,
         NULL,
         sysarray2,
         NULL,
         actpart,
         actsolv,
         actintra,
         NULL,
         assemble_action,
         container);
#endif
/*----------------------------------------------------------------------*/
if(actsolv->sysarray_typ[sysarray1]==oll)
{
  switch(*action)/*=== set flag dependent on calculation-flag */
  {
    case calc_struct_nlnstiffmass    : 
      actsolv->sysarray[sysarray2].oll->is_masked = 1;
    case calc_struct_linstiff        :
    case calc_struct_nlnstiff        : 
      actsolv->sysarray[sysarray1].oll->is_masked = 1; break;
    case calc_struct_internalforce   :
    case calc_struct_eleload         :
    case calc_struct_stress          :
    case calc_struct_ste             :
    case calc_struct_stm             :
    case calc_struct_def             :
    case calc_struct_stv             :
    case calc_struct_dee             :
    case calc_deriv_self_adj         : 
    case calc_struct_dmc             :
    case update_struct_odens         :
    case calc_struct_update_istep    :
    case calc_struct_update_stepback : break;
    case calc_ale_stiff              : 
      actsolv->sysarray[sysarray1].oll->is_masked = 1; break;
    case calc_ale_rhs                : break;
    case calc_fluid                  :
      actsolv->sysarray[sysarray1].oll->is_masked = 1; break;
    default: dserror("Unknown type of assembly 1"); break;
  }
}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of calelm */





/*----------------------------------------------------------------------*
 |  routine to call elements to init                     m.gee 7/01     |
 *----------------------------------------------------------------------*/
void calinit(FIELD       *actfield,   /* the active physical field */ 
             PARTITION   *actpart,    /* my partition of this field */
             CALC_ACTION *action,
             CONTAINER   *container)  /*!< contains variables defined in container.h */
{
INT i;                        /* a counter */
INT kk;                       
INT is_shell8=0;              /* flags to check for presents of certain element types */
INT is_shell9=0;   
INT is_brick1=0;
INT is_wall1 =0;
INT is_fluid2=0;
INT is_fluid2_tu=0;
INT is_fluid3=0;
INT is_ale3=0;
INT is_ale2=0;

ELEMENT *actele;              /* active element */
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("calinit");
#endif
/*-------------------------- define dense element matrices for assembly */
if (estif_global.Typ != cca_DA)
{
amdef("estif",&estif_global,(MAXNOD*MAXDOFPERNODE),(MAXNOD*MAXDOFPERNODE),"DA");
amdef("emass",&emass_global,(MAXNOD*MAXDOFPERNODE),(MAXNOD*MAXDOFPERNODE),"DA");
amdef("etforce",&etforce_global,(MAXNOD*MAXDOFPERNODE),1,"DV");
amdef("eproforce",&eproforce_global,(MAXNOD*MAXDOFPERNODE),1,"DV");
amdef("eiforce",&eiforce_global,(MAXNOD*MAXDOFPERNODE),1,"DV");
amdef("edforce",&edforce_global,(MAXNOD*MAXDOFPERNODE),1,"DV");
amdef("inforce",&intforce_global,(MAXNOD*MAXDOFPERNODE),1,"DV");
}
/*--------------------what kind of elements are there in this example ? */
for (kk=0;kk<actfield->ndis;kk++)
for (i=0; i<actfield->dis[kk].numele; i++)
{
   actele = &(actfield->dis[kk].element[i]);
   switch(actele->eltyp)
   {
   case el_shell8:
      is_shell8=1;
   break;
   case el_shell9:
      is_shell9=1;
   break;
   case el_brick1:
      is_brick1=1;
   break;
   case el_wall1:
      is_wall1=1;
   break;
   case el_fluid2:
      is_fluid2=1;
   break;
   case el_fluid2_tu:
      is_fluid2_tu=1;
   break;
   case el_fluid3:
      is_fluid3=1;
   break;
   case el_ale3:
      is_ale3=1;
   break;
   case el_ale2:
      is_ale2=1;
   break;
   default:
      dserror("Unknown typ of element");
   break;   
   }
}/* end of loop over all elements */
/*--------------------- init the element routines for all present types */
container->kstep = 0;  
/*------------------------------- init all kind of routines for shell8  */
if (is_shell8==1)
{
   container->handsize = 0;
   container->handles  = NULL;
   shell8(actfield,actpart,NULL,NULL,&estif_global,&emass_global,&intforce_global,
          action,container);
}
/*------------------------------- init all kind of routines for shell9  */
if (is_shell9==1)
{
   container->handsize = 0;
   container->handles  = NULL;
   shell9(actfield,actpart,NULL,NULL,&estif_global,&emass_global,&intforce_global,
          action,container);
}
/*-------------------------------- init all kind of routines for brick1 */
if (is_brick1==1)
{
   brick1(actpart,NULL,NULL,&estif_global,&emass_global,NULL,action,container);
}
/*-------------------------------- init all kind of routines for wall1  */
if (is_wall1==1)
{
   container->handsize = 0;
   container->handles  = NULL;
   wall1(actpart,NULL,NULL,&estif_global,&emass_global,&intforce_global,
         action,container);
}
/*-------------------------------- init all kind of routines for fluid2 */
if (is_fluid2==1)
{
   fluid2(actpart,NULL,NULL,NULL,
          &estif_global,&emass_global,
          &etforce_global,&eiforce_global,&edforce_global,
          action,NULL,NULL,container);
}
/*-------------------------------- init all kind of routines for fluid2_tu */
if (is_fluid2_tu==1)
{
   fluid2_tu(actpart,NULL,NULL,NULL,
             &estif_global,&emass_global,
             &etforce_global,&eiforce_global,&edforce_global,
             &eproforce_global,action,NULL,NULL,container);
}
/*-------------------------------- init all kind of routines for fluid3 */
if (is_fluid3==1)
{
   fluid3(actpart,NULL,NULL,
          &estif_global,&emass_global,
          &etforce_global,&eiforce_global,&edforce_global,
          action,NULL,NULL,container);
}
/*----------------------------------- init all kind of routines for ale */
if (is_ale3==1)
{
   ale3(actpart,NULL,NULL,&estif_global,action,container);
}
/*----------------------------------- init all kind of routines for ale */
if (is_ale2==1)
{
   ale2(actpart,NULL,NULL,&estif_global,action,container);
}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of calinit */







/*----------------------------------------------------------------------*
 |  in here the element's results are made redundant     m.gee 12/01    |
 *----------------------------------------------------------------------*/
void calreduce(FIELD       *actfield, /* the active field */
               PARTITION   *actpart,  /* my partition of this field */
               INTRA       *actintra, /* the field's intra-communicator */
               CALC_ACTION *action,   /* action for element routines */
               CONTAINER   *container)/* contains variables defined in container.h */
{
INT i;
INT is_shell8=0;
INT is_shell9=0;
INT is_brick1=0;
INT is_wall1 =0;
INT is_fluid1=0;
INT is_fluid3=0;
INT is_ale3=0;
ELEMENT *actele;
#ifdef DEBUG 
dstrc_enter("calreduce");
#endif
/*----------------------------------------------------------------------*/
/*--------------------what kind of elements are there in this example ? */
for (i=0; i<actfield->dis[0].numele; i++)
{
   actele = &(actfield->dis[0].element[i]);
   switch(actele->eltyp)
   {
   case el_shell8:
      is_shell8=1;
   break;
   case el_shell9:
      is_shell9=1;
   break;
   case el_brick1:
      is_brick1=1;
   break;
   case el_wall1:
      is_wall1=1;
   break;
   case el_fluid2:
      is_fluid1=1;
   break;
   case el_fluid3:
      is_fluid3=1;
   break;
   case el_ale3:
      is_ale3=1;
   break;
   default:
      dserror("Unknown typ of element");
   break;   
   }
}/* end of loop over all elements */
/*-------------------------------------------reduce results for shell8  */
if (is_shell8==1)
{
   container->handsize = 0;
   container->handles  = NULL;
   shell8(actfield,actpart,actintra,NULL,NULL,NULL,NULL,action,container);
}
/*-------------------------------------------reduce results for shell9  */
if (is_shell9==1)
{
   container->handsize = 0;
   container->handles  = NULL;
   shell9(actfield,actpart,actintra,NULL,NULL,NULL,NULL,action,container);
}
/*--------------------------------------------reduce results for brick1 */
if (is_brick1==1)
{
}
/*---------------------------------------------reduce results for wall1 */
if (is_wall1==1)
{
}
/*--------------------------------------------reduce results for fluid1 */
if (is_fluid1==1)
{
}
/*--------------------------------------------reduce results for fluid3 */
if (is_fluid3==1)
{
}
/*-----------------------------------------------reduce results for ale */
if (is_ale3==1)
{
}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of calreduce */
