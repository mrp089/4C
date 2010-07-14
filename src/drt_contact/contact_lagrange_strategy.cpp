/*!----------------------------------------------------------------------
\file contact_lagrange_strategy.cpp

<pre>
-------------------------------------------------------------------------
                        BACI Contact library
            Copyright (2008) Technical University of Munich

Under terms of contract T004.008.000 there is a non-exclusive license for use
of this work by or on behalf of Rolls-Royce Ltd & Co KG, Germany.

This library is proprietary software. It must not be published, distributed,
copied or altered in any form or any media without written permission
of the copyright holder. It may be used under terms and conditions of the
above mentioned license by or on behalf of Rolls-Royce Ltd & Co KG, Germany.

This library contains and makes use of software copyrighted by Sandia Corporation
and distributed under LGPL licence. Licensing does not apply to this or any
other third party software used here.

Questions? Contact Dr. Michael W. Gee (gee@lnm.mw.tum.de)
                   or
                   Prof. Dr. Wolfgang A. Wall (wall@lnm.mw.tum.de)

http://www.lnm.mw.tum.de

-------------------------------------------------------------------------
</pre>

<pre>
Maintainer: Alexander Popp
            popp@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15264
</pre>

*----------------------------------------------------------------------*/
#ifdef CCADISCRET

#include "Epetra_SerialComm.h"
#include "contact_lagrange_strategy.H"
#include "contact_interface.H"
#include "contact_defines.H"
#include "friction_node.H"
#include "../drt_inpar/inpar_contact.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_io/io.H"
#include "../linalg/linalg_solver.H"
#include "../linalg/linalg_utils.H"

/*----------------------------------------------------------------------*
 | ctor (public)                                              popp 05/09|
 *----------------------------------------------------------------------*/
CONTACT::CoLagrangeStrategy::CoLagrangeStrategy(RCP<Epetra_Map> problemrowmap,
                                                Teuchos::ParameterList params,
                                                vector<RCP<CONTACT::CoInterface> > interface,
                                                int dim, RCP<Epetra_Comm> comm, double alphaf) :
CoAbstractStrategy(problemrowmap,params,interface,dim,comm,alphaf),
activesetssconv_(false),
activesetconv_(false),
activesetsteps_(1)
{
	// empty constructor body
	return;
}

/*----------------------------------------------------------------------*
 | initialize global contact variables for next Newton step   popp 06/09|
 *----------------------------------------------------------------------*/
void CONTACT::CoLagrangeStrategy::Initialize()
{
  // (re)setup global normal and tangent matrices
  nmatrix_ = rcp(new LINALG::SparseMatrix(*gactiven_,3));
  tmatrix_ = rcp(new LINALG::SparseMatrix(*gactivet_,3));

  // (re)setup global matrix containing gap derivatives
  smatrix_ = rcp(new LINALG::SparseMatrix(*gactiven_,3));

  // further terms depend on friction case
  // (re)setup global matrix containing "no-friction"-derivatives
  if (!friction_)
  {
    pmatrix_ = rcp(new LINALG::SparseMatrix(*gactivet_,3));
  }
  // (re)setup of global friction
  else
  {
    // here the calculation of gstickt is necessary
    RCP<Epetra_Map> gstickt = LINALG::SplitMap(*gactivet_,*gslipt_);
    linstickLM_ = rcp(new LINALG::SparseMatrix(*gstickt,3));
    linstickDIS_ = rcp(new LINALG::SparseMatrix(*gstickt,3));
    linstickRHS_ = LINALG::CreateVector(*gstickt,true);

    linslipLM_ = rcp(new LINALG::SparseMatrix(*gslipt_,3));
    linslipDIS_ = rcp(new LINALG::SparseMatrix(*gslipt_,3));
    linslipRHS_ = LINALG::CreateVector(*gslipt_,true);
  }

  return;
}

/*----------------------------------------------------------------------*
 | evaluate frictional contact (public)                    gitterle 06/08|
 *----------------------------------------------------------------------*/
