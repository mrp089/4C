/*----------------------------------------------------------------------*/
/*! \file

\brief Deprecated structure field adapter
       (see ad_str_structure_new.H/.cpp for the new version)

\level 1

*/
/*----------------------------------------------------------------------*/

#ifndef FOUR_C_ADAPTER_STR_STRUCTURE_HPP
#define FOUR_C_ADAPTER_STR_STRUCTURE_HPP


#include "4C_config.hpp"

#include "4C_adapter_field.hpp"
#include "4C_inpar_structure.hpp"
#include "4C_lib_elements_paramsinterface.hpp"
#include "4C_utils_result_test.hpp"

#include <Epetra_Operator.h>
#include <Teuchos_ParameterList.hpp>

FOUR_C_NAMESPACE_OPEN

// forward declarations
namespace DRT
{
  class Discretization;
  class ResultTest;
  namespace UTILS
  {
    class LocsysManager;
  }  // namespace UTILS
}  // namespace DRT

namespace IO
{
  class DiscretizationWriter;
}

namespace CORE::LINALG
{
  class Solver;
  class SparseMatrix;
  class BlockSparseMatrixBase;
  class MapExtractor;
  class MultiMapExtractor;
}  // namespace CORE::LINALG

namespace CONTACT
{
  class MeshtyingContactBridge;
}

namespace CONSTRAINTS
{
  class ConstrManager;
  class SpringDashpotManager;
}  // namespace CONSTRAINTS

namespace UTILS
{
  class Cardiovascular0DManager;
}  // namespace UTILS

namespace TIMINT
{
  template <typename>
  class TimIntMStep;
}

namespace STR::MODELEVALUATOR
{
  class Generic;
}  // namespace STR::MODELEVALUATOR

namespace ADAPTER
{
  /// general structural field interface
  /*!

  The point is to keep FSI as far apart from our field solvers as
  possible. Each structure field solver we want to use should get its own
  subclass of this. The FSI algorithm should be able to extract all the
  information from the structure field it needs using this interface.

  All FSI algorithms use this adapter to communicate with the structural
  field. There are different ways to use this adapter.

  In all cases you need to tell the structural algorithm about your time
  step. Therefore PrepareTimeStep(), Update() and Output() must be called at
  the appropriate position in the FSI algorithm.

  <h3>Dirichlet-Neumann coupled FSI</h3>

  A good starting displacement can be guessed with PredictInterfaceDispnp().

  Dirichlet-Neumann coupled FSI will need to Solve() the nonlinear
  structural problem for each time step after the fluid forces have been
  applied (ApplyInterfaceForces()). Solve() will be called many times for each
  time step until the interface equilibrium is reached. The structural
  algorithm has to preserve its state until Update() is called.

  After each Solve() you get the interface forces by ExtractInterfaceDispnp().

  A Dirichlet-Neumann FSI with steepest descent relaxation or matrix free
  Newton Krylov will want to solve the structural problem linearly without
  history and prescribed interface forces: RelaxationSolve().

  <h3>Monolithic FSI</h3>

  Monolithic FSI is based on Evaluate() of elements. This results in a new
  RHS() and a new SysMat(). Together with the InitialGuess() these form the
  building blocks for a block based Newton's method.

  \warning Further cleanup is still needed.

  \sa Fluid, Ale
  \author u.kue
  \date 11/07
  */
  class Structure : public Field
  {
   public:
    //! @name Construction
    //@{

    /*! \brief Setup all class internal objects and members

     Setup() is not supposed to have any input arguments !

     Must only be called after Init().

     Construct all objects depending on the parallel distribution and
     relying on valid maps like, e.g. the state vectors, system matrices, etc.

     Call all Setup() routines on previously initialized internal objects and members.

    \note Must only be called after parallel (re-)distribution of discretizations is finished !
          Otherwise, e.g. vectors may have wrong maps.

    \warning none
    \return void
    \date 08/16
    \author rauch  */
    virtual void Setup() = 0;

    //@}

    //! @name Vector access
    //@{

    /// initial guess of Newton's method
    virtual Teuchos::RCP<const Epetra_Vector> InitialGuess() = 0;

    /// rhs of Newton's method
    Teuchos::RCP<const Epetra_Vector> RHS() override = 0;

    /// unknown displacements at \f$t_{n+1}\f$
    [[nodiscard]] virtual Teuchos::RCP<const Epetra_Vector> Dispnp() const = 0;

