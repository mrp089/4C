/*!----------------------------------------------------------------------
\file
\brief 

<pre>
Maintainer: Malte Neumann
            neumann@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/neumann/
            0711 - 685-6121
</pre>

*----------------------------------------------------------------------*/
#include "../headers/standardtypes.h"
#include "../solver/solver.h"
/*----------------------------------------------------------------------*
 | global dense matrices for element routines             m.gee 9/01    |
 | (defined in global_calelm.c, so they are extern here)                |                
 *----------------------------------------------------------------------*/
extern struct _ARRAY estif_global;
extern struct _ARRAY emass_global;

/*----------------------------------------------------------------------*
 |  routine to assemble element arrays to global sparse arrays m.gee 9/01|
 *----------------------------------------------------------------------*/
void assemble(
                 INT                    sysarray1, /* number of first sparse system matrix */
                 struct _ARRAY         *elearray1, /* pointer to first dense element matrix */
                 INT                    sysarray2, /* number of first sparse system matrix or -1 if not given */
                 struct _ARRAY         *elearray2, /* pointer to second dense element matrix or NULL is not present*/
                 struct _PARTITION     *actpart,   /* my partition of theactive field */
                 struct _SOLVAR        *actsolv,   /* the active SOLVAR */
                 struct _INTRA         *actintra,  /* the active intracommunicator */
                 struct _ELEMENT       *actele,    /* the element to assemble */
                 enum _ASSEMBLE_ACTION  assemble_action,  /* the assembly option */
                 CONTAINER             *container  /* contains variables defined in container.h */
                )