void CONTACT::CoLagrangeStrategy::EvaluateFriction(RCP<LINALG::SparseOperator>& kteff,
                                                   RCP<Epetra_Vector>& feff)
{
  // input parameters
  bool fulllin = Teuchos::getIntegralValue<int>(Params(),"FULL_LINEARIZATION");
  
  // complete stiffness matrix
  // (this is a prerequisite for the Split2x2 methods to be called later)
  kteff->Complete();

  /**********************************************************************/
  /* export weighted gap vector to gactiveN-map                         */
  /**********************************************************************/
  RCP<Epetra_Vector> gact = LINALG::CreateVector(*gactivenodes_,true);
  if (gact->GlobalLength())
  {
    LINALG::Export(*g_,*gact);
    gact->ReplaceMap(*gactiven_);
  }

  /**********************************************************************/
  /* build global matrix n with normal vectors of active nodes          */
  /* and global matrix t with tangent vectors of active nodes           */
  /* and global matrix s with normal derivatives of active nodes        */
  /* and global matrix linstick with derivatives of stick nodes         */
  /* and global matrix linslip with derivatives of slip nodes           */
  /**********************************************************************/
  // here and for the splitting later, we need the combined sm rowmap
  // (this map is NOT allowed to have an overlap !!!)
  RCP<Epetra_Map> gsmdofs = LINALG::MergeMap(gsdofrowmap_,gmdofrowmap_,false);

  for (int i=0; i<(int)interface_.size(); ++i)
  {
    interface_[i]->AssembleNT(*nmatrix_,*tmatrix_);
    interface_[i]->AssembleS(*smatrix_);
    interface_[i]->AssembleLinDM(*lindmatrix_,*linmmatrix_);
    interface_[i]->AssembleLinStick(*linstickLM_,*linstickDIS_,*linstickRHS_);
    interface_[i]->AssembleLinSlip(*linslipLM_,*linslipDIS_,*linslipRHS_);
  }
  
  // FillComplete() global matrices N and T and L
  nmatrix_->Complete(*gactivedofs_,*gactiven_);
  tmatrix_->Complete(*gactivedofs_,*gactivet_);

  // FillComplete() global matrix S
  smatrix_->Complete(*gsmdofs,*gactiven_);

  // FillComplete() global matrices LinD, LinM
  // (again for linD gsdofrowmap_ is sufficient as domain map,
  // but in the edge node modification case, master entries occur!)
  lindmatrix_->Complete(*gsmdofs,*gsdofrowmap_);
  linmmatrix_->Complete(*gsmdofs,*gmdofrowmap_);

  // FillComplete global Matrix LinStick
  RCP<Epetra_Map> gstickt = LINALG::SplitMap(*gactivet_,*gslipt_);
  RCP<Epetra_Map> gstickdofs = LINALG::SplitMap(*gactivedofs_,*gslipdofs_);
  linstickLM_->Complete(*gstickdofs,*gstickt);
  linstickDIS_->Complete(*gsmdofs,*gstickt);

  // FillComplete global Matrix linslipLM and linslipDIS
  linslipLM_->Complete(*gslipdofs_,*gslipt_);
  linslipDIS_->Complete(*gsmdofs,*gslipt_);
  
  //----------------------------------------------------------------------
	// CHECK IF WE NEED TRANSFORMATION MATRICES FOR SLAVE DISPLACEMENT DOFS
	//----------------------------------------------------------------------
	// Concretely, we apply the following transformations:
  // LinD      ---->   T^(-T) * LinD
	//----------------------------------------------------------------------
	if (Dualquadslave3d())
	{
		// modify lindmatrix_
		RCP<LINALG::SparseMatrix> temp1 = LINALG::MLMultiply(*invtrafo_,true,*lindmatrix_,false,false,false,true);
		lindmatrix_   = temp1;
	}

  // shape function and system types
  INPAR::MORTAR::ShapeFcn shapefcn = Teuchos::getIntegralValue<INPAR::MORTAR::ShapeFcn>(Params(),"SHAPEFCN");
  INPAR::CONTACT::SystemType systype = Teuchos::getIntegralValue<INPAR::CONTACT::SystemType>(Params(),"SYSTEM");
  
  //**********************************************************************
  //**********************************************************************
  // CASE A: CONDENSED SYSTEM (DUAL)
  //**********************************************************************
  //**********************************************************************
  if (systype == INPAR::CONTACT::system_condensed)
  {
    // double-check if this is a dual LM system
    if (shapefcn!=INPAR::MORTAR::shape_dual) dserror("Condensation only for dual LM");
        
    /**********************************************************************/
    /* Multiply Mortar matrices: m^ = inv(d) * m                          */
    /**********************************************************************/
    RCP<LINALG::SparseMatrix> invd = rcp(new LINALG::SparseMatrix(*dmatrix_));
    RCP<Epetra_Vector> diag = LINALG::CreateVector(*gsdofrowmap_,true);
    int err = 0;

    // extract diagonal of invd into diag
    invd->ExtractDiagonalCopy(*diag);

    // set zero diagonal values to dummy 1.0
    for (int i=0;i<diag->MyLength();++i)
      if ((*diag)[i]==0.0) (*diag)[i]=1.0;

    // scalar inversion of diagonal values
    err = diag->Reciprocal(*diag);
    if (err>0) dserror("ERROR: Reciprocal: Zero diagonal entry!");

    // re-insert inverted diagonal into invd
    err = invd->ReplaceDiagonalValues(*diag);
    // we cannot use this check, as we deliberately replaced zero entries
    //if (err>0) dserror("ERROR: ReplaceDiagonalValues: Missing diagonal entry!");

    // do the multiplication M^ = inv(D) * M
    mhatmatrix_ = LINALG::MLMultiply(*invd,false,*mmatrix_,false,false,false);

    /**********************************************************************/
    /* Add contact stiffness terms to kteff                               */
    /**********************************************************************/
    if (fulllin)
    {
      kteff->UnComplete();
      kteff->Add(*lindmatrix_,false,1.0-alphaf_,1.0);
      kteff->Add(*linmmatrix_,false,1.0-alphaf_,1.0);
      kteff->Complete();
    }

    /**********************************************************************/
    /* Split kteff into 3x3 block matrix                                  */
    /**********************************************************************/
    // we want to split k into 3 groups s,m,n = 9 blocks
    RCP<LINALG::SparseMatrix> kss, ksm, ksn, kms, kmm, kmn, kns, knm, knn;

    // temporarily we need the blocks ksmsm, ksmn, knsm
    // (FIXME: because a direct SplitMatrix3x3 is still missing!)
    RCP<LINALG::SparseMatrix> ksmsm, ksmn, knsm;

    // some temporary RCPs
    RCP<Epetra_Map> tempmap;
    RCP<LINALG::SparseMatrix> tempmtx1;
    RCP<LINALG::SparseMatrix> tempmtx2;
    RCP<LINALG::SparseMatrix> tempmtx3;

    // split into slave/master part + structure part
    RCP<LINALG::SparseMatrix> kteffmatrix = Teuchos::rcp_dynamic_cast<LINALG::SparseMatrix>(kteff);
    LINALG::SplitMatrix2x2(kteffmatrix,gsmdofs,gndofrowmap_,gsmdofs,gndofrowmap_,ksmsm,ksmn,knsm,knn);
   
    // further splits into slave part + master part
    LINALG::SplitMatrix2x2(ksmsm,gsdofrowmap_,gmdofrowmap_,gsdofrowmap_,gmdofrowmap_,kss,ksm,kms,kmm);
    LINALG::SplitMatrix2x2(ksmn,gsdofrowmap_,gmdofrowmap_,gndofrowmap_,tempmap,ksn,tempmtx1,kmn,tempmtx2);
    LINALG::SplitMatrix2x2(knsm,gndofrowmap_,tempmap,gsdofrowmap_,gmdofrowmap_,kns,knm,tempmtx1,tempmtx2);

     /**********************************************************************/
    /* Split feff into 3 subvectors                                       */
    /**********************************************************************/
    // we want to split f into 3 groups s.m,n
    RCP<Epetra_Vector> fs, fm, fn;

    // temporarily we need the group sm
    RCP<Epetra_Vector> fsm;

    // do the vector splitting smn -> sm+n -> s+m+n
    LINALG::SplitVector(*problemrowmap_,*feff,gsmdofs,fsm,gndofrowmap_,fn);
    LINALG::SplitVector(*gsmdofs,*fsm,gsdofrowmap_,fs,gmdofrowmap_,fm);
    
    // abbreviations for slave set
    int sset = gsdofrowmap_->NumGlobalElements();

    // store some stuff for static condensation of LM
    fs_   = fs;
    invd_ = invd;
    ksn_  = ksn;
    ksm_  = ksm;
    kss_  = kss;

    //----------------------------------------------------------------------
		// CHECK IF WE NEED TRANSFORMATION MATRICES FOR SLAVE DISPLACEMENT DOFS
		//----------------------------------------------------------------------
		// Concretely, we apply the following transformations:
		// D         ---->   D * T^(-1)
		// D^(-1)    ---->   T * D^(-1)
		// \hat{M}   ---->   T * \hat{M}
		//----------------------------------------------------------------------
		if (Dualquadslave3d())
		{
			dserror("ERROR: Dual LM condensation not yet fully impl. for 3D quadratic contact");

			// modify dmatrix_, invd_ and mhatmatrix_
			RCP<LINALG::SparseMatrix> temp2 = LINALG::MLMultiply(*dmatrix_,false,*invtrafo_,false,false,false,true);
			RCP<LINALG::SparseMatrix> temp3 = LINALG::MLMultiply(*trafo_,false,*invd_,false,false,false,true);
			RCP<LINALG::SparseMatrix> temp4 = LINALG::MLMultiply(*trafo_,false,*mhatmatrix_,false,false,false,true);
			dmatrix_    = temp2;
			invd_       = temp3;
			mhatmatrix_ = temp4;
		}

    /**********************************************************************/
    /* Split slave quantities into active / inactive                      */
    /**********************************************************************/
    // we want to split kssmod into 2 groups a,i = 4 blocks
    RCP<LINALG::SparseMatrix> kaa, kai, kia, kii;

    // we want to split ksn / ksm / kms into 2 groups a,i = 2 blocks
    RCP<LINALG::SparseMatrix> kan, kin, kam, kim, kma, kmi;

    // we will get the i rowmap as a by-product
    RCP<Epetra_Map> gidofs;

    // do the splitting
    LINALG::SplitMatrix2x2(kss,gactivedofs_,gidofs,gactivedofs_,gidofs,kaa,kai,kia,kii);
    LINALG::SplitMatrix2x2(ksn,gactivedofs_,gidofs,gndofrowmap_,tempmap,kan,tempmtx1,kin,tempmtx2);
    LINALG::SplitMatrix2x2(ksm,gactivedofs_,gidofs,gmdofrowmap_,tempmap,kam,tempmtx1,kim,tempmtx2);
    LINALG::SplitMatrix2x2(kms,gmdofrowmap_,tempmap,gactivedofs_,gidofs,kma,kmi,tempmtx1,tempmtx2);
    
    /**********************************************************************/
    /* Split active quantities into slip / stick                          */
    /**********************************************************************/

    // we want to split kaa into 2 groups sl,st = 4 blocks
    RCP<LINALG::SparseMatrix> kslsl, kslst, kstsl, kstst;

    // we want to split kan / kam / kai into 2 groups sl,st = 2 blocks
    RCP<LINALG::SparseMatrix> ksln, kstn, kslm, kstm, ksli, ksti;

    // some temporary RCPs
    RCP<Epetra_Map> temp1map;
    RCP<LINALG::SparseMatrix> temp1mtx1;
    RCP<LINALG::SparseMatrix> temp1mtx2;
    RCP<LINALG::SparseMatrix> temp1mtx3;
    RCP<LINALG::SparseMatrix> temp1mtx4;

    // we will get the stick rowmap as a by-product
    RCP<Epetra_Map> gstdofs;

    // do the splitting
    LINALG::SplitMatrix2x2(kaa,gslipdofs_,gstdofs,gslipdofs_,gstdofs,kslsl,kslst,kstsl,kstst);
    LINALG::SplitMatrix2x2(kan,gslipdofs_,gstdofs,gndofrowmap_,temp1map,ksln,temp1mtx1,kstn,temp1mtx2);
    LINALG::SplitMatrix2x2(kam,gslipdofs_,gstdofs,gmdofrowmap_,temp1map,kslm,temp1mtx1,kstm,temp1mtx2);
    LINALG::SplitMatrix2x2(kai,gslipdofs_,gstdofs,gidofs,temp1map,ksli,temp1mtx1,ksti,temp1mtx2);

    // abbreviations for active and inactive, stick and slip set
    int aset = gactivedofs_->NumGlobalElements();
    int iset = gidofs->NumGlobalElements();
    int stickset = gstdofs->NumGlobalElements();
    int slipset = gslipdofs_->NumGlobalElements();
    
    // we want to split fs into 2 groups a,i
    RCP<Epetra_Vector> fa = rcp(new Epetra_Vector(*gactivedofs_));
    RCP<Epetra_Vector> fi = rcp(new Epetra_Vector(*gidofs));

    // do the vector splitting s -> a+i
    LINALG::SplitVector(*gsdofrowmap_,*fs,gactivedofs_,fa,gidofs,fi);

    /**********************************************************************/
    /* Isolate active and slip part from mhat, invd and dold              */
    /* Also isolate slip part form dmatrix_, mmatrix_, dold_ and mold_    */
    /* Isolate slip part from T                                           */
    /**********************************************************************/

    RCP<LINALG::SparseMatrix> mhata;
    LINALG::SplitMatrix2x2(mhatmatrix_,gactivedofs_,gidofs,gmdofrowmap_,tempmap,mhata,tempmtx1,tempmtx2,tempmtx3);

    RCP<LINALG::SparseMatrix> invda, invdsl, invdst;
    LINALG::SplitMatrix2x2(invd_,gactivedofs_,gidofs,gactivedofs_,gidofs,invda,tempmtx1,tempmtx2,tempmtx3);
    LINALG::SplitMatrix2x2(invd_,gslipdofs_,gstdofs,gslipdofs_,gstdofs,invdsl,tempmtx1,tempmtx2,invdst);
    invda->Scale(1/(1-alphaf_));
    invdsl->Scale(1/(1-alphaf_));
    invdst->Scale(1/(1-alphaf_));

    RCP<LINALG::SparseMatrix> dolda, doldi;
    LINALG::SplitMatrix2x2(dold_,gactivedofs_,gidofs,gactivedofs_,gidofs,dolda,tempmtx1,tempmtx2,doldi);

    RCP<LINALG::SparseMatrix> dmatrixsl, doldsl, dmatrixst, doldst, mmatrixsl, mmatrixst, moldsl, moldst;
    LINALG::SplitMatrix2x2(dmatrix_,gslipdofs_,gstdofs,gslipdofs_,gstdofs,dmatrixsl,tempmtx1,tempmtx2,dmatrixst);
    LINALG::SplitMatrix2x2(dold_,gslipdofs_,gstdofs,gslipdofs_,gstdofs,doldsl,tempmtx1,tempmtx2,doldst);
    LINALG::SplitMatrix2x2(mmatrix_,gslipdofs_,gstdofs,gmdofrowmap_,tempmap,mmatrixsl,tempmtx2,mmatrixst,tempmtx3);
    LINALG::SplitMatrix2x2(mold_,gslipdofs_,gstdofs,gmdofrowmap_,tempmap,moldsl,tempmtx2,moldst,tempmtx3);

    // FIXGIT: Is this scaling really necessary
    dmatrixsl->Scale(1/(1-alphaf_));
    doldsl->Scale(1/(1-alphaf_));
    mmatrixsl->Scale(1/(1-alphaf_));
    moldsl->Scale(1/(1-alphaf_));

    // temporary RCPs
    RCP<Epetra_Map> tmap;

    // temporary RCPs
    RCP<LINALG::SparseMatrix> tm1, tm2;

    // we want to split the tmatrix_ into 2 groups
    RCP<LINALG::SparseMatrix> tslmatrix, tstmatrix;
    LINALG::SplitMatrix2x2(tmatrix_,gslipt_,gstickt,gslipdofs_,tmap,tslmatrix,tm1,tm2,tstmatrix);

    /**********************************************************************/
    /* Build the final K and f blocks                                     */
    /**********************************************************************/
    // knn: nothing to do

    // knm: nothing to do

    // kns: nothing to do

    // kmn: add T(mbaractive)*kan
    RCP<LINALG::SparseMatrix> kmnmod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
    kmnmod->Add(*kmn,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmnadd = LINALG::MLMultiply(*mhata,true,*kan,false,false,false,true);
    kmnmod->Add(*kmnadd,false,1.0,1.0);
    kmnmod->Complete(kmn->DomainMap(),kmn->RowMap());
    
    // kmm: add T(mbaractive)*kam
    RCP<LINALG::SparseMatrix> kmmmod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
    kmmmod->Add(*kmm,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmmadd = LINALG::MLMultiply(*mhata,true,*kam,false,false,false,true);
    kmmmod->Add(*kmmadd,false,1.0,1.0);
    kmmmod->Complete(kmm->DomainMap(),kmm->RowMap());

    // kmi: add T(mbaractive)*kai
    RCP<LINALG::SparseMatrix> kmimod;
    if (iset)
    {
      kmimod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
      kmimod->Add(*kmi,false,1.0,1.0);
      RCP<LINALG::SparseMatrix> kmiadd = LINALG::MLMultiply(*mhata,true,*kai,false,false,false,true);
      kmimod->Add(*kmiadd,false,1.0,1.0);
      kmimod->Complete(kmi->DomainMap(),kmi->RowMap());
    }
   
    // kma: add T(mbaractive)*kaa
    RCP<LINALG::SparseMatrix> kmamod;
    if (aset)
    {
      kmamod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
      kmamod->Add(*kma,false,1.0,1.0);
      RCP<LINALG::SparseMatrix> kmaadd = LINALG::MLMultiply(*mhata,true,*kaa,false,false,false,true);
      kmamod->Add(*kmaadd,false,1.0,1.0);
      kmamod->Complete(kma->DomainMap(),kma->RowMap());
    }

    // kin: nothing to do

    // kim: nothing to do

    // kii: nothing to do

    // kisl: nothing to do

    // kist: nothing to do

    // n*mbaractive: do the multiplication
    RCP<LINALG::SparseMatrix> nmhata;
    if (aset) nmhata = LINALG::MLMultiply(*nmatrix_,false,*mhata,false,false,false,true);

   // nmatrix: nothing to do

   // blocks for complementary conditions (stick nodes) - from LM

    // kstn: multiply with linstickLM
    RCP<LINALG::SparseMatrix> kstnmod;
    if (stickset)
    {  
      kstnmod = LINALG::MLMultiply(*linstickLM_,false,*invdst,false,false,false,true);
      kstnmod = LINALG::MLMultiply(*kstnmod,false,*kstn,false,false,false,true);
      kstnmod->Complete(kstn->DomainMap(),kstn->RowMap());
    }
    
    // kstm: multiply with linstickLM
    RCP<LINALG::SparseMatrix> kstmmod;
    if(stickset)
    {
      kstmmod = LINALG::MLMultiply(*linstickLM_,false,*invdst,false,false,false,true);
      kstmmod = LINALG::MLMultiply(*kstmmod,false,*kstm,false,false,false,false);
      kstmmod->Complete(kstm->DomainMap(),kstm->RowMap());
    }
      
    // ksti: multiply with linstickLM
    RCP<LINALG::SparseMatrix> kstimod;
    if(stickset && iset)
    {  
      kstimod = LINALG::MLMultiply(*linstickLM_,false,*invdst,false,false,false,true);
      kstimod = LINALG::MLMultiply(*kstimod,false,*ksti,false,false,false,true);
      kstimod->Complete(ksti->DomainMap(),ksti->RowMap());
    }
    // kstsl: multiply with linstickLM
    RCP<LINALG::SparseMatrix> kstslmod;
    if(stickset && slipset)
    {  
      kstslmod = LINALG::MLMultiply(*linstickLM_,false,*invdst,false,false,false,true);
      kstslmod = LINALG::MLMultiply(*kstslmod,false,*kstsl,false,false,false,true);
      kstslmod->Complete(kstsl->DomainMap(),kstsl->RowMap());
    }
    
    // kststmod: multiply with linstickLM
    RCP<LINALG::SparseMatrix> kststmod;
    if (stickset)
    {
      kststmod = LINALG::MLMultiply(*linstickLM_,false,*invdst,false,false,false,true);
      kststmod = LINALG::MLMultiply(*kststmod,false,*kstst,false,false,false,true);
      kststmod->Complete(kstst->DomainMap(),kstst->RowMap());
    }
    // blocks for complementary conditions (slip nodes) - from LM

    // ksln: multiply with linslipLM
    RCP<LINALG::SparseMatrix> kslnmod;
    if(slipset)
    {
      kslnmod = LINALG::MLMultiply(*linslipLM_,false,*invdsl,false,false,false,true);
      kslnmod = LINALG::MLMultiply(*kslnmod,false,*ksln,false,false,false,true);
      kslnmod->Complete(ksln->DomainMap(),ksln->RowMap());
    } 
    
    // kslm: multiply with linslipLM
    RCP<LINALG::SparseMatrix> kslmmod;
    if(slipset)
    {  
    kslmmod = LINALG::MLMultiply(*linslipLM_,false,*invdsl,false,false,false,true);
    kslmmod = LINALG::MLMultiply(*kslmmod,false,*kslm,false,false,false,false);
    kslmmod->Complete(kslm->DomainMap(),kslm->RowMap());
    }
    
    // ksli: multiply with linslipLM
    RCP<LINALG::SparseMatrix> kslimod;
    if (slipset && iset)
    {  
      kslimod = LINALG::MLMultiply(*linslipLM_,false,*invdsl,false,false,false,true);
      kslimod = LINALG::MLMultiply(*kslimod,false,*ksli,false,false,false,true);
      kslimod->Complete(ksli->DomainMap(),ksli->RowMap());
    }
    
    // kslsl: multiply with linslipLM
    RCP<LINALG::SparseMatrix> kslslmod;
    if(slipset)
    {
      kslslmod = LINALG::MLMultiply(*linslipLM_,false,*invdsl,false,false,false,true);
      kslslmod = LINALG::MLMultiply(*kslslmod,false,*kslsl,false,false,false,true);
      kslslmod->Complete(kslsl->DomainMap(),kslsl->RowMap());
    }
    
    // slstmod: multiply with linslipLM
    RCP<LINALG::SparseMatrix> kslstmod;
    if (slipset && stickset)
    {  
      kslstmod = LINALG::MLMultiply(*linslipLM_,false,*invdsl,false,false,false,true);
      kslstmod = LINALG::MLMultiply(*kslstmod,false,*kslst,false,false,false,true);
      kslstmod->Complete(kslst->DomainMap(),kslst->RowMap());
    }
    
    // fn: nothing to do

    // fi: subtract alphaf * old contact forces (t_n)
    if (iset)
    {
      RCP<Epetra_Vector> modi = rcp(new Epetra_Vector(*gidofs));
      LINALG::Export(*zold_,*modi);
      RCP<Epetra_Vector> tempveci = rcp(new Epetra_Vector(*gidofs));
      doldi->Multiply(false,*modi,*tempveci);
      fi->Update(-alphaf_,*tempveci,1.0);
    }

    // fa: subtract alphaf * old contact forces (t_n)
    if (aset)
    {
      RCP<Epetra_Vector> mod = rcp(new Epetra_Vector(*gactivedofs_));
      LINALG::Export(*zold_,*mod);
      RCP<Epetra_Vector> tempvec = rcp(new Epetra_Vector(*gactivedofs_));
      dolda->Multiply(false,*mod,*tempvec);
      fa->Update(-alphaf_,*tempvec,1.0);
    }

    // we want to split famod into 2 groups sl,st
    RCP<Epetra_Vector> fsl, fst;

    // do the vector splitting a -> sl+st
    if(aset)
      LINALG::SplitVector(*gactivedofs_,*fa,gslipdofs_,fsl,gstdofs,fst);

    // fm: add alphaf * old contact forces (t_n)
    RCP<Epetra_Vector> tempvecm = rcp(new Epetra_Vector(*gmdofrowmap_));
    mold_->Multiply(true,*zold_,*tempvecm);
    fm->Update(alphaf_,*tempvecm,1.0);

    // fm: add T(mbaractive)*fa
    RCP<Epetra_Vector> fmmod = rcp(new Epetra_Vector(*gmdofrowmap_));
    if (aset) mhata->Multiply(true,*fa,*fmmod);
    fmmod->Update(1.0,*fm,1.0);

    // fst: mutliply with linstickLM
    // (this had to wait as we had to modify fm first)
    RCP<Epetra_Vector> fstmod;
    if (stickset)
    {  
      fstmod = rcp(new Epetra_Vector(*gstickt));
      RCP<LINALG::SparseMatrix> temp1 = LINALG::MLMultiply(*linstickLM_,false,*invdst,false,false,false,true);
      temp1->Multiply(false,*fst,*fstmod);
    }
    
    // fsl: mutliply with linslipLM
    // (this had to wait as we had to modify fm first)
    RCP<Epetra_Vector> fslmod;
    if (slipset)
    {  
      fslmod = rcp(new Epetra_Vector(*gslipt_));
      RCP<LINALG::SparseMatrix> temp = LINALG::MLMultiply(*linslipLM_,false,*invdsl,false,false,false,true);
      temp->Multiply(false,*fsl,*fslmod);
    }
    
    // gactive: nothing to do

    /**********************************************************************/
    /* Global setup of kteffnew, feffnew (including contact)              */
    /**********************************************************************/
    RCP<LINALG::SparseMatrix> kteffnew = rcp(new LINALG::SparseMatrix(*problemrowmap_,81,true,false,kteffmatrix->GetMatrixtype()));
    RCP<Epetra_Vector> feffnew = LINALG::CreateVector(*problemrowmap_);

    // add n submatrices to kteffnew
    kteffnew->Add(*knn,false,1.0,1.0);
    kteffnew->Add(*knm,false,1.0,1.0);
    if (sset) kteffnew->Add(*kns,false,1.0,1.0);

    // add m submatrices to kteffnew
    kteffnew->Add(*kmnmod,false,1.0,1.0);
    kteffnew->Add(*kmmmod,false,1.0,1.0);
    if (iset) kteffnew->Add(*kmimod,false,1.0,1.0);
    if (aset) kteffnew->Add(*kmamod,false,1.0,1.0);

    // add i submatrices to kteffnew
    if (iset) kteffnew->Add(*kin,false,1.0,1.0);
    if (iset) kteffnew->Add(*kim,false,1.0,1.0);
    if (iset) kteffnew->Add(*kii,false,1.0,1.0);
    if (iset && aset) kteffnew->Add(*kia,false,1.0,1.0);

    // add matrices n and nmhata to kteffnew
    // this is only done for the "NO full linearization" case
    if (!fulllin)
    {
      if (aset) kteffnew->Add(*nmatrix_,false,1.0,1.0);
      if (aset) kteffnew->Add(*nmhata,false,-1.0,1.0);
    }

    // add full linearization terms to kteffnew
    if (fulllin)
    {
     if (aset) kteffnew->Add(*smatrix_,false,-1.0,1.0);
    }

    // add terms of linearization of sick condition to kteffnew
    if (stickset) kteffnew->Add(*linstickDIS_,false,-1.0,1.0);

    // add terms of linearization of slip condition to kteffnew and feffnew
    if (slipset)
    {
      kteffnew->Add(*linslipDIS_,false,-1.0,+1.0);

      RCP<Epetra_Vector> linslipRHSexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*linslipRHS_,*linslipRHSexp);
      feffnew->Update(-1.0,*linslipRHSexp,1.0);
    }

    // add terms of linearization feffnew
    // this is done also for evalutating the relative velocity with material
    // velocities
     if (stickset)
     {
        RCP<Epetra_Vector> linstickRHSexp = rcp(new Epetra_Vector(*problemrowmap_));
        LINALG::Export(*linstickRHS_,*linstickRHSexp);
        feffnew->Update(-1.0,*linstickRHSexp,1.0);
     }
     
     // add a submatrices to kteffnew
     if (stickset) kteffnew->Add(*kstnmod,false,1.0,1.0);
     if (stickset) kteffnew->Add(*kstmmod,false,1.0,1.0);
     if (stickset && iset) kteffnew->Add(*kstimod,false,1.0,1.0);
     if (stickset && slipset) kteffnew->Add(*kstslmod,false,1.0,1.0);
     if (stickset) kteffnew->Add(*kststmod,false,1.0,1.0);

    // add a submatrices to kteffnew
    if (slipset) kteffnew->Add(*kslnmod,false,1.0,1.0);
    if (slipset) kteffnew->Add(*kslmmod,false,1.0,1.0);
    if (slipset && iset) kteffnew->Add(*kslimod,false,1.0,1.0);
    if (slipset) kteffnew->Add(*kslslmod,false,1.0,1.0);
    if (slipset && stickset) kteffnew->Add(*kslstmod,false,1.0,1.0);
    
    // FillComplete kteffnew (square)
    kteffnew->Complete();

    // add n subvector to feffnew
    RCP<Epetra_Vector> fnexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fn,*fnexp);
    feffnew->Update(1.0,*fnexp,1.0);

    // add m subvector to feffnew
    RCP<Epetra_Vector> fmmodexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fmmod,*fmmodexp);
    feffnew->Update(1.0,*fmmodexp,1.0);

    // add i and sl subvector to feffnew
    RCP<Epetra_Vector> fiexp;
    if (iset)
    {
      fiexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*fi,*fiexp);
      feffnew->Update(1.0,*fiexp,1.0);
    }

    // add a subvector to feffnew
    RCP<Epetra_Vector> fstmodexp;
    if (stickset)
    {
      fstmodexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*fstmod,*fstmodexp);
      feffnew->Update(1.0,*fstmodexp,+1.0);
    }
    
    // add a subvector to feffnew
    RCP<Epetra_Vector> fslmodexp;
    if (slipset)
    {
      fslmodexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*fslmod,*fslmodexp);
      feffnew->Update(1.0,*fslmodexp,1.0);
    }
    
    // add weighted gap vector to feffnew, if existing
    RCP<Epetra_Vector> gexp;
    if (aset)
    {
      gexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*gact,*gexp);
      feffnew->Update(1.0,*gexp,1.0);
    }

    /**********************************************************************/
    /* Replace kteff and feff by kteffnew and feffnew                     */
    /**********************************************************************/
    kteff = kteffnew;
    feff = feffnew;
  }
  
  //**********************************************************************
  //**********************************************************************
  // CASE B: SADDLE POINT SYSTEM
  //**********************************************************************
  //**********************************************************************
  else
  {
    //----------------------------------------------------------------------
  	// CHECK IF WE NEED TRANSFORMATION MATRICES FOR SLAVE DISPLACEMENT DOFS
  	//----------------------------------------------------------------------
  	// Concretely, we apply the following transformations:
  	// D         ---->   D * T^(-1)
  	//----------------------------------------------------------------------
  	if (Dualquadslave3d())
  	{
  		// modify dmatrix_
  		RCP<LINALG::SparseMatrix> temp2 = LINALG::MLMultiply(*dmatrix_,false,*invtrafo_,false,false,false,true);
  		dmatrix_    = temp2;
  	}

  	// add contact stiffness
    kteff->UnComplete();
    kteff->Add(*lindmatrix_,false,1.0-alphaf_,1.0);
    kteff->Add(*linmmatrix_,false,1.0-alphaf_,1.0);
    kteff->Complete();
    
    // add contact force terms
    RCP<Epetra_Vector> fs = rcp(new Epetra_Vector(*gsdofrowmap_));
    dmatrix_->Multiply(true,*z_,*fs);
    RCP<Epetra_Vector> fsexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fs,*fsexp);
    feff->Update(-(1.0-alphaf_),*fsexp,1.0);
    
    RCP<Epetra_Vector> fm = rcp(new Epetra_Vector(*gmdofrowmap_));
    mmatrix_->Multiply(true,*z_,*fm);
    RCP<Epetra_Vector> fmexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fm,*fmexp);
    feff->Update(1.0-alphaf_,*fmexp,1.0);

    // add old contact forces (t_n)
    RCP<Epetra_Vector> fsold = rcp(new Epetra_Vector(*gsdofrowmap_));
    dold_->Multiply(true,*zold_,*fsold);
    RCP<Epetra_Vector> fsoldexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fsold,*fsoldexp);
    feff->Update(-alphaf_,*fsoldexp,1.0);

    RCP<Epetra_Vector> fmold = rcp(new Epetra_Vector(*gmdofrowmap_));
    mold_->Multiply(true,*zold_,*fmold);
    RCP<Epetra_Vector> fmoldexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fmold,*fmoldexp);
    feff->Update(alphaf_,*fmoldexp,1.0);
  }
    