    /// known displacements at \f$t_{n}\f$
    [[nodiscard]] virtual Teuchos::RCP<const Epetra_Vector> Dispn() const = 0;

    /// unknown velocity at \f$t_{n+1}\f$
    [[nodiscard]] virtual Teuchos::RCP<const Epetra_Vector> Velnp() const = 0;

    /// known velocity at \f$t_{n}\f$
    [[nodiscard]] virtual Teuchos::RCP<const Epetra_Vector> Veln() const = 0;

    /// known velocity at \f$t_{n-1}\f$
    [[nodiscard]] virtual Teuchos::RCP<const Epetra_Vector> Velnm() const = 0;

    /// unknown acceleration at \f$t_{n+1}\f$
    [[nodiscard]] virtual Teuchos::RCP<const Epetra_Vector> Accnp() const = 0;

    /// known acceleration at \f$t_{n}\f$
    [[nodiscard]] virtual Teuchos::RCP<const Epetra_Vector> Accn() const = 0;

    virtual void ResizeMStepTimAda() = 0;

    //@}

    //! @name Misc

    /// dof map of vector of unknowns
    Teuchos::RCP<const Epetra_Map> DofRowMap() override = 0;

    /// DOF map of vector of unknowns for multiple dofsets
    virtual Teuchos::RCP<const Epetra_Map> DofRowMap(unsigned nds) = 0;

    /// DOF map view of vector of unknowns
    virtual const Epetra_Map* DofRowMapView() = 0;

    /// domain map of system matrix (do we really need this?)
    [[nodiscard]] virtual const Epetra_Map& DomainMap() const = 0;

    /// direct access to system matrix
    Teuchos::RCP<CORE::LINALG::SparseMatrix> SystemMatrix() override = 0;

    /// direct access to system matrix
    Teuchos::RCP<CORE::LINALG::BlockSparseMatrixBase> BlockSystemMatrix() override = 0;

    /// switch structure field to block matrix
    virtual void UseBlockMatrix(Teuchos::RCP<const CORE::LINALG::MultiMapExtractor> domainmaps,
        Teuchos::RCP<const CORE::LINALG::MultiMapExtractor> rangemaps) = 0;

    /// return contact/meshtying bridge
    virtual Teuchos::RCP<CONTACT::MeshtyingContactBridge> MeshtyingContactBridge() = 0;

    /// do we have this model
    virtual bool HaveModel(INPAR::STR::ModelType model)
    {
      FOUR_C_THROW("new time integration only");
      return false;
    }

    /// return model evaluator
    virtual STR::MODELEVALUATOR::Generic& ModelEvaluator(INPAR::STR::ModelType mtype) = 0;

    // access to locsys manager
    virtual Teuchos::RCP<DRT::UTILS::LocsysManager> LocsysManager() = 0;

    /// direct access to discretization
    virtual Teuchos::RCP<DRT::Discretization> Discretization() = 0;

    /// are there any algebraic constraints?
    virtual bool HaveConstraint() = 0;

    /// are there any spring dashpot bcs?
    virtual bool HaveSpringDashpot() = 0;

    /// get constraint manager defined in the structure
    virtual Teuchos::RCP<CONSTRAINTS::ConstrManager> GetConstraintManager() = 0;

    /// get SpringDashpot manager defined in the structure
    virtual Teuchos::RCP<CONSTRAINTS::SpringDashpotManager> GetSpringDashpotManager() = 0;

    /// Get type of thickness scaling for thin shell structures
    virtual INPAR::STR::StcScale GetSTCAlgo() = 0;

    /// Access to scaling matrix for STC
    virtual Teuchos::RCP<CORE::LINALG::SparseMatrix> GetSTCMat() = 0;

    /// Return MapExtractor for Dirichlet boundary conditions
    virtual Teuchos::RCP<const CORE::LINALG::MapExtractor> GetDBCMapExtractor() = 0;

    /// expand dirichlet bc map
    virtual void AddDirichDofs(const Teuchos::RCP<const Epetra_Map> maptoadd){
        /* This is only needed for the old structural time integration.
           For the new structural time integration this is already
           implemented in str_dbc.cpp and str_dbc.H ! rauch 02/17 */
    };

    /// contract dirichlet bc map
    virtual void RemoveDirichDofs(const Teuchos::RCP<const Epetra_Map> maptoremove){
        /* This is only needed for the old structural time integration.
           For the new structural time integration this is already
           implemented in str_dbc.cpp and str_dbc.H ! rauch 02/17 */
    };

