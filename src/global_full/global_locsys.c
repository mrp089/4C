/*!----------------------------------------------------------------------
\file
\brief 

<pre>
Maintainer: Steffen Genkinger
            genkinger@statik.uni-stuttgart.de
            http://www.uni-stuttgart.de/ibs/members/genkinger/
            0711 - 685-6127
</pre>

*----------------------------------------------------------------------*/
#include "../headers/standardtypes.h"
#include "../fluid2/fluid2.h"
#include "../fluid3/fluid3.h"
#include "../ale2/ale2.h"
#include "../ale3/ale3.h"
/*!----------------------------------------------------------------------*
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
 |                                                        genk 04/04    |
 | number of local co-ordinate systems                                  |
 | vector of structures of local co-ordinate systems                    |
 | defined in input_locsys.c                                          |
 *----------------------------------------------------------------------*/
extern INT            numlocsys; 
extern struct _LOCSYS *locsys;
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

/*-------------------------------------- static variables for this file */
static ARRAY     trans_a;
static DOUBLE  **trans;          /* transformation matrix */
static ARRAY     workm_a;
static DOUBLE  **workm;          /* working matrix for temp. result */
static ARRAY     workv_a;
static DOUBLE   *workv;          /* working vector for temp. result */
static ARRAY     nodalwork_a;
static DOUBLE   *nodalwork;      /* nodal working vector */