/*----------------------------------------------------------------------*/
{
enum  _SPARSE_TYP    sysa1_typ;
union _SPARSE_ARRAY *sysa1;
enum  _SPARSE_TYP    sysa2_typ;
union _SPARSE_ARRAY *sysa2;
#ifdef DEBUG 
dstrc_enter("assemble");
#endif
/*----------------------------------------------------------------------*/
if (assemble_action==assemble_do_nothing) goto end;
/*----------------------- check for presence and typ of system matrices */
if (sysarray1>=0) 
{
   sysa1       = &(actsolv->sysarray[sysarray1]);
   sysa1_typ   =   actsolv->sysarray_typ[sysarray1]; 
}
else              
{
   sysa1     = NULL;
   sysa1_typ = sparse_none; 
}
if (sysarray2>=0) 
{
   sysa2     = &(actsolv->sysarray[sysarray2]);
   sysa2_typ =   actsolv->sysarray_typ[sysarray2];
}
else              
{
   sysa2     = NULL;
   sysa2_typ = sparse_none;
}
/*----------------------------------------------------------------------*/
if (assemble_action==assemble_two_matrix)
{
   if (sysa1_typ != sysa2_typ)
   dserror("Assembly of element matrices in different types of sparse mat. not impl.");
/*------------------------------------------------ switch typ of matrix */
/*-------------------------------------------- add to 2 system matrices */
   switch(sysa1_typ)
   {
#ifdef MLIB_PACKAGE
   case mds:
      dserror("Simultanous assembly of 2 system matrices not yet impl.");
   break;
#endif

#ifdef AZTEC_PACKAGE
   case msr:
      add_msr(actpart,actsolv,actintra,actele,sysa1->msr,sysa2->msr);
   break;
#endif

#ifdef HYPRE_PACKAGE
   case parcsr:
      dserror("Simultanous assembly of 2 system matrices not yet impl.");
   break;
#endif

#ifdef PARSUPERLU_PACKAGE
   case ucchb:
      dserror("Simultanous assembly of 2 system matrices not yet impl.");
   break;
#endif

   case dense:
      add_dense(actpart,actsolv,actintra,actele,sysa1->dense,sysa2->dense);
   break;

#ifdef MUMPS_PACKAGE
   case rc_ptr:
      add_rc_ptr(actpart,actsolv,actintra,actele,sysa1->rc_ptr,sysa2->rc_ptr);
   break;
#endif

#ifdef UMFPACK
   case ccf:
      add_ccf(actpart,actsolv,actintra,actele,sysa1->ccf,sysa2->ccf);
   break;
#endif

   case skymatrix:
      add_skyline(actpart,actsolv,actintra,actele,sysa1->sky,sysa2->sky);
   break;

#ifdef SPOOLES_PACKAGE
   case spoolmatrix:
      add_spo(actpart,actsolv,actintra,actele,sysa1->spo,sysa2->spo);
   break;
#endif

#ifdef MLPCG
   case bdcsr:
      add_bdcsr(actpart,actsolv,actintra,actele,sysa1->bdcsr,sysa2->bdcsr);
   break;
#endif

   case oll:
      add_oll(actpart,actintra,actele,sysa1->oll,sysa2->oll);
   break;

   case sparse_none:
      dserror("Unspecified type of system matrix");
   break;

   default:
      dserror("Unspecified type of system matrix");
   break;
   }
}
/*--------------------------------------------- add to 1 system matrix */
if (assemble_action==assemble_one_matrix)
{
   switch(sysa1_typ)
   {

#ifdef MLIB_PACKAGE
   case mds:
      add_mds(actpart,actsolv,actele,sysa1->mds);
   break;
#endif

#ifdef AZTEC_PACKAGE
   case msr:
      add_msr(actpart,actsolv,actintra,actele,sysa1->msr,NULL);
   break;
#endif

#ifdef HYPRE_PACKAGE
   case parcsr:
      add_parcsr(actpart,actsolv,actintra,actele,sysa1->parcsr);
   break;
#endif

#ifdef PARSUPERLU_PACKAGE
   case ucchb:
      add_ucchb(actpart,actsolv,actintra,actele,sysa1->ucchb);
   break;
#endif

   case dense:
      add_dense(actpart,actsolv,actintra,actele,sysa1->dense,NULL);
   break;

#ifdef MUMPS_PACKAGE
   case rc_ptr:
      add_rc_ptr(actpart,actsolv,actintra,actele,sysa1->rc_ptr,NULL);
   break;
#endif

#ifdef UMFPACK
   case ccf:
      add_ccf(actpart,actsolv,actintra,actele,sysa1->ccf,NULL);
   break;
#endif

   case skymatrix:
      add_skyline(actpart,actsolv,actintra,actele,sysa1->sky,NULL);
   break;

#ifdef SPOOLES_PACKAGE
   case spoolmatrix:
      add_spo(actpart,actsolv,actintra,actele,sysa1->spo,NULL);
   break;
#endif

#ifdef MLPCG
   case bdcsr:
      add_bdcsr(actpart,actsolv,actintra,actele,sysa1->bdcsr,NULL);
   break;
#endif

   case oll:
      add_oll(actpart,actintra,actele,sysa1->oll,NULL);
   break;

   case sparse_none:
      dserror("Unspecified typ of system matrix");
   break;

   default:
      dserror("Unspecified typ of system matrix");
   break;
   }
}
/*----------------- close the system matrix, or close two system matices */
if (assemble_action==assemble_close_1matrix)
{
   switch(sysa1_typ)
   {
#ifdef MLIB_PACKAGE
   case mds:
   break;
#endif

#ifdef AZTEC_PACKAGE
   case msr:
   break;
#endif

#ifdef HYPRE_PACKAGE
   case parcsr:
   break;
#endif

#ifdef PARSUPERLU_PACKAGE
   case ucchb:
   break;
#endif

   case dense:
   break;

#ifdef MUMPS_PACKAGE
   case rc_ptr:
   break;
#endif

#ifdef UMFPACK
   case ccf:
   break;
#endif

   case skymatrix:
   break;

#ifdef SPOOLES_PACKAGE
   case spoolmatrix:
      close_spooles_matrix(sysa1->spo,actintra);
   break;
#endif

   case oll:
   break;

   case sparse_none:
      dserror("Unspecified typ of system matrix");
   break;

   default:
      dserror("Unspecified typ of system matrix");
   break;
   }
}
if (assemble_action==assemble_close_2matrix)
{
   switch(sysa1_typ)
   {
#ifdef MLIB_PACKAGE
   case mds:
   break;
#endif

#ifdef AZTEC_PACKAGE
   case msr:
   break;
#endif

#ifdef HYPRE_PACKAGE
   case parcsr:
   break;
#endif

#ifdef PARSUPERLU_PACKAGE
   case ucchb:
   break;
#endif

   case dense:
   break;

#ifdef MUMPS_PACKAGE
   case rc_ptr:
   break;
#endif


#ifdef UMFPACK
   case ccf:
   break;
#endif

   case skymatrix:
   break;

#ifdef SPOOLES_PACKAGE
   case spoolmatrix:
      close_spooles_matrix(sysa1->spo,actintra);
      close_spooles_matrix(sysa2->spo,actintra);
   break;
#endif

   case oll:
   break;
   
   case sparse_none:
      dserror("Unspecified typ of system matrix");
   break;

   default:
      dserror("Unspecified typ of system matrix");
   break;
   }
}
/*-------------- option==1 is exchange of coupled dofs among processors */
/*                    (which, of course, only occures in parallel case) */
#ifdef PARALLEL 
/*----------------------------------------exchange of 2 system matrices */
if (assemble_action==assemble_two_exchange)
{
/*------------------------------------------------ switch typ of matrix */
      switch(sysa1_typ)
      {

#ifdef AZTEC_PACKAGE
      case msr:
         exchange_coup_msr(actpart,actsolv,actintra,sysa1->msr);
         exchange_coup_msr(actpart,actsolv,actintra,sysa2->msr);
      break;
#endif

#ifdef HYPRE_PACKAGE
      case parcsr:
         dserror("Simultanous assembly of 2 system matrices not yet impl.");
      break;
#endif

#ifdef PARALLEL
      case ucchb:
         dserror("Simultanous assembly of 2 system matrices not yet impl.");
      break;
#endif

      case dense:
         redundant_dense(actpart,actsolv,actintra,sysa1->dense,sysa2->dense);
      break;

#ifdef MUMPS_PACKAGE
      case rc_ptr:
         exchange_coup_rc_ptr(actpart,actsolv,actintra,sysa1->rc_ptr);
         exchange_coup_rc_ptr(actpart,actsolv,actintra,sysa2->rc_ptr);
      break;
#endif

#ifdef SPOOLES_PACKAGE
      case spoolmatrix:
         exchange_coup_spo(actpart,actsolv,actintra,sysa1->spo);
         exchange_coup_spo(actpart,actsolv,actintra,sysa2->spo);
      break;
#endif

#ifdef UMFPACK
      case ccf:
         redundant_ccf(actpart,actsolv,actintra,sysa1->ccf,sysa2->ccf);
      break;
#endif

      case skymatrix:
         redundant_skyline(actpart,actsolv,actintra,sysa1->sky,sysa2->sky);
      break;

#ifdef MLPCG
      case bdcsr:;
      break;
#endif

      case oll:
         exchange_coup_oll(actpart,actintra,sysa1->oll);
         exchange_coup_oll(actpart,actintra,sysa2->oll);
      break;

      case sparse_none:
         dserror("Unspecified type of system matrix");
      break;

      default:
         dserror("Unspecified type of system matrix");
      break;
      }
}
/*-------------------------------------------exchange of 1 system matrix */
if (assemble_action==assemble_one_exchange)
{
      switch(sysa1_typ)
      {

#ifdef AZTEC_PACKAGE
      case msr:
         exchange_coup_msr(actpart,actsolv,actintra,sysa1->msr);
      break;
#endif

#ifdef HYPRE_PACKAGE
      case parcsr:
         exchange_coup_parcsr(actpart,actsolv,actintra,sysa1->parcsr);
      break;
#endif

#ifdef PARALLEL
      case ucchb:
         redundant_ucchb(actpart,actsolv,actintra,sysa1->ucchb);
      break;
#endif

      case dense:
         redundant_dense(actpart,actsolv,actintra,sysa1->dense,NULL);
      break;

      case skymatrix:
         redundant_skyline(actpart,actsolv,actintra,sysa1->sky,NULL);
      break;

#ifdef MUMPS_PACKAGE
      case rc_ptr:
         exchange_coup_rc_ptr(actpart,actsolv,actintra,sysa1->rc_ptr);
      break;
#endif

#ifdef SPOOLES_PACKAGE
      case spoolmatrix:
         exchange_coup_spo(actpart,actsolv,actintra,sysa1->spo);
      break;
#endif

#ifdef UMFPACK
      case ccf:
         redundant_ccf(actpart,actsolv,actintra,sysa1->ccf,NULL);
      break;
#endif

#ifdef MLPCG
      case bdcsr:;
      break;
#endif

      case oll:
         exchange_coup_oll(actpart,actintra,sysa1->oll);
      break;

      case sparse_none:
         dserror("Unspecified type of system matrix");
      break;

      default:
         dserror("Unspecified type of system matrix");
      break;
      }
}
#endif
/*------------------------------------- close the dynamic system matrix */
/*----------------------------------------------------------------------*/
end:
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of assemble */




/*----------------------------------------------------------------------*
 |  routine to                                           m.gee 9/01     |
 |  allocate the send and recv buffers for                              |
 |  coupling conditions                                                 |
 |  and to perform other inits which may become necessary for assembly  |
 | #################################################################### |
 |  to the paremeter list of this function I added                      |
 |  INT actndis!!!  - number of the actual discretisation               |
 |  this has to be done for all other calls of init_assembly            |
 |                                                           genk 08/02 |
 | #################################################################### |
 *----------------------------------------------------------------------*/
void init_assembly(
                       struct _PARTITION      *actpart,
                       struct _SOLVAR         *actsolv,
                       struct _INTRA          *actintra,
                       struct _FIELD          *actfield,
                       INT                     actsysarray,
		       INT                     actndis
                     )
{

#ifdef PARALLEL 
INT         i,j,k,counter;
INT         numeq;
INT         numsend;
INT         numrecv;
INT         minusone=-1;
INT         imyrank;
INT         inprocs;
SPARSE_TYP  sysarraytyp;
ARRAY      *coupledofs;
ELEMENT    *actele;

INT        *numcoupsend;
INT        *numcouprecv;
ARRAY     **couple_d_send_ptr;
ARRAY     **couple_i_send_ptr;
ARRAY     **couple_d_recv_ptr;
ARRAY     **couple_i_recv_ptr;
ARRAY      *dummyarray;
#endif

#ifdef DEBUG 
dstrc_enter("init_assembly");
#endif
/*----------------------------------------------------------------------*/
#ifdef PARALLEL 
/*----------------------------------------------------------------------*/
imyrank = actintra->intra_rank;
inprocs = actintra->intra_nprocs;
/*----------------------------------------------- check typ of sysarray */
sysarraytyp = actsolv->sysarray_typ[actsysarray];
switch(sysarraytyp)
{

#ifdef AZTEC_PACKAGE
case msr:
   numcoupsend       = &(actsolv->sysarray[actsysarray].msr->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].msr->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].msr->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].msr->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].msr->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].msr->couple_i_recv);
