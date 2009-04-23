/*!----------------------------------------------------------------------
\file charmm.cpp
\brief CHARMm Interface to compute the mechanical properties of integrins

<pre>
Maintainer: Robert Metzke
	    metzke@lnm.mw.tum.de
	    http://www.lnm.mw.tum.de
	    089 - 289-15244
</pre>
 *----------------------------------------------------------------------*/
#ifdef CCADISCRET

#include <vector>
#include <Epetra_SerialDenseMatrix.h>
#include <Epetra_SerialDenseVector.h>
#include "../drt_lib/linalg_serialdensematrix.H"
#include "../drt_lib/linalg_serialdensevector.H"
#include "../drt_lib/linalg_utils.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_lib/drt_dserror.H"
#include "sys/types.h"
#include "sys/stat.h"
#include <math.h>
#include "charmm.H"
#include "../drt_so3/so_hex8.H"

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
MAT::PAR::CHARMM::CHARMM(Teuchos::RCP<MAT::PAR::Material> matdata)
: Parameter(matdata),
density_(matdata->GetDouble("DENS")) {
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
MAT::CHARMM::CHARMM()
: params_(NULL) {
}

MAT::CHARMM::CHARMM(MAT::PAR::CHARMM* params)
: params_(params) {
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::CHARMM::Pack(vector<char>& data) const {
    data.resize(0);

    // pack type of this instance of ParObject
    int type = UniqueParObjectId();
    AddtoPack(data, type);

    // matid
    int matid = -1;
    if (params_ != NULL) matid = params_->Id(); // in case we are in post-process mode
    AddtoPack(data, matid);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::CHARMM::Unpack(const vector<char>& data) {
    int position = 0;
    // extract type
    int type = 0;
    ExtractfromPack(position, data, type);
    if (type != UniqueParObjectId()) dserror("wrong instance type data");

    // matid
    int matid;
    ExtractfromPack(position, data, matid);
    // in post-process mode we do not have any instance of DRT::Problem
    if (DRT::Problem::NumInstances() > 0) {
	const int probinst = DRT::Problem::Instance()->Materials()->GetReadFromProblem();
	MAT::PAR::Parameter* mat = DRT::Problem::Instance(probinst)->Materials()->ParameterById(matid);
	if (mat->Type() == MaterialType())
	    params_ = static_cast<MAT::PAR::CHARMM*> (mat);
	else
	    dserror("Type of parameter material %d does not fit to calling type %d", mat->Type(), MaterialType());
    } else {
	params_ = NULL;
    }

    if (position != (int) data.size())
	dserror("Mismatch in size of data %d <-> %d", (int) data.size(), position);
}


/*----------------------------------------------------------------------*/
//! Setup CHARMm history variables
// Actual and history variables, which need to be stored
// The updated version will be written in every iteration step.
// his_charmm = (    updated time, lasttime,
//                updated lambda(1), lambda(1)(t-dt),
//                updated lambda(2), lambda(2)(t-dt),
//                updated lambda(3), lambda(3)(t-dt),
//                I1,  I1(t-dt))
// his_mat[0] = c1 Neohookean from CHARMM for complete element
/*---------------------------------------------------------------------*/
void MAT::CHARMM::Setup(DRT::Container& data_) {

    // The following needs to come from the parameter in the final version
    vector<string> strain_type;
    strain_type.push_back("principal");
    strain_type.push_back("vector");

    vector<double> his_charmm;
    his_charmm.push_back(0.0); // actual time
    his_charmm.push_back(0.0); // time at last timestep
    for (int i = 0; i < (int) strain_type.size(); i++) {
	his_charmm.push_back(1.0); // updated lambda(1)(t)
	his_charmm.push_back(1.0); // lambda(1)(t-dt)
	his_charmm.push_back(1.0); // updated lambda(2)(t)
	his_charmm.push_back(1.0); // lambda(2)(t-dt)
	his_charmm.push_back(1.0); // updated lambda(3)(t)
	his_charmm.push_back(1.0); // lambda(3)(t-dt)
	his_charmm.push_back(3.0); // updated I1(t)
	his_charmm.push_back(3.0); // I1(t-dt)
	his_charmm.push_back(0.0); // updated v(t)
	his_charmm.push_back(0.0); // v(t-dt)
    }
    data_.Add("his_charmm", his_charmm);

    vector<double> his_mat(1);
    data_.Add("his_mat", his_mat); // material property from CHARMm

    return;
}

/*----------------------------------------------------------------------*/
//! Compute second PK and constitutive tensor
/*----------------------------------------------------------------------*/
void MAT::CHARMM::Evaluate(const LINALG::Matrix<NUM_STRESS_3D, 1 > * glstrain,
	LINALG::Matrix<NUM_STRESS_3D, NUM_STRESS_3D>* cmat,
	LINALG::Matrix<NUM_STRESS_3D, 1 > * stress,
	const int ele_ID,
	const int gp,
	DRT::Container& data_,
	const double time,
	const LINALG::SerialDenseMatrix& xrefe,
	const LINALG::SerialDenseMatrix& xcurr) {
#ifdef DEBUG
    if (!glstrain || !cmat || !stress)
	dserror("Data missing upon input in material CHARMm");
#endif

    // Parameter collection
    // evaluate lamda at origin or at gp
    bool origin = false; // change only of xref and xcurr really working!!!!
    // length of the protein in the main pulling direction [A]
    vector<double> characteristic_length(2);
    // Integrin length !!!!
    characteristic_length[0] = 40.625; //50; //originally 44
    // Collagen length !!!!
    characteristic_length[1] = 100.0;
    // characteristic direction of the protein
    // Possible selcetions:
    // principal = main strain direction (biggest eigenvalue)
    // vector = using the given vector
    // none = don't use the direction
    vector<string> strain_type;
    strain_type.push_back("principal");
    strain_type.push_back("vector");
    vector<LINALG::SerialDenseVector> d;
    LINALG::SerialDenseVector d_1(3);
    LINALG::SerialDenseVector d_2(3);
    d_1(0) = 0;
    d_1(1) = 1;
    d_1(2) = 0;
    d_2(0) = 1;
    d_2(1) = 0;
    d_2(2) = 0;
    d.push_back(d_1);
    d.push_back(d_2);
    // Add the directional space in case of principal direction
    vector<LINALG::SerialDenseVector> ds;
    LINALG::SerialDenseVector ds_1(3);
    LINALG::SerialDenseVector ds_2(3);
    ds_1(0) = 0;
    ds_1(1) = -1;
    ds_1(2) = 0;
    ds_2(0) = 0;
    ds_2(1) = 0;
    ds_2(2) = 0;
    ds.push_back(ds_1);
    ds.push_back(ds_2);
    // Use FCD to compute the acceleration in that direction to compute the
    // pulling force in CHARMm
    bool FCDAcc = true;
    double atomic_mass = 18; // amu; water
    double Facc_scale = 1E26;
    // Use the hard coded charmm results (charmmfakeapi == true) or call charmm really (charmmfakeapi == false)
    bool charmmhard = true;
    // Scale factor (by default c_CHARMm will be in N/m^2. This should be revised)
    const double c_scale = 1E-9;

    // Identity Matrix
    LINALG::Matrix < 3, 3 > I(true);
    for (int i = 0; i < 3; ++i) I(i, i) = 1.0;

    // Green-Lagrange Strain Tensor
    LINALG::Matrix < 3, 3 > E(false);
    E(0, 0) = (*glstrain)(0);
    E(1, 1) = (*glstrain)(1);
    E(2, 2) = (*glstrain)(2);
    E(0, 1) = 0.5 * (*glstrain)(3);
    E(1, 0) = 0.5 * (*glstrain)(3);
    E(1, 2) = 0.5 * (*glstrain)(4);
    E(2, 1) = 0.5 * (*glstrain)(4);
    E(0, 2) = 0.5 * (*glstrain)(5);
    E(2, 0) = 0.5 * (*glstrain)(5);

    // Right Cauchy-Green Tensor  C = 2 * E + I
    LINALG::Matrix < 3, 3 > C(E);
    C.Scale(2.0);
    C += I;

    // Principal Invariants I1 = tr(C) and I3 = det(C)
    double I1 = C(0, 0) + C(1, 1) + C(2, 2);
    double I3 = C(0, 0) * C(1, 1) * C(2, 2) + C(0, 1) * C(1, 2) * C(2, 0)
	    + C(0, 2) * C(1, 0) * C(2, 1) - (C(0, 2) * C(1, 1) * C(2, 0)
	    + C(0, 1) * C(1, 0) * C(2, 2) + C(0, 0) * C(1, 2) * C(2, 1));

    // Calculation of C^-1 (Cinv)
    LINALG::Matrix < 3, 3 > Cinv(C);
    Cinv.Invert();

    /////////////////////////////////////////////////////////////////////CHARMm
    // CHARMm things come here
    if (gp == 0) {

	// Get the strains in the characteristic directions
	LINALG::Matrix < 3, 3 > V(C);
	LINALG::SerialDenseVector lambda(3);
	vector<LINALG::SerialDenseVector> dir_lambdas;
	vector<LINALG::Matrix < 3, 3 > > dir_eigenv;
	// go through number of directions
	for (int i = 0; i < (int) strain_type.size(); i++) {
	    if (strain_type[i].compare("principal") == 0) {
		V.SetCopy(C);
		EvalStrain(origin, xrefe, xcurr, V, lambda);
		// flip the unit vector in case it's showing not in the right direction.
		for (int j = 0; j < 3; j++) {
		    if (ds[i](j) != 0 && ((ds[i](j) < 0 && V(j, 2) > 0) || (ds[i](j) > 0 && V(j, 2) < 0))) {
			V(0, 2) *= -1;
			V(1, 2) *= -1;
			V(2, 2) *= -1;
		    }
		}
		dir_lambdas.push_back(lambda);
		dir_eigenv.push_back(V);
	    } else if (strain_type[i].compare("vector") == 0) {
		V.SetCopy(C);
		for (int k = 0; k < 3; k++) for (int l = 0; l < 3; l++) V(k, l) = d[i](k) * V(k, l) * d[i](l);
		EvalStrain(origin, xrefe, xcurr, V, lambda);
		dir_lambdas.push_back(lambda);
		dir_eigenv.push_back(V);
	    } else if (strain_type[i].compare("none") == 0) {
		V.Clear();
		lambda.Zero();
		dir_lambdas.push_back(lambda);
		dir_eigenv.push_back(V);
	    } else {
		dserror("No valid strain type given for CHARMm!");
	    }
	}
	//cout << dir_lambdas[1](0) << " : " << dir_lambdas[1](1) << " : " << dir_lambdas[1](2) << endl;
	//cout << dir_eigenv[0] << endl;


	// Update and reconfigure history
	vector<double>* his;
	his = data_.GetMutable<vector<double> >("his_charmm");
	if ((*his)[0] < time) {
	    (*his)[1] = (*his)[0]; // time
	    for (int i = 0; i < (int) strain_type.size(); i++) {
		(*his)[3 + (i * 10)] = (*his)[2 + (i * 10)]; // lambda(0)
		(*his)[5 + (i * 10)] = (*his)[4 + (i * 10)]; // lambda(1)
		(*his)[7 + (i * 10)] = (*his)[6 + (i * 10)]; // lambda(2)
		(*his)[9 + (i * 10)] = (*his)[8 + (i * 10)]; // I1
		(*his)[11 + (i * 10)] = (*his)[10 + (i * 10)]; // v
	    }
	}
	(*his)[0] = time;
	for (int i = 0; i < (int) strain_type.size(); i++) {
	    (*his)[2 + (i * 10)] = dir_lambdas[i](0);
	    (*his)[4 + (i * 10)] = dir_lambdas[i](1);
	    (*his)[6 + (i * 10)] = dir_lambdas[i](2);
	    (*his)[8 + (i * 10)] = I1;
	}
	//cout  << (*his)[0] << " : " << (*his)[1] << " : ";
	//for(int i=0;i<(int)strain_type.size();i++) {
	//    cout    << (*his)[2+(i*8)] << " : " << (*his)[3+(i*8)] << " : "
	//            << (*his)[4+(i*8)] << " : " << (*his)[5+(i*8)] << " : "
	//            << (*his)[6+(i*8)] << " : " << (*his)[7+(i*8)] << " : "
	//            << (*his)[8+(i*8)] << " : " << (*his)[9+(i*8)] << " : "
	//	      << (*his)[8+(i*8)] << " : " << (*his)[9+(i*8)] << " : ";
	//}
	//cout <<  endl;

	// Prepare and call CHARMm in its beauty itself
	// get lambda t-dt information
	vector<double> lambda_his;
	for (int i = 0; i < (int) strain_type.size(); i++) {
	    lambda_his.push_back((*his)[7 + (i * 8)]);
	}

	// Data preparation for CHARMm
	// First charateristic direction (FCD)
	// calculate STARTD and ENDD for CHARMm (integrin)
	double FCD_STARTD = characteristic_length[0] * (1 - lambda_his[0]);
	double FCD_ENDD = characteristic_length[0] * (1 - dir_lambdas[0](2)); // Check for better way to choose!!!!
	// get direction for FCD (integrin)
	LINALG::SerialDenseVector FCD_direction(3);
	FCD_direction(0) = dir_eigenv[0](0, 2);
	FCD_direction(1) = dir_eigenv[0](1, 2);
	FCD_direction(2) = dir_eigenv[0](2, 2);
	//cout << "FCD: " << time << " STARTD: " << FCD_STARTD << " ENDD: " << FCD_ENDD << endl;
	
	// Compute the acceleration in FCD direction
	double FCD_v;
	double FCD_a;
	double FCD_Force;
	if (FCDAcc) {
	    EvalAccForce(FCD_STARTD,FCD_ENDD,(*his)[1],time,(*his)[11],atomic_mass,Facc_scale,FCD_v,FCD_a,FCD_Force);
	    (*his)[10] = FCD_v;
	}

	// Second charateristic direction (SCD)
	// calculate STARTD and ENDD for CHARMm (collagen)
	double SCD_STARTD = characteristic_length[1] * (1 - lambda_his[1]);
	double SCD_ENDD = characteristic_length[1] * (1 - dir_lambdas[1](2));
	// get direction for SCD (collagen)
	LINALG::SerialDenseVector SCD_direction(3);
	SCD_direction(0) = dir_eigenv[1](0, 2);
	SCD_direction(1) = dir_eigenv[1](1, 2);
	SCD_direction(2) = dir_eigenv[1](2, 2);
	//cout << "SCD: " << "STARTD: " << SCD_STARTD << " ENDD: " << SCD_ENDD << SCD_direction << endl;


	// Check if results actually can be computed by CHARMm
	//if (STARTD != ENDD) dserror("STARTD and ENDD identical! CHARMm will not produce any results.");

	// Call API to CHARMM
	// Results vector: charmm_result
	// (Energy STARTD, Energy ENDD, #Atoms STARTD, #Atoms ENDD, Volume STARTD, Volume ENDD)
	LINALG::SerialDenseVector direction(3);
	LINALG::SerialDenseVector charmm_result(6);
	if (charmmhard) {
	    // Just give the starting and ending strain in hard coded case
	    CHARMmfakeapi(FCD_STARTD, FCD_ENDD, charmm_result);
	} else {
	    if (FCD_STARTD != FCD_ENDD) {
		CHARMmfileapi(	FCD_STARTD, FCD_ENDD, FCD_direction, FCD_Force, SCD_STARTD, SCD_ENDD, SCD_direction, charmm_result);
	    }
	}

	// Calculate new c (Neo-Hooke) parameter
	// c = E_FE / (I1 - 3) [N/m^2]
	// E_FE = E_MD * 1000 * 4.1868 * ( #Atoms / N_a )
	double E_MD = charmm_result[1] - charmm_result[0]; // kcal/mole
	double Volume = (charmm_result[4]) * 1E-30; // A^3 *  (10^-10)^3
	//double Volume = 1 * 1E-27; // nm^3 *  (10^-9)^3
	double noAtoms = charmm_result[3];
	double I1_lastt = (*his)[9];
	double c = 1 / (I1 - I1_lastt + 3) * 1 / Volume * E_MD * 1000 * 4.1868 * (noAtoms / 6.02214E23);
	if (isnan(c)) c = 0;
	if (isinf(c)) c = 0;
	// c is in N/m^2 -> scaling necessary
	c = c_scale * c;
	//c = 1E-12 * c;
	vector<double>* his_mat;
	his_mat = data_.GetMutable<vector<double> >("his_mat");
	if (FCD_STARTD != FCD_ENDD) {
	    if (I1 == 3) (*his_mat)[0] = c;
	    else (*his_mat)[0] = c * ((I1 - I1_lastt) / (I1 - 3));
	    //cout << "c = " << (*his_mat)[0] << " I1_lastt = " << I1_lastt <<  endl
	    //<< "-------------------------------------------------------" << endl;
	} else {
	    (*his_mat)[0] = 0.0;
	}

	//cout << "MD Result: " << charmm_result[0] << ":" << charmm_result[1] << " " 
	//        << ( charmm_result[1] - charmm_result[0]) << " " 
	//        << charmm_result[5] << endl;

    }

    //
    ///////////////////////////////////////////////////////////////////////////

    // Material Constants c1 and beta
    double ym = 1000; // intermediate for testing purpose only
    double nu = 0.3; // intermediate for testing purpose only
    double c1 = 0.5 * ym / (2 * (1 + nu)); // intermediate for testing purpose only
    double beta = nu / (1 - 2 * nu);
    if (time > 0.0) {
	vector<double>* his_mat = data_.GetMutable<vector<double> >("his_mat");
	if ((*his_mat)[0] != 0.0) c1 = (*his_mat)[0];
	//cout << time << " " << c1 << endl;
    } else {
	//c1 = 0.05;
    }

    // Energy
    //double W = c1/beta * (pow(I3,-beta) - 1) + c1 * (I1-3);

    // PK2 Stresses
    LINALG::Matrix < 3, 3 > PK2(false);
    int i, j;
    for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++) {
	    PK2(i, j) = 2 * c1 * (I(i, j) - pow(I3, -beta) * Cinv(i, j));
	}

    // Transfer PK2 tensor to stress vector
    (*stress)(0) = PK2(0, 0);
    (*stress)(1) = PK2(1, 1);
    (*stress)(2) = PK2(2, 2);
    (*stress)(3) = PK2(0, 1);
    (*stress)(4) = PK2(1, 2);
    (*stress)(5) = PK2(0, 2);

    // Elasticity Tensor
    double delta6 = 4. * c1 * beta * pow(I3, -beta);
    double delta7 = 4. * c1 * pow(I3, -beta);

    int k, l;
    LINALG::Matrix < 9, 9 > ET(false);


    for (k = 0; k < 3; k++)
	for (l = 0; l < 3; l++) {
	    ET(k, l) = delta6 * (Cinv(0, 0) * Cinv(k, l)) +
		    delta7 * 0.5 * (Cinv(0, k) * Cinv(0, l) + Cinv(0, l) * Cinv(0, k));
	    ET(k + 3, l) = delta6 * (Cinv(1, 0) * Cinv(k, l)) +
		    delta7 * 0.5 * (Cinv(1, k) * Cinv(0, l) + Cinv(1, l) * Cinv(0, k));
	    ET(k + 3, l + 3) = delta6 * (Cinv(1, 1) * Cinv(k, l)) +
		    delta7 * 0.5 * (Cinv(1, k) * Cinv(1, l) + Cinv(1, l) * Cinv(1, k));
	    ET(k + 6, l) = delta6 * (Cinv(2, 0) * Cinv(k, l)) +
		    delta7 * 0.5 * (Cinv(2, k) * Cinv(0, l) + Cinv(2, l) * Cinv(0, k));
	    ET(k + 6, l + 3) = delta6 * (Cinv(2, 1) * Cinv(k, l)) +
		    delta7 * 0.5 * (Cinv(2, k) * Cinv(1, l) + Cinv(2, l) * Cinv(1, k));
	    ET(k + 6, l + 6) = delta6 * (Cinv(2, 2) * Cinv(k, l)) +
		    delta7 * 0.5 * (Cinv(2, k) * Cinv(2, l) + Cinv(2, l) * Cinv(2, k));
	}

    (*cmat)(0, 0) = ET(0, 0);
    (*cmat)(0, 1) = ET(1, 1);
    (*cmat)(0, 2) = ET(2, 2);
    (*cmat)(0, 3) = ET(1, 0);
    (*cmat)(0, 4) = ET(2, 1);
    (*cmat)(0, 5) = ET(2, 0);

    (*cmat)(1, 0) = ET(3, 3);
    (*cmat)(1, 1) = ET(4, 4);
    (*cmat)(1, 2) = ET(5, 5);
    (*cmat)(1, 3) = ET(4, 3);
    (*cmat)(1, 4) = ET(5, 4);
    (*cmat)(1, 5) = ET(5, 3);

    (*cmat)(2, 0) = ET(6, 6);
    (*cmat)(2, 1) = ET(7, 7);
    (*cmat)(2, 2) = ET(8, 8);
    (*cmat)(2, 3) = ET(7, 6);
    (*cmat)(2, 4) = ET(8, 7);
    (*cmat)(2, 5) = ET(8, 6);

    (*cmat)(3, 0) = ET(3, 0);
    (*cmat)(3, 1) = ET(4, 1);
    (*cmat)(3, 2) = ET(5, 2);
    (*cmat)(3, 3) = ET(4, 0);
    (*cmat)(3, 4) = ET(5, 1);
    (*cmat)(3, 5) = ET(5, 0);

    (*cmat)(4, 0) = ET(6, 3);
    (*cmat)(4, 1) = ET(7, 4);
    (*cmat)(4, 2) = ET(8, 5);
    (*cmat)(4, 3) = ET(7, 3);
    (*cmat)(4, 4) = ET(8, 4);
    (*cmat)(4, 5) = ET(8, 3);

    (*cmat)(5, 0) = ET(6, 0);
    (*cmat)(5, 1) = ET(7, 1);
    (*cmat)(5, 2) = ET(8, 2);
    (*cmat)(5, 3) = ET(7, 0);
    (*cmat)(5, 4) = ET(8, 1);
    (*cmat)(5, 5) = ET(8, 0);

    return;
}


/*----------------------------------------------------------------------*/
//! Evaluate strains in the charateristic directions
/*----------------------------------------------------------------------*/
void MAT::CHARMM::EvalStrain(const bool& origin,
	const LINALG::SerialDenseMatrix& xrefe,
	const LINALG::SerialDenseMatrix& xcurr,
	LINALG::Matrix < 3, 3 > & C,
	LINALG::SerialDenseVector& lambda) {
    LINALG::SerialDenseVector lambda2(3);
    LINALG::SerialDenseMatrix Ctmp(3, 3);
    if (origin) {
	// vector of dN/dxsi |r=s=t=0.0
	double dN0_vector[24] ={-0.125, -0.125, -0.125,
	    +0.125, -0.125, -0.125,
	    +0.125, +0.125, -0.125,
	    -0.125, +0.125, -0.125,
	    -0.125, -0.125, +0.125,
	    +0.125, -0.125, +0.125,
	    +0.125, +0.125, +0.125,
	    -0.125, +0.125, +0.125};

	// shape function derivatives, evaluated at origin (r=s=t=0.0)
	Epetra_DataAccess CV = Copy;
	Epetra_SerialDenseMatrix dN0(CV, dN0_vector, 3, 3, 8);

	// compute Jacobian, evaluated at element origin (r=s=t=0.0)
	LINALG::SerialDenseMatrix invJacobian0(3, 3);
	invJacobian0.Multiply('N', 'N', 1.0, dN0, xrefe, 0.0);
	const double detJacobian0 = LINALG::NonsymInverse3x3(invJacobian0);
	if (detJacobian0 < 0.0) dserror("Jacobian at origin negativ (CHARMMAPI)");

	//cout << invJacobian0 << endl;
	LINALG::SerialDenseMatrix N_XYZ(3, 8);
	//compute derivatives N_XYZ at gp w.r.t. material coordinates
	// by N_XYZ = J^-1 * N_rst
	N_XYZ.Multiply('N', 'N', 1.0, invJacobian0, dN0, 0.0);
	// (material) deformation gradient F = d xcurr / d xrefe = xcurr^T * N_XYZ^T
	LINALG::SerialDenseMatrix defgrd0(3, 3);
	defgrd0.Multiply('T', 'T', 1.0, xcurr, N_XYZ, 0.0);
	// Right Cauchy-Green tensor = F^T * F
	LINALG::SerialDenseMatrix C0(3, 3);
	C0.Multiply('T', 'N', 1.0, defgrd0, defgrd0, 0.0);

	// compute current eigenvalues of gaussian point C
	for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) Ctmp(i, j) = C0(i, j);
	LINALG::SymmetricEigen(Ctmp, lambda2, 'V', false);
	for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) C(i, j) = Ctmp(i, j);
    } else {
	// compute current eigenvalues of gaussian point C
	LINALG::SerialDenseMatrix Ctmp(3, 3);
	for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) Ctmp(i, j) = C(i, j);
	LINALG::SymmetricEigen(Ctmp, lambda2, 'V', false);
	for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) C(i, j) = Ctmp(i, j);
    }
    for (int i = 0; i < 3; i++) lambda(i) = sqrt(lambda2(i));
}


