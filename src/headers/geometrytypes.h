/*----------------------------------------------------------------------*
 | type definitions of geometry based information         m.gee 8/00    |
 *----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 | 1 NODE                                                 m.gee 8/00    |
 *----------------------------------------------------------------------*/
typedef struct _NODE
{
     INT                        Id;            /* global Id (numbering starts with 0)*/              
     INT                        Id_loc;        /* field-local Id  (numbering starts with 0)*/
     INT                        proc;          /* my owner intra-proc */

     DOUBLE                     x[3];          /* my coordinates */

     struct _ARRAY              sol;           /* my solution history */
     struct _ARRAY              sol_increment; /* my incremental solution */
     struct _ARRAY              sol_residual ; /* my residual solution */
     struct _ARRAY              sol_mf;        /* my multifield coupling values */
     INT                        numdf;         /* my number of degrees of freedom */
     INT                       *dof;           /* my dof-numbers  */

     INT                        numele;        /* number of elements to me */
     struct _ELEMENT          **element;       /* ptrs to elements to me */

     struct _GNODE             *gnode;         /* ptr to my gnode */
#ifdef D_FLUID   
     struct _FLUID_VARIA       *fluid_varia;   /* ptr to my fluid_varia */
#endif
} NODE;


/*----------------------------------------------------------------------*
 | 1 ELEMENT                                              m.gee 8/00    |
 *----------------------------------------------------------------------*/
typedef struct _ELEMENT
{  
     INT                        Id;             /* global Id (numbering starts with 0)*/                          
     INT                        Id_loc;         /* field-local Id (numbering starts with 0)*/
     INT                        proc;           /* my owner intra-proc */

     INT                        numnp;          /* number of nodes to me */
     INT                       *lm;             /* only used for reading from input (this will be eliminated)*/
     struct _NODE             **node;           /* ptrs to my nodes */

     INT                        mat;            /* number of material law associated with me */

     enum _ELEMENT_TYP          eltyp;          /* my element type */
     enum _DIS_TYP              distyp;         /* my actual discretization type */
                                                 
     union                                      /* union pointer to elementformulation */
     {
     struct _SHELL9     *s9;                    /* shell9 element */
     struct _SHELL8     *s8;                    /* shell8 element */
     struct _BRICK1     *c1;                    /* structural volume element */
     struct _WALL1      *w1;                    /* 2D plane stress - plane strain element */
     struct _FLUID2     *f2;                    /* 2D fluid element */
     struct _FLUID2_PRO *f2pro;                 /* 2D fluid element projection method */
     struct _FLUID2_TU  *f2_tu;                 /* 2D fluid element for turbulence*/
     struct _FLUID3     *f3;                    /* 3D fluid element */
     struct _ALE2       *ale2;                  /* pseudo structural 2D ale element */
     struct _ALE3       *ale3;                  /* pseudo structural 3D ale element */
     }                          e;              /* name of union */ 

     union
     {
     struct _GSURF    *gsurf;                   /* my gsurf, if I am a 2D element */
     struct _GVOL     *gvol;                    /* my gvol,  if I am a 3D element */
     }                          g;              /* name of union */           
/*----------------------------------------------------------------------*/
#ifdef D_OPTIM                   /* include optimization code to ccarat */
/*----------------------------------------------------------------------*/
     INT    *optdata;               /* optimization variable number ... */
     DOUBLE mylinweight;    /*element weighting for material linking AS */  
/*----------------------------------------------------------------------*/
#endif                   /* stop including optimization code to ccarat :*/
/*----------------------------------------------------------------------*/



} ELEMENT;



/*----------------------------------------------------------------------*
 | 1 GNODE                                                 m.gee 3/02   |
 *----------------------------------------------------------------------*/
