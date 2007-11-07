#ifdef CCADISCRET

#include "vm3_solver.H"
#include "../drt_lib/drt_dserror.H"
#include "../drt_lib/linalg_utils.H"
#include "../drt_lib/linalg_solver.H"

/*----------------------------------------------------------------------*
 |  ctor (public)                                               vg 06/07|
 *----------------------------------------------------------------------*/
VM3_Solver::VM3_Solver(
                                    RefCountPtr<Epetra_CrsMatrix> Aplus,
                                    RefCountPtr<Epetra_CrsMatrix> A,
                                    ParameterList& mlparams,
                                    bool compute) :
iscomputed_(false),
mlparams_(mlparams),
Aplus_(Aplus),
A_(A)
{
  label_  = "VM3_Solver";
  if (compute) Compute();
  return;
}

/*----------------------------------------------------------------------*
 |  multigrid solver for VM3                                    vg 06/07|
 *----------------------------------------------------------------------*/
int VM3_Solver::Solve(const Epetra_Vector& B, Epetra_Vector& X, ParameterList& params)
{
    // do setup if not already done
  if (!iscomputed_) Compute();

  RCP<Epetra_Vector> x = LINALG::CreateVector(Acombined_->OperatorDomainMap(),true);
  RCP<Epetra_Vector> b = LINALG::CreateVector(Acombined_->OperatorRangeMap(),true);
  LINALG::Export(X,*x);
  LINALG::Export(B,*b);
  
  const Epetra_BlockMap& bmap = X.Map();
  Space space;
  space.Reshape(bmap.NumGlobalElements(),bmap.NumMyElements(),bmap.MyGlobalElements());
  
  MultiVector mvX(space,X.Pointers(),1);
  MultiVector mvB(space,B.Pointers(),1);
  
  MultiVector bcoarse;
  MultiVector xcoarse;
  bcoarse = Rtent_ * mvB;
  xcoarse = Rtent_ * mvX;
  
  RCP<Epetra_Vector> xcshifted = LINALG::CreateVector(*coarsermap_,true);
  RCP<Epetra_Vector> bcshifted = LINALG::CreateVector(*coarsermap_,true);
  const int mylength = xcshifted->MyLength();
  for (int i=0; i<mylength; ++i)
  {
    (*xcshifted)[i] = xcoarse(i,0);
    (*bcshifted)[i] = bcoarse(i,0);
  }
  LINALG::Export(*xcshifted,*x);
  LINALG::Export(*bcshifted,*b);
  
  RCP<ParameterList> rcpparams = rcp( new ParameterList(params));
  
  LINALG::Solver solver(rcpparams,Acombined_->RowMatrixRowMap().Comm(),NULL);
  solver.Solve(Acombined_,x,b,true,true);
  //cout << *Acombined_;
  //AztecOO azsolver(Acombined_.get(),x.get(),b.get());
  //azsolver.Iterate(5000,1.0e-08);
  
  LINALG::Export(*x,*xcshifted);
  LINALG::Export(*x,X);
  for (int i=0; i<mylength; ++i)
    xcoarse(i,0) = (*xcshifted)[i];
 
  MultiVector x3h_h;
  x3h_h = Ptent_ * xcoarse;
  
  const int longlength = X.MyLength();
  for (int i=0; i<longlength; ++i)
    X[i] += x3h_h(i,0);

  return 0;

}

/*----------------------------------------------------------------------*
 |  apply multigrid linear preconditioner (public)            vg 06/07|
 *----------------------------------------------------------------------*/
int VM3_Solver::ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const
{
  // apply the preconditioner to X and return result in Y

  // do setup if not already done
  if (!iscomputed_)
  {
    VM3_Solver& tmp = const_cast<VM3_Solver&>(*this);
    tmp.Compute();
  }

  // create a Space
  const Epetra_BlockMap& bmap = X.Map();
  Space space;
  space.Reshape(bmap.NumGlobalElements(),bmap.NumMyElements(),bmap.MyGlobalElements());

  // create input/output mlapi multivectors
  MultiVector b_f(space,1,false);
  MultiVector x_f(space,1,false);
  const int nele = X.Map().NumMyElements();
  for (int i=0; i<nele; ++i)
  {
    x_f(i) = Y[0][i];
    b_f(i) = X[0][i];
  }

  // call AMG
  MultiLevelVCycle(b_f,x_f);

  // copy solution back
  for (int i=0; i<nele; ++i)
    Y[0][i] = x_f(i);

  return 0;
}