/*----------------------------------------------------------------------*/
//! Compute acceleration and force in characteristic direction
/*----------------------------------------------------------------------*/
void MAT::CHARMM::EvalAccForce(
	const double& FCD_STARTD,
	const double& FCD_ENDD,
	const double& time_STARTD,
	const double& time_ENDD,
	const double& v_his,
	const double& atomic_mass,
	const double& Facc_scale,
	double& v,
	double& a,
	double& Force) {

    double amu_to_kg = 1.66053886E-27;
    double v_0 = v_his;
    v = 0.0;
    // Compute velocity
    v = abs(FCD_ENDD - FCD_STARTD) / (time_ENDD - time_STARTD);
    // Round v and v_0 off, otherwise comparison is not working
    int d = 5;
    double n = v;
    v = floor(n * pow(10., d) + .5) / pow(10., d);
    n = v_0;
    v_0 = floor(n * pow(10., d) + .5) / pow(10., d);
    if (v == v_his) v_0 = 0.0; // Switch between tangent and secant?? Sure to do that??
    // Compute acceleration and Force
    a = (v - v_0) / (time_ENDD - time_STARTD);
    Force = atomic_mass * amu_to_kg * a * Facc_scale;

    cout    << "ACC: " << a << " " << Force << " "
	    << FCD_STARTD << " " << FCD_ENDD << " "
	    << time_STARTD << " " << time_ENDD << " "
	    << v << " " << v_his << endl;

}

