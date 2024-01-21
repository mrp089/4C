/*----------------------------------------------------------------------*/
/*! \file
\brief scalar transport material according to Sutherland law with
       Arrhenius-type chemical kinetics (temperature)

\level 2

*/
/*----------------------------------------------------------------------*/


#include "baci_mat_arrhenius_temp.H"

#include "baci_global_data.H"
#include "baci_mat_par_bundle.H"

#include <vector>

BACI_NAMESPACE_OPEN


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
MAT::PAR::ArrheniusTemp::ArrheniusTemp(Teuchos::RCP<MAT::PAR::Material> matdata)
    : Parameter(matdata),
      refvisc_(matdata->GetDouble("REFVISC")),
      reftemp_(matdata->GetDouble("REFTEMP")),
      suthtemp_(matdata->GetDouble("SUTHTEMP")),
      shc_(matdata->GetDouble("SHC")),
      pranum_(matdata->GetDouble("PRANUM")),
      reaheat_(matdata->GetDouble("REAHEAT")),
      preexcon_(matdata->GetDouble("PREEXCON")),
      tempexp_(matdata->GetDouble("TEMPEXP")),
      actemp_(matdata->GetDouble("ACTEMP")),
      gasconst_(matdata->GetDouble("GASCON"))
{
}

Teuchos::RCP<MAT::Material> MAT::PAR::ArrheniusTemp::CreateMaterial()
{
  return Teuchos::rcp(new MAT::ArrheniusTemp(this));
}


MAT::ArrheniusTempType MAT::ArrheniusTempType::instance_;


CORE::COMM::ParObject* MAT::ArrheniusTempType::Create(const std::vector<char>& data)
{
  MAT::ArrheniusTemp* arrhenius_temp = new MAT::ArrheniusTemp();
  arrhenius_temp->Unpack(data);
  return arrhenius_temp;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
MAT::ArrheniusTemp::ArrheniusTemp() : params_(nullptr) {}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
MAT::ArrheniusTemp::ArrheniusTemp(MAT::PAR::ArrheniusTemp* params) : params_(params) {}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ArrheniusTemp::Pack(CORE::COMM::PackBuffer& data) const
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


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void MAT::ArrheniusTemp::Unpack(const std::vector<char>& data)
{
  std::vector<char>::size_type position = 0;

  CORE::COMM::ExtractAndAssertId(position, data, UniqueParObjectId());

  // matid and recover params_
  int matid;
  ExtractfromPack(position, data, matid);
  params_ = nullptr;
  if (DRT::Problem::Instance()->Materials() != Teuchos::null)
    if (DRT::Problem::Instance()->Materials()->Num() != 0)
    {
      const int probinst = DRT::Problem::Instance()->Materials()->GetReadFromProblem();
      MAT::PAR::Parameter* mat =
          DRT::Problem::Instance(probinst)->Materials()->ParameterById(matid);
      if (mat->Type() == MaterialType())
        params_ = static_cast<MAT::PAR::ArrheniusTemp*>(mat);
      else
        dserror("Type of parameter material %d does not fit to calling type %d", mat->Type(),
            MaterialType());
    }

  if (position != data.size()) dserror("Mismatch in size of data %d <-> %d", data.size(), position);
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
double MAT::ArrheniusTemp::ComputeViscosity(const double temp) const
{
  // previous implementation using "pow"-function appears to be extremely
  // time-consuming sometimes, at least on the computing cluster
  // const double visc =
  // std::pow((temp/RefTemp()),1.5)*((RefTemp()+SuthTemp())/(temp+SuthTemp()))*RefVisc();
  const double visc = sqrt((temp / RefTemp()) * (temp / RefTemp()) * (temp / RefTemp())) *
                      ((RefTemp() + SuthTemp()) / (temp + SuthTemp())) * RefVisc();

  return visc;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
double MAT::ArrheniusTemp::ComputeDiffusivity(const double temp) const
{
  // previous implementation using "pow"-function appears to be extremely
  // time-consuming sometimes, at least on the computing cluster
  // const double diffus =
  // std::pow((temp/RefTemp()),1.5)*((RefTemp()+SuthTemp())/(temp+SuthTemp()))*RefVisc()/PraNum();
  const double diffus = sqrt((temp / RefTemp()) * (temp / RefTemp()) * (temp / RefTemp())) *
                        ((RefTemp() + SuthTemp()) / (temp + SuthTemp())) * RefVisc() / PraNum();

  return diffus;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
double MAT::ArrheniusTemp::ComputeDensity(const double temp, const double thermpress) const
{
  const double density = thermpress / (GasConst() * temp);

  return density;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
double MAT::ArrheniusTemp::ComputeReactionRHS(const double spmf, const double temp) const
{
  const double rearhs =
      -ReaHeat() * PreExCon() * pow(temp, TempExp()) * spmf * exp(-AcTemp() / temp);

  return rearhs;
}

BACI_NAMESPACE_CLOSE