break;
#endif

#ifdef HYPRE_PACKAGE
case parcsr:
   numcoupsend       = &(actsolv->sysarray[actsysarray].parcsr->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].parcsr->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].parcsr->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].parcsr->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].parcsr->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].parcsr->couple_i_recv);
break;
#endif

#ifdef PARALLEL
case ucchb:
   numcoupsend       = &(actsolv->sysarray[actsysarray].ucchb->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].ucchb->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].ucchb->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].ucchb->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].ucchb->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].ucchb->couple_i_recv);
break;
#endif

case dense:
   numcoupsend       = &(actsolv->sysarray[actsysarray].dense->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].dense->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].dense->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].dense->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].dense->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].dense->couple_i_recv);
break;

#ifdef MUMPS_PACKAGE
case rc_ptr:
   numcoupsend       = &(actsolv->sysarray[actsysarray].rc_ptr->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].rc_ptr->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].rc_ptr->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].rc_ptr->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].rc_ptr->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].rc_ptr->couple_i_recv);
break;
#endif

#ifdef UMFPACK
case ccf:
   numcoupsend       = &(actsolv->sysarray[actsysarray].ccf->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].ccf->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].ccf->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].ccf->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].ccf->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].ccf->couple_i_recv);
break;
#endif

case skymatrix:
   numcoupsend       = &(actsolv->sysarray[actsysarray].sky->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].sky->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].sky->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].sky->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].sky->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].sky->couple_i_recv);