/*----------------------------------------------------------------------*/
//! File based API to CHARMM
/*----------------------------------------------------------------------*/
void MAT::CHARMM::CHARMmfileapi(
	const double FCD_STARTD,
	const double FCD_ENDD,
	const LINALG::SerialDenseVector FCD_direction,
	const double FCD_Force,
	const double SCD_STARTD,
	const double SCD_ENDD,
	const LINALG::SerialDenseVector SCD_direction,
	LINALG::SerialDenseVector& charmm_result) {

    FILE* tty;
    int debug = 0; // write more for debug output
    ostringstream output(ios_base::out);
    ostringstream energy(ios_base::out);
    ostringstream volume(ios_base::out);
    struct stat outfileinfo;
    struct stat energyfileinfo;
    struct stat volumefileinfo;
    ios_base::fmtflags flags = cout.flags(); // Save original flags

    ////////////////////////////////////////////////////////////////////////////
    // Variables needed for CHARMM and getting the results
    // Decide if parallel or seriell
    const bool dont_use_old_results = true;
    const string serpar = "par"; // ser = seriell; par = mpirun; pbs = PBS Torque
    // FC6 setup
    //const char* path = "/home/metzke/ccarat.dev/codedev/charmm.fe.codedev/";
    //const char* charmm = "/home/metzke/bin/charmm";
    //const char* mpicharmm = "/home/metzke/bin/mpicharmm";
    // Mac setup
    const char* path = "/Users/rmetzke/research/baci.dev/codedev/charmm.fe.codedev/";
    const char* charmm = "/Users/rmetzke/bin/charmm";
    const char* mpicharmm = "/Users/rmetzke/bin/mpicharmm";
    //char* input = "1dzi_fem.inp";
    const char* input = "1dzi_fem_min.inp";
    //char* output = "output/FE_cold.out";
    //char* energy = "output/energy_coupling_0kbt.out";
    //char* volume = "output/volume_coupling_0kbt.out";
    const string mdnature = "cold"; // cold = minimization; hot = fully dynamic with thermal energy; pert = pertubation
    output << "output/ACEcold_" << FCD_STARTD << "_" << FCD_ENDD << ".out";
    energy << "output/energy_" << FCD_STARTD << "_" << FCD_ENDD << ".out";
    volume << "output/volume_" << FCD_STARTD << "_" << FCD_ENDD << ".out";
    ////////////////////////////////////////////////////////////////////////////

    // Assemble all file and path names first
    ostringstream outputfile(ios_base::out);
    outputfile << path << output.str();
    ostringstream energyfile(ios_base::out);
    energyfile << path << energy.str();
    ostringstream volumefile(ios_base::out);
    volumefile << path << volume.str();

    // Print out the beginning of the CHARMM info line
    if (debug == 0) cout << setw(4) << left << "MD (" << showpoint << FCD_STARTD << setw(2) << "->" << FCD_ENDD << setw(3) << "): " << flush;

    // Check if the results files already exists
    // In that case skip the charmm call
    if (stat(outputfile.str().c_str(), &outfileinfo) != 0 || stat(energyfile.str().c_str(), &energyfileinfo) != 0 || stat(volumefile.str().c_str(), &volumefileinfo) != 0 || dont_use_old_results) {
	// Assemble the command line for charmm
	ostringstream command(ios_base::out);
	if (serpar.compare("ser") == 0) {
	    command << "cd " << path << " && "
		    << charmm << " FCDSTARTD=" << FCD_STARTD << " FCDENDD=" << FCD_ENDD
		    << " FCDX=" << FCD_direction(0) << " FCDY=" << FCD_direction(1) << " FCDZ=" << FCD_direction(2)
		    << " FCDForce=" << FCD_Force
		    << " SCDSTARTD=" << SCD_STARTD << " SCDENDD=" << SCD_ENDD
		    << " SCDX=" << SCD_direction(0) << " SCDY=" << SCD_direction(1) << " SCDZ=" << SCD_direction(2)
		    << " < " << input << " > " << output.str();
	} else if (serpar.compare("par") == 0) {
	    command << "cd " << path << " && "
		    << "openmpirun -np 2 " << mpicharmm << " FCDSTARTD=" << FCD_STARTD << " FCDENDD=" << FCD_ENDD
		    << " FCDX=" << FCD_direction(0) << " FCDY=" << FCD_direction(1) << " FCDZ=" << FCD_direction(2)
		    << " FCDForce=" << FCD_Force
		    << " SCDSTARTD=" << SCD_STARTD << " SCDENDD=" << SCD_ENDD
		    << " SCDX=" << SCD_direction(0) << " SCDY=" << SCD_direction(1) << " SCDZ=" << SCD_direction(2)
		    << " INPUTFILE=" << input
		    << " < " << "stream.inp" << " > " << output.str();
	} else dserror("What you want now? Parallel or not!");
	if (debug == 1) cout << "CHARMM command:" << endl << command.str() << endl;
	// Open terminal and execute CHARMM
	if (debug == 0) cout << "0|" << flush;
	if ((tty = popen(command.str().c_str(), "r")) == NULL) dserror("CHARMM can not be started!");
	int runresult = pclose(tty);
	if (debug == 1) cout << "Run Result (popen): " << runresult << endl;
	if (debug == 0) cout << runresult << "|";
    } else {
	if (debug == 0) cout << "-1|-1|" << flush;
    }


    // Read the results
    if (mdnature.compare("cold") == 0) {
	Readcoldresults(outputfile, energyfile, volumefile, debug, charmm_result);
    } else {
	dserror("No included MD Simulation technique given!.");
    }

    cout.flags(flags); // Set the flags to the way they were
}


