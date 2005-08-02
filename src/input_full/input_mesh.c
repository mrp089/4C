/*!----------------------------------------------------------------------
\file
\brief input of mesh data

<pre>
Maintainer: Malte Neumann
            neumann@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/neumann/
            0711 - 685-6121
</pre>

*----------------------------------------------------------------------*/
#include "../headers/standardtypes.h"
#include "../shell8/shell8.h"
#include "../shell9/shell9.h"
#include "../wall1/wall1.h"
#include "../brick1/brick1.h"
#include "../fluid3/fluid3.h"
#include "../fluid3/fluid3_prototypes.h"
#include "../fluid2/fluid2.h"
#include "../fluid2/fluid2_prototypes.h"
#include "../fluid2_pro/fluid2pro_prototypes.h"
#include "../ale3/ale3.h"
#include "../ale2/ale2.h"
#include "../axishell/axishell.h"
#include "../beam3/beam3.h"
#include "../interf/interf.h"

#ifdef D_WALLGE
#include "../wallge/wallge.h"
#endif

#ifdef D_FLUID2_PRO
#include "../fluid2_pro/fluid2pro_prototypes.h"
#endif

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
/*!----------------------------------------------------------------------
\brief file pointers

<pre>                                                         m.gee 8/00
This structure struct _FILES allfiles is defined in input_control_global.c
and the type is in standardtypes.h
It holds all file pointers and some variables needed for the FRSYSTEM
</pre>
*----------------------------------------------------------------------*/
extern struct _FILES  allfiles;
/* global variable: flag for the creation of a second discretisation */
extern INT      create_dis;
extern struct _TWOPHASE_DATA     *twophase_data;
/*----------------------------------------------------------------------*
 | Global variables for this file                        m.gee 11/00    |
 *----------------------------------------------------------------------*/
static ARRAY tmpnodes1;
static ARRAY tmpnodes2;

#ifdef D_SSI
void inp_struct_field_ssi(FIELD *masterfield, FIELD *slavefield);
#endif

/*----------------------------------------------------------------------*
 | input of fields                                        m.gee 4/01    |
 *----------------------------------------------------------------------*/