#ifdef CONTACTFDSTICK

  if (gstickt->NumGlobalElements())
  {
    // FD check of stick condition
    for (int i=0; i<(int)interface_.size(); ++i)
    {
      RCP<LINALG::SparseMatrix> deriv1 = rcp(new LINALG::SparseMatrix(*gactivet_,81));
      RCP<LINALG::SparseMatrix> deriv2 = rcp(new LINALG::SparseMatrix(*gactivet_,81));

      deriv1->Add(*linstickLM_,false,1.0,1.0);
      deriv1->Complete(*gsmdofs,*gactivet_);

      deriv2->Add(*linstickDIS_,false,1.0,1.0);
      deriv2->Complete(*gsmdofs,*gactivet_);

      cout << *deriv1 << endl;
      cout << *deriv2 << endl;

      interface_[i]->FDCheckStickDeriv();
    }
  }
#endif // #ifdef CONTACTFDSTICK

#ifdef CONTACTFDSLIP

  if (gslipnodes_->NumGlobalElements())
  {
    // FD check of slip condition
    for (int i=0; i<(int)interface_.size(); ++i)
    {
      RCP<LINALG::SparseMatrix> deriv1 = rcp(new LINALG::SparseMatrix(*gactivet_,81));
      RCP<LINALG::SparseMatrix> deriv2 = rcp(new LINALG::SparseMatrix(*gactivet_,81));
  
      deriv1->Add(*linslipLM_,false,1.0,1.0);
      deriv1->Complete(*gsmdofs,*gslipt_);
  
      deriv2->Add(*linslipDIS_,false,1.0,1.0);
      deriv2->Complete(*gsmdofs,*gslipt_);
  
      cout << *deriv1 << endl;
      cout << *deriv2 << endl;
  
      interface_[i]->FDCheckSlipDeriv();
    }
  }
#endif // #ifdef CONTACTFDSLIP
  
  return;
}

/*----------------------------------------------------------------------*
 |  evaluate contact (public)                                 popp 04/08|
 *----------------------------------------------------------------------*/
