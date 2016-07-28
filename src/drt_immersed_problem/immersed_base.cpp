/*!----------------------------------------------------------------------
\file immersed_base.cpp

\brief base class for all immersed algorithms

\level 2

\maintainer Andreas Rauch
            rauch@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289 -15240

*----------------------------------------------------------------------*/
#include "immersed_base.H"
#include "../linalg/linalg_utils.H"


IMMERSED::ImmersedBase::ImmersedBase()
{

}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IMMERSED::ImmersedBase::CreateVolumeCondition(const Teuchos::RCP<DRT::Discretization>& dis,
                                                   const std::vector<int> dvol_fenode,
                                                   const DRT::Condition::ConditionType condtype,
                                                   const std::string condname)
{
  // determine id of condition
  std::multimap<std::string,Teuchos::RCP<DRT::Condition> > allconditions;
  allconditions = dis->GetAllConditions();
  int id = (int)allconditions.size();
  id += 1;

  // build condition
  bool buildgeometry = true; // needed for now to check number of elements in neumannnaumann.cpp
  Teuchos::RCP<DRT::Condition> condition =
          Teuchos::rcp(new DRT::Condition(id,condtype,buildgeometry,DRT::Condition::Volume));

  // add nodes to conditions
   condition->Add("Node Ids",dvol_fenode);

   // add condition to discretization
   dis->SetCondition(condname,condition);

   // fill complete if necessary
   if (!dis->Filled())
     dis -> FillComplete();

   //debug
#ifdef DEBUG
   std::cout<<"PROC "<<dis->Comm().MyPID()<<" : Number of conditioned elements: "<<dis->GetCondition(condname)->Geometry().size()<<" ("<<condname<<")"<<std::endl;
#endif

}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IMMERSED::ImmersedBase::EvaluateImmersed(Teuchos::ParameterList& params,
                                              Teuchos::RCP<DRT::Discretization> dis,
                                              DRT::AssembleStrategy* strategy,
                                              std::map<int,std::set<int> >* elementstoeval,
                                              Teuchos::RCP<GEO::SearchTree> structsearchtree,
                                              std::map<int,LINALG::Matrix<3,1> >* currpositions_struct,
                                              int action,
                                              bool evaluateonlyboundary)
{
  // pointer to element
  DRT::Element* ele;

  for(std::map<int, std::set<int> >::const_iterator closele = elementstoeval->begin(); closele != elementstoeval->end(); closele++)
  {
    for(std::set<int>::const_iterator eleIter = (closele->second).begin(); eleIter != (closele->second).end(); eleIter++)
    {
      ele=dis->gElement(*eleIter);

      DRT::ELEMENTS::FluidImmersedBase* immersedelebase = dynamic_cast<DRT::ELEMENTS::FluidImmersedBase*>(ele);
      if(immersedelebase==NULL)
        dserror("dynamic cast from DRT::Element* to DRT::ELEMENTS::FluidImmersedBase* failed");

      // evaluate this element and fill vector with immersed dirichlets
      int row = strategy->FirstDofSet();
      int col = strategy->SecondDofSet();

      params.set<int>("action",action);
      params.set<Teuchos::RCP<GEO::SearchTree> >("structsearchtree_rcp",structsearchtree);
      params.set<std::map<int,LINALG::Matrix<3,1> >* >("currpositions_struct",currpositions_struct);
      params.set<int>("Physical Type",INPAR::FLUID::poro_p1);
      if(dis->Name()=="fluid")
        params.set<std::string>("immerseddisname","structure");
      else if (dis->Name()=="porofluid")
        params.set<std::string>("immerseddisname","cell");
      else
        dserror("no corresponding immerseddisname set for this type of backgrounddis!");

      DRT::Element::LocationArray la(1);
      immersedelebase->LocationVector(*dis,la,false);
      strategy->ClearElementStorage( la[row].Size(), la[col].Size() );

      if(!evaluateonlyboundary)
        immersedelebase->Evaluate(params,*dis,la[0].lm_,
            strategy->Elematrix1(),
            strategy->Elematrix2(),
            strategy->Elevector1(),
            strategy->Elevector2(),
            strategy->Elevector3());
      else
      {
        if(immersedelebase->IsBoundaryImmersed())
          immersedelebase->Evaluate(params,*dis,la[0].lm_,
              strategy->Elematrix1(),
              strategy->Elematrix2(),
              strategy->Elevector1(),
              strategy->Elevector2(),
              strategy->Elevector3());
      }

      strategy->AssembleVector1( la[row].lm_, la[row].lmowner_ );
    }
  }
  return;
} // EvaluateImmersed

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IMMERSED::ImmersedBase::EvaluateImmersedNoAssembly(Teuchos::ParameterList& params,
                                              Teuchos::RCP<DRT::Discretization> dis,
                                              std::map<int,std::set<int> >* elementstoeval,
                                              Teuchos::RCP<GEO::SearchTree> structsearchtree,
                                              std::map<int,LINALG::Matrix<3,1> >* currpositions_struct,
                                              int action
                                                        )
{
  // pointer to element
  DRT::Element* ele;

  for(std::map<int, std::set<int> >::const_iterator closele = elementstoeval->begin(); closele != elementstoeval->end(); closele++)
  {
    for(std::set<int>::const_iterator eleIter = (closele->second).begin(); eleIter != (closele->second).end(); eleIter++)
    {
      ele=dis->gElement(*eleIter);

      DRT::ELEMENTS::FluidImmersedBase* immersedelebase = dynamic_cast<DRT::ELEMENTS::FluidImmersedBase*>(ele);
      if(immersedelebase==NULL)
        dserror("dynamic cast from DRT::Element* to DRT::ELEMENTS::FluidImmersedBase* failed");

      // provide important objects to ParameterList
      params.set<int>("action",action);
      params.set<Teuchos::RCP<GEO::SearchTree> >("structsearchtree_rcp",structsearchtree);
      params.set<std::map<int,LINALG::Matrix<3,1> >* >("currpositions_struct",currpositions_struct);
      params.set<int>("Physical Type",INPAR::FLUID::poro_p1);
      if(dis->Name()=="fluid")
        params.set<std::string>("immerseddisname","structure");
      else if (dis->Name()=="porofluid")
        params.set<std::string>("immerseddisname","cell");
      else
        dserror("no corresponding immerseddisname set for this type of backgrounddis!");

      // evaluate the element
      Epetra_SerialDenseMatrix dummymat;
      Epetra_SerialDenseVector dummyvec;

      DRT::Element::LocationArray la(1);
      immersedelebase->LocationVector(*dis,la,false);

      immersedelebase->Evaluate(params,*dis,la[0].lm_,dummymat,dummymat,dummyvec,dummyvec,dummyvec);
    }
  }
  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IMMERSED::ImmersedBase::EvaluateScaTraWithInternalCommunication(Teuchos::RCP<DRT::Discretization> dis,
                                                                     const Teuchos::RCP<const DRT::Discretization> idis,
                                                                     DRT::AssembleStrategy* strategy,
                                                                     std::map<int,std::set<int> >* elementstoeval,
                                                                     Teuchos::RCP<GEO::SearchTree> structsearchtree,
                                                                     std::map<int,LINALG::Matrix<3,1> >* currpositions_struct,
                                                                     Teuchos::ParameterList& params,
                                                                     bool evaluateonlyboundary)
{
  // pointer to element
  DRT::Element* ele;
  DRT::Element* iele;

  for(std::map<int, std::set<int> >::const_iterator closele = elementstoeval->begin(); closele != elementstoeval->end(); closele++)
  {
    for(std::set<int>::const_iterator eleIter = (closele->second).begin(); eleIter != (closele->second).end(); eleIter++)
    {
      ele=dis->gElement(*eleIter);
      iele=idis->gElement(*eleIter);

      DRT::ELEMENTS::FluidImmersedBase* immersedelebase = dynamic_cast<DRT::ELEMENTS::FluidImmersedBase*>(iele);
      if(immersedelebase==NULL)
        dserror("dynamic cast from DRT::Element* to DRT::ELEMENTS::FluidImmersedBase* failed");

      // evaluate this element and fill vector with immersed dirichlets
      int row = strategy->FirstDofSet();
      int col = strategy->SecondDofSet();

      params.set<Teuchos::RCP<GEO::SearchTree> >("structsearchtree_rcp",structsearchtree);
      params.set<std::map<int,LINALG::Matrix<3,1> >* >("currpositions_struct",currpositions_struct);
      params.set<int>("Physical Type",INPAR::FLUID::poro_p1);

      DRT::Element::LocationArray la(dis->NumDofSets());
      ele->LocationVector(*dis,la,false);
      strategy->ClearElementStorage( la[row].Size(), la[col].Size() );

      if(!evaluateonlyboundary)
        ele->Evaluate(params,*dis,la,
            strategy->Elematrix1(),
            strategy->Elematrix2(),
            strategy->Elevector1(),
            strategy->Elevector2(),
            strategy->Elevector3());
      else
      {
        if(immersedelebase->IsBoundaryImmersed())
          ele->Evaluate(params,*dis,la,
              strategy->Elematrix1(),
              strategy->Elematrix2(),
              strategy->Elevector1(),
              strategy->Elevector2(),
              strategy->Elevector3());
      }

      strategy->AssembleVector1( la[row].lm_, la[row].lmowner_ );
    }
  }
} // EvaluateWithInternalCommunication

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
/// Reduces to standard EvaluateCondition on one proc.
/// Evaluate a specific condition using assemble strategy allowing communication at element level
/// until every conditioned element is evaluated. Needed especially during interpolation from an
/// other discretization to the conditioned elements (e.g. in immersed method).
/// The integration point of a conditioned element requesting a quantity may be owned by another
/// proc as the interpolating element providing this quantity.  rauch 05/14
void IMMERSED::ImmersedBase::EvaluateInterpolationCondition
(
    Teuchos::RCP<DRT::Discretization> evaldis,
    Teuchos::ParameterList& params,
    DRT::AssembleStrategy & strategy,
    const std::string& condstring,
    const int condid
)
{
# ifdef DEBUG
  if (!(evaldis->Filled()) ) dserror("FillComplete() was not called");
  if (!(evaldis->HaveDofs())) dserror("AssignDegreesOfFreedom() was not called");
# endif

  int row = strategy.FirstDofSet();
  int col = strategy.SecondDofSet();

  // get the current time
  bool usetime = true;
  const double time = params.get("total time",-1.0);
  if (time<0.0) usetime = false;

  params.set<int>("dummy_call",0);

  DRT::Element::LocationArray la(evaldis->NumDofSets());

  std::multimap<std::string,Teuchos::RCP<DRT::Condition> >::iterator fool;

  //----------------------------------------------------------------------
  // loop through conditions and evaluate them if they match the criterion
  //----------------------------------------------------------------------
  for (fool=evaldis->GetAllConditions().begin(); fool!=evaldis->GetAllConditions().end(); ++fool)
  {
    if (fool->first == condstring)
    {
      DRT::Condition& cond = *(fool->second);
      if (condid == -1 || condid ==cond.GetInt("ConditionID"))
      {
        std::map<int,Teuchos::RCP<DRT::Element> >& geom = cond.Geometry();
        if (geom.empty()) dserror("evaluation of condition with empty geometry on proc %d",evaldis->Comm().MyPID());

        std::map<int,Teuchos::RCP<DRT::Element> >::iterator curr;

        // Evaluate Loadcurve if defined. Put current load factor in parameterlist
        const std::vector<int>* curve  = cond.Get<std::vector<int> >("curve");
        int curvenum = -1;
        if (curve) curvenum = (*curve)[0];
        double curvefac = 1.0;
        if (curvenum>=0 && usetime)
          curvefac = DRT::Problem::Instance()->Curve(curvenum).f(time);

        // Get ConditionID of current condition if defined and write value in parameterlist
        const std::vector<int>*    CondIDVec  = cond.Get<std::vector<int> >("ConditionID");
        if (CondIDVec)
        {
          params.set("ConditionID",(*CondIDVec)[0]);
          char factorname[30];
          sprintf(factorname,"LoadCurveFactor %d",(*CondIDVec)[0]);
          params.set(factorname,curvefac);
        }
        else
        {
          params.set("LoadCurveFactor",curvefac);
        }
        params.set<Teuchos::RCP<DRT::Condition> >("condition", fool->second);

        int mygeometrysize=-1234;
        if(geom.empty()==true)
          mygeometrysize=0;
        else
          mygeometrysize=geom.size();
        int maxgeometrysize=-1234;
        evaldis->Comm().MaxAll(&mygeometrysize,&maxgeometrysize,1);
        curr=geom.begin();

#ifdef DEBUG
        std::cout<<"PROC "<<evaldis->Comm().MyPID()<<": mygeometrysize = "<<mygeometrysize<<" maxgeometrysize = "<<maxgeometrysize<<std::endl;
#endif


        // enter loop on every proc until the last proc evaluated his last geometry element
        // because there is communication happening inside
        for (int i=0;i<maxgeometrysize;++i)
        {
          if(i>=mygeometrysize)
            params.set<int>("dummy_call",1);

          // get element location vector and ownerships
          // the LocationVector method will return the the location vector
          // of the dofs this condition is meant to assemble into.
          // These dofs do not need to be the same as the dofs of the element
          // (this is the standard case, though). Special boundary conditions,
          // like weak dirichlet conditions, assemble into the dofs of the parent element.
          curr->second->LocationVector(*evaldis,la,false,condstring,params);

          // get dimension of element matrices and vectors
          // Reshape element matrices and vectors and init to zero

          strategy.ClearElementStorage( la[row].Size(), la[col].Size() );

          // call the element specific evaluate method
          int err = curr->second->Evaluate(params,*evaldis,la,
              strategy.Elematrix1(),
              strategy.Elematrix2(),
              strategy.Elevector1(),
              strategy.Elevector2(),
              strategy.Elevector3());
          if (err) dserror("error while evaluating elements");

          // assemble every element contribution only once
          // do not assemble after dummy call for internal communication
          if(i<mygeometrysize)
          {
            // assembly
            int eid = curr->second->Id();
            strategy.AssembleMatrix1( eid, la[row].lm_, la[col].lm_, la[row].lmowner_, la[col].stride_ );
            strategy.AssembleMatrix2( eid, la[row].lm_, la[col].lm_, la[row].lmowner_, la[col].stride_ );
            strategy.AssembleVector1( la[row].lm_, la[row].lmowner_ );
            strategy.AssembleVector2( la[row].lm_, la[row].lmowner_ );
            strategy.AssembleVector3( la[row].lm_, la[row].lmowner_ );
          }

          // go to next element
          if (i<(mygeometrysize-1))
            ++curr;

        } // for 0 to max. geometrysize over all procs
      } // if check of condid successful
    } // if condstring found
  } //for (fool=condition_.begin(); fool!=condition_.end(); ++fool)
  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IMMERSED::ImmersedBase::SearchPotentiallyCoveredBackgrdElements(
        std::map<int,std::set<int> >* current_subset_tofill,
        Teuchos::RCP<GEO::SearchTree> backgrd_SearchTree,
        const DRT::Discretization& dis,
        const std::map<int, LINALG::Matrix<3, 1> >& currentpositions,
        const LINALG::Matrix<3, 1>& point,
        const double radius,
        const int label)
{

  *current_subset_tofill = backgrd_SearchTree->searchElementsInRadius(dis,currentpositions,point,radius,label);

  return;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IMMERSED::ImmersedBase::EvaluateSubsetElements(Teuchos::ParameterList& params,
                                                    Teuchos::RCP<DRT::Discretization> dis,
                                                    std::map<int,std::set<int> >& elementstoeval,
                                                    int action)
{
  // pointer to element
  DRT::Element* ele;

  // initialize location array
  DRT::Element::LocationArray la(1);

  for(std::map<int, std::set<int> >::const_iterator closele = elementstoeval.begin(); closele != elementstoeval.end(); closele++)
  {
    for(std::set<int>::const_iterator eleIter = (closele->second).begin(); eleIter != (closele->second).end(); eleIter++)
    {
        ele = dis->gElement(*eleIter);

        Epetra_SerialDenseMatrix dummymatrix;
        Epetra_SerialDenseVector dummyvector;
        ele->Evaluate(params,*dis,la,dummymatrix,dummymatrix,dummyvector,dummyvector,dummyvector);

    }
  }

  return;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IMMERSED::ImmersedBase::CreateGhosting(const Teuchos::RCP<DRT::Discretization>& distobeghosted)
{
  if(distobeghosted->Comm().MyPID() == 0)
  {
    std::cout<<"\n################################################################################################"<<std::endl;
    std::cout<<"###   Ghost discretization "<<distobeghosted->Name()<<" redundantly on all procs ... "<<std::endl;
    std::cout<<"################################################################################################"<<std::endl;
  }

  std::vector<int> allproc(distobeghosted->Comm().NumProc());
  for (int i=0; i<distobeghosted->Comm().NumProc(); ++i) allproc[i] = i;

  // fill my own row node ids
  const Epetra_Map* noderowmap = distobeghosted->NodeRowMap();
  std::vector<int> sdata;
  for (int i=0; i<noderowmap->NumMyElements(); ++i)
  {
    int gid = noderowmap->GID(i);
    DRT::Node* node = distobeghosted->gNode(gid);
    if (!node) dserror("ERROR: Cannot find node with gid %",gid);
    sdata.push_back(gid);
  }

  // gather all master row node gids redundantly
  std::vector<int> rdata;
  LINALG::Gather<int>(sdata,rdata,(int)allproc.size(),&allproc[0],distobeghosted->Comm());

  // build new node column map (on ALL processors)
  Teuchos::RCP<Epetra_Map> newnodecolmap = Teuchos::rcp(new Epetra_Map(-1,(int)rdata.size(),&rdata[0],0,distobeghosted->Comm()));
  sdata.clear();
  rdata.clear();

  // fill my own row element ids
  const Epetra_Map* elerowmap  = distobeghosted->ElementRowMap();
  sdata.resize(0);
  for (int i=0; i<elerowmap->NumMyElements(); ++i)
  {
    int gid = elerowmap->GID(i);
    DRT::Element* ele = distobeghosted->gElement(gid);
    if (!ele) dserror("ERROR: Cannot find element with gid %",gid);
    sdata.push_back(gid);
  }

  // gather all gids of elements redundantly
  rdata.resize(0);
  LINALG::Gather<int>(sdata,rdata,(int)allproc.size(),&allproc[0],distobeghosted->Comm());

  // build new element column map (on ALL processors)
  Teuchos::RCP<Epetra_Map> newelecolmap = Teuchos::rcp(new Epetra_Map(-1,(int)rdata.size(),&rdata[0],0,distobeghosted->Comm()));
  sdata.clear();
  rdata.clear();
  allproc.clear();

  // redistribute the discretization of the interface according to the
  // new node / element column layout (i.e. master = full overlap)
  distobeghosted->ExportColumnNodes(*newnodecolmap);
  distobeghosted->ExportColumnElements(*newelecolmap);

  // wait for all procs to finish ghosting
  distobeghosted->Comm().Barrier();
  // complete the discretization
  distobeghosted->FillComplete();

#ifdef DEBUG
  int nummycolnodes = newnodecolmap->NumMyElements();
  int nummycolelements = newelecolmap->NumMyElements();
  int sizelist[distobeghosted->Comm().NumProc()];
  distobeghosted->Comm().GatherAll(&nummycolnodes,&sizelist[0],1);
  std::cout<<"PROC "<<distobeghosted->Comm().MyPID()<<" : "<<nummycolnodes<<" colnodes"<<std::endl;
  distobeghosted->Comm().Barrier();
  std::cout<<"PROC "<<distobeghosted->Comm().MyPID()<<" : "<<nummycolelements<<" colelements"<<std::endl;
  distobeghosted->Comm().Barrier();
  std::cout<<"PROC "<<distobeghosted->Comm().MyPID()<<" : "<<distobeghosted->lColElement(0)->Nodes()[0]->Id()<<" first ID of first node of first colele"<<std::endl;
  distobeghosted->Comm().Barrier(); // wait for procs
  for(int k=1;k<distobeghosted->Comm().NumProc();++k)
  {
    if(sizelist[k-1]!=nummycolnodes)
      dserror("Since whole dis is ghosted every processor should have the same number of colnodes. This is not the case! Fix this!");
  }
#endif

}
