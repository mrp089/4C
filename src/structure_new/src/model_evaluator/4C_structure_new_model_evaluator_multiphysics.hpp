/*-----------------------------------------------------------*/
/*! \file

\brief Base class for modelevaluators in partitioned algorithms.


\level 3
*/
/*-----------------------------------------------------------*/


#ifndef FOUR_C_STRUCTURE_NEW_MODEL_EVALUATOR_MULTIPHYSICS_HPP
#define FOUR_C_STRUCTURE_NEW_MODEL_EVALUATOR_MULTIPHYSICS_HPP


#include "4C_config.hpp"

#include "4C_structure_new_model_evaluator_generic.hpp"

// forward declaration
class Epetra_Map;
#include "4C_utils_parameter_list.fwd.hpp"

FOUR_C_NAMESPACE_OPEN

namespace Core::LinAlg
{
  class SparseOperator;
}  // namespace Core::LinAlg
namespace Core::IO
{
  class DiscretizationWriter;
  class DiscretizationReader;
}  // namespace Core::IO
namespace Core::FE
{
  class Discretization;
}  // namespace Core::FE
namespace Solid
{
  class Integrator;

  namespace TimeInt
  {
    class BaseDataGlobalState;
    class BaseDataIO;
    class Base;
  }  // namespace TimeInt

  namespace ModelEvaluator
  {
    class Data;

    //! supported multiphysic problems
    enum MultiphysicType
    {
      mt_none = 0,  //!< none specific default value
      mt_fsi = 1,   //!< multiphysics type fluid-structure-interaction
      mt_ssi = 2    //!< multiphysics type structure-scalar-interaction
    };              // MultiphysicType


    /*! \brief This is the base class for all multiphysics models.
     *
     *  This class summarizes the functionality which all model multiphysics model
     *  evaluators share.
     *
     *  \date 11/16
     *  \author rauch */
    class Multiphysics : public Generic
    {
     public:
      //! constructor
      Multiphysics();


      //! initialize the class variables
      void init(const Teuchos::RCP<Solid::ModelEvaluator::Data>& eval_data_ptr,
          const Teuchos::RCP<Solid::TimeInt::BaseDataGlobalState>& gstate_ptr,
          const Teuchos::RCP<Solid::TimeInt::BaseDataIO>& gio_ptr,
          const Teuchos::RCP<Solid::Integrator>& int_ptr,
          const Teuchos::RCP<const Solid::TimeInt::Base>& timint_ptr,
          const int& dof_offset) override;

      //! setup class variables
      void setup() override;

      //! set the active model type wrapped in this class.
      //! only active model type is evaluated.
      //! e.g. mt_fsi in case fluid-structure interaction is to be evaluated
      void set_active_model_type(enum Solid::ModelEvaluator::MultiphysicType mtype)
      {
        active_mt_ = mtype;
      };

      void check_active_model_type() const
      {
        if (active_mt_ == mt_none) FOUR_C_THROW("No active model evaluator set for Multiphysics");
      };

      //! @name Functions which are derived from the base generic class
      //! @{
      //! [derived]
      Inpar::Solid::ModelType type() const override
      {
        return Inpar::Solid::model_partitioned_coupling;
      }

      //! reset class variables (without jacobian) [derived]
      void reset(const Core::LinAlg::Vector<double>& x) override;

      //! [derived]
      bool evaluate_force() override;

      //! [derived]
      bool evaluate_stiff() override;

      //! [derived] not needed in partitioned scheme
      bool evaluate_force_stiff() override;

      //! derived
      void pre_evaluate() override{};

      //! derived
      void post_evaluate() override{};

      //! derived
      bool assemble_force(Core::LinAlg::Vector<double>& f, const double& timefac_np) const override;

      //! Assemble the jacobian at \f$t_{n+1}\f$ not needed in partitioned scheme
      bool assemble_jacobian(
          Core::LinAlg::SparseOperator& jac, const double& timefac_np) const override;

      //! [derived]
      void write_restart(Core::IO::DiscretizationWriter& iowriter,
          const bool& forced_writerestart) const override{};

      //! [derived]
      void read_restart(Core::IO::DiscretizationReader& ioreader) override{};

      //! [derived]
      void predict(const Inpar::Solid::PredEnum& pred_type) override{};

      //! derived
      void run_pre_compute_x(const Core::LinAlg::Vector<double>& xold,
          Core::LinAlg::Vector<double>& dir_mutable, const NOX::Nln::Group& curr_grp) override{};

      //! recover condensed Lagrange multipliers
      void run_post_compute_x(const Core::LinAlg::Vector<double>& xold,
          const Core::LinAlg::Vector<double>& dir,
          const Core::LinAlg::Vector<double>& xnew) override{};

      //! derived
      void run_post_iterate(const ::NOX::Solver::Generic& solver) override{};

      //! [derived]
      void update_step_state(const double& timefac_n) override;

      //! [derived]
      void update_step_element() override{};

      //! [derived]
      void determine_stress_strain() override{};

      //! [derived]
      void determine_energy() override{};

      //! [derived]
      void determine_optional_quantity() override{};

      //! [derived]
      void output_step_state(Core::IO::DiscretizationWriter& iowriter) const override{};

      //! derived
      void reset_step_state() override{};

      //! [derived]
      void post_output() override{};

      //! @name Accessors to model specific things
      //! @{

      //! Returns a pointer to the model specific dof row map
      Teuchos::RCP<const Epetra_Map> get_block_dof_row_map_ptr() const override
      {
        return Teuchos::null;
      };

      //! Returns a pointer to the current model solution vector (usually the Lagrange multiplier
      //! vector)
      Teuchos::RCP<const Core::LinAlg::Vector<double>> get_current_solution_ptr() const override
      {
        return Teuchos::null;
      };

      //! Returns a pointer to the model solution vector of the last time step (usually the Lagrange
      //! multiplier vector)
      Teuchos::RCP<const Core::LinAlg::Vector<double>> get_last_time_step_solution_ptr()
          const override
      {
        return Teuchos::null;
      };

      //! @}

     protected:
      //! map containing the model evaluators of the sub modules
      std::map<enum Solid::ModelEvaluator::MultiphysicType,
          Teuchos::RCP<Solid::ModelEvaluator::Generic>>
          me_map_;

      //! currently active model evaluator type
      Solid::ModelEvaluator::MultiphysicType active_mt_;

      //! return reference to map containing the model evaluators
      std::map<enum Solid::ModelEvaluator::MultiphysicType,
          Teuchos::RCP<Solid::ModelEvaluator::Generic>>&
      get_model_evalutaor_map()
      {
        return me_map_;
      };

     public:
      //! return RCP to model evaluator of specific MultiphysicType
      Teuchos::RCP<Solid::ModelEvaluator::Generic> get_model_evaluator_from_map(
          enum Solid::ModelEvaluator::MultiphysicType mtype) const
      {
        return me_map_.at(mtype);
      }


    };  // class Multiphysics

  }  // namespace ModelEvaluator
}  // namespace Solid

FOUR_C_NAMESPACE_CLOSE

#endif
