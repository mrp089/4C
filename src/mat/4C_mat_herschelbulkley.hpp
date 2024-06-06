/*----------------------------------------------------------------------*/
/*! \file
\brief non-Newtonian fluid of Herschel-Bulkley type

\level 2

*/
/*----------------------------------------------------------------------*/
#ifndef FOUR_C_MAT_HERSCHELBULKLEY_HPP
#define FOUR_C_MAT_HERSCHELBULKLEY_HPP



#include "4C_config.hpp"

#include "4C_comm_parobjectfactory.hpp"
#include "4C_mat_material_factory.hpp"
#include "4C_material_base.hpp"
#include "4C_material_parameter_base.hpp"

FOUR_C_NAMESPACE_OPEN

namespace Mat
{
  namespace PAR
  {
    /*----------------------------------------------------------------------*/
    /// material parameters
    class HerschelBulkley : public Core::Mat::PAR::Parameter
    {
     public:
      /// standard constructor
      HerschelBulkley(Teuchos::RCP<Core::Mat::PAR::Material> matdata);

      /// @name material parameters
      //@{

      const double tau0_;           /* yield stress */
      const double kfac_;           /* constant factor */
      const double nexp_;           /* exponent */
      const double mexp_;           /* exponent */
      const double lolimshearrate_; /* lower limit of shear rate */
      const double uplimshearrate_; /* upper limit of shear rate */
      const double density_;        /* density */

      //@}

      /// create material instance of matching type with my parameters
      Teuchos::RCP<Core::Mat::Material> create_material() override;

    };  // class HerschelBulkley

  }  // namespace PAR

  class HerschelBulkleyType : public Core::Communication::ParObjectType
  {
   public:
    std::string Name() const override { return "HerschelBulkleyType"; }

    static HerschelBulkleyType& Instance() { return instance_; };

    Core::Communication::ParObject* Create(const std::vector<char>& data) override;

   private:
    static HerschelBulkleyType instance_;
  };

  /// Nonlinear viscosity according to HerschelBulkley
  class HerschelBulkley : public Core::Mat::Material
  {
   public:
    /// construct empty material object
    HerschelBulkley();

    /// construct the material object given material parameters
    explicit HerschelBulkley(Mat::PAR::HerschelBulkley* params);

    //! @name Packing and Unpacking

    /*!
      \brief Return unique ParObject id

      every class implementing ParObject needs a unique id defined at the
      top of parobject.H (this file) and should return it in this method.
    */

    int UniqueParObjectId() const override
    {
      return HerschelBulkleyType::Instance().UniqueParObjectId();
    }

    /*!
      \brief Pack this class so it can be communicated

      Resizes the vector data and stores all information of a class in it.
      The first information to be stored in data has to be the
      unique parobject id delivered by UniqueParObjectId() which will then
      identify the exact class on the receiving processor.

      \param data (in/out): char vector to store class information
    */
    void Pack(Core::Communication::PackBuffer& data) const override;

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
    Core::Materials::MaterialType MaterialType() const override
    {
      return Core::Materials::m_herschelbulkley;
    }

    /// return copy of this material object
    Teuchos::RCP<Core::Mat::Material> Clone() const override
    {
      return Teuchos::rcp(new HerschelBulkley(*this));
    }

    /// return material parameters for element calculation
    //@{

    /// yield stress
    double Tau0() const { return params_->tau0_; }
    /// constant factor
    double KFac() const { return params_->kfac_; }
    /// exponent
    double NExp() const { return params_->nexp_; }
    /// exponent
    double MExp() const { return params_->mexp_; }
    /// lower limit of shear rate
    double LoLimShearRate() const { return params_->lolimshearrate_; }
    /// upper limit of shear rate
    double UpLimShearRate() const { return params_->uplimshearrate_; }
    /// density
    double Density() const override { return params_->density_; }

    //@}

    /// Return quick accessible material parameter data
    Core::Mat::PAR::Parameter* Parameter() const override { return params_; }

   private:
    /// my material parameters
    Mat::PAR::HerschelBulkley* params_;
  };

}  // namespace Mat

FOUR_C_NAMESPACE_CLOSE

#endif