/*!--------------------------------------------------------------------- 
\brief inherit local co-ordinate system to elements

<pre>                                                         genk 04/04

local co-ordinate systems are assigned to design elements (points, lines
surfs, vols). They are NOT inherited to their lower design elements.

create local co-ordinate system on node level
			     
</pre>
\return void                                               
                                 
------------------------------------------------------------------------*/
void locsys_inherit_to_node()
{
INT      i,j,k,l;
INT      locsysId;
NODE     *actnode;
GNODE    *actgnode;
ELEMENT  *actele;

#ifdef DEBUG 
dstrc_enter("locsys_inherit_to_ele");
#endif


/*-------------------------- allocate transformation and working arrays 
  since there may be internally defined locsys, these arrays have to be
  allocated for ALL cases                                               */
trans    =amdef("trans"    ,&trans_a    ,MAXDOFPERELE,MAXDOFPERELE,"DA");
workm    =amdef("workm"    ,&workm_a    ,MAXDOFPERELE,MAXDOFPERELE,"DA");
workv    =amdef("workv"    ,&workv_a    ,MAXDOFPERELE,1           ,"DV");
nodalwork=amdef("nodalwork",&nodalwork_a,MAXDOFPERELE,1           ,"DV");

if (numlocsys==0) goto end;

/*--------------------------------------------------------- loop fields */
for (i=0; i<genprob.numfld; i++)
{
   /*--------------------------------------------- loop discretisations */
   for (j=0; j<field[i].ndis; j++)
   {
      /*---------------------------------------------------- loop nodes */
      for (k=0; k<field[i].dis[j].numnp; k++)
      {
         actnode=&(field[i].dis[j].node[k]);
         /*------ local co-cordinate system defined by design condition */
         actgnode=actnode->gnode;
         switch(actgnode->ondesigntyp)
         {
         case ondnode: locsysId = actgnode->d.dnode->locsysId;   break;
         case ondline: locsysId = actgnode->d.dline->locsysId;   break;
         case ondsurf: locsysId = actgnode->d.dsurf->locsysId;   break;
         case ondvol : locsysId = actgnode->d.dvol->locsysId;    break;
         case ondnothing:   
            dserror("GNODE not owned by any design object");     break;
         default: 
            dserror("Cannot create locsys on element level");    break;
         }
         actnode->locsysId=locsysId;
         if (locsysId>0)
         {
            for (l=0;l<actnode->numele;l++)
            {
               actele=actnode->element[l];
               actele->locsys=locsys_yes;
            } /* end loop over nodes */
         } /* endif (locsysId>0) */     
      } /* end loop over nodes */
   } /* end loop over discretisations */
} /* end loop over fields */

/*----------------------------------------------------------------------*/
end:
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of locsys_create */

/*!--------------------------------------------------------------------- 
\brief transform element stiffness matrix

<pre>                                                         genk 04/04

the element stiffness matrix of the actual element is transformed from
the global XYZ - co-ordinate system to the local one.
This names are a bit confusing, since the "local" co-ordinate system is
the one we are solving in, so we better introduce new names:
			     
estif  = stiffness matrix in the given XYZ cartesian co-system
estif* = stiffness matrix in the alternative xyz* co-system defined 
         in the input file
         
eload  = load vector in the given XYZ cartesian co-system
eload* = load vector in the alternative xyz* co-system

trans  = transformation matrix containing the direction cosini between
         XYZ and xyz*
                             
</pre>
\param   *ele        ELEMENT     (i)      actual element
\param  **estif1     DOUBLE      (i/o)    element matrix
\param  **estif2     DOUBLE      (i/o)    element matrix
\param   *vec1       DOUBLE      (i/o)    element RHS
\param   *vec2       DOUBLE      (i/o)    element RHS 
\sa locsys_trans_nodval

\return void                                               
                                 
------------------------------------------------------------------------*/
void locsys_trans(ELEMENT *ele, DOUBLE **estif1, DOUBLE **estif2, 
                                DOUBLE *vec1,    DOUBLE *vec2)
{
INT      i,j;        /* simply some counters */
INT      nd;         /* counter for total number of element dofs */
INT      ilocsys;    /* index of locsys */
INT      numdf;      /* number of dofs at actual node */
NODE     *actnode;   /* actual node */

#ifdef DEBUG 
dstrc_enter("locsys_trans");
#endif
#ifdef PERF
  perf_begin(22);
#endif

/*------------------------------------------------- initialise matrices */
amzero(&trans_a);

/*------------------------------------------ fill transformation matrix */
nd=0;
switch (ele->eltyp)
{
#ifdef D_FLUID2
case el_fluid2:
   dsassert (ele->e.f2->fs_on<=2,
             "no local co-ordinate system on free surface allowed!\n");
   /*----------------------------------------------- loop element nodes */
   for (i=0;i<ele->numnp;i++) 
   {
      actnode=ele->node[i];
      numdf=actnode->numdf;      
      ilocsys=actnode->locsysId-1;
      if(ilocsys>=0)
      {
         dsassert(ilocsys<numlocsys,"locsysId not existent!\n");
         if (numdf<4)
         {
            trans[nd][nd]     = locsys[ilocsys].lXx;
            trans[nd+1][nd]   = locsys[ilocsys].lXy;
            trans[nd][nd+1]   = locsys[ilocsys].lYx;
            trans[nd+1][nd+1] = locsys[ilocsys].lYy;
            trans[nd+2][nd+2] = ONE;
         }
         else if (numdf==4)
         {
            dserror("transformation for fluid node with 4 dofs not implemented!\n");         
         }
         else if (numdf==5) /*  node at free surf. w/ five dofs 
                            [vel, vel, pre, velg, velg]                 */
         {
            trans[nd][nd]     = locsys[ilocsys].lXx;
            trans[nd+1][nd]   = locsys[ilocsys].lXy;
            trans[nd][nd+1]   = locsys[ilocsys].lYx;
            trans[nd+1][nd+1] = locsys[ilocsys].lYy;
            trans[nd+2][nd+2] = ONE;
            trans[nd+3][nd+3] = locsys[ilocsys].lXx;
            trans[nd+4][nd+3] = locsys[ilocsys].lXy;
            trans[nd+3][nd+4] = locsys[ilocsys].lYx;
            trans[nd+4][nd+4] = locsys[ilocsys].lYy;         
         }
         else
            dserror("transformation not possible!\n");
      }
      else
      {
         for (j=0;j<numdf;j++) trans[nd+j][nd+j] = ONE;
      }
      nd+=numdf;
   } /* end loop over element nodes */
break;
#endif
#ifdef D_FLUID3
case el_fluid3:
   dsassert (ele->e.f3->fs_on<=2,
             "no local co-ordinate system on free surface allowed!\n");
   /*----------------------------------------------- loop element nodes */
   for (i=0;i<ele->numnp;i++) 
   {
      actnode=ele->node[i];
      numdf=actnode->numdf;      
      ilocsys=actnode->locsysId-1;
      if(ilocsys>=0)
      {
         dsassert(ilocsys<numlocsys,"locsysId not existent!\n");
         if (numdf<5)
         {
            trans[nd][nd]     = locsys[ilocsys].lXx;
            trans[nd+1][nd]   = locsys[ilocsys].lXy;
            trans[nd+2][nd]   = locsys[ilocsys].lXz;
            trans[nd][nd+1]   = locsys[ilocsys].lYx;
            trans[nd+1][nd+1] = locsys[ilocsys].lYy;
            trans[nd+2][nd+1] = locsys[ilocsys].lYz;
            trans[nd][nd+2]   = locsys[ilocsys].lZx;
            trans[nd+1][nd+2] = locsys[ilocsys].lZy;
            trans[nd+2][nd+2] = locsys[ilocsys].lZz;
            trans[nd+3][nd+3] = ONE;
         }
         else if (numdf==5)
         {
            dserror("transformation for fluid node with 5 dofs not implemented!\n"); 
         }
         else if (numdf==7) /*  node at free surf. w/ five dofs 
                          [vel, vel, vel, pre, velg, velg, velg]        */
         {
            trans[nd][nd]     = locsys[ilocsys].lXx;
            trans[nd+1][nd]   = locsys[ilocsys].lXy;
            trans[nd+2][nd]   = locsys[ilocsys].lXz;
            trans[nd][nd+1]   = locsys[ilocsys].lYx;
            trans[nd+1][nd+1] = locsys[ilocsys].lYy;
            trans[nd+2][nd+1] = locsys[ilocsys].lYz;
            trans[nd][nd+2]   = locsys[ilocsys].lZx;
            trans[nd+1][nd+2] = locsys[ilocsys].lZy;
            trans[nd+2][nd+2] = locsys[ilocsys].lZz;
            trans[nd+3][nd+3] = ONE;
            trans[nd+4][nd+4] = locsys[ilocsys].lXx;
            trans[nd+5][nd+4] = locsys[ilocsys].lXy;
            trans[nd+6][nd+4] = locsys[ilocsys].lXz;
            trans[nd+4][nd+5] = locsys[ilocsys].lYx;
            trans[nd+5][nd+5] = locsys[ilocsys].lYy;
            trans[nd+6][nd+5] = locsys[ilocsys].lYz;
            trans[nd+4][nd+6] = locsys[ilocsys].lZx;
            trans[nd+5][nd+6] = locsys[ilocsys].lZy;
            trans[nd+6][nd+6] = locsys[ilocsys].lZz;
         }
         else
            dserror("transformation not possible!\n");
      }
      else
      {
         for (j=0;j<numdf;j++) trans[nd+j][nd+j] = ONE;
      }
      nd+=numdf;
   } /* end loop over element nodes */
break;      
#endif
#ifdef D_ALE
case el_ale2:
   /*----------------------------------------------- loop element nodes */
   for (i=0;i<ele->numnp;i++) 
   {
      actnode=ele->node[i];
      numdf=actnode->numdf;
      dsassert(numdf==2,
            "numdf of ale2-ele not possible to combine with locsys!\n");
      ilocsys=actnode->locsysId-1;
      if(ilocsys>=0)
      {
         dsassert(ilocsys<numlocsys,"locsysId not existent!\n");
         trans[nd][nd]     = locsys[ilocsys].lXx;
         trans[nd+1][nd]   = locsys[ilocsys].lXy;
         trans[nd][nd+1]   = locsys[ilocsys].lYx;
         trans[nd+1][nd+1] = locsys[ilocsys].lYy;
      }
      else
      {
         for (j=0;j<numdf;j++) trans[nd+j][nd+j] = ONE;
      }
      nd+=numdf;
   } /* end loop over element nodes */
break;
#endif
default: dserror("no transformation implemented for this kind of element!\n");
} /* end switch (ele->eltyp */

/*------ perform the transformation: estif* = trans * estif * trans^t --*/
if (estif1!=NULL)
{
   /* workm = estif1 * trans^t */
   math_matmattrndense(workm,estif1,trans,nd,nd,nd,0,ONE);
   /* estif1* = trans * workm */
   math_matmatdense(estif1,trans,workm,nd,nd,nd,0,ONE);
} /* endif (estif1!=NULL) */

/*------ perform the transformation: estif* = trans * estif * trans^t --*/
if (estif2!=NULL)
{
   /* workm = estif2 * trans^t */
   math_matmattrndense(workm,estif2,trans,nd,nd,nd,0,ONE);
   /* estif2* = trans * workm */
   math_matmatdense(estif2,trans,workm,nd,nd,nd,0,ONE);
} /* endif (estif2!=NULL) */

/*------------------ perform the transformation: eload* = trans * eload */
if (vec1!=NULL)
{
   /* workv = trans * vec1 */
   math_matvecdense(workv,trans,vec1,nd,nd,0,ONE);
   /* copy result to vec1 */
   for(i=0;i<nd;i++) vec1[i]=workv[i];
} /* endif (vec1!=NULL) */

/*------------------ perform the transformation: eload* = trans * eload */
if (vec2!=NULL)
{
   /* workv = trans * vec2 */
   math_matvecdense(workv,trans,vec2,nd,nd,0,ONE);
   /* copy result to vec2 */ 
   for(i=0;i<nd;i++) vec2[i]=workv[i];
} /* endif (vec2!=NULL) */
   
/*----------------------------------------------------------------------*/
#ifdef PERF
  perf_end(22);
#endif
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of locsys_trans_ele */

/*!--------------------------------------------------------------------- 
\brief transform solution to global co-ordinate system

<pre>                                                         genk 04/04

the solution of the actual node may have to be transformed between
the global XYZ - co-ordinate system to the local one.
This names are a bit confusing, since the "local" co-ordinate system is
the one we are solving in, so we better introduce new names:

sol  = sol vector in the given XYZ cartesian co-system
sol* = sol vector in the alternative xyz* co-system

array   index of the array,        0 = sol                    
                                   1 = sol_increment          
                                   2 = sol_residual           
                                   3 = sol_mf                 

flag = 1 : transform sol in xyz* to XYZ
flag = 0 : transform sol in XYZ to xyz*
			     
</pre>
\param   *actfield      FIELD       (i)   actual field
\param    dis           INT         (i/o) index for dis
\param    array         INT         (i)   index for nodal array
\param    place         INT         (i)   place in nodal array
\param    flag          INT         (i)   just a flag 
\sa  locsys_trans_nodval()	

\return void                                               
                                 
------------------------------------------------------------------------*/
void locsys_trans_sol(FIELD *actfield, INT idis, INT array, 
                      INT place, INT flag)
{
INT      i,j;           /* simply some counters */
INT      iloccsys;      /* index of locsys */
INT      numdf;         /* number of dofs at node */
INT      numnp_total;   /* total number of nodes in field */
DOUBLE   **nodalsol;    /* pointer to nodal array */
NODE     *actnode;      /* actual node */
ELEMENT  *actele;       /* actual element */

#ifdef DEBUG 
dstrc_enter("locsys_trans_sol");
#endif

if (numlocsys==0) goto end;

numnp_total=actfield->dis[idis].numnp;

for (i=0;i<numnp_total;i++)
{
   actnode=&(actfield->dis[idis].node[i]);   
   /*----- any element can be used to find the local co-ordinate system */
   iloccsys=actnode->locsysId-1;
   if (iloccsys<0) continue; /* no locsys for this node */
   /*----- any element can be used to find the local co-ordinate system */
   actele=actnode->element[0];
   numdf=actnode->numdf;
   /*----------------------------------- fill the local solution vector */
   switch (array)
   {
   case 0:  nodalsol=actnode->sol.a.da;           break;
   case 1:  nodalsol=actnode->sol_increment.a.da; break;
   case 2:  nodalsol=actnode->sol_residual.a.da;  break;
   case 3:  nodalsol=actnode->sol_mf.a.da;        break;
   default: dserror("index out of range!\n");     break;
   } /* end switch (array) */
   
   /* copy values to working vector */
   for (j=0;j<numdf;j++) nodalwork[j]=nodalsol[place][j];
   /* transform vector */
   locsys_trans_nodval(actele,nodalwork,numdf,iloccsys,flag);
   /* copy result back to nodal sol-field */
   for (j=0;j<numdf;j++) nodalsol[place][j]=nodalwork[j];
} /* end loop numnp_total */

/*----------------------------------------------------------------------*/
end:
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of locsys_trans_sol */

/*!--------------------------------------------------------------------- 
\brief transform solution to global co-ordinate system

<pre>                                                         genk 04/04

the solution of the actual node may have to be transformed between
the global XYZ - co-ordinate system to the local one.
Here only nodes with a dirichlet condition are tranformed.
This names are a bit confusing, since the "local" co-ordinate system is
the one we are solving in, so we better introduce new names:

sol  = sol vector in the given XYZ cartesian co-system
sol* = sol vector in the alternative xyz* co-system

array   index of the array,        0 = sol                    
                                   1 = sol_increment          
                                   2 = sol_residual           
                                   3 = sol_mf                 

flag = 1 : transform sol in xyz* to XYZ
flag = 0 : transform sol in XYZ to xyz*
			     
</pre>
\param   *actfield      FIELD       (i)   actual field
\param    dis           INT         (i/o) index for dis
\param    array         INT         (i)   index for nodal array
\param    place         INT         (i)   place in nodal array
\param    flag          INT         (i)   just a flag 
\sa  locsys_trans_nodval()	

\return void                                               
                                 
------------------------------------------------------------------------*/
void locsys_trans_sol_dirich(FIELD *actfield, INT idis, INT array, 
                             INT place, INT flag)
{
INT      i,j;           /* simply some counters */
INT      iloccsys;      /* index of locsys */
INT      numdf;         /* number of dofs at node */
INT      numnp_total;   /* total number of nodes in field */
DOUBLE   **nodalsol;    /* pointer to nodal array */
NODE     *actnode;      /* actual node */
GNODE    *actgnode;     /* actual gnode */
ELEMENT  *actele;       /* actual element */

#ifdef DEBUG 
dstrc_enter("locsys_trans_sol");
#endif

if (numlocsys==0) goto end;

numnp_total=actfield->dis[idis].numnp;

for (i=0;i<numnp_total;i++)
{
   actnode=&(actfield->dis[idis].node[i]);
   actgnode=actnode->gnode;
   if (actgnode->dirich==NULL) continue; 
   /*----- any element can be used to find the local co-ordinate system */
   iloccsys=actnode->locsysId-1;
   if (iloccsys<0) continue; /* no locsys for this node */
   /*----- any element can be used to find the local co-ordinate system */
   actele=actnode->element[0];
   numdf=actnode->numdf;
   /*----------------------------------- fill the local solution vector */
   switch (array)
   {
   case 0:  nodalsol=actnode->sol.a.da;           break;
   case 1:  nodalsol=actnode->sol_increment.a.da; break;
   case 2:  nodalsol=actnode->sol_residual.a.da;  break;
   case 3:  nodalsol=actnode->sol_mf.a.da;        break;
   default: dserror("index out of range!\n");     break;
   } /* end switch (array) */
   
   /* copy values to working vector */
   for (j=0;j<numdf;j++) nodalwork[j]=nodalsol[place][j];
   /* transform vector */
   locsys_trans_nodval(actele,nodalwork,numdf,iloccsys,flag);
   /* copy result back to nodal sol-field */
   for (j=0;j<numdf;j++) nodalsol[place][j]=nodalwork[j];
} /* end loop numnp_total */

/*----------------------------------------------------------------------*/
end:
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of locsys_trans_sol */

/*!--------------------------------------------------------------------- 
\brief transform solution to global co-ordinate system

<pre>                                                         genk 04/04

the solution of the actual node is transformed from
the global XYZ - co-ordinate system to the local one.
This names are a bit confusing, since the "local" co-ordinate system is
the one we are solving in, so we better introduce new names:

    Transformation of displacements (3D):

     | (Dx*) |   | cos(Xx*)   cos(Yx*)   cos(Zx*) | | (DX) | 
     | (Dy*) | = | cos(Xy*)   cos(Yy*)   cos(Zy*) | | (DY) |
     | (Dz*) |   | cos(Xz*)   cos(Yz*)   cos(Zz*) | | (DZ) |

      val*     =                  T                   val      

cos(Yx*) =  cosine of angle between the given Y-base vector
            and the alternative x*-base vector

val   Displ./Vel. vector in the given XYZ cartesian co-system
val*  Displ./Vel. vector in the alternative xyz* co-system
T     Transformation matrix

flag = 1 : transform val in xyz* to XYZ
flag = 0 : transform val in XYZ to xyz*

</pre>
\param   *actele     ELEMENT        (i)   actual element
\param   *val        DOUBLE         (i/o) values to transform
\param    numdf      INT            (i)   number of dofs
\param    ilocsys    INT            (i)   index of locsys
\param    flag       INT            (i)   just a flag 

\sa inp_read_locsys()			     
\return void                                               
                                 
------------------------------------------------------------------------*/
void locsys_trans_nodval(ELEMENT *actele, DOUBLE *val, INT numdf, 
                         INT iloccsys, INT flag)
{
INT      i,j;           /* simply some counters */
LOCSYS   *actlocsys;    /* the actual local co-system xzy* */

#ifdef DEBUG 
dstrc_enter("locsys_trans_nodval");
#endif
#ifdef PERF
  perf_begin(22);
#endif
if (numlocsys==0) goto end;

/*------------------------------------------------- initialise matrices */
for(i=0;i<numdf;i++)
   for(j=0;j<numdf;j++)
      trans[i][j]=ZERO;

dsassert(iloccsys>=0,"locsysId out of range!\n");
dsassert(iloccsys<numlocsys,"locsysId not existent!\n");
actlocsys=&(locsys[iloccsys]);

/*-------------------------------- fill the nodal transformation matrix */
switch (actele->eltyp)
{
#ifdef D_FLUID2
case el_fluid2:
   if (numdf<4)
   {
      trans[0][0] = actlocsys->lXx;
      trans[1][0] = actlocsys->lXy;
      trans[0][1] = actlocsys->lYx;
      trans[1][1] = actlocsys->lYy;
      /*-- don't transform pressure dof, so reduce number of nodal dofs */
      numdf--;
   }
   else if (numdf==4)
   {
      dserror("transformation for fluid node with 4 dofs not implemented!\n");
   }
   else if (numdf==5) /*  node at free surf. w/ five dofs 
                          [vel, vel, pre, velg, velg]                   */
   {
      trans[0][0] = actlocsys->lXx;
      trans[1][0] = actlocsys->lXy;
      trans[0][1] = actlocsys->lYx;
      trans[1][1] = actlocsys->lYy;
      trans[2][2] = ONE;
      trans[3][3] = actlocsys->lXx;
      trans[4][3] = actlocsys->lXy;
      trans[3][4] = actlocsys->lYx;
      trans[4][4] = actlocsys->lYy;      
   }
   else
      dserror("transformation not possible!\n");
break; 
#endif         
#ifdef D_FLUID3
case el_fluid3:
   if (numdf<5)
   {
      trans[0][0] = actlocsys->lXx;
      trans[1][0] = actlocsys->lXy;
      trans[2][0] = actlocsys->lXz;
      trans[0][1] = actlocsys->lYx;
      trans[1][1] = actlocsys->lYy;
      trans[2][1] = actlocsys->lYz;
      trans[0][2] = actlocsys->lZx;
      trans[1][2] = actlocsys->lZy;
      trans[2][2] = actlocsys->lZz;
      trans[3][3] = ONE;
   }
   else if (numdf==5)
   {
      dserror("transformation for fluid node with 5 dofs not implemented!\n"); 
   }
   else if (numdf==7) /*  node at free surf. w/ five dofs 
                    [vel, vel, vel, pre, velg, velg, velg]        */
   {
      trans[0][0] = actlocsys->lXx;
      trans[1][0] = actlocsys->lXy;
      trans[2][0] = actlocsys->lXz;
      trans[0][1] = actlocsys->lYx;
      trans[1][1] = actlocsys->lYy;
      trans[2][1] = actlocsys->lYz;
      trans[0][2] = actlocsys->lZx;
      trans[1][2] = actlocsys->lZy;
      trans[2][2] = actlocsys->lZz;
      trans[3][3] = ONE;
      trans[4][4] = actlocsys->lXx;
      trans[5][4] = actlocsys->lXy;
      trans[6][4] = actlocsys->lXz;
      trans[4][5] = actlocsys->lYx;
      trans[5][5] = actlocsys->lYy;
      trans[6][5] = actlocsys->lYz;
      trans[4][6] = actlocsys->lZx;
      trans[5][6] = actlocsys->lZy;
      trans[6][6] = actlocsys->lZz;
   }
   else
      dserror("transformation not possible!\n");
break;      
#endif
#ifdef D_ALE
case el_ale2:
   trans[0][0] = actlocsys->lXx;
   trans[1][0] = actlocsys->lXy;
   trans[0][1] = actlocsys->lYx;
   trans[1][1] = actlocsys->lYy;   
break;
#endif
default: dserror("no transformation implemented for this kind of element!\n");
} /* end switch (actele->eltyp) */

switch (flag)
{
case 0:/* transformation: val* = trans * val */
   math_matvecdense(workv,trans,val,numdf,numdf,0,ONE);
break;
case 1: /* transformation: val = trans^t * val* */
   math_mattrnvecdense(workv,trans,val,numdf,numdf,0,ONE);
break;
default:
   dserror("flag out of range!\n");
} /* end switch (flag) */

/* copy result back to val */
for (j=0;j<numdf;j++) val[j]=workv[j];

/*----------------------------------------------------------------------*/
end:
#ifdef PERF
  perf_end(22);
#endif
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of locsys_trans_nodval */
