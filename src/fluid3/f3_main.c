/*!----------------------------------------------------------------------
\file
\brief main routine fluid3 element

------------------------------------------------------------------------*/
#include "../headers/standardtypes.h"
#include "fluid3_prototypes.h"
#include "fluid3.h"

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
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | vector of numfld FIELDs, defined in global_control.c                 |
 *----------------------------------------------------------------------*/
extern struct _FIELD      *field;
/*----------------------------------------------------------------------*
 | global dense matrices for element routines             genk 04/02    |
 *----------------------------------------------------------------------*/
static FLUID_DATA      *data;

/*!---------------------------------------------------------------------                                         
\brief main fluid3 control routine

<pre>                                                         genk 05/02
</pre>
\param  *actpart	 PARTITION    (i)	    
\param	*actintra	 INTRA        (i)
\param	*ele		 ELEMENT      (i)    actual element
\param	*estif_global	 ARRAY        (o)    element stiffness matrix
\param  *emass_global	 ARRAY        (o)    element mass matrix
\param	*etforce_global  ARRAY        (o)    element time force vector
\param  *eiforce_global  ARRAY        (o)    element iter force vecotr
\param	*edforce_global  ARRAY        (o)    ele dirichl. force vector
\param	*action	         CALC_ACTION  (i)
\param	*hasdirich	 INT          (o)    flag
\param  *hasext          INT          (o)    flag
\return void

------------------------------------------------------------------------*/
void fluid3(PARTITION   *actpart,
            INTRA       *actintra,
            ELEMENT     *ele,
            ARRAY       *estif_global,
            ARRAY       *emass_global, 
	    ARRAY       *etforce_global,
	    ARRAY       *eiforce_global,
	    ARRAY       *edforce_global,
            CALC_ACTION *action,
	    INT         *hasdirich,
	    INT         *hasext,
            CONTAINER   *container     	    
	   )
{
#ifdef D_FLUID3 
static INT              numff;      /* number of fluid field            */
DOUBLE                 *intforce;
MATERIAL               *actmat;     /* actual material                  */
FLUID_DYNAMIC          *fdyn;
static FLUID_DYN_CALC  *dynvar;
static FLUID_DYN_ML    *mlvar;
static FLUID_ML_SMESH  *submesh;
static FLUID_ML_SMESH  *ssmesh;
static INT              ndum;       /* dummy variable                   */
static INT              xele,yele,zele;/* numb. of subm. ele. in x,y,z  */
FIELD                  *actfield;   /* actual field                     */
INT smisal;

#ifdef DEBUG 
dstrc_enter("fluid3");
#endif

fdyn = alldyn[0].fdyn;
/*------------------------------------------------- switch to do option */
switch (*action)
{
/*------------------------------------------------------ initialization */
case calc_fluid_init:
/* ----------------------------------------- find number of fluid field */
  for (numff=0;numff<genprob.numfld;numff++)
  {
     actfield=&(field[numff]);
     if (actfield->fieldtyp==fluid)
     break;
  }
  dynvar = &(alldyn[numff].fdyn->dynvar);
  data   = &(alldyn[numff].fdyn->dynvar.data);
/*------------------------------------------- init the element routines */   
  f3_intg(data,0);
  f3_calele(data,dynvar,NULL,estif_global,emass_global,etforce_global,
            eiforce_global,edforce_global,NULL,NULL,1);
/*---------------------------------------------------- multi-level FEM? */   
  if (fdyn->mlfem==1) 
  {  
    mlvar   = &(alldyn[numff].fdyn->mlvar);
    submesh = &(alldyn[numff].fdyn->mlvar.submesh);
    ssmesh  = &(alldyn[numff].fdyn->mlvar.ssmesh);
/*------- determine number of submesh elements in coordinate directions */   
    math_intextract(mlvar->smelenum,&ndum,&xele,&yele,&zele);
/*------------------------------------- create submesh on parent domain */   
    f3_pdsubmesh(submesh,xele,yele,zele,mlvar->smorder,0);
/*-------------------- three-level FEM, i.e. dynamic subgrid viscosity? */   
    if (mlvar->smsgvi>2) 
    {
/*--- determine number of sub-submesh elements in coordinate directions */   
      math_intextract(mlvar->ssmelenum,&ndum,&xele,&yele,&zele);
/*--------------------------------- create sub-submesh on parent domain */   
      f3_pdsubmesh(ssmesh,xele,yele,zele,mlvar->ssmorder,1);
    }
/*----------------------- init the element routines for multi-level FEM */   
    f3_lsele(data,dynvar,mlvar,submesh,ssmesh,ele,estif_global,emass_global,
             etforce_global,eiforce_global,edforce_global,hasdirich,hasext,1); 
  }     	    
break;

/*------------------------------------------- call the element routines */
case calc_fluid:
/*---------------------------------------------------- multi-level FEM? */   
if (fdyn->mlfem==1) 
{
  smisal = ele->e.f3->smisal;
  if (smisal!=1) 
  {
/*------------------------------ create element submesh if not yet done */   
    f3_elesubmesh(ele,submesh,0);
/*-------------------------- create element sub-submesh if not yet done */   
    if (mlvar->smsgvi>2) f3_elesubmesh(ele,ssmesh,1);
  }  
  f3_lsele(data,dynvar,mlvar,submesh,ssmesh,ele,estif_global,emass_global,
           etforce_global,eiforce_global,edforce_global,hasdirich,hasext,0);
}	      
else  
  f3_calele(data,dynvar,ele,estif_global,emass_global,etforce_global,
                 eiforce_global,edforce_global,hasdirich,hasext,0);
break;

/*----------------------------------------------------------------------*/
default:
   dserror("action unknown\n");
break;
} /* end switch (*action) */

/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
/*----------------------------------------------------------------------*/
#endif
/*----------------------------------------------------------------------*/
return; 
} /* end of fluid3 */

