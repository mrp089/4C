/*!----------------------------------------------------------------------
\file
\brief contains the routine 'opt_g_out',
       controlling output of optimization

*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
#ifdef D_OPTIM                   /* include optimization code to ccarat */
/*----------------------------------------------------------------------*/

#include "../headers/standardtypes.h"
#include "../headers/solution_mlpcg.h"
#include "../headers/solution.h"
#include "../headers/optimization.h"
#include "../output/gid.h"
#include "../wall1/wall1.h"
#include "../brick1/brick1.h"
#include "opt_prototypes.h"
/*! 
\addtogroup OPTIMIZATION 
*//*! @{ (documentation module open)*/




/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | global variable *partition, vector of lenght numfld of structures    |
 | PARTITION is defined in global_control.c                             |
 *----------------------------------------------------------------------*/
extern struct _PARTITION  *partition;
/*!----------------------------------------------------------------------
\brief ranks and communicators

<pre>                                                         m.gee 8/00
This structure struct _PAR par; is defined in main_ccarat.c
and the type is in partition.h                                                  
</pre>

*----------------------------------------------------------------------*/
 extern struct _PAR   par;                      
/*!----------------------------------------------------------------------
\brief file pointers

<pre>                                                         m.gee 8/00
This structure struct _FILES allfiles is defined in input_control_global.c
and the type is in standardtypes.h                                                  
It holds all file pointers and some variables needed for the FRSYSTEM
</pre>
*----------------------------------------------------------------------*/
extern struct _FILES  allfiles;
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | struct _GENPROB       genprob; defined in global_control.c           |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;
/*----------------------------------------------------------------------*
 | global variable *solv, vector of lenght numfld of structures SOLVAR  |
 | defined in solver_control.c                                          |
 |                                                                      |
 |                                                       m.gee 11/00    |
 *----------------------------------------------------------------------*/
extern struct _SOLVAR  *solv;
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | vector of numfld FIELDs, defined in global_control.c                 |
 *----------------------------------------------------------------------*/
extern struct _FIELD      *field;
/*!----------------------------------------------------------------------
\brief the optimization main structure
<pre>                                                            al 06/01   
defined in opt_cal_main.c
</pre>
*----------------------------------------------------------------------*/
 struct _OPTI *opt;

/*----------------------------------------------------------------------*
 | control output of optimization data                      al 05/01    |
 *----------------------------------------------------------------------*/
