/*!----------------------------------------------------------------------*
\file so_nstet_nodalstrain.cpp

<pre>
Maintainer: Michael Gee
            gee@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15239
</pre>

*----------------------------------------------------------------------*/
#ifdef D_SOLID3
#ifdef CCADISCRET

#include <Teuchos_TimeMonitor.hpp>

#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_lib/drt_dserror.H"
#include "../drt_lib/drt_timecurve.H"
#include "../linalg/linalg_utils.H"
#include "Epetra_SerialDenseSolver.h"
#include "Epetra_FECrsMatrix.h"
#include "Sacado.hpp"

#include "../drt_mat/micromaterial.H"
#include "../drt_mat/stvenantkirchhoff.H"
#include "../drt_mat/lung_penalty.H"
#include "../drt_mat/lung_ogden.H"
#include "../drt_mat/neohooke.H"
#include "../drt_mat/anisotropic_balzani.H"
#include "../drt_mat/aaaneohooke.H"
#include "../drt_mat/mooneyrivlin.H"

#include "so_nstet.H"

#if 0 // FAD version, not needed for element def gradient
/*----------------------------------------------------------------------*
 |                                                             gee 10/10|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::ElementDeformationGradient(DRT::Discretization& dis)
{
  typedef Sacado::Fad::DFad<double> FAD;
  
  // current displacement
  RCP<const Epetra_Vector> disp = dis.GetState("displacement");
  if (disp==null) dserror("Cannot get state vector 'displacement'");
  // loop elements
  std::map<int,DRT::ELEMENTS::NStet*>::iterator ele;
  for (ele=elecids_.begin(); ele != elecids_.end(); ++ele)
  {
    DRT::ELEMENTS::NStet* e = ele->second;
    vector<int> lm;
    vector<int> lmowner;
    e->LocationVector(dis,lm,lmowner);
    vector<double> mydisp(lm.size());
    DRT::UTILS::ExtractMyValues(*disp,mydisp,lm);
    
    // create dfad version of nxyz and mydisp
    Teuchos::SerialDenseMatrix<int,FAD> disp(4,3,false);
    for (int i=0; i<4; ++i)
      for (int j=0; j<3; ++j)
        disp(i,j) = FAD(12,i*3+j,mydisp[i*3+j]);
    
    // create deformation gradient
    e->F() = e->BuildF<FAD>(disp,e->Nxyz());
    
  } // ele
  return;
}
#endif

/*----------------------------------------------------------------------*
 |                                                             gee 10/10|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::ElementDeformationGradient(DRT::Discretization& dis)
{
  // current displacement
  RCP<const Epetra_Vector> disp = dis.GetState("displacement");
  if (disp==null) dserror("Cannot get state vector 'displacement'");
  // loop elements
  std::map<int,DRT::ELEMENTS::NStet*>::iterator ele;
  for (ele=elecids_.begin(); ele != elecids_.end(); ++ele)
  {
    DRT::ELEMENTS::NStet* e = ele->second;
    vector<int> lm;
    vector<int> lmowner;
    e->LocationVector(dis,lm,lmowner);
    vector<double> mydisp(lm.size());
    DRT::UTILS::ExtractMyValues(*disp,mydisp,lm);
    
    // create dfad version of nxyz and mydisp
    LINALG::Matrix<4,3> disp(false);
    for (int i=0; i<4; ++i)
      for (int j=0; j<3; ++j)
        disp(i,j) = mydisp[i*3+j];
    
    // create deformation gradient
    e->F() = e->BuildF(disp,e->Nxyz());
    
  } // ele
  return;
}


void AutoDiffDemo();
void AutoDiffDemo(DRT::Discretization& dis);

/*----------------------------------------------------------------------*
 |  pre-evaluation of elements (public)                        gee 05/08|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::PreEvaluate(DRT::Discretization& dis,
                                          Teuchos::ParameterList& p,
                                          RCP<LINALG::SparseOperator> systemmatrix1,
                                          RCP<LINALG::SparseOperator> systemmatrix2,
                                          RCP<Epetra_Vector>          systemvector1,
                                          RCP<Epetra_Vector>          systemvector2,
                                          RCP<Epetra_Vector>          systemvector3)
{
  TEUCHOS_FUNC_TIME_MONITOR("DRT::ELEMENTS::NStetType::PreEvaluate");

#ifdef NSTET_TIMINGS
  const int myrank = dis.Comm().MyPID();
#endif

  // nodal integration for nlnstiff and internal forces only
  // (this method does not compute stresses/strains/element updates/mass matrix)
  string& action = p.get<string>("action","none");
  if (action != "calc_struct_nlnstiffmass" &&
      action != "calc_struct_nlnstiff"     &&
      action != "calc_struct_stress") return;

  // These get filled in here, so remove old stuff
  if (action == "calc_struct_stress")
  {
    nstress_ = Teuchos::rcp(new Epetra_MultiVector(*dis.NodeRowMap(),6,false));
    nstrain_ = Teuchos::rcp(new Epetra_MultiVector(*dis.NodeRowMap(),6,false));
    pstab_nstress_ = Teuchos::rcp(new Epetra_MultiVector(*dis.NodeRowMap(),6,true));
    pstab_nstrain_ = Teuchos::rcp(new Epetra_MultiVector(*dis.NodeRowMap(),6,true));
  }
  else
  {
    nstress_ = Teuchos::null;
    nstrain_ = Teuchos::null;
    pstab_nstress_ = Teuchos::null;
    pstab_nstrain_ = Teuchos::null;
  }

  // see what we have for input
  bool assemblemat1 = systemmatrix1!=Teuchos::null;
  bool assemblevec1 = systemvector1!=Teuchos::null;
  bool assemblevec2 = systemvector2!=Teuchos::null;
  bool assemblevec3 = systemvector3!=Teuchos::null;
  if (assemblevec2 || assemblevec3) dserror("Wrong assembly expectations");

  //-----------------------------------------------------------------
  // nodal stiffness and force (we don't do mass here)
  LINALG::SerialDenseMatrix stiff;
  LINALG::SerialDenseVector force;
  LINALG::SerialDenseMatrix mis_stiff;
  LINALG::SerialDenseVector mis_force;

  //-------------------------------------- construct F for each NStet
  //AutoDiffDemo();
  //AutoDiffDemo(dis);
#ifdef NSTET_TIMINGS
  Epetra_Time timer(dis.Comm());
  double t0 = timer.ElapsedTime();
#endif
  ElementDeformationGradient(dis);
#ifdef NSTET_TIMINGS
  double t1 = timer.ElapsedTime();
  if (!myrank) printf("Time %15.10e ElementDeformationGradient()\n",t1-t0);
#endif

  //-----------------------------------------------------------------
  // create a temporary matrix to assemble to in a baci-unusual way
  // (across-parallel-interface assembly)
  const Epetra_Map& rmap = *dis.DofRowMap();
  const Epetra_Map& dmap = rmap;

  RCP<Epetra_FECrsMatrix> stifftmp;
  RCP<LINALG::SparseMatrix> systemmatrix = rcp_dynamic_cast<LINALG::SparseMatrix>(systemmatrix1);
  if (systemmatrix != null && systemmatrix->Filled())
    stifftmp = rcp(new Epetra_FECrsMatrix(::Copy,systemmatrix->EpetraMatrix()->Graph()));
  else
    stifftmp = rcp(new Epetra_FECrsMatrix(::Copy,rmap,256,false));

  //-----------------------------------------------------------------
  // make some tests for fast assembly
  if (systemmatrix != null && systemmatrix->Filled())
  {
    Epetra_CrsMatrix& matrix = *(systemmatrix->EpetraMatrix());
    if (!matrix.StorageOptimized()) dserror("Matrix must be StorageOptimized() when Filled()");
  }

  //-----------------------------------------------------------------
  // create temporary vector in column map to assemble to
  Epetra_Vector forcetmp1(*dis.DofColMap(),true);

  //-----------------------------------------------------------------
  // current displacements
  RCP<const Epetra_Vector> disp = dis.GetState("displacement");

  //================================================== do nodal stiffness
#ifdef NSTET_TIMINGS
  double t2 = timer.ElapsedTime();
  double tmis = 0.0;
  double tnode = 0.0;
  double tassemble = 0.0;
  double tassemblemis = 0.0;
#endif
  std::map<int,DRT::Node*>::iterator node;
  for (node=noderids_.begin(); node != noderids_.end(); ++node)
  {
    DRT::Node* nodeL   = node->second;     // row node
    const int  nodeLid = nodeL->Id();
    bool mis = (pstab_adjele_.find(nodeLid) != pstab_adjele_.end());

    // standard quantities for all nodes
    vector<DRT::ELEMENTS::NStet*>& adjele = adjele_[nodeLid];
    map<int,DRT::Node*>& adjnode = adjnode_[nodeLid];
    vector<int>& lm = adjlm_[nodeLid];
    const int ndofperpatch = (int)lm.size();

    // quantities for mis nodes
    int mis_ndofperpatch = 0;
    vector<DRT::ELEMENTS::NStet*>* mis_adjele = NULL;
    map<int,DRT::Node*>* mis_adjnode = NULL;
    vector<double>* mis_weight = NULL;
    vector<int>* mis_lm = NULL;
    if (mis)
    {
      mis_adjele = &pstab_adjele_[nodeLid];
      mis_adjnode = &pstab_adjnode_[nodeLid];
      mis_weight = &pstab_adjele_weight_[nodeLid];
      mis_lm = &pstab_adjlm_[nodeLid];
      mis_ndofperpatch = (int)(*mis_lm).size();
    }

    if (action != "calc_struct_stress")
    {
      // do nodal integration of stiffness and internal force
      stiff.LightShape(ndofperpatch,ndofperpatch);
      force.LightSize(ndofperpatch);
#ifdef NSTET_TIMINGS
      double t4 = timer.ElapsedTime();
#endif
      NodalIntegration(&stiff,&force,adjnode,adjele,lm,*disp,dis,
                       NULL,NULL,INPAR::STR::stress_none,INPAR::STR::strain_none);
#ifdef NSTET_TIMINGS
      double t5 = timer.ElapsedTime();
      tnode += (t5-t4);
#endif

#ifndef PUSOSOLBERG
      if (mis)
      {
#ifdef NSTET_TIMINGS
        double t4 = timer.ElapsedTime();
#endif
        mis_stiff.LightShape(mis_ndofperpatch,mis_ndofperpatch); // put in Light once values are present
        mis_force.LightSize(mis_ndofperpatch);                   // put in Light once values are present
        MISNodalIntegration(&mis_stiff,&mis_force,*mis_adjnode,*mis_adjele,*mis_weight,*mis_lm,*disp,dis,
                            NULL,NULL,INPAR::STR::stress_none,INPAR::STR::strain_none);
#ifdef NSTET_TIMINGS
        double t5 = timer.ElapsedTime();
        tmis += (t5-t4);
#endif
      } // mis
#endif
    }
    else
    {
      INPAR::STR::StressType iostress = p.get<INPAR::STR::StressType>("iostress",INPAR::STR::stress_none);
      INPAR::STR::StrainType iostrain = p.get<INPAR::STR::StrainType>("iostrain",INPAR::STR::strain_none);
      vector<double> nodalstress(6);
      vector<double> nodalstrain(6);
#ifdef NSTET_TIMINGS
      double t4 = timer.ElapsedTime();
#endif
      NodalIntegration(NULL,NULL,adjnode,adjele,lm,*disp,dis,
                       &nodalstress,&nodalstrain,iostress,iostrain);
#ifdef NSTET_TIMINGS
      double t5 = timer.ElapsedTime();
      tnode += (t5-t4);
#endif

      const int lid = dis.NodeRowMap()->LID(nodeLid);
      if (lid==-1) dserror("Cannot find local id for row node");
      for (int i=0; i<6; ++i)
      {
        (*(*nstress_)(i))[lid] = nodalstress[i];
        (*(*nstrain_)(i))[lid] = nodalstrain[i];
      }
      
#ifndef PUSOSOLBERG
      if (mis)
      {
        vector<double> mis_nodalstress(6);
        vector<double> mis_nodalstrain(6);
#ifdef NSTET_TIMINGS
        double t4 = timer.ElapsedTime();
#endif
        MISNodalIntegration(NULL,NULL,*mis_adjnode,*mis_adjele,*mis_weight,*mis_lm,*disp,dis,
                            &mis_nodalstress,&mis_nodalstrain,iostress,iostrain);
#ifdef NSTET_TIMINGS
        double t5 = timer.ElapsedTime();
        tmis += (t5-t4);
#endif
      
        for (int i=0; i<6; ++i)
        {
          (*(*pstab_nstress_)(i))[lid] = mis_nodalstress[i];
          (*(*pstab_nstrain_)(i))[lid] = mis_nodalstrain[i];
        }
      } // mis
#endif      

    }


    //---------------------- do assembly of stiffness and internal force
    // (note: this is non-standard-baci assembly and therefore a do it all yourself version!)
    if (assemblemat1)
    {
#ifdef NSTET_TIMINGS
      double t4 = timer.ElapsedTime();
#endif
      const Epetra_Map& dofrowmap = systemmatrix->RowMap();
      const Epetra_Map& dofcolmap = systemmatrix->ColMap();
      vector<int> lrlm(ndofperpatch);
      vector<int> lclm(ndofperpatch);
      for (int i=0; i<ndofperpatch; ++i)
      {
        lrlm[i] = dofrowmap.LID(lm[i]);
        lclm[i] = dofcolmap.LID(lm[i]);
      }
      
      for (int i=0; i<ndofperpatch; ++i)
      {
        if (lrlm[i]==-1) // off-processor row
        {
          for (int j=0; j<ndofperpatch; ++j)
          {
            int errone = stifftmp->SumIntoGlobalValues(1,&lm[i],1,&lm[j],&stiff(i,j));
            if (errone>0)
            {
              int errtwo = stifftmp->InsertGlobalValues(1,&lm[i],1,&lm[j],&stiff(i,j));
              if (errtwo<0) dserror("Epetra_FECrsMatrix::InsertGlobalValues returned error code %d",errtwo);
            }
            else if (errone) dserror("Epetra_FECrsMatrix::SumIntoGlobalValues returned error code %d",errone);
          }
        }
        else // local row
        {
          if (systemmatrix != null && systemmatrix->Filled())
          {
            Epetra_CrsMatrix& matrix = *(systemmatrix->EpetraMatrix());
#if 0
            int length;
            double* values;
            int* indices;
            matrix.ExtractMyRowView(lrlm[i],length,values,indices);
#endif
            for (int j=0; j<ndofperpatch; ++j)
            {
#if 0
              int* loc = std::lower_bound(indices,indices+length,lclm[j]);
#ifdef DEBUG
              if (*loc != lclm[j]) dserror("Cannot find local column entry %d",lclm[j]);
#endif
              int pos = loc-indices;
              values[pos++] += stiff(i,j++);
              values[pos++] += stiff(i,j++);
              values[pos]   += stiff(i,j);
#else
              int err = matrix.SumIntoMyValues(lrlm[i],1,&stiff(i,j),&lclm[j]);
              if (err) dserror("Epetra_CrsMatrix::SumIntoMyValues returned err=%d",err);
#endif              
            }
          }
          else
          {
            for (int j=0; j<ndofperpatch; ++j)
              systemmatrix1->Assemble(stiff(i,j),lm[i],lm[j]);
          }
        }
      }
#ifdef NSTET_TIMINGS
      double t5 = timer.ElapsedTime();
      tassemble += (t5-t4);
#endif
      
#ifndef PUSOSOLBERG
      if (mis)
      {
#ifdef NSTET_TIMINGS
        double t4 = timer.ElapsedTime();
#endif
        lrlm.resize(mis_ndofperpatch);
        lclm.resize(mis_ndofperpatch);
        for (int i=0; i<mis_ndofperpatch; ++i)
        {
          lrlm[i] = dofrowmap.LID((*mis_lm)[i]);
          lclm[i] = dofcolmap.LID((*mis_lm)[i]);
        }
        
        for (int i=0; i<mis_ndofperpatch; ++i)
        {
          if (lrlm[i]==-1) // off processor row
          {
            for (int j=0; j<mis_ndofperpatch; ++j)
            {
              int errone = stifftmp->SumIntoGlobalValues(1,&(*mis_lm)[i],1,&(*mis_lm)[j],&mis_stiff(i,j));
              if (errone>0)
              {
                int errtwo = stifftmp->InsertGlobalValues(1,&(*mis_lm)[i],1,&(*mis_lm)[j],&mis_stiff(i,j));
                if (errtwo<0) dserror("Epetra_FECrsMatrix::InsertGlobalValues returned error code %d",errtwo);
              }
              else if (errone)
                dserror("Epetra_FECrsMatrix::SumIntoGlobalValues returned error code %d",errone);
            }
          }
          else
          {
            if (systemmatrix != null && systemmatrix->Filled())
            {
              Epetra_CrsMatrix& matrix = *(systemmatrix->EpetraMatrix());
#if 0
              int length;
              double* values;
              int* indices;
              matrix.ExtractMyRowView(lrlm[i],length,values,indices);
#endif
              for (int j=0; j<mis_ndofperpatch; ++j)
              {
#if 0
                int* loc = std::lower_bound(indices,indices+length,lclm[j]);
#ifdef DEBUG
                if (*loc != lclm[j]) dserror("Cannot find local column entry %d",lclm[j]);
#endif
                int pos = loc-indices;
                values[pos++] += mis_stiff(i,j++);
                values[pos++] += mis_stiff(i,j++);
                values[pos]   += mis_stiff(i,j);
#else
                int err = matrix.SumIntoMyValues(lrlm[i],1,&mis_stiff(i,j),&lclm[j]);
                if (err) dserror("Epetra_CrsMatrix::SumIntoMyValues returned err=%d",err);
#endif                
              }
            }
            else
            {
              for (int j=0; j<mis_ndofperpatch; ++j)
                systemmatrix1->Assemble(mis_stiff(i,j),(*mis_lm)[i],(*mis_lm)[j]);
            }
          }
        }
#ifdef NSTET_TIMINGS
        double t5 = timer.ElapsedTime();
        tassemblemis += (t5-t4);
#endif
      } // mis
#endif      
    }
    
    //-----------------------------------------------------------------------------------
    if (assemblevec1)
    {
      for (int i=0; i<ndofperpatch; ++i)
      {
        const int rgid = lm[i];
        const int lid = forcetmp1.Map().LID(rgid);
        if (lid<0) dserror("global row %d does not exist in column map",rgid);
        forcetmp1[lid] += force[i];
      }
#ifndef PUSOSOLBERG
      if (mis)
      {
        for (int i=0; i<mis_ndofperpatch; ++i)
        {
          const int rgid = (*mis_lm)[i];
          const int lid = forcetmp1.Map().LID(rgid);
          if (lid<0) dserror("global row %d does not exist in column map",rgid);
          forcetmp1[lid] += mis_force[i];
        }
      } // mis
#endif
    }

  //=========================================================================
  } // for (node=noderids_.begin(); node != noderids_.end(); ++node)
#ifdef NSTET_TIMINGS
  double t3 = timer.ElapsedTime();
  if (!myrank)
  {
    printf("Time %15.10e node integration\n",tnode);
    printf("Time %15.10e mis  integration\n",tmis);
    printf("Time %15.10e node assemble\n",tassemble);
    printf("Time %15.10e mis assemble\n",tassemblemis);
    printf("Time %15.10e Total nodal loop\n",t3-t2);
  }
#endif
  
  //-------------------------------------------------------------------------
  if (action == "calc_struct_stress")
  {
    // we have to export the nodal stresses and strains to column map
    // so they can be written by the elements
    RCP<Epetra_MultiVector> tmp = Teuchos::rcp(new Epetra_MultiVector(*dis.NodeColMap(),6,false));
    LINALG::Export(*nstress_,*tmp);
    nstress_ = tmp;
    tmp = Teuchos::rcp(new Epetra_MultiVector(*dis.NodeColMap(),6,false));
    LINALG::Export(*nstrain_,*tmp);
    nstrain_ = tmp;
    
#ifndef PUSOSOLBERG
    // export mis stress and strains to mis overlapping map to allow for output
    tmp = Teuchos::rcp(new Epetra_MultiVector(*pstab_misstressout_,6,true));
    LINALG::Export(*pstab_nstress_,*tmp);
    pstab_nstress_ = tmp;
    tmp = Teuchos::rcp(new Epetra_MultiVector(*pstab_misstressout_,6,true));
    LINALG::Export(*pstab_nstrain_,*tmp);
    pstab_nstrain_ = tmp;
#endif
  }


  //-------------------------------------------------------------------------
  // need to export forcetmp to systemvector1 and insert stiffnesses from stifftmp
  // into systemmatrix1
  // Note that fillComplete is never called on stifftmp
#ifdef NSTET_TIMINGS
  double tglobalassemble = 0.0;
  double tassemble2 = 0.0;
#endif
  if (assemblevec1)
  {
    Epetra_Vector tmp(systemvector1->Map(),false);
    Epetra_Export exporter(forcetmp1.Map(),tmp.Map());
    int err = tmp.Export(forcetmp1,exporter,Add);
    if (err) dserror("Export using exporter returned err=%d",err);
    systemvector1->Update(1.0,tmp,1.0);
  }
  if (assemblemat1)
  {
#ifdef NSTET_TIMINGS
    double t4 = timer.ElapsedTime();
#endif
    int err = stifftmp->GlobalAssemble(dmap,rmap,false);
    if (err) dserror("Epetra_FECrsMatrix::GlobalAssemble returned err=%d",err);
#ifdef NSTET_TIMINGS
    double t5 = timer.ElapsedTime();
    tglobalassemble += (t5-t4);
    t4 = timer.ElapsedTime();
#endif
    const Epetra_Map& cmap = stifftmp->ColMap();
    for (int lrow=0; lrow<stifftmp->NumMyRows(); ++lrow)
    {
      int numentries;
      double* values;
      if (!stifftmp->Filled())
      {
        const int grow = stifftmp->RowMap().GID(lrow);
        int* gindices;
        int err = stifftmp->ExtractGlobalRowView(grow,numentries,values,gindices);
        if (err) dserror("Epetra_FECrsMatrix::ExtractGlobalRowView returned err=%d",err);
        for (int j=0; j<numentries; ++j)
          systemmatrix1->Assemble(values[j],grow,gindices[j]);
      }
      else
      {
        int* lindices;
        int err = stifftmp->ExtractMyRowView(lrow,numentries,values,lindices);
        if (err) dserror("Epetra_FECrsMatrix::ExtractMyRowView returned err=%d",err);
        if (systemmatrix != null && systemmatrix->Filled())
        {
          Epetra_CrsMatrix& matrix = *systemmatrix->EpetraMatrix();
          for (int j=0; j<numentries; ++j)
          {
            int err = matrix.SumIntoMyValues(lrow,1,&values[j],&lindices[j]);
            if (err) dserror("Epetra_CrsMatrix::SumIntoMyValues returned err=%d",err);
          }
        }
        else
        {
          const int grow = stifftmp->RowMap().GID(lrow);
          for (int j=0; j<numentries; ++j)
            systemmatrix1->Assemble(values[j],grow,cmap.GID(lindices[j]));
        }
      }
    }
#ifdef NSTET_TIMINGS
    t5 = timer.ElapsedTime();
    tassemble2 += (t5-t4);
#endif
  }
#ifdef NSTET_TIMINGS
  if (!myrank)
  {
    printf("Time %15.10e GlobalAssemble\n",tglobalassemble);
    printf("Time %15.10e FeCrs to Crs assemble\n",tassemble2);
    printf("----------------------------------------\n");
  }
#endif  


  return;
}

/*----------------------------------------------------------------------*
 |  do nodal integration (public)                              gee 05/08|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::NodalIntegration(Epetra_SerialDenseMatrix*       stiff,
                                                Epetra_SerialDenseVector*       force,
                                                map<int,DRT::Node*>&            adjnode,
                                                vector<DRT::ELEMENTS::NStet*>&  adjele,
                                                vector<int>&                    lm,
                                                const Epetra_Vector&            disp,
                                                DRT::Discretization&            dis,
                                                vector<double>*                 nodalstress,
                                                vector<double>*                 nodalstrain,
                                                const INPAR::STR::StressType    iostress,
                                                const INPAR::STR::StrainType    iostrain)
{
  TEUCHOS_FUNC_TIME_MONITOR("DRT::ELEMENTS::NStetType::NodalIntegration");
  typedef Sacado::Fad::DFad<double> FAD; // for first derivs
  
  //-------------------------------------------------- standard quantities
  const int nnodeinpatch = (int)adjnode.size();
  const int ndofinpatch  = nnodeinpatch * 3;
  const int neleinpatch  = (int)adjele.size();

  //------------------------------ see whether materials in patch are equal
  bool matequal = true;
  {
    int mat = adjele[0]->material_;
    for (int i=1; i<neleinpatch; ++i)
      if (mat != adjele[i]->material_)
      {
        matequal = false;
        break;
      }
  }

  //-----------------------------------------------------------------------
  // get displacements of this patch
  vector<FAD> patchdisp(ndofinpatch);
  for (int i=0; i<ndofinpatch; ++i)
  {
    int lid = disp.Map().LID(lm[i]);
    if (lid==-1) dserror("Cannot find degree of freedom on this proc");
    patchdisp[i] = disp[disp.Map().LID(lm[i])];
    patchdisp[i].diff(i,ndofinpatch);
  }
  
  //-----------------------------------------------------------------------
  // build averaged F, det(F) and volume of node (using sacado)
  double VnodeL = 0.0;
  FAD fad_Jnode = 0.0;
  Teuchos::SerialDenseMatrix<int,FAD> fad_FnodeL(3,3,true);
  
  vector<vector<int> > lmlm(neleinpatch);
#if 0
  vector<double> Jeles(neleinpatch);
#endif
  for (int i=0; i<neleinpatch; ++i)
  {
    const double V = adjele[i]->Vol()/4;
    VnodeL += V;

    // get the element's displacements out of the patch' displacements
    vector<int> elelm;
    vector<int> lmowner;
    adjele[i]->LocationVector(dis,elelm,lmowner);

    // have to find position of elelm[i] in lm
    // lmlm[i][j] : element i, degree of freedom j, lmlm[i][j] position in patchdisp[0..ndofinpatch]
    lmlm[i].resize(12);
    for (int j=0; j<12; ++j)
    {
      vector<int>::iterator k = find(lm.begin(),lm.end(),elelm[j]);
      lmlm[i][j] = k-lm.begin(); // the position of elelm[j] in lm
    }

    // copy element disp to 4x3 format
    Teuchos::SerialDenseMatrix<int,FAD> eledispmat(4,3,false);
    for (int j=0; j<4; ++j)
      for (int k=0; k<3; ++k) 
        eledispmat(j,k) = patchdisp[lmlm[i][j*3+k]];
    
    // build F of this element
    Teuchos::SerialDenseMatrix<int,FAD> Fele = 
      adjele[i]->BuildF<FAD>(eledispmat,adjele[i]->Nxyz());
    
#if 0
    Jeles[i] = adjele[i]->Determinant3x3<FAD>(Fele).val();
#endif
    
    // add up to nodal deformation gradient
    Fele *= V;
    fad_FnodeL += Fele;
    
  } // for (int i=0; i<neleinpatch; ++i)

  // do averaging
  fad_FnodeL *= (1.0/VnodeL);

  // compute det(F)
  fad_Jnode = adjele[0]->Determinant3x3<FAD>(fad_FnodeL);
  
#if 0
  for (int i=0; i<neleinpatch; ++i)
  {
//    Jeles[i] = pow(fad_Jnode.val()/Jeles[i],-1./3.);
    Jeles[i] = pow(Jeles[i]/fad_Jnode.val(),-1./3.);
  }
#endif
  
  // copy values of fad to 'normal' values
  double Jnode = fad_Jnode.val();
  LINALG::Matrix<3,3> FnodeL(false);
  for (int j=0; j<3; ++j)
    for (int k=0; k<3; ++k)
      FnodeL(j,k) = fad_FnodeL(j,k).val();
 
  //-----------------------------------------------------------------------
  // build B operator
  Epetra_SerialDenseMatrix bop(6,ndofinpatch);
  // loop elements in patch
  for (int ele=0; ele<neleinpatch; ++ele)
  {
    // current element
    DRT::ELEMENTS::NStet* actele = adjele[ele];

    // volume of that element assigned to node L
    double V = actele->Vol()/4;
    
    // volume ratio of volume per node of this element to
    // whole volume of node L
    const double ratio = V/VnodeL;
    
    // get derivatives with respect to X
    LINALG::Matrix<4,3>& nxyz = actele->Nxyz();
    
    // get defgrd
    LINALG::Matrix<3,3>& F = actele->F();
    
    LINALG::Matrix<6,12> bele(false);
    for (int i=0; i<4; ++i)
    {
      bele(0,3*i+0) = F(0,0)*nxyz(i,0);
      bele(0,3*i+1) = F(1,0)*nxyz(i,0);
      bele(0,3*i+2) = F(2,0)*nxyz(i,0);
      bele(1,3*i+0) = F(0,1)*nxyz(i,1);
      bele(1,3*i+1) = F(1,1)*nxyz(i,1);
      bele(1,3*i+2) = F(2,1)*nxyz(i,1);
      bele(2,3*i+0) = F(0,2)*nxyz(i,2);
      bele(2,3*i+1) = F(1,2)*nxyz(i,2);
      bele(2,3*i+2) = F(2,2)*nxyz(i,2);
      
      bele(3,3*i+0) = F(0,0)*nxyz(i,1) + F(0,1)*nxyz(i,0);
      bele(3,3*i+1) = F(1,0)*nxyz(i,1) + F(1,1)*nxyz(i,0);
      bele(3,3*i+2) = F(2,0)*nxyz(i,1) + F(2,1)*nxyz(i,0);
      bele(4,3*i+0) = F(0,1)*nxyz(i,2) + F(0,2)*nxyz(i,1);
      bele(4,3*i+1) = F(1,1)*nxyz(i,2) + F(1,2)*nxyz(i,1);
      bele(4,3*i+2) = F(2,1)*nxyz(i,2) + F(2,2)*nxyz(i,1);
      bele(5,3*i+0) = F(0,2)*nxyz(i,0) + F(0,0)*nxyz(i,2);
      bele(5,3*i+1) = F(1,2)*nxyz(i,0) + F(1,0)*nxyz(i,2);
      bele(5,3*i+2) = F(2,2)*nxyz(i,0) + F(2,0)*nxyz(i,2);
    }
    
    for (int k=0; k<6; ++k)
      for (int j=0; j<12; ++j)
#if 0
        bop(k,lmlm[ele][j]) += Jeles[ele] * ratio * bele(k,j);
#else        
        bop(k,lmlm[ele][j]) += ratio * bele(k,j);
#endif
  } // for (int ele=0; ele<neleinpatch; ++ele)

  //-------------------------------------------------------------- averaged strain
  // right cauchy green
  Teuchos::SerialDenseMatrix<int,FAD> CG(3,3,false);
  CG.multiply(Teuchos::TRANS,Teuchos::NO_TRANS,1.0,fad_FnodeL,fad_FnodeL,0.0);
  vector<FAD> Ebar(6);
  Ebar[0] = 0.5 * (CG(0,0) - 1.0);
  Ebar[1] = 0.5 * (CG(1,1) - 1.0);
  Ebar[2] = 0.5 * (CG(2,2) - 1.0);
  Ebar[3] =        CG(0,1);
  Ebar[4] =        CG(1,2);
  Ebar[5] =        CG(2,0);

  // for material law and output, copy to baci object
  LINALG::Matrix<3,3> cauchygreen(false);
  for (int i=0; i<3; ++i)
    for (int j=0; j<3; ++j)
      cauchygreen(i,j) = CG(i,j).val();
  LINALG::Matrix<6,1> glstrain(false);
  glstrain(0) = Ebar[0].val();
  glstrain(1) = Ebar[1].val();
  glstrain(2) = Ebar[2].val();
  glstrain(3) = Ebar[3].val();
  glstrain(4) = Ebar[4].val();
  glstrain(5) = Ebar[5].val();
  
  //-------------------------------------------------------- output of strain
  if (iostrain != INPAR::STR::strain_none)
#ifndef PUSOSOLBERG
    StrainOutput(iostrain,*nodalstrain,FnodeL,Jnode,1.0-BETA_NSTET,1.0-ALPHA_NSTET);
#else
    StrainOutput(iostrain,*nodalstrain,FnodeL,glstrain,1.0-ALPHA_NSTET);
#endif
  
  //-------------------------------------------------------------------------
  // build a second B-operator from the averaged strains that are based on
  // the averaged F
  Epetra_SerialDenseMatrix bopbar(6,ndofinpatch);
  for (int i=0; i<ndofinpatch; ++i)
    for (int k=0; k<6; ++k)
      bopbar(k,i) = Ebar[k].fastAccessDx(i);

  //----------------------------------------- averaged material and stresses
  LINALG::Matrix<6,6> cmat(true);
  LINALG::Matrix<6,1> stress(true);

  //-----------------------------------------------------------------------
  // material law
  if (matequal) // element patch has single material
  {
    double density; // just a dummy density
    RCP<MAT::Material> mat = adjele[0]->Material();
    SelectMaterial(mat,stress,cmat,density,glstrain,FnodeL,0);
  }
  else
  {
    double density; // just a dummy density
    LINALG::Matrix<6,6> cmatele;
    LINALG::Matrix<6,1> stressele;
    for (int ele=0; ele<neleinpatch; ++ele)
    {
      cmatele = 0.0;
      stressele = 0.0;
      // current element
      DRT::ELEMENTS::NStet* actele = adjele[ele];
      // volume of that element assigned to node L
      const double V = actele->Vol()/4;
      // def-gradient of the element
      RCP<MAT::Material> mat = actele->Material();
      SelectMaterial(mat,stressele,cmatele,density,glstrain,FnodeL,0);
      cmat.Update(V,cmatele,1.0);
      stress.Update(V,stressele,1.0);
    } // for (int ele=0; ele<neleinpatch; ++ele)
    stress.Scale(1.0/VnodeL);
    cmat.Scale(1.0/VnodeL);
  }

  //-----------------------------------------------------------------------
  // stress is split as follows:
  // stress = beta * vol_misnode + (1-beta) * vol_node + (1-alpha) * dev_node + alpha * dev_ele
#ifndef PUSOSOLBERG
  {
    LINALG::Matrix<6,1> stressdev(true);
    LINALG::Matrix<6,6> cmatdev(true);
    LINALG::Matrix<6,1> stressvol(false);
    LINALG::Matrix<6,6> cmatvol(false);
  
    // compute deviatoric stress and tangent from total stress and tangent
    DevStressTangent(stressdev,cmatdev,cmat,stress,cauchygreen);
    
    // compute volumetric stress and tangent
    stressvol.Update(-1.0,stressdev,1.0,stress,0.0);
    cmatvol.Update(-1.0,cmatdev,1.0,cmat,0.0);
    
    // compute nodal stress
    stress.Update(1.0-BETA_NSTET,stressvol,1-ALPHA_NSTET,stressdev,0.0);
    cmat.Update(1.0-BETA_NSTET,cmatvol,1-ALPHA_NSTET,cmatdev,0.0);
  }
#else
  {
    stress.Scale(1.-ALPHA_NSTET);
    cmat.Scale(1.-ALPHA_NSTET);
  }
#endif  

  //-----------------------------------------------------------------------
  // stress output
  if (iostress != INPAR::STR::stress_none)
    StressOutput(iostress,*nodalstress,stress,FnodeL,Jnode);

  //----------------------------------------------------- internal forces
  if (force)
  {
    Epetra_SerialDenseVector stress_epetra(::View,stress.A(),stress.Rows());
    force->Multiply('T','N',VnodeL,bop,stress_epetra,0.0);
  }
  
  //--------------------------------------------------- elastic stiffness
  if (stiff)
  {
    Epetra_SerialDenseMatrix cmat_epetra(::View,cmat.A(),cmat.Rows(),cmat.Rows(),cmat.Columns());
    LINALG::SerialDenseMatrix cb(6,ndofinpatch);
    cb.Multiply('N','N',1.0,cmat_epetra,bopbar,0.0);
    stiff->Multiply('T','N',VnodeL,bop,cb,0.0);
  }

  //----------------------------------------------------- geom. stiffness
  // do not use sacado for second derivative of E as it is way too expensive!
  // As long as the 2nd deriv is as easy as this, do it by hand
  if (stiff)
  {
    // loop elements in patch
    for (int ele=0; ele<neleinpatch; ++ele)
    {
      // material deriv of element
      LINALG::Matrix<4,3>& nxyz = adjele[ele]->Nxyz();
      // volume of element assigned to node L
      const double V = adjele[ele]->Vol()/4;
      
      // loop nodes of that element
      double SmBL[3];
      for (int i=0; i<4; ++i)
      {
        SmBL[0] = V*(stress(0)*nxyz(i,0) + stress(3)*nxyz(i,1) + stress(5)*nxyz(i,2));
        SmBL[1] = V*(stress(3)*nxyz(i,0) + stress(1)*nxyz(i,1) + stress(4)*nxyz(i,2));
        SmBL[2] = V*(stress(5)*nxyz(i,0) + stress(4)*nxyz(i,1) + stress(2)*nxyz(i,2));
        for (int j=0; j<4; ++j)
        {
          double bopstrbop = 0.0;
          for (int dim=0; dim<3; ++dim) bopstrbop += nxyz(j,dim) * SmBL[dim];
          (*stiff)(lmlm[ele][i*3+0],lmlm[ele][j*3+0]) += bopstrbop;
          (*stiff)(lmlm[ele][i*3+1],lmlm[ele][j*3+1]) += bopstrbop;
          (*stiff)(lmlm[ele][i*3+2],lmlm[ele][j*3+2]) += bopstrbop;
        } // for (int j=0; j<4; ++j)
      } // for (int i=0; i<4; ++i)
    } // for (int ele=0; ele<neleinpatch; ++ele)
  } // if (stiff)

  return;
}


/*----------------------------------------------------------------------*
 |                                                             gee 10/10|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::MISNodalIntegration(
                           Epetra_SerialDenseMatrix*       stiff,
                           Epetra_SerialDenseVector*       force,
                           map<int,DRT::Node*>&            adjnode,
                           vector<DRT::ELEMENTS::NStet*>&  adjele,
                           vector<double>&                 weight,
                           vector<int>&                    lm,
                           const Epetra_Vector&            disp,
                           DRT::Discretization&            dis,
                           vector<double>*                 nodalstress,
                           vector<double>*                 nodalstrain,
                           const INPAR::STR::StressType    iostress,
                           const INPAR::STR::StrainType    iostrain)
{
  TEUCHOS_FUNC_TIME_MONITOR("DRT::ELEMENTS::NStetType::MISNodalIntegration");
  typedef Sacado::Fad::DFad<double> FAD; // for first derivs

  //-------------------------------------------------- standard quantities
  const int nnodeinpatch = (int)adjnode.size();
  const int ndofinpatch  = nnodeinpatch * 3;
  const int neleinpatch  = (int)adjele.size();
  
  //------------------------------ see whether materials in patch are equal
  bool matequal = true;
  {
    int mat = adjele[0]->material_;
    for (int i=1; i<neleinpatch; ++i)
      if (mat != adjele[i]->material_)
      {
        matequal = false;
        break;
      }
  }

  //-----------------------------------------------------------------------
  // get displacements of this patch
  vector<FAD> patchdisp(ndofinpatch);
  for (int i=0; i<ndofinpatch; ++i)
  {
    int lid = disp.Map().LID(lm[i]);
    if (lid==-1) dserror("Cannot find degree of freedom on this proc");
    patchdisp[i] = disp[disp.Map().LID(lm[i])];
    patchdisp[i].diff(i,ndofinpatch);
  }

  //-----------------------------------------------------------------------
  // build averaged F, det(F) and volume of node (using sacado)
  double VnodeL = 0.0;
  vector<vector<int> > lmlm(neleinpatch);
  FAD fad_Jnode = 0.0;
#if 0
  vector<double> Jeles(neleinpatch);
#endif
  for (int i=0; i<neleinpatch; ++i)
  {
    const double V = weight[i] * adjele[i]->Vol();
    VnodeL += V;
    
    // get the element's displacements out of the patch' displacements
    vector<int> elelm;
    vector<int> lmowner;
    adjele[i]->LocationVector(dis,elelm,lmowner);
    // have to find position of elelm[i] in lm
    // lmlm[i][j] : element i, degree of freedom j, lmlm[i][j] position in patchdisp[0..ndofinpatch]
    lmlm[i].resize(12);
    for (int j=0; j<12; ++j)
    {
      vector<int>::iterator k = find(lm.begin(),lm.end(),elelm[j]);
      lmlm[i][j] = k-lm.begin(); // the position of elelm[j] in lm
    }

    // copy eledisp to 4x3 format
    Teuchos::SerialDenseMatrix<int,FAD> eledispmat(4,3,false);
    for (int j=0; j<4; ++j)
      for (int k=0; k<3; ++k) 
        eledispmat(j,k) = patchdisp[lmlm[i][j*3+k]];
    
    // build F and det(F) of this element
    Teuchos::SerialDenseMatrix<int,FAD> Fele = adjele[i]->BuildF<FAD>(eledispmat,adjele[i]->nxyz_);
    FAD Jele = adjele[i]->Determinant3x3<FAD>(Fele);
    
#if 0
    Jeles[i] = Jele.val();
#endif
    
    fad_Jnode += V * Jele;

  } // for (int i=0; i<neleinpatch; ++i)

  // do averaging
  fad_Jnode /= VnodeL;
  
  FAD Jpowthird = pow(fad_Jnode,1./3.);

#if 0
  for (int i=0; i<neleinpatch; ++i)
//    Jeles[i] = pow(fad_Jnode.val()/Jeles[i],-1./3.);
    Jeles[i] = pow(Jeles[i]/fad_Jnode.val(),-1./3.);
#endif

  // build volumetric deformation gradient
  Teuchos::SerialDenseMatrix<int,FAD> fad_FnodeL(3,3,true);
  for (int i=0; i<3; ++i) 
    fad_FnodeL(i,i) = Jpowthird;
    
  // copy to baci-type objects for output of strain
  const double Jnode = fad_Jnode.val();
  LINALG::Matrix<3,3> FnodeL(false);
  for (int j=0; j<3; ++j)
    for (int k=0; k<3; ++k)
      FnodeL(j,k) = fad_FnodeL(j,k).val();
    
  //-----------------------------------------------------------------------
  // build \delta B operator, this is the unmodified operator
  Epetra_SerialDenseMatrix bop(6,ndofinpatch);
  for (int ele=0; ele<neleinpatch; ++ele)
  {
    // current element
    DRT::ELEMENTS::NStet* actele = adjele[ele];
    
    // volume of that element assigned to node L
    const double V = weight[ele] * actele->Vol();
    
    // volume ratio of volume per node of this element to
    // whole volume of node L
    const double ratio = V/VnodeL;

    // get derivatives with respect to X
    LINALG::Matrix<4,3>& nxyz = actele->Nxyz();
    
    // get defgrd
    LINALG::Matrix<3,3>& F = actele->F();

    LINALG::Matrix<6,12> bele(false);
    for (int i=0; i<4; ++i)
    {
      bele(0,3*i+0) = F(0,0)*nxyz(i,0);
      bele(0,3*i+1) = F(1,0)*nxyz(i,0);
      bele(0,3*i+2) = F(2,0)*nxyz(i,0);
      bele(1,3*i+0) = F(0,1)*nxyz(i,1);
      bele(1,3*i+1) = F(1,1)*nxyz(i,1);
      bele(1,3*i+2) = F(2,1)*nxyz(i,1);
      bele(2,3*i+0) = F(0,2)*nxyz(i,2);
      bele(2,3*i+1) = F(1,2)*nxyz(i,2);
      bele(2,3*i+2) = F(2,2)*nxyz(i,2);
      
      bele(3,3*i+0) = F(0,0)*nxyz(i,1) + F(0,1)*nxyz(i,0);
      bele(3,3*i+1) = F(1,0)*nxyz(i,1) + F(1,1)*nxyz(i,0);
      bele(3,3*i+2) = F(2,0)*nxyz(i,1) + F(2,1)*nxyz(i,0);
      bele(4,3*i+0) = F(0,1)*nxyz(i,2) + F(0,2)*nxyz(i,1);
      bele(4,3*i+1) = F(1,1)*nxyz(i,2) + F(1,2)*nxyz(i,1);
      bele(4,3*i+2) = F(2,1)*nxyz(i,2) + F(2,2)*nxyz(i,1);
      bele(5,3*i+0) = F(0,2)*nxyz(i,0) + F(0,0)*nxyz(i,2);
      bele(5,3*i+1) = F(1,2)*nxyz(i,0) + F(1,0)*nxyz(i,2);
      bele(5,3*i+2) = F(2,2)*nxyz(i,0) + F(2,0)*nxyz(i,2);
    }

    for (int k=0; k<6; ++k)
      for (int j=0; j<12; ++j)
#if 0
        bop(k,lmlm[ele][j]) += Jeles[ele] * ratio * bele(k,j);
#else
        bop(k,lmlm[ele][j]) += ratio * bele(k,j);
#endif        

  } // for (int ele=0; ele<neleinpatch; ++ele)
  
  //-----------------------------------------------------------------------
  // green-lagrange strains based on averaged volumetric F
  Teuchos::SerialDenseMatrix<int,FAD> CG(3,3,false);
  CG.multiply(Teuchos::TRANS,Teuchos::NO_TRANS,1.0,fad_FnodeL,fad_FnodeL,0.0);
  vector<FAD> Ebar(6);
  Ebar[0] = 0.5 * (CG(0,0) - 1.0);
  Ebar[1] = 0.5 * (CG(1,1) - 1.0);
  Ebar[2] = 0.5 * (CG(2,2) - 1.0);
  Ebar[3] =        CG(0,1);
  Ebar[4] =        CG(1,2);
  Ebar[5] =        CG(2,0);
  
  // for material law and output, copy to baci object
  LINALG::Matrix<3,3> cauchygreen(false);
  for (int i=0; i<3; ++i)
    for (int j=0; j<3; ++j)
      cauchygreen(i,j) = CG(i,j).val();

  LINALG::Matrix<6,1> glstrain(false);
  for (int i=0; i<6; ++i)
    glstrain(i) = Ebar[i].val();

  //-------------------------------------------------------- output of strain
  if (iostrain != INPAR::STR::strain_none)
    StrainOutput(iostrain,*nodalstrain,FnodeL,Jnode,BETA_NSTET,0.0);
  
  //-------------------------------------------------------------------------
  // build a second B-operator from the volumetric averaged strains that are based on
  // the averaged F
  Epetra_SerialDenseMatrix bopbar(6,ndofinpatch);
  for (int i=0; i<ndofinpatch; ++i)
    for (int k=0; k<6; ++k)
      bopbar(k,i) = Ebar[k].fastAccessDx(i);
  
  //----------------------------------------- averaged material and stresses
  LINALG::Matrix<6,6> cmat(true);
  LINALG::Matrix<6,1> stress(true);
  
  //-----------------------------------------------------------------------
  // material law
  if (matequal) // element patch has single material
  {
    double density; // just a dummy density
    RCP<MAT::Material> mat = adjele[0]->Material();
    SelectMaterial(mat,stress,cmat,density,glstrain,FnodeL,0);
  }
  else
  {
    double density; // just a dummy density
    LINALG::Matrix<6,6> cmatele;
    LINALG::Matrix<6,1> stressele;
    for (int ele=0; ele<neleinpatch; ++ele)
    {
      cmatele = 0.0;
      stressele = 0.0;
      // current element
      DRT::ELEMENTS::NStet* actele = adjele[ele];
      // volume of that element assigned to node L
      const double V = weight[ele] * actele->Vol();
      // def-gradient of the element
      RCP<MAT::Material> mat = actele->Material();
      SelectMaterial(mat,stressele,cmatele,density,glstrain,FnodeL,0);
      cmat.Update(V,cmatele,1.0);
      stress.Update(V,stressele,1.0);
    } // for (int ele=0; ele<neleinpatch; ++ele)
    stress.Scale(1.0/VnodeL);
    cmat.Scale(1.0/VnodeL);
  }


  //-----------------------------------------------------------------------
  // stress is split as follows:
  // stress = beta * vol_misnode + (1-beta) * vol_node + (1-alpha) * dev_node + alpha * dev_ele
  {
    LINALG::Matrix<6,1> stressdev(true);
    LINALG::Matrix<6,6> cmatdev(true);
    LINALG::Matrix<6,1> stressvol(false);
    LINALG::Matrix<6,6> cmatvol(false);
  
    // compute deviatoric stress and tangent from total stress and tangent
    DevStressTangent(stressdev,cmatdev,cmat,stress,cauchygreen);
    
    // compute volumetric stress and tangent
    stressvol.Update(-1.0,stressdev,1.0,stress,0.0);
    cmatvol.Update(-1.0,cmatdev,1.0,cmat,0.0);
    
    // compute nodal stress
    stress.Update(BETA_NSTET,stressvol,0.0);
    cmat.Update(BETA_NSTET,cmatvol,0.0);
  }

  //-----------------------------------------------------------------------
  // stress output
  if (iostress != INPAR::STR::stress_none)
    StressOutput(iostress,*nodalstress,stress,FnodeL,Jnode);

  //----------------------------------------------------- internal forces
  if (force)
  {
    Epetra_SerialDenseVector stress_epetra(::View,stress.A(),stress.Rows());
    force->Multiply('T','N',VnodeL,bop,stress_epetra,0.0);
  }

  //--------------------------------------------------- elastic stiffness
  if (stiff)
  {
    Epetra_SerialDenseMatrix cmat_epetra(::View,cmat.A(),cmat.Rows(),cmat.Rows(),cmat.Columns());
    LINALG::SerialDenseMatrix cb(6,ndofinpatch);
    cb.Multiply('N','N',1.0,cmat_epetra,bopbar,0.0);
    stiff->Multiply('T','N',VnodeL,bop,cb,0.0);
  }

  //----------------------------------------------------- geom. stiffness
  // do not use sacado for second derivative of E as it is way too expensive!
  // As long as the 2nd deriv is as easy as this, do it by hand
  if (stiff)
  {
    // loop elements in patch
    for (int ele=0; ele<neleinpatch; ++ele)
    {
      // material deriv of that element
      LINALG::Matrix<4,3>& nxyz = adjele[ele]->Nxyz();
      // volume of actele assigned to node L
      double V = weight[ele] * adjele[ele]->Vol();
      
      // loop nodes of that element
      double SmBL[3];
      for (int i=0; i<4; ++i)
      {
        SmBL[0] = V*(stress(0)*nxyz(i,0) + stress(3)*nxyz(i,1) + stress(5)*nxyz(i,2));
        SmBL[1] = V*(stress(3)*nxyz(i,0) + stress(1)*nxyz(i,1) + stress(4)*nxyz(i,2));
        SmBL[2] = V*(stress(5)*nxyz(i,0) + stress(4)*nxyz(i,1) + stress(2)*nxyz(i,2));
        for (int j=0; j<4; ++j)
        {
          double bopstrbop = 0.0;
          for (int dim=0; dim<3; ++dim) bopstrbop += nxyz(j,dim) * SmBL[dim];
          (*stiff)(lmlm[ele][i*3+0],lmlm[ele][j*3+0]) += bopstrbop;
          (*stiff)(lmlm[ele][i*3+1],lmlm[ele][j*3+1]) += bopstrbop;
          (*stiff)(lmlm[ele][i*3+2],lmlm[ele][j*3+2]) += bopstrbop;
        } // for (int j=0; j<4; ++j)
      } // for (int i=0; i<4; ++i)
    } // for (int ele=0; ele<neleinpatch; ++ele)
  } // if (stiff)
  




  return;
}










/*----------------------------------------------------------------------*
 | material laws for NStet (protected)                          gee 10/08|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::SelectMaterial(
                      RCP<MAT::Material> mat,
                      LINALG::Matrix<6,1>& stress,
                      LINALG::Matrix<6,6>& cmat,
                      double& density,
                      LINALG::Matrix<6,1>& glstrain,
                      LINALG::Matrix<3,3>& defgrd,
                      int gp)
{
  switch (mat->MaterialType())
  {
    case INPAR::MAT::m_stvenant: /*------------------ st.venant-kirchhoff-material */
    {
      MAT::StVenantKirchhoff* stvk = static_cast<MAT::StVenantKirchhoff*>(mat.get());
      stvk->Evaluate(glstrain,cmat,stress);
      density = stvk->Density();
    }
    break;
    case INPAR::MAT::m_neohooke: /*----------------- NeoHookean Material */
    {
      MAT::NeoHooke* neo = static_cast<MAT::NeoHooke*>(mat.get());
      neo->Evaluate(glstrain,cmat,stress);
      density = neo->Density();
    }
    break;
    case INPAR::MAT::m_aaaneohooke: /*-- special case of generalised NeoHookean material see Raghavan, Vorp */
    {
      MAT::AAAneohooke* aaa = static_cast<MAT::AAAneohooke*>(mat.get());
      aaa->Evaluate(glstrain,cmat,stress);
      density = aaa->Density();
    }
    break;
    case INPAR::MAT::m_lung_ogden: /* lung tissue material with Ogden for volumetric part */
    {
      MAT::LungOgden* lungog = static_cast <MAT::LungOgden*>(mat.get());
      lungog->Evaluate(&glstrain,&cmat,&stress);
      density = lungog->Density();
      return;
      break;
    }
    case INPAR::MAT::m_lung_penalty: /* lung tissue material with penalty function for incompressibility constraint */
    {
      MAT::LungPenalty* lungpen = static_cast <MAT::LungPenalty*>(mat.get());

      lungpen->Evaluate(&glstrain,&cmat,&stress);

      density = lungpen->Density();
      return;
      break;
    }
    default:
      dserror("Illegal type %d of material for element NStet tet4", mat->MaterialType());
    break;
  }

  /*--------------------------------------------------------------------*/
  return;
}  // DRT::ELEMENTS::NStet::SelectMaterial