/*----------------------------------------------------------------------*
 |  apply multigrid linear preconditioner (private)          vg 06/07|
 *----------------------------------------------------------------------*/
int VM3_Solver::MultiLevelVCycle(MultiVector& b_f,
                                 MultiVector& x_f)
const
{
  int level, levelm1;

  // smoothing on finest level
  {
      // multivector definitions
      MultiVector x_p0(P(0).GetRangeSpace(),1,false);
      MultiVector b_f0(P(0).GetRangeSpace(),1,false);

      // step 1: scale separation
      // scale part on level 1 prolongated to level 0
      x_p0 = P(0) * R(0) * x_f;

      // scale part on current level 0
      x_f = x_f - x_p0;

      // step 2: RHS computation
      // compute additional RHS-term for scale part on level 1
      b_f0 = b_f - A(0) * x_p0;

      // step 3: pre-smoothing
      S(0).Apply(b_f0,x_f);

      // step 4: composition of complete solution
      x_f = x_p0 + x_f;
  }

  // smoothing on medium levels
  for (level=1; level<maxlevels_-1; ++level)
  {
      levelm1=level-1;

      // multivector definitions
      MultiVector b_r(P(level).GetRangeSpace(),1,false);
      MultiVector x_r(P(level).GetRangeSpace(),1,false);
      MultiVector x_p(P(level).GetRangeSpace(),1,false);
      MultiVector b_c(P(level).GetRangeSpace(),1,false);
      MultiVector x_c(P(level).GetRangeSpace(),1,false);
      MultiVector x_rm(P(levelm1).GetRangeSpace(),1,false);
      MultiVector x_m(P(levelm1).GetRangeSpace(),1,false);
      MultiVector x_p0(P(0).GetRangeSpace(),1,false);
      MultiVector x_c0(P(0).GetRangeSpace(),1,false);
      MultiVector x_m0(P(0).GetRangeSpace(),1,false);

      // pre-step: solution and RHS restricted to current level
      Restrict(x_f,x_r,level);

      // step 1: scale separation
      // scale part on level+1 prolongated to current level
      x_p = P(level) * R(level) * x_r;
      // and prolongated to finest level
      Prolong(x_p,x_p0,level);

      // scale part on current level
      x_c = x_r - x_p;

      // (combined) scale parts on finer levels restricted to level-1
      // and prolongated to finest level if necessary
      if (level > 1)
      {
         Restrict(x_f,x_rm,levelm1);
         x_m = x_rm - P(levelm1) * R(levelm1) * x_rm;
         Prolong(x_m,x_m0,levelm1);
      }
      else
      {
         x_m = x_f - P(0) * R(0) * x_f;
         x_m0 = x_m;
      }

      // step 2: RHS computation
      // compute additional RHS-terms for scale parts on level+1 and finer levels
      Restrict(b_f,b_r,level);
      b_c = b_r - A(level) * x_p;
      b_c = b_c - R(levelm1) * A(levelm1) * x_m;

      // step 3: pre-smoothing
      S(level).Apply(b_c,x_c);

      // step 4: composition of complete solution
      // before: result for current scale part prolongated to finest level
      Prolong(x_c,x_c0,level);
      x_f = x_p0 + x_c0 + x_m0;
  }

  // solution on coarsest level
  {
      level = maxlevels_-1;
      levelm1=level-1;

      // multivector definitions
      MultiVector b_r(P(level).GetRangeSpace(),1,false);
      MultiVector x_r(P(level).GetRangeSpace(),1,false);
      MultiVector b_c(P(level).GetRangeSpace(),1,false);
      MultiVector x_rm(P(levelm1).GetRangeSpace(),1,false);
      MultiVector x_m(P(levelm1).GetRangeSpace(),1,false);
      MultiVector x_c0(P(0).GetRangeSpace(),1,false);
      MultiVector x_m0(P(0).GetRangeSpace(),1,false);

      // pre-step: solution restricted to current level
      Restrict(x_f,x_r,level);

      // step 1: scale separation
      // (combined) scale parts on finer levels restricted to level-1
      // and prolongated to finest level if necessary
      if (level > 1)
      {
         Restrict(x_f,x_rm,levelm1);
         x_m = x_rm - P(levelm1) * R(levelm1) * x_rm;
         Prolong(x_m,x_m0,levelm1);
      }
      else
      {
         x_m = x_f - P(0) * R(0) * x_f;
         x_m0 = x_m;
      }

      // step 2: RHS computation
      // compute additional RHS-terms for scale parts on finer levels
      Restrict(b_f,b_r,level);
      b_c = b_r - R(levelm1) * A(levelm1) * x_m;

      // step 3: solution
      S(level).Apply(b_c,x_r);
      //x_r = S(level) * b_c;

      // step 4: composition of complete solution
      // before: result for current scale part prolongated to finest level
      Prolong(x_r,x_c0,level);
      x_f = x_c0 + x_m0;
  }

  // smoothing on medium levels
  for (level=maxlevels_-2; level>0; --level)
  {
      levelm1=level-1;

      // multivector definitions
      MultiVector b_r(P(level).GetRangeSpace(),1,false);
      MultiVector x_r(P(level).GetRangeSpace(),1,false);
      MultiVector x_p(P(level).GetRangeSpace(),1,false);
      MultiVector b_c(P(level).GetRangeSpace(),1,false);
      MultiVector x_c(P(level).GetRangeSpace(),1,false);
      MultiVector x_rm(P(levelm1).GetRangeSpace(),1,false);
      MultiVector x_m(P(levelm1).GetRangeSpace(),1,false);
      MultiVector x_p0(P(0).GetRangeSpace(),1,false);
      MultiVector x_c0(P(0).GetRangeSpace(),1,false);
      MultiVector x_m0(P(0).GetRangeSpace(),1,false);

      // pre-step: solution restricted to current level
      Restrict(x_f,x_r,level);

      // step 1: scale separation
      // scale part on level+1 prolongated to current level
      x_p = P(level) * R(level) * x_r;
      // and prolongated to finest level
      Prolong(x_p,x_p0,level);

      // scale part on current level
      x_c = x_r - x_p;

      // (combined) scale parts on finer levels restricted to level-1
      // and prolongated to finest level if necessary
      if (level > 1)
      {
         Restrict(x_f,x_rm,levelm1);
         x_m = x_rm - P(levelm1) * R(levelm1) * x_rm;
         Prolong(x_m,x_m0,levelm1);
      }
      else
      {
         x_m = x_f - P(0) * R(0) * x_f;
         x_m0 = x_m;
      }

      // step 2: RHS computation
      // compute additional RHS-terms for scales on level+1 and finer levels
      Restrict(b_f,b_r,level);
      b_c = b_r - A(level) * x_p;
      b_c = b_c - R(levelm1) * A(levelm1) * x_m;

      // step 3: pre-smoothing
      S(level).Apply(b_c,x_c);

      // step 4: composition of complete solution
      // before: result for current scale part prolongated to finest level
      Prolong(x_c,x_c0,level);
      x_f = x_p0 + x_c0 + x_m0;
  }

  // smoothing on finest level
  {
      // multivector definitions
      MultiVector x_p0(P(0).GetRangeSpace(),1,false);
      MultiVector b_f0(P(0).GetRangeSpace(),1,false);

      // step 1: scale separation
      // scale part on level 1 prolongated to level 0
      x_p0 = P(0) * R(0) * x_f;

      // scale part on current level 0
      x_f = x_f - x_p0;

      // step 2: RHS computation
      // compute additional RHS-term for scale part on level 1
      b_f0 = b_f - A(0) * x_p0;

      // step 3: pre-smoothing
      S(0).Apply(b_f0,x_f);

      // step 4: composition of complete solution
      x_f = x_p0 + x_f;
  }

  return 0;
}

