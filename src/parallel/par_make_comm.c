/*!---------------------------------------------------------------------
\file
\brief domain decomposition and metis routines

---------------------------------------------------------------------*/
#include "../headers/standardtypes.h"
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | vector of numfld FIELDs, defined in global_control.c                 |
 *----------------------------------------------------------------------*/
extern struct _FIELD      *field;
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | global variable GENPROB genprob is defined in global_control.c       |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;


/*! 
\addtogroup PARALLEL 
*/

/*! @{ (documentation module open)*/


/*!----------------------------------------------------------------------
\brief ranks and communicators

<pre>                                                         m.gee 8/00
This structure struct _PAR par; is defined in main_ccarat.c
and the type is in partition.h                                                  
</pre>

*----------------------------------------------------------------------*/
 extern struct _PAR   par;                      

/*!---------------------------------------------------------------------

\brief create field-specific intra-communicators                                              


<pre>                                                        m.gee 9/01 
at the moment there is a communicator created for each field.        
All of these n communicators are identical to MPI_COMM_WORLD,        
but the code now uses the field specific communicator for            
all calculations & communications inside a field.                    
For Inter-field communcations the communicator MPI_COMM_WORLD        
is used.                                                             
This opens the opportunity to have parallel execution of different   
fields in future.                                                    
Example:                                                             
While solving the structure in the                                   
structure's communicator with 2 procs, the other 14 procs are        
doing the fluid field in their own communicating space. None         
of both groups gets disturbed by the other one. They only get        
synchronized through MPI_COMM_WORLD at the moment of the             
fluid-structure-coupling.                                            

To create the intra-communicators the followin gsteps are done:
-MPI_WORLD_GROUP is extracted from MPI_COMM_WORLD
-numfield copies of this MPI_WORLD_GROUP are created named MPI_INTRA_GROUP
-communicators MPI_INTRA_COMM are created from these MPI_INTRA_GROUP
-MPI_INTRA_GROUPs are kept, because I'am not sure whether the 
 MPI_INTRA_COMM gets damaged when the corresponding GROUP is freed
</pre>

\return void                                               

------------------------------------------------------------------------*/
void create_communicators()
{
INT         i,j;
INT        *ranklist;
#ifdef PARALLEL 
MPI_Group   MPI_WORLD_GROUP;
#endif

#ifdef DEBUG 
dstrc_enter("create_communicators");
#endif
/*----------------------------------------------------------------------*/
#ifdef PARALLEL 
/*----------------------------------------------------------------------*/
ranklist = (INT*)CCAMALLOC(par.nprocs*sizeof(INT));
if (!ranklist) dserror("Allocation of memory failed");
/*----------------------------------------------- save number of fields */
par.numfld      = genprob.numfld;
/*-------------- allocate a structure INTRA for each field on each proc */
par.intra = (INTRA*)CCACALLOC(par.numfld,sizeof(INTRA));
if (!par.intra) dserror("Allocation of memory failed");
/*---------------- get the group definition belonging to MPI_COMM_WORLD */
   MPI_Comm_group(MPI_COMM_WORLD,&MPI_WORLD_GROUP);
/*------------------------------------------------- now loop the fields */
for (i=0; i<par.numfld; i++)
{
/*---------------------------------------------------------- initialize */
   par.intra[i].intra_fieldtyp = field[i].fieldtyp;
   par.intra[i].intra_rank     = 0;
   par.intra[i].intra_nprocs   = 0;
/*------  make a list of all processes, which are supposed to take part */
/* in the solution of the actual field. At the moment this is all procs */
/*--------- Remark: This is the place to build in other groups later on */
   for (j=0; j<par.nprocs; j++) ranklist[j]=j;
/*----- construct a subgroup of these procs, derive it from the default */   
/*                                                group MPI_WORLD_GROUP */
   MPI_Group_incl(
                  MPI_WORLD_GROUP,
                  par.nprocs,
                  ranklist,
                  &(par.intra[i].MPI_INTRA_GROUP)
                 );
   if (par.intra[i].MPI_INTRA_GROUP == MPI_GROUP_NULL)
   dserror("Creation of MPI_INTRA_GROUP failed");
/*------------------------now construct the communicator from this group */                 
/*     Remark: This call returns MPI_COMM_NULL to all procs not in group */
/*                  it is a collective call to be performed by ALL procs */
   MPI_Comm_create( 
                   MPI_COMM_WORLD,
                   par.intra[i].MPI_INTRA_GROUP,
                   &(par.intra[i].MPI_INTRA_COMM)
                  );
/*------------- at the moment, all procs shall be in all MPI_INTRA_COMMs */
   if (par.intra[i].MPI_INTRA_COMM==MPI_COMM_NULL) 
   dserror("Creation of intra communicator MPI_INTRA_COMM failed");
/*---------------- now get the new ranks of each proc from the intra_com */
   MPI_Comm_rank(par.intra[i].MPI_INTRA_COMM,&(par.intra[i].intra_rank));
/*------------------ now get the size of newly created intracommunicator */   
   MPI_Comm_size(par.intra[i].MPI_INTRA_COMM,&(par.intra[i].intra_nprocs));
   if (par.intra[i].intra_nprocs>MAXPROC) dserror("Define of MAXPROC too small");

   /* SPOOLES requires the MPI_TAG_UB (that contains the highest
      possible tag value) in every communicator. It's guaranteed to be
      defined in MPI_COMM_WORLD, but apparently mpich doesn't copy it
      when creating new communicators. So lets do it ourselves. */
   INT *iptr, flag, tag_bound;
   iptr = &tag_bound;
   MPI_Attr_get(par.intra[i].MPI_INTRA_COMM, MPI_TAG_UB, (void **) &iptr, &flag);
   if ( flag == 0 ) {
       /*dserror("No MPI_TAG_UB in new communicator.");*/
       MPI_Attr_get(MPI_COMM_WORLD, MPI_TAG_UB, (void **) &iptr, &flag);
       if ( flag == 0 ) {
           dserror("No MPI_TAG_UB in MPI_COMM_WORLD. Panic!");
       }
       MPI_Attr_put(par.intra[i].MPI_INTRA_COMM, MPI_TAG_UB, (void *) iptr);
   }
/*-----------------------------------------------------------------------*/   
}/* end of loop over fields */
/*--------------------------- free the MPI_WORLD_GROUP, no longer needed */
   MPI_Group_free(&MPI_WORLD_GROUP);



CCAFREE(ranklist);
/*----------------------------------------------------------------------*/
#endif /* end of PARALLEL */
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of create_communicators */

/*! @} (documentation module close)*/
