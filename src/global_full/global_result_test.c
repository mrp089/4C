/*!----------------------------------------------------------------------
\file
\brief 

<pre>
Maintainer: Ulrich Kuettler
            kuettler@lnm.mw.tum.de
            http://www.lnm.mw.tum.de/Members/kuettler
            089 - 289-15238
</pre>

*----------------------------------------------------------------------*/
#ifdef RESULTTEST
#include "../headers/standardtypes.h"
#include "../axishell/axishell.h"
#include "../shell9/shell9.h"
#include "../fluid_full/fluid_prototypes.h"
/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | general problem data                                                 |
 | global variable GENPROB genprob is defined in global_control.c       |
 *----------------------------------------------------------------------*/
extern struct _GENPROB     genprob;
/*!----------------------------------------------------------------------
\brief file pointers

<pre>                                                         m.gee 8/00
This structure struct _FILES allfiles is defined in input_control_global.c
and the type is in standardtypes.h                                                  
It holds all file pointers and some variables needed for the FRSYSTEM
</pre>
*----------------------------------------------------------------------*/
extern struct _FILES  allfiles;
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
 | vector of numfld FIELDs, defined in global_control.c                 |
 *----------------------------------------------------------------------*/
extern struct _FIELD      *field;


extern struct _PARTITION  *partition;


/*
 * an array of expected results
 */
struct _RESULTDESCR      *resultdescr;


/*
 * Read a positions description. See if its name equals |name|. If so
 * read |nargs| integer arguments and store them in |args|.
 * 
 * \param position   a string of the form "name(x1,...,xn)" read from the input file
 * \param name       the expected name
 * \param nargs      the expected number of arguments
 * \param args       an array of size |nargs| that is going to be filled
 */
static INT parse_position_descr(CHAR* position, CHAR* name, INT nargs, INT* args)
{
  CHAR* lp;
  INT name_length;
  INT ret = -1234567890;

#ifdef DEBUG 
  dstrc_enter("parse_position_descr");
#endif
  
  lp = strpbrk(position, "(");
  if (lp == NULL) {
    dserror("Missing left parenthesis in position description");
  }
  name_length = lp - position;

  if (strncmp(position, name, name_length) == 0) {
    CHAR* pos = lp;
    CHAR* rp;
    INT i;

    for (i=0; i<nargs-1; ++i) {
      args[i] = atoi(pos+1) - 1;
      pos = strpbrk(pos+1, ",");
      if (pos == NULL) {
        dserror("Missing comma in position description");
      }
    }
    args[nargs-1] = atoi(pos+1) - 1;
    rp = strpbrk(pos+1, ")");
    if (rp == NULL) {
      dserror("Missing right parenthesis in position description");
    }
    ret = 1;
  }
  else {
    ret = 0;
  }

#ifdef DEBUG 
  dstrc_exit();
#endif
  return ret;
}


/*
 * return the specified value
 * 
 * \param actnode    a node
 * \param position   a string of the form "name(x1,...,xn)" read from the input file
 *                   It describes a value in one of the solution array
 *                   of the given node.
 */
static DOUBLE get_node_result_value(NODE* actnode, CHAR* position)
{
  INT args[2];
  DOUBLE ret = -1234567890;
  
#ifdef DEBUG 
  dstrc_enter("get_node_result_value");
#endif

  /* for debugging...
  {
    INT i, j;
    for (i=0; i<actnode->sol.fdim; ++i) {
      printf("%d ", i);
      for (j=0; j<actnode->sol.sdim; ++j) {
        printf("%f ", actnode->sol.a.da[i][j]);
      }
      printf("\n");
    }
    printf("\n");
  }
  */
    
  if (parse_position_descr(position, "sol", 2, args) == 1) {
    ret = actnode->sol.a.da[args[0]][args[1]];
  }
  else if (parse_position_descr(position, "sol_increment", 2, args) == 1) {
    ret = actnode->sol_increment.a.da[args[0]][args[1]];
  }
  else if (parse_position_descr(position, "sol_residual", 2, args) == 1) {
    ret = actnode->sol_residual.a.da[args[0]][args[1]];
  }
  else if (parse_position_descr(position, "sol_mf", 2, args) == 1) {
    ret = actnode->sol_mf.a.da[args[0]][args[1]];
  }
  else {
    dserror("Unknown position specifier: %s", position);
  }
  
#ifdef DEBUG 
  dstrc_exit();
#endif
  return ret;
}


/*
 * Compare |actresult| with |givenresult| and return |0| if they are
 * considered to be equal.
 * 
 * \param err        the file where to document both values
 * \param res        the describtion of the expected result including name and tolerance
 */
static int compare_values(FILE* err, DOUBLE actresult, DOUBLE givenresult, RESULTDESCR *res)
{
  INT ret = 0;
  
#ifdef DEBUG 
  dstrc_enter("compare_values");
#endif

  fprintf(err,"actual = %24.16f, given = %24.16f\n", actresult, givenresult);
  if (!(FABS(FABS(actresult-givenresult)-FABS(actresult-givenresult)) < res->tolerance)) {
    printf("RESULTCHECK: %s is NAN!\n", res->name);
    ret = 1;
  }
  else if (FABS(actresult-givenresult) > res->tolerance) {
    printf("RESULTCHECK: %s not correct. actresult=%f, givenresult=%f\n", res->name, actresult, givenresult);
    ret = 1;
  }

#ifdef DEBUG 
  dstrc_exit();
#endif
  return ret;
}



