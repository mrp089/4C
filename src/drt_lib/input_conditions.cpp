/*!----------------------------------------------------------------------
\file
\brief

<pre>
Maintainer: Michael Gee
            gee@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15239
</pre>

*----------------------------------------------------------------------*/
#ifdef CCADISCRET

#include <ctime>
#include <cstdlib>
#include <iostream>

#ifdef PARALLEL
#include <mpi.h>
#endif

#include "Epetra_SerialDenseMatrix.h"
#include "global_inp_control2.H"
#include "drt_timecurve.H"
#include "drt_function.H"


/*! number of values that we can read for a neumann or dirichlet condition
 * -> GiD has 6 boxes to insert values
 */
const int numreadvalue = 6;

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

/*----------------------------------------------------------------------*
 |                                                       m.gee 06/01    |
 | structure of flags to control output                                 |
 | defined in out_global.c                                              |
 *----------------------------------------------------------------------*/
extern struct _IO_FLAGS     ioflags;

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
 | Standard Neumann & Dirichlet                             gee 02/07   |
 *----------------------------------------------------------------------*/
static void input_point_dirich(multimap<int,RefCountPtr<DRT::Condition> >& pdmap, bool ale=false);
static void input_line_dirich(multimap<int,RefCountPtr<DRT::Condition> >& ldmap, bool ale=false);
static void input_surf_dirich(multimap<int,RefCountPtr<DRT::Condition> >& sdmap, bool ale=false);
static void input_vol_dirich(multimap<int,RefCountPtr<DRT::Condition> >& vdmapx);

static void input_point_neum(multimap<int,RefCountPtr<DRT::Condition> >& pnmap);
static void input_line_neum(multimap<int,RefCountPtr<DRT::Condition> >& lnmap);
static void input_surf_neum(multimap<int,RefCountPtr<DRT::Condition> >& snmap);
static void input_vol_neum(multimap<int,RefCountPtr<DRT::Condition> >& vnmap);

/*----------------------------------------------------------------------*
 | Contact                                                  gee 10/07   |
 *----------------------------------------------------------------------*/
static void input_line_contact(multimap<int,RefCountPtr<DRT::Condition> >& ldmap);
static void input_surf_contact(multimap<int,RefCountPtr<DRT::Condition> >& sdmap);

/*----------------------------------------------------------------------*
 | periodic boundary conditions                           gammi 05/07   |
 *----------------------------------------------------------------------*/
static void input_line_periodic(multimap<int,RefCountPtr<DRT::Condition> >& lpbcmap);
static void input_surf_periodic(multimap<int,RefCountPtr<DRT::Condition> >& spbcmap);


/*----------------------------------------------------------------------*
 | fsi coupling conditions                                u.kue 05/07   |
 *----------------------------------------------------------------------*/
static void input_line_fsi_coupling(multimap<int,RefCountPtr<DRT::Condition> >& lfsicoupmap);
static void input_surf_fsi_coupling(multimap<int,RefCountPtr<DRT::Condition> >& sfsicoupmap);


/*----------------------------------------------------------------------*
 | xfem coupling                                                  u.may 05/07   |
 *----------------------------------------------------------------------*/
static void input_line_xfem_coupling(multimap<int,RefCountPtr<DRT::Condition> >& lxfemcoupmap);
static void input_surf_xfem_coupling(multimap<int,RefCountPtr<DRT::Condition> >& sxfemcoupmap);


static void setup_condition(const multimap<int,RefCountPtr<DRT::Condition> >& cond,
                            const vector<vector<int> >& fenode);
static void add_nodeids_to_condition(const int id, RefCountPtr<DRT::Condition> cond,
                                     const vector<vector<int> > d_fenode);
static void register_condition(string name,
                               string description,
                               const multimap<int,RefCountPtr<DRT::Condition> >& cond,
                               RefCountPtr<DRT::Discretization> actdis,
                               const Epetra_Map* noderowmap);

/*----------------------------------------------------------------------*
 | surface stress conditions                                 lw 06/07   |
 *----------------------------------------------------------------------*/
static void input_surf_stress(multimap<int,RefCountPtr<DRT::Condition> >& ssmap);

/*----------------------------------------------------------------------*
 | microscale boundary                                       lw 06/07   |
 *----------------------------------------------------------------------*/
static void input_micro_bc(multimap<int,RefCountPtr<DRT::Condition> >& mbcmap);

/*----------------------------------------------------------------------*
 | surface stress calculation for fluid                   g.bau 07/07   |
 *----------------------------------------------------------------------*/
static void input_line_stress_calc(multimap<int,RefCountPtr<DRT::Condition> >& lnmap);
static void input_surf_stress_calc(multimap<int,RefCountPtr<DRT::Condition> >& snmap);

/*----------------------------------------------------------------------*
 | lift and drag calculation from fluid                  m.levy 09/07   |
 *----------------------------------------------------------------------*/
static void input_line_LIFTDRAG(multimap<int,RefCountPtr<DRT::Condition> >& lldmap);
static void input_surf_LIFTDRAG(multimap<int,RefCountPtr<DRT::Condition> >& sldmap);

/*----------------------------------------------------------------------*
 | boundary surface for volume constraint                    tk 10/07   |
 *----------------------------------------------------------------------*/
static void input_surf_volconstr(multimap<int,RefCountPtr<DRT::Condition> >& vcbcmap);

/*----------------------------------------------------------------------*
 | input of conditions                                    m.gee 11/06   |
 *----------------------------------------------------------------------*/