break;

#ifdef SPOOLES_PACKAGE
case spoolmatrix:
   numcoupsend       = &(actsolv->sysarray[actsysarray].spo->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].spo->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].spo->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].spo->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].spo->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].spo->couple_i_recv);
break;
#endif

case oll:
   numcoupsend       = &(actsolv->sysarray[actsysarray].oll->numcoupsend);
   numcouprecv       = &(actsolv->sysarray[actsysarray].oll->numcouprecv);
   couple_d_send_ptr = &(actsolv->sysarray[actsysarray].oll->couple_d_send);
   couple_i_send_ptr = &(actsolv->sysarray[actsysarray].oll->couple_i_send);
   couple_d_recv_ptr = &(actsolv->sysarray[actsysarray].oll->couple_d_recv);
   couple_i_recv_ptr = &(actsolv->sysarray[actsysarray].oll->couple_i_recv);
break;

#ifdef MLPCG
case bdcsr:
   goto end; /* coupled dofs are not supported in bdcsr */
#endif

default:
   dserror("Unknown typ of sparse array");
break;
}
/*---------------- now check for coupling dofs and interdomain coupling */
coupledofs = &(actpart->pdis[actndis].coupledofs);
numsend = 0;
numrecv = 0;
numeq   = actfield->dis[actndis].numeq;
/* 
   An inter-proc coupled equation produces communications calculating the 
   sparsity mask of the matrix
   An inter-proc coupled equation produces communications adding element
   matrices to the system matrix
   An inter-proc coupled equation ruins the bandwith locally
   ->
   Now one processor has to be owner of the coupled equation. 
   Try to distribute the coupled equations equally over the processors

   The matrix has the following style (after allreduce on all procs the same):
   
               ----------------------
               | 12 | 2 | 0 | 1 | 0 |
               | 40 | 2 | 0 | 0 | 0 |
               | 41 | 1 | 2 | 1 | 1 |
               | 76 | 0 | 1 | 2 | 0 |
               ----------------------
               
               column 0                : number of the coupled equation
               column 1 - inprocs+1 : proc has coupled equation or not
                                         2 indicates owner of equation
*/
/* calculate the number of sends and receives to expect during assemblage */
for (i=0; i<coupledofs->fdim; i++)
{
   /*--------------------------- check whether I am master owner of dof */
   if (coupledofs->a.ia[i][imyrank+1]==2)
   {
      /*-------------------------- check whether other procs are slaves */
      for (j=1; j<coupledofs->sdim; j++)
      {
         if (coupledofs->a.ia[i][j]==1) numrecv++;
      }
   }
   /*---------------------------- check whether I am slave owner of dof */
   if (coupledofs->a.ia[i][imyrank+1]==1) numsend++;
}
*numcoupsend=numsend;
*numcouprecv=numrecv;
/*-------------------------- allocate the necessary send and recv buffs */
/* note:
   Es waere sinnvoll, die sends und recv in einem Matrizenkompressionsformat
   durchzufuehren. Damit waere aber die Art dieses send + recv von dem
   verwendeten Loeser abhaengig, und man muesste das fuer jeden Loeser
   gesondert schreiben. Weil es fuer alle Loeser funktioniert wird hier als
   send und recv buffer eine komplette Zeile der Systemmatrix pro coupled dof 
   verwendet 
*/
if (numsend) /*-------- I have to send couple dof entries to other proc */
{
   *couple_d_send_ptr = (ARRAY*)CCACALLOC(1,sizeof(ARRAY));
   *couple_i_send_ptr = (ARRAY*)CCACALLOC(1,sizeof(ARRAY));

   if (!(*couple_d_send_ptr) || !(*couple_i_send_ptr))
   dserror("Allocation of send/recv buffers for coupled dofs failed");

   amdef("c_d_send",(*couple_d_send_ptr),numsend,numeq,"DA");
   amdef("c_i_send",(*couple_i_send_ptr),numsend,2,"IA");

   aminit(*couple_i_send_ptr,&minusone);

   /*----------------------- put the dof number to couple_i_send[0] */
   counter=0;
   for (i=0; i<coupledofs->fdim; i++)
   {
       if (coupledofs->a.ia[i][imyrank+1]==1)
       {
          (*couple_i_send_ptr)->a.ia[counter][0] = coupledofs->a.ia[i][0];
          counter++;
       }
   }
} 
else /*----------------------------------------- I have nothing to send */
{
   *couple_d_send_ptr = NULL;
   *couple_i_send_ptr = NULL;
}


