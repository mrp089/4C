/*!---------------------------------------------------------------------
\file
\brief contains wall_contact_update routine used to update some
variables within each Newton iteraion

---------------------------------------------------------------------*/
#ifdef WALLCONTACT
/*!----------------------------------------------------------------------
\brief the header of everything
*----------------------------------------------------------------------*/
#include "../headers/standardtypes.h"
#include "../headers/solution_mlpcg.h"
#include "../headers/solution.h"
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

/*Inner product function which is frequently used in the code.*/
double inner_pr(double *a , double *b);

/*Heaviside function*/
double Heaviside(double a);

/*!----------------------------------------------------------------------
\brief short description
                                                      
<pre>                                                         m.gee 10/02    
This routine is used to update some history variables within each 
Newton iteration which is mainly necessary for frictional problems.
</pre>
\param actfield FIELD*      (i)   the discretization
\param actintra   INTRA*    (i)   the intra-communicator of this field                  
\return void


---------------------------------------------------------------------*/
void wall_contact_update(FIELD *actfield, INTRA *actintra)
{
int    i,j,k,l,m,n,p,q,r,s,t,tt,qq;
int    ii,jj;
int    fr_flag, cet_flag;
int    myrank,nproc,index,master_owner[3];
ARRAY  con_flag_s,con_flag_r;
SPOOLMAT *K;
int    numeq;
int    numeq_total;
double distance;
double pr_d;
double temp1, temp2;
double dist1, dist2;
double local_coordinate;
double cr_length, rf_length;
double pen_par, tan_pen_par;
double t_n, t_aux, t_tan, t_trial, norm_t_trial, phi_trial, del_gama;
double g, g_aux;
double value;
double M11, m11, friction;
double norm_1 , norm_2;
double unit_norm_1 , unit_norm_2;
double unit_v1[3], unit_v2[3], unit_v3[3], unit_v3_aux[3], *tangent, relative_pos[3], cr_length_vec[3], rf_length_vec[3];
double pos_ybar1[3], pos_ybar2[3]; 
double norm_vec_1[3], norm_vec_2[3];
double rel_pos_ybar1[3], rel_pos_ybar2[3];
DISCRET *dis;
NODE *closestptr;
NODE *neigbour_nodes[2];
NODE *triple[3];
NODE *element_nodes[3];
GLINE *neigbour_glineptr;
#ifdef PARALLEL
MPI_Status status;
#endif
/*---------------------------------------------------------------------*/
pen_par     = contact.n_pen_par;  /*-----------Normal penalty parameter*/
tan_pen_par = contact.t_pen_par;  /*-------Tangential penalty parameter*/
friction    = contact.fr_coef;   /*-------------coefficient of friction*/
fr_flag     = contact.FR_flag;   /*-----------------------friction flag*/
cet_flag    = contact.CET_flag; /*------Augmneted Lagr. or Penalty flag*/
/*---------------------------------------------------------------------*/

k = l = m = n = p = q = r = s = t = tt = qq = 0;
t_n = 0.0;
t_aux = 0.0;

#ifdef DEBUG 
dstrc_enter("wall_contact_update");
#endif
/*----------------------------------------------------------------------*/
myrank = actintra->intra_rank;
nproc  = actintra->intra_nprocs;
/*----------------------------------------------------------------------*/
/*All the contact nodes are assigned the contact_flag of contact_off*/
     for(i=0; i<contact.ng_slavenode;i++) contact.g_slavenode[i]->contactflag = contact_off;
     for(i=0; i<contact.ng_masternode;i++) contact.g_masternode[i]->contactflag = contact_off;

/*Current positions of the contact nodes (both master and slave) are updated.*/ 
  for(i=0; i<contact.ng_slavenode;i++){
    for(k=0; k<3; k++)
      contact.g_slavenode[i]->node->x_cr[k] = contact.g_slavenode[i]->node->x[k] + contact.g_slavenode[i]->node->sol.a.da[0][k];     
  }
  
  for(i=0; i<contact.ng_masternode;i++){
    for(k=0; k<3; k++)
      contact.g_masternode[i]->node->x_cr[k] = contact.g_masternode[i]->node->x[k] + contact.g_masternode[i]->node->sol.a.da[0][k];     
  } 



  for(i=0;i<contact.ng_slavenode;i++){  /* Loop over all slave nodes starts*/

  if ( contact.g_slavenode[i]->node->proc != myrank) continue; 

  distance = 0.0;
  pr_d      = 1.0e12;
  
  for(j=0; j<3; j++) {
    triple[j] = NULL;         /*In triple the previous node, closest node and next node (in the CCW direction of element local system) are stored */
    element_nodes[j] = NULL;  /*In element nodes, the nodes of the master segment are stored.*/
  }                           /*element_nodes[0] = starting node; element_nodes[1] = end node */
  
    neigbour_nodes[0] = NULL; /*Neigbour nodes of the closest node are stored(Previous and next) but*/
    neigbour_nodes[1] = NULL; /*their order is not known.(which one is previous and which one is next)*/
    neigbour_glineptr = NULL; /*pointer to the glines of the closest node. It is used in determination*/
                              /*of the order of the neigbour nodes*/
    for(j=0;j<contact.ng_masternode;j++){   /*for each slave node, closest master node is determined by distance*/       
      distance = 0.0;
      for(k=0; k<3; k++) {                  /*check over all master nodes.*/
        distance += DSQR((contact.g_slavenode[i]->node->x_cr[k] - contact.g_masternode[j]->node->x_cr[k]));
        }
        distance = sqrt(distance);      
        if(distance < pr_d){
        closestptr = contact.g_masternode[j]->node;
        pr_d = distance;      
        }
    }
    	  	
  triple[1] = closestptr;     /*closest node pointer is assigned to triple[1]*/
  n = 0; 
  for(l=0; l<closestptr->gnode->ngline; l++){    /* loop over the glines of the closest node*/  
    if(closestptr->gnode->gline[l]->contype != contact_none){   /*check whether the gline is a contact line or not*/
      neigbour_glineptr = closestptr->gnode->gline[l]; 
        for(m=0; m<2; m++){
          if(neigbour_glineptr->gnode[m] != closestptr->gnode){   /*loop over the gnodes of the gline*/
          neigbour_nodes[n] = neigbour_glineptr->gnode[m]->node;  /*if it is different than the closest node*/
	  n++;                                                    /*then this node is one of the neigbour nodes*/
	 }    
       }
     }
   }	          

   	 
  for(p=0; p<closestptr->numele; p++){	   /*loop over the elements of the closest node*/  
    for(q=0; q<closestptr->element[p]->numnp; q++)     /*for each element reach the nodes of this element*/	     
      if(closestptr->element[p]->node[q] == closestptr)  s = q;   /*element node numbering is in CCW fashion.*/
                                                                  /*First determine the position of the closest node*/
    for(q=0; q<closestptr->element[p]->numnp; q++){   		  /*in the local sysytem and assign it to s.*/
      for(m=0; m<2; m++){	                          
        if(neigbour_nodes[m] == closestptr->element[p]->node[q]){ /* For each neigbour node determine the location */
        r = q;                                                    /* in which neigbour element (q) and the order in */
	tt = m;                                                   /* the local system (m) and store these in r and tt*/  
	}
      }	
    }	 		  
	   
    for(t=0; t<closestptr->element[p]->numnp; t++){   /*loop over the nodes of each element of the closest node*/     
    s = (s+t) % 4;                                    /*to keep within the numbering system of 0-3 make use of mod operator*/
      if( closestptr->element[p]->node[s] == closestptr->element[p]->node[r]){  /*Moving from the source (s = closest node)*/
      qq = t;	                                                                /*determine how many steps are required to reach the */
      break;}                                                                   /*target(r = neigbour node) and break*/ 
    }	
     
      if(qq==1) triple[2] = neigbour_nodes[tt];  /* if just one step is marched to reach the target(neigbour) then this neigbour*/
      else triple[0] = neigbour_nodes[tt];       /* is the next node(assigned to triple[2]) otherwise it is the previous node*/	                                                                    
 }                                               /* Notice that if the closest node is a corner node than previous node or next node*/
                                                 /* may not exist.*/
    
    
  
  if(triple[0] != NULL && triple[2] != NULL){    /* if the closest node is not a corner node*/
    for(t=0; t<3; t++){
      unit_v1[t] = triple[1]->x_cr[t] - triple[0]->x_cr[t]; /*unit_v1 is from previous to closest*/
      unit_v2[t] = triple[2]->x_cr[t] - triple[1]->x_cr[t]; /*unit_v2 is from closest to next*/
      }     
    
    unit_norm_1 = sqrt(inner_pr(unit_v1,unit_v1));
    unit_norm_2 = sqrt(inner_pr(unit_v2,unit_v2));
    
    for(t=0; t<3; t++){    /* These vectors are normalized.*/
      unit_v1[t] = unit_v1[t] / unit_norm_1; 
      unit_v2[t] = unit_v2[t] / unit_norm_2;
      }          
      
  } 
          	     
  if(triple[2] == NULL){  /* Lower Corner*/
    for(t=0; t<3; t++){
      unit_v1[t] = triple[1]->x_cr[t] - triple[0]->x_cr[t]; /*unit_v1 is from previous to closest.*/      
      unit_v2[t] = 0.0;                                     /*no unit_v2*/
      }
    
    unit_norm_1 = sqrt(inner_pr(unit_v1,unit_v1));        
    
    for(t=0; t<3; t++)    /* These vectors are normalized.*/
      unit_v1[t] = unit_v1[t] / unit_norm_1;
          
  }
    
  if(triple[0] == NULL){  /* Upper Corner */
    for(t=0; t<3; t++){
      unit_v1[t] = triple[2]->x_cr[t] - triple[1]->x_cr[t]; /*unit_v1 is from  to closest to next.*/      
      unit_v2[t] = 0.0;                                     /*no unit_v2*/
      }
     
    unit_norm_1 = sqrt(inner_pr(unit_v1,unit_v1));             
    
    for(t=0; t<3; t++)   /* These vectors are normalized.*/
      unit_v1[t] = unit_v1[t] / unit_norm_1;
    
  }
    
  for(t=0; t<3; t++)
    relative_pos[t] = contact.g_slavenode[i]->node->x_cr[t] - closestptr->x_cr[t];

    
    if(triple[0] != NULL && triple[2] != NULL) {          /*if the closest node is not a corner node*/
    /*----------------------------------------------------------------------------------------------*/
    /* A refers to the closest node*/
    /* A+1 refers to the next node*/
    /* A-1 refers to the previous node*/
    /* Simple inner product sign checks are done to determine the master segment containing the projection.*/
    /* Chapter 5 of the book by Laursen*/
    /*----------------------------------------------------------------------------------------------*/
      if(inner_pr(relative_pos,unit_v1) > 0.0 && inner_pr(relative_pos,unit_v2) >= 0.0){                   
      /*A   A+1*/
      
      unit_v3[0] =   unit_v2[1]; /*outward normal is stored in unit_v3 and obtained by cross product of unit_v2 and e3*/
      unit_v3[1] = - unit_v2[0];
      unit_v3[2] =   0.0;     
      
      g = -1.0 * inner_pr(relative_pos, unit_v3);  /*value of gap function.*/
      t_n = contact.g_slavenode[i]->history->pr_multipliers[0] + pen_par * g;     
      
        if(t_n <= 0.0){   
          contact.g_slavenode[i]->contactflag = contact_off;     /* contact flag is switched to the proper value*/
          contact.g_slavenode[i]->history->pr_masters[0] = NULL;
          contact.g_slavenode[i]->history->pr_masters[1] = NULL;
          contact.g_slavenode[i]->history->pr_closest     = closestptr;
          contact.g_slavenode[i]->history->cr_g           = g;
          continue;                                            /* No contact----Continue with the next slave node*/
        }
      
        else {
          contact.g_slavenode[i]->contactflag = contact_on;   /*contact flag is switched to the proper value*/
	 
          element_nodes[0] = contact.g_slavenode[i]->node;    /*Nodes of the contact element element are assigned. element_nodes[0] = slave*/
          element_nodes[1] = triple[1];                       /*element_nodes[1] = closest node (depends on the case)*/
          element_nodes[2] = triple[2];                       /*element_nodes[2] = next node (depends on the case)*/
	  tangent = unit_v2;                                  /*Correct unit vector is assigned to the tangent vector(basis)*/
	  contact.g_slavenode[i]->mymasters[0] = triple[1]->gnode; /*Master nodes are assigned.(defined in gnode)*/
	  contact.g_slavenode[i]->mymasters[1] = triple[2]->gnode; /*These are used in parallel version.*/
	
	  for(l=0; l<3; l++) cr_length_vec[l] = element_nodes[2]->x_cr[l] - element_nodes[1]->x_cr[l]; 
	  cr_length = sqrt(inner_pr(cr_length_vec, cr_length_vec));    /*Current length of the master segment(appears in Fint and K expressions.)*/
        
	  local_coordinate = inner_pr(relative_pos, unit_v2) / cr_length;  /*local coordinate of the projection*/
	
	}
      } 
     		
      else if(inner_pr(relative_pos,unit_v1) <= 0.0 && inner_pr(relative_pos,unit_v2) < 0.0){         
      /*A-1    A*/
      unit_v3[0] =   unit_v1[1]; /*outward normal is stored in unit_v3 and obtained by cross product of unit_v1 and e3*/
      unit_v3[1] = - unit_v1[0];
      unit_v3[2] =   0.0;      
        
      g = -1.0 * inner_pr(relative_pos, unit_v3);  /*value of gap function.*/     
      t_n = contact.g_slavenode[i]->history->pr_multipliers[0] + pen_par * g;
      
        if(t_n <= 0.0){   
          contact.g_slavenode[i]->contactflag = contact_off;    /* contact flag is switched to the proper value*/
          contact.g_slavenode[i]->history->pr_masters[0] = NULL;
          contact.g_slavenode[i]->history->pr_masters[1] = NULL;
          contact.g_slavenode[i]->history->pr_closest     = closestptr;
          contact.g_slavenode[i]->history->cr_g           = g;
          continue;                                           /* No contact----Continue with the next slave node*/
        }
	
	else{
	
    	  contact.g_slavenode[i]->contactflag = contact_on;       /*contact flag is switched to the proper value*/
	  element_nodes[0] = contact.g_slavenode[i]->node;        /*see line 279-285*/
          element_nodes[1] = triple[0];  
          element_nodes[2] = triple[1];
          tangent = unit_v1;
  	  contact.g_slavenode[i]->mymasters[0] = triple[0]->gnode;
	  contact.g_slavenode[i]->mymasters[1] = triple[1]->gnode;
	
	  for(l=0; l<3; l++) cr_length_vec[l] = element_nodes[2]->x_cr[l] - element_nodes[1]->x_cr[l]; 
	  cr_length = sqrt(inner_pr(cr_length_vec, cr_length_vec));  
        
	  local_coordinate = 1.0 - FABS(inner_pr(relative_pos, unit_v1) / cr_length);
	
	}  		
    }
      	   	
      else if(inner_pr(relative_pos,unit_v1) <= 0.0 && inner_pr(relative_pos,unit_v2) >= 0.0){ 
      /*Either element can contain the projection*/
        
	
	unit_v3[0] =   unit_v1[1]; /*outward normal is stored in unit_v3 and obtained by cross product of unit_v1 and e3*/
        unit_v3[1] = - unit_v1[0];
        unit_v3[2] =   0.0;     
      
        unit_v3_aux[0] =  unit_v2[1];
	unit_v3_aux[1] = -unit_v2[0];
	unit_v3_aux[2] =  0.0;
            
        g     = -1.0 * inner_pr(relative_pos, unit_v3);  /*value of gap function.*/     
	t_n = contact.g_slavenode[i]->history->pr_multipliers[0] + pen_par * g;
	
	g_aux = -1.0 * inner_pr(relative_pos, unit_v3_aux);
	t_aux = contact.g_slavenode[i]->history->pr_multipliers[0] + pen_par * g_aux;
		
	if(t_n<=0.0 && t_aux<=0.0){
	  contact.g_slavenode[i]->contactflag = contact_off;         /* contact flag is switched to the proper value*/
          contact.g_slavenode[i]->history->pr_masters[0] = NULL;
          contact.g_slavenode[i]->history->pr_masters[1] = NULL;
          contact.g_slavenode[i]->history->pr_closest     = closestptr;
          contact.g_slavenode[i]->history->cr_g           = g;
          continue;                                                /* No contact----Continue with the next slave node*/
        }
	else{
	
	contact.g_slavenode[i]->contactflag = contact_on;             /*contact flag is switched to the proper value*/
	
         for(l=0; l<3; l++){
	   norm_vec_1[l] = triple[1]->x_cr[l] - triple[0]->x_cr[l];
	   norm_vec_2[l] = triple[2]->x_cr[l] - triple[1]->x_cr[l];
	 }
	 
	norm_1 = sqrt(inner_pr(norm_vec_1, norm_vec_1));
	norm_2 = sqrt(inner_pr(norm_vec_2, norm_vec_2));     

	temp1 = 1.0 - FABS(inner_pr(relative_pos,unit_v1) / norm_1);
	temp2 = inner_pr(relative_pos,unit_v2) / norm_2;
	
	  for(t=0; t<3; t++){
	    pos_ybar1[t] = (1.0 - temp1)*triple[0]->x_cr[t] + temp1*triple[1]->x_cr[t];  /*Position vector of the slave node on each segment*/
	    pos_ybar2[t] = temp2*triple[1]->x_cr[t] + (1.0 - temp2)*triple[2]->x_cr[t];  /*is calculated.*/
	  }
	  for(t=0; t<3; t++){
	    rel_pos_ybar1[t] = contact.g_slavenode[i]->node->x_cr[t] - pos_ybar1[t];  /*Relative position vector of the slave node w.r.t. the*/
	    rel_pos_ybar2[t] = contact.g_slavenode[i]->node->x_cr[t] - pos_ybar2[t];  /*closest node is calculated for both case.*/
	  }
	
	dist1 = sqrt(inner_pr(rel_pos_ybar1,rel_pos_ybar1));  /*two different penetration values are calculated.*/
	dist2 = sqrt(inner_pr(rel_pos_ybar2,rel_pos_ybar2));  /*and the larger one is penalized. Therefore the corresponding element*/
	  	                                              /*is assumed to be the owner of the projection.*/
        
        
	if(dist1 >= dist2){
	
	  local_coordinate = temp1;                           /*see line 279-285*/
	  element_nodes[0] = contact.g_slavenode[i]->node;
	  element_nodes[1] = triple[0];
	  element_nodes[2] = triple[1];
	  tangent = unit_v1;
	  contact.g_slavenode[i]->mymasters[0] = triple[0]->gnode;
	  contact.g_slavenode[i]->mymasters[1] = triple[1]->gnode;
	  
	  for(l=0; l<3; l++) cr_length_vec [l] = element_nodes[2]->x_cr[l] - element_nodes[1]->x_cr[l]; 
	  cr_length = sqrt(inner_pr(cr_length_vec, cr_length_vec));  
	}
	
    	else if(dist1 < dist2){
     
	  local_coordinate = temp2;                           /*see line 279-285*/  
	  element_nodes[0] = contact.g_slavenode[i]->node;
          element_nodes[1] = triple[1];
	  element_nodes[2] = triple[2];
	  tangent = unit_v2;
	  contact.g_slavenode[i]->mymasters[0] = triple[1]->gnode;
	  contact.g_slavenode[i]->mymasters[1] = triple[2]->gnode;
	  g = g_aux;
	   
	  for(l=0; l<3; l++){
	    cr_length_vec [l] = element_nodes[2]->x_cr[l] - element_nodes[1]->x_cr[l]; 
	    unit_v3[l] = unit_v3_aux[l];
	    }
	    
	  cr_length = sqrt(inner_pr(cr_length_vec, cr_length_vec));  
	} 
      }
    }
    
      else if(inner_pr(relative_pos,unit_v1) > 0.0 && inner_pr(relative_pos,unit_v2) < 0.0){
   /*Closest node is the projection of the slave node*/
      unit_v3[0] =  0.5*( unit_v2[1]+unit_v1[1] ); /*outward normal is stored in unit_v3 and obtained by the average of        */
      unit_v3[1] = -0.5*( unit_v2[0]+unit_v1[0] ); /* (cross product of unit_v2 and e3)  and (cross product of unit_v1 and e3) */
      unit_v3[2] =  0.0;     
      
      g = -1.0 * inner_pr(relative_pos, unit_v3);  /*value of gap function.*/     
      t_n = contact.g_slavenode[i]->history->pr_multipliers[0] + pen_par * g;
      
        if(t_n <= 0.0){   
          contact.g_slavenode[i]->contactflag = contact_off;    /* contact flag is switched to the proper value*/
          contact.g_slavenode[i]->history->pr_masters[0] = NULL;
          contact.g_slavenode[i]->history->pr_masters[1] = NULL;
          contact.g_slavenode[i]->history->pr_closest     = closestptr;
          contact.g_slavenode[i]->history->cr_g           = g;
          continue;                                           /* No contact----Continue with the next slave node*/
        }
	
	else{
	
	  contact.g_slavenode[i]->contactflag = contact_on;       /*contact flag is switched to the proper value*/
	  local_coordinate = 0.0;                                 /*see line 279-285*/
          element_nodes[0] = contact.g_slavenode[i]->node;
          element_nodes[1] = triple[1];
          element_nodes[2] = triple[2];
	  tangent = unit_v2;
	  contact.g_slavenode[i]->mymasters[0] = triple[1]->gnode;
	  contact.g_slavenode[i]->mymasters[1] = triple[2]->gnode;
	
   	  for(l=0; l<3; l++) cr_length_vec [l] = element_nodes[2]->x_cr[l] - element_nodes[1]->x_cr[l]; 
	  cr_length = sqrt(inner_pr(cr_length_vec, cr_length_vec));         
        }
      }
      
   }   
      else if(triple[0] == NULL){      /* Upper Corner */
     
      unit_v3[0] =   unit_v1[1]; /*outward normal is stored in unit_v3 and obtained by cross product of unit_v2 and e3*/
      unit_v3[1] = - unit_v1[0];
      unit_v3[2] =   0.0;     
      
      unit_v3[0] = unit_v3[0] / sqrt(inner_pr(unit_v3,unit_v3));
      unit_v3[1] = unit_v3[1] / sqrt(inner_pr(unit_v3,unit_v3));
	
      g = -1.0 * inner_pr(relative_pos, unit_v3);  /*value of gap function.*/
      t_n = contact.g_slavenode[i]->history->pr_multipliers[0] + pen_par * g;     
      
        if(t_n <= 0.0){   
          contact.g_slavenode[i]->contactflag = contact_off;         /* contact flag is switched to the proper value*/
          contact.g_slavenode[i]->history->pr_masters[0] = NULL;
          contact.g_slavenode[i]->history->pr_masters[1] = NULL;
          contact.g_slavenode[i]->history->pr_closest     = closestptr;
          contact.g_slavenode[i]->history->cr_g           = g;
          continue;                                               /* No contact----Continue with the next slave node*/
        }
      
        else{
        
	  contact.g_slavenode[i]->contactflag = contact_on;          /*contact flag is switched to the proper value*/
          element_nodes[0] = contact.g_slavenode[i]->node;           /*see line 279-285*/
          element_nodes[1] = triple[1];
          element_nodes[2] = triple[2];
          tangent = unit_v1;
          contact.g_slavenode[i]->mymasters[0] = triple[1]->gnode;
          contact.g_slavenode[i]->mymasters[1] = triple[2]->gnode;
	
          for(l=0; l<3; l++) cr_length_vec [l] = element_nodes[2]->x_cr[l] - element_nodes[1]->x_cr[l]; 
          cr_length = sqrt(inner_pr(cr_length_vec, cr_length_vec));  
        
	 local_coordinate = inner_pr(relative_pos,unit_v1) / cr_length;
	
	}
      }	
    
      else if(triple[2] == NULL){     /* Lower Corner */

      unit_v3[0] =   unit_v1[1]; /*outward normal is stored in unit_v3 and obtained by cross product of unit_v2 and e3*/
      unit_v3[1] = - unit_v1[0];
      unit_v3[2] =   0.0;     
      
      g = -1.0 * inner_pr(relative_pos, unit_v3);  /*value of gap function.*/     
      t_n = contact.g_slavenode[i]->history->pr_multipliers[0] + pen_par * g;
      
        if(t_n <= 0.0){   
          contact.g_slavenode[i]->contactflag = contact_off;          /* contact flag is switched to the proper value*/
          contact.g_slavenode[i]->history->pr_masters[0] = NULL;
          contact.g_slavenode[i]->history->pr_masters[1] = NULL;
          contact.g_slavenode[i]->history->pr_closest     = closestptr;
          contact.g_slavenode[i]->history->cr_g           = g;
          continue;                                                /* No contact----Continue with the next slave node*/
        }
	
	else{

	  contact.g_slavenode[i]->contactflag = contact_on;           /*contact flag is switched to the proper value*/          
          element_nodes[0] = contact.g_slavenode[i]->node;            /*see line 279 285*/
          element_nodes[1] = triple[0];
          element_nodes[2] = triple[1];
          tangent = unit_v1;
          contact.g_slavenode[i]->mymasters[0] = triple[0]->gnode;
          contact.g_slavenode[i]->mymasters[1] = triple[1]->gnode;
      
          for(l=0; l<3; l++) cr_length_vec [l] = element_nodes[2]->x_cr[l] - element_nodes[1]->x_cr[l]; 
          cr_length = sqrt(inner_pr(cr_length_vec, cr_length_vec));  
        	
	 local_coordinate = 1.0 - FABS(inner_pr(relative_pos,unit_v1) / cr_length);
	
	}    
      }
    
      /*Determination of the normal component of traction vector*/
    t_n = 0.0;
    t_n = contact.g_slavenode[i]->history->pr_multipliers[0] + pen_par * g;
    if(t_n < 0.0) t_n = 0.0;
   
   
    local_coordinate = 2.0 * local_coordinate - 1.0; 
    
    	
   /*Determination of the reference metric coefficient M11*/
    for(l=0; l<3; l++) rf_length_vec[l] = contact.g_slavenode[i]->mymasters[1]->node->x[l] - contact.g_slavenode[i]->mymasters[0]->node->x[l];
    rf_length = sqrt(inner_pr(rf_length_vec,rf_length_vec));					    
    M11 = 0.25*DSQR(rf_length);  
    
    if(fr_flag==1) {
      if(cet_flag==0)
      t_trial = contact.g_slavenode[i]->history->pr_t_tan + tan_pen_par * M11 *
                (local_coordinate - contact.g_slavenode[i]->history->pr_local_coord); 
      else if(cet_flag==1)
      t_trial = contact.g_slavenode[i]->history->pr_multipliers[1] + tan_pen_par * M11 *
                (local_coordinate - contact.g_slavenode[i]->history->pr_local_coord);
	      
      norm_t_trial = sqrt(t_trial*1.0/M11*t_trial);
   
      phi_trial = norm_t_trial - friction * t_n; 
    
      if(phi_trial <= 0.0) 
         t_tan = t_trial;        	                                                                                
      else{
        del_gama = phi_trial / tan_pen_par;
        t_tan = t_trial - tan_pen_par*del_gama*t_trial/norm_t_trial;       
          }	
    }		    	                                                      
    
    if(contact.g_slavenode[i]->history->pr_flag == contact_off)      
       contact.g_slavenode[i]->history->pr_local_coord = local_coordinate;   
             
    /*if slave node slides onto another element*/
    /*depending on the direction of motion of the slave node modify the previous local coordinate*/
    
    else if(contact.g_slavenode[i]->mymasters[0] == contact.g_slavenode[i]->history->pr_masters[1])
      contact.g_slavenode[i]->history->pr_local_coord = -1.0 - ( 1.0 - contact.g_slavenode[i]->history->pr_local_coord );
    
    else if(contact.g_slavenode[i]->mymasters[1] == contact.g_slavenode[i]->history->pr_masters[0])    
      contact.g_slavenode[i]->history->pr_local_coord =  1.0 + ( 1.0 + contact.g_slavenode[i]->history->pr_local_coord );
   
   
   
     contact.g_slavenode[i]->history->pr_masters[0]  = contact.g_slavenode[i]->mymasters[0]; /*Update the history variables of the slave node*/
     contact.g_slavenode[i]->history->pr_masters[1]  = contact.g_slavenode[i]->mymasters[1]; 
     contact.g_slavenode[i]->history->pr_closest     = closestptr;
     contact.g_slavenode[i]->history->cr_local_coord = local_coordinate; 
     contact.g_slavenode[i]->history->R_Metric = M11;
     contact.g_slavenode[i]->history->cr_g     = g;
     contact.g_slavenode[i]->history->cr_tan   = t_tan;
     contact.g_slavenode[i]->history->cr_force = t_n; 
 

 } /* end of loop over slavenodes */

  
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} 
/* end of wall_contact_update*/
  
  
/*! @} (documentation module close)*/
#endif
