/*----------------------------------------------------------------------*/
/*! \file
\brief permeable fluid

\level 2

*/
/*----------------------------------------------------------------------*/
#ifndef FOUR_C_MAT_PERMEABLEFLUID_HPP
#define FOUR_C_MAT_PERMEABLEFLUID_HPP



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
    /// material parameters for Permeable fluid
    ///
    /// This object exists only once for each read Newton fluid.
    class PermeableFluid : public Parameter
    {
     public:
      /// standard constructor
      PermeableFluid(Teuchos::RCP<MAT::PAR::Material> matdata);

      /// @name material parameters
      //@{

      /// problem type: Darcy or Darcy-Stokes
      const std::string* type_;
      /// kinematic or dynamic viscosity
      const double viscosity_;
      /// density
      const double density_;
      /// permeability
      const double permeability_;

      //@}

      /// create material instance of matching type with my parameters
      Teuchos::RCP<MAT::Material> CreateMaterial() override;

    };  // class PermeableFluid

  }  // namespace PAR

  class PermeableFluidType : public CORE::COMM::ParObjectType
  {
   public:
    std::string Name() const override { return "PermeableFluidType"; }

    static PermeableFluidType& Instance() { return instance_; };

    CORE::COMM::ParObject* Create(const std::vector<char>& data) override;

   private:
    static PermeableFluidType instance_;
  };

  /*----------------------------------------------------------------------*/
  /// Wrapper for Permeable fluid material
  ///
  /// This object exists (several times) at every element
  class PermeableFluid : public Material
  {
   public:
    /// construct empty material object
    PermeableFluid();

    /// construct the material object given material parameters
    explicit PermeableFluid(MAT::PAR::PermeableFluid* params);

    //! @name Packing and Unpacking

    /*!
      \brief Return unique ParObject id

      every class implementing ParObject needs a unique id defined at the
      top of parobject.H (this file) and should return it in this method.
    */
    int UniqueParObjectId() const override
    {
      return PermeableFluidType::Instance().UniqueParObjectId();
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
    INPAR::MAT::MaterialType MaterialType() const override { return INPAR::MAT::m_permeable_fluid; }

    /// return copy of this material object
    Teuchos::RCP<Material> Clone() const override
    {
      return Teuchos::rcp(new PermeableFluid(*this));
    }

    /// compute reaction coefficient
    double ComputeReactionCoeff() const;

    /// set viscosity (zero for Darcy and greater than zero for Darcy-Stokes)
    double SetViscosity() const;

    /// return type
    std::string Type() const { return *params_->type_; }

    /// return density
    double Density() const override { return params_->density_; }

    /// return viscosity
    double Viscosity() const { return params_->viscosity_; }

    /// return permeability
    double Permeability() const { return params_->permeability_; }

    /// Return quick accessible material parameter data
    MAT::PAR::Parameter* Parameter() const override { return params_; }

   private:
    /// my material parameters
    MAT::PAR::PermeableFluid* params_;
  };

}  // namespace MAT

FOUR_C_NAMESPACE_CLOSE

#endif
