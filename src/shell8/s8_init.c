/*!----------------------------------------------------------------------
\file
\brief 

<pre>
Maintainer: Michael Gee
            gee@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/gee/
            0771 - 685-6572
</pre>

*----------------------------------------------------------------------*/
#ifdef D_SHELL8
#include "../headers/standardtypes.h"
#include "shell8.h"
/*----------------------------------------------------------------------*
 | initialize the element                                 m.gee 6/01    |
 *----------------------------------------------------------------------*/
void s8init(FIELD *actfield)
{
INT          i,j,k;
ELEMENT     *actele;
NODE        *actnode;
S8_DATA      data;
/*DOUBLE     **a3ref;*/
INT          numa3;
DOUBLE       a3[3];
ARRAY        collaverdir_a;
DOUBLE     **collaverdir;

#ifdef DEBUG 
dstrc_enter("s8init");
#endif
/*----------------------------------------------------------------------*/
for (i=0; i<actfield->dis[0].numele; i++)
{
   actele = &(actfield->dis[0].element[i]);
   if (actele->eltyp != el_shell8) continue;
   /*------------------------------------------ init integration points */
   s8intg(actele,&data,0);/* ueberfluessig! ?*/
   /*---------------------------------------- init directors of element */
   s8a3(actele,&data,0);
   /*---------------------------------- allocate the space for stresses */
   am4def("forces",&(actele->e.s8->forces),1,18,MAXGAUSS,0,"D3");
}
/*--------------------- now do modification of directors bischoff style */
/*------------------------------ allocate space for directors at a node */
collaverdir = amdef("averdir",&collaverdir_a,3,MAXELE,"DA");
/*----------------------------------------------------------------------*/
for (i=0; i<actfield->dis[0].numnp; i++)
{
   actnode = &(actfield->dis[0].node[i]);
   numa3=0;
   for (j=0; j<actnode->numele; j++)
   {
      actele = actnode->element[j];
      if (actele->eltyp != el_shell8) continue;
      for (k=0; k<actele->numnp; k++)
      {
         if (actele->node[k] == actnode)
         {
            collaverdir[0][numa3] = actele->e.s8->a3ref.a.da[0][k];
            collaverdir[1][numa3] = actele->e.s8->a3ref.a.da[1][k];
            collaverdir[2][numa3] = actele->e.s8->a3ref.a.da[2][k];
            numa3++;
            if (numa3 > MAXELE) dserror("Too many elements to a node, MAXELE too small");
            break;
         }
      }
   }
/*----------- do nothing if there is only one shell8 element to actnode */
   if (numa3 <= 1) continue;
/*------------------------------------------------ make shared director */
   s8averdir(collaverdir,numa3,a3);
/*---------------------------------put shared director back to elements */
   for (j=0; j<actnode->numele; j++)
   {
      actele = actnode->element[j];
      if (actele->eltyp != el_shell8) continue;
      for (k=0; k<actele->numnp; k++)
      {
         if (actele->node[k] == actnode)
         {
            actele->e.s8->a3ref.a.da[0][k] = a3[0];
            actele->e.s8->a3ref.a.da[1][k] = a3[1];
            actele->e.s8->a3ref.a.da[2][k] = a3[2];
            break;
         }
      }
   }
}/* end of loop over all nodes */
/*----------------------------------------------------------------------*/
amdel(&collaverdir_a);
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of s8init */
#endif
 