/*----------------------------------------------------------------------*
 |  restriction operation (private)                            vg 06/07|
 *----------------------------------------------------------------------*/
int VM3_Solver::Restrict(const MultiVector& x_f,
                         MultiVector& x_r,
                         int   level)
const
{
  switch(level)
  {
    case 1:
       x_r = R(0) * x_f;
       break;
    case 2:
       x_r = R(1) * R(0) * x_f;
       break;
    case 3:
       x_r = R(2) * R(1) * R(0) * x_f;
       break;
    case 4:
       x_r = R(3) * R(2) * R(1) * R(0) * x_f;
       break;
    case 5:
       x_r = R(4) * R(3) * R(2) * R(1) * R(0) * x_f;
       break;
    case 6:
       x_r = R(5) * R(4) * R(3) * R(2) * R(1) * R(0) * x_f;
       break;
    default:
      x_r = R(0) * x_f;
  } // end of switch(level)

  return 0;
}

/*----------------------------------------------------------------------*
 |  prolongation operation (private)                            vg 08/07|
 *----------------------------------------------------------------------*/
int VM3_Solver::Prolong(const MultiVector& x_l,
                        MultiVector& x_0,
                        int   level)
const
{
  switch(level)
  {
    case 1:
       x_0 = P(0) * x_l;
       break;
    case 2:
       x_0 = P(0) * P(1) * x_l;
       break;
    case 3:
       x_0 = P(0) * P(1) * P(2) * x_l;
       break;
    case 4:
       x_0 = P(0) * P(1) * P(2) * P(3) * x_l;
       break;
    case 5:
       x_0 = P(0) * P(1) * P(2) * P(3) * P(4) * x_l;
       break;
    case 6:
       x_0 = P(0) * P(1) * P(2) * P(3) * P(4) * P(5) * x_l;
       break;
    default:
      x_0 = P(0) * x_l;
  } // end of switch(level)

  return 0;
}