typedef struct _GNODE
{
#ifdef DEBUG 
     INT                           Id;          /* for debugging only, do not use in code !*/
#endif
   /*------------fe topology section */
     struct _NODE                 *node;        /* pointer to my node */
     INT                           ngline;      /* number of GLINEs to me */
     struct _GLINE               **gline;       /* pointers to the GLINEs to me */
   /*------- design topology section */
     enum
     {
        ondnothing,                             /* I am NOT positioned on any design object (error!) */
        ondnode,                                /* I am on a DNODE */
        ondline,                                /* I am on a DLINE */
        ondsurf,                                /* I am on a DSURF */
        ondvol                                  /* I am inside a DVOL */
     }                             ondesigntyp; 
     union
     {
        struct _DNODE      *dnode;              
        struct _DLINE      *dline;
        struct _DSURF      *dsurf;
        struct _DVOL       *dvol;
     }                             d;           /* ptr to the design object I am positioned on */
   /* boundary and coupling conditions */
     struct _DIRICH_CONDITION     *dirich;      /* a dirichlet condition on this gnode, else NULL */
     struct _COUPLE_CONDITION     *couple;      /* a coupling conditions on this gnode, else NULL */
     struct _NEUM_CONDITION       *neum;        /* a neumann condition on this gnode, else NULL */
#ifdef D_FSI
     struct _FSI_COUPLE_CONDITION *fsicouple;
     struct _FLUID_FREESURF_CONDITION *freesurf;
     struct _NODE                **mfcpnode;    /* ptrs to multi-field coupling nodes */
#endif
} GNODE;


/*----------------------------------------------------------------------*
 | 1 GLINE                                                 m.gee 3/02   |
 *----------------------------------------------------------------------*/
typedef struct _GLINE
{
#ifdef DEBUG 
     INT                        Id;             /* for debugging only, do not use in code !*/
#endif
     INT                        proc;           /* my owner intra-proc */
   /*------------fe topology section */
     INT                        ngnode;         /* number of gnodes on me */
     struct _GNODE            **gnode;          /* vector of ptrs to these gnodes */

     INT                        ngsurf;         /* number of gsurfs to me */
     struct _GSURF            **gsurf;          /* vector of ptrs to these gsurfs */

   /*------- design topology section */
     struct _DLINE             *dline;          /* The DLINE I am on, else NULL */

   /*----------- boundary conditions */
     struct _NEUM_CONDITION       *neum;        /* neumann conditions to this GLINE, else NULL */
#ifdef D_FSI
     struct _FSI_COUPLE_CONDITION *fsicouple;
     struct _FLUID_FREESURF_CONDITION *freesurf;
#endif
} GLINE;
/*----------------------------------------------------------------------*
 | 1 GSURF                                                 m.gee 3/02   |
 *----------------------------------------------------------------------*/
typedef struct _GSURF
{
#ifdef DEBUG 
     INT                        Id;             /* for debugging only, do not use in code !*/
#endif
   /*------------fe topology section */
     struct _ELEMENT           *element;        /* ptr to my ELEMENT, if I am a 2D element, else NULL */

     INT                        ngnode;         /* number of GNODEs to me */
     struct _GNODE            **gnode;          /* ptrs to these GNODEs */

     INT                        ngline;         /* number of GLINEs to me */
     struct _GLINE            **gline;          /* ptrs to these GLINEs */

     INT                        ngvol;          /* number of GVOLs to me */
     struct _GVOL             **gvol;           /* ptrs to these GVOLs, else NULL */

   /*------- design topology section */
     struct _DSURF             *dsurf;          /* DSURF I am on, else NULL */

   /*----------- boundary conditions */
     struct _NEUM_CONDITION       *neum;        /* neumann conditions to this GSURF, else NULL */
} GSURF;
/*----------------------------------------------------------------------*
 | 1 GVOL                                                  m.gee 3/02   |
 *----------------------------------------------------------------------*/
typedef struct _GVOL
{
#ifdef DEBUG 
     INT                        Id;             /* for debugging only, do not use in code !*/
#endif
   /*------------fe topology section */
     struct _ELEMENT           *element;        /* ptr to my ELEMENT */

     INT                        ngline;         /* number of GLINEs to me */
     struct _GLINE            **gline;          /* ptrs to these GLINEs */

     INT                        ngsurf;         /* number of GSURFs to me */
     struct _GSURF            **gsurf;          /* ptrs to these GSURFs */

   /*------- design topology section */
     struct _DVOL              *dvol;           /* the DVOL I am placed in */

   /*----------- boundary conditions */
     struct _NEUM_CONDITION       *neum;        /* neumann conditions to this GVOL, else NULL */
} GVOL;




