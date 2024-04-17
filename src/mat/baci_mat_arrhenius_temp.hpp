/*----------------------------------------------------------------------*/
/*! \file
\brief scalar transport material according to Sutherland law with
       Arrhenius-type chemical kinetics (temperature)

\level 2

*/
/*----------------------------------------------------------------------*/
#ifndef FOUR_C_MAT_ARRHENIUS_TEMP_HPP
#define FOUR_C_MAT_ARRHENIUS_TEMP_HPP



#include "baci_config.hpp"

#include "baci_comm_parobjectfactory.hpp"
#include "baci_mat_material.hpp"
#include "baci_mat_par_parameter.hpp"

FOUR_C_NAMESPACE_OPEN

namespace MAT
{
  namespace PAR
  {
    /*----------------------------------------------------------------------*/
    /// parameters for scalar transport material according to Sutherland law with Arrhenius-type
    /// chemical kinetics (temperature)
    class ArrheniusTemp : public Parameter
    {
     public:
      /// standard constructor
      ArrheniusTemp(Teuchos::RCP<MAT::PAR::Material> matdata);

      /// @name material parameters
      //@{

      /// reference dynamic viscosity (kg/(m*s))
      const double refvisc_;
      /// reference temperature (K)
      const double reftemp_;
      /// Sutherland temperature (K)
      const double suthtemp_;
      /// specific heat capacity
      const double shc_;
      /// Prandtl number
      const double pranum_;
      /// heat of reaction per unit mass
      const double reaheat_;
      /// pre-exponential constant
      const double preexcon_;
      /// exponent of temperature dependence
      const double tempexp_;
      /// activation temperature
      const double actemp_;
      /// specific gas constant R
      const double gasconst_;

      //@}

      /// create material instance of matching type with my parameters
      Teuchos::RCP<MAT::Material> CreateMaterial() override;

    };  // class ArrheniusTemp

  }  // namespace PAR

  class ArrheniusTempType : public CORE::COMM::ParObjectType
  {
   public:
    std::string Name() const override { return "ArrheniusTempType"; }

    static ArrheniusTempType& Instance() { return instance_; };

    CORE::COMM::ParObject* Create(const std::vector<char>& data) override;

   private:
    static ArrheniusTempType instance_;
  };

  /*----------------------------------------------------------------------*/
  /// wrapper for scalar transport material according to Sutherland law with Arrhenius-type chemical
  /// kinetics (temperature)
  class ArrheniusTemp : public Material
  {
   public:
    /// construct empty material object
    ArrheniusTemp();

    /// construct the material object given material parameters
    explicit ArrheniusTemp(MAT::PAR::ArrheniusTemp* params);

    //! @name Packing and Unpacking

    /*!
      \brief Return unique ParObject id

      every class implementing ParObject needs a unique id defined at the
      top of parobject.H (this file) and should return it in this method.
    */

    int UniqueParObjectId() const override
    {
      return ArrheniusTempType::Instance().UniqueParObjectId();
    }

    /*!
      \brief Pack this class so it can be communicated

      Resizes the vector data and stores all information of a class in it.
      The first information to be stored in data has to be the
      unique parobject id delivered by UniqueParObjectId() which will then
      identify the exact class on the receiving processor.

      \param data (in/out): char vector to store class information
    */
    void Pack(CORE::COMM::PackBuffer& data) const override;

    /*!
      \brief Unpack data from a char vector into this class

      The vector data contains all information to rebuild the
      exact copy of an instance of a class on a different processor.
      The first entry in data has to be an integer which is the unique
      parobject id defined at the top of this file and delivered by
      UniqueParObjectId().

      \param data (in) : vector storing all data to be unpacked into this
      instance.
    */
    void Unpack(const std::vector<char>& data) override;

    //@}

    /// material type
    INPAR::MAT::MaterialType MaterialType() const override { return INPAR::MAT::m_arrhenius_temp; }

    /// return copy of this material object
    Teuchos::RCP<Material> Clone() const override { return Teuchos::rcp(new ArrheniusTemp(*this)); }

    /// compute viscosity
    double ComputeViscosity(const double temp) const;

    /// compute diffusivity
    double ComputeDiffusivity(const double temp) const;

    /// compute density
    double ComputeDensity(const double temp, const double thermpress) const;

    /// compute reaction coefficient
    double ComputeReactionRHS(const double spmf, const double temp) const;

    /// return material parameters for element calculation
    //@{

    /// reference dynamic viscosity (kg/(m*s))
    double RefVisc() const { return params_->refvisc_; }
    /// reference temperature (K)
    double RefTemp() const { return params_->reftemp_; }
    /// Sutherland temperature (K)
    double SuthTemp() const { return params_->suthtemp_; }
    /// specific heat capacity
    double Shc() const { return params_->shc_; }
    /// Prandtl number
    double PraNum() const { return params_->pranum_; }
    /// heat of reaction per unit mass
    double ReaHeat() const { return params_->reaheat_; }
    /// pre-exponential constant
    double PreExCon() const { return params_->preexcon_; }
    /// exponent of temperature dependence
    double TempExp() const { return params_->tempexp_; }
    /// activation temperature
    double AcTemp() const { return params_->actemp_; }
    /// specific gas constant R
    double GasConst() const { return params_->gasconst_; }

    //@}

    /// Return quick accessible material parameter data
    MAT::PAR::Parameter* Parameter() const override { return params_; }

   private:
    /// my material parameters
    MAT::PAR::ArrheniusTemp* params_;
  };

}  // namespace MAT

FOUR_C_NAMESPACE_CLOSE

#endif
