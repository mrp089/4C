/*----------------------------------------------------------------------*/
/*! \file
\brief class for handling of micro-macro transitions

\level 3

*/
/*----------------------------------------------------------------------*/


#include "baci_comm_exporter.H"
#include "baci_comm_utils.H"
#include "baci_global_data.H"
#include "baci_lib_container.H"
#include "baci_lib_discret.H"
#include "baci_linalg_utils_densematrix_svd.H"
#include "baci_mat_micromaterial.H"
#include "baci_mat_micromaterialgp_static.H"
#include "baci_mat_par_bundle.H"

BACI_NAMESPACE_OPEN



// This function has to be separated from the remainder of the
// MicroMaterial class. MicroMaterialGP is NOT a member of
// FILTER_OBJECTS hence the MicroMaterial::Evaluate function that
// builds the connection to MicroMaterialGP is not either. In
// post_evaluation.cpp this function is defined to content the
// compiler. If during postprocessing the MicroMaterial::Evaluate
// function should be called, an error is invoked.
//
// -> see also Makefile.objects and setup-objects.sh
//
// In case of any changes of the function prototype make sure that the
// corresponding prototype in src/filter_common/filter_evaluation.cpp is adapted, too!!

// evaluate for master procs
void MAT::MicroMaterial::Evaluate(const CORE::LINALG::Matrix<3, 3>* defgrd,
    const CORE::LINALG::Matrix<6, 1>* glstrain, Teuchos::ParameterList& params,
    CORE::LINALG::Matrix<6, 1>* stress, CORE::LINALG::Matrix<6, 6>* cmat, const int gp,
    const int eleGID)
{
  if (eleGID == -1) dserror("no element ID provided in material");

  CORE::LINALG::Matrix<3, 3>* defgrd_enh = const_cast<CORE::LINALG::Matrix<3, 3>*>(defgrd);

  if (params.get("EASTYPE", "none") != "none")
  {
    // In this case, we have to calculate the "enhanced" deformation gradient
    // from the enhanced GL strains with the help of two polar decompositions

    // First step: determine enhanced material stretch tensor U_enh from C_enh=U_enh^T*U_enh
    // -> get C_enh from enhanced GL strains
    CORE::LINALG::Matrix<3, 3> C_enh;
    for (int i = 0; i < 3; ++i) C_enh(i, i) = 2.0 * (*glstrain)(i) + 1.0;
    // off-diagonal terms are already twice in the Voigt-GLstrain-vector
    C_enh(0, 1) = (*glstrain)(3);
    C_enh(1, 0) = (*glstrain)(3);
    C_enh(1, 2) = (*glstrain)(4);
    C_enh(2, 1) = (*glstrain)(4);
    C_enh(0, 2) = (*glstrain)(5);
    C_enh(2, 0) = (*glstrain)(5);

    // -> polar decomposition of (U^mod)^2
    CORE::LINALG::Matrix<3, 3> Q;
    CORE::LINALG::Matrix<3, 3> S;
    CORE::LINALG::Matrix<3, 3> VT;
    CORE::LINALG::SVD<3, 3>(C_enh, Q, S, VT);  // Singular Value Decomposition
    CORE::LINALG::Matrix<3, 3> U_enh;
    CORE::LINALG::Matrix<3, 3> temp;
    for (int i = 0; i < 3; ++i) S(i, i) = sqrt(S(i, i));
    temp.MultiplyNN(Q, S);
    U_enh.MultiplyNN(temp, VT);

    // Second step: determine rotation tensor R from F (F=R*U)
    // -> polar decomposition of displacement based F
    CORE::LINALG::SVD<3, 3>(*(defgrd_enh), Q, S, VT);  // Singular Value Decomposition
    CORE::LINALG::Matrix<3, 3> R;
    R.MultiplyNN(Q, VT);

    // Third step: determine "enhanced" deformation gradient (F_enh=R*U_enh)
    defgrd_enh->MultiplyNN(R, U_enh);
  }

  // activate microscale material

  int microdisnum = MicroDisNum();
  double V0 = InitVol();
  GLOBAL::Problem::Instance()->Materials()->SetReadFromProblem(microdisnum);

  // avoid writing output also for ghosted elements
  const bool eleowner =
      GLOBAL::Problem::Instance(0)->GetDis("structure")->ElementRowMap()->MyGID(eleGID);

  // get sub communicator including the supporting procs
  Teuchos::RCP<Epetra_Comm> subcomm = GLOBAL::Problem::Instance(0)->GetCommunicators()->SubComm();

  // tell the supporting procs that the micro material will be evaluated
  int task[2] = {0, eleGID};
  subcomm->Broadcast(task, 2, 0);

  // container is filled with data for supporting procs
  std::map<int, Teuchos::RCP<DRT::Container>> condnamemap;
  condnamemap[0] = Teuchos::rcp(new DRT::Container());

  const auto convert_to_serial_dense_matrix = [](const auto& matrix)
  {
    using MatrixType = std::decay_t<decltype(matrix)>;
    constexpr int n_rows = MatrixType::numRows();
    constexpr int n_cols = MatrixType::numCols();
    CORE::LINALG::SerialDenseMatrix data(n_rows, n_cols);
    for (int i = 0; i < n_rows; i++)
      for (int j = 0; j < n_cols; j++) data(i, j) = matrix(i, j);
    return data;
  };

  condnamemap[0]->Add("defgrd", convert_to_serial_dense_matrix(*defgrd_enh));
  condnamemap[0]->Add("cmat", convert_to_serial_dense_matrix(*cmat));
  condnamemap[0]->Add("stress", convert_to_serial_dense_matrix(*stress));
  condnamemap[0]->Add("gp", gp);
  condnamemap[0]->Add("microdisnum", microdisnum);
  condnamemap[0]->Add("V0", V0);
  condnamemap[0]->Add("eleowner", eleowner);

  // maps are created and data is broadcast to the supporting procs
  int tag = 0;
  Teuchos::RCP<Epetra_Map> oldmap = Teuchos::rcp(new Epetra_Map(1, 1, &tag, 0, *subcomm));
  Teuchos::RCP<Epetra_Map> newmap = Teuchos::rcp(new Epetra_Map(1, 1, &tag, 0, *subcomm));
  CORE::COMM::Exporter exporter(*oldmap, *newmap, *subcomm);
  exporter.Export<DRT::Container>(condnamemap);

  // standard evaluation of the micro material
  if (matgp_.find(gp) == matgp_.end())
  {
    matgp_[gp] = Teuchos::rcp(new MicroMaterialGP(gp, eleGID, eleowner, microdisnum, V0));

    /// save density of this micromaterial
    /// -> since we can assign only one material per element, all Gauss points have
    /// the same density -> arbitrarily ask micromaterialgp at gp=0
    if (gp == 0)
    {
      Teuchos::RCP<MicroMaterialGP> actmicromatgp = matgp_[gp];
      density_ = actmicromatgp->Density();
    }
  }

  Teuchos::RCP<MicroMaterialGP> actmicromatgp = matgp_[gp];

  // perform microscale simulation and homogenization (if fint and stiff/mass or stress calculation
  // is required)
  actmicromatgp->PerformMicroSimulation(defgrd_enh, stress, cmat);

  // reactivate macroscale material
  GLOBAL::Problem::Instance()->Materials()->ResetReadFromProblem();

  return;
}