#if 0 // Volker's version
/*----------------------------------------------------------------------*
 |  compute the preconditioner (public)                         vg 06/07|
 *----------------------------------------------------------------------*/
bool VM3_Solver::Compute()
{
  // setup phase of multigrid
  iscomputed_ = false;

  // this is important to have!!!
  MLAPI::Init();

  // get parameters
  int     maxlevels     = mlparams_.get("max levels",10);
  int     maxcoarsesize = mlparams_.get("coarse: max size",10);
  double* nullspace     = mlparams_.get("null space: vectors",(double*)NULL);
  if (!nullspace) dserror("No nullspace supplied in parameter list");
  int     nsdim         = mlparams_.get("null space: dimension",1);
  int     numpde        = mlparams_.get("PDE equations",1);
  double  damping       = mlparams_.get("aggregation: damping factor",1.33);
  string  eigenanalysis = mlparams_.get("eigen-analysis: type", "Anorm");
  string  ptype         = mlparams_.get("prolongator: type","mod_full");

  // Enhanced parameter extraction in case of list generated via ML-Prec.
  // string  fsmoothertype = mlparams_.get("smoother: type (level 0)","symmetric Gauss-Seidel");
  // double  fsmootherdamp = mlparams_.get("smoother: damping factor (level 0)",1.33);
  // int     fsmoothsweeps = mlparams_.get("smoother: sweeps (level 0)",2);
  // string  smoothertype  = mlparams_.get("smoother: type (level 1)","symmetric Gauss-Seidel");
  // double  smootherdamp  = mlparams_.get("smoother: damping factor (level 1)",1.33);
  // int     smoothsweeps  = mlparams_.get("smoother: sweeps (level 1)",2);
  string  coarsetype    = mlparams_.get("coarse: type","Amesos-KLU");
  // double  coarsedamp    = mlparams_.get("coarse: damping factor",1.33);
  // int     coarsesweeps  = mlparams_.get("coarse: sweeps",2);

  // Currently, only one smoother type can be extracted anyway (in MLAPI::InverseOperator::Reshape),
  // so it doesn't make sense to include different relaxations here. Similarly, only
  // one smoother type is included in the parameter list mlparams. See linalg_solver.cpp
  string  smoothertype  = mlparams_.get("smoother: type","symmetric Gauss-Seidel");
  string  fsmoothertype = smoothertype;

  Space space(A_->RowMatrixRowMap());
  Operator mlapiA(space,space,A_.get(),false);
  Operator mlapiAplus(space,space,Aplus_.get(),false);


  mlapiRmod_.resize(maxlevels);
  mlapiPmod_.resize(maxlevels);
  //mlapiRP_.resize(maxlevels);
  //mlapiPR_.resize(maxlevels);
  //mlapiRA_.resize(maxlevels);
  mlapiA_.resize(maxlevels);
  mlapiS_.resize(maxlevels);
  mlapiAplus_.resize(1);

  // build nullspace;
  MultiVector NS;
  MultiVector NextNS;

  NS.Reshape(mlapiA.GetRangeSpace(),nsdim);
  if (nullspace)
  {
    const int length = NS.GetMyLength();
    for (int i=0; i<nsdim; ++i)
      for (int j=0; j<length; ++j)
        NS(j,i) = nullspace[i*length+j];
  }

  double lambdamax;
  Operator Ptent;
  Operator P;
  Operator Rtent;
  Operator R;
  Operator IminusA;
  Operator C;

  Operator Pmod;
  Operator Rmod;
  InverseOperator S;

  mlapiAplus_[0] = mlapiAplus;
  mlapiA_[0] = mlapiA;

  // build the operators for level 0 first
  int level = 0;

  // build smoother
  if (Comm().MyPID()==0)
  {
    ML_print_line("-", 78);
    cout << "VM3 solver : creating smoother level " << level << endl;
    fflush(stdout);
  }
  S.Reshape(mlapiAplus,fsmoothertype,mlparams_);

  if (level) mlparams_.set("PDE equations", NS.GetNumVectors());


  if (Comm().MyPID()==0)
  {
    ML_print_line("-", 80);
    cout << "VM3 solver: creating level " << level+1 << endl;
    ML_print_line("-", 80);
    fflush(stdout);
  }
  mlparams_.set("workspace: current level",level);
  GetPtent(mlapiA,mlparams_,NS,Ptent,NextNS);
  NS = NextNS;

  if (damping)
  {
    if (eigenanalysis == "Anorm")
      lambdamax = MaxEigAnorm(mlapiA,true);
    else if (eigenanalysis == "cg")
      lambdamax = MaxEigCG(mlapiA,true);
    else if (eigenanalysis == "power-method")
      lambdamax = MaxEigPowerMethod(mlapiA,true);
    else ML_THROW("incorrect parameter (" + eigenanalysis + ")", -1);

    IminusA = GetJacobiIterationOperator(mlapiA,damping/lambdamax);
    P = IminusA * Ptent;
  }
  else
  {
    P = Ptent;
    lambdamax = -1.0;
  }

  R = GetTranspose(P);
  if (damping)
    Rtent = GetTranspose(Ptent);
  else
    Rtent = R;

  // variational coarse grid
  C = GetRAP(R,mlapiA,P);

  // build the matrix-matrix products R*A, R*P and P*R
//  ML_Operator* Rmat = R.GetML_Operator();
//  ML_Operator* Amat = mlapiA.GetML_Operator();
//  ML_Operator* Pmat = P.GetML_Operator();
//  ML_Operator* RAmat;
//  ML_Operator* RPmat;
//  ML_Operator* PRmat;
//  ML_matmat_mult(Rmat, Amat, &RAmat);
//  ML_matmat_mult(Rmat, Pmat, &RPmat);
//  ML_matmat_mult(Pmat, Rmat, &PRmat);
//  Operator RA(mlapiA.GetDomainSpace(),R.GetRangeSpace(), RAmat,false);
//  Operator RP(P.GetDomainSpace(),R.GetRangeSpace(), RPmat,false);
//  Operator PR(R.GetDomainSpace(),P.GetRangeSpace(), PRmat,false);

  // write the temporary values into the correct slot
//  mlapiRA_[level]       = RA;
//  mlapiRP_[level]       = RP;
//  mlapiPR_[level]       = PR;
  mlapiRmod_[level]     = R;
  mlapiPmod_[level]     = P;
  mlapiA_[level+1]      = C;
  mlapiS_[level]        = S;

  // loop for level 1 -- maxleves-1
  for (level=1; level<maxlevels-1; ++level)
  {
    // this level's operator
    mlapiA = mlapiA_[level];

    // build smoother
    if (Comm().MyPID()==0)
    {
      ML_print_line("-", 78);
      cout << "VM3 solver : creating smoother level " << level << endl;
      fflush(stdout);
    }
    S.Reshape(mlapiA,smoothertype,mlparams_);

    if (level) mlparams_.set("PDE equations", NS.GetNumVectors());


    if (Comm().MyPID()==0)
    {
      ML_print_line("-", 80);
      cout << "VM3 solver : creating level " << level+1 << endl;
      ML_print_line("-", 80);
      fflush(stdout);
    }

    mlparams_.set("workspace: current level",level);
    GetPtent(mlapiA,mlparams_,NS,Ptent,NextNS);
    NS = NextNS;

    if (damping)
    {
      if (eigenanalysis == "Anorm")
        lambdamax = MaxEigAnorm(mlapiA,true);
      else if (eigenanalysis == "cg")
        lambdamax = MaxEigCG(mlapiA,true);
      else if (eigenanalysis == "power-method")
        lambdamax = MaxEigPowerMethod(mlapiA,true);
      else ML_THROW("incorrect parameter (" + eigenanalysis + ")", -1);

      IminusA = GetJacobiIterationOperator(mlapiA,damping/lambdamax);
      P = IminusA * Ptent;
    }
    else
    {
      P = Ptent;
      lambdamax = -1.0;
    }

    R = GetTranspose(P);
    if (damping)
      Rtent = GetTranspose(Ptent);
    else
      Rtent = R;

    // variational coarse grid
    C = GetRAP(R,mlapiA,P);

    // build the matrix-matrix products R*A, R*P and P*R
//    Rmat = R.GetML_Operator();
//    Amat = mlapiA.GetML_Operator();
//    Pmat = P.GetML_Operator();
//    ML_matmat_mult(Rmat, Amat, &RAmat);
//    ML_matmat_mult(Rmat, Pmat, &RPmat);
//    ML_matmat_mult(Pmat, Rmat, &PRmat);
//    Operator RA(mlapiA.GetDomainSpace(),R.GetRangeSpace(), RAmat,false);
//    Operator RP(P.GetDomainSpace(),R.GetRangeSpace(), RPmat,false);
//    Operator PR(R.GetDomainSpace(),P.GetRangeSpace(), PRmat,false);

    // write the temporary values into the correct slot
//    mlapiRA_[level]       = RA;
//    mlapiRP_[level]       = RP;
//    mlapiPR_[level]       = PR;
    mlapiRmod_[level]     = R;
    mlapiPmod_[level]     = P;
    mlapiA_[level+1]      = C;
    mlapiS_[level]        = S;

    // break if coarsest level is below specified size
    if (C.GetNumGlobalRows() <= maxcoarsesize)
    {
      ++level;
      break;
    }

  } // for (level=1; level<maxlevels-1; ++level)

  // set coarse solver
  if (Comm().MyPID()==0)
  {
    ML_print_line("-", 78);
    cout << "VM3 solver : creating coarse solver level " << level << endl;
    fflush(stdout);
  }
  S.Reshape(mlapiA_[level],coarsetype,mlparams_);

  mlapiS_[level] = S;

  // store number of levels
  maxlevels_ = level+1;

  iscomputed_ = true;
  return true;
}
#endif