void CONTACT::CoLagrangeStrategy::EvaluateContact(RCP<LINALG::SparseOperator>& kteff,
                                                  RCP<Epetra_Vector>& feff)
{
  // input parameters
  bool fulllin = Teuchos::getIntegralValue<int>(Params(),"FULL_LINEARIZATION");

  // complete stiffness matrix
  // (this is a prerequisite for the Split2x2 methods to be called later)
  kteff->Complete();
  
  /**********************************************************************/
  /* export weighted gap vector to gactiveN-map                         */
  /**********************************************************************/
  RCP<Epetra_Vector> gact = LINALG::CreateVector(*gactivenodes_,true);
  if (gact->GlobalLength())
  {
    LINALG::Export(*g_,*gact);
    gact->ReplaceMap(*gactiven_);
  }

  /**********************************************************************/
  /* build global matrix n with normal vectors of active nodes          */
  /* and global matrix t with tangent vectors of active nodes           */
  /* and global matrix s with normal derivatives of active nodes        */
  /**********************************************************************/
  // here and for the splitting later, we need the combined sm rowmap
  // (this map is NOT allowed to have an overlap !!!)
  RCP<Epetra_Map> gsmdofs = LINALG::MergeMap(gsdofrowmap_,gmdofrowmap_,false);

  for (int i=0; i<(int)interface_.size(); ++i)
  {
    interface_[i]->AssembleNT(*nmatrix_,*tmatrix_);
    interface_[i]->AssembleS(*smatrix_);
    interface_[i]->AssembleP(*pmatrix_);
    interface_[i]->AssembleLinDM(*lindmatrix_,*linmmatrix_);
  }

  // FillComplete() global matrices N and T
  nmatrix_->Complete(*gactivedofs_,*gactiven_);
  tmatrix_->Complete(*gactivedofs_,*gactivet_);

  // FillComplete() global matrix S
  smatrix_->Complete(*gsmdofs,*gactiven_);

  // FillComplete() global matrix P
  // (actually gsdofrowmap_ is in general sufficient as domain map,
  // but in the edge node modification case, master entries occur!)
  pmatrix_->Complete(*gsmdofs,*gactivet_);

  // FillComplete() global matrices LinD, LinM
  // (again for linD gsdofrowmap_ is sufficient as domain map,
  // but in the edge node modification case, master entries occur!)
  lindmatrix_->Complete(*gsmdofs,*gsdofrowmap_);
  linmmatrix_->Complete(*gsmdofs,*gmdofrowmap_);

  //----------------------------------------------------------------------
	// CHECK IF WE NEED TRANSFORMATION MATRICES FOR SLAVE DISPLACEMENT DOFS
	//----------------------------------------------------------------------
	// Concretely, we apply the following transformations:
  // LinD      ---->   T^(-T) * LinD
	//----------------------------------------------------------------------
	if (Dualquadslave3d())
	{
		// modify lindmatrix_
		RCP<LINALG::SparseMatrix> temp1 = LINALG::MLMultiply(*invtrafo_,true,*lindmatrix_,false,false,false,true);
		lindmatrix_   = temp1;
	}

  // shape function and system types
  INPAR::MORTAR::ShapeFcn shapefcn = Teuchos::getIntegralValue<INPAR::MORTAR::ShapeFcn>(Params(),"SHAPEFCN");
  INPAR::CONTACT::SystemType systype = Teuchos::getIntegralValue<INPAR::CONTACT::SystemType>(Params(),"SYSTEM");
  
  //**********************************************************************
  //**********************************************************************
  // CASE A: CONDENSED SYSTEM (DUAL)
  //**********************************************************************
  //**********************************************************************
  if (systype == INPAR::CONTACT::system_condensed)
  {
    // double-check if this is a dual LM system
    if (shapefcn!=INPAR::MORTAR::shape_dual) dserror("Condensation only for dual LM");
    
#ifdef CONTACTBASISTRAFO
    /**********************************************************************/
    /* Multiply Mortar matrices: m^ = inv(d) * m                          */
    /**********************************************************************/
    RCP<LINALG::SparseMatrix> invd = rcp(new LINALG::SparseMatrix(*dmatrix_));
    RCP<Epetra_Vector> diag = LINALG::CreateVector(*gsdofrowmap_,true);
    int err = 0;

    // extract diagonal of invd into diag
    invd->ExtractDiagonalCopy(*diag);

    // set zero diagonal values to dummy 1.0
    for (int i=0;i<diag->MyLength();++i)
      if ((*diag)[i]==0.0) (*diag)[i]=1.0;

    // scalar inversion of diagonal values
    err = diag->Reciprocal(*diag);
    if (err>0) dserror("ERROR: Reciprocal: Zero diagonal entry!");

    // re-insert inverted diagonal into invd
    err = invd->ReplaceDiagonalValues(*diag);
    // we cannot use this check, as we deliberately replaced zero entries
    //if (err>0) dserror("ERROR: ReplaceDiagonalValues: Missing diagonal entry!");

    // do the multiplication M^ = inv(D) * M
    mhatmatrix_ = LINALG::MLMultiply(*invd,false,*mmatrix_,false,false,false,true);

    /**********************************************************************/
    /* Add contact stiffness terms to kteff                               */
    /**********************************************************************/
    if (fulllin)
    {
      kteff->UnComplete();
      kteff->Add(*lindmatrix_,false,1.0-alphaf_,1.0);
      kteff->Add(*linmmatrix_,false,1.0-alphaf_,1.0);
      kteff->Complete();
    }

    /**********************************************************************/
    /* Split kteff into 3x3 block matrix                                  */
    /**********************************************************************/
    // we want to split k into 3 groups s,m,n = 9 blocks
    RCP<LINALG::SparseMatrix> kss, ksm, ksn, kms, kmm, kmn, kns, knm, knn;

    // temporarily we need the blocks ksmsm, ksmn, knsm
    // (FIXME: because a direct SplitMatrix3x3 is still missing!)
    RCP<LINALG::SparseMatrix> ksmsm, ksmn, knsm;

    // some temporary RCPs
    RCP<Epetra_Map> tempmap;
    RCP<LINALG::SparseMatrix> tempmtx1;
    RCP<LINALG::SparseMatrix> tempmtx2;
    RCP<LINALG::SparseMatrix> tempmtx3;

    // split into slave/master part + structure part
    RCP<LINALG::SparseMatrix> kteffmatrix = Teuchos::rcp_dynamic_cast<LINALG::SparseMatrix>(kteff);
    LINALG::SplitMatrix2x2(kteffmatrix,gsmdofs,gndofrowmap_,gsmdofs,gndofrowmap_,ksmsm,ksmn,knsm,knn);

    // further splits into slave part + master part
    LINALG::SplitMatrix2x2(ksmsm,gsdofrowmap_,gmdofrowmap_,gsdofrowmap_,gmdofrowmap_,kss,ksm,kms,kmm);
    LINALG::SplitMatrix2x2(ksmn,gsdofrowmap_,gmdofrowmap_,gndofrowmap_,tempmap,ksn,tempmtx1,kmn,tempmtx2);
    LINALG::SplitMatrix2x2(knsm,gndofrowmap_,tempmap,gsdofrowmap_,gmdofrowmap_,kns,knm,tempmtx1,tempmtx2);

    /**********************************************************************/
    /* Split feff into 3 subvectors                                       */
    /**********************************************************************/
    // we want to split f into 3 groups s.m,n
    RCP<Epetra_Vector> fs, fm, fn;

    // temporarily we need the group sm
    RCP<Epetra_Vector> fsm;

    // do the vector splitting smn -> sm+n
    LINALG::SplitVector(*problemrowmap_,*feff,gsmdofs,fsm,gndofrowmap_,fn);

    // abbreviations for slave  and master set
    int sset = gsdofrowmap_->NumGlobalElements();
    int mset = gmdofrowmap_->NumGlobalElements();
    
    // we want to split fsm into 2 groups s,m
    fs = rcp(new Epetra_Vector(*gsdofrowmap_));
    fm = rcp(new Epetra_Vector(*gmdofrowmap_));

    // do the vector splitting sm -> s+m
    LINALG::SplitVector(*gsmdofs,*fsm,gsdofrowmap_,fs,gmdofrowmap_,fm);

    // store some stuff for static condensation of LM
    fs_   = fs;
    invd_ = invd;
    ksn_  = ksn;
    ksm_  = ksm;
    kss_  = kss;

    //----------------------------------------------------------------------
		// CHECK IF WE NEED TRANSFORMATION MATRICES FOR SLAVE DISPLACEMENT DOFS
		//----------------------------------------------------------------------
		// Concretely, we apply the following transformations:
		// D         ---->   D * T^(-1)
		// D^(-1)    ---->   T * D^(-1)
		// \hat{M}   ---->   T * \hat{M}
		//----------------------------------------------------------------------
		if (Dualquadslave3d())
		{
			dserror("ERROR: Dual LM condensation with basis transformation not yet impl. for 3D quadratic contact");

			// modify dmatrix_, invd_ and mhatmatrix_
			RCP<LINALG::SparseMatrix> temp2 = LINALG::MLMultiply(*dmatrix_,false,*invtrafo_,false,false,false,true);
			RCP<LINALG::SparseMatrix> temp3 = LINALG::MLMultiply(*trafo_,false,*invd_,false,false,false,true);
			RCP<LINALG::SparseMatrix> temp4 = LINALG::MLMultiply(*trafo_,false,*mhatmatrix_,false,false,false,true);
			dmatrix_    = temp2;
			invd_       = temp3;
			mhatmatrix_ = temp4;
		}

    /**********************************************************************/
    /* Split slave quantities into active / inactive                      */
    /**********************************************************************/
    // we want to split kssmod into 2 groups a,i = 4 blocks
    RCP<LINALG::SparseMatrix> kaa, kai, kia, kii, kas, kis;

    // we want to split ksn / ksm / kms into 2 groups a,i = 2 blocks
    RCP<LINALG::SparseMatrix> kan, kin, kam, kim, kma, kmi;

    // we will get the i rowmap as a by-product
    RCP<Epetra_Map> gidofs;

    // do the splitting
    LINALG::SplitMatrix2x2(kss,gactivedofs_,gidofs,gsdofrowmap_,tempmap,kas,tempmtx1,kis,tempmtx2);
    LINALG::SplitMatrix2x2(kss,gactivedofs_,gidofs,gactivedofs_,gidofs,kaa,kai,kia,kii);
    LINALG::SplitMatrix2x2(ksn,gactivedofs_,gidofs,gndofrowmap_,tempmap,kan,tempmtx1,kin,tempmtx2);
    LINALG::SplitMatrix2x2(ksm,gactivedofs_,gidofs,gmdofrowmap_,tempmap,kam,tempmtx1,kim,tempmtx2);
    LINALG::SplitMatrix2x2(kms,gmdofrowmap_,tempmap,gactivedofs_,gidofs,kma,kmi,tempmtx1,tempmtx2);

    // abbreviations for active and inactive set
    int aset = gactivedofs_->NumGlobalElements();
    int iset = gidofs->NumGlobalElements();
    
    // we want to split fsmod into 2 groups a,i
    RCP<Epetra_Vector> fa = rcp(new Epetra_Vector(*gactivedofs_));
    RCP<Epetra_Vector> fi = rcp(new Epetra_Vector(*gidofs));

    // do the vector splitting s -> a+i
    LINALG::SplitVector(*gsdofrowmap_,*fs,gactivedofs_,fa,gidofs,fi);

    /**********************************************************************/
    /* Isolate active part from mhat and invd                             */
    /**********************************************************************/
    RCP<LINALG::SparseMatrix> mhata;
    LINALG::SplitMatrix2x2(mhatmatrix_,gactivedofs_,gidofs,gmdofrowmap_,tempmap,mhata,tempmtx1,tempmtx2,tempmtx3);

    RCP<LINALG::SparseMatrix> invda;
    LINALG::SplitMatrix2x2(invd_,gactivedofs_,gidofs,gactivedofs_,gidofs,invda,tempmtx1,tempmtx2,tempmtx3);
    invda->Scale(1/(1-alphaf_));

    /**********************************************************************/
    /* Split constraint terms into master and slave part                  */
    /**********************************************************************/
    // we want to split smatrix and pmatrix
    RCP<LINALG::SparseMatrix> smatrixm, smatrixs, pmatrixm, pmatrixs;
    
    // do the splitting
    LINALG::SplitMatrix2x2(smatrix_,gactiven_,tempmap,gmdofrowmap_,gsdofrowmap_,smatrixm,smatrixs,tempmtx1,tempmtx2);
    LINALG::SplitMatrix2x2(pmatrix_,gactivet_,tempmap,gmdofrowmap_,gsdofrowmap_,pmatrixm,pmatrixs,tempmtx1,tempmtx2);
    
    /**********************************************************************/
    /* Build the final K and f blocks                                     */
    /**********************************************************************/
    // knn: nothing to do

    // knm: add kns*mhat
    RCP<LINALG::SparseMatrix> knmmod = rcp(new LINALG::SparseMatrix(*gndofrowmap_,100));
    knmmod->Add(*knm,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> knmadd = LINALG::MLMultiply(*kns,false,*mhatmatrix_,false,false,false,true);
    knmmod->Add(*knmadd,false,1.0,1.0);
    knmmod->Complete(knm->DomainMap(),knm->RowMap());

    // kns: nothing to do

    // kmn: add T(mhat)*ksn
    RCP<LINALG::SparseMatrix> kmnmod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
    kmnmod->Add(*kmn,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmnadd = LINALG::MLMultiply(*mhatmatrix_,true,*ksn,false,false,false,true);
    kmnmod->Add(*kmnadd,false,1.0,1.0);
    kmnmod->Complete(kmn->DomainMap(),kmn->RowMap());

    // kmm: add kms*mhat and T(mhat)*ksm and T(mhat)*kss*mhat
    RCP<LINALG::SparseMatrix> kmmmod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
    kmmmod->Add(*kmm,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmmadd1 = LINALG::MLMultiply(*kms,false,*mhatmatrix_,false,false,false,true);
    kmmmod->Add(*kmmadd1,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmmadd2 = LINALG::MLMultiply(*mhatmatrix_,true,*ksm,false,false,false,true);
    kmmmod->Add(*kmmadd2,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmmadd3 = LINALG::MLMultiply(*kss,false,*mhatmatrix_,false,false,false,true);
    kmmadd3 = LINALG::MLMultiply(*mhatmatrix_,true,*kmmadd3,false,false,false,true);
    kmmmod->Add(*kmmadd3,false,1.0,1.0);
    kmmmod->Complete(kmm->DomainMap(),kmm->RowMap());
    
    // kms: add T(mhat)*kss
    RCP<LINALG::SparseMatrix> kmsmod;
    kmsmod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
    kmsmod->Add(*kms,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmsadd = LINALG::MLMultiply(*mhatmatrix_,true,*kss,false,false,false,true);
    kmsmod->Add(*kmsadd,false,1.0,1.0);
    kmsmod->Complete(kms->DomainMap(),kms->RowMap());

    // kin: nothing to do

    // kim: add kis*mhat
    RCP<LINALG::SparseMatrix> kimmod = rcp(new LINALG::SparseMatrix(*gidofs,100));
    kimmod->Add(*kim,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kimadd = LINALG::MLMultiply(*kis,false,*mhatmatrix_,false,false,false,true);
    kimmod->Add(*kimadd,false,1.0,1.0);
    kimmod->Complete(kim->DomainMap(),kim->RowMap());
    
    // kii: nothing to do

    // kia: nothing to do

    // n*mbaractive: do the multiplication
    RCP<LINALG::SparseMatrix> nmhata;
    if (aset) nmhata = LINALG::MLMultiply(*nmatrix_,false,*mhata,false,false,false,true);

    // nmatrix: nothing to do

    // kan: multiply with tmatrix*inv(D)
    RCP<LINALG::SparseMatrix> kanmod;
    if (aset)
    {
      kanmod = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      kanmod = LINALG::MLMultiply(*kanmod,false,*kan,false,false,false,true);
    }

    // kam: add kas*mhat and multiply with tmatrix*inv(D)
    RCP<LINALG::SparseMatrix> kammod;
    if (aset)
    {
      kammod = rcp(new LINALG::SparseMatrix(*gactivedofs_,100));
      kammod->Add(*kam,false,1.0,1.0);
      RCP<LINALG::SparseMatrix> kamadd = LINALG::MLMultiply(*kas,false,*mhatmatrix_,false,false,false,true);
      kammod->Add(*kamadd,false,1.0,1.0);
      kammod->Complete(kam->DomainMap(),kam->RowMap());
      kammod = LINALG::MLMultiply(*invda,true,*kammod,false,false,false,true);
      kammod = LINALG::MLMultiply(*tmatrix_,false,*kammod,false,false,false,true);
    }

    // kai: multiply with tmatrix*inv(D)
    RCP<LINALG::SparseMatrix> kaimod;
    if (aset && iset)
    {
      kaimod = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      kaimod = LINALG::MLMultiply(*kaimod,false,*kai,false,false,false,true);
    }

    // kaa: multiply with tmatrix*inv(D)
    RCP<LINALG::SparseMatrix> kaamod;
    if (aset)
    {
      kaamod = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      kaamod = LINALG::MLMultiply(*kaamod,false,*kaa,false,false,false,true);
    }

    // fn: nothing to do

    // fs: subtract alphaf * old contact forces (t_n)
    RCP<Epetra_Vector> fsmod = rcp(new Epetra_Vector(*gsdofrowmap_));
    fsmod->Update(1.0,*fs,0.0);
    RCP<Epetra_Vector> fsadd = rcp(new Epetra_Vector(*gsdofrowmap_));
    dold_->Multiply(true,*zold_,*fsadd);
    fsmod->Update(-alphaf_,*fsadd,1.0);
    
    // fi: subtract alphaf * old contact forces (t_n)
    if (iset)
    {
      RCP<Epetra_Vector> fiadd = rcp(new Epetra_Vector(*gidofs));
      LINALG::Export(*fsadd,*fiadd);
      fi->Update(-alphaf_,*fiadd,1.0);
    }

    // fa: subtract alphaf * old contact forces (t_n)
    if (aset)
    {
      RCP<Epetra_Vector> faadd = rcp(new Epetra_Vector(*gactivedofs_));
      LINALG::Export(*fsadd,*faadd);
      fa->Update(-alphaf_,*faadd,1.0);
    }

    // fm: add alphaf * old contact forces (t_n)

    // for self contact, slave and master sets may have changed,
    // thus we have to export the product Mold^T * zold to fit
    if (IsSelfContact())
    {
      RCP<Epetra_Vector> tempvecm = rcp(new Epetra_Vector(*gmdofrowmap_));
      RCP<Epetra_Vector> tempvecm2  = rcp(new Epetra_Vector(mold_->DomainMap()));
      RCP<Epetra_Vector> zoldexp  = rcp(new Epetra_Vector(mold_->RowMap()));
      if (mold_->RowMap().NumGlobalElements()) LINALG::Export(*zold_,*zoldexp);
      mold_->Multiply(true,*zoldexp,*tempvecm2);
      if (mset) LINALG::Export(*tempvecm2,*tempvecm);
      fm->Update(alphaf_,*tempvecm,1.0);
    }
    // if there is no self contact everything is ok
    else
    {
      RCP<Epetra_Vector> tempvecm = rcp(new Epetra_Vector(*gmdofrowmap_));
      mold_->Multiply(true,*zold_,*tempvecm);
      fm->Update(alphaf_,*tempvecm,1.0);
    }

    // fm: add T(mhat)*fsmod
    RCP<Epetra_Vector> fmmod = rcp(new Epetra_Vector(*gmdofrowmap_));
    mhatmatrix_->Multiply(true,*fsmod,*fmmod);
    fmmod->Update(1.0,*fm,1.0);

    // fa: mutliply with tmatrix*inv(D)
    // (this had to wait as we had to modify fm first)
    RCP<Epetra_Vector> famod;
    RCP<LINALG::SparseMatrix> tinvda;
    if (aset)
    {
      famod = rcp(new Epetra_Vector(*gactivet_));
      tinvda = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      tinvda->Multiply(false,*fa,*famod);
    }

    // gactive: nothing to do

    /**********************************************************************/
    /* Global setup of kteffnew, feffnew (including contact)              */
    /**********************************************************************/
    RCP<LINALG::SparseMatrix> kteffnew = rcp(new LINALG::SparseMatrix(*problemrowmap_,81,true,false,kteffmatrix->GetMatrixtype()));
    RCP<Epetra_Vector> feffnew = LINALG::CreateVector(*problemrowmap_);

    // add n submatrices to kteffnew
    kteffnew->Add(*knn,false,1.0,1.0);
    kteffnew->Add(*knmmod,false,1.0,1.0);
    if (sset) kteffnew->Add(*kns,false,1.0,1.0);

    // add m submatrices to kteffnew
    kteffnew->Add(*kmnmod,false,1.0,1.0);
    kteffnew->Add(*kmmmod,false,1.0,1.0);
    kteffnew->Add(*kmsmod,false,1.0,1.0);

    // add i submatrices to kteffnew
    if (iset) kteffnew->Add(*kin,false,1.0,1.0);
    if (iset) kteffnew->Add(*kimmod,false,1.0,1.0);
    if (iset) kteffnew->Add(*kii,false,1.0,1.0);
    if (iset) kteffnew->Add(*kia,false,1.0,1.0);

    // add matrices n and nmhata to kteffnew
    // this is only done for the "NO full linearization" case
    if (!fulllin)
    {
      if (aset) kteffnew->Add(*nmatrix_,false,1.0,1.0);
      if (aset) kteffnew->Add(*nmhata,false,-1.0,1.0);
    }

    // add full linearization terms to kteffnew
    if (fulllin && aset)
    {
      kteffnew->Add(*smatrixm,false,-1.0,1.0);
      RCP<LINALG::SparseMatrix> smatrixmadd = LINALG::MLMultiply(*smatrixs,false,*mhatmatrix_,false,false,false,true);
      kteffnew->Add(*smatrixmadd,false,-1.0,1.0);
      kteffnew->Add(*smatrixs,false,-1.0,1.0);
      
      kteffnew->Add(*pmatrixm,false,-1.0,1.0);
      RCP<LINALG::SparseMatrix> pmatrixmadd = LINALG::MLMultiply(*pmatrixs,false,*mhatmatrix_,false,false,false,true);
      kteffnew->Add(*pmatrixmadd,false,-1.0,1.0);
      kteffnew->Add(*pmatrixs,false,-1.0,1.0);
    }

    // add a submatrices to kteffnew
    if (aset) kteffnew->Add(*kanmod,false,1.0,1.0);
    if (aset) kteffnew->Add(*kammod,false,1.0,1.0);
    if (aset && iset) kteffnew->Add(*kaimod,false,1.0,1.0);
    if (aset) kteffnew->Add(*kaamod,false,1.0,1.0);

    // FillComplete kteffnew (square)
    kteffnew->Complete();

    // add n subvector to feffnew
    RCP<Epetra_Vector> fnexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fn,*fnexp);
    feffnew->Update(1.0,*fnexp,1.0);

    // add m subvector to feffnew
    RCP<Epetra_Vector> fmmodexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fmmod,*fmmodexp);
    feffnew->Update(1.0,*fmmodexp,1.0);

    // add i subvector to feffnew
    RCP<Epetra_Vector> fiexp;
    if (iset)
    {
      fiexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*fi,*fiexp);
      feffnew->Update(1.0,*fiexp,1.0);
    }

    // add weighted gap vector to feffnew, if existing
    RCP<Epetra_Vector> gexp;
    if (aset)
    {
      gexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*gact,*gexp);
      feffnew->Update(1.0,*gexp,1.0);
    }

    // add a subvector to feffnew
    RCP<Epetra_Vector> famodexp;
    if (aset)
    {
      famodexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*famod,*famodexp);
      feffnew->Update(1.0,*famodexp,1.0);
    }
    
#else   
    /**********************************************************************/
    /* Multiply Mortar matrices: m^ = inv(d) * m                          */
    /**********************************************************************/
    RCP<LINALG::SparseMatrix> invd = rcp(new LINALG::SparseMatrix(*dmatrix_));
    RCP<Epetra_Vector> diag = LINALG::CreateVector(*gsdofrowmap_,true);
    int err = 0;

    // extract diagonal of invd into diag
    invd->ExtractDiagonalCopy(*diag);

    // set zero diagonal values to dummy 1.0
    for (int i=0;i<diag->MyLength();++i)
      if ((*diag)[i]==0.0) (*diag)[i]=1.0;

    // scalar inversion of diagonal values
    err = diag->Reciprocal(*diag);
    if (err>0) dserror("ERROR: Reciprocal: Zero diagonal entry!");

    // re-insert inverted diagonal into invd
    err = invd->ReplaceDiagonalValues(*diag);
    // we cannot use this check, as we deliberately replaced zero entries
    //if (err>0) dserror("ERROR: ReplaceDiagonalValues: Missing diagonal entry!");

    // do the multiplication M^ = inv(D) * M
    mhatmatrix_ = LINALG::MLMultiply(*invd,false,*mmatrix_,false,false,false,true);

    /**********************************************************************/
    /* Add contact stiffness terms to kteff                               */
    /**********************************************************************/
    if (fulllin)
    {
      kteff->UnComplete();
      kteff->Add(*lindmatrix_,false,1.0-alphaf_,1.0);
      kteff->Add(*linmmatrix_,false,1.0-alphaf_,1.0);
      kteff->Complete();
    }
    
    /**********************************************************************/
    /* Split kteff into 3x3 block matrix                                  */
    /**********************************************************************/
    // we want to split k into 3 groups s,m,n = 9 blocks
    RCP<LINALG::SparseMatrix> kss, ksm, ksn, kms, kmm, kmn, kns, knm, knn;

    // temporarily we need the blocks ksmsm, ksmn, knsm
    // (FIXME: because a direct SplitMatrix3x3 is still missing!)
    RCP<LINALG::SparseMatrix> ksmsm, ksmn, knsm;

    // some temporary RCPs
    RCP<Epetra_Map> tempmap;
    RCP<LINALG::SparseMatrix> tempmtx1;
    RCP<LINALG::SparseMatrix> tempmtx2;
    RCP<LINALG::SparseMatrix> tempmtx3;

    // split into slave/master part + structure part
    RCP<LINALG::SparseMatrix> kteffmatrix = Teuchos::rcp_dynamic_cast<LINALG::SparseMatrix>(kteff);
    LINALG::SplitMatrix2x2(kteffmatrix,gsmdofs,gndofrowmap_,gsmdofs,gndofrowmap_,ksmsm,ksmn,knsm,knn);

    // further splits into slave part + master part
    LINALG::SplitMatrix2x2(ksmsm,gsdofrowmap_,gmdofrowmap_,gsdofrowmap_,gmdofrowmap_,kss,ksm,kms,kmm);
    LINALG::SplitMatrix2x2(ksmn,gsdofrowmap_,gmdofrowmap_,gndofrowmap_,tempmap,ksn,tempmtx1,kmn,tempmtx2);
    LINALG::SplitMatrix2x2(knsm,gndofrowmap_,tempmap,gsdofrowmap_,gmdofrowmap_,kns,knm,tempmtx1,tempmtx2);

    /**********************************************************************/
    /* Split feff into 3 subvectors                                       */
    /**********************************************************************/
    // we want to split f into 3 groups s.m,n
    RCP<Epetra_Vector> fs, fm, fn;

    // temporarily we need the group sm
    RCP<Epetra_Vector> fsm;

    // do the vector splitting smn -> sm+n
    LINALG::SplitVector(*problemrowmap_,*feff,gsmdofs,fsm,gndofrowmap_,fn);

    // abbreviations for slave  and master set
    int sset = gsdofrowmap_->NumGlobalElements();
    int mset = gmdofrowmap_->NumGlobalElements();
    
    // we want to split fsm into 2 groups s,m
    fs = rcp(new Epetra_Vector(*gsdofrowmap_));
    fm = rcp(new Epetra_Vector(*gmdofrowmap_));

    // do the vector splitting sm -> s+m
    LINALG::SplitVector(*gsmdofs,*fsm,gsdofrowmap_,fs,gmdofrowmap_,fm);

    // store some stuff for static condensation of LM
    fs_   = fs;
    invd_ = invd;
    ksn_  = ksn;
    ksm_  = ksm;
    kss_  = kss;

    //----------------------------------------------------------------------
  	// CHECK IF WE NEED TRANSFORMATION MATRICES FOR SLAVE DISPLACEMENT DOFS
  	//----------------------------------------------------------------------
  	// Concretely, we apply the following transformations:
  	// D         ---->   D * T^(-1)
  	// D^(-1)    ---->   T * D^(-1)
  	// \hat{M}   ---->   T * \hat{M}
  	//----------------------------------------------------------------------
  	if (Dualquadslave3d())
  	{
  		dserror("ERROR: Dual LM condensation not yet fully impl. for 3D quadratic contact");

  		// modify dmatrix_, invd_ and mhatmatrix_
  		RCP<LINALG::SparseMatrix> temp2 = LINALG::MLMultiply(*dmatrix_,false,*invtrafo_,false,false,false,true);
  		RCP<LINALG::SparseMatrix> temp3 = LINALG::MLMultiply(*trafo_,false,*invd_,false,false,false,true);
  		RCP<LINALG::SparseMatrix> temp4 = LINALG::MLMultiply(*trafo_,false,*mhatmatrix_,false,false,false,true);
  		dmatrix_    = temp2;
  		invd_       = temp3;
  		mhatmatrix_ = temp4;
  	}

    /**********************************************************************/
    /* Split slave quantities into active / inactive                      */
    /**********************************************************************/
    // we want to split kssmod into 2 groups a,i = 4 blocks
    RCP<LINALG::SparseMatrix> kaa, kai, kia, kii;

    // we want to split ksn / ksm / kms into 2 groups a,i = 2 blocks
    RCP<LINALG::SparseMatrix> kan, kin, kam, kim, kma, kmi;

    // we will get the i rowmap as a by-product
    RCP<Epetra_Map> gidofs;

    // do the splitting
    LINALG::SplitMatrix2x2(kss,gactivedofs_,gidofs,gactivedofs_,gidofs,kaa,kai,kia,kii);
    LINALG::SplitMatrix2x2(ksn,gactivedofs_,gidofs,gndofrowmap_,tempmap,kan,tempmtx1,kin,tempmtx2);
    LINALG::SplitMatrix2x2(ksm,gactivedofs_,gidofs,gmdofrowmap_,tempmap,kam,tempmtx1,kim,tempmtx2);
    LINALG::SplitMatrix2x2(kms,gmdofrowmap_,tempmap,gactivedofs_,gidofs,kma,kmi,tempmtx1,tempmtx2);

    // abbreviations for active and inactive set
    int aset = gactivedofs_->NumGlobalElements();
    int iset = gidofs->NumGlobalElements();
    
    // we want to split fsmod into 2 groups a,i
    RCP<Epetra_Vector> fa = rcp(new Epetra_Vector(*gactivedofs_));
    RCP<Epetra_Vector> fi = rcp(new Epetra_Vector(*gidofs));

    // do the vector splitting s -> a+i
    LINALG::SplitVector(*gsdofrowmap_,*fs,gactivedofs_,fa,gidofs,fi);

    /**********************************************************************/
    /* Isolate active part from mhat and invd                             */
    /**********************************************************************/
    RCP<LINALG::SparseMatrix> mhata;
    LINALG::SplitMatrix2x2(mhatmatrix_,gactivedofs_,gidofs,gmdofrowmap_,tempmap,mhata,tempmtx1,tempmtx2,tempmtx3);

    RCP<LINALG::SparseMatrix> invda;
    LINALG::SplitMatrix2x2(invd_,gactivedofs_,gidofs,gactivedofs_,gidofs,invda,tempmtx1,tempmtx2,tempmtx3);
    invda->Scale(1/(1-alphaf_));

    /**********************************************************************/
    /* Build the final K and f blocks                                     */
    /**********************************************************************/
    // knn: nothing to do

    // knm: nothing to do

    // kns: nothing to do

    // kmn: add T(mbaractive)*kan
    RCP<LINALG::SparseMatrix> kmnmod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
    kmnmod->Add(*kmn,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmnadd = LINALG::MLMultiply(*mhata,true,*kan,false,false,false,true);
    kmnmod->Add(*kmnadd,false,1.0,1.0);
    kmnmod->Complete(kmn->DomainMap(),kmn->RowMap());

    // kmm: add T(mbaractive)*kam
    RCP<LINALG::SparseMatrix> kmmmod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
    kmmmod->Add(*kmm,false,1.0,1.0);
    RCP<LINALG::SparseMatrix> kmmadd = LINALG::MLMultiply(*mhata,true,*kam,false,false,false,true);
    kmmmod->Add(*kmmadd,false,1.0,1.0);
    kmmmod->Complete(kmm->DomainMap(),kmm->RowMap());

    // kmi: add T(mbaractive)*kai
    RCP<LINALG::SparseMatrix> kmimod;
    if (iset)
    {
      kmimod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
      kmimod->Add(*kmi,false,1.0,1.0);
      RCP<LINALG::SparseMatrix> kmiadd = LINALG::MLMultiply(*mhata,true,*kai,false,false,false,true);
      kmimod->Add(*kmiadd,false,1.0,1.0);
      kmimod->Complete(kmi->DomainMap(),kmi->RowMap());
    }

    // kma: add T(mbaractive)*kaa
    RCP<LINALG::SparseMatrix> kmamod;
    if (aset)
    {
      kmamod = rcp(new LINALG::SparseMatrix(*gmdofrowmap_,100));
      kmamod->Add(*kma,false,1.0,1.0);
      RCP<LINALG::SparseMatrix> kmaadd = LINALG::MLMultiply(*mhata,true,*kaa,false,false,false,true);
      kmamod->Add(*kmaadd,false,1.0,1.0);
      kmamod->Complete(kma->DomainMap(),kma->RowMap());
    }

    // kin: nothing to do

    // kim: nothing to do

    // kii: nothing to do

    // kia: nothing to do

    // n*mbaractive: do the multiplication
    RCP<LINALG::SparseMatrix> nmhata;
    if (aset) nmhata = LINALG::MLMultiply(*nmatrix_,false,*mhata,false,false,false,true);

    // nmatrix: nothing to do

    // kan: multiply with tmatrix
    RCP<LINALG::SparseMatrix> kanmod;
    if (aset)
    {
      kanmod = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      kanmod = LINALG::MLMultiply(*kanmod,false,*kan,false,false,false,true);
    }

    // kam: multiply with tmatrix
    RCP<LINALG::SparseMatrix> kammod;
    if (aset)
    {
      kammod = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      kammod = LINALG::MLMultiply(*kammod,false,*kam,false,false,false,true);
    }

    // kai: multiply with tmatrix
    RCP<LINALG::SparseMatrix> kaimod;
    if (aset && iset)
    {
      kaimod = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      kaimod = LINALG::MLMultiply(*kaimod,false,*kai,false,false,false,true);
    }

    // kaa: multiply with tmatrix
    RCP<LINALG::SparseMatrix> kaamod;
    if (aset)
    {
      kaamod = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      kaamod = LINALG::MLMultiply(*kaamod,false,*kaa,false,false,false,true);
    }

    // fn: nothing to do

    // fs: prepare alphaf * old contact forces (t_n)
		RCP<Epetra_Vector> fsadd = rcp(new Epetra_Vector(*gsdofrowmap_));
		dold_->Multiply(true,*zold_,*fsadd);

		// fi: subtract alphaf * old contact forces (t_n)
		if (iset)
		{
			RCP<Epetra_Vector> fiadd = rcp(new Epetra_Vector(*gidofs));
			LINALG::Export(*fsadd,*fiadd);
			fi->Update(-alphaf_,*fiadd,1.0);
		}

		// fa: subtract alphaf * old contact forces (t_n)
		if (aset)
		{
			RCP<Epetra_Vector> faadd = rcp(new Epetra_Vector(*gactivedofs_));
			LINALG::Export(*fsadd,*faadd);
			fa->Update(-alphaf_,*faadd,1.0);
		}

    // fm: add alphaf * old contact forces (t_n)

    // for self contact, slave and master sets may have changed,
    // thus we have to export the product Mold^T * zold to fit
    if (IsSelfContact())
    {
      RCP<Epetra_Vector> tempvecm = rcp(new Epetra_Vector(*gmdofrowmap_));
      RCP<Epetra_Vector> tempvecm2  = rcp(new Epetra_Vector(mold_->DomainMap()));
      RCP<Epetra_Vector> zoldexp  = rcp(new Epetra_Vector(mold_->RowMap()));
      if (mold_->RowMap().NumGlobalElements()) LINALG::Export(*zold_,*zoldexp);
      mold_->Multiply(true,*zoldexp,*tempvecm2);
      if (mset) LINALG::Export(*tempvecm2,*tempvecm);
      fm->Update(alphaf_,*tempvecm,1.0);
    }
    // if there is no self contact everything is ok
    else
    {
      RCP<Epetra_Vector> tempvecm = rcp(new Epetra_Vector(*gmdofrowmap_));
      mold_->Multiply(true,*zold_,*tempvecm);
      fm->Update(alphaf_,*tempvecm,1.0);
    }

    // fm: add T(mbaractive)*fa
    RCP<Epetra_Vector> fmmod = rcp(new Epetra_Vector(*gmdofrowmap_));
    if (aset) mhata->Multiply(true,*fa,*fmmod);
    fmmod->Update(1.0,*fm,1.0);

    // fa: mutliply with tmatrix
    // (this had to wait as we had to modify fm first)
    RCP<Epetra_Vector> famod;
    RCP<LINALG::SparseMatrix> tinvda;
    if (aset)
    {
      famod = rcp(new Epetra_Vector(*gactivet_));
      tinvda = LINALG::MLMultiply(*tmatrix_,false,*invda,true,false,false,true);
      tinvda->Multiply(false,*fa,*famod);
    }

    // gactive: nothing to do

    /**********************************************************************/
    /* Global setup of kteffnew, feffnew (including contact)              */
    /**********************************************************************/
    RCP<LINALG::SparseMatrix> kteffnew = rcp(new LINALG::SparseMatrix(*problemrowmap_,81,true,false,kteffmatrix->GetMatrixtype()));
    RCP<Epetra_Vector> feffnew = LINALG::CreateVector(*problemrowmap_);

    // add n submatrices to kteffnew
    kteffnew->Add(*knn,false,1.0,1.0);
    kteffnew->Add(*knm,false,1.0,1.0);
    if (sset) kteffnew->Add(*kns,false,1.0,1.0);

    // add m submatrices to kteffnew
    kteffnew->Add(*kmnmod,false,1.0,1.0);
    kteffnew->Add(*kmmmod,false,1.0,1.0);
    if (iset) kteffnew->Add(*kmimod,false,1.0,1.0);
    if (aset) kteffnew->Add(*kmamod,false,1.0,1.0);

    // add i submatrices to kteffnew
    if (iset) kteffnew->Add(*kin,false,1.0,1.0);
    if (iset) kteffnew->Add(*kim,false,1.0,1.0);
    if (iset) kteffnew->Add(*kii,false,1.0,1.0);
    if (iset) kteffnew->Add(*kia,false,1.0,1.0);

    // add matrices n and nmhata to kteffnew
    // this is only done for the "NO full linearization" case
    if (!fulllin)
    {
      if (aset) kteffnew->Add(*nmatrix_,false,1.0,1.0);
      if (aset) kteffnew->Add(*nmhata,false,-1.0,1.0);
    }

    // add full linearization terms to kteffnew
    if (fulllin)
    {
     if (aset) kteffnew->Add(*smatrix_,false,-1.0,1.0);
     if (aset) kteffnew->Add(*pmatrix_,false,-1.0,1.0);
    }

    // add a submatrices to kteffnew
    if (aset) kteffnew->Add(*kanmod,false,1.0,1.0);
    if (aset) kteffnew->Add(*kammod,false,1.0,1.0);
    if (aset && iset) kteffnew->Add(*kaimod,false,1.0,1.0);
    if (aset) kteffnew->Add(*kaamod,false,1.0,1.0);

    // FillComplete kteffnew (square)
    kteffnew->Complete();

    // add n subvector to feffnew
    RCP<Epetra_Vector> fnexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fn,*fnexp);
    feffnew->Update(1.0,*fnexp,1.0);

    // add m subvector to feffnew
    RCP<Epetra_Vector> fmmodexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fmmod,*fmmodexp);
    feffnew->Update(1.0,*fmmodexp,1.0);

    // add i subvector to feffnew
    RCP<Epetra_Vector> fiexp;
    if (iset)
    {
      fiexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*fi,*fiexp);
      feffnew->Update(1.0,*fiexp,1.0);
    }

    // add weighted gap vector to feffnew, if existing
    RCP<Epetra_Vector> gexp;
    if (aset)
    {
      gexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*gact,*gexp);
      feffnew->Update(1.0,*gexp,1.0);
    }

    // add a subvector to feffnew
    RCP<Epetra_Vector> famodexp;
    if (aset)
    {
      famodexp = rcp(new Epetra_Vector(*problemrowmap_));
      LINALG::Export(*famod,*famodexp);
      feffnew->Update(1.0,*famodexp,1.0);
    }
#endif // #ifdef CONTACTBASISTRAFO

    /**********************************************************************/
    /* Replace kteff and feff by kteffnew and feffnew                     */
    /**********************************************************************/
    kteff = kteffnew;
    feff = feffnew;
  }
  
  //**********************************************************************
  //**********************************************************************
  // CASE B: SADDLE POINT SYSTEM
  //**********************************************************************
  //**********************************************************************
  else
  {
    //----------------------------------------------------------------------
  	// CHECK IF WE NEED TRANSFORMATION MATRICES FOR SLAVE DISPLACEMENT DOFS
  	//----------------------------------------------------------------------
  	// Concretely, we apply the following transformations:
  	// D         ---->   D * T^(-1)
  	//----------------------------------------------------------------------
  	if (Dualquadslave3d())
  	{
  		// modify dmatrix_
  		RCP<LINALG::SparseMatrix> temp2 = LINALG::MLMultiply(*dmatrix_,false,*invtrafo_,false,false,false,true);
  		dmatrix_    = temp2;
  	}

    // add contact stiffness
    kteff->UnComplete();
    kteff->Add(*lindmatrix_,false,1.0-alphaf_,1.0);
    kteff->Add(*linmmatrix_,false,1.0-alphaf_,1.0);
    kteff->Complete();
    
    // add contact force terms
    RCP<Epetra_Vector> fs = rcp(new Epetra_Vector(*gsdofrowmap_));
    dmatrix_->Multiply(true,*z_,*fs);
    RCP<Epetra_Vector> fsexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fs,*fsexp);
    feff->Update(-(1.0-alphaf_),*fsexp,1.0);
    
    RCP<Epetra_Vector> fm = rcp(new Epetra_Vector(*gmdofrowmap_));
    mmatrix_->Multiply(true,*z_,*fm);
    RCP<Epetra_Vector> fmexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fm,*fmexp);
    feff->Update(1.0-alphaf_,*fmexp,1.0);

    // add old contact forces (t_n)
    RCP<Epetra_Vector> fsold = rcp(new Epetra_Vector(*gsdofrowmap_));
    dold_->Multiply(true,*zold_,*fsold);
    RCP<Epetra_Vector> fsoldexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fsold,*fsoldexp);
    feff->Update(-alphaf_,*fsoldexp,1.0);

    RCP<Epetra_Vector> fmold = rcp(new Epetra_Vector(*gmdofrowmap_));
    mold_->Multiply(true,*zold_,*fmold);
    RCP<Epetra_Vector> fmoldexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fmold,*fmoldexp);
    feff->Update(alphaf_,*fmoldexp,1.0);
  }
  