void DRT::Problem::ReadConditions()
{
  /*---------------------------------------------- input of time curves */
  DRT::UTILS::TimeCurveManager::Instance().ReadInput();
  /*---------------------------------------- input of spatial functions */
  DRT::UTILS::FunctionManager::Instance().ReadInput();
  //------------------------------- read number of design objects we have
  // this currently serves to determine how many node sets we might have
  const Teuchos::ParameterList& design = DesignDescriptionParams();
  int ndnode = design.get<int>("NDPOINT");
  int ndline = design.get<int>("NDLINE");
  int ndsurf = design.get<int>("NDSURF");
  int ndvol  = design.get<int>("NDVOL");

  frrewind();
  //--------------------------------------------- read generic node sets
  // read design nodes <-> nodes
  vector<int> ndnode_fenode(ndnode);
  vector<vector<int> > dnode_fenode(ndnode);
  for (int i=0; i<ndnode; ++i)
    ndnode_fenode[i] = 0;
  input_design_dpoint_fenode_read(dnode_fenode,ndnode_fenode);

  // read design lines <-> nodes
  vector<int> ndline_fenode(ndline);
  vector<vector<int> > dline_fenode(ndline);
  for (int i=0; i<ndline; ++i)
    ndline_fenode[i] = 0;
  input_design_dline_fenode_read(dline_fenode,ndline_fenode);

  // read design surfaces <-> nodes
  vector<int> ndsurf_fenode(ndsurf);
  vector<vector<int> > dsurf_fenode(ndsurf);
  for (int i=0; i<ndsurf; ++i)
    ndsurf_fenode[i] = 0;
  input_design_dsurf_fenode_read(dsurf_fenode,ndsurf_fenode);

  // read design volumes <-> nodes
  vector<int> ndvol_fenode(ndvol);
  vector<vector<int> > dvol_fenode(ndvol);
  for (int i=0; i<ndvol; ++i)
    ndvol_fenode[i] = 0;
  input_design_dvol_fenode_read(dvol_fenode,ndvol_fenode);

  //-------------------------------------read point dirichlet conditions
  multimap<int,RefCountPtr<DRT::Condition> > pointdirich;
  input_point_dirich(pointdirich);
  setup_condition(pointdirich, dnode_fenode);
  //-------------------------------------read line dirichlet conditions
  multimap<int,RefCountPtr<DRT::Condition> > linedirich;
  input_line_dirich(linedirich);
  setup_condition(linedirich, dline_fenode);
  //-------------------------------------read surface dirichlet conditions
  multimap<int,RefCountPtr<DRT::Condition> > surfdirich;
  input_surf_dirich(surfdirich);
  setup_condition(surfdirich, dsurf_fenode);
  //-------------------------------------read volume dirichlet conditions
  multimap<int,RefCountPtr<DRT::Condition> > voldirich;
  input_vol_dirich(voldirich);
  setup_condition(voldirich, dvol_fenode);

  //--------------------------------------- read point neumann conditions
  multimap<int,RefCountPtr<DRT::Condition> > pointneum;
  input_point_neum(pointneum);
  setup_condition(pointneum, dnode_fenode);
  //--------------------------------------- read line neumann conditions
  multimap<int,RefCountPtr<DRT::Condition> > lineneum;
  input_line_neum(lineneum);
  setup_condition(lineneum, dline_fenode);
  //--------------------------------------- read surface neumann conditions
  multimap<int,RefCountPtr<DRT::Condition> > surfneum;
  input_surf_neum(surfneum);
  setup_condition(surfneum, dsurf_fenode);
  //--------------------------------------- read vol neumann conditions
  multimap<int,RefCountPtr<DRT::Condition> > volneum;
  input_vol_neum(volneum);
  setup_condition(volneum, dvol_fenode);

  //-------------------------------------read line contact conditions
  multimap<int,RefCountPtr<DRT::Condition> > linecontact;
  input_line_contact(linecontact);
  setup_condition(linecontact, dline_fenode);
  //-------------------------------------read surface contact conditions
  multimap<int,RefCountPtr<DRT::Condition> > surfcontact;
  input_surf_contact(surfcontact);
  setup_condition(surfcontact, dsurf_fenode);

  //-------------------------------------read point aledirichlet conditions
  multimap<int,RefCountPtr<DRT::Condition> > pointaledirich;
  input_point_dirich(pointaledirich,true);
  setup_condition(pointaledirich, dnode_fenode);
  //-------------------------------------read line aledirichlet conditions
  multimap<int,RefCountPtr<DRT::Condition> > linealedirich;
  input_line_dirich(linealedirich,true);
  setup_condition(linealedirich, dline_fenode);
  //-------------------------------------read surface aledirichlet conditions
  multimap<int,RefCountPtr<DRT::Condition> > surfaledirich;
  input_surf_dirich(surfaledirich,true);
  setup_condition(surfaledirich, dsurf_fenode);

  //------------------------------------------- read line periodic condition
  multimap<int,RefCountPtr<DRT::Condition> > linepbc;
  input_line_periodic(linepbc);
  setup_condition(linepbc, dline_fenode);
  //---------------------------------------- read surface periodic condition
  multimap<int,RefCountPtr<DRT::Condition> > surfpbc;
  input_surf_periodic(surfpbc);
  setup_condition(surfpbc, dsurf_fenode);

  //--------------------------------------- read line fsi coupling condition
  multimap<int,RefCountPtr<DRT::Condition> > linefsicoup;
  input_line_fsi_coupling(linefsicoup);
  setup_condition(linefsicoup, dline_fenode);
  //------------------------------------ read surface fsi coupling condition
  multimap<int,RefCountPtr<DRT::Condition> > surffsicoup;
  input_surf_fsi_coupling(surffsicoup);
  setup_condition(surffsicoup, dsurf_fenode);
  //--------------------------------------- read line xfem coupling condition
  multimap<int,RefCountPtr<DRT::Condition> > linexfemcoup;
  input_line_xfem_coupling(linexfemcoup);
  setup_condition(linexfemcoup, dline_fenode);
  //------------------------------------ read surface xfem coupling condition
  multimap<int,RefCountPtr<DRT::Condition> > surfxfemcoup;
  input_surf_xfem_coupling(surfxfemcoup);
  setup_condition(surfxfemcoup, dsurf_fenode);

  //--------------------------------------- read surface stress conditions
  multimap<int,RefCountPtr<DRT::Condition> > surfstress;
  input_surf_stress(surfstress);
  setup_condition(surfstress, dsurf_fenode);

  //------------read microscale boundary conditions for multiscale analyses
  multimap<int,RefCountPtr<DRT::Condition> > microbc;
  input_micro_bc(microbc);
  setup_condition(microbc, dsurf_fenode);

  //--------------------- read line conditions for fluid stress calculation
  multimap<int,RefCountPtr<DRT::Condition> > linefluidstresscalc;
  input_line_stress_calc(linefluidstresscalc);
  setup_condition(linefluidstresscalc, dline_fenode);
  //------------------ read surface conditions for fluid stress calculation
  multimap<int,RefCountPtr<DRT::Condition> > surffluidstresscalc;
  input_surf_stress_calc(surffluidstresscalc);
  setup_condition(surffluidstresscalc, dsurf_fenode);

  //-------------------------------- read line LIFTDRAG conditions
  multimap<int,RefCountPtr<DRT::Condition> > lineLIFTDRAG;
  input_line_LIFTDRAG(lineLIFTDRAG);
  setup_condition(lineLIFTDRAG, dline_fenode);
  //--------------------------------read surface LIFTDRAG conditions
  multimap<int,RefCountPtr<DRT::Condition> > surfLIFTDRAG;
  input_surf_LIFTDRAG(surfLIFTDRAG);
  setup_condition(surfLIFTDRAG, dsurf_fenode);

  //--------------------- read surf conditions for volume constraint
  multimap<int,RefCountPtr<DRT::Condition> > surfvolconstr;
  input_surf_volconstr(surfvolconstr);
  setup_condition(surfvolconstr, dsurf_fenode);


  // Iterate through all discretizations and sort the appropiate condition into
  // the correct discretization it applies to
  for (unsigned i=0; i<NumFields(); ++i)
  {
    for (unsigned j=0; j<NumDis(i); ++j)
    {
      RefCountPtr<DRT::Discretization> actdis = Dis(i,j);
      const Epetra_Map* noderowmap = actdis->NodeRowMap();

      register_condition("Dirichlet", "Point Dirichlet", pointdirich, actdis, noderowmap);
      register_condition("Dirichlet", "Line Dirichlet", linedirich, actdis, noderowmap);
      register_condition("Dirichlet", "Surface Dirichlet", surfdirich, actdis, noderowmap);
      register_condition("Dirichlet", "Volume Dirichlet", voldirich, actdis, noderowmap);

      register_condition("PointNeumann", "Point Neumann", pointneum, actdis, noderowmap);
      register_condition("LineNeumann", "Line Neumann", lineneum, actdis, noderowmap);
      register_condition("SurfaceNeumann", "Surface Neumann", surfneum, actdis, noderowmap);
      register_condition("VolumeNeumann", "Volume Neumann", volneum, actdis, noderowmap);

      register_condition("Contact", "Line Contact", linecontact, actdis, noderowmap);
      register_condition("Contact", "Surface Contact", surfcontact, actdis, noderowmap);

      register_condition("ALEDirichlet", "Point Dirichlet", pointaledirich, actdis, noderowmap);
      register_condition("ALEDirichlet", "Line Dirichlet", linealedirich, actdis, noderowmap);
      register_condition("ALEDirichlet", "Surface Dirichlet", surfaledirich, actdis, noderowmap);

      register_condition("LinePeriodic", "Line periodic", linepbc, actdis, noderowmap);
      register_condition("SurfacePeriodic", "Surface periodic", surfpbc, actdis, noderowmap);

      register_condition("FSICoupling", "FSI Coupling", linefsicoup, actdis, noderowmap);
      register_condition("FSICoupling", "FSI Coupling", surffsicoup, actdis, noderowmap);

      register_condition("XFEMCoupling", "XFEM Coupling", linexfemcoup, actdis, noderowmap);
      register_condition("XFEMCoupling", "XFEM Coupling", surfxfemcoup, actdis, noderowmap);

      register_condition("SurfaceStress", "Surface Stress", surfstress, actdis, noderowmap);
      register_condition("MicroBoundary", "Microscale Boundary", microbc, actdis, noderowmap);

      register_condition("FluidStressCalc", "Line Fluid Stress Calculation", linefluidstresscalc, actdis, noderowmap);
      register_condition("FluidStressCalc", "Surf Fluid Stress Calculation", surffluidstresscalc, actdis, noderowmap);

      register_condition("LIFTDRAG", "Line LIFTDRAG", lineLIFTDRAG, actdis, noderowmap);
      register_condition("LIFTDRAG", "Surf LIFTDRAG", surfLIFTDRAG, actdis, noderowmap);

      register_condition("VolumeConstraint_3D","Surface Volume Constraint",surfvolconstr,actdis, noderowmap);
    }
  }
}


/*----------------------------------------------------------------------*
 | setup condition                                        u.kue 05/07   |
 *----------------------------------------------------------------------*/
void setup_condition(const multimap<int,RefCountPtr<DRT::Condition> >& cond,
                     const vector<vector<int> >& fenode)
{
  multimap<int,RefCountPtr<DRT::Condition> >::const_iterator curr;
  // iterate through conditions and add fe nodes
  for (curr=cond.begin(); curr!=cond.end(); ++curr)
    add_nodeids_to_condition(curr->first,curr->second,fenode);
}


/*----------------------------------------------------------------------*
 | add node ids to a condition                            m.gee 01/07   |
 *----------------------------------------------------------------------*/
void add_nodeids_to_condition(const int id, RefCountPtr<DRT::Condition> cond,
                              const vector<vector<int> > d_fenode)
{
  // vector of finite element node ids in this node set
  const vector<int>& nodes = d_fenode[id];

  // add the list of nodal ids to the condition
  cond->Add("Node Ids",nodes);
  return;
} // add_nodeids_to_condition


/*----------------------------------------------------------------------*
 | add condition to discretization                        u.kue 05/07   |
 *----------------------------------------------------------------------*/
void register_condition(string name,
                        string description,
                        const multimap<int,RefCountPtr<DRT::Condition> >& cond,
                        RefCountPtr<DRT::Discretization> actdis,
                        const Epetra_Map* noderowmap)
{
  multimap<int,RefCountPtr<DRT::Condition> >::const_iterator curr;
  for (curr=cond.begin(); curr!=cond.end(); ++curr)
  {
    const vector<int>* nodes = curr->second->Get<vector<int> >("Node Ids");
    if (!(int)nodes->size()) dserror("%s condition %d has no nodal cloud",
                                     description.c_str(),
                                     curr->second->Id());
    const int firstnode = (*nodes)[0];
    int foundit = 0;
    if (noderowmap->MyGID(firstnode)) foundit = 1;
    int found=0;
    noderowmap->Comm().SumAll(&foundit,&found,1);
    if (found)
      actdis->SetCondition(name,curr->second);
  }
}