#if 1 // Build monolythic system of equations
/*----------------------------------------------------------------------*
 |  compute the preconditioner (public)                         vg 06/07|
 *----------------------------------------------------------------------*/
bool VM3_Solver::Compute()
{
  // setup phase of multigrid
  iscomputed_ = false;

  // this is important to have!!!
  MLAPI::Init();

  // get parameters
  //int     maxlevels     = mlparams_.get("max levels",10);
  //int     maxcoarsesize = mlparams_.get("coarse: max size",10);
  double* nullspace     = mlparams_.get("null space: vectors",(double*)NULL);
  if (!nullspace) dserror("No nullspace supplied in parameter list");
  int     nsdim         = mlparams_.get("null space: dimension",1);
  //int     numpde        = mlparams_.get("PDE equations",1);
  //double  damping       = mlparams_.get("aggregation: damping factor",1.33);
  string  eigenanalysis = mlparams_.get("eigen-analysis: type", "Anorm");
  string  ptype         = mlparams_.get("prolongator: type","mod_full");
  string  coarsetype    = mlparams_.get("coarse: type","Amesos-KLU");
  string  smoothertype  = mlparams_.get("smoother: type","symmetric Gauss-Seidel");
  string  fsmoothertype = smoothertype;

  Space space(A_->RowMatrixRowMap());
  Operator mlapiA(space,space,A_.get(),false);
  Operator mlapiAplus(space,space,Aplus_.get(),false);

  // build nullspace;
  MultiVector NS;
  MultiVector NextNS;
  NS.Reshape(mlapiA.GetRangeSpace(),nsdim);
  if (nullspace)
  {
    const int length = NS.GetMyLength();
    for (int i=0; i<nsdim; ++i)
      for (int j=0; j<length; ++j)
        NS(j,i) = nullspace[i*length+j];
  }

  // get plain aggregation P and R
  Operator Ptent;
  Operator Rtent;
  GetPtent(mlapiA,mlparams_,NS,Ptent,NextNS);
  Rtent = GetTranspose(Ptent);

  // get coarse grid matrix K11 = R ( K+M ) P
  Operator K11;
  K11 = GetRAP(Rtent,mlapiA,Ptent);
  
  // get coarse grid matrix K12 = R ( K+M )
  Operator K12;
  K12 = Rtent * mlapiA;
  
  // get fine grid matrix K21 = (K+M) * P
  Operator K21;
  K21 = mlapiA * Ptent;
  
  // fine grid matrix is K22 = (K+M+M_fine);
  Operator K22;
  K22 = mlapiAplus;
  
  // get row map of K22
  const Epetra_Map& k22rmap = K22.GetRowMatrix()->RowMatrixRowMap();
  
  // get row map of K11
  const Epetra_Map& k11rmap = K11.GetRowMatrix()->RowMatrixRowMap();
  
  // build new map for row map of K11 that does not overlap with map of K22
  const int maxgid22 = k22rmap.MaxAllGID();
  const int mygidsnewsize = k11rmap.NumMyElements();
  vector<int> mygidsnew(mygidsnewsize);
  for (int i=0; i<mygidsnewsize; ++i)
    mygidsnew[i] = k11rmap.GID(i) + maxgid22+1;
  Epetra_Map k11rmapnew(-1,mygidsnewsize,&mygidsnew[0],0,k11rmap.Comm());

  // get combined row map of K11 and K22
  const int k22rmapsize = k22rmap.NumMyElements();
  const int mytotallength = mygidsnewsize + k22rmapsize;
  mygidsnew.resize(mytotallength);
  int last = 0;
  for (int i=0; i<k22rmapsize; ++i)
  {
    mygidsnew[i] = k22rmap.GID(i);
    last = i;
  }
  last +=1;
  const int k11rmapnewsize = k11rmapnew.NumMyElements();
  for (int i=0; i<k11rmapnewsize; ++i)
    mygidsnew[i+last] = k11rmapnew.GID(i);
  Epetra_Map kcombinedrmap(-1,mytotallength,&mygidsnew[0],0,k11rmap.Comm());
  
  // copy matrices K11 and K12 from row map k11rmap to k11rmapnew
  RCP<Epetra_CrsMatrix> K11new = LINALG::CreateMatrix(k11rmapnew,K11.GetRowMatrix()->MaxNumEntries()+100);
  RCP<Epetra_CrsMatrix> K12new = LINALG::CreateMatrix(k11rmapnew,K11.GetRowMatrix()->MaxNumEntries()+100);
  int length = K12.GetRowMatrix()->RowMatrixColMap().NumMyElements();
  vector<int>    cindices(length);
  vector<double> values(length);
  const int k11rmapsize = k11rmap.NumMyElements();
  for (int i=0; i<k11rmapsize; ++i)
  {
    int numindices = 0;
    int err = K11.GetRowMatrix()->ExtractMyRowCopy(i,length,numindices,&values[0],&cindices[0]);
    if (err<0) dserror("ExtractMyRowCopy returned %d",err);
    // new global row id
    int grid = k11rmapnew.GID(i);
    // new global column id
    for (int j=0; j<numindices; ++j) cindices[j] += (maxgid22+1);
    err = K11new->InsertGlobalValues(grid,numindices,&values[0],&cindices[0]);
    if (err<0) dserror("InsertGlobalValues returned %d",err);
    
    numindices = 0;
    err = K12.GetRowMatrix()->ExtractMyRowCopy(i,length,numindices,&values[0],&cindices[0]);
    err = K12new->InsertGlobalValues(grid,numindices,&values[0],&cindices[0]);
    if (err<0) dserror("InsertGlobalValues returned %d",err);
  }
  K11new->FillComplete(k11rmapnew,k11rmapnew);
  K12new->FillComplete(k22rmap,k11rmapnew);

  // copy matrix K21 to new column map
  RCP<Epetra_CrsMatrix> K21new = LINALG::CreateMatrix(k22rmap,K22.GetRowMatrix()->MaxNumEntries()+100);
  length = K21.GetRowMatrix()->RowMatrixColMap().NumMyElements();
  cindices.resize(length);
  values.resize(length);
  for (int i=0; i<k22rmapsize; ++i)
  {
    int numindices=0;
    int err = K21.GetRowMatrix()->ExtractMyRowCopy(i,length,numindices,&values[0],&cindices[0]);
    int grid = k22rmap.GID(i);
    for (int j=0; j<numindices; ++j) cindices[j] += (maxgid22+1);
    err = K21new->InsertGlobalValues(grid,numindices,&values[0],&cindices[0]);
    if (err<0) dserror("InsertGlobalValues returned %d",err);
  }
  K21new->FillComplete(k11rmapnew,k22rmap);

  // copy all matrices into one big matrix and store it
  int clength = K22.GetRowMatrix()->MaxNumEntries() + K11new->MaxNumEntries() + 100;
  RCP<Epetra_CrsMatrix> Kcombined = LINALG::CreateMatrix(kcombinedrmap,clength);
  LINALG::Add(*Aplus_,false,1.0,*Kcombined,0.0);
  LINALG::Add(*K21new,false,1.0,*Kcombined,1.0);
  LINALG::Add(*K12new,false,1.0,*Kcombined,1.0);
  LINALG::Add(*K11new,false,1.0,*Kcombined,1.0);
  Kcombined->FillComplete(kcombinedrmap,kcombinedrmap);
  Kcombined->OptimizeStorage();
  Acombined_ = Kcombined;

  // finally, we have to fix the nullspace in the parameter list to match the new matrix size
  RCP<vector<double> > newnullsp = rcp(new vector<double>(nsdim*kcombinedrmap.NumMyElements()));
  for (int i=0; i<(int)newnullsp->size(); ++i) (*newnullsp)[i] = 1.0;
  mlparams_.set("null space: vectors",&(*newnullsp)[0]);
  mlparams_.set<RCP<vector<double> > >("nullspace",newnullsp);

  // store Ptent and Rtent
  Ptent_ = Ptent;
  Rtent_ = Rtent;
  
  // store new k11 row map
  coarsermap_ = rcp(new Epetra_Map(k11rmapnew));

  iscomputed_ = true;
  return true;
}
#endif



#endif