/*
 * Find node with id |nodenum|. Only the given partition and
 * discretization is searched.
 */
static NODE* find_node(PARTITION* part, INT disnum, INT nodenum)
{
  INT i;
  NODE* res = 0;
  PARTDISCRET* pdis;
  
#ifdef DEBUG 
  dstrc_enter("find_node");
#endif

  pdis = &(part->pdis[disnum]);
  for (i=0; i<pdis->numnp; ++i) {
    if (pdis->node[i]->Id == nodenum) {
      res = pdis->node[i];
      break;
    }
  }
  
#ifdef DEBUG 
  dstrc_exit();
#endif
  return res;
}


/*
 * Find element with id |elenum|. Only the given partition and
 * discretization is searched.
 */
static ELEMENT* find_element(PARTITION* part, INT disnum, INT elenum)
{
  INT i;
  ELEMENT* res = 0;
  PARTDISCRET* pdis;
  
#ifdef DEBUG 
  dstrc_enter("find_element");
#endif

  pdis = &(part->pdis[disnum]);
  for (i=0; i<pdis->numele; ++i) {
    if (pdis->element[i]->Id == elenum) {
      res = pdis->element[i];
      break;
    }
  }
  
#ifdef DEBUG 
  dstrc_exit();
#endif
  return res;
}


/*!---------------------------------------------------------------------  
\brief testing of result 

<pre>                                                         genk 10/03  

Before checking in the latest version it's necessery to check the whole
program. In this context it seems to be useful to check the numerical
results, too.

</pre>  

------------------------------------------------------------------------*/
void global_result_test() 
{
#ifndef PARALLEL
  FIELD  *alefield     = NULL;
  FIELD  *structfield  = NULL;
  FIELD  *fluidfield   = NULL;
#endif
  PARTITION *alepart     = NULL;
  PARTITION *structpart  = NULL;
  PARTITION *fluidpart   = NULL;
  NODE   *actnode;
  DOUBLE  actresult;
  FILE   *err = allfiles.out_err;
  INT i;
  INT nerr = 0;

#ifdef DEBUG 
  dstrc_enter("global_result_test");
#endif

#ifndef PARALLEL
  if (genprob.numff>-1) fluidfield  = &(field[genprob.numff]);
  if (genprob.numsf>-1) structfield = &(field[genprob.numsf]);
  if (genprob.numaf>-1) alefield    = &(field[genprob.numaf]);
#endif

  if (genprob.numff>-1) fluidpart  = &(partition[genprob.numff]);
  if (genprob.numsf>-1) structpart = &(partition[genprob.numsf]);
  if (genprob.numaf>-1) alepart    = &(partition[genprob.numaf]);

  if (genprob.numresults>0) {
    /* let's do it in a fency style :) */
    printf("\n[37;1mChecking results ...[m\n");
  }

  for (i=0; i<genprob.numresults; ++i) {
    PARTITION* actpart = NULL;
    RESULTDESCR* res = &(resultdescr[i]);
    
    switch (res->field) {
    case fluid:
      actpart = fluidpart;
      break;
    case ale:
      actpart = alepart;
      break;
    case structure:
      actpart = structpart;
      break;
    default:
      dserror("Unknown field typ");
    }

    if (res->node != -1) {
      actnode = find_node(actpart, res->dis, res->node);
      if (actnode != 0) {
        actresult = get_node_result_value(actnode, res->position);
        nerr += compare_values(err, actresult, res->value, res);
      }
    }
    else if (res->element != -1) {
      ELEMENT* actelement = find_element(actpart, res->dis, res->element);
      if (actelement == 0) {
        continue;
      }
    
#ifdef D_AXISHELL
      if (actelement->eltyp == el_axishell) {
        INT args[3];
        if (parse_position_descr(res->position, "stress_GP", 3, args) == 1) {
          actresult = actelement->e.saxi->stress_GP.a.d3[args[0]][args[1]][args[2]];
          nerr += compare_values(err, actresult, res->value, res);
        }
        else if (parse_position_descr(res->position, "stress_ND", 3, args) == 1) {
          actresult = actelement->e.saxi->stress_ND.a.d3[args[0]][args[1]][args[2]];
          nerr += compare_values(err, actresult, res->value, res);
        }
        else {
          dserror("Unknown position specifier");
        }
      }
#endif
    
#ifdef D_SHELL9
      if (actelement->eltyp == el_shell9) {
        INT args[3];
        if (parse_position_descr(res->position, "stresses", 3, args) == 1) {
          actresult = actelement->e.s9->stresses.a.d3[args[0]][args[1]][args[2]];
          nerr += compare_values(err, actresult, res->value, res);
        }
        else {
          dserror("Unknown position specifier");
        }
      }
#endif

    }
    else {
      /* special cases that need further code support */
      switch (genprob.probtyp) {
      case prb_fluid:
#ifdef D_FLUID
#ifndef PARALLEL
        fluid_cal_error(fluidfield,res->dis);
#endif
        break;
#endif
      default:
        break;
      }
    }
  }

  if (nerr > 0) {
    dserror("Result check failed");
  }


/*----------------------------------------------------------------------*/
#ifdef DEBUG 
  dstrc_exit();
#endif
  return;
} /* end of global_result_test */
#endif