/*----------------------------------------------------------------------*/
//! Read results from cold CHARMm results files
/*----------------------------------------------------------------------*/
void MAT::CHARMM::Readcoldresults(const ostringstream& outputfile,
	const ostringstream& energyfile,
	const ostringstream& volumefile,
	const int debug,
	LINALG::SerialDenseVector& charmm_result) {

    // Check charmm result file for success
    string line;
    // The text line in the outputfile, which shows that the CHARMm execution ended normal
    string charmm_success("                    NORMAL TERMINATION BY NORMAL STOP");
    int resultstatus = 1;
    if (debug == 1) cout << "Outputfile path: " << endl << outputfile.str() << endl;
    ifstream outputstream(outputfile.str().c_str());
    if (outputstream.is_open()) {
	while (!outputstream.eof()) {
	    getline(outputstream, line);
	    if (charmm_success.compare(line) == 0) resultstatus = 0;
	}
	outputstream.close();
    } else dserror("CHARMM API: CHARMM Ouput cannot be read!");
    if (debug == 1) cout << "Result File Check: " << resultstatus << endl;
    if (debug == 0) cout << setw(5) << left << resultstatus << flush;
    if (resultstatus == 1) dserror("CHARMM API: CHARMM run error!");

    // Read energy results
    double ene_old, ene_new;
    vector<string> tokens;
    if (debug == 1) cout << "Energyfile path: " << endl << energyfile.str().c_str() << endl;
    ifstream energystream(energyfile.str().c_str());
    if (energystream.is_open()) {
	while (!energystream.eof()) {
	    getline(energystream, line);
	    if (line.compare(0, 5, "PRIN>") == 0) {
		//cout << line << endl;
		string buf;
		stringstream linestream(line);
		while (linestream >> buf)
		    tokens.push_back(buf);
	    }
	}
	energystream.close();
    } else dserror("CHARMM API: Energy File cannot be opened!");
    ene_old = atof(tokens[2].c_str());
    ene_new = atof(tokens[7].c_str());
    tokens.clear();
    // Output for energy
    if (debug == 1) cout << setw(35) << left << "Energy (string) old | new: " << tokens[2] << " | " << tokens[7] << endl;
    if (debug == 1) cout << setw(35) << left << "Energy (double) old | new | dV: " << setprecision(10) << ene_old << " | " << ene_new << " | " << (ene_old - ene_new) << endl;
    if (debug == 0) cout << setw(4) << "dV:" << setw(15) << left << scientific << setprecision(6) << (ene_new - ene_old);

    // Read # of atoms and volume from file
    double nofatoms_old, nofatoms_new, volume_old, volume_new;
    vector<string> volutokens;
    if (debug == 1) cout << "Volumefile path: " << endl << volumefile.str().c_str() << endl;
    ifstream volumestream(volumefile.str().c_str());
    if (volumestream.is_open()) {
	while (!volumestream.eof()) {
	    getline(volumestream, line);
	    if (line.compare(0, 8, " SELRPN>") == 0) {
		//cout << line << endl;
		string buf;
		stringstream linestream(line);
		while (linestream >> buf)
		    tokens.push_back(buf);
	    }
	    if (line.compare(0, 15, " TOTAL OCCUPIED") == 0) {
		//cout << line << endl;
		string buf;
		stringstream linestream(line);
		while (linestream >> buf)
		    volutokens.push_back(buf);
	    }
	}
	volumestream.close();
    } else dserror("CHARMM API: Volume file can not be opened!");
    // Check if enough text has been found. If not, then unbinding has taken place.
    if (tokens.size() <= 11) tokens.resize(11, "NAN");
    if (volutokens.size() <= 10) volutokens.resize(10, "NAN");
    // Change string to double 
    nofatoms_old = atof(tokens[1].c_str());
    nofatoms_new = atof(tokens[10].c_str());
    volume_old = atof(volutokens[4].c_str());
    volume_new = atof(volutokens[9].c_str());
    // Output for # of atoms and volume
    if (debug == 1) cout << setw(35) << left << "# Atoms (string) old | new: " << tokens[1] << " | " << tokens[10] << endl;
    if (debug == 1) cout << setw(35) << left << "# Atoms (double) old | new | d#: " << nofatoms_old << " | " << nofatoms_new << " | " << (nofatoms_old - nofatoms_new) << endl;
    if (debug == 1) cout << setw(35) << left << "Volume (string) old | new: " << volutokens[4] << " | " << volutokens[9] << endl;
    if (debug == 1) cout << setw(35) << left << "Volume (string) old | new | dVol: " << volume_old << " | " << volume_new << " | " << (volume_old - volume_new) << endl;
    if (debug == 0) cout << setw(8) << "#Atoms:" << setw(10) << left << fixed << setprecision(0) << nofatoms_new << setw(8) << "Volume:" << setw(12) << left << setprecision(2) << volume_new << endl;
    tokens.clear();

    ////////////////////////////////////////////////////////////////////////////
    // Results vector: charmm_result
    // (Energy STARTD, Energy ENDD, #Atoms STARTD, #Atoms ENDD, Volume STARTD, Volume ENDD)
    charmm_result[0] = ene_old;
    charmm_result[1] = ene_new;
    charmm_result[2] = nofatoms_old;
    charmm_result[3] = nofatoms_new;
    charmm_result[4] = volume_old;
    charmm_result[5] = volume_new;
    ////////////////////////////////////////////////////////////////////////////

}


