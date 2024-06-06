/*-----------------------------------------------------------*/
/*! \file

\brief One step theta time integrator.


\level 3

*/
/*-----------------------------------------------------------*/

#ifndef FOUR_C_STRUCTURE_NEW_IMPL_OST_HPP
#define FOUR_C_STRUCTURE_NEW_IMPL_OST_HPP

#include "4C_config.hpp"

#include "4C_structure_new_impl_generic.hpp"

#include <Epetra_MultiVector.h>

FOUR_C_NAMESPACE_OPEN

namespace STR
{
  namespace IMPLICIT
  {
    class OneStepTheta : public Generic
    {
     public:
      //! constructor
      OneStepTheta();


      //! Setup the class variables
      void Setup() override;

      //! (derived)
      void post_setup() override;

      //! Reset state variables (derived)
      void set_state(const Epetra_Vector& x) override;

      //! Apply the rhs only (derived)
      bool ApplyForce(const Epetra_Vector& x, Epetra_Vector& f) override;

      //! Apply the stiffness only (derived)
      bool ApplyStiff(const Epetra_Vector& x, Core::LinAlg::SparseOperator& jac) override;

      //! Apply force and stiff at once (derived)
      bool ApplyForceStiff(
          const Epetra_Vector& x, Epetra_Vector& f, Core::LinAlg::SparseOperator& jac) override;

      //! (derived)
      bool assemble_force(Epetra_Vector& f,
          const std::vector<Inpar::STR::ModelType>* without_these_models = nullptr) const override;

      //! (derived)
      void write_restart(
          Core::IO::DiscretizationWriter& iowriter, const bool& forced_writerestart) const override;

      //! (derived)
      void read_restart(Core::IO::DiscretizationReader& ioreader) override;

      //! (derived)
      double CalcRefNormForce(const enum ::NOX::Abstract::Vector::NormType& type) const override;

      //! (derived)
      double GetIntParam() const override;

      //! @name Monolithic update routines
      //! @{
      //! Update configuration after time step (derived)
      void UpdateStepState() override;

      //! Update everything on element level after time step and after output (derived)
      void UpdateStepElement() override;

      /*! \brief things that should be done after updating [derived]
       *
       *  We use in the OneStepTheta case to update constant contributions (during one time step)
       *  of the set_state routine.*/
      void post_update() override;
      //! @}

      //! @name Predictor routines (dependent on the implicit integration scheme)
      //! @{
      /*! Predict constant displacements, consistent velocities and accelerations (derived) */
      void predict_const_dis_consist_vel_acc(
          Epetra_Vector& disnp, Epetra_Vector& velnp, Epetra_Vector& accnp) const override;

      /*! Predict displacements based on constant velocities and consistent accelerations (derived)
       */
      bool predict_const_vel_consist_acc(
          Epetra_Vector& disnp, Epetra_Vector& velnp, Epetra_Vector& accnp) const override;

      /*! Predict displacements based on constant accelerations and consistent velocities (derived)
       */
      bool PredictConstAcc(
          Epetra_Vector& disnp, Epetra_Vector& velnp, Epetra_Vector& accnp) const override;
      //! @}

      /*! \brief Update constant contributions of the current state for the new time step
       * \f$ t_{n+1} \f$ based on the one-step theta scheme:
       *
       * \f[
       *      V_{n+1} = - (1.0 - \theta)/\theta * V_{n} - 1.0/(\theta * dt) * D_{n}
       *                + 1.0/(\theta * dt) * D_{n+1}
       *      A_{n+1} = - (1.0 - \theta)/\theta * A_{n} - 1.0/(\theta^2 * dt) * V_{n}
       *                - 1.0/(\theta * dt)^2 * D_{n} + 1.0/(\theta * dt)^2 * D_{n+1}
       * \f]
       *
       * Only the constant contributions, i.e. all components that depend on the state n are stored
       * in the const_vel_acc_update_ptr_ multi-vector pointer. The 1st entry represents the
       * velocity, and the 2nd the acceleration.
       *
       *  See the set_state() routine for the iterative update of the current state. */
      void update_constant_state_contributions() override;

      //! @name Attributes access functions
      //@{

      //! Return name
      enum Inpar::STR::DynamicType MethodName() const override
      {
        return Inpar::STR::dyna_onesteptheta;
      }

      //! Provide number of steps, a single-step method returns 1
      int MethodSteps() const override { return 1; }

      //! Give local order of accuracy of displacement part
      int method_order_of_accuracy_dis() const override
      {
        return fabs(MethodLinErrCoeff1()) < 1e-6 ? 2 : 1;
      }