    /// set evaluation action
    virtual void SetActionType(const DRT::ELEMENTS::ActionType& action) = 0;

    //@}

    //! @name Time step helpers
    //@{

    /// return time integration factor
    [[nodiscard]] virtual double TimIntParam() const = 0;

    //! Return current time \f$t_{n}\f$
    [[nodiscard]] virtual double TimeOld() const = 0;

    //! Return target time \f$t_{n+1}\f$
    [[nodiscard]] virtual double Time() const = 0;

    /// Get upper limit of time range of interest
    [[nodiscard]] virtual double GetTimeEnd() const = 0;

    //! Set upper limit of time range of interest
    virtual void SetTimeEnd(double timemax) = 0;

    /// Get time step size \f$\Delta t_n\f$
    [[nodiscard]] virtual double Dt() const = 0;

    /// Return current step number $n$
    [[nodiscard]] virtual int StepOld() const = 0;

    /// Return current step number $n+1$
    [[nodiscard]] virtual int Step() const = 0;

    /// Get number of time steps
    [[nodiscard]] virtual int NumStep() const = 0;

    /// Take the time and integrate (time loop)
    /// \date 11/08
    virtual int Integrate() = 0;

    //! do something in case nonlinear solution does not converge for some reason
    virtual INPAR::STR::ConvergenceStatus PerformErrorAction(
        INPAR::STR::ConvergenceStatus nonlinsoldiv) = 0;

    /// tests if there are more time steps to do
    [[nodiscard]] virtual bool NotFinished() const = 0;

    /// start new time step
    void PrepareTimeStep() override = 0;

    /// set time step size
    virtual void SetDt(const double dtnew) = 0;

    //! Sets the current time \f$t_{n}\f$
    virtual void SetTime(const double time) = 0;

    //! Sets the current step \f$n\f$
    virtual void SetStep(int step) = 0;

    //! Sets the current step \f$n+1\f$
    virtual void SetStepn(int step) = 0;

    //! Sets the target time \f$t_{n+1}\f$ of this time step
    virtual void SetTimen(const double time) = 0;

    /*!
    \brief update displacement and evaluate elements

    There are two displacement increments possible

    \f$x^n+1_i+1 = x^n+1_i + disiterinc\f$  (sometimes referred to as residual increment), and

    \f$x^n+1_i+1 = x^n     + disstepinc\f$

    with \f$n\f$ and \f$i\f$ being time and Newton iteration step

    Note: The structure expects an iteration increment.
    In case the StructureNOXCorrectionWrapper is applied, the step increment is expected
    which is then transformed into an iteration increment
    */
    void Evaluate(Teuchos::RCP<const Epetra_Vector>
            disiterinc  ///< displacement increment between Newton iteration i and i+1
        ) override = 0;

    /// don't update displacement but evaluate elements (implicit only)
    virtual void Evaluate() = 0;

    //! Calculate stresses and strains
    virtual void DetermineStressStrain() = 0;

    /// update at time step end
    void Update() override = 0;

    /// update at time step end in case of FSI time adaptivity
    virtual void Update(double endtime) = 0;

    /// Update iteration
    /// Add residual increment to Lagrange multipliers stored in Constraint manager
    virtual void UpdateIterIncrConstr(Teuchos::RCP<Epetra_Vector> lagrincr) = 0;

    /// Update iteration
    /// Add residual increment to pressures stored in Cardiovascular0D manager
    virtual void UpdateIterIncrCardiovascular0D(Teuchos::RCP<Epetra_Vector> presincr) = 0;

    /// Access to output object
    virtual Teuchos::RCP<IO::DiscretizationWriter> DiscWriter() = 0;

    /// prepare output (i.e. calculate stresses, strains, energies)
    void PrepareOutput(bool force_prepare_timestep) override = 0;

    // Get restart data
    virtual void GetRestartData(Teuchos::RCP<int> step, Teuchos::RCP<double> time,
        Teuchos::RCP<Epetra_Vector> disn, Teuchos::RCP<Epetra_Vector> veln,
        Teuchos::RCP<Epetra_Vector> accn, Teuchos::RCP<std::vector<char>> elementdata,
        Teuchos::RCP<std::vector<char>> nodedata) = 0;

    /// output results
    void Output(bool forced_writerestart = false) override = 0;

    /// output results to screen
    virtual void PrintStep() = 0;