#ifdef CONTACTFDGAP
  // FD check of weighted gap g derivatives (non-penetr. condition)
  for (int i=0; i<(int)interface_.size(); ++i)
  {
    cout << *smatrix_ << endl;
    interface_[i]->FDCheckGapDeriv();
  }
#endif // #ifdef CONTACTFDGAP

#ifdef CONTACTFDTANGLM
  // FD check of tangential LM derivatives (frictionless condition)
  for (int i=0; i<(int)interface_.size();++i)
  {
    cout << *pmatrix_ << endl;
    interface_[i]->FDCheckTangLMDeriv();
  }
#endif // #ifdef CONTACTFDTANGLM

  return;
}

/*----------------------------------------------------------------------*
 | Solve linear system of saddle point type                   popp 03/10|
 *----------------------------------------------------------------------*/
void CONTACT::CoLagrangeStrategy::SaddlePointSolve(LINALG::Solver& solver,
                  RCP<LINALG::SparseOperator> kdd,  RCP<Epetra_Vector> fd,
                  RCP<Epetra_Vector>  sold, RCP<Epetra_Vector> dirichtoggle,
                  int numiter)
{
  //**********************************************************************
  // prepare saddle point system
  //**********************************************************************
  // get system type
  INPAR::CONTACT::SystemType systype = Teuchos::getIntegralValue<INPAR::CONTACT::SystemType>(Params(),"SYSTEM");
    
  // some pointers and variables
  RCP<LINALG::SparseMatrix> stiffmt   = Teuchos::rcp_dynamic_cast<LINALG::SparseMatrix>(kdd);
  RCP<Epetra_Map>           dispmap   = problemrowmap_;
  RCP<Epetra_Map>           slavemap  = gsdofrowmap_;
  RCP<Epetra_Map>           mastermap = gmdofrowmap_;
  RCP<Epetra_Map>           lmmap     = glmdofrowmap_;
      
  // initialize merged system (matrix, rhs, sol)
  RCP<Epetra_Map>           mergedmap   = LINALG::MergeMap(dispmap,lmmap,false); 
  RCP<LINALG::SparseMatrix> mergedmt    = rcp(new LINALG::SparseMatrix(*mergedmap,100,false,true));
  RCP<Epetra_Vector>        mergedrhs   = LINALG::CreateVector(*mergedmap);
  RCP<Epetra_Vector>        mergedsol   = LINALG::CreateVector(*mergedmap);
  RCP<Epetra_Vector>        mergedzeros = LINALG::CreateVector(*mergedmap);
  
  // initialite constraint r.h.s. (stll with wrong map)
  RCP<Epetra_Vector> constrrhs = rcp(new Epetra_Vector(*slavemap));
  
  // initialize transformed constraint matrices
  RCP<LINALG::SparseMatrix> trkdz = rcp(new LINALG::SparseMatrix(*dispmap,100,false,true));
  RCP<LINALG::SparseMatrix> trkzd = rcp(new LINALG::SparseMatrix(*lmmap,100,false,true));
  RCP<LINALG::SparseMatrix> trkzz = rcp(new LINALG::SparseMatrix(*lmmap,100,false,true));
  
  //**********************************************************************
  // build matrix and vector blocks
  //**********************************************************************
  // *** CASE 1: FRICTIONLESS CONTACT ************************************
  if (!friction_)
  {
    // build constraint matrix kdz
    RCP<LINALG::SparseMatrix> kdz = rcp(new LINALG::SparseMatrix(*dispmap,100,false,true));
    kdz->Add(*dmatrix_,true,1.0-alphaf_,1.0);
    kdz->Add(*mmatrix_,true,-(1.0-alphaf_),1.0);
    kdz->Complete(*slavemap,*dispmap);

    // mapping of gids
    map<int,int> gidmap;
    DRT::Exporter ex(kdz->RowMap(),kdz->ColMap(),kdz->Comm());
    for (int i=0; i<slavemap->NumMyElements(); ++i) gidmap[slavemap->GID(i)] = lmmap->GID(i);
    ex.Export(gidmap);
      
    // transform constraint matrix kdz to lmdofmap 
    for (int i=0;i<(kdz->EpetraMatrix())->NumMyRows();++i)
    {
      int NumEntries = 0;
      double *Values;
      int *Indices;
      int err = (kdz->EpetraMatrix())->ExtractMyRowView(i, NumEntries, Values, Indices);
      if (err!=0) dserror("ExtractMyRowView error: %d", err);
      std::vector<int> idx;
      std::vector<double> vals;
      idx.reserve(NumEntries);
      vals.reserve(NumEntries);

      for (int j=0;j<NumEntries;++j)
      {
        int gid = (kdz->ColMap()).GID(Indices[j]);
        std::map<int,int>::const_iterator iter = gidmap.find(gid);
        if (iter!=gidmap.end())
        {
          idx.push_back(iter->second);
          vals.push_back(Values[j]);
        }
        else
          dserror("gid %d not found in map for lid %d at %d", gid, Indices[j], j);
      }

      Values = &vals[0];
      NumEntries = vals.size();
      err = (trkdz->EpetraMatrix())->InsertGlobalValues(kdz->RowMap().GID(i), NumEntries, const_cast<double*>(Values),&idx[0]);
      if (err<0) dserror("InsertGlobalValues error: %d", err);
    }
    
    // complete transformed constraint matrix kdz
    trkdz->Complete(*lmmap,*dispmap);
   
    // build constraint matrix kzd
    RCP<LINALG::SparseMatrix> kzd = rcp(new LINALG::SparseMatrix(*slavemap,100,false,true));
    if (gactiven_->NumGlobalElements()) kzd->Add(*smatrix_,false,1.0,1.0);
    if (gactivet_->NumGlobalElements()) kzd->Add(*pmatrix_,false,1.0,1.0);
    kzd->Complete(*dispmap,*slavemap);
    
    // transform constraint matrix kzd to lmdofmap
    for (int i=0; i<(kzd->EpetraMatrix())->NumMyRows(); ++i)
    {
      int NumEntries = 0;
      double *Values;
      int *Indices;
      int err = (kzd->EpetraMatrix())->ExtractMyRowView(i, NumEntries, Values, Indices);
      if (err!=0) dserror("ExtractMyRowView error: %d", err);

      // pull indices back to global
      std::vector<int> idx(NumEntries);
      for (int j=0; j<NumEntries; ++j)
      {
        idx[j] = (kzd->ColMap()).GID(Indices[j]);
      }

      err = (trkzd->EpetraMatrix())->InsertGlobalValues(lmmap->GID(i), NumEntries, const_cast<double*>(Values),&idx[0]);
      if (err<0) dserror("InsertGlobalValues error: %d", err);
    }
     
    // complete transformed constraint matrix kzd
    trkzd->Complete(*dispmap,*lmmap);

    // build unity matrix for inactive dofs
    RCP<Epetra_Map> gidofs = LINALG::SplitMap(*slavemap,*gactivedofs_);
    RCP<Epetra_Vector> ones = rcp (new Epetra_Vector(*gidofs));
    ones->PutScalar(1.0);
    RCP<LINALG::SparseMatrix> onesdiag = rcp(new LINALG::SparseMatrix(*ones));
    onesdiag->Complete();
    
    // build constraint matrix kzz
    RCP<LINALG::SparseMatrix> kzz = rcp(new LINALG::SparseMatrix(*slavemap,100,false,true));
    if (gidofs->NumGlobalElements())    kzz->Add(*onesdiag,false,1.0,1.0);
    if (gactivet_->NumGlobalElements()) kzz->Add(*tmatrix_,false,1.0,1.0);
    kzz->Complete(*slavemap,*slavemap);
    
    // mapping of gids
    map<int,int> gidmapzz;
    DRT::Exporter exzz(kzz->RowMap(),kzz->ColMap(),kzz->Comm());
    for (int i=0; i<slavemap->NumMyElements(); ++i) gidmapzz[slavemap->GID(i)] = lmmap->GID(i);
    exzz.Export(gidmapzz);
      
    // transform constraint matrix kzz to lmdofmap
    for (int i=0;i<(kzz->EpetraMatrix())->NumMyRows();++i)
    {
      int NumEntries = 0;
      double *Values;
      int *Indices;
      int err = (kzz->EpetraMatrix())->ExtractMyRowView(i, NumEntries, Values, Indices);
      if (err!=0) dserror("ExtractMyRowView error: %d", err);
      std::vector<int> idx;
      std::vector<double> vals;
      idx.reserve(NumEntries);
      vals.reserve(NumEntries);

      for (int j=0;j<NumEntries;++j)
      {
        int gid = (kzz->ColMap()).GID(Indices[j]);
        std::map<int,int>::const_iterator iter = gidmapzz.find(gid);
        if (iter!=gidmapzz.end())
        {
          idx.push_back(iter->second);
          vals.push_back(Values[j]);
        }
        else
          dserror("gid %d not found in map for lid %d at %d", gid, Indices[j], j);
      }

      Values = &vals[0];
      NumEntries = vals.size();
      err = (trkzz->EpetraMatrix())->InsertGlobalValues(lmmap->GID(i), NumEntries, const_cast<double*>(Values),&idx[0]);
      if (err<0) dserror("InsertGlobalValues error: %d", err);
    }
    
    // complete transformed constraint matrix kzz
    trkzz->Complete(*lmmap,*lmmap);

    // remove contact force terms again
    // (solve directly for z_ and not for increment of z_)
    RCP<Epetra_Vector> fs = rcp(new Epetra_Vector(*gsdofrowmap_));
    dmatrix_->Multiply(true,*z_,*fs);
    RCP<Epetra_Vector> fsexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fs,*fsexp);
    fd->Update((1.0-alphaf_),*fsexp,1.0);
    
    RCP<Epetra_Vector> fm = rcp(new Epetra_Vector(*gmdofrowmap_));
    mmatrix_->Multiply(true,*z_,*fm);
    RCP<Epetra_Vector> fmexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fm,*fmexp);
    fd->Update(-(1.0-alphaf_),*fmexp,1.0);
    
    // export weighted gap vector
    RCP<Epetra_Vector> gact = LINALG::CreateVector(*gactivenodes_,true);
    if (gactiven_->NumGlobalElements())
    {
      LINALG::Export(*g_,*gact);
      gact->ReplaceMap(*gactiven_);
    }
    RCP<Epetra_Vector> gactexp = rcp(new Epetra_Vector(*slavemap));
    LINALG::Export(*gact,*gactexp);
    
    // build constraint rhs
    constrrhs->Update(-1.0,*gactexp,1.0);
    constrrhs->ReplaceMap(*lmmap);
  }
  
  //**********************************************************************
  // build matrix and vector blocks
  //**********************************************************************
  // *** CASE 2: FRICTIONAL CONTACT **************************************
  else
  {
    // global stick dof map
    RCP<Epetra_Map> gstickt = LINALG::SplitMap(*gactivet_,*gslipt_);
    
    // build constraint matrix kdz
    RCP<LINALG::SparseMatrix> kdz = rcp(new LINALG::SparseMatrix(*dispmap,100,false,true));
    kdz->Add(*dmatrix_,true,1.0-alphaf_,1.0);
    kdz->Add(*mmatrix_,true,-(1.0-alphaf_),1.0);
    kdz->Complete(*slavemap,*dispmap);

    // mapping of gids
    map<int,int> gidmap;
    DRT::Exporter ex(kdz->RowMap(),kdz->ColMap(),kdz->Comm());
    for (int i=0; i<slavemap->NumMyElements(); ++i) gidmap[slavemap->GID(i)] = lmmap->GID(i);
    ex.Export(gidmap);
      
    // transform constraint matrix kdz to lmdofmap 
    for (int i=0;i<(kdz->EpetraMatrix())->NumMyRows();++i)
    {
      int NumEntries = 0;
      double *Values;
      int *Indices;
      int err = (kdz->EpetraMatrix())->ExtractMyRowView(i, NumEntries, Values, Indices);
      if (err!=0) dserror("ExtractMyRowView error: %d", err);
      std::vector<int> idx;
      std::vector<double> vals;
      idx.reserve(NumEntries);
      vals.reserve(NumEntries);

      for (int j=0;j<NumEntries;++j)
      {
        int gid = (kdz->ColMap()).GID(Indices[j]);
        std::map<int,int>::const_iterator iter = gidmap.find(gid);
        if (iter!=gidmap.end())
        {
          idx.push_back(iter->second);
          vals.push_back(Values[j]);
        }
        else
          dserror("gid %d not found in map for lid %d at %d", gid, Indices[j], j);
      }

      Values = &vals[0];
      NumEntries = vals.size();
      err = (trkdz->EpetraMatrix())->InsertGlobalValues(kdz->RowMap().GID(i), NumEntries, const_cast<double*>(Values),&idx[0]);
      if (err<0) dserror("InsertGlobalValues error: %d", err);
    }
    
    // complete transformed constraint matrix kdz
    trkdz->Complete(*lmmap,*dispmap);
   
    // build constraint matrix kzd
    RCP<LINALG::SparseMatrix> kzd = rcp(new LINALG::SparseMatrix(*slavemap,100,false,true));
    if (gactiven_->NumGlobalElements()) kzd->Add(*smatrix_,false,1.0,1.0);
    if (gstickt->NumGlobalElements()) kzd->Add(*linstickDIS_,false,1.0,1.0);
    if (gslipt_->NumGlobalElements()) kzd->Add(*linslipDIS_,false,1.0,1.0);
    kzd->Complete(*dispmap,*slavemap);
    
    // transform constraint matrix kzd to lmdofmap
    for (int i=0; i<(kzd->EpetraMatrix())->NumMyRows(); ++i)
    {
      int NumEntries = 0;
      double *Values;
      int *Indices;
      int err = (kzd->EpetraMatrix())->ExtractMyRowView(i, NumEntries, Values, Indices);
      if (err!=0) dserror("ExtractMyRowView error: %d", err);

      // pull indices back to global
      std::vector<int> idx(NumEntries);
      for (int j=0; j<NumEntries; ++j)
      {
        idx[j] = (kzd->ColMap()).GID(Indices[j]);
      }

      err = (trkzd->EpetraMatrix())->InsertGlobalValues(lmmap->GID(i), NumEntries, const_cast<double*>(Values),&idx[0]);
      if (err<0) dserror("InsertGlobalValues error: %d", err);
    }
     
    // complete transformed constraint matrix kzd
    trkzd->Complete(*dispmap,*lmmap);

    // build unity matrix for inactive dofs
    RCP<Epetra_Map> gidofs = LINALG::SplitMap(*slavemap,*gactivedofs_);
    RCP<Epetra_Vector> ones = rcp (new Epetra_Vector(*gidofs));
    ones->PutScalar(1.0);
    RCP<LINALG::SparseMatrix> onesdiag = rcp(new LINALG::SparseMatrix(*ones));
    onesdiag->Complete();
    
    // build constraint matrix kzz
    RCP<LINALG::SparseMatrix> kzz = rcp(new LINALG::SparseMatrix(*slavemap,100,false,true));
    if (gidofs->NumGlobalElements())    kzz->Add(*onesdiag,false,1.0,1.0);
    if (gstickt->NumGlobalElements()) kzz->Add(*linstickLM_,false,1.0,1.0);
    if (gslipt_->NumGlobalElements()) kzz->Add(*linslipLM_,false,1.0,1.0);
    kzz->Complete(*slavemap,*slavemap);
    
    // mapping of gids
    map<int,int> gidmapzz;
    DRT::Exporter exzz(kzz->RowMap(),kzz->ColMap(),kzz->Comm());
    for (int i=0; i<slavemap->NumMyElements(); ++i) gidmapzz[slavemap->GID(i)] = lmmap->GID(i);
    exzz.Export(gidmapzz);
      
    // transform constraint matrix kzz to lmdofmap
    for (int i=0;i<(kzz->EpetraMatrix())->NumMyRows();++i)
    {
      int NumEntries = 0;
      double *Values;
      int *Indices;
      int err = (kzz->EpetraMatrix())->ExtractMyRowView(i, NumEntries, Values, Indices);
      if (err!=0) dserror("ExtractMyRowView error: %d", err);
      std::vector<int> idx;
      std::vector<double> vals;
      idx.reserve(NumEntries);
      vals.reserve(NumEntries);

      for (int j=0;j<NumEntries;++j)
      {
        int gid = (kzz->ColMap()).GID(Indices[j]);
        std::map<int,int>::const_iterator iter = gidmapzz.find(gid);
        if (iter!=gidmapzz.end())
        {
          idx.push_back(iter->second);
          vals.push_back(Values[j]);
        }
        else
          dserror("gid %d not found in map for lid %d at %d", gid, Indices[j], j);
      }

      Values = &vals[0];
      NumEntries = vals.size();
      err = (trkzz->EpetraMatrix())->InsertGlobalValues(lmmap->GID(i), NumEntries, const_cast<double*>(Values),&idx[0]);
      if (err<0) dserror("InsertGlobalValues error: %d", err);
    }
    
    // complete transformed constraint matrix kzz
    trkzz->Complete(*lmmap,*lmmap);

    // remove contact force terms again
    // (solve directly for z_ and not for increment of z_)
    RCP<Epetra_Vector> fs = rcp(new Epetra_Vector(*gsdofrowmap_));
    dmatrix_->Multiply(true,*z_,*fs);
    RCP<Epetra_Vector> fsexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fs,*fsexp);
    fd->Update((1.0-alphaf_),*fsexp,1.0);
    
    RCP<Epetra_Vector> fm = rcp(new Epetra_Vector(*gmdofrowmap_));
    mmatrix_->Multiply(true,*z_,*fm);
    RCP<Epetra_Vector> fmexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*fm,*fmexp);
    fd->Update(-(1.0-alphaf_),*fmexp,1.0);
    
    // export weighted gap vector
    RCP<Epetra_Vector> gact = LINALG::CreateVector(*gactivenodes_,true);
    if (gactiven_->NumGlobalElements())
    {
      LINALG::Export(*g_,*gact);
      gact->ReplaceMap(*gactiven_);
    }
    RCP<Epetra_Vector> gactexp = rcp(new Epetra_Vector(*slavemap));
    LINALG::Export(*gact,*gactexp);
    
    // export stick and slip r.h.s.
    RCP<Epetra_Vector> stickexp = rcp(new Epetra_Vector(*slavemap));
    LINALG::Export(*linstickRHS_,*stickexp);
    RCP<Epetra_Vector> slipexp = rcp(new Epetra_Vector(*slavemap));
    LINALG::Export(*linslipRHS_,*slipexp);
    
    // build constraint rhs
    constrrhs->Update(-1.0,*gactexp,1.0);
    constrrhs->Update(1.0,*stickexp,1.0);
    constrrhs->Update(1.0,*slipexp,1.0);
    constrrhs->ReplaceMap(*lmmap);
  }

  //**********************************************************************
  // Build and solve saddle point system
  // (A) Standard coupled version
  //**********************************************************************
  if (systype==INPAR::CONTACT::system_spcoupled)
  {
    // build merged matrix    
    mergedmt->Add(*stiffmt,false,1.0,1.0);
    mergedmt->Add(*trkdz,false,1.0,1.0);
    mergedmt->Add(*trkzd,false,1.0,1.0);
    mergedmt->Add(*trkzz,false,1.0,1.0);
    mergedmt->Complete();    
       
    // build merged rhs
    RCP<Epetra_Vector> fresmexp = rcp(new Epetra_Vector(*mergedmap));
    LINALG::Export(*fd,*fresmexp);
    mergedrhs->Update(1.0,*fresmexp,1.0);
    RCP<Epetra_Vector> constrexp = rcp(new Epetra_Vector(*mergedmap));
    LINALG::Export(*constrrhs,*constrexp);
    mergedrhs->Update(1.0,*constrexp,1.0);
    
    // adapt dirichtoggle vector and apply DBC
    RCP<Epetra_Vector> dirichtoggleexp = rcp(new Epetra_Vector(*mergedmap));
    LINALG::Export(*dirichtoggle,*dirichtoggleexp);
    LINALG::ApplyDirichlettoSystem(mergedmt,mergedsol,mergedrhs,mergedzeros,dirichtoggleexp);
    
    // standard solver call
    solver.Solve(mergedmt->EpetraMatrix(),mergedsol,mergedrhs,true,numiter==0);
  }
  
  //**********************************************************************
  // Build and solve saddle point system
  // (B) SIMPLER preconditioner version
  //**********************************************************************
  else if (systype==INPAR::CONTACT::system_spsimpler)
  {
    // apply Dirichlet conditions to (0,0) and (0,1) blocks
    RCP<Epetra_Vector> zeros   = rcp(new Epetra_Vector(*dispmap,true));
    RCP<Epetra_Vector> rhscopy = rcp(new Epetra_Vector(*fd));
    LINALG::ApplyDirichlettoSystem(stiffmt,sold,rhscopy,zeros,dirichtoggle);
    trkdz->ApplyDirichlet(dirichtoggle,false);
    
    // row map (equals domain map) extractor
    LINALG::MapExtractor rowmapext(*mergedmap,lmmap,dispmap);
    LINALG::MapExtractor dommapext(*mergedmap,lmmap,dispmap);

    // make solver SIMPLER-ready
    solver.PutSolverParamsToSubParams("SIMPLER", DRT::Problem::Instance()->FluidPressureSolverParams());
    
    // build block matrix for SIMPLER
    Teuchos::RCP<LINALG::BlockSparseMatrix<LINALG::DefaultBlockMatrixStrategy> > mat =
      rcp(new LINALG::BlockSparseMatrix<LINALG::DefaultBlockMatrixStrategy>(dommapext,rowmapext,81,false,false));
    mat->Assign(0,0,View,*stiffmt);
    mat->Assign(0,1,View,*trkdz);
    mat->Assign(1,0,View,*trkzd);
    mat->Assign(1,1,View,*trkzz);
    mat->Complete();
    
    // we also need merged rhs here
    RCP<Epetra_Vector> fresmexp = rcp(new Epetra_Vector(*mergedmap));
    LINALG::Export(*fd,*fresmexp);
    mergedrhs->Update(1.0,*fresmexp,1.0);
    RCP<Epetra_Vector> constrexp = rcp(new Epetra_Vector(*mergedmap));
    LINALG::Export(*constrrhs,*constrexp);
    mergedrhs->Update(1.0,*constrexp,1.0);
    
    // we need a dummy merged matrix here in order to be able
    // to apply Dirichlet B.C. to mergedrhs and mergedsol
    mergedmt->Complete();
    
    // adapt dirichtoggle vector and apply DBC
    RCP<Epetra_Vector> dirichtoggleexp = rcp(new Epetra_Vector(*mergedmap));
    LINALG::Export(*dirichtoggle,*dirichtoggleexp);
    LINALG::ApplyDirichlettoSystem(mergedmt,mergedsol,mergedrhs,mergedzeros,dirichtoggleexp);
    
    // SIMPLER preconditioning solver call
    solver.Solve(mat->EpetraOperator(),mergedsol,mergedrhs,true,numiter==0);
  }
  
  //**********************************************************************
  // invalid system types
  //**********************************************************************
  else dserror("ERROR: Invalid system type in SaddlePontSolve");
  
  //**********************************************************************
  // extract results for displacement and LM increments
  //**********************************************************************
  RCP<Epetra_Vector> sollm = rcp(new Epetra_Vector(*lmmap));
  LINALG::MapExtractor mapext(*mergedmap,dispmap,lmmap);
  mapext.ExtractCondVector(mergedsol,sold);
  mapext.ExtractOtherVector(mergedsol,sollm);
  sollm->ReplaceMap(*slavemap);
  z_->Update(1.0,*sollm,0.0);
  
  return;
}