      //! Give local order of accuracy of velocity part
      int method_order_of_accuracy_vel() const override { return method_order_of_accuracy_dis(); }

      //! Return linear error coefficient of displacements
      double method_lin_err_coeff_dis() const override
      {
        if (method_order_of_accuracy_dis() == 1)
          return MethodLinErrCoeff1();
        else
          return MethodLinErrCoeff2();
      }

      //! Return linear error coefficient of velocities
      double method_lin_err_coeff_vel() const override { return method_lin_err_coeff_dis(); }

      //! Linear error coefficient if 1st order accurate
      double MethodLinErrCoeff1() const { return 1. / 2. - theta_; }

      //! Linear error coefficient if 2nd order accurate
      double MethodLinErrCoeff2() const
      {
        return 1. / 6. - theta_ / 2.;  // this is -1/12
      }

      //@}

     protected:
      //! reset the time step dependent parameters for the element evaluation [derived]
      void reset_eval_params() override;

     private:
      /*! \brief Add the viscous and mass contributions to the right hand side (TR-rule)
       *
       * \remark The remaining contributions have been considered in the corresponding model
       *         evaluators. This is due to the fact, that some models use a different
       *         time integration scheme for their terms (e.g. GenAlpha for the structure
       *         and OST for the remaining things).
       *
       *  \f[
       *    Res = M \cdot [\theta  * A_{n+1} + (1-\theta) * A_{n}]
       *        + C \cdot [\theta  * V_{n+1} + (1-\theta) * V_{n}]
       *        + \theta * Res_{\mathrm{statics},n+1} + (1-\theta) * Res_{\mathrm{statics},n}
       *  \f] */
      void add_visco_mass_contributions(Epetra_Vector& f) const override;

      /*! \brief Add the viscous and mass contributions to the jacobian (TR-rule)
       *
       *  \remark The remaining blocks have been considered in the corresponding model
       *          evaluators. This is due to the fact, that some models use a different
       *          time integration scheme for their terms (e.g. GenAlpha for the structure
       *          and OST for the remaining things). Furthermore, constraint/Lagrange
       *          multiplier blocks need no scaling anyway.
       *
       *  \f[
       *    \boldsymbol{K}_{T,effdyn} = (1 - \frac{1}{\theta (\Delta t)^{2}} \boldsymbol{M}
       *                + (1 - \frac{1}{\Delta t} \boldsymbol{C}
       *                + theta  \boldsymbol{K}_{T}
       *  \f] */
      void add_visco_mass_contributions(Core::LinAlg::SparseOperator& jac) const override;

      /** \brief Access the time integration coefficient \f$\theta\f$
       *
       * If Init() and Setup() have already been called, #theta_ is already set correctly,
       * so we can just return it.
       *
       * However, we sometimes need the value of \f$\theta\f$
       * before this time integration scheme has been properly setup. Then, i.e. if Init()
       * and Setup() haven't been called yet, we read the value of \f$\theta\f$ from
       * a data container.
       *
       * @return Time integration coefficient \f$\theta\f$ for time instance \f$t_{n+1}\f$
       */
      double get_theta() const;

     private:
      //! theta factor: feasible interval (0,1]
      double theta_;

      /*! @name New vectors for internal use only
       *
       *  If an external use seems necessary, move these vectors to the
       *  global state data container and just store a pointer to the global
       *  state variable. */
      //! @{

      //! viscous mid-point force vector F_viscous F_{viscous;n+1}
      Teuchos::RCP<Epetra_Vector> fvisconp_ptr_;

      //! viscous mid-point force vector F_viscous F_{viscous;n}
      Teuchos::RCP<Epetra_Vector> fviscon_ptr_;

      /*! \brief Holds the during a time step constant contributions to
       *  the velocity and acceleration state update.
       *
       *  entry (0): constant velocity contribution \f$\tilde{V}_{n+1}\f$
       *  entry (1): constant acceleration contribution \f$\tilde{A}_{n+1}\f$ */
      Teuchos::RCP<Epetra_MultiVector> const_vel_acc_update_ptr_;
      //! @}

      //! @name pointers to the global state data container content
      //! @{

      //! pointer to inertial force vector F_{inertial,n} at last time
      Teuchos::RCP<Epetra_Vector> finertian_ptr_;

      //! pointer to inertial force vector F_{inertial,n+1} at new time
      Teuchos::RCP<Epetra_Vector> finertianp_ptr_;
      //! @}
    };
  }  // namespace IMPLICIT
}  // namespace STR


FOUR_C_NAMESPACE_CLOSE

#endif