    /// read restart information for given time step
    void ReadRestart(const int step) override = 0;

    /*!
    \brief Reset time step

    In case of time step size adaptivity, time steps might have to be repeated.
    Therefore, we need to reset the solution back to the initial solution of the
    time step.

    \author mayr.mt
    \date 08/2013
    */
    virtual void ResetStep() = 0;

    /// set restart information for parameter continuation
    virtual void SetRestart(int step, double time, Teuchos::RCP<Epetra_Vector> disn,
        Teuchos::RCP<Epetra_Vector> veln, Teuchos::RCP<Epetra_Vector> accn,
        Teuchos::RCP<std::vector<char>> elementdata, Teuchos::RCP<std::vector<char>> nodedata) = 0;

    /// set the state of the nox group and the global state data container (implicit only)
    virtual void SetState(const Teuchos::RCP<Epetra_Vector>& x) = 0;

    /// wrapper for things that should be done before PrepareTimeStep is called
    virtual void PrePredict() = 0;

    /// wrapper for things that should be done before solving the nonlinear iterations
    virtual void PreSolve() = 0;

    /// wrapper for things that should be done before updating
    virtual void PreUpdate() = 0;

    /// wrapper for things that should be done after solving the update
    virtual void PostUpdate() = 0;

    /// wrapper for things that should be done after the output
    virtual void PostOutput() = 0;

    /// wrapper for things that should be done after the actual time loop is finished
    virtual void PostTimeLoop() = 0;

    //@}

    //! @name Solver calls

    /*!
    \brief nonlinear solve

    Do the nonlinear solve, i.e. (multiple) corrector,
    for the time step. All boundary conditions have
    been set.
    */
    virtual INPAR::STR::ConvergenceStatus Solve() = 0;

    /*!
    \brief linear structure solve with just a interface load

    The very special solve done in steepest descent relaxation
    calculation (and matrix free Newton Krylov).

    \note Can only be called after a valid structural solve.
    */
    virtual Teuchos::RCP<Epetra_Vector> SolveRelaxationLinear() = 0;

    /// get the linear solver object used for this field
    virtual Teuchos::RCP<CORE::LINALG::Solver> LinearSolver() = 0;

    //@}

    //! @name Write access to field solution variables at \f$t^{n+1}\f$
    //@{

    /// write access to extract displacements at \f$t^{n+1}\f$
    virtual Teuchos::RCP<Epetra_Vector> WriteAccessDispnp() = 0;

    /// write access to extract velocities at \f$t^{n+1}\f$
    virtual Teuchos::RCP<Epetra_Vector> WriteAccessVelnp() = 0;

    /// write access to extract displacements at \f$t^{n}\f$
    virtual Teuchos::RCP<Epetra_Vector> WriteAccessDispn() = 0;

    /// write access to extract velocities at \f$t^{n}\f$
    virtual Teuchos::RCP<Epetra_Vector> WriteAccessVeln() = 0;

    //@}

    /// extract rhs (used to calculate reaction force for post-processing)
    virtual Teuchos::RCP<Epetra_Vector> Freact() = 0;


    //! @name volume coupled specific methods
    //@{

    /// Set forces due to interface with fluid, the force is expected external-force-like
    ///
    /// \note This method will be deprecated as soon as new structural time integration is
    ///       completely engulfed by all algorithms using this method.
    virtual void SetForceInterface(Teuchos::RCP<Epetra_MultiVector> iforce) = 0;

    //! specific method for iterative staggered partitioned TSI

    /// Identify residual
    /// This method does not predict the target solution but
    /// evaluates the residual and the stiffness matrix.
    /// In partitioned solution schemes, it is better to keep the current
    /// solution instead of evaluating the initial guess (as the predictor)
    /// does.
    //! will be obsolete after switch to new structural timint.
    virtual void PreparePartitionStep() = 0;

    //@}

    //! @name Structure with ale specific methods
    //@{

    /// material displacements (structure with ale)
    virtual Teuchos::RCP<Epetra_Vector> DispMat() = 0;

    /// apply material displacements to structure field (structure with ale)
    virtual void ApplyDisMat(Teuchos::RCP<Epetra_Vector> dismat) = 0;

    //@}

    /// create result test for encapsulated structure algorithm
    virtual Teuchos::RCP<CORE::UTILS::ResultTest> CreateFieldTest() = 0;

    /// reset time and state vectors (needed for biofilm growth simulations)
    virtual void Reset() = 0;

