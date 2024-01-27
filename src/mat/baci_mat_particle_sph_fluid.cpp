/*---------------------------------------------------------------------------*/
/*! \file
\brief particle material for SPH fluid

\level 3


*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | headers                                                    sfuchs 06/2018 |
 *---------------------------------------------------------------------------*/
#include "baci_mat_particle_sph_fluid.H"

#include "baci_global_data.H"
#include "baci_mat_par_bundle.H"

BACI_NAMESPACE_OPEN

/*---------------------------------------------------------------------------*
 | define static class member                                 sfuchs 06/2018 |
 *---------------------------------------------------------------------------*/
MAT::ParticleMaterialSPHFluidType MAT::ParticleMaterialSPHFluidType::instance_;

/*---------------------------------------------------------------------------*
 | constructor                                                sfuchs 06/2018 |
 *---------------------------------------------------------------------------*/
MAT::PAR::ParticleMaterialSPHFluid::ParticleMaterialSPHFluid(
    Teuchos::RCP<MAT::PAR::Material> matdata)
    : Parameter(matdata),
      ParticleMaterialBase(matdata),
      ParticleMaterialThermo(matdata),
      refDensFac_(matdata->GetDouble("REFDENSFAC")),
      exponent_(matdata->GetDouble("EXPONENT")),
      backgroundPressure_(matdata->GetDouble("BACKGROUNDPRESSURE")),
      bulkModulus_(matdata->GetDouble("BULK_MODULUS")),
      dynamicViscosity_(matdata->GetDouble("DYNAMIC_VISCOSITY")),
      bulkViscosity_(matdata->GetDouble("BULK_VISCOSITY")),
      artificialViscosity_(matdata->GetDouble("ARTIFICIAL_VISCOSITY"))
{
  // empty constructor
}

/*---------------------------------------------------------------------------*
 | create material instance of matching type with parameters  sfuchs 06/2018 |
 *---------------------------------------------------------------------------*/
Teuchos::RCP<MAT::Material> MAT::PAR::ParticleMaterialSPHFluid::CreateMaterial()
{
  return Teuchos::rcp(new MAT::ParticleMaterialSPHFluid(this));
}

/*---------------------------------------------------------------------------*
 *---------------------------------------------------------------------------*/
CORE::COMM::ParObject* MAT::ParticleMaterialSPHFluidType::Create(const std::vector<char>& data)
{
  MAT::ParticleMaterialSPHFluid* particlematsph = new MAT::ParticleMaterialSPHFluid();
  particlematsph->Unpack(data);
  return particlematsph;
}

/*---------------------------------------------------------------------------*
 | constructor (empty material object)                        sfuchs 06/2018 |
 *---------------------------------------------------------------------------*/
MAT::ParticleMaterialSPHFluid::ParticleMaterialSPHFluid() : params_(nullptr)
{
  // empty constructor
}

/*---------------------------------------------------------------------------*
 | constructor (with given material parameters)               sfuchs 06/2018 |
 *---------------------------------------------------------------------------*/
MAT::ParticleMaterialSPHFluid::ParticleMaterialSPHFluid(MAT::PAR::ParticleMaterialSPHFluid* params)
    : params_(params)
{
  // empty constructor
}

/*---------------------------------------------------------------------------*
 | pack                                                       sfuchs 06/2018 |
 *---------------------------------------------------------------------------*/
void MAT::ParticleMaterialSPHFluid::Pack(CORE::COMM::PackBuffer& data) const
{
  CORE::COMM::PackBuffer::SizeMarker sm(data);
  sm.Insert();

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data, type);

  // matid
  int matid = -1;
  if (params_ != nullptr) matid = params_->Id();  // in case we are in post-process mode
  AddtoPack(data, matid);
}

/*---------------------------------------------------------------------------*
 | unpack                                                     sfuchs 06/2018 |
 *---------------------------------------------------------------------------*/
void MAT::ParticleMaterialSPHFluid::Unpack(const std::vector<char>& data)
{
  std::vector<char>::size_type position = 0;

  CORE::COMM::ExtractAndAssertId(position, data, UniqueParObjectId());

  // matid and recover params_
  int matid;
  ExtractfromPack(position, data, matid);
  params_ = nullptr;
  if (GLOBAL::Problem::Instance()->Materials() != Teuchos::null)
    if (GLOBAL::Problem::Instance()->Materials()->Num() != 0)
    {
      // note: dynamic_cast needed due diamond inheritance structure
      const int probinst = GLOBAL::Problem::Instance()->Materials()->GetReadFromProblem();
      MAT::PAR::Parameter* mat =
          GLOBAL::Problem::Instance(probinst)->Materials()->ParameterById(matid);
      if (mat->Type() == MaterialType())
        params_ = dynamic_cast<MAT::PAR::ParticleMaterialSPHFluid*>(mat);
      else
        dserror("Type of parameter material %d does not fit to calling type %d", mat->Type(),
            MaterialType());
    }

  if (position != data.size()) dserror("Mismatch in size of data %d <-> %d", data.size(), position);
}

BACI_NAMESPACE_CLOSE