/*----------------------------------------------------------------------*
 | Recovery method                                            popp 04/08|
 *----------------------------------------------------------------------*/
void CONTACT::CoLagrangeStrategy::Recover(RCP<Epetra_Vector> disi)
{
  // shape function and system types
  INPAR::MORTAR::ShapeFcn shapefcn = Teuchos::getIntegralValue<INPAR::MORTAR::ShapeFcn>(Params(),"SHAPEFCN");
  INPAR::CONTACT::SystemType systype = Teuchos::getIntegralValue<INPAR::CONTACT::SystemType>(Params(),"SYSTEM");
 
  //**********************************************************************
  //**********************************************************************
  // CASE A: CONDENSED SYSTEM (DUAL)
  //**********************************************************************
  //**********************************************************************
  if (systype == INPAR::CONTACT::system_condensed)
  {
    // double-check if this is a dual LM system
    if (shapefcn!=INPAR::MORTAR::shape_dual) dserror("Condensation only for dual LM");
        
    // extract slave displacements from disi
    RCP<Epetra_Vector> disis = rcp(new Epetra_Vector(*gsdofrowmap_));
    if (gsdofrowmap_->NumGlobalElements()) LINALG::Export(*disi, *disis);
  
    // extract master displacements from disi
    RCP<Epetra_Vector> disim = rcp(new Epetra_Vector(*gmdofrowmap_));
    if (gmdofrowmap_->NumGlobalElements()) LINALG::Export(*disi, *disim);
  
    // extract other displacements from disi
    RCP<Epetra_Vector> disin = rcp(new Epetra_Vector(*gndofrowmap_));
    if (gndofrowmap_->NumGlobalElements()) LINALG::Export(*disi,*disin);
  
#ifdef CONTACTBASISTRAFO
    /**********************************************************************/
    /* Update slave displacments from jump                                */
    /**********************************************************************/
    RCP<Epetra_Vector> adddisis = rcp(new Epetra_Vector(*gsdofrowmap_));
    mhatmatrix_->Multiply(false,*disim,*adddisis);
    disis->Update(1.0,*adddisis,1.0);
    RCP<Epetra_Vector> adddisisexp = rcp(new Epetra_Vector(*problemrowmap_));
    LINALG::Export(*adddisis,*adddisisexp);
    disi->Update(1.0,*adddisisexp,1.0);
#endif // #ifdef CONTACTBASISTRAFO
    
    /**********************************************************************/
    /* Update Lagrange multipliers z_n+1                                  */
    /**********************************************************************/
  
    // for self contact, slave and master sets may have changed,
    // thus we have to export the products Dold * zold and Mold^T * zold to fit
    if (IsSelfContact())
    {
      // approximate update
      //z_ = rcp(new Epetra_Vector(*gsdofrowmap_));
      //invd_->Multiply(false,*fs_,*z_);
  
      // full update
      z_ = rcp(new Epetra_Vector(*gsdofrowmap_));
      z_->Update(1.0,*fs_,0.0);
      RCP<Epetra_Vector> mod = rcp(new Epetra_Vector(*gsdofrowmap_));
      kss_->Multiply(false,*disis,*mod);
      z_->Update(-1.0,*mod,1.0);
      ksm_->Multiply(false,*disim,*mod);
      z_->Update(-1.0,*mod,1.0);
      ksn_->Multiply(false,*disin,*mod);
      z_->Update(-1.0,*mod,1.0);
      RCP<Epetra_Vector> mod2 = rcp(new Epetra_Vector((dold_->RowMap())));
      if (dold_->RowMap().NumGlobalElements()) LINALG::Export(*zold_,*mod2);
      RCP<Epetra_Vector> mod3 = rcp(new Epetra_Vector((dold_->RowMap())));
      dold_->Multiply(true,*mod2,*mod3);
      RCP<Epetra_Vector> mod4 = rcp(new Epetra_Vector(*gsdofrowmap_));
      if (gsdofrowmap_->NumGlobalElements()) LINALG::Export(*mod3,*mod4);
      z_->Update(-alphaf_,*mod4,1.0);
      RCP<Epetra_Vector> zcopy = rcp(new Epetra_Vector(*z_));
      invd_->Multiply(true,*zcopy,*z_);
      z_->Scale(1/(1-alphaf_));
    }
    else
    {
      // approximate update
      //invd_->Multiply(false,*fs_,*z_);
  
      // full update
      z_->Update(1.0,*fs_,0.0);
      RCP<Epetra_Vector> mod = rcp(new Epetra_Vector(*gsdofrowmap_));
      kss_->Multiply(false,*disis,*mod);
      z_->Update(-1.0,*mod,1.0);
      ksm_->Multiply(false,*disim,*mod);
      z_->Update(-1.0,*mod,1.0);
      ksn_->Multiply(false,*disin,*mod);
      z_->Update(-1.0,*mod,1.0);
      dold_->Multiply(true,*zold_,*mod);
      z_->Update(-alphaf_,*mod,1.0);
      RCP<Epetra_Vector> zcopy = rcp(new Epetra_Vector(*z_));
      invd_->Multiply(true,*zcopy,*z_);
      z_->Scale(1/(1-alphaf_));
    }
  }
   
  //**********************************************************************
  //**********************************************************************
  // CASE B: SADDLE POINT SYSTEM
  //**********************************************************************
  //**********************************************************************
  else
  {
    // do nothing (z_ was part of soultion already)
  }

  // store updated LM into nodes
  StoreNodalQuantities(MORTAR::StrategyBase::lmupdate);
  
  return;
}