if (numrecv) /* I am master of a coupled dof and expect entries from other procs */
{
   *couple_d_recv_ptr = (ARRAY*)CCACALLOC(1,sizeof(ARRAY));
   *couple_i_recv_ptr = (ARRAY*)CCACALLOC(1,sizeof(ARRAY));

   if (!(*couple_d_recv_ptr) || !(*couple_i_recv_ptr))
   dserror("Allocation of send/recv buffers for coupled dofs failed");

   amdef("c_d_recv",(*couple_d_recv_ptr),numrecv,numeq,"DA");
   amdef("c_i_recv",(*couple_i_recv_ptr),numrecv,2,"IA");
}
else /*----------------------- I do not expect entries from other procs */
{
   *couple_d_recv_ptr = NULL;
   *couple_i_recv_ptr = NULL;
}
/*----------------------------------------------------------------------*/
end:
#endif /* end of PARALLEL */
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of init_assembly */





/*----------------------------------------------------------------------*
 |  routine to assemble a global vector to a dist. vector   m.gee 10/01 |
 |  irhs & drhs are vectors of global lenght, rhs is a DIST_VECTOR      |
 |  and is filled in a style, which is very much dependent on the       |
 |  typ of system matrix it belongs to                                  |
 *----------------------------------------------------------------------*/
