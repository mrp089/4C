/*!---------------------------------------------------------------------
\file
\brief contains wall_contact_augmentation_em routine used for the update 
of Lagrange Multipliers in case of Energy-Momentum conserving Int. scheme 

---------------------------------------------------------------------*/
#ifdef GEMM
#ifdef WALLCONTACT
/*!----------------------------------------------------------------------
\brief the header of everything
*----------------------------------------------------------------------*/
#include "../headers/standardtypes.h"
#include "../headers/solution_mlpcg.h"
#include "../headers/solution.h"
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
 | vector of numfld FIELDs, defined in global_control.c                 |
 *----------------------------------------------------------------------*/
extern struct _FIELD      *field;
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
 | ranks and communicators                                              |
 | This structure struct _PAR par; is defined in main_ccarat.c
 *----------------------------------------------------------------------*/
 extern struct _PAR   par;                      
/*-----------------------------------------------------------------------*/
double Heaviside(double a);
double Mac(double a);
/*-----------------------------------------------------------------------*/

/*! 
\addtogroup CONTACT 
*//*! @{ (documentation module open)*/
/*!------------------------------------------------------------------------
\brief main structure for 2-D contact (bilinear discretization)

<pre>                                                           m.gee 10/02
defined in wall_contact_detection.c                             
</pre>

-------------------------------------------------------------------------*/
#ifdef WALLCONTACT
extern struct _WALL_CONTACT contact;
#endif
/*!----------------------------------------------------------------------
\brief short description
                                                      
<pre>                                                         m.gee 10/02    
This routine is used for the update of Lagrange Multipliers which is called 
when Augmented Lagrangian Method is used to enforce the contact constraints.
Since in Eneergy-Momentum conserving scheme only the frictionless contact is
considered normal component of Lagrange Multiplier is to be updated. 
</pre>
\param actintra INTRA*    (i) the intra-communicator of this field
\return void


---------------------------------------------------------------------*/
void wall_contact_augmentation_em(INTRA *actintra)
{
int    i,j,k;
int myrank;
double pen_par;
double lamda_n;
/*--------------------------------------------------------------*/
pen_par     = contact.n_pen_par; /*-----Normal penalty parameter*/
/*--------------------------------------------------------------*/

#ifdef DEBUG 
dstrc_enter("wall_contact_augmentation_em");
#endif
 
 myrank = actintra->intra_rank;
 
  for(i=0; i<contact.set_size; i++) {       /*loop over active slave nodes*/
  
      if ( contact.contact_set[i]->node->proc != myrank) continue;    /*check ownership*/
      
      lamda_n = Heaviside(contact.contact_set[i]->history->g_n)                                                                 /*Update of normal multiplier*/
              * Mac(contact.contact_set[i]->history->pr_multipliers[0] + pen_par * contact.contact_set[i]->history->g_tilda );  /*Chapter 7 of book by Laursen*/
      contact.contact_set[i]->history->pr_multipliers[0] = lamda_n;       
  }
  
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} 
/* end of wall_contact_augmentation_em*/
  
/*! @} (documentation module close)*/
#endif
#endif