/*----------------------------------------------------------------------*
 | input of design node neumann conditions                m.gee 01/07   |
 *----------------------------------------------------------------------*/
void input_point_neum(multimap<int,RefCountPtr<DRT::Condition> >& pnmap)
{
  // currently, we always have 6 values read from file
  // this might change at some point
  const int numread = 6;

  /*-------------------- find the beginning of nodal dirichlet conditions */
  if (frfind("--DESIGN POINT NEUMANN CONDITIONS")==0) return;
  frread();

  /*------------------------ read number of design points with conditions */
  int ierr=0;
  int ndnode=0;
  frint("DPOINT",&ndnode,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-nodal neumann conditions");
  frread();

  /*-------------------------------------- start reading the design nodes */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design node Id */
    int dnodeid = -1;
    frint("E",&dnodeid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-nodal neumann conditions");
    dnodeid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-nodal neumann conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    vector<int>    neum_onoff(numreadvalue);
    vector<double> neum_val(numreadvalue);
    vector<int>    neum_funct(numreadvalue);
    for (int i=0; i<numreadvalue; ++i)
    {
      neum_onoff[i] = 0;
      neum_val[i]   = 0.0;
      neum_funct[i] = 0;
    }

    //---------------------------------- read the curve number of "none"
    int curve=0;
    char buffer[200];
    ierr=sscanf(colptr," %s ",buffer);
    if(ierr!=1)
      dserror("Cannot read design-nodal neumann conditions");
    if (strncmp(buffer,"none",4)==0)
    {
       curve = -1;
       colptr = strstr(allfiles.actplace,"none");
       if(colptr==NULL)
         dserror("Cannot read design-nodal neumann conditions");
       colptr += 4;
    }
    else
    {
       ierr=sscanf(colptr," %d ",&curve);
       curve--;
       if(ierr!=1)
       dserror("Cannot read design-nodal neumann conditions");
       colptr = strpbrk(colptr,"1234567890");
       colptr++;
    }


    /* NOTE: number of read values = 6  does not need to be */
    /*       equivalent to the numreadvalue -> e.g. for shell9! sh 12/02 */
    // read on/off toggles
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        neum_onoff[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // read values
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        neum_val[i] = strtod(colptr,&colptr);
      else
        strtod(colptr,&colptr);

     // read function numbers
     for (int i=0; i<numread; ++i)
      if (i < numreadvalue)
	neum_funct[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
             rcp(new DRT::Condition(dnodeid,DRT::Condition::PointNeumann,false,
                                    DRT::Condition::Point));

    // read whether load is on surface (shells)
    frchk("Mid",&ierr);
    if (ierr) condition->Add("surface","mid");
    frchk("Top",&ierr);
    if (ierr) condition->Add("surface","top");
    frchk("Bot",&ierr);
    if (ierr) condition->Add("surface","bot");

    // add stuff to boundary condition
    condition->Add("onoff",neum_onoff);
    condition->Add("val",neum_val);
    condition->Add("curve",&curve,1);
    condition->Add("funct",neum_funct);

    //------------------------------- put condition in map of conditions
    pnmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dnodeid,condition));
    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_point_neum

/*----------------------------------------------------------------------*
 | input of design line neumann conditions                m.gee 11/06   |
 *----------------------------------------------------------------------*/
void input_line_neum(multimap<int,RefCountPtr<DRT::Condition> >& lnmap)
{
  // currently, we always have 6 values read from file
  // this might change at some point
  const int numread = 6;

  /*-------------------- find the beginning of line dirichlet conditions */
  if (frfind("--DESIGN LINE NEUMANN CONDITIONS")==0) return;
  frread();

  /*------------------------ read number of design lines with conditions */
  int ierr=0;
  int ndline=0;
  frint("DLINE",&ndline,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-line neumann conditions");
  frread();

  /*------------------------------------- start reading the design lines */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design line Id */
    int dlineid = -1;
    frint("E",&dlineid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-line neumann conditions");
    dlineid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-line neumann conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    vector<int>    neum_onoff(numreadvalue);
    vector<double> neum_val(numreadvalue);
    vector<int>    neum_funct(numreadvalue);

    for (int i=0; i<numreadvalue; ++i)
    {
      neum_onoff[i] = 0;
      neum_val[i]   = 0.0;
      neum_funct[i] = 0;
    }

    //---------------------------------- read the curve number of "none"
    int curve=0;
    char buffer[200];
    ierr=sscanf(colptr," %s ",buffer);
    if(ierr!=1)
      dserror("Cannot read design-line neumann conditions");
    if (strncmp(buffer,"none",4)==0)
    {
       curve = -1;
       colptr = strstr(allfiles.actplace,"none");
       if(colptr==NULL)
         dserror("Cannot read design-line neumann conditions");
       colptr += 4;
    }
    else
    {
       ierr=sscanf(colptr," %d ",&curve);
       curve--;
       if(ierr!=1)
         dserror("Cannot read design-line neumann conditions");
       colptr = strpbrk(colptr,"1234567890");
       colptr++;
    }


    /* NOTE: number of read values = 6  does not need to be */
    /*       equivalent to the numreadvalue -> e.g. for shell9! sh 12/02 */
    // read on/off toggles
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        neum_onoff[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // read values
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        neum_val[i] = strtod(colptr,&colptr);
      else
        strtod(colptr,&colptr);

    // read function numbers
     for (int i=0; i<numread; ++i)
      if (i < numreadvalue)
	neum_funct[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
              rcp(new DRT::Condition(dlineid,DRT::Condition::LineNeumann,true,
                                     DRT::Condition::Line));

    // read whether load is on surface (shells)
    condition->Add("type","neum_live");
    frchk("Live",&ierr);
    if (ierr) condition->Add("type","neum_live");
    frchk("Dead",&ierr);
    if (ierr) condition->Add("type","neum_dead");
    frchk("PrescribedDomainLoad",&ierr);
    if (ierr) condition->Add("type","pres_domain_load");
    frchk("constHydro_z",&ierr);
    if (ierr) condition->Add("type","neum_consthydro_z");
    frchk("increaseHydro_z",&ierr);
    if (ierr) condition->Add("type","neum_increhydro_z");
    frchk("orthopressure",&ierr);
    if (ierr) condition->Add("type","neum_orthopressure");
    frchk("LAS",&ierr);
    if (ierr) condition->Add("type","neum_LAS");
    /*----------- read if load is applied on surface -> shell elements */
    frchk("Mid",&ierr);
    if (ierr) condition->Add("surface","mid");
    frchk("Top",&ierr);
    if (ierr) condition->Add("surface","top");
    frchk("Bot",&ierr);
    if (ierr) condition->Add("surface","bot");

    // add stuff to boundary condition
    condition->Add("onoff",neum_onoff);
    condition->Add("val",neum_val);
    condition->Add("curve",&curve,1);
    condition->Add("funct",neum_funct);

    //------------------------------- put condition in map of conditions
    lnmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dlineid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_line_neum




/*----------------------------------------------------------------------*
 | input of design surface neumann conditions             m.gee 11/06   |
 *----------------------------------------------------------------------*/
void input_surf_neum(multimap<int,RefCountPtr<DRT::Condition> >& snmap)
{
  // currently, we always have 6 values read from file
  // this might change at some point
  const int numread = 6;

  /*----------------- find the beginning of surface dirichlet conditions */
  if (frfind("--DESIGN SURF NEUMANN CONDITIONS")==0) return;
  frread();

  /*------------------------ read number of design surfs with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-surface neumann conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-surface neumann conditions");
    dsurfid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-surface neumann conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    vector<int>    neum_onoff(numreadvalue);
    vector<double> neum_val(numreadvalue);
    vector<int>    neum_funct(numreadvalue);
    for (int i=0; i<numreadvalue; ++i)
    {
      neum_onoff[i] = 0;
      neum_val[i]   = 0.0;
      neum_funct[i] = 0;
    }

    //---------------------------------- read the curve number or "none"
    int curve=0;
    char buffer[200];
    ierr=sscanf(colptr," %s ",buffer);
    if(ierr!=1)
      dserror("Cannot read design-surface neumann conditions");
    if (strncmp(buffer,"none",4)==0)
    {
       curve = -1;
       colptr = strstr(allfiles.actplace,"none");
       if(colptr==NULL)
         dserror("Cannot read design-surface neumann conditions");
       colptr += 4;
    }
    else
    {
       ierr=sscanf(colptr," %d ",&curve);
       curve--;
       if(ierr!=1)
         dserror("Cannot read design-surface neumann conditions");
       colptr = strpbrk(colptr,"1234567890");
       colptr++;
    }


    /* NOTE: number of read values = 6  does not need to be */
    /*       equivalent to the numreadvalue -> e.g. for shell9! sh 12/02 */
    // read on/off toggles
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        neum_onoff[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // read values
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        neum_val[i] = strtod(colptr,&colptr);
      else
        strtod(colptr,&colptr);

     // read function numbers
     for (int i=0; i<numread; ++i)
      if (i < numreadvalue)
	neum_funct[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
           rcp(new DRT::Condition(dsurfid,DRT::Condition::SurfaceNeumann,true,
                                  DRT::Condition::Surface));

    // read whether load is on surface (shells)
    condition->Add("type","neum_live");
    frchk("Live",&ierr);
    if (ierr) condition->Add("type","neum_live");
    frchk("Dead",&ierr);
    if (ierr) condition->Add("type","neum_dead");
    frchk("PrescribedDomainLoad",&ierr);
    if (ierr) condition->Add("type","pres_domain_load");
    frchk("constHydro_z",&ierr);
    if (ierr) condition->Add("type","neum_consthydro_z");
    frchk("increaseHydro_z",&ierr);
    if (ierr) condition->Add("type","neum_increhydro_z");
    frchk("orthopressure",&ierr);
    if (ierr) condition->Add("type","neum_orthopressure");
    frchk("LAS",&ierr);
    if (ierr) condition->Add("type","neum_LAS");
    frchk("BioPressure",&ierr);
    if (ierr) condition->Add("type","neum_BioPressure");
    /*----------- read if load is applied on surface -> shell elements */
    frchk("Mid",&ierr);
    if (ierr) condition->Add("surface","mid");
    frchk("Top",&ierr);
    if (ierr) condition->Add("surface","top");
    frchk("Bot",&ierr);
    if (ierr) condition->Add("surface","bot");

    // add stuff to boundary condition
    condition->Add("onoff",neum_onoff);
    condition->Add("val",neum_val);
    condition->Add("curve",&curve,1);
    condition->Add("funct",neum_funct);

    //------------------------------- put condition in map of conditions
    snmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_surf_neum




/*----------------------------------------------------------------------*
 | input of design volume neumann conditions             m.gee 11/06   |
 *----------------------------------------------------------------------*/
void input_vol_neum(multimap<int,RefCountPtr<DRT::Condition> >& vnmap)
{
  // currently, we always have 6 values read from file
  // this might change at some point
  const int numread = 6;

  /*------------------ find the beginning of volume dirichlet conditions */
  if (frfind("--DESIGN VOL NEUMANN CONDITIONS")==0) return;
  frread();

  /*---------------------- read number of design volumes with conditions */
  int ierr=0;
  int ndvol=0;
  frint("DVOL",&ndvol,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-volume neumann conditions");
  frread();

  /*----------------------------------- start reading the design volumes */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*---------------------------------------- read the design volume Id */
    int dvolid = -1;
    frint("E",&dvolid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-volume neumann conditions");
    dvolid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-volume neumann conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    vector<int>    neum_onoff(numreadvalue);
    vector<double> neum_val(numreadvalue);
    vector<int>    neum_funct(numreadvalue);
    for (int i=0; i<numreadvalue; ++i)
    {
      neum_onoff[i] = 0;
      neum_val[i]   = 0.0;
      neum_funct[i] = 0;
    }

    //---------------------------------- read the curve number of "none"
    int curve=0;
    char buffer[200];
    ierr=sscanf(colptr," %s ",buffer);
    if(ierr!=1)
      dserror("Cannot read design-volume neumann conditions");
    if (strncmp(buffer,"none",4)==0)
    {
       curve = -1;
       colptr = strstr(allfiles.actplace,"none");
       if(colptr==NULL)
         dserror("Cannot read design-volume neumann conditions");
       colptr += 4;
    }
    else
    {
       ierr=sscanf(colptr," %d ",&curve);
       curve--;
       if(ierr!=1)
         dserror("Cannot read design-volume neumann conditions");
       colptr = strpbrk(colptr,"1234567890");
       colptr++;
    }


    /* NOTE: number of read values = 6  does not need to be */
    /*       equivalent to the numreadvalue -> e.g. for shell9! sh 12/02 */
    // read on/off toggles
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        neum_onoff[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // read values
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        neum_val[i] = strtod(colptr,&colptr);
      else
        strtod(colptr,&colptr);

     // read function numbers
     for (int i=0; i<numread; ++i)
      if (i < numreadvalue)
	neum_funct[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
            rcp(new DRT::Condition(dvolid,DRT::Condition::VolumeNeumann,true,
                                   DRT::Condition::Volume));

    // read whether load is on surface (shells)
    condition->Add("type","neum_dead");
    frchk("Dead",&ierr);
    if (ierr) condition->Add("type","neum_dead");
    frchk("LAS",&ierr);
    if (ierr) condition->Add("type","neum_LAS");

    // add stuff to boundary condition
    condition->Add("onoff",neum_onoff);
    condition->Add("val",neum_val);
    condition->Add("curve",&curve,1);
    condition->Add("funct",neum_funct);

    //------------------------------- put condition in map of conditions
    vnmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dvolid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_vol_neum


/*----------------------------------------------------------------------*
 | input of design node dirichlet conditions              m.gee 01/07   |
 *----------------------------------------------------------------------*/
void input_point_dirich(multimap<int,RefCountPtr<DRT::Condition> >& pdmap, bool ale)
{
  // currently, we always have 6 values read from file
  // this might change at some point
  const int numread = 6;

  /*-------------------- find the beginning of nodal dirichlet conditions */
  if (not ale)
  {
    if (frfind("--DESIGN POINT DIRICH CONDITIONS")==0) return;
  }
  else
  {
    if (frfind("--DESIGN POINT ALE DIRICH CONDITIONS")==0) return;
  }
  frread();

  /*------------------------ read number of design points with conditions */
  int ierr=0;
  int ndnode=0;
  frint("DPOINT",&ndnode,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-nodal dirichlet conditions");
  frread();

  /*-------------------------------------- start reading the design nodes */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design node Id */
    int dnodeid = -1;
    frint("E",&dnodeid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-nodal dirichlet conditions");
    dnodeid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-nodal dirichlet conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    vector<int>    dirich_onoff(numreadvalue);
    vector<double> dirich_val(numreadvalue);
    vector<int>    dirich_curve(numreadvalue);
    vector<int>    dirich_funct(numreadvalue);
    for (int i=0; i<numreadvalue; ++i)
    {
      dirich_onoff[i] = 0;
      dirich_val[i]   = 0.0;
      dirich_curve[i] = -1;
      dirich_funct[i] = 0;
    }

    /* NOTE: number of read values = 6  does not need to be */
    /*       equivalent to the numreadvalue -> e.g. for shell9! sh 12/02 */
    // read on/off toggles
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        dirich_onoff[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // read values
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        dirich_val[i] = strtod(colptr,&colptr);
      else
        strtod(colptr,&colptr);

    // read curve number or 'none'
    for (int i=0; i<numread; ++i)
    {
      char buffer[200];
      ierr=sscanf(colptr," %s ",buffer);
      if(ierr!=1)
        dserror("Cannot read design-nodal dirichlet conditions");
      if (strncmp(buffer,"none",4)==0)
      {
        colptr = strstr(colptr,"none");
        if(colptr==NULL)
          dserror("Cannot read design-nodal dirichlet conditions");
        colptr += 4;
      }
      else
      {
        ierr=1;
        if (i < numreadvalue)
        {
          ierr=sscanf(colptr," %d ",&dirich_curve[i]);
          dirich_curve[i]--;
        }
        if(ierr!=1)
          dserror("Cannot read design-nodal dirichlet conditions");
        colptr = strpbrk(colptr,"1234567890");
        colptr++;
      }
    }

    // read function number
    for (int i=0; i<numread; ++i)
    {
      if (i < numreadvalue)
        dirich_funct[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);
    }

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
          rcp(new DRT::Condition(dnodeid,DRT::Condition::PointDirichlet,false,
                                 DRT::Condition::Point));
    condition->Add("onoff",dirich_onoff);
    condition->Add("val",dirich_val);
    condition->Add("curve",dirich_curve);
    condition->Add("funct",dirich_funct);

    //---------------------- add the condition to the map of all conditions
    pdmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dnodeid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_point_dirich

/*----------------------------------------------------------------------*
 | input of design line dirichlet conditions              m.gee 01/07   |
 *----------------------------------------------------------------------*/
void input_line_dirich(multimap<int,RefCountPtr<DRT::Condition> >& ldmap, bool ale)
{
  // currently, we always have 6 values read from file
  // this might change at some point
  const int numread = 6;

  /*-------------------- find the beginning of line dirichlet conditions */
  if (not ale)
  {
    if (frfind("--DESIGN LINE DIRICH CONDITIONS")==0) return;
  }
  else
  {
    if (frfind("--DESIGN LINE ALE DIRICH CONDITIONS")==0) return;
  }
  frread();

  /*------------------------ read number of design lines with conditions */
  int ierr=0;
  int ndline=0;
  frint("DLINE",&ndline,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-line dirichlet conditions");
  frread();

  /*-------------------------------------- start reading the design lines */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design line Id */
    int dlineid = -1;
    frint("E",&dlineid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-line dirichlet conditions");
    dlineid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-line dirichlet conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    vector<int>    dirich_onoff(numreadvalue);
    vector<double> dirich_val(numreadvalue);
    vector<int>    dirich_curve(numreadvalue);
    vector<int>    dirich_funct(numreadvalue);
    for (int i=0; i<numreadvalue; ++i)
    {
      dirich_onoff[i] = 0;
      dirich_val[i]   = 0.0;
      dirich_curve[i] = -1;
      dirich_funct[i] = 0;
    }

    /* NOTE: number of read values = 6  does not need to be */
    /*       equivalent to the numreadvalue -> e.g. for shell9! sh 12/02 */
    // read on/off toggles
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        dirich_onoff[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // read values
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        dirich_val[i] = strtod(colptr,&colptr);
      else
        strtod(colptr,&colptr);

    // read curve number or 'none'
    for (int i=0; i<numread; ++i)
    {
      char buffer[200];
      ierr=sscanf(colptr," %s ",buffer);
      if(ierr!=1)
        dserror("Cannot read design-line dirichlet conditions");
      if (strncmp(buffer,"none",4)==0)
      {
        colptr = strstr(colptr,"none");
        if(colptr==NULL)
          dserror("Cannot read design-line dirichlet conditions");
        colptr += 4;
      }
      else
      {
        ierr=1;
        if (i < numreadvalue)
        {
          ierr=sscanf(colptr," %d ",&dirich_curve[i]);
          dirich_curve[i]--;
        }
        if(ierr!=1)
          dserror("Cannot read design-line dirichlet conditions");
        colptr = strpbrk(colptr,"1234567890");
        colptr++;
      }
    }

    // read function number
    for (int i=0; i<numread; ++i)
      if (i < numreadvalue)
        dirich_funct[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
           rcp(new DRT::Condition(dlineid,DRT::Condition::LineDirichlet,false,
                                  DRT::Condition::Line));
    condition->Add("onoff",dirich_onoff);
    condition->Add("val",dirich_val);
    condition->Add("curve",dirich_curve);
    condition->Add("funct",dirich_funct);

    //---------------------- add the condition to the map of all conditions
    ldmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dlineid,condition));
    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_line_dirich

/*----------------------------------------------------------------------*
 | input of design surface dirichlet conditions           m.gee 01/07   |
 *----------------------------------------------------------------------*/
void input_surf_dirich(multimap<int,RefCountPtr<DRT::Condition> >& sdmap, bool ale)
{
  // currently, we always have 6 values read from file
  // this might change at some point
  const int numread = 6;

  /*----------------- find the beginning of surface dirichlet conditions */
  if (not ale)
  {
    if (frfind("--DESIGN SURF DIRICH CONDITIONS")==0) return;
  }
  else
  {
    if (frfind("--DESIGN SURF ALE DIRICH CONDITIONS")==0) return;
  }
  frread();

  /*------------------------ read number of design surfs with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-surface dirichlet conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-surface dirichlet conditions");
    dsurfid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-surface dirichlet conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    vector<int>    dirich_onoff(numreadvalue);
    vector<double> dirich_val(numreadvalue);
    vector<int>    dirich_curve(numreadvalue);
    vector<int>    dirich_funct(numreadvalue);
    for (int i=0; i<numreadvalue; ++i)
    {
      dirich_onoff[i] = 0;
      dirich_val[i]   = 0.0;
      dirich_curve[i] = -1;
      dirich_funct[i] = 0;
    }

    /* NOTE: number of read values = 6  does not need to be */
    /*       equivalent to the numreadvalue -> e.g. for shell9! sh 12/02 */
    // read on/off toggles
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        dirich_onoff[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // read values
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        dirich_val[i] = strtod(colptr,&colptr);
      else
        strtod(colptr,&colptr);

    // read curve number or 'none'
    for (int i=0; i<numread; ++i)
    {
      char buffer[200];
      ierr=sscanf(colptr," %s ",buffer);
      if(ierr!=1)
        dserror("Cannot read design-surface dirichlet conditions");
      if (strncmp(buffer,"none",4)==0)
      {
        colptr = strstr(colptr,"none");
        if(colptr==NULL)
          dserror("Cannot read design-surface dirichlet conditions");
        colptr += 4;
      }
      else
      {
        ierr=1;
        if (i < numreadvalue)
        {
          ierr=sscanf(colptr," %d ",&dirich_curve[i]);
          dirich_curve[i]--;
        }
        if(ierr!=1)
          dserror("Cannot read design-line surface conditions");
        colptr = strpbrk(colptr,"1234567890");
        colptr++;
      }
    }

    // read function number
    for (int i=0; i<numread; ++i)
      if (i < numreadvalue)
        dirich_funct[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
        rcp(new DRT::Condition(dsurfid,DRT::Condition::SurfaceDirichlet,false,
                               DRT::Condition::Surface));
    condition->Add("onoff",dirich_onoff);
    condition->Add("val",dirich_val);
    condition->Add("curve",dirich_curve);
    condition->Add("funct",dirich_funct);

    //--------------------------------- add condition to map of conditions
    sdmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,condition));
    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_surf_dirich

/*----------------------------------------------------------------------*
 | input of design volume dirichlet conditions            m.gee 01/07   |
 *----------------------------------------------------------------------*/
void input_vol_dirich(multimap<int,RefCountPtr<DRT::Condition> >& vdmap)
{
  // currently, we always have 6 values read from file
  // this might change at some point
  const int numread = 6;

  /*------------------ find the beginning of volume dirichlet conditions */
  if (frfind("--DESIGN VOL DIRICH CONDITIONS")==0) return;
  frread();

  /*---------------------- read number of design volumes with conditions */
  int ierr=0;
  int ndvol=0;
  frint("DVOL",&ndvol,&ierr);
  if(ierr!=1)
    dserror("Cannot read volume dirichlet conditions");
  frread();

  /*----------------------------------- start reading the design volumes */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*---------------------------------------- read the design volume Id */
    int dvolid = -1;
    frint("E",&dvolid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-volume dirichlet conditions");
    dvolid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-volume dirichlet conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    vector<int>    dirich_onoff(numreadvalue);
    vector<double> dirich_val(numreadvalue);
    vector<int>    dirich_curve(numreadvalue);
    vector<int>    dirich_funct(numreadvalue);
    for (int i=0; i<numreadvalue; ++i)
    {
      dirich_onoff[i] = 0;
      dirich_val[i]   = 0.0;
      dirich_curve[i] = -1;
      dirich_funct[i] = 0;
    }

    /* NOTE: number of read values = 6  does not need to be */
    /*       equivalent to the numreadvalue -> e.g. for shell9! sh 12/02 */
    // read on/off toggles
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        dirich_onoff[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // read values
    for (int i=0; i<numread; ++i)
      if (i<numreadvalue)
        dirich_val[i] = strtod(colptr,&colptr);
      else
        strtod(colptr,&colptr);

    // read curve number or 'none'
    for (int i=0; i<numread; ++i)
    {
      char buffer[200];
      ierr=sscanf(colptr," %s ",buffer);
      if(ierr!=1)
        dserror("Cannot read design-volume dirichlet conditions");
      if (strncmp(buffer,"none",4)==0)
      {
        colptr = strstr(colptr,"none");
        if(colptr==NULL)
          dserror("Cannot read design-volume dirichlet conditions");
        colptr += 4;
      }
      else
      {
        ierr=1;
        if (i < numreadvalue)
        {
          ierr=sscanf(colptr," %d ",&dirich_curve[i]);
          dirich_curve[i]--;
        }
        if(ierr!=1)
          dserror("Cannot read design-volume conditions");
        colptr = strpbrk(colptr,"1234567890");
        colptr++;
      }
    }

    // read function number
    for (int i=0; i<numread; ++i)
      if (i < numreadvalue)
        dirich_funct[i] = strtol(colptr,&colptr,10);
      else
        strtol(colptr,&colptr,10);

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
          rcp(new DRT::Condition(dvolid,DRT::Condition::VolumeDirichlet,false,
                                 DRT::Condition::Volume));
    condition->Add("onoff",dirich_onoff);
    condition->Add("val",dirich_val);
    condition->Add("curve",dirich_curve);
    condition->Add("funct",dirich_funct);

    //--------------------------------- put condition in map of conditions
    vdmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dvolid,condition));
    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_vol_dirich


/*----------------------------------------------------------------------*
 | input of design line dirichlet conditions              m.gee 01/07   |
 *----------------------------------------------------------------------*/
void input_line_contact(multimap<int,RefCountPtr<DRT::Condition> >& ldmap)
{
  /*-------------------- find the beginning of line contact conditions */
  if (frfind("----CONTACT CONDITIONS 2D")==0) return;
  frread();

  /*----------------------- read number of design lines with condition */
  int ierr=0;
  int ndline=0;
  frint("DLINE",&ndline,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-line contact conditions");
  frread();

  /*-------------------------------------- start reading the design lines */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design line Id */
    int dlineid = -1;
    frint("E",&dlineid,&ierr);
    if(ierr!=1) dserror("Cannot read design-line contact conditions");
    dlineid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL) dserror("Cannot read design-line contact conditions");
    colptr++;
    //--------------------------------------- read unique contact pair id
    int contactpairid = -1;
    contactpairid = strtol(colptr,&colptr,10);
    if (contactpairid==-1) dserror("Cannot read design-line contact conditions");

    // read Master or Slave
    char buffer[200];
    string side;
    ierr=sscanf(colptr," %s ",buffer);
    if(ierr!=1) dserror("Cannot read design-line contact conditions");
    if (strncmp(buffer,"Master",6)==0)
    {
      side = "Master";
      colptr = strstr(colptr,"Master");
      if(colptr==NULL) dserror("Cannot read design-line contact conditions");
      colptr += 6;
    }
    else if (strncmp(buffer,"Slave",5)==0)
    {
      side = "Slave";
      colptr = strstr(colptr,"Slave");
      if(colptr==NULL) dserror("Cannot read design-line contact conditions");
      colptr += 5;
    }
    else dserror("Cannot read design-line contact conditions");

    // create boundary condition
    RCP<DRT::Condition> c = rcp(new DRT::Condition(dlineid,DRT::Condition::Contact,true,
                                                   DRT::Condition::Line));
    c->Add("contact id",contactpairid);
    c->Add("Side",side);
    //---------------------- add the condition to the map of all conditions
    ldmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dlineid,c));
    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_line_contact

/*----------------------------------------------------------------------*
 | input of design surface dirichlet conditions           m.gee 10/07   |
 *----------------------------------------------------------------------*/
void input_surf_contact(multimap<int,RefCountPtr<DRT::Condition> >& sdmap)
{
  if (frfind("----CONTACT CONDITIONS 3D")==0) return;
  frread();

  /*------------------------ read number of design surfs with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);
  if(ierr!=1) dserror("Cannot read design-surface contact conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1) dserror("Cannot read design-surface contact conditions");
    dsurfid--;
    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL) dserror("Cannot read design-surface contact conditions");
    colptr++;

    //------------------------------- define some temporary reading vectors
    int contactpairid = -1;
    contactpairid = strtol(colptr,&colptr,10);
    if (contactpairid==-1) dserror("Cannot read design-surface contact conditions");

    // read Master or Slave
    char buffer[200];
    string side;
    ierr=sscanf(colptr," %s ",buffer);
    if(ierr!=1) dserror("Cannot read design-surface contact conditions");
    if (strncmp(buffer,"Master",6)==0)
    {
      side = "Master";
      colptr = strstr(colptr,"Master");
      if(colptr==NULL) dserror("Cannot read design-surface contact conditions");
      colptr += 6;
    }
    else if (strncmp(buffer,"Slave",5)==0)
    {
      side = "Slave";
      colptr = strstr(colptr,"Slave");
      if(colptr==NULL) dserror("Cannot read design-surface contact conditions");
      colptr += 5;
    }
    else dserror("Cannot read design-surface contact conditions");

    // create boundary condition
    RefCountPtr<DRT::Condition> c = rcp(new DRT::Condition(dsurfid,DRT::Condition::Contact,true,
                                                           DRT::Condition::Surface));

    c->Add("contact id",contactpairid);
    c->Add("Side",side);


    //--------------------------------- add condition to map of conditions
    sdmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,c));
    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_surf_contact


/*----------------------------------------------------------------------*
 | input of design line periodic boundary conditions (for 2d channel    |
 | flows and similar things)                              gammi 04/07   |
 *----------------------------------------------------------------------*/
void input_line_periodic(
  multimap<int,RefCountPtr<DRT::Condition> >& lpbcmap)
{
  /*--------- find the beginning of line periodic boundary conditions */
  if (frfind("--DESIGN LINE PERIODIC BOUNDARY CONDITIONS")==0) return;
  frread();

  /*---------------------- read number of design lines with conditions */
  int ierr=0;
  int ndline=0;
  frint("DLINE",&ndline,&ierr);

  if(ierr!=1)
    dserror("Cannot read design-line pbc");
  frread();

  // the number of matching pbc pairs
  int numdiffpbc=ndline/2;

  if(ndline%2!=0)
    dserror("Pbc requires matching pairs of lines");

  vector<int> togglemasterslave(numdiffpbc);
  for(int i=0;i<numdiffpbc;i++)
  {
    togglemasterslave[i]=0;
  }

  /*----------------------------------- start reading the design lines */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dlineid = -1;
    frint("E",&dlineid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-line pbc");
    dlineid--;


    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-line pbc");
    colptr++;

    //----------------- define some temporary reading vectors and variables
    char     buffer[50];
    // the id of the pbc
    int         dlinepbcid;
    // degrees of freedom defining the periodic boundary conditions plane
    vector<int> dofsforpbcplane(2);

    // read Id of pbc. Must be smaller than or equal to the number of pbc pairs
    dlinepbcid = strtol(colptr,&colptr,10);


    if (dlinepbcid>numdiffpbc)
    {
      dserror("number of pbc higher than number of different pbcs!");
    }

    // we use the id to adress data in a C array -> start from 0
    dlinepbcid--;

    // we expect master/slave pairs of pbcs!!!
    if (togglemasterslave[dlinepbcid] >1)
    {
      dserror("you are not allowed to use more than two matching pbc lines yet");
    }

    // read the orientation of the plane which contains the pbc --- required for
    // node matching
    frchar("PLANE",buffer,&ierr);
    if (ierr!=1) dserror("cannot read orientation of pbc plane\n");

    if (strncmp(buffer,"xy",2)==0 || strncmp(buffer,"yx",2)==0)
    {
      dofsforpbcplane[0]=0;
      dofsforpbcplane[1]=1;
    }
    else if (strncmp(buffer,"yz",2)==0 || strncmp(buffer,"zy",2)==0)
    {
      dofsforpbcplane[0]=1;
      dofsforpbcplane[1]=2;
    }
    else if (strncmp(buffer,"xz",2)==0 || strncmp(buffer,"zx",2)==0)
    {
      dofsforpbcplane[0]=0;
      dofsforpbcplane[1]=2;
    }

    // create periodic boundary condition
    RefCountPtr<DRT::Condition> condition =
          rcp(new DRT::Condition(dlineid,
                                 DRT::Condition::LinePeriodic,
                                 false,
                                 DRT::Condition::Line));

    condition->Add("Is slave periodic boundary condition",&(togglemasterslave[dlinepbcid]),1);
    condition->Add("Id of periodic boundary condition"   ,&dlinepbcid,1);
    condition->Add("degrees of freedom for the pbc plane",dofsforpbcplane);

    // the next pbc with this Id will be the slave condition
    togglemasterslave[dlinepbcid]++;

    //--------------------------------- put condition in map of conditions
    lpbcmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dlineid,condition));



    //-------------------------------------------------- read the next line
    frread();
  }
  return;
} // input_line_periodic



/*----------------------------------------------------------------------*
 | input of design surface periodic boundary conditions for 3d channel  |
 | flows (and similar things)                             gammi 04/07   |
 *----------------------------------------------------------------------*/
void input_surf_periodic(
  multimap<int,RefCountPtr<DRT::Condition> >& spbcmap)
{
  /*--------- find the beginning of line periodic boundary conditions */
  if (frfind("--DESIGN SURF PERIODIC BOUNDARY CONDITIONS")==0) return;
  frread();

  /*------------------- read number of design surfs with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);

  if(ierr!=1)
    dserror("Cannot read design-surf pbc");
  frread();

  // the number of matching pbc pairs
  int numdiffpbc=ndsurf/2;

  if(ndsurf%2!=0)
    dserror("Pbc requires matching pairs of surfaces");

  vector<int> togglemasterslave(numdiffpbc);
  for(int i=0;i<numdiffpbc;i++)
  {
    togglemasterslave[i]=0;
  }

  /*-------------------------------- start reading the design surfaces */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-surface pbc");
    dsurfid--;


    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-surface pbc");
    colptr++;

    //----------------- define some temporary reading vectors and variables
    char     buffer[50];
    // the id of the pbc
    int         dsurfpbcid;
    // degrees of freedom defining the periodic boundary conditions plane
    vector<int> dofsforpbcplane(2);

    // read Id of pbc. Must be smaller than or equal to the number of pbc pairs
    dsurfpbcid = strtol(colptr,&colptr,10);


    if (dsurfpbcid>numdiffpbc)
    {
      dserror("number of pbc higher than number of different pbcs!");
    }

    // we use the id to adress data in a C array -> start from 0
    dsurfpbcid--;

    // we expect master/slave pairs of pbcs!!!
    if (togglemasterslave[dsurfpbcid] >1)
    {
      dserror("you are not allowed to use more than two matching pbc surfaces yet");
    }

    // read the orientation of the plane which contains the pbc --- required for
    // node matching
    frchar("PLANE",buffer,&ierr);
    if (ierr!=1) dserror("cannot read orientation of pbc plane\n");

    if (strncmp(buffer,"xy",2)==0 || strncmp(buffer,"yx",2)==0)
    {
      dofsforpbcplane[0]=0;
      dofsforpbcplane[1]=1;
    }
    else if (strncmp(buffer,"yz",2)==0 || strncmp(buffer,"zy",2)==0)
    {
      dofsforpbcplane[0]=1;
      dofsforpbcplane[1]=2;
    }
    else if (strncmp(buffer,"xz",2)==0 || strncmp(buffer,"zx",2)==0)
    {
      dofsforpbcplane[0]=0;
      dofsforpbcplane[1]=2;
    }

    // create periodic boundary condition
    RefCountPtr<DRT::Condition> condition =
          rcp(new DRT::Condition(dsurfid,
                                 DRT::Condition::SurfacePeriodic,
                                 false,
                                 DRT::Condition::Surface));

    condition->Add("Is slave periodic boundary condition",&(togglemasterslave[dsurfpbcid]),1);
    condition->Add("Id of periodic boundary condition"   ,&dsurfpbcid,1);
    condition->Add("degrees of freedom for the pbc plane",dofsforpbcplane);

    // the next pbc with this Id will be the slave condition
    togglemasterslave[dsurfpbcid]++;

    //--------------------------------- put condition in map of conditions
    spbcmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,condition));

    //-------------------------------------------------- read the next line
    frread();
  }

  for(int i=0;i<numdiffpbc;i++)
  {
    if(togglemasterslave[i]!=2)
    {
      dserror("reading of pbc pairs failed");
    }
  }

  return;
} // input_surf_periodic


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void input_line_fsi_coupling(multimap<int,RefCountPtr<DRT::Condition> >& lfsicoupmap)
{
  if (frfind("--DESIGN FSI COUPLING LINE CONDITIONS")==0) return;
  frread();

  /*---------------------- read number of design lines with conditions */
  int ierr=0;
  int ndline=0;
  frint("DLINE",&ndline,&ierr);

  if (ierr!=1) dserror("Cannot read line fsi coupling");
  frread();

  /*----------------------------------- start reading the design lines */
  while (strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dlineid = -1;
    frint("E",&dlineid,&ierr);
    if (ierr!=1) dserror("Cannot read line fsi coupling");
    dlineid--;

    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if (colptr==NULL) dserror("Cannot read line fsi coupling");
    colptr++;

    /*----------------------------------------- read the  fsi couplingId */
    int coupleId=-1;
    coupleId = strtol(colptr,&colptr,10);
    if (coupleId<=0) dserror("Cannot read line fsi coupling");

    /*------------------------------------------------ read the fieldtyp */
    char buffer[50];
    ierr=sscanf(colptr," %s ",buffer);
    if (ierr!=1) dserror("Cannot read line fsi coupling");

    // create periodic boundary condition
    RefCountPtr<DRT::Condition> condition = rcp(new DRT::Condition(dlineid,
                                                                   DRT::Condition::FSICoupling,
                                                                   true,
                                                                   DRT::Condition::Line));
    condition->Add("field", string(buffer));

    //--------------------------------- put condition in map of conditions
    lfsicoupmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dlineid,condition));

    //-------------------------------------------------- read the next line
    frread();
  }
}



/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void input_surf_fsi_coupling(multimap<int,RefCountPtr<DRT::Condition> >& sfsicoupmap)
{
  if (frfind("--DESIGN FSI COUPLING SURF CONDITIONS")==0) return;
  frread();

  /*---------------------- read number of design surfaces with conditions */
  int ierr=0;
  int ndsurface=0;
  frint("DSURF",&ndsurface,&ierr);

  if (ierr!=1) dserror("Cannot read surface fsi coupling");
  frread();

  /*----------------------------------- start reading the design surfaces */
  while (strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surface Id */
    int dsurfaceid = -1;
    frint("E",&dsurfaceid,&ierr);
    if (ierr!=1) dserror("Cannot read surface fsi coupling");
    dsurfaceid--;

    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if (colptr==NULL) dserror("Cannot read surface fsi coupling");
    colptr++;

    /*----------------------------------------- read the  fsi couplingId */
    int coupleId=-1;
    coupleId = strtol(colptr,&colptr,10);
    if (coupleId<=0) dserror("Cannot read surface fsi coupling");

    /*------------------------------------------------ read the fieldtyp */
    char buffer[50];
    ierr=sscanf(colptr," %s ",buffer);
    if (ierr!=1) dserror("Cannot read surface fsi coupling");

    // create periodic boundary condition
    RefCountPtr<DRT::Condition> condition = rcp(new DRT::Condition(dsurfaceid,
                                                                   DRT::Condition::FSICoupling,
                                                                   true,
                                                                   DRT::Condition::Surface));
    condition->Add("field", string(buffer));

    //--------------------------------- put condition in map of conditions
    sfsicoupmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfaceid,condition));

    //-------------------------------------------------- read the next surface
    frread();
  }
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void input_line_xfem_coupling(multimap<int,RefCountPtr<DRT::Condition> >& lxfemcoupmap)
{
  if (frfind("--DESIGN XFEM COUPLING LINE CONDITIONS")==0) return;
  frread();

  /*---------------------- read number of design lines with conditions */
  int ierr=0;
  int ndline=0;
  frint("DLINE",&ndline,&ierr);

  if (ierr!=1) dserror("Cannot read line xfem coupling");
  frread();

  /*----------------------------------- start reading the design lines */
  while (strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dlineid = -1;
    frint("E",&dlineid,&ierr);
    if (ierr!=1) dserror("Cannot read line xfem coupling");
    dlineid--;

    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if (colptr==NULL) dserror("Cannot read line xfem coupling");
    colptr++;

    /*----------------------------------------- read the  fsi couplingId */
    int coupleId=-1;
    coupleId = strtol(colptr,&colptr,10);
    if (coupleId<=0) dserror("Cannot read line xfem coupling");

    // create periodic boundary condition
    RefCountPtr<DRT::Condition> condition = rcp(new DRT::Condition(dlineid,
                                                                   DRT::Condition::XFEMCoupling,
                                                                   true,
                                                                   DRT::Condition::Line));

    //--------------------------------- put condition in map of conditions
    lxfemcoupmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dlineid,condition));

    //-------------------------------------------------- read the next line
    frread();
  }
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void input_surf_xfem_coupling(multimap<int,RefCountPtr<DRT::Condition> >& sxfemcoupmap)
{
  if (frfind("--DESIGN XFEM COUPLING SURF CONDITIONS")==0) return;
  frread();

  /*---------------------- read number of design surfaces with conditions */
  int ierr=0;
  int ndsurface=0;
  frint("DSURF",&ndsurface,&ierr);

  if (ierr!=1) dserror("Cannot read surface xfem coupling");
  frread();

  /*----------------------------------- start reading the design surfaces */
  while (strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surface Id */
    int dsurfaceid = -1;
    frint("E",&dsurfaceid,&ierr);
    if (ierr!=1) dserror("Cannot read surface xfem coupling");
    dsurfaceid--;

    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if (colptr==NULL) dserror("Cannot read surface xfem coupling");
    colptr++;

    /*----------------------------------------- read the xfem coupling Id */
    int coupleId=-1;
    coupleId = strtol(colptr,&colptr,10);
    if (coupleId<=0) dserror("Cannot read surface xfem coupling");

    // create boundary condition
    RefCountPtr<DRT::Condition> condition = rcp(new DRT::Condition(dsurfaceid,
                                                                   DRT::Condition::XFEMCoupling,
                                                                   true,
                                                                   DRT::Condition::Surface));

    condition->Add("label", int(coupleId));

    //--------------------------------- put condition in map of conditions
    sxfemcoupmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfaceid,condition));

    //-------------------------------------------------- read the next surface
    frread();
  }
}




/*----------------------------------------------------------------------*
 | input of design surface stress conditions                 lw 06/07   |
 *----------------------------------------------------------------------*/
void input_surf_stress(multimap<int,RefCountPtr<DRT::Condition> >& ssmap)
{
  /*-------------------- find the beginning of surface stress conditions */
  if (frfind("--SURFACE CONDITIONS")==0) return;
  frread();

  /*------------------------ read number of design surfs with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-surface stress conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-surface stress conditions");
    dsurfid--;

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
           rcp(new DRT::Condition(dsurfid,DRT::Condition::SurfaceStress,true,
                                  DRT::Condition::Surface));

    // read input in case of dynamic surfactant model

    ierr=0;
    frchk("SURFACTANT", &ierr);
    if (ierr)
    {
      condition->Add("surface_flag", 0);
      double temp;
//       condition->Add("type","surfactant");
//       frdouble("k1", &temp, &ierr);
//       if (ierr == 1)
//         condition->Add("k1", temp);
//       else
//         dserror("Cannot read k1 for surfactant");
      double k1xC;
      frdouble("k1xCbulk", &k1xC, &ierr);
      if (ierr == 1)
        condition->Add("k1xCbulk", k1xC);
      else
        dserror("Cannot read k1xCbulk for surfactant");

      double k2;
      frdouble("k2", &k2, &ierr);
      if (ierr == 1)
        condition->Add("k2", k2);
      else
        dserror("Cannot read k2 for surfactant");
//       frdouble("Cbulk", &temp, &ierr);
//       if (ierr == 1)
//         condition->Add("Cbulk", temp);
//       else
//         dserror("Cannot read Cbulk for surfactant");
      frdouble("m1", &temp, &ierr);
      if (ierr == 1)
        condition->Add("m1", temp);
      else
        dserror("Cannot read m1 for surfactant");
      double m2;
      frdouble("m2", &m2, &ierr);
      if (ierr == 1)
        condition->Add("m2", m2);
      else
        dserror("Cannot read m2 for surfactant");
      frdouble("gamma_0", &temp, &ierr);
      if (ierr == 1)
        condition->Add("gamma_0", temp);
      else
        dserror("Cannot read gamma_0 for surfactant");
      double gamma_min;
      frdouble("gamma_min", &gamma_min, &ierr);
      if (ierr == 1)
        condition->Add("gamma_min", gamma_min);
      else
        dserror("Cannot read gamma_min for surfactant");
      double gamma_min_eq;
      frdouble("gamma_min_eq", &gamma_min_eq, &ierr);
      if (ierr == 1)
        condition->Add("gamma_min_eq", gamma_min_eq);
      else
        dserror("Cannot read gamma_min_eq for surfactant");

      double con_quot_max=(gamma_min_eq-gamma_min)/m2+1.;
      condition->Add("con_quot_max", con_quot_max);
      double con_quot_eq=(k1xC)/(k1xC+k2);
      condition->Add("con_quot_eq", con_quot_eq);
    }

    // read input in case of constant surface tension

    int ierr1=0;
    frchk("SURFACE TENSION", &ierr1);
    if (ierr1)
    {
      condition->Add("surface_flag", 1);
      double temp;
      condition->Add("type","surftension");
      frdouble("gamma", &temp, &ierr);
      if (ierr == 1)
        condition->Add("gamma", temp);
      else
        dserror("Cannot read gamma for surfactant");
    }

    if (ierr1==0 && ierr==0)
      dserror("Unknown type of surface stress condition");


    //------------------------------- put condition in map of conditions
    ssmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_surf_stress


/*----------------------------------------------------------------------------*
 | input of microscale boundary conditions for multiscale analyses   lw 06/07 |
 *----------------------------------------------------------------------------*/
void input_micro_bc(multimap<int,RefCountPtr<DRT::Condition> >& mbcmap)
{
  /*--------------- find the beginning of microscale boundary conditions */
  if (frfind("--MICROSCALE CONDITIONS")==0) return;
  frread();

  /*------------------------ read number of design surfs with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-surface microscale conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-surface stress conditions");
    dsurfid--;

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
           rcp(new DRT::Condition(dsurfid,DRT::Condition::MicroBoundary,true,
                                  DRT::Condition::Surface));

    //------------------------------- put condition in map of conditions
    mbcmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
} // input_micro_bc


/*----------------------------------------------------------------------------*
 | input of design line fluid stress calculation conditions       g.bau 07/07 |
 *----------------------------------------------------------------------------*/
void input_line_stress_calc(multimap<int,RefCountPtr<DRT::Condition> >& lnmap)
{
  /*----------- find the beginning of line stress calculation conditions */
  if (frfind("---DESIGN LINE STRESS CALC CONDITIONS")==0) return;
  frread();

  /*------------------------ read number of design lines with conditions */
  int ierr=0;
  int ndline=0;
  frint("DLINE",&ndline,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-line stress calculation conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design line Id */
    int dlineid = -1;
    frint("E",&dlineid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-line stress calculation conditions");
    dlineid--;

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
           rcp(new DRT::Condition(dlineid,DRT::Condition::FluidStressCalc,true,
                                  DRT::Condition::Line));

    //------------------------------- put condition in map of conditions
    lnmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dlineid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
};


/*----------------------------------------------------------------------------*
 | input of design surface fluid stress calculation conditions    g.bau 07/07 |
 *----------------------------------------------------------------------------*/
void input_surf_stress_calc(multimap<int,RefCountPtr<DRT::Condition> >& snmap)
{
  /*-------- find the beginning of surface stress calculation conditions */
  if (frfind("---DESIGN SURF STRESS CALC CONDITIONS")==0) return;
  frread();
  /*--------------------- read number of design surfaces with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-surface stress calculation conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-surface stress calculation conditions");
    dsurfid--;

    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
           rcp(new DRT::Condition(dsurfid,DRT::Condition::FluidStressCalc,true,
                                  DRT::Condition::Surface));

    //------------------------------- put condition in map of conditions
    snmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
};

/*----------------------------------------------------------------------*
 | input of design line LIFTDRAG conditions              m.levy 09/07   |
 *----------------------------------------------------------------------*/
void input_line_LIFTDRAG(multimap<int,RefCountPtr<DRT::Condition> >& lldmap)
{
  /*----------- find the beginning of line LIFTDRAG conditions */
  if (frfind("---DESIGN FLUID LINE LIFT&DRAG")==0) return;
  frread();

  /*------------------------ read number of design lines with conditions */
  int ierr=0;
  int ndline=0;
  frint("DLINE",&ndline,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-line LIFTDRAG conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design line Id */
    int dlineid = -1;
    frint("E",&dlineid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-line LIFTDRAG conditions");
    dlineid--;

    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-line LIFTDRAG conditions");
    colptr++;

    /*------------------------------------------------------ read values */
    int label = strtol(colptr,&colptr,10);
    vector<double> centercoord(3,0.0);
    centercoord[0] = strtod(colptr,&colptr);
    centercoord[1] = strtod(colptr,&colptr);

    if (label <= 0)
      dserror("LiftDrag Label must be greater than 0!");

    /*----------------------------------------- create boundary condition */
    RefCountPtr<DRT::Condition> myldcond =
           rcp(new DRT::Condition(dlineid,DRT::Condition::LineLIFTDRAG,true,
                                  DRT::Condition::Line));
    myldcond->Add("label",label);
    myldcond->Add("centerCoord",centercoord);

    /*------------------- add the condition to the map of all conditions */
    lldmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dlineid,myldcond));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
};   //input_line_LIFTDRAG


/*----------------------------------------------------------------------*
 | input of design surf LIFTDRAG conditions              m.levy 09/07   |
 *----------------------------------------------------------------------*/
void input_surf_LIFTDRAG(multimap<int,RefCountPtr<DRT::Condition> >& sldmap)
{
  /*-------- find the beginning of surface stress calculation conditions */
  if (frfind("---DESIGN FLUID SURF LIFT&DRAG")==0) return;
  frread();

  /*--------------------- read number of design surfaces with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-surface LIFTDRAG conditions");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-surface LIFTDRAG conditions");
    dsurfid--;

    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
      dserror("Cannot read design-surface LIFTDRAG conditions");
    colptr++;

    //-------------------------------read values
    int label = strtol(colptr,&colptr,10);
    vector<double> centercoord(3,0.0);
    centercoord[0] = strtod(colptr,&colptr);
    centercoord[1] = strtod(colptr,&colptr);
    centercoord[2] = strtod(colptr,&colptr);

    if (label <= 0)
      dserror("LiftDrag Label must be greater than 0!");

    /*----------------------------------------- create boundary condition */
	// create boundary condition
    RefCountPtr<DRT::Condition> condition =
           rcp(new DRT::Condition(dsurfid,DRT::Condition::SurfLIFTDRAG,true,
                                  DRT::Condition::Surface));
    condition->Add("label",label);
    condition->Add("centerCoord",centercoord);

    //---------------------- add the condition to the map of all conditions
    sldmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,condition));
    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
};  //input_surf_LIFTDRAG

/*----------------------------------------------------------------------------*
 | input of volume constraint surfaces                               tk 10/07 |
 *----------------------------------------------------------------------------*/
void input_surf_volconstr(multimap<int,RefCountPtr<DRT::Condition> >& snmap)
{
  /*-------------------- find the beginning of volume constraint surface */
  if (frfind("---DESIGN SURFACE VOLUME CONSTRAINT 3D")==0)
	  {
	    return;
	  }

  frread();
  /*--------------------- read number of design surfaces with conditions */
  int ierr=0;
  int ndsurf=0;
  frint("DSURF",&ndsurf,&ierr);
  if(ierr!=1)
    dserror("Cannot read design-surface for volume constraint 3D");
  frread();

  /*------------------------------------- start reading the design surfs */
  while(strncmp(allfiles.actplace,"------",6)!=0)
  {
    /*------------------------------------------ read the design surf Id */
    int dsurfid = -1;
    frint("E",&dsurfid,&ierr);
    if(ierr!=1)
      dserror("Cannot read design-surface for volume constraint 3D");
    dsurfid--;

    /*--------------------------------- move pointer behind the "-" sign */
    char* colptr = strstr(allfiles.actplace,"-");
    if(colptr==NULL)
       dserror("Cannot read design-surface for volume constraint 3D");
    colptr++;

    //------------------------------- define some temporary reading vectors
    int    	VolConstrID=strtol(colptr,&colptr,10);
    int  	VolConstrCurve=-1;

    //---------------------------------- read the curve number or 'none'
    char buffer[200];
    ierr=sscanf(colptr," %s ",buffer);
    if(ierr!=1)
      dserror("Cannot read design-surface for volume constraint 3D");
    if (strncmp(buffer,"none",4)==0)
    {
       colptr = strstr(allfiles.actplace,"none");
       if(colptr==NULL)
         dserror("Cannot read design-surface for volume constraint 3D");
       colptr += 4;
    }
    else
    {
       ierr=sscanf(colptr," %d ",&VolConstrCurve);
       VolConstrCurve--;
       if(ierr!=1)
         dserror("Cannot read design-surface for volume constraint 3D");
       colptr = strpbrk(colptr,"1234567890");
       colptr++;
    }


    // create boundary condition
    RefCountPtr<DRT::Condition> condition =
           rcp(new DRT::Condition(dsurfid,DRT::Condition::VolumeConstraint_3D,true,
                                  DRT::Condition::Surface));

    condition->Add("ConditionID",&VolConstrID,1);
    condition->Add("curve",&VolConstrCurve,1);

    //------------------------------- put condition in map of conditions
    snmap.insert(pair<int,RefCountPtr<DRT::Condition> >(dsurfid,condition));

    //-------------------------------------------------- read the next line
    frread();
  } // while(strncmp(allfiles.actplace,"------",6)!=0)
  return;
};

#endif  // #ifdef CCADISCRET