void opt_g_out(OPT_GR_OUT gract)
{
/*----------------------------------------------------------------------*/
  static INT nummeshw;       /* local Id of mesh, which will be written */
  static INT numdataw;       /* local Id of data, which will be written */
  static INT numdispw;       /* local Id of data, which will be written */
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("opt_g_out");
#endif
/*----------------------------------------------------------------------*/
  switch(gract)
  {
  case gr_init:
     nummeshw = 0;
     numdataw = 0;
     numdispw = 0;
  break;
  case gr_mesh: /* write mesh with current loads and boundaries */
     nummeshw++;
     og_write_mesh(nummeshw);
  break;
  case gr_dens: /* write element densities in case of topoopt.  */
     numdataw++;
     og_write_eledens(numdataw);
  break;
  case gr_disp: /* write displacements */
     numdispw++;
     og_write_displacements(numdispw);
  break;
  }
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of opt_g_out */

/*----------------------------------------------------------------------*
 | output of fe-mesh, loads, dirichlet conditions...        al 05/01    |
 *----------------------------------------------------------------------*/
void og_write_mesh(INT nmesh)
{
/*----------------------------------------------------------------------*/
INT j, k, l;
INT dof, bnodeflag, lnodeflag;
DOUBLE lval;
/*----------------------------------------------------------------------*/
static FILE         *out;
static FILE         *fp_tmp;
/*----------------------------------------------------------------------*/
char         *line1="_";
char         *line2="=";
/*----------------------------------------------------------------------*/
 static ARRAY   rhspv_a;
 static DOUBLE *rhspv;
/*----------------------------------------------------------------------*/
  SOLVAR       *actsolv;          /* pointer to the fields SOLVAR structure */
  PARTITION    *actpart;          /* pointer to the fields PARTITION structure */
  INTRA        *actintra;         /* pointer to the fields intra-communicator structure */
  FIELD        *actfield;         /* pointer to the structural FIELD  */
  ELEMENT      *actele;           /* active element                   */
  NODE         *actnode;          /* active node                      */
/*----------------------------------------------------------------------*/
actfield    = &(field[0]);
actsolv     = &(solv[0]);
actpart     = &(partition[0]);
#ifdef PARALLEL 
actintra    = &(par.intra[0]);
#else
actintra    = (INTRA*)CCACALLOC(1,sizeof(INTRA));
if (!actintra) dserror("Allocation of INTRA failed");
actintra->intra_fieldtyp = structure;
actintra->intra_rank   = 0;
actintra->intra_nprocs   = 1;
#endif
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("og_write_mesh");
#endif
/*----------------------------------------------------------------------*/
 if (par.myrank==0)
 {/* proc 0 */
/*------------------------------------------------- write template file */
  fp_tmp = fopen("zgout/cgs.tmesh","w");
/*----------------------------------------------------------------------*/
  if(fp_tmp==NULL) goto endopen0;
/*----------------------------------------------------------------------*/
  fprintf(fp_tmp," %d    meshes     \n",nmesh);
/*----------------------------------------------------------------------*/
  fclose(fp_tmp);
/*----------------------------------------------------------------------*/
  endopen0:;
/*----------------------------------------------------------------------*/
 }/* proc 0 */
    
    
 if (par.myrank==0)
 {/* proc 0 */
/*----------------------------------------------------------------------*/
  out = fopen("zgout/cgs.mesh", "w");
/*----------------------------------------------------------------------*/
  if(out==NULL) goto endopen01;
/*-------------------------------------------------------- print header */
  fprintf(out,"title: %s\n",allfiles.title[0]);
/*---------------------------------------------------- number of fields */
  fprintf(out,"number_of_fields %-6d\n",1);
/*---------------------------------------------------- loop over fields */
  for (l=0; l<200; l++)fprintf(out,"%s",line2);fprintf(out,"\n");

   /*----------------------------------- print the meshes of this field */
    actfield = &(field[0]);
   /*-------------------------------------------------- number of nodes */
    fprintf(out,"number_of_nodes: %-6d\n",actfield->dis[0].numnp );
   /*----------------------------------------------- number of elements */
    fprintf(out,"number_of_elems: %-6d\n",actfield->dis[0].numele);
   /*------------------------------------------------------------ nodes */
    for (l=0; l<200; l++)fprintf(out,"%s",line1);fprintf(out,"\n\n");
    for (j=0; j<actfield->dis[0].numnp; j++)
    {
      actnode = &(actfield->dis[0].node[j]);
      fprintf(out,"%-6d %-18.5#f %-18.5#f %-18.5#f\n",
                                                     actnode->Id+1,
                                                     actnode->x[0],
                                                     actnode->x[1],
                                                     actnode->x[2]);
    }
   /*--------------------------------------------------------- elements */
    for (l=0; l<200; l++)fprintf(out,"%s",line1);fprintf(out,"\n\n");
    for (j=0; j<actfield->dis[0].numele; j++)
    {
      actele = &(actfield->dis[0].element[j]);
      fprintf(out,"%-6d %-6d ",actele->Id+1,actele->Id_loc+1);
      
      switch(actele->eltyp)
      {
      case el_shell8:
        fprintf(out,"SHELL8 ");
        fprintf(out,"%2d  ",actele->numnp);
        for (k=0; k<actele->numnp; k++) fprintf(out,"%-6d ",actele->node[k]->Id_loc+1);
        fprintf(out,"\n");
      break;
      case el_brick1:
        fprintf(out,"BRICK1 ");
        fprintf(out,"%2d  ",actele->numnp);
        for (k=0; k<8; k++) fprintf(out,"%-6d ",actele->node[k]->Id_loc+1);
        if(actele->numnp==20)
        {
          for (k=8; k<12; k++) fprintf(out,"%-6d ",actele->node[k]->Id_loc+1);
          for (k=16; k<20; k++) fprintf(out,"%-6d ",actele->node[k]->Id_loc+1);
          for (k=12; k<16; k++) fprintf(out,"%-6d ",actele->node[k]->Id_loc+1);
        }
        fprintf(out,"\n");
      break;
      case el_wall1:
        fprintf(out,"WALL1 ");
        fprintf(out,"%2d  ",actele->numnp);
        for (k=0; k<actele->numnp; k++) fprintf(out,"%-6d ",actele->node[k]->Id_loc+1);
        fprintf(out,"\n");
      break;
      case el_shell1:
        fprintf(out,"WALL1 ");
        fprintf(out,"%2d  ",actele->numnp);
        for (k=0; k<actele->numnp; k++) fprintf(out,"%-6d ",actele->node[k]->Id_loc+1);
        fprintf(out,"\n");
      break;
      default:
        dserror("Cannot print elementtype");
      break;
      }

    }
endopen01:;
 }/* proc 0 */ 
   /*------------------------------------------------------------ loads */
   /*------------------------- allreduce rhs */

    #ifdef PARALLEL 
    rhspv = amdef("rhspv",&rhspv_a,solv->rhs[0].numeq_total,1,"DV");
    /*       amdel(&rhspv_a);*/
   
    
    solserv_reddistvec(&solv->rhs[0],
                      &(solv->sysarray[0]),&(solv->sysarray_typ[0]),
                      rhspv, solv->rhs[0].numeq_total,actintra);
    #endif
    /* */
 if (par.myrank==0)
 {/* proc 0 */
/*----------------------------------------------------------------------*/
  if(out==NULL) goto endopen02;
    for (l=0; l<200; l++)fprintf(out,"%s",line1);fprintf(out,"\n\n");
     
    
    for (j=0; j<actfield->dis[0].numnp; j++)
    {
      actnode = &(actfield->dis[0].node[j]);
      lnodeflag=0;
      for (k=0; k<actnode->numdf; k++)
      {
        dof = actnode->dof[k];
        /* neumann condition on dof */
    #ifdef PARALLEL 
        if (dof >= actfield->dis[0].numeq) continue;
        if(fabs(rhspv[dof])>0.00000001) lnodeflag=1;
    #else
        if (dof >= solv->rhs[0].numeq) continue;
        if(fabs(solv->rhs[0].vec.a.dv[dof])>0.00000001) lnodeflag=1;
    #endif
     }
      
      if(!lnodeflag) continue;
     
      fprintf(out,"%-6d %-6d ", actnode->Id+1, actnode->numdf);
      
      for (k=0; k<actnode->numdf; k++)
      {
          dof = actnode->dof[k];
    #ifdef PARALLEL 
          if (dof >= actfield->dis[0].numeq) continue;
          else lval = rhspv[dof];
    #else
          if (dof >= solv->rhs[0].numeq) continue;
          else lval = solv->rhs[0].vec.a.dv[dof];
    #endif
          fprintf(out," %-18.5#f",lval);
      } 
      fprintf(out,"\n");
      
    }
   /*--------------------------------------------------------- boundary */
    for (l=0; l<200; l++)fprintf(out,"%s",line1);fprintf(out,"\n\n");
    for (j=0; j<actfield->dis[0].numnp; j++)
    {
      actnode = &(actfield->dis[0].node[j]);
      
      bnodeflag=0;
      for (k=0; k<actnode->numdf; k++)
      {
        dof = actnode->dof[k];
        /* dirichlet condition on dof */
        if (dof >= actfield->dis[0].numeq) /* boundary condition ... */ 
        {
           bnodeflag=1;
        }
      }

      if(!bnodeflag) continue;

      fprintf(out,"%-6d %-6d ", actnode->Id+1, actnode->numdf);
      for (k=0; k<actnode->numdf; k++)
      {
        dof = actnode->dof[k];
        /* dirichlet condition on dof */
        if (dof >= actfield->dis[0].numeq)  fprintf(out," 1");
        else                                fprintf(out," 0");
       }
       fprintf(out,"\n");
    }
/*----------------------------------------------------------------------*/
for (l=0; l<200; l++)fprintf(out,"%s",line2);fprintf(out,"\n");
/*----------------------------------------------------------------------*/
    fprintf(out,"END_OF_CGSFILE\n");
    fclose(out);
/*----------------------------------------------------------------------*/
endopen02:;
/*----------------------------------------------------------------------*/
 }/* proc 0 */ 
/*----------------------------------------------------------------------*/
    #ifdef PARALLEL 
    amdel(&rhspv_a);
    #endif
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of og_write_mesh */
/*----------------------------------------------------------------------*
 | output of element density in case of topoopt             al 05/01    |
 *----------------------------------------------------------------------*/
void og_write_eledens(INT ndataofmesh)
{
/*----------------------------------------------------------------------*/
  INT i;
#ifdef PARALLEL 
  static  DOUBLE *svec;   /* vector with sensitivities on element level */
  static  DOUBLE *sveh;   /* necsessary for allreducing element values  */
#endif
/*----------------------------------------------------------------------*/
  PARTITION    *actpart;          /* pointer to the fields PARTITION structure */
  INTRA        *actintra;         /* pointer to the fields intra-communicator structure */
  FIELD        *actfield;         /* pointer to the structural FIELD  */
  ELEMENT      *actele;           /* active element                   */
/*----------------------------------------------------------------------*/
FILE    *fp_out;
FILE    *fp_tmp;
char    *appf="a";
char    *newf="w";
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("og_write_eledens");
#endif
/*----------------------------------------------------------------------*/
actfield    = &(field[0]);
actpart     = &(partition[0]);
#ifdef PARALLEL 
actintra    = &(par.intra[0]);
#else
actintra    = (INTRA*)CCACALLOC(1,sizeof(INTRA));
if (!actintra) dserror("Allocation of INTRA failed");
actintra->intra_fieldtyp = structure;
actintra->intra_rank   = 0;
actintra->intra_nprocs   = 1;
#endif
/*----------------------------------------------------------------------*/
#ifdef PARALLEL 
  svec  = (DOUBLE*)CCACALLOC(actfield->dis[0].numele,sizeof(DOUBLE));
  sveh  = (DOUBLE*)CCACALLOC(actfield->dis[0].numele,sizeof(DOUBLE));
  for (i=0; i<actfield->dis[0].numele; i++) svec[i]=0.;
  for (i=0; i<actfield->dis[0].numele; i++) sveh[i]=0.;
  
  for (i=0; i<actfield->dis[0].numele; i++)
  {
     actele = &(actfield->dis[0].element[i]);
    if(actele->proc!=actintra->intra_rank) continue;
    /*--*/
    if (actele->eltyp == el_wall1)
    {
      sveh[i]=actele->e.w1[0].elewa[0].matdata[0];
    }
    if (actele->eltyp == el_brick1)
    {
      sveh[i]=actele->e.c1[0].elewa[0].matdata[0];
    }
    /*--*/
  }
   /*--------------------------------- allreduce element density values */
    MPI_Allreduce(sveh,svec,actfield->dis[0].numele,MPI_DOUBLE,MPI_SUM,actintra->MPI_INTRA_COMM);
#endif
/*----------------------------------------------------------------------*/
 if (par.myrank==0)
 {/* proc 0 */
/*------------------------------------------------- write template file */
  fp_tmp = fopen("zgout/cgs.tval","w");
  if(fp_tmp==NULL) goto endopen1;
  /*--------------------------------------------------------------------*/
  fprintf(fp_tmp," %d    data sets \n",1);
  fprintf(fp_tmp," %d    time steps\n",ndataofmesh);
  fprintf(fp_tmp,"elementdensity\n");
  fprintf(fp_tmp," %d number of data (elements ...)\n",
                                                 actfield->dis[0].numele);
  fprintf(fp_tmp,"2            [1 val-n-e 2 v-e 3 v-n]\n");
   
  /*--------------------------------------------------------------------*/
  fclose(fp_tmp);
/*----------------------------------------------------------------------*/
  endopen1:;
/*----------------------------------------------------- print densities */
if(ndataofmesh==1) fp_out=fopen("zgout/cgs.vval","w");/* open and clear */
if(ndataofmesh >1) fp_out=fopen("zgout/cgs.vval","a");/* open and add   */
  if(fp_out==NULL) goto endopen2;
/*----------------------------------------------------------------------*/
  if(ndataofmesh==1)
  {
    fprintf(fp_out,
             "[time step] [obj] [data_set]  [numval] [numval x value]\n");
  } 

  for (i=0; i<actfield->dis[0].numele; i++)
  {
     actele = &(actfield->dis[0].element[i]);
#ifdef PARALLEL 
      fprintf(fp_out,"%d %d %d %d %18.5#E \n",ndataofmesh, 1,i+1,1,svec[i]);
#else
    /*--*/
    if (actele->eltyp == el_wall1)
    {
      fprintf(fp_out,"%d %d %d %d %18.5#E \n",
              ndataofmesh, 1,i+1,1,actele->e.w1[0].elewa[0].matdata[0]);
    }
    if (actele->eltyp == el_brick1)
    {
      fprintf(fp_out,"%d %d %d %d %18.5#E \n",
              ndataofmesh, 1,i+1,1,actele->e.c1[0].elewa[0].matdata[0]);
    }
    /*--*/
#endif
  }

/*----------------------------------------------------------------------*/
fclose(fp_out);
/*----------------------------------------------------------------------*/
endopen2:;
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
 }/* proc 0 */ 
#ifdef PARALLEL 
  CCAFREE(svec);
  CCAFREE(sveh);
#endif
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of og_write_eledens */
/*----------------------------------------------------------------------*/
void og_write_displacements(INT kstep)
{
/*----------------------------------------------------------------------*/
INT i;
NODE    *actnode;
FIELD   *actfield;
/*----------------------------------------------------------------------*/
FILE    *fp_out;
FILE    *fp_tmp;
char    *appf="a";
char    *newf="w";
char    *modf;
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_enter("og_write_displacements");
#endif
/*----------------------------------------------------------------------*/
 if (par.myrank==0)
 {/* proc 0 */
/*--------------------------------------- loop all fields and init data */
for (i=0; i<genprob.numfld; i++)
{
   actfield         = &(field[i]);
/*------------------------------------------------- write template file */
  fp_tmp = fopen("zgout/cgs.tdis","w");
  if(fp_tmp==NULL) goto endopen3;
/*----------------------------------------------------------------------*/
   fprintf(fp_tmp," %d    nodes     \n",actfield->dis[0].numnp);
   fprintf(fp_tmp," %d    time steps\n",kstep);
   fprintf(fp_tmp," %d    data sets \n",1);
   fprintf(fp_tmp," disp-stanln     \n",1);
/*----------------------------------------------------------------------*/
fclose(fp_tmp);
/*----------------------------------------------------------------------*/
endopen3:;
/*----------------------------------------------------------------------*/
/*------------------------------------------------- print displacements */
modf = appf;
if(kstep==1) modf = newf; 
fp_out = fopen("zgout/cgs.vdis",modf);
  if(fp_out==NULL) goto endopen4;
/*----------------------------------------------------------------------*/
   for (i=0; i<actfield->dis[0].numnp; i++)
   {
      actnode = &(actfield->dis[0].node[i]);
      
      if(actnode->sol.sdim==2)
      {
        fprintf(fp_out," %6d %18.5#E %18.5#E %18.5#E\n",
                                                   actnode->Id+1,
                                                   actnode->sol.a.da[0][0],
                                                   actnode->sol.a.da[0][1],
                                                   0.0
                                                   );
      }
      else
      {
        fprintf(fp_out," %6d %18.5#E %18.5#E %18.5#E\n",
                                                   actnode->Id+1,
                                                   actnode->sol.a.da[0][0],
                                                   actnode->sol.a.da[0][1],
                                                   actnode->sol.a.da[0][2]
                                                   );
      }
      
   }
/*----------------------------------------------------------------------*/
fclose(fp_out);
/*----------------------------------------------------------------------*/
endopen4:;
/*----------------------------------------------------------------------*/
}
/*----------------------------------------------------------------------*/
 }/* proc 0 */ 
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of og_write_displacements */
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
#endif /* stop including optimization code to ccarat :*/
/*----------------------------------------------------------------------*/

/*! @} (documentation module close)*/

