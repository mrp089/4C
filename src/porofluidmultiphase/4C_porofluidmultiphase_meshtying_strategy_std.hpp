/*----------------------------------------------------------------------*/
/*! \file
\brief standard case without mesh tying

\level 3

*----------------------------------------------------------------------*/

#ifndef FOUR_C_POROFLUIDMULTIPHASE_MESHTYING_STRATEGY_STD_HPP
#define FOUR_C_POROFLUIDMULTIPHASE_MESHTYING_STRATEGY_STD_HPP

#include "4C_config.hpp"

#include "4C_porofluidmultiphase_meshtying_strategy_base.hpp"

FOUR_C_NAMESPACE_OPEN

namespace POROFLUIDMULTIPHASE
{
  class MeshtyingStrategyStd : public MeshtyingStrategyBase
  {
   public:
    //! constructor
    explicit MeshtyingStrategyStd(POROFLUIDMULTIPHASE::TimIntImpl* porofluidmultitimint,
        const Teuchos::ParameterList& probparams, const Teuchos::ParameterList& poroparams);


    //! prepare time loop
    void prepare_time_loop() override;

    //! prepare time step
    void prepare_time_step() override;

    //! update
    void update() override;

    //! output
    void output() override;

    //! Initialize the linear solver
    void initialize_linear_solver(Teuchos::RCP<Core::LinAlg::Solver> solver) override;

    //! solve linear system of equations
    void linear_solve(Teuchos::RCP<Core::LinAlg::Solver> solver,
        Teuchos::RCP<Core::LinAlg::SparseOperator> sysmat,
        Teuchos::RCP<Core::LinAlg::Vector<double>> increment,
        Teuchos::RCP<Core::LinAlg::Vector<double>> residual,
        Core::LinAlg::SolverParams& solver_params) override;

    //! calculate norms for convergence checks
    void calculate_norms(std::vector<double>& preresnorm, std::vector<double>& incprenorm,
        std::vector<double>& prenorm,
        const Teuchos::RCP<const Core::LinAlg::Vector<double>> increment) override;

    //! create the field test
    void create_field_test() override;

    //! restart
    void read_restart(const int step) override;

    //! evaluate mesh tying
    void evaluate() override;

    //! extract increments and update mesh tying
    Teuchos::RCP<const Core::LinAlg::Vector<double>> extract_and_update_iter(
        const Teuchos::RCP<const Core::LinAlg::Vector<double>> inc) override;

    //! access to global (combined) increment of coupled problem
    Teuchos::RCP<const Core::LinAlg::Vector<double>> combined_increment(
        const Teuchos::RCP<const Core::LinAlg::Vector<double>> inc) const override;

    //! check if initial fields on coupled DOFs are equal
    void check_initial_fields(
        Teuchos::RCP<const Core::LinAlg::Vector<double>> vec_cont) const override;

    //! set the element pairs that are close as found by search algorithm
    void set_nearby_ele_pairs(const std::map<int, std::set<int>>* nearbyelepairs) override;

    //! setup the strategy
    void setup() override;

    //! apply the mesh movement
    void apply_mesh_movement() const override;
  };

}  // namespace POROFLUIDMULTIPHASE



FOUR_C_NAMESPACE_CLOSE

#endif