double MAT::MicroMaterial::Density() const { return density_; }


// evaluate for supporting procs
void MAT::MicroMaterial::Evaluate(CORE::LINALG::Matrix<3, 3>* defgrd,
    CORE::LINALG::Matrix<6, 6>* cmat, CORE::LINALG::Matrix<6, 1>* stress, const int gp,
    const int ele_ID, const int microdisnum, double V0, bool eleowner)
{
  GLOBAL::Problem::Instance()->Materials()->SetReadFromProblem(microdisnum);

  if (matgp_.find(gp) == matgp_.end())
  {
    matgp_[gp] = Teuchos::rcp(new MicroMaterialGP(gp, ele_ID, eleowner, microdisnum, V0));
  }

  Teuchos::RCP<MicroMaterialGP> actmicromatgp = matgp_[gp];

  // perform microscale simulation and homogenization (if fint and stiff/mass or stress calculation
  // is required)
  actmicromatgp->PerformMicroSimulation(defgrd, stress, cmat);

  // reactivate macroscale material
  GLOBAL::Problem::Instance()->Materials()->ResetReadFromProblem();

  return;
}


// update for all procs
void MAT::MicroMaterial::Update()
{
  // get sub communicator including the supporting procs
  Teuchos::RCP<Epetra_Comm> subcomm = GLOBAL::Problem::Instance(0)->GetCommunicators()->SubComm();
  if (subcomm->MyPID() == 0)
  {
    // tell the supporting procs that the micro material will be evaluated for the element with id
    // eleID
    int eleID = matgp_.begin()->second->eleID();
    int task[2] = {2, eleID};
    subcomm->Broadcast(task, 2, 0);
  }

  std::map<int, Teuchos::RCP<MicroMaterialGP>>::iterator it;
  for (it = matgp_.begin(); it != matgp_.end(); ++it)
  {
    Teuchos::RCP<MicroMaterialGP> actmicromatgp = (*it).second;
    actmicromatgp->Update();
  }
}