/*----------------------------------------------------------------------*/
//! Hard coupling without calling CHARMm
/*----------------------------------------------------------------------*/
void MAT::CHARMM::CHARMmfakeapi(const double STARTD,
	const double ENDD,
	LINALG::SerialDenseVector& charmm_result) {
    // Define the number n of steps / results from CHARMm (or any MD simluation)
    // If n=2 then it is assumes that always the same values should be used for all steps.
    const int n = 2;
    // Define roundoff for choosing in which step we are
    const double roundoff = 0.005;
    // Hard coded data structure (second variable):
    // (STARTD, Energy, # of Atoms, Volume)
    LINALG::SerialDenseMatrix MD(n, 4);

    ////////////////////////////////////////////////////////////////////////////
    // Hard coded results from MD
    MD(0, 0) = 0.0;
    MD(0, 1) = -330.912;
    MD(0, 2) = 1202;
    MD(0, 3) = 9954.29;

    MD(1, 0) = -0.8125;
    MD(1, 1) = -321.671;
    MD(1, 2) = 1141;
    MD(1, 3) = 9441.08;
    //
    ////////////////////////////////////////////////////////////////////////////

    // Compute the charmm_result vector
    // (Energy STARTD, Energy ENDD, #Atoms STARTD, #Atoms ENDD, Volume STARTD, Volume ENDD)
    ios_base::fmtflags flags = cout.flags(); // Save original flags

    for (int i = n - 1; i >= 0; i--) {
	//cout << ENDD << " " << MD(i,0);
	if (abs(ENDD) == 0.0) { // start call at the beginning; just to give some information
	    i = 0;
	    cout << setw(4) << left << "MD (" << showpoint << STARTD << setw(2) << "->" << ENDD << setw(3) << "): " << flush;
	    charmm_result[0] = NAN;
	    charmm_result[1] = MD(i, 1);
	    charmm_result[2] = NAN;
	    charmm_result[3] = MD(i, 2);
	    charmm_result[4] = NAN;
	    charmm_result[5] = MD(i, 3);
	    cout << setw(4) << "V(0):" << setw(15) << left << scientific << setprecision(6) << (charmm_result[1]);
	    cout << setw(8) << "#Atoms:" << setw(10) << left << fixed << setprecision(0) << charmm_result[3] << setw(8) << "Volume:" << setw(12) << left << setprecision(2) << charmm_result[5] << endl;
	    i = -1; //break loop
	} else if (abs(ENDD) < (abs(MD(i, 0)) + roundoff) && abs(ENDD) > (abs(MD(i, 0)) - roundoff)) {
	    // main loop where basically at every step the data is given
	    cout << setw(4) << left << "MD (" << showpoint << STARTD << setw(2) << "->" << ENDD << setw(3) << "): " << flush;
	    charmm_result[0] = MD(i - 1, 1);
	    charmm_result[1] = MD(i, 1);
	    charmm_result[2] = MD(i - 1, 2);
	    charmm_result[3] = MD(i, 2);
	    charmm_result[4] = MD(i - 1, 3);
	    charmm_result[5] = MD(i, 3);
	    cout << setw(4) << "dV:" << setw(15) << left << scientific << setprecision(6) << (charmm_result[1] - charmm_result[0]);
	    cout << setw(8) << "#Atoms:" << setw(10) << left << fixed << setprecision(0) << charmm_result[3] << setw(8) << "Volume:" << setw(12) << left << setprecision(2) << charmm_result[5] << endl;
	    i = -1; //break loop
	} else {
	    // in case that only one dV is given, use it for all. If more then break.
	    if (n == 2) {
		cout << setw(4) << left << "MD (" << showpoint << STARTD << setw(2) << "->" << ENDD << setw(3) << "): " << flush;
		charmm_result[0] = MD(0, 1);
		charmm_result[1] = MD(1, 1);
		charmm_result[2] = MD(0, 2);
		charmm_result[3] = MD(1, 2);
		charmm_result[4] = MD(0, 3);
		charmm_result[5] = MD(1, 3);
		cout << setw(4) << "dV:" << setw(15) << left << scientific << setprecision(6) << (charmm_result[1] - charmm_result[0]);
		cout << setw(8) << "#Atoms:" << setw(10) << left << fixed << setprecision(0) << charmm_result[3] << setw(8) << "Volume:" << setw(12) << left << setprecision(2) << charmm_result[5] << endl;
		i = -1; //break loop
	    } else {
		dserror("No appropriate MD result found for ENDD");
	    }
	}
    }
    cout.flags(flags);
}


#endif