void inpfield()
{
INT  i,j,k;
INT  node_id;
INT  numnd;
INT  nnode_total = 0;

#ifdef DEBUG
dstrc_enter("inpfield");
#endif

create_dis = 0;
genprob.maxnode    = 0;
/*--------------------------------------read node coordinates from file */
inpnodes();
/*--------------------------------------------------------- read field  */
/*----------------------------------------------- FSI 3D typ of problem */
if (genprob.probtyp == prb_fsi)
{
   if (genprob.numfld!=3) dserror("numfld != 3 for FSI");

   field = (FIELD*)CCACALLOC(genprob.numfld,sizeof(FIELD));

   field[genprob.numsf].fieldtyp = structure;
   inpdis(&(field[genprob.numsf]));
   inp_struct_field(&(field[genprob.numsf]));

   field[genprob.numff].fieldtyp = fluid;
   inpdis(&(field[genprob.numff]));
   inp_fluid_field (&(field[genprob.numff]));

   field[genprob.numaf].fieldtyp = ale;
   inpdis(&(field[genprob.numaf]));
   inp_ale_field  (&(field[genprob.numaf]));

/*--- ale and fluid field are supposed to be compatible, so inherit info */
}

#ifdef D_SSI
if (genprob.probtyp == prb_ssi)
{
   if (genprob.numfld!=2) dserror("numfld != 2 for FSI");

   field = (FIELD*)CCACALLOC(genprob.numfld,sizeof(FIELD));

   field[0].fieldtyp = structure;
   field[1].fieldtyp = structure;
   field[0].ndis=1;
   field[1].ndis=1;
   inp_struct_field_ssi(&(field[0]),&(field[1]));
}
#endif

/*------------------------------------------- structure type of problem */
if (genprob.probtyp==prb_structure)
{
   if (genprob.numfld!=1) dserror("numfld != 1 for structural problem");
   field = (FIELD*)CCACALLOC(genprob.numfld,sizeof(FIELD));

   field[genprob.numsf].fieldtyp = structure;
   inpdis(&(field[genprob.numsf]));
   inp_struct_field(&(field[genprob.numsf]));
}

/*---------------------------------------- Optimisation type of problem */
if (genprob.probtyp == prb_opt)
{  /*-- structure type of problem */
   if (genprob.numfld!=1) dserror("numfld != 1 for structural problem");
   field = (FIELD*)CCACALLOC(genprob.numfld,sizeof(FIELD));

   field[0].fieldtyp = structure;
   inpdis(&(field[0]));
   inp_struct_field(&(field[0]));
}

/*----------------------------------------------- fluid type of problem */
if (genprob.probtyp==prb_fluid)
{
   if (genprob.numfld==1) /* single field fluid problem                 */
   {
      field = (FIELD*)CCACALLOC(genprob.numfld,sizeof(FIELD));

      field[genprob.numff].fieldtyp = fluid;
      inpdis(&(field[genprob.numff]));
      inp_fluid_field (&(field[genprob.numff]));
   }
   else if (genprob.numfld==2) /* two field fluid problem (fluid+ale)       */
   {
      field = (FIELD*)CCACALLOC(genprob.numfld,sizeof(FIELD));

      field[genprob.numff].fieldtyp = fluid;
      inpdis(&(field[genprob.numff]));
      inp_fluid_field (&(field[genprob.numff]));

      field[genprob.numaf].fieldtyp = ale;
      inpdis(&(field[genprob.numaf]));
      inp_ale_field  (&(field[genprob.numaf]));
   }
   else dserror("NUMFLD>2 not allowed for Problemtype FLUID\n");
}

/*------------------------------------------------- ale type of problem */
if (genprob.probtyp==prb_ale)
{
   if (genprob.numfld!=1) dserror("numfld != 1 for ale problem");
   field = (FIELD*)CCACALLOC(genprob.numfld,sizeof(FIELD));

   field[genprob.numaf].fieldtyp = ale;
   inpdis(&(field[genprob.numaf]));
   inp_ale_field(&(field[genprob.numaf]));
}
/*---------------------------------------- Optimisation type of problem */
if (genprob.probtyp == prb_opt)
{
/* not yet implemented */
}
/* copy nodes for the second discretisation */
if (create_dis == 1)
{
  /*dserror("Copying of nodes for second discretisarion not yet implemented!!");*/

  numnd = genprob.nnode;
  /*genprob.nnode = 2*genprob.nnode;*/
  amredef(&tmpnodes1,2*(genprob.nnode),3,"DA");
  amredef(&tmpnodes2,2*(genprob.nnode),1,"IV");

  for (i=numnd; i<2*genprob.nnode; i++)
  {
    tmpnodes1.a.da[i][0] = tmpnodes1.a.da[i-numnd][0];
    tmpnodes1.a.da[i][1] = tmpnodes1.a.da[i-numnd][1];
    tmpnodes1.a.da[i][2] = tmpnodes1.a.da[i-numnd][2];
    tmpnodes2.a.iv[i]    = tmpnodes2.a.iv[i-numnd] + numnd;
  }
}
/*-------------------------------------- assign the nodes to the fields */
for (i=0; i<genprob.numfld; i++)
for (j=0;j<field[i].ndis;j++)
{
  inp_assign_nodes(&(field[i].dis[j]));
  nnode_total += field[i].dis[j].numnp;
}

amdel(&tmpnodes1);
amdel(&tmpnodes2);
/*---------------------------------- make element-node-element topology */
  genprob.nnode = nnode_total;
  genprob.nodes = (NODE**)CCACALLOC(genprob.maxnode,sizeof(NODE*));

  for (i=0; i<genprob.numfld; i++)
  {
    for (j=0;j<field[i].ndis;j++)
    {
      /* make pointers to all nodes in genprob.nodes */
      for (k=0; k<field[i].dis[j].numnp; k++)
      {
        node_id = field[i].dis[j].node[k].Id;
        dsassert(node_id <= genprob.maxnode,"Zu wenig KNOTEN");
        genprob.nodes[node_id] = &(field[i].dis[j].node[k]);
      }
      inp_topology(&(field[i].dis[j]));
    }
  }
/*----------------------------------------------------------------------*/
#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of inpfield */





/*----------------------------------------------------------------------*
 | sort nodes to the fields                               m.gee 4/01    |
 *----------------------------------------------------------------------*/
void inp_assign_nodes(DISCRET *actdis)
{
INT  i,j;
INT  node_Id;
INT counter;
INT minusone=-1;
ARRAY nodeflag;
ELEMENT *actele;
#ifdef DEBUG
dstrc_enter("inp_assign_nodes");
#endif
amdef("nodeflag",&nodeflag,2*genprob.nnode,1,"IV");
aminit(&nodeflag,&minusone);
/*----------------  set a flag to the node_id for each node in the field */
for (i=0; i<actdis->numele; i++)
{
   actele = &(actdis->element[i]);
   for (j=0; j<actele->numnp; j++)
   {
      node_Id = actele->lm[j];
      nodeflag.a.iv[node_Id]=node_Id;
   }
}
/*----------------------------------------------------- count the flags */
counter=0;
for (i=0; i<2*genprob.nnode; i++)
{
   if (nodeflag.a.iv[i]!=-1) counter++;
}
actdis->numnp=counter;
dsassert(actdis->numnp>0, "No nodes in discretization");
/*-------------------------------------- Allocate the nodes to the field */
actdis->node = (NODE*)CCACALLOC(counter,sizeof(NODE));
/*---------------- assign the node Ids and coords to the NODE structure */
counter=0;
for (i=0; i<2*genprob.nnode; i++)
{
   if (nodeflag.a.iv[i]!=-1)
   {
      actdis->node[counter].Id = nodeflag.a.iv[i];
      for (j=0; j<3; j++)
      {
         actdis->node[counter].x[j] = tmpnodes1.a.da[i][j];
      }

      if (genprob.maxnode < (actdis->node[counter].Id+1))
        genprob.maxnode = actdis->node[counter].Id + 1;

      counter++;
   }
}
/*----------------------------------------------------------------------*/
amdel(&nodeflag);
#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of inp_assign_nodes */



/*!---------------------------------------------------------------------
\brief input of disretisation data

<pre>                                                         genk 08/02
</pre>

\return void

------------------------------------------------------------------------*/
void inpdis(FIELD *actfield)
{
INT  ierr=0;
#ifdef DEBUG
dstrc_enter("inpdis");
#endif
/*--------------------------------------------------- set default value */
actfield->ndis=1;

/*------------------------------------------------- read discretisation */
if (frfind("--DISCRETISATION")==0) goto end;
frread();
switch(actfield->fieldtyp)
{
  case fluid:
    while(strncmp(allfiles.actplace,"------",6)!=0)
    {
      frint("NUMFLUIDDIS", &(actfield->ndis),&ierr);
      frread();
    }
    break;
  case structure:
    while(strncmp(allfiles.actplace,"------",6)!=0)
    {
      frint("NUMSTRUCDIS", &(actfield->ndis),&ierr);
      frread();
    }
    break;
  case ale:
    while(strncmp(allfiles.actplace,"------",6)!=0)
    {
      frint("NUMALEDIS", &(actfield->ndis),&ierr);
      frread();
    }
    break;
  default:
    dserror("Unknown fieldtype");
    break;
}
frrewind();
/*----------------------------------------------------------------------*/

end:
#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of inpdis */



/*----------------------------------------------------------------------*
 | input of node coords                                   m.gee 4/01    |
 *----------------------------------------------------------------------*/
void inpnodes()
{
INT  ierr=0;
INT  counter;
#ifdef DEBUG
dstrc_enter("inpnodes");
#endif
/*--------------- allocate temporary array for coordinates of all nodes */
amdef("tempnod1",&tmpnodes1,(genprob.nnode),3,"DA");
amdef("tempnod2",&tmpnodes2,(genprob.nnode),1,"IV");
/*---------------------------------------------------------- read nodes */
if (frfind("--NODE COORDS")==0) dserror("frfind: NODE COORDS is not in input file");
frread();
counter=0;
while(strncmp(allfiles.actplace,"------",6)!=0)
{
   frint("NODE",&(tmpnodes2.a.iv[counter]),&ierr);
   if (ierr!=1) dserror("reading of nodes failed");
   (tmpnodes2.a.iv[counter])--;

   frdouble_n("COORD",&(tmpnodes1.a.da[counter][0]),3,&ierr);
   if (ierr!=1) dserror("reading of nodes failed");

   counter++;

   frread();
}
frrewind();
/*----------------------------------------------------------------------*/

#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of inpnodes */





/*----------------------------------------------------------------------*
 | input of structure field                               m.gee 4/01    |
 *----------------------------------------------------------------------*/
void inp_struct_field(FIELD *structfield)
{
INT  ierr;
INT  counter=0;
INT  elenumber;
char *colpointer;
#ifdef DEBUG
dstrc_enter("inp_struct_field");
#endif
/*----------------------------------------- allocate one discretization */
/*structfield->ndis=1;*/
if (structfield->ndis>1)
   dserror("different discretisations not implemented yet for structural elements\n");
structfield->dis = (DISCRET*)CCACALLOC(structfield->ndis,sizeof(DISCRET));
/*-------------------------------------------- count number of elements */
if (frfind("--STRUCTURE ELEMENTS")==1)
{
  frread();
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    counter++;
    frread();
  }
}
structfield->dis[0].numele = counter;
/*--------------------------------------------------- allocate elements */
structfield->dis[0].element=(ELEMENT*)CCACALLOC(structfield->dis[0].numele,sizeof(ELEMENT));
/*------------------------------------------------------- read elements */
if (frfind("--STRUCTURE ELEMENTS")==0) goto end;
frread();
counter=0;
while(strncmp(allfiles.actplace,"------",6)!=0)
{
   colpointer = allfiles.actplace;
   elenumber  = strtol(colpointer,&colpointer,10);
   structfield->dis[0].element[counter].Id = --elenumber;
/*---------- read the typ of element and call element readning function */
/*------------------------------------------------ elementtyp is SHELL8 */
   frchk("SHELL8",&ierr);
   if (ierr==1)
   {
#ifndef D_SHELL8
      dserror("SHELL8 needed but not defined in Makefile");
#endif
   }
#ifdef D_SHELL8
   if (ierr==1)
   {
      structfield->dis[0].element[counter].eltyp=el_shell8;
      s8inp(&(structfield->dis[0].element[counter]));
   }
#endif
/*------------------------------------------------ elementtyp is SHELL9 */
   frchk("SHELL9",&ierr);
   if (ierr==1)
   {
#ifndef D_SHELL9
      dserror("SHELL9 needed but not defined in Makefile");
#endif
   }
#ifdef D_SHELL9
   if (ierr==1)
   {
      structfield->dis[0].element[counter].eltyp=el_shell9;
      s9inp(&(structfield->dis[0].element[counter]));
   }
#endif
/*------------------------------------------------ elementtyp is BRICK1 */
   frchk("BRICK1",&ierr);
   if (ierr==1)
   {
#ifndef D_BRICK1
      dserror("BRICK1 needed but not defined in Makefile");
#endif
   }
#ifdef D_BRICK1
   if (ierr==1)
   {
      structfield->dis[0].element[counter].eltyp=el_brick1;
      c1inp(&(structfield->dis[0].element[counter]));
   }
#endif
/*------------------------------------------------ elementtyp is WALL  */
   frchk("WALL",&ierr);
   if (ierr==1)
   {
#ifndef D_WALL1
      dserror("WALL1 needed but not defined in Makefile");
#endif
   }
#ifdef D_WALL1
   if (ierr==1)
   {
      structfield->dis[0].element[counter].eltyp=el_wall1;
      w1inp(&(structfield->dis[0].element[counter]));
   }
#endif
/*------------------------------------------------ elementtyp is BEAM3  */
   frchk("BEAM3",&ierr);
   if (ierr==1)
   {
#ifndef D_BEAM3
      dserror("BEAM3 needed but not defined in Makefile");
#endif
   }
#ifdef D_BEAM3
   if (ierr==1)
   {
      structfield->dis[0].element[counter].eltyp=el_beam3;
      b3inp(&(structfield->dis[0].element[counter]));
   }
#endif
/*------------------------------------------------ elementtyp is WALL  */
   frchk("SAXI",&ierr);
   if (ierr==1)
   {
#ifndef D_AXISHELL
      /*dserror("AXISHELL needed but not defined in Makefile");*/
#endif
   }
#ifdef D_AXISHELL
   if (ierr==1)
   {
      structfield->dis[0].element[counter].eltyp=el_axishell;
      saxi_inp(&(structfield->dis[0].element[counter]));
   }
#endif
/*-------------------------------------------- elementtyp is Interf  */
   frchk("IF",&ierr);
   if (ierr==1)
   {
#ifndef D_INTERF
      dserror("INTERF needed but not defined in Makefile");
#endif
   }
#ifdef D_INTERF
   if (ierr==1)
   {
      structfield->dis[0].element[counter].eltyp=el_interf;
      interf_inp(&(structfield->dis[0].element[counter]));
   }
#endif
/*-------------------------------------------- elementtyp is Wallge  */
   frchk("WGE",&ierr);
   if (ierr==1)
   {
#ifndef D_WALLGE
      dserror("WALLGE needed but not defined in Makefile");
#endif
   }
#ifdef D_WALLGE
   if (ierr==1)
   {
      structfield->dis[0].element[counter].eltyp=el_wallge;
      wge_inp(&(structfield->dis[0].element[counter]));
   }
#endif
/*--------------------------------------------other structural elements */
   counter++;
   frread();
}
frrewind();
/*----------------------------------------------------------------------*/

end:
#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of inp_struct_field */


/*----------------------------------------------------------------------*
 | input of structure field for ssi                      chfoe 07/04    |
 *----------------------------------------------------------------------*/
#ifdef D_SSI
void inp_struct_field_ssi(FIELD *masterfield, FIELD *slavefield)
{
INT  ierr,ierr_s,ierr_m;
INT  slavecounter=0;
INT  mastercounter=0;
INT  counter=0;
INT  elenumber;
char *colpointer;
INT  *flag;
ARRAY flag_a;

#ifdef DEBUG
dstrc_enter("inp_struct_field_ssi");
#endif
/*----------------------------------------- allocate one discretization */
if (masterfield->ndis>1)
   dserror("different discretisations not implemented yet for structural elements\n");
if (slavefield->ndis>1)
   dserror("different discretisations not implemented yet for structural elements\n");
masterfield->dis = (DISCRET*)CCACALLOC(masterfield->ndis,sizeof(DISCRET));
slavefield->dis  = (DISCRET*)CCACALLOC(slavefield->ndis,sizeof(DISCRET));
/*-------------------------------------------- count number of elements */
#ifndef D_WALL1
      dserror("WALL1 needed but not defined in Makefile");
#endif

if (frfind("--STRUCTURE ELEMENTS")==1)
{
  frread();
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    counter++;
    frread();
  }
}

flag = amdef("flag",&flag_a,counter,1,"IV");
counter=0;

if (frfind("--STRUCTURE ELEMENTS")==1)
{
  frread();
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    frchk("WALL",&ierr);
    if (ierr!=1)
    dserror("SSI only possible with wall elements!");
    frchk("Master",&ierr_m);
    frchk("Slave",&ierr_s);
    if (ierr_s==1)
    {
       slavecounter++;
       flag[counter]=1;
    }
    else if (ierr_m==1)
    {
       mastercounter++;
       flag[counter]=0;
    }
    else dserror("SSI_COUPTYP not possible for wall element!");
    counter++;
    frread();
  }
}
masterfield->dis[0].numele = mastercounter;
slavefield->dis[0].numele = slavecounter;
/*--------------------------------------------------- allocate elements */
masterfield->dis[0].element=(ELEMENT*)CCACALLOC(masterfield->dis[0].numele,sizeof(ELEMENT));
slavefield->dis[0].element=(ELEMENT*)CCACALLOC(slavefield->dis[0].numele,sizeof(ELEMENT));
/*------------------------------------------------------- read elements */
if (frfind("--STRUCTURE ELEMENTS")==0) goto end;
frread();
counter=0;
mastercounter=0;
slavecounter=0;
while(strncmp(allfiles.actplace,"------",6)!=0)
{
   switch (flag[counter])
   {
   case 0: /*master field */
      colpointer = allfiles.actplace;
      elenumber  = strtol(colpointer,&colpointer,10);
      masterfield->dis[0].element[mastercounter].Id = --elenumber;
/*---------- read the typ of element and call element readning function */
/*------------------------------------------------ elementtyp is SHELL8 */

/*------------------------------------------------ elementtyp is WALL  */
      frchk("WALL",&ierr);
      if (ierr==1)
      {
#ifndef D_WALL1
         dserror("WALL1 needed but not defined in Makefile");
#endif
      }
#ifdef D_WALL1
      if (ierr==1)
      {
         masterfield->dis[0].element[mastercounter].eltyp=el_wall1;
         w1inp(&(masterfield->dis[0].element[mastercounter]));
      }
#endif
      mastercounter++;
   break;
   case 1:
      colpointer = allfiles.actplace;
      elenumber  = strtol(colpointer,&colpointer,10);
      slavefield->dis[0].element[slavecounter].Id = --elenumber;
/*---------- read the typ of element and call element readning function */
/*------------------------------------------------ elementtyp is SHELL8 */

/*------------------------------------------------ elementtyp is WALL  */
      frchk("WALL",&ierr);
      if (ierr==1)
      {
#ifndef D_WALL1
         dserror("WALL1 needed but not defined in Makefile");
#endif
      }
#ifdef D_WALL1
      if (ierr==1)
      {
         slavefield->dis[0].element[slavecounter].eltyp=el_wall1;
         w1inp(&(slavefield->dis[0].element[slavecounter]));
      }
#endif
      slavecounter++;
   break;
   default:
      dserror("flag out of range!\n");
   }
/*--------------------------------------------other structural elements */
   counter++;
   frread();
}
frrewind();
/*----------------------------------------------------------------------*/

end:
#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of inp_struct_field_ssi */
#endif




/*----------------------------------------------------------------------*
 | input of fluid field                                   m.gee 4/01    |
 *----------------------------------------------------------------------*/
void inp_fluid_field(FIELD *fluidfield)
{
INT  ierr;
INT  counter=0;
#if defined(D_FLUID2TU) || defined(D_FLUID2_PRO)
INT  cpro=0;
#endif
INT  elenumber;
char *colpointer;

#ifdef DEBUG
dstrc_enter("inp_fluid_field");
#endif
/*-------------------------------------------- allocate discretizations */
/*fluidfield->ndis=1; */
fluidfield->dis = (DISCRET*)CCACALLOC(fluidfield->ndis,sizeof(DISCRET));

/*
remarks about different discretisations:
we asume to read in one "global" discretisation from the input file.
from this discretisation all the other ones can be directly derived!!!  */
/*-------------------------------------------- count number of elements */
if (frfind("--FLUID ELEMENTS")==1)
{
  frread();
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    counter++;
    frread();
  }
}
fluidfield->dis[0].numele = counter;
/*--------------------------------------------------- allocate elements */
fluidfield->dis[0].element=(ELEMENT*)CCACALLOC(fluidfield->dis[0].numele,sizeof(ELEMENT));
/*------------------------------------------------------- read elements */
if (frfind("--FLUID ELEMENTS")==0) goto end;
frread();
counter=0;
while(strncmp(allfiles.actplace,"------",6)!=0)
{
   colpointer = allfiles.actplace;
   elenumber  = strtol(colpointer,&colpointer,10);
   fluidfield->dis[0].element[counter].Id = --elenumber;
/*---------- read the typ of element and call element readning function */
/*-------------------------------------------- elementtyp is FLUID2_PRO */
   frchk("FLUID2_PRO",&ierr);
   if (ierr==1)
   {
#ifndef D_FLUID2_PRO
      dserror("FLUID2_PRO needed but not defined in Makefile");
#endif
   }
#ifdef D_FLUID2_PRO
   if (ierr==1)
   {
      /*-------------------------- allocate elements of second discretisation */
      if (cpro==0)
      {
         if(fluidfield->ndis<2)
            dserror("NUMFLUIDDIS has to be g.t. 1 for FLUID2_PRO Elements!\n");
         fluidfield->dis[1].numele = fluidfield->dis[0].numele;
	 fluidfield->dis[1].element=(ELEMENT*)CCACALLOC(fluidfield->dis[1].numele,sizeof(ELEMENT));
         cpro++;
         create_dis = 1;
      } /* endif (cpro==0) */
      fluidfield->dis[0].element[counter].eltyp=el_fluid2_pro;
      fluidfield->dis[1].element[counter].eltyp=el_fluid2_pro;
      f2pro_inp(&(fluidfield->dis[0].element[counter]));
      f2pro_dis(&(fluidfield->dis[0].element[counter]),&(fluidfield->dis[1].element[counter]),
           genprob.nele,genprob.nnode);
      genprob.nodeshift = genprob.nnode;
      goto read;
   }
#endif


/* elementtyp is FLUID3 */
   frchk("FLUID3 ",&ierr);
   if (ierr==1)
   {
#ifndef D_FLUID3
      dserror("FLUID3 needed but not defined in Makefile");
#endif
   }
#ifdef D_FLUID3
   if (ierr==1)
   {
      fluidfield->dis[0].element[counter].eltyp=el_fluid3;
      f3inp(&(fluidfield->dis[0].element[counter]),counter);
   }
#endif


/* elementtyp is FLUID3_FAST */
   frchk("FLUID3_FAST",&ierr);
   if (ierr==1)
   {
#ifndef D_FLUID3_F
      dserror("FLUID3_F needed but not defined in Makefile");
#else
      fluidfield->dis[0].element[counter].eltyp=el_fluid3_fast;
      f3inp(&(fluidfield->dis[0].element[counter]),counter);
#endif
   }


/*------------------------------------------------ elementtyp is FLUID2 */
   frchk("FLUID2",&ierr);
   if (ierr==1)
   {
#ifndef D_FLUID2
     dserror("FLUID2 needed but not defined in Makefile");
#endif
   }
#ifdef D_FLUID2
   if (ierr==1)
   {
     fluidfield->dis[0].element[counter].eltyp=el_fluid2;
     f2_inp(&(fluidfield->dis[0].element[counter]),counter);

#ifdef D_FLUID2TU
     if (fluidfield->dis[0].element[counter].e.f2->turbu == 2 ||
         fluidfield->dis[0].element[counter].e.f2->turbu == 3 )
     {
       if(cpro==0)
       {
         fluidfield->dis[1].numele = fluidfield->dis[0].numele;
         fluidfield->dis[1].element=(ELEMENT*)CCACALLOC(fluidfield->dis[1].numele,sizeof(ELEMENT));
         cpro++;
         create_dis = 1;
       } /* endif (cpro==0) */
       fluidfield->dis[1].element[counter].eltyp=el_fluid2_tu;
       f2tu_dis(&(fluidfield->dis[0].element[counter]),&(fluidfield->dis[1].element[counter]),
           genprob.nele,genprob.nnode);
       genprob.nodeshift = genprob.nnode;
     } /* endif (e.f2->turbu == 2 || 3) */
#endif
   }
#endif
/*----------------------------------------------------------------------*/
#ifdef D_FLUID2_PRO
read:
#endif
   counter++;
   frread();
}
frrewind();
/*----------------------------------------------------------------------*/

end:
#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of inp_fluid_field */








/*----------------------------------------------------------------------*
 | input of ale field                                     m.gee 4/01    |
 *----------------------------------------------------------------------*/
void inp_ale_field(FIELD *alefield)
{
INT  ierr;
INT  counter=0;
INT  elenumber;
char *colpointer;
#ifdef DEBUG
dstrc_enter("inp_ale_field");
#endif

/*----------------------------------------- allocate one discretization */
/*alefield->ndis=1;*/
if (alefield->ndis>1)
   dserror("different discretisations not implemented yet for structural elements\n");
alefield->dis = (DISCRET*)CCACALLOC(alefield->ndis,sizeof(DISCRET));
/*-------------------------------------------- count number of elements */
if (frfind("--ALE ELEMENTS")==1)
{
  frread();
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    counter++;
    frread();
  }
}
frrewind();
alefield->dis[0].numele = counter;
/*--------------------------------------------------- allocate elements */
alefield->dis[0].element=(ELEMENT*)CCACALLOC(alefield->dis[0].numele,sizeof(ELEMENT));
/*------------------------------------------------------- read elements */
frrewind();
if (frfind("--ALE ELEMENTS")==0) goto end;
frread();
counter=0;
while(strncmp(allfiles.actplace,"------",6)!=0)
{
   colpointer = allfiles.actplace;
   elenumber  = strtol(colpointer,&colpointer,10);
   alefield->dis[0].element[counter].Id = --elenumber;
/*---------- read the typ of element and call element reading function */
/*------------------------------------------------ elementtyp is ALE3 */
   frchk("ALE3",&ierr);
   if (ierr==1)
   {
#ifndef D_ALE
      dserror("ALE3 needed but not defined in Makefile");
#endif
   }
#ifdef D_ALE
   if (ierr==1)
   {
      alefield->dis[0].element[counter].eltyp=el_ale3;
      ale3inp(&(alefield->dis[0].element[counter]));
   }
#endif
/*------------------------------------------------ elementtyp is ALE2 */
   frchk("ALE2",&ierr);
   if (ierr==1)
   {
#ifndef D_ALE
      dserror("ALE2 needed but not defined in Makefile");
#endif
   }
#ifdef D_ALE
   if (ierr==1)
   {
      alefield->dis[0].element[counter].eltyp=el_ale2;
      ale2inp(&(alefield->dis[0].element[counter]));
   }
#endif
/*------------------------------------------------------- elementtyp is */
   counter++;
   frread();
}
frrewind();
/*----------------------------------------------------------------------*/

end:
#ifdef DEBUG
dstrc_exit();
#endif
return;
} /* end of inp_ale_field */