void assemble_vec(INTRA        *actintra,
                  SPARSE_TYP   *sysarraytyp,
                  SPARSE_ARRAY *sysarray,
                  DIST_VECTOR  *rhs,
                  DOUBLE       *drhs,
                  DOUBLE        factor)
{
INT                   i;
INT                   dof;
INT                   imyrank;

#ifdef MLIB_PACKAGE
ML_ARRAY_MDS         *mds_array;
#endif

#ifdef AZTEC_PACKAGE
AZ_ARRAY_MSR         *msr_array;
#endif

#ifdef HYPRE_PACKAGE
H_PARCSR             *parcsr_array;
#endif

#ifdef PARSUPERLU_PACKAGE
UCCHB                *ucchb_array;
#endif

DENSE                *dense_array;

#ifdef MUMPS_PACKAGE
RC_PTR               *rcptr_array;
#endif

#ifdef UMFPACK
CCF                  *ccf_array;
#endif

SKYMATRIX            *sky_array;

#ifdef SPOOLES_PACKAGE
SPOOLMAT             *spo;
#endif

#ifdef MLPCG
DBCSR                *bdcsr_array;
#endif

OLL                  *oll_array;
#ifdef DEBUG 
dstrc_enter("assemble_vec");
#endif
/*----------------------------------------------------------------------*/
imyrank = actintra->intra_rank;
/*----------------------------------------------------------------------*/
switch(*sysarraytyp)
{

#ifdef MLIB_PACKAGE
case mds:
    mds_array = sysarray->mds;
    for (i=0; i<rhs->numeq; i++)
    {
       rhs->vec.a.dv[i] += drhs[i]*factor;
    }
break;
#endif

#ifdef AZTEC_PACKAGE
case msr:
    msr_array = sysarray->msr;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = msr_array->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;
#endif

#ifdef HYPRE_PACKAGE
case parcsr:
    parcsr_array = sysarray->parcsr;
    for (i=0; i<rhs->numeq; i++)
    {
       dof     = parcsr_array->update.a.ia[imyrank][i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;
#endif

#ifdef PARSUPERLU_PACKAGE
case ucchb:
    ucchb_array = sysarray->ucchb;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = ucchb_array->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;
#endif

case dense:
    dense_array = sysarray->dense;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = dense_array->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;

case skymatrix:
    sky_array = sysarray->sky;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = sky_array->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;

#ifdef MUMPS_PACKAGE
case rc_ptr:
    rcptr_array = sysarray->rc_ptr;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = rcptr_array->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;
#endif

#ifdef UMFPACK
case ccf:
    ccf_array = sysarray->ccf;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = ccf_array->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;
#endif

#ifdef SPOOLES_PACKAGE
case spoolmatrix:
    spo = sysarray->spo;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = spo->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;
#endif

#ifdef MLPCG
case bdcsr:
    bdcsr_array = sysarray->bdcsr;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = bdcsr_array->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;
#endif

case oll:
    oll_array = sysarray->oll;
    for (i=0; i<rhs->numeq; i++)
    {
       dof = oll_array->update.a.iv[i];
       rhs->vec.a.dv[i] += drhs[dof]*factor;
    }
break;
 
default:
   dserror("Unknown typ of system matrix");
break;
}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of assemble_vec */


/*----------------------------------------------------------------------*
 |  routine to sum a global vector                          m.gee 6/02 |
 *----------------------------------------------------------------------*/
void sum_vec(INTRA        *actintra,
             SPARSE_TYP   *sysarraytyp,
             SPARSE_ARRAY *sysarray,
             DOUBLE       *drhs,
             INT           numeq,
             DOUBLE       *sum)
{
INT                   i;
INT                   dof;
INT                   imyrank;
#ifdef PARALLEL
DOUBLE                recv;
#endif

#ifdef MLIB_PACKAGE
ML_ARRAY_MDS         *mds_array;

#endif

#ifdef AZTEC_PACKAGE
AZ_ARRAY_MSR         *msr_array;
#endif

#ifdef HYPRE_PACKAGE
H_PARCSR             *parcsr_array;
#endif

#ifdef PARSUPERLU_PACKAGE
UCCHB                *ucchb_array;
#endif

DENSE                *dense_array;

#ifdef MUMPS_PACKAGE
RC_PTR               *rcptr_array;
#endif

#ifdef UMFPACK
CCF                  *ccf_array;
#endif

SKYMATRIX            *sky_array;

#ifdef SPOOLES_PACKAGE
SPOOLMAT             *spo;
#endif

OLL                  *oll_array;
#ifdef DEBUG 
dstrc_enter("sum_vec");
#endif
/*----------------------------------------------------------------------*/
imyrank = actintra->intra_rank;
/*----------------------------------------------------------------------*/
*sum=0.0;
/*----------------------------------------------------------------------*/
switch(*sysarraytyp)
{

#ifdef MLIB_PACKAGE
case mds:
    mds_array = sysarray->mds;
    for (i=0; i<numeq; i++)
    {
       *sum += drhs[i];
    }
break;
#endif

#ifdef AZTEC_PACKAGE
case msr:
    msr_array = sysarray->msr;
    for (i=0; i<numeq; i++)
    {
       dof = msr_array->update.a.iv[i];
       *sum += drhs[dof];
    }
break;
#endif

#ifdef HYPRE_PACKAGE
case parcsr:
    parcsr_array = sysarray->parcsr;
    for (i=0; i<numeq; i++)
    {
       dof     = parcsr_array->update.a.ia[imyrank][i];
       *sum += drhs[dof];
    }
break;
#endif

#ifdef PARSUPERLU_PACKAGE
case ucchb:
    ucchb_array = sysarray->ucchb;
    for (i=0; i<numeq; i++)
    {
       dof = ucchb_array->update.a.iv[i];
       *sum += drhs[dof];
    }
break;
#endif

case dense:
    dense_array = sysarray->dense;
    for (i=0; i<numeq; i++)
    {
       dof = dense_array->update.a.iv[i];
       *sum += drhs[dof];
    }
break;

case skymatrix:
    sky_array = sysarray->sky;
    for (i=0; i<numeq; i++)
    {
       dof = sky_array->update.a.iv[i];
       *sum += drhs[dof];
    }
break;

#ifdef MUMPS_PACKAGE
case rc_ptr:
    rcptr_array = sysarray->rc_ptr;
    for (i=0; i<numeq; i++)
    {
       dof = rcptr_array->update.a.iv[i];
       *sum += drhs[dof];
    }
break;
#endif

#ifdef UMFPACK
case ccf:
    ccf_array = sysarray->ccf;
    for (i=0; i<numeq; i++)
    {
       dof = ccf_array->update.a.iv[i];
       *sum += drhs[dof];
    }
break;
#endif

#ifdef SPOOLES_PACKAGE
case spoolmatrix:
    spo = sysarray->spo;
    for (i=0; i<numeq; i++)
    {
       dof = spo->update.a.iv[i];
       *sum += drhs[dof];
    }
break;
#endif

case oll:
    oll_array = sysarray->oll;
    for (i=0; i<numeq; i++)
    {
       dof = oll_array->update.a.iv[i];
       *sum += drhs[dof];
    }
break;

default:
   dserror("Unknown typ of system matrix");
break;
}
/*----------------------------------------------------------------------*/
#ifdef PARALLEL 
MPI_Allreduce(sum,&recv,1,MPI_DOUBLE,MPI_SUM,actintra->MPI_INTRA_COMM);
*sum = recv;
#endif
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of sum_vec */




/*----------------------------------------------------------------------*
 |  assembles an element vector to a redundant global vector m.gee 3/02 |
 *----------------------------------------------------------------------*/
void assemble_intforce(ELEMENT *actele,ARRAY *elevec_a,CONTAINER *container,
                       INTRA *actintra)
{
INT                   i,j;
INT                   dof;
INT                   numdf;
#ifdef PARALLEL 
INT                   imyrank;
#endif
INT                   irow;
DOUBLE               *elevec;
#ifdef DEBUG 
dstrc_enter("assemble_intforce");
#endif
/*----------------------------------------------------------------------*/
#ifdef PARALLEL 
imyrank = actintra->intra_rank;
#endif
/*----------------------------------------------------------------------*/
elevec = elevec_a->a.dv;
irow=-1;
/*----------------------------------------------------------------------*/
for (i=0; i<actele->numnp; i++)
{
   numdf = actele->node[i]->numdf;
#ifdef PARALLEL 
   if(actele->node[i]->proc!= imyrank)
   {
     irow+=numdf;
     continue;
   }
#endif
   for (j=0; j<numdf; j++)
   {
      irow++;
      dof = actele->node[i]->dof[j];
      if (dof >= container->global_numeq) continue;
       container->dvec[dof] += elevec[irow];
/*      container->dvec[dof] += elevec[i*numdf+j]; */
   }
}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of assemble_intforce */


/*---------------------------------------------------------------------------*
 |  dirichlet conditions to an element vector elevec_a       m.gee 3/02      |
 |  and then assembles this element vector of cond. dirich.conditions to the |
 |  global vector fullvec                                                    |
 *---------------------------------------------------------------------------*/
void assemble_dirich(ELEMENT *actele, ARRAY *estif_global, CONTAINER *container)
{
INT                   i,j;
INT                   numdf;
INT                   nd=0;
DOUBLE              **estif;
DOUBLE                dirich[MAXDOFPERELE];
DOUBLE                dforces[MAXDOFPERELE];
INT                   dirich_onoff[MAXDOFPERELE];
INT                   lm[MAXDOFPERELE];
GNODE                *actgnode;
#ifdef DEBUG 
dstrc_enter("assemble_dirich");
#endif
/*----------------------------------------------------------------------*/
estif  = estif_global->a.da;
/*---------------------------------- set number of dofs on this element */
for (i=0; i<actele->numnp; i++) nd += actele->node[i]->numdf;
/*---------------------------- init the vectors dirich and dirich_onoff */
for (i=0; i<nd; i++)
{
   dirich[i] = 0.0;
   dforces[i] = 0.0;
   dirich_onoff[i] = 0;
}
/*-------------------------------- fill vectors dirich and dirich_onoff */
for (i=0; i<actele->numnp; i++)
{
   numdf    = actele->node[i]->numdf;
   actgnode = actele->node[i]->gnode;
   for (j=0; j<numdf; j++)
   {
      lm[i*numdf+j] = actele->node[i]->dof[j];
      if (actgnode->dirich==NULL) continue;
      dirich_onoff[i*numdf+j] = actgnode->dirich->dirich_onoff.a.iv[j];
      dirich[i*numdf+j] = actgnode->dirich->dirich_val.a.dv[j];
   }
}
/*----------------------------------------- loop rows of element matrix */
for (i=0; i<nd; i++)
{
   /*------------------------------------- do nothing for supported row */
   if (dirich_onoff[i]!=0) continue;
   /*---------------------------------- loop columns of unsupported row */
   for (j=0; j<nd; j++)
   {
      /*---------------------------- do nothing for unsupported columns */
      if (dirich_onoff[j]==0) continue;
      dforces[i] += estif[i][j] * dirich[j];
   }/* loop j over columns */
}/* loop i over rows */
/*-------- now assemble the vector dforces to the global vector fullvec */
for (i=0; i<nd; i++)
{
   if (lm[i] >= container->global_numeq) continue;
   container->dirich[lm[i]] += dforces[i];
}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of assemble_dirich */