// prepare output for all procs
void MAT::MicroMaterial::PrepareOutput()
{
  // get sub communicator including the supporting procs
  Teuchos::RCP<Epetra_Comm> subcomm = GLOBAL::Problem::Instance(0)->GetCommunicators()->SubComm();
  if (subcomm->MyPID() == 0)
  {
    // tell the supporting procs that the micro material will be prepared for output
    int eleID = matgp_.begin()->second->eleID();
    int task[2] = {1, eleID};
    subcomm->Broadcast(task, 2, 0);
  }

  std::map<int, Teuchos::RCP<MicroMaterialGP>>::iterator it;
  for (it = matgp_.begin(); it != matgp_.end(); ++it)
  {
    Teuchos::RCP<MicroMaterialGP> actmicromatgp = (*it).second;
    actmicromatgp->PrepareOutput();
  }
}


// output for all procs
void MAT::MicroMaterial::Output()
{
  // get sub communicator including the supporting procs
  Teuchos::RCP<Epetra_Comm> subcomm = GLOBAL::Problem::Instance(0)->GetCommunicators()->SubComm();
  if (subcomm->MyPID() == 0)
  {
    // tell the supporting procs that the micro material will be output
    int eleID = matgp_.begin()->second->eleID();
    int task[2] = {3, eleID};
    subcomm->Broadcast(task, 2, 0);
  }

  std::map<int, Teuchos::RCP<MicroMaterialGP>>::iterator it;
  for (it = matgp_.begin(); it != matgp_.end(); ++it)
  {
    Teuchos::RCP<MicroMaterialGP> actmicromatgp = (*it).second;
    actmicromatgp->Output();
  }
}


// read restart for master procs
void MAT::MicroMaterial::ReadRestart(const int gp, const int eleID, const bool eleowner)
{
  int microdisnum = MicroDisNum();
  double V0 = InitVol();

  // get sub communicator including the supporting procs
  Teuchos::RCP<Epetra_Comm> subcomm = GLOBAL::Problem::Instance(0)->GetCommunicators()->SubComm();

  // tell the supporting procs that the micro material will restart
  int task[2] = {4, eleID};
  subcomm->Broadcast(task, 2, 0);

  // container is filled with data for supporting procs
  std::map<int, Teuchos::RCP<DRT::Container>> condnamemap;
  condnamemap[0] = Teuchos::rcp(new DRT::Container());

  condnamemap[0]->Add("gp", gp);
  condnamemap[0]->Add("microdisnum", microdisnum);
  condnamemap[0]->Add("V0", V0);
  condnamemap[0]->Add("eleowner", eleowner);

  // maps are created and data is broadcast to the supporting procs
  int tag = 0;
  Teuchos::RCP<Epetra_Map> oldmap = Teuchos::rcp(new Epetra_Map(1, 1, &tag, 0, *subcomm));
  Teuchos::RCP<Epetra_Map> newmap = Teuchos::rcp(new Epetra_Map(1, 1, &tag, 0, *subcomm));
  CORE::COMM::Exporter exporter(*oldmap, *newmap, *subcomm);
  exporter.Export<DRT::Container>(condnamemap);

  if (matgp_.find(gp) == matgp_.end())
  {
    matgp_[gp] = Teuchos::rcp(new MicroMaterialGP(gp, eleID, eleowner, microdisnum, V0));
  }

  Teuchos::RCP<MicroMaterialGP> actmicromatgp = matgp_[gp];
  actmicromatgp->ReadRestart();
}


// read restart for supporting procs
void MAT::MicroMaterial::ReadRestart(
    const int gp, const int eleID, const bool eleowner, int microdisnum, double V0)
{
  if (matgp_.find(gp) == matgp_.end())
  {
    matgp_[gp] = Teuchos::rcp(new MicroMaterialGP(gp, eleID, eleowner, microdisnum, V0));
  }

  Teuchos::RCP<MicroMaterialGP> actmicromatgp = matgp_[gp];
  actmicromatgp->ReadRestart();
}

BACI_NAMESPACE_CLOSE
