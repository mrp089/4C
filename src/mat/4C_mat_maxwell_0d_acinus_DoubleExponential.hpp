/*----------------------------------------------------------------------*/
/*! \file

\brief Four-element Maxwell material model for reduced dimensional acinus elements with non-linear
spring with double-exponential behaviour, inherits from Maxwell_0d_acinus

The originally linear spring (Stiffness1) of the 4-element Maxwell model is substituted by a
double-exponential pressure-volume relation (derivation: see Ismail Mahmoud's dissertation,
chapter 3.4)


\level 3
*/
/*----------------------------------------------------------------------*/
#ifndef FOUR_C_MAT_MAXWELL_0D_ACINUS_DOUBLEEXPONENTIAL_HPP
#define FOUR_C_MAT_MAXWELL_0D_ACINUS_DOUBLEEXPONENTIAL_HPP


#include "4C_config.hpp"

#include "4C_mat_maxwell_0d_acinus.hpp"
#include "4C_red_airways_elem_params.hpp"
#include "4C_red_airways_elementbase.hpp"

FOUR_C_NAMESPACE_OPEN


namespace Mat
{
  namespace PAR
  {
    /*----------------------------------------------------------------------*/
    /// material parameters for Maxwell 0D acinar material
    ///
    class Maxwell0dAcinusDoubleExponential : public Maxwell0dAcinus
    {
     public:
      /// standard constructor
      Maxwell0dAcinusDoubleExponential(Teuchos::RCP<Core::Mat::PAR::Material> matdata);

      /// create material instance of matching type with my parameters
      Teuchos::RCP<Core::Mat::Material> create_material() override;

    };  // class Maxwell_0d_acinus_DoubleExponential
  }     // namespace PAR


  class Maxwell0dAcinusDoubleExponentialType : public Maxwell0dAcinusType
  {
   public:
    std::string Name() const override { return "maxwell_0d_acinusDoubleExponentialType"; }

    static Maxwell0dAcinusDoubleExponentialType& Instance() { return instance_; };

    Core::Communication::ParObject* Create(const std::vector<char>& data) override;

   private:
    static Maxwell0dAcinusDoubleExponentialType instance_;
  };

  /*----------------------------------------------------------------------*/
  /// Wrapper for Maxwell 0D acinar material
  ///
  /// This object exists (several times) at every element
  class Maxwell0dAcinusDoubleExponential : public Maxwell0dAcinus
  {
   public:
    /// construct empty material object
    Maxwell0dAcinusDoubleExponential();

    /// construct the material object given material parameters
    Maxwell0dAcinusDoubleExponential(Mat::PAR::Maxwell0dAcinus* params);

    //! @name Packing and Unpacking

    /*!
      \brief Return unique ParObject id

      every class implementing ParObject needs a unique id defined at the
      top of parobject.H (this file) and should return it in this method.
    */
    int UniqueParObjectId() const override
    {
      return Maxwell0dAcinusDoubleExponentialType::Instance().UniqueParObjectId();
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
      return Core::Materials::m_0d_maxwell_acinus_doubleexponential;
    }

    /// return copy of this material object
    Teuchos::RCP<Core::Mat::Material> Clone() const override
    {
      return Teuchos::rcp(new Maxwell0dAcinus(*this));
    }

    /*!
      \brief
    */
    void Setup(Input::LineDefinition* linedef) override;

    /*!
       \brief
     */
    void Evaluate(Core::LinAlg::SerialDenseVector& epnp, Core::LinAlg::SerialDenseVector& epn,
        Core::LinAlg::SerialDenseVector& epnm, Core::LinAlg::SerialDenseMatrix& sysmat,
        Core::LinAlg::SerialDenseVector& rhs, const Discret::ReducedLung::ElemParams& params,
        const double NumOfAcini, const double Vo, double time, double dt) override;

   private:
    double e1_01_;
    double e1_lin1_;
    double e1_exp1_;
    double tau1_;

    double e1_02_;
    double e1_lin2_;
    double e1_exp2_;
    double tau2_;
  };

}  // namespace Mat

FOUR_C_NAMESPACE_CLOSE

#endif