/*----------------------------------------------------------------------*
 |  compute deviatoric tangent and stresses (private/static)   gee 06/08|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::DevStressTangent(
  LINALG::Matrix<6,1>& Sdev,
  LINALG::Matrix<6,6>& CCdev,
  LINALG::Matrix<6,6>& CC,
  const LINALG::Matrix<6,1>& S,
  const LINALG::Matrix<3,3>& C)
{

  //---------------------------------- things that we'll definitely need
  // inverse of C
  LINALG::Matrix<3,3> Cinv;
  const double detC = Cinv.Invert(C);

  // J = det(F) = sqrt(detC)
  const double J = sqrt(detC);

  // S as a 3x3 matrix
  LINALG::Matrix<3,3> Smat;
  Smat(0,0) = S(0);
  Smat(0,1) = S(3);
  Smat(0,2) = S(5);
  Smat(1,0) = Smat(0,1);
  Smat(1,1) = S(1);
  Smat(1,2) = S(4);
  Smat(2,0) = Smat(0,2);
  Smat(2,1) = Smat(1,2);
  Smat(2,2) = S(2);

  //--------------------------------------------- pressure p = -1/(3J) S:C
  double p = 0.0;
  for (int i=0; i<3; ++i)
    for (int j=0; j<3; ++j)
      p += Smat(i,j)*C(i,j);
  p *= (-1./(3.*J));

  //-------------------------------- compute volumetric PK2 Svol = -p J Cinv
  //-------------------------------------------------------- Sdev = S - Svol
  const double fac = -p*J;
  Sdev(0) = Smat(0,0) - fac*Cinv(0,0);
  Sdev(1) = Smat(1,1) - fac*Cinv(1,1);
  Sdev(2) = Smat(2,2) - fac*Cinv(2,2);
  Sdev(3) = Smat(0,1) - fac*Cinv(0,1);
  Sdev(4) = Smat(1,2) - fac*Cinv(1,2);
  Sdev(5) = Smat(0,2) - fac*Cinv(0,2);

  //======================================== volumetric tangent matrix CCvol
  LINALG::Matrix<6,6> CCvol(true); // fill with zeros

  //--------------------------------------- CCvol += 2pJ (Cinv boeppel Cinv)
  MAT::ElastSymTensor_o_Multiply(CCvol,-2.0*fac,Cinv,Cinv,0.0);

  //------------------------------------------ CCvol += 2/3 * Cinv dyad S
  MAT::ElastSymTensorMultiply(CCvol,2.0/3.0,Cinv,Smat,1.0);

  //-------------------------------------- CCvol += 1/3 Cinv dyad ( CC : C )
  {
    // C as Voigt vector
    LINALG::Matrix<6,1> Cvec;
    Cvec(0) = C(0,0);
    Cvec(1) = C(1,1);
    Cvec(2) = C(2,2);
    Cvec(3) = 2.0*C(0,1);
    Cvec(4) = 2.0*C(1,2);
    Cvec(5) = 2.0*C(0,2);

    LINALG::Matrix<6,1> CCcolonC;
    CCcolonC.Multiply(CC,Cvec);

    LINALG::Matrix<3,3> CCcC;
    CCcC(0,0) = CCcolonC(0);
    CCcC(0,1) = CCcolonC(3);
    CCcC(0,2) = CCcolonC(5);
    CCcC(1,0) = CCcC(0,1);
    CCcC(1,1) = CCcolonC(1);
    CCcC(1,2) = CCcolonC(4);
    CCcC(2,0) = CCcC(0,2);
    CCcC(2,1) = CCcC(1,2);
    CCcC(2,2) = CCcolonC(2);
    MAT::ElastSymTensorMultiply(CCvol,1./3.,Cinv,CCcC,1.0);
  }

  //----------------------------------------------------- CCdev = CC - CCvol
  CCdev.Update(1.0,CC,-1.0,CCvol);

  return;
}

/*----------------------------------------------------------------------*
 |                                                             gee 10/10|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::StrainOutput(
                    const INPAR::STR::StrainType iostrain,
                    vector<double>&              nodalstrain,
                    LINALG::Matrix<3,3>&         F,
                    const double&                detF,
                    const double                 volweight,
                    const double                 devweight)
{
  LINALG::Matrix<3,3> Fiso = F;
  Fiso.Scale(pow(detF,-1.0/3.0));
  
  LINALG::Matrix<3,3> Fvol(true);
  Fvol(0,0) = 1.0; Fvol(1,1) = 1.0; Fvol(2,2) = 1.0;
  Fvol.Scale(pow(detF,1.0/3.0));
  
  LINALG::Matrix<3,3> cauchygreeniso(false);
  cauchygreeniso.MultiplyTN(Fiso,Fiso);
  
  LINALG::Matrix<3,3> cauchygreenvol(false);
  cauchygreenvol.MultiplyTN(Fvol,Fvol);
  
  LINALG::Matrix<3,3> glstrainiso(false);
  glstrainiso(0,0) = 0.5 * (cauchygreeniso(0,0) - 1.0);
  glstrainiso(0,1) = 0.5 *  cauchygreeniso(0,1);
  glstrainiso(0,2) = 0.5 *  cauchygreeniso(0,2);
  glstrainiso(1,0) = glstrainiso(0,1);
  glstrainiso(1,1) = 0.5 * (cauchygreeniso(1,1) - 1.0);
  glstrainiso(1,2) = 0.5 *  cauchygreeniso(1,2);
  glstrainiso(2,0) = glstrainiso(0,2);
  glstrainiso(2,1) = glstrainiso(1,2);
  glstrainiso(2,2) = 0.5 * (cauchygreeniso(2,2) - 1.0);
  
  LINALG::Matrix<3,3> glstrainvol(false);
  glstrainvol(0,0) = 0.5 * (cauchygreenvol(0,0) - 1.0);
  glstrainvol(0,1) = 0.5 *  cauchygreenvol(0,1);
  glstrainvol(0,2) = 0.5 *  cauchygreenvol(0,2);
  glstrainvol(1,0) = glstrainvol(0,1);
  glstrainvol(1,1) = 0.5 * (cauchygreenvol(1,1) - 1.0);
  glstrainvol(1,2) = 0.5 *  cauchygreenvol(1,2);
  glstrainvol(2,0) = glstrainvol(0,2);
  glstrainvol(2,1) = glstrainvol(1,2);
  glstrainvol(2,2) = 0.5 * (cauchygreenvol(2,2) - 1.0);
  
  LINALG::Matrix<3,3> glstrainout = glstrainiso;
  glstrainout.Update(volweight,glstrainvol,devweight);
  
  switch (iostrain)
  {
  case INPAR::STR::strain_gl:
  {
    nodalstrain[0] = glstrainout(0,0);
    nodalstrain[1] = glstrainout(1,1);
    nodalstrain[2] = glstrainout(2,2);
    nodalstrain[3] = glstrainout(0,1);
    nodalstrain[4] = glstrainout(1,2);
    nodalstrain[5] = glstrainout(0,2);
  }
  break;
  case INPAR::STR::strain_ea:
  {
    // inverse of deformation gradient
    LINALG::Matrix<3,3> invdefgrd;
    invdefgrd.Invert(F); 
    LINALG::Matrix<3,3> temp;
    LINALG::Matrix<3,3> euler_almansi;
    temp.Multiply(glstrainout,invdefgrd);
    euler_almansi.MultiplyTN(invdefgrd,temp);
    nodalstrain[0] = euler_almansi(0,0);
    nodalstrain[1] = euler_almansi(1,1);
    nodalstrain[2] = euler_almansi(2,2);
    nodalstrain[3] = euler_almansi(0,1);
    nodalstrain[4] = euler_almansi(1,2);
    nodalstrain[5] = euler_almansi(0,2);
  }
  break;
  case INPAR::STR::strain_none:
    break;
  default:
    dserror("requested strain type not available");
  }
  
  return;
}


/*----------------------------------------------------------------------*
 |                                                             gee 10/10|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::StrainOutput(
                    const INPAR::STR::StrainType iostrain,
                    vector<double>&              nodalstrain,
                    LINALG::Matrix<3,3>&         F,
                    LINALG::Matrix<6,1>&         glstrain,
                    const double                 weight)
{
  LINALG::Matrix<3,3> glstrainout;
  
  glstrainout(0,0) = weight * glstrain(0); 
  glstrainout(1,1) = weight * glstrain(1); 
  glstrainout(2,2) = weight * glstrain(2); 
  glstrainout(0,1) = weight * glstrain(3); 
  glstrainout(1,2) = weight * glstrain(4); 
  glstrainout(0,2) = weight * glstrain(5); 
  
  
  switch (iostrain)
  {
  case INPAR::STR::strain_gl:
  {
    nodalstrain[0] = glstrainout(0,0);
    nodalstrain[1] = glstrainout(1,1);
    nodalstrain[2] = glstrainout(2,2);
    nodalstrain[3] = glstrainout(0,1);
    nodalstrain[4] = glstrainout(1,2);
    nodalstrain[5] = glstrainout(0,2);
  }
  break;
  case INPAR::STR::strain_ea:
  {
    // inverse of deformation gradient
    LINALG::Matrix<3,3> invdefgrd;
    invdefgrd.Invert(F); 
    LINALG::Matrix<3,3> temp;
    LINALG::Matrix<3,3> euler_almansi;
    temp.Multiply(glstrainout,invdefgrd);
    euler_almansi.MultiplyTN(invdefgrd,temp);
    nodalstrain[0] = euler_almansi(0,0);
    nodalstrain[1] = euler_almansi(1,1);
    nodalstrain[2] = euler_almansi(2,2);
    nodalstrain[3] = euler_almansi(0,1);
    nodalstrain[4] = euler_almansi(1,2);
    nodalstrain[5] = euler_almansi(0,2);
  }
  break;
  case INPAR::STR::strain_none:
    break;
  default:
    dserror("requested strain type not available");
  }
  
  return;
}


/*----------------------------------------------------------------------*
 |                                                             gee 10/10|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::NStetType::StressOutput(
                    const INPAR::STR::StressType iostress,
                    vector<double>&              nodalstress,
                    LINALG::Matrix<6,1>&         stress,
                    LINALG::Matrix<3,3>&         F,
                    const double&                detF)
{
  switch (iostress)
  {
  case INPAR::STR::stress_2pk:
  {
    for (int i = 0; i < 6; ++i) nodalstress[i] = stress(i);
  }
  break;
  case INPAR::STR::stress_cauchy:
  {
    LINALG::Matrix<3,3> pkstress;
    pkstress(0,0) = stress(0);
    pkstress(0,1) = stress(3);
    pkstress(0,2) = stress(5);
    pkstress(1,0) = pkstress(0,1);
    pkstress(1,1) = stress(1);
    pkstress(1,2) = stress(4);
    pkstress(2,0) = pkstress(0,2);
    pkstress(2,1) = pkstress(1,2);
    pkstress(2,2) = stress(2);
    LINALG::Matrix<3,3> temp;
    LINALG::Matrix<3,3> cauchystress;
    temp.Multiply(1.0/detF,F,pkstress);
    cauchystress.MultiplyNT(temp,F);
    nodalstress[0] = cauchystress(0,0);
    nodalstress[1] = cauchystress(1,1);
    nodalstress[2] = cauchystress(2,2);
    nodalstress[3] = cauchystress(0,1);
    nodalstress[4] = cauchystress(1,2);
    nodalstress[5] = cauchystress(0,2);
  }
  break;
  case INPAR::STR::stress_none:
    break;
  default:
    dserror("requested stress type not available");
  }
  return;
}




#endif  // #ifdef CCADISCRET
#endif  // #ifdef D_SOLID3