/*----------------------------------------------------------------------*
 |  Update active set and check for convergence               popp 02/08|
 *----------------------------------------------------------------------*/
void CONTACT::CoLagrangeStrategy::UpdateActiveSet()
{
  // get input parameter ftype
  INPAR::CONTACT::FrictionType ftype =
    Teuchos::getIntegralValue<INPAR::CONTACT::FrictionType>(Params(),"FRICTION");

  // assume that active set has converged and check for opposite
  activesetconv_=true;

  // loop over all interfaces
  for (int i=0; i<(int)interface_.size(); ++i)
  {
    //if (i>0) dserror("ERROR: UpdateActiveSet: Double active node check needed for n interfaces!");

    // loop over all slave nodes on the current interface
    for (int j=0;j<interface_[i]->SlaveRowNodes()->NumMyElements();++j)
    {
      int gid = interface_[i]->SlaveRowNodes()->GID(j);
      DRT::Node* node = interface_[i]->Discret().gNode(gid);
      if (!node) dserror("ERROR: Cannot find node with gid %",gid);
      CoNode* cnode = static_cast<CoNode*>(node);

      // compute weighted gap
      double wgap = (*g_)[g_->Map().LID(gid)];

      // compute normal part of Lagrange multiplier
      double nz = 0.0;
      double nzold = 0.0;
      for (int k=0;k<3;++k)
      {
        nz += cnode->MoData().n()[k] * cnode->MoData().lm()[k];
        nzold += cnode->MoData().n()[k] * cnode->MoData().lmold()[k];
      }
      
      // friction
      double tz = 0.0;
      double tjump = 0.0;

      if (friction_)
      {
        FriNode* frinode = static_cast<FriNode*>(cnode);
        
        // compute tangential part of Lagrange multiplier
        tz = frinode->CoData().txi()[0]*frinode->MoData().lm()[0] + frinode->CoData().txi()[1]*frinode->MoData().lm()[1];

        // compute tangential part of jump
        tjump = frinode->CoData().txi()[0]*frinode->Data().jump()[0] + frinode->CoData().txi()[1]*frinode->Data().jump()[1];
      }

      // check nodes of inactive set *************************************
      // (by definition they fulfill the condition z_j = 0)
      // (thus we only have to check ncr.disp. jump and weighted gap)
      if (cnode->Active()==false)
      {
        // check for fulfilment of contact condition
        //if (abs(nz) > 1e-8)
        //  cout << "ERROR: UpdateActiveSet: Exact inactive node condition violated "
        //       <<  "for node ID: " << cnode->Id() << endl;

        // check for penetration
        if (wgap < 0)
        {
          cnode->Active() = true;
          activesetconv_ = false;
#ifdef CONTACTFRICTIONLESSFIRST
       if (static_cast<FriNode*>(cnode)->Data().ActiveOld()==false)
         static_cast<FriNode*>(cnode)->Data().Slip() = true;
#endif
        }
      }

      // check nodes of active set ***************************************
      // (by definition they fulfill the non-penetration condition)
      // (thus we only have to check for positive Lagrange multipliers)
      else
      {
        // check for fulfilment of contact condition
        //if (abs(wgap) > 1e-8)
        //  cout << "ERROR: UpdateActiveSet: Exact active node condition violated "
        //       << "for node ID: " << cnode->Id() << endl;

        // check for tensile contact forces
        if (nz <= 0) // no averaging of Lagrange multipliers
        //if (0.5*nz+0.5*nzold <= 0) // averaging of Lagrange multipliers
        {
          cnode->Active() = false;
          activesetconv_ = false;
          
          // friction
          if (friction_) static_cast<FriNode*>(cnode)->Data().Slip() = false;     
        }
        
        // only do something for friction
        else
        {
          // friction tresca
          if (ftype == INPAR::CONTACT::friction_tresca)
          {
            FriNode* frinode = static_cast<FriNode*>(cnode);
            double frbound = Params().get<double>("FRBOUND");
            double ct = Params().get<double>("SEMI_SMOOTH_CT");

            if(frinode->Data().Slip() == false)
            {
              // check (tz+ct*tjump)-frbound <= 0
              if(abs(tz+ct*tjump)-frbound <= 0) {}
                // do nothing (stick was correct)
              else
              {
                 frinode->Data().Slip() = true;
                 activesetconv_ = false;
              }
            }
            else
            {
              // check (tz+ct*tjump)-frbound > 0
              if(abs(tz+ct*tjump)-frbound > 0) {}
                // do nothing (slip was correct)
              else
              {
#ifdef CONTACTFRICTIONLESSFIRST
                if(frinode->Data().ActiveOld()==false)
                {}
                else
                {
                 frinode->Data().Slip() = false;
                 activesetconv_ = false;
                }
#else
                frinode->Data().Slip() = false;
                activesetconv_ = false;
#endif
              }
            }
          } // if (ftype == INPAR::CONTACT::friction_tresca)

          // friction coulomb
          if (ftype == INPAR::CONTACT::friction_coulomb)
          {
            FriNode* frinode = static_cast<FriNode*>(cnode);
            double frcoeff = Params().get<double>("FRCOEFF");
            double ct = Params().get<double>("SEMI_SMOOTH_CT");

            if(frinode->Data().Slip() == false)
            {
              // check (tz+ct*tjump)-frbound <= 0
              if(abs(tz+ct*tjump)-frcoeff*nz <= 0) {}
                // do nothing (stick was correct)
              else
              {
                 frinode->Data().Slip() = true;
                 activesetconv_ = false;
              }
            }
            else
            {
              // check (tz+ct*tjump)-frbound > 0
              if(abs(tz+ct*tjump)-frcoeff*nz > 0) {}
                // do nothing (slip was correct)
              else
              {
#ifdef CONTACTFRICTIONLESSFIRST
                if(frinode->Data().ActiveOld()==false)
                {}
                else
                {
                 frinode->Data().Slip() = false;
                 activesetconv_ = false;
                }
#else
                frinode->Data().Slip() = false;
                activesetconv_ = false;
#endif
              }
            }
          } // if (ftype == INPAR::CONTACT::friction_coulomb)
        } // if (nz <= 0)
      } // if (cnode->Active()==false)
    } // loop over all slave nodes
  } // loop over all interfaces

  // broadcast convergence status among processors
  int convcheck = 0;
  int localcheck = activesetconv_;
  Comm().SumAll(&localcheck,&convcheck,1);

  // active set is only converged, if converged on all procs
  // if not, increase no. of active set steps too
  if (convcheck!=Comm().NumProc())
  {
    activesetconv_=false;
    activesetsteps_ += 1;
  }

  // update zig-zagging history (shift by one)
  if (zigzagtwo_!=null) zigzagthree_  = rcp(new Epetra_Map(*zigzagtwo_));
  if (zigzagone_!=null) zigzagtwo_    = rcp(new Epetra_Map(*zigzagone_));
  if (gactivenodes_!=null) zigzagone_ = rcp(new Epetra_Map(*gactivenodes_));

  // (re)setup active global Epetra_Maps
  gactivenodes_ = null;
  gactivedofs_ = null;
  gactiven_ = null;
  gactivet_ = null;
  gslipnodes_ = null;
  gslipdofs_ = null;
  gslipt_ = null;

  // update active sets of all interfaces
  // (these maps are NOT allowed to be overlapping !!!)
  for (int i=0;i<(int)interface_.size();++i)
  {
    interface_[i]->BuildActiveSet();
    gactivenodes_ = LINALG::MergeMap(gactivenodes_,interface_[i]->ActiveNodes(),false);
    gactivedofs_ = LINALG::MergeMap(gactivedofs_,interface_[i]->ActiveDofs(),false);
    gactiven_ = LINALG::MergeMap(gactiven_,interface_[i]->ActiveNDofs(),false);
    gactivet_ = LINALG::MergeMap(gactivet_,interface_[i]->ActiveTDofs(),false);
    if(friction_)
    {  
      gslipnodes_ = LINALG::MergeMap(gslipnodes_,interface_[i]->SlipNodes(),false);
      gslipdofs_ = LINALG::MergeMap(gslipdofs_,interface_[i]->SlipDofs(),false);
      gslipt_ = LINALG::MergeMap(gslipt_,interface_[i]->SlipTDofs(),false);
    }
  }

  // CHECK FOR ZIG-ZAGGING / JAMMING OF THE ACTIVE SET
  // *********************************************************************
  // A problem of the active set strategy which sometimes arises is known
  // from optimization literature as jamming or zig-zagging. This means
  // that within a load/time-step the algorithm can have more than one
  // solution due to the fact that the active set is not unique. Hence the
  // algorithm jumps between the solutions of the active set. The non-
  // uniquenesss results either from highly curved contact surfaces or
  // from the FE discretization, Thus the uniqueness of the closest-point-
  // projection cannot be guaranteed.
  // *********************************************************************
  // To overcome this problem we monitor the development of the active
  // set scheme in our contact algorithms. We can identify zig-zagging by
  // comparing the current active set with the active set of the second-
  // and third-last iteration. If an identity occurs, we consider the
  // active set strategy as converged instantly, accepting the current
  // version of the active set and proceeding with the next time/load step.
  // This very simple approach helps stabilizing the contact algorithm!
  // *********************************************************************
  bool zigzagging = false;
  // FIXGIT: For tresca friction zig-zagging is not eliminated
  if (ftype != INPAR::CONTACT::friction_tresca && ftype != INPAR::CONTACT::friction_coulomb)
  {
    // frictionless contact
    if (ActiveSetSteps()>2)
    {
      if (zigzagtwo_!=null)
      {
        if (zigzagtwo_->SameAs(*gactivenodes_))
        {
          // set active set converged
          activesetconv_ = true;
          zigzagging = true;
  
          // output to screen
          if (Comm().MyPID()==0)
            cout << "DETECTED 1-2 ZIG-ZAGGING OF ACTIVE SET................." << endl;
        }
      }
  
      if (zigzagthree_!=null)
      {
        if (zigzagthree_->SameAs(*gactivenodes_))
        {
          // set active set converged
          activesetconv_ = true;
          zigzagging = true;
  
          // output to screen
          if (Comm().MyPID()==0)
            cout << "DETECTED 1-2-3 ZIG-ZAGGING OF ACTIVE SET................" << endl;
        }
      }
    }
  } // if (ftype != INPAR::CONTACT::friction_tresca && ftype != INPAR::CONTACT::friction_coulomb)

  
  // reset zig-zagging history
  if (activesetconv_==true)
  {
    zigzagone_  = null;
    zigzagtwo_  = null;
    zigzagthree_= null;
  }

  // output of active set status to screen
  if (Comm().MyPID()==0 && activesetconv_==false)
    cout << "ACTIVE SET ITERATION " << ActiveSetSteps()-1
         << " NOT CONVERGED - REPEAT TIME STEP................." << endl;
  else if (Comm().MyPID()==0 && activesetconv_==true)
    cout << "ACTIVE SET CONVERGED IN " << ActiveSetSteps()-zigzagging
         << " STEP(S)................." << endl;

  // update flag for global contact status
  if (gactivenodes_->NumGlobalElements())
    IsInContact()=true;

  return;
}