    /// set structure displacement vector due to biofilm growth
    virtual void SetStrGrDisp(Teuchos::RCP<Epetra_Vector> struct_growth_disp) = 0;

    /// Write Gmsh output for structural field
    virtual void WriteGmshStrucOutputStep() = 0;

    /// bool indicating if micro material is used
    virtual bool HaveMicroMat() = 0;

    /// \brief Returns true if the final state has been written
    [[nodiscard]] virtual bool HasFinalStateBeenWritten() const = 0;
  };


  /// structure field solver
  class StructureBaseAlgorithm
  {
   public:
    /// constructor
    StructureBaseAlgorithm(const Teuchos::ParameterList& prbdyn, const Teuchos::ParameterList& sdyn,
        Teuchos::RCP<DRT::Discretization> actdis);

    /// virtual destructor to support polymorph destruction
    virtual ~StructureBaseAlgorithm() = default;

    /// structural field solver
    Teuchos::RCP<Structure> StructureField() { return structure_; }

    /// structural field solver
    // Teuchos::RCP<STR::TimInt> StructureTimeIntegrator() { return str_tim_int_; }

   private:
    /// Create structure algorithm
    void CreateStructure(const Teuchos::ParameterList& prbdyn, const Teuchos::ParameterList& sdyn,
        Teuchos::RCP<DRT::Discretization> actdis);

    /// setup structure algorithm of STR::TimIntImpl type
    void CreateTimInt(const Teuchos::ParameterList& prbdyn, const Teuchos::ParameterList& sdyn,
        Teuchos::RCP<DRT::Discretization> actdis);

    /*! \brief Create linear solver for contact/meshtying problems
     *
     * Per default the CONTACT SOLVER block from the input file is used for generating the solver
     * object. The idea is, that this linear solver object is used whenever there is contact between
     * (two) structures. Otherwise the standard structural solver block is used (generated by
     * <tt>CreateLinearSolver</tt>. So we can use highly optimized solvers for symmetric pure
     * structural problems, but choose a different solver for the hard nonsymmetric contact case. We
     * automatically switch from the contact solver (in case of contact) to the structure solver
     * (pure structural problem, no contact) and back again.
     *
     * \note For contact/meshtying problems in the saddlepoint formulation (not condensed), this
     * routines requires a block preconditioner (eg <tt>CheapSIMPLE</tt>) as preconditioner for the
     * contact solver. The structure solver block specified in <tt>STRUCTURAL
     * DYNAMICS->LINEAR_SOLVER</tt>is used for the primary (structural) variables and the contact
     * solver block specified in <tt>CONTACT DYNAMIC->LINEAR_SOLVER</tt> is used for the
     * saddle-point system.
     *
     * \note Condensed meshtying problems use the standard structural solver block
     * (generated by CreateLinearSolver()). We assume that in contrary to contact problems,
     * the domain configuration is not changing for meshtying over the time.
     *
     * \param actdis Discretization with all structural elements
     * \param[in] sdyn Structural parameters from input file
     *
     * \return Contact solver object
     *
     * \sa CreateLinearSolver()
     */
    Teuchos::RCP<CORE::LINALG::Solver> CreateContactMeshtyingSolver(
        Teuchos::RCP<DRT::Discretization>& actdis, const Teuchos::ParameterList& sdyn);

    /*! \brief Create linear solver for pure structure problems
     *
     * The solver block in the input file is specified by the parameter <tt>LINEAR_SOLVER</tt> in
     * the <tt>STRUCTURAL DYNAMICS</tt> block of the 4C input file. This solver is used for pure
     * structural problems, whenever there is no contact.
     *
     * To create the solver, we use the ID of the solver block to access the solver parameter list.
     * This is then used to create a CORE::LINALG::Solver.
     *
     * We also compute the nullspace information if this is required by the chosen solver.
     *
     * \param actdis Discretization with all structural elements
     * \param[in] sdyn Structural parameters from input file
     *
     * \return Linear solver object for pure structural problems
     *
     * \sa CreateContactMeshtyingSolver()
     */
    Teuchos::RCP<CORE::LINALG::Solver> CreateLinearSolver(
        Teuchos::RCP<DRT::Discretization>& actdis, const Teuchos::ParameterList& sdyn);

    /// structural field solver
    Teuchos::RCP<Structure> structure_;
  };

}  // namespace ADAPTER

FOUR_C_NAMESPACE_CLOSE

#endif
