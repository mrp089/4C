/*----------------------------------------------------------------------*
 | neumann condition                                      m.gee 3/02    |
 |                                                                      |
 | this structure holds a neumann condition                             |
 | depend on number of dofs to a fe-node and type of element it is      |
 | is connected to, the arrays can be defined in several styles         |
 *----------------------------------------------------------------------*/
typedef struct _NEUM_CONDITION
{
     int                       curve;        /* number of load curve associated with this conditions */        

     struct _ARRAY             neum_onoff;   /* array of on-off flags */
     struct _ARRAY             neum_val;     /* values of this condition */    
     
     enum
     {
     neum_none,
     neum_live,
     neum_dead,
     neum_FSI,
     pres_domain_load,
     neum_consthydro_z,
     neum_increhydro_z
     }                         neum_type;

     enum
     {
     mid,  /*nsurf=1*/
     top,  /*nsurf=2*/
     bot   /*nsurf=3*/
     }                         neum_surf;    /* load applied on top, bottom or middle surface of shell element */

} NEUM_CONDITION;
/*----------------------------------------------------------------------*
 | dirichlet condition                                    m.gee 3/02    |
 |                                                                      |
 | this structure holds a dirichlet condition                           |
 | depend on number of dofs to a fe-node and type of element it is      |
 | is connected to, the arrays can be defined in several styles         |
 *----------------------------------------------------------------------*/
typedef struct _DIRICH_CONDITION
{
     struct _ARRAY             curve;

     struct _ARRAY             dirich_onoff; /* array of on-off flags */
     struct _ARRAY             dirich_val;   /* values of this condition */    
     enum
     {
     dirich_none,
     dirich_FSI,
     dirich_freesurf
     }                         dirich_type;                       

} DIRICH_CONDITION;
/*----------------------------------------------------------------------*
 | coupling condition                                     m.gee 3/02    |
 |                                                                      |
 | this structure is assigned to nodes, which are coupled in some or    |
 | all of their dofs                                                    |
 *----------------------------------------------------------------------*/
typedef struct _COUPLE_CONDITION
{
     enum _FIELDTYP            fieldtyp;        /* type of field this structure is in */
     struct _ARRAY             couple;          /* array of the coupling conditions */
} COUPLE_CONDITION;
/*----------------------------------------------------------------------*
 | fsi coupling condition                                 genk 10/02    |
 |                                                                      |
 *----------------------------------------------------------------------*/
typedef struct _FSI_COUPLE_CONDITION
{
     enum _FIELDTYP            fieldtyp;        /* type of field this structure is in */
     int                       fsi_coupleId;
     enum _FSI_MESH            fsi_mesh;
     
} FSI_COUPLE_CONDITION;
/*----------------------------------------------------------------------*
 | fluid freesurface condition                            genk 10/02    |
 |                                                                      |        
 *----------------------------------------------------------------------*/
typedef struct _FLUID_FREESURF_CONDITION
{
     enum _FIELDTYP            fieldtyp;        /* type of field this structure is in */
     struct _ARRAY             fixed_onoff;     /* flags for local slippage model */     
} FLUID_FREESURF_CONDITION;