/*----------------------------------------------------------------------*
 |  Update active set and check for convergence (public)      popp 06/08|
 *----------------------------------------------------------------------*/
void CONTACT::CoLagrangeStrategy::UpdateActiveSetSemiSmooth()
{
  // FIXME: Here we do not consider zig-zagging yet!

  // get out gof here if not in the semi-smooth Newton case
  bool semismooth = Teuchos::getIntegralValue<int>(Params(),"SEMI_SMOOTH_NEWTON");
  if (!semismooth) return;
  
  // get input parameter ftype
  INPAR::CONTACT::FrictionType ftype =
    Teuchos::getIntegralValue<INPAR::CONTACT::FrictionType>(Params(),"FRICTION");
  
  // read weighting factor cn
  // (this is necessary in semi-smooth Newton case, as the search for the
  // active set is now part of the Newton iteration. Thus, we do not know
  // the active / inactive status in advance and we can have a state in
  // which both the condition znormal = 0 and wgap = 0 are violated. Here
  // we have to weigh the two violations via cn!
  double cn = Params().get<double>("SEMI_SMOOTH_CN");

  // assume that active set has converged and check for opposite
  activesetconv_=true;

  // loop over all interfaces
  for (int i=0; i<(int)interface_.size(); ++i)
  {
    //if (i>0) dserror("ERROR: UpdateActiveSet: Double active node check needed for n interfaces!");

    // loop over all slave nodes on the current interface
    for (int j=0;j<interface_[i]->SlaveRowNodes()->NumMyElements();++j)
    {
      int gid = interface_[i]->SlaveRowNodes()->GID(j);
      DRT::Node* node = interface_[i]->Discret().gNode(gid);
      if (!node) dserror("ERROR: Cannot find node with gid %",gid);
      CoNode* cnode = static_cast<CoNode*>(node);

      // compute weighted gap
      double wgap = (*g_)[g_->Map().LID(gid)];

      // compute normal part of Lagrange multiplier
      double nz = 0.0;
      double nzold = 0.0;
      for (int k=0;k<3;++k)
      {
        nz += cnode->MoData().n()[k] * cnode->MoData().lm()[k];
        nzold += cnode->MoData().n()[k] * cnode->MoData().lmold()[k];
      }
      
      // friction
      double ct = Params().get<double>("SEMI_SMOOTH_CT");
      vector<double> tz (Dim()-1,0);
      vector<double> tjump (Dim()-1,0);
      double euclidean = 0.0;

      if (friction_)
      {
        // static cast
        FriNode* frinode = static_cast<FriNode*>(cnode);
                
        // compute tangential parts and of Lagrange multiplier and incremental jumps
        for (int i=0;i<Dim();++i)
        {          
          tz[0] += frinode->CoData().txi()[i]*frinode->MoData().lm()[i];
          if(Dim()==3) tz[1] += frinode->CoData().teta()[i]*frinode->MoData().lm()[i];

           tjump[0] += frinode->CoData().txi()[i]*frinode->Data().jump()[i];
           if(Dim()==3) tjump[1] += frinode->CoData().teta()[i]*frinode->Data().jump()[i];
        }

        // evaluate euclidean norm |tz+ct.tjump|
        vector<double> sum (Dim()-1,0);
        sum[0] = tz[0]+ct*tjump[0];
        if (Dim()==3) sum[1] = tz[1]+ct*tjump[1];
        if (Dim()==2) euclidean = abs(sum[0]);
        if (Dim()==3) euclidean = sqrt(sum[0]*sum[0]+sum[1]*sum[1]);
       }

      // check nodes of inactive set *************************************
      if (cnode->Active()==false)
      {
        // check for fulfilment of contact condition
        //if (abs(nz) > 1e-8)
        //  cout << "ERROR: UpdateActiveSet: Exact inactive node condition violated "
        //       <<  "for node ID: " << cnode->Id() << endl;

        // check for penetration and/or tensile contact forces
        if (nz - cn*wgap > 0) // no averaging of Lagrange multipliers
        //if ((0.5*nz+0.5*nzold) - cn*wgap > 0) // averaging of Lagrange multipliers
        {
          cnode->Active() = true;
          activesetconv_ = false;
          
          // friction
          if (friction_)
          {
            // nodes coming into contact
            static_cast<FriNode*>(cnode)->Data().Slip() = true;
#ifdef CONTACTFRICTIONLESSFIRST
            if (static_cast<FriNode*>(cnode)->Data().ActiveOld()==false)
              static_cast<FriNode*>(cnode)->Data().Slip() = true;
#endif
          } 
        }
      }

      // check nodes of active set ***************************************
      else
      {
        // check for fulfilment of contact condition
        //if (abs(wgap) > 1e-8)
        //  cout << "ERROR: UpdateActiveSet: Exact active node condition violated "
        //       << "for node ID: " << cnode->Id() << endl;

        // check for tensile contact forces and/or penetration
        if (nz - cn*wgap <= 0) // no averaging of Lagrange multipliers
        //if ((0.5*nz+0.5*nzold) - cn*wgap <= 0) // averaging of Lagrange multipliers
        {
          cnode->Active() = false;
          activesetconv_ = false;
          
          // friction
          if (friction_) static_cast<FriNode*>(cnode)->Data().Slip() = false;
        }
        
        // only do something for friction
        else
        {  
          // friction tresca
          if (ftype == INPAR::CONTACT::friction_tresca)
          {
            FriNode* frinode = static_cast<FriNode*>(cnode);
            double frbound = Params().get<double>("FRBOUND");

            if(frinode->Data().Slip() == false)
            {
              // check (euclidean)-frbound <= 0
              if(euclidean-frbound <= 0) {}
                // do nothing (stick was correct)
              else
              {
                 frinode->Data().Slip() = true;
                 activesetconv_ = false;
              }
            }
            else
            {
              // check (euclidean)-frbound > 0
              if(euclidean-frbound > 0) {}
               // do nothing (slip was correct)
              else
              {
#ifdef CONTACTFRICTIONLESSFIRST
                if(frinode->Data().ActiveOld()==false)
                {}
                else
                {
                 frinode->Data().Slip() = false;
                 activesetconv_ = false;
                }
#else
                frinode->Data().Slip() = false;
                activesetconv_ = false;
#endif
              }
            }
          } // if (fytpe=="tresca")

          // friction coulomb
          if (ftype == INPAR::CONTACT::friction_coulomb)
          {
            FriNode* frinode = static_cast<FriNode*>(cnode);
            double frcoeff = Params().get<double>("FRCOEFF");
            if(frinode->Data().Slip() == false)
            {
              // check (euclidean)-frbound <= 0
#ifdef CONTACTCOMPHUEBER
            if(euclidean-frcoeff*(nz-cn*wgap) <= 0) {}
#else
            if(euclidean-frcoeff*nz <= 0) {}
#endif
                // do nothing (stick was correct)
              else
              {
                 frinode->Data().Slip() = true;
                 activesetconv_ = false;
              }
            }
            else
            {
              // check (euclidean)-frbound > 0
#ifdef CONTACTCOMPHUEBER
              if(euclidean-frcoeff*(nz-cn*wgap) > 0) {}
#else
              if(euclidean-frcoeff*nz > 0) {}
#endif
              // do nothing (slip was correct)
              else
              {
#ifdef CONTACTFRICTIONLESSFIRST
                if(frinode->Data().ActiveOld()==false)
                {}
                else
                {
                 frinode->Data().Slip() = false;
                 activesetconv_ = false;
                }
#else
                frinode->Data().Slip() = false;
                activesetconv_ = false;
#endif
              }
            }
          } // if (ftype == INPAR::CONTACT::friction_coulomb)
        } // if (nz - cn*wgap <= 0)
      } // if (cnode->Active()==false)
    } // loop over all slave nodes
  } // loop over all interfaces

  // broadcast convergence status among processors
  int convcheck = 0;
  int localcheck = activesetconv_;
  Comm().SumAll(&localcheck,&convcheck,1);

  // active set is only converged, if converged on all procs
  // if not, increase no. of active set steps too
  if (convcheck!=Comm().NumProc())
  {
    activesetconv_ = false;
    activesetsteps_ += 1;
  }
  
  // also update special flag for semi-smooth Newton convergence
  activesetssconv_ = activesetconv_;

  // (re)setup active global Epetra_Maps
  gactivenodes_ = null;
  gactivedofs_ = null;
  gactiven_ = null;
  gactivet_ = null;
  gslipnodes_ = null;
  gslipdofs_ = null;
  gslipt_ = null;

  // update active sets of all interfaces
  // (these maps are NOT allowed to be overlapping !!!)
  for (int i=0;i<(int)interface_.size();++i)
  {
    interface_[i]->BuildActiveSet();
    gactivenodes_ = LINALG::MergeMap(gactivenodes_,interface_[i]->ActiveNodes(),false);
    gactivedofs_ = LINALG::MergeMap(gactivedofs_,interface_[i]->ActiveDofs(),false);
    gactiven_ = LINALG::MergeMap(gactiven_,interface_[i]->ActiveNDofs(),false);
    gactivet_ = LINALG::MergeMap(gactivet_,interface_[i]->ActiveTDofs(),false);
    if(friction_)
    {
      gslipnodes_ = LINALG::MergeMap(gslipnodes_,interface_[i]->SlipNodes(),false);
      gslipdofs_ = LINALG::MergeMap(gslipdofs_,interface_[i]->SlipDofs(),false);
      gslipt_ = LINALG::MergeMap(gslipt_,interface_[i]->SlipTDofs(),false);
    } 
  }

  // output of active set status to screen
  if (Comm().MyPID()==0 && activesetconv_==false)
    cout << "ACTIVE SET HAS CHANGED... CHANGE No. " << ActiveSetSteps()-1 << endl;

  // update flag for global contact status
  if (gactivenodes_->NumGlobalElements())
    IsInContact()=true;

  return;
}

#endif // CCADISCRET
