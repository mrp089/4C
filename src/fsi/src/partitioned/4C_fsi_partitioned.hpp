/*----------------------------------------------------------------------*/
/*! \file

\brief Partitioned FSI base

\level 1

*/
/*----------------------------------------------------------------------*/



#ifndef FOUR_C_FSI_PARTITIONED_HPP
#define FOUR_C_FSI_PARTITIONED_HPP

#include "4C_config.hpp"

#include "4C_coupling_adapter_mortar.hpp"
#include "4C_fsi_algorithm.hpp"
#include "4C_io.hpp"

#include <AztecOO.h>
#include <Epetra_CrsGraph.h>
#include <Epetra_CrsMatrix.h>
#include <Epetra_LinearProblem.h>
#include <Epetra_Map.h>
#include <Epetra_RowMatrix.h>
#include <Epetra_Vector.h>
#include <NOX.H>
#include <NOX_Epetra.H>
#include <NOX_Epetra_Interface_Required.H>
#include <NOX_Epetra_LinearSystem_AztecOO.H>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>

FOUR_C_NAMESPACE_OPEN

// forward declarations
namespace ADAPTER
{
  class CouplingMortar;
}

namespace FSI
{
  namespace UTILS
  {
    class DebugWriter;
  }

  /**
   *
   * \brief Base class for all partitioned FSI algorithms
   *
   * This is the framework for partitioned FSI. The actual work is done by
   * subclasses.
   *
   * This is the algorithm class of partitioned FSI problems. Here we
   * do the time loop and the coupling between fields. The fields
   * themselves are solved using appropriate field algorithms (that are
   * used for standalone solvers as well.) The FSI interface problem is
   * solved using NOX.
   *
   * Many different things come together in this class. First and
   * foremost there is the Timeloop() method that contains the overall
   * FSI time stepping scheme. The time loop is build around the FSI
   * interface problem, that is the problem of finding the correct interface
   * coupling quantities that satisfy the coupled problem. The time loop knows
   *  nothing about the fields themselves.
   *
   * Inside the time loop the interface problem is solved using NOX. To
   * do so Timeloop() needs to know an object which defines the nonlinear
   * residual of the FSI problem. This is always a Teuchos::RCP to the
   * DirichletNeumannCoupling object itself!
   *
   * So the second part of this class consists of the interface residual
   * evaluation computeF(). This method does one FSI cycle, that is one
   * solve of all participating fields. But for sake of clarity
   * this cycle is expressed via the two operator methods FluidOp() and
   * StructOp().
   *
   * This coupling process build on the available field solvers. However,
   * the independent parallel distribution of the fields complicates the
   * exchange of coupling information. Therefore three instances of the
   * Coupling class are used that couple those fields. On top of these
   * there are helper methods StructToAle(), struct_to_fluid(),
   * fluid_to_struct() and AleToFluid() to easily exchange distributed
   * interface vectors between fields.
   *
   * The FSI algorithm requires repeated evaluations of the interface
   * residual via computeF(). So the field solvers themselves must be
   * clean, subsequent calls with the same interface input must yield the
   * same results. The time stepping therefore needs the further methods
   * prepare_time_step() to start a new time step as wellas Update() and
   * Output() to finish the current step, save the result and write the
   * files.
   */
  class Partitioned : public Algorithm, public ::NOX::Epetra::Interface::Required
  {
   public:
    /*! \brief Constructor
     *
     * \param[in] comm Communicator
     */
    explicit Partitioned(const Epetra_Comm& comm);

    /// setup this object
    void Setup() override;

    /*! \brief Outer level FSI time loop
     *
     * @param interface Our interface to NOX
     */
    virtual void Timeloop(const Teuchos::RCP<::NOX::Epetra::Interface::Required>& interface);

    /// compute FSI interface residual S^{-1}(F(d)) - d
    bool computeF(const Epetra_Vector& x, Epetra_Vector& F, const FillType fillFlag) override;

    /// return true if nodes at interface are matching
    bool matchingnodes() { return matchingnodes_; }

    /// open door in the time loop for sliding ale algo to do remeshing
    virtual void Remeshing();

    /// setup of coupling at fsi interface
    virtual void setup_coupling(const Teuchos::ParameterList& fsidyn, const Epetra_Comm& comm);

    /// read restart data
    void read_restart(int step) override;

   protected:
    //! @name Transfer helpers

    Teuchos::RCP<Epetra_Vector> struct_to_fluid(Teuchos::RCP<Epetra_Vector> iv) override;
    Teuchos::RCP<Epetra_Vector> fluid_to_struct(Teuchos::RCP<Epetra_Vector> iv) override;

    //@}

    //! @name Operators implemented by subclasses

    /// composed FSI operator
    virtual void fsi_op(const Epetra_Vector& x, Epetra_Vector& F, const FillType fillFlag);

    /// interface fluid operator
    virtual Teuchos::RCP<Epetra_Vector> fluid_op(
        Teuchos::RCP<Epetra_Vector> idisp, const FillType fillFlag);

    /// interface structural operator
    virtual Teuchos::RCP<Epetra_Vector> struct_op(
        Teuchos::RCP<Epetra_Vector> iforce, const FillType fillFlag);

    //@}

    //! @name Encapsulation of interface unknown
    /// default is displacement, but subclasses might change that

    virtual Teuchos::RCP<Epetra_Vector> initial_guess();

    //@}

    //! @name Access methods for subclasses

    /*! \brief Calculate interface velocity based on given interface displacements
     *
     *  Two options to transform the structural interface displacement into a fluid interface
     * velocity:
     *  - second order (cf. eq. (6.1.2) in [2])               --> set input parameter SECONDORDER =
     * Yes
     *  - fisrt order Backward Euler (cf. eq. (6.1.2) in [2]) --> set input parameter SECONDORDER =
     * No
     *
     *  A derivation of these kinematic coupling conditions is given in chapter 6.2.3 in [1].
     *
     *  References:
     *  - [1] C Foerster, Robust methods for fluid-structure interaction with stabilised finite
     * elemtes, PhD-Thesis, 2007
     *  - [2] U Kuettler, Effiziente Loesungsverfahren fuer Fluid-Struktur-Interaktions-Probleme,
     *    PhD-Thesis, 2009
     */
    Teuchos::RCP<Epetra_Vector> interface_velocity(Teuchos::RCP<const Epetra_Vector> idispnp) const;

    /// current interface displacements
    /*!
      Extract structural displacement at t(n+1)
     */
    Teuchos::RCP<Epetra_Vector> interface_disp();

    /// current interface forces
    /*!
      Extract fluid force at t(n+1)
     */
    Teuchos::RCP<Epetra_Vector> interface_force();

    //@}

    /// create convergence tests
    virtual void create_status_test(Teuchos::ParameterList& nlParams,
        Teuchos::RCP<::NOX::Epetra::Group> grp, Teuchos::RCP<::NOX::StatusTest::Combo> converged);

    Teuchos::RCP<UTILS::DebugWriter> my_debug_writer() const { return debugwriter_; }

    /// return coupsfm_
    CORE::ADAPTER::CouplingMortar& structure_fluid_coupling_mortar();

    /// return coupsfm_
    const CORE::ADAPTER::CouplingMortar& structure_fluid_coupling_mortar() const;

    /// access to iteration counter
    virtual std::vector<int> iteration_counter() { return counter_; };

    /// extract idispn_ iveln_
    virtual void extract_previous_interface_solution();

    /// interface displacement from time step begin
    Teuchos::RCP<Epetra_Vector> idispn_;

    /// interface velocity from time step begin
    Teuchos::RCP<Epetra_Vector> iveln_;

    /// setup list with default parameters
    virtual void set_default_parameters(
        const Teuchos::ParameterList& fsidyn, Teuchos::ParameterList& list);

    /// write output
    void output() override;

   private:
    /// create linear solver framework
    Teuchos::RCP<::NOX::Epetra::LinearSystem> create_linear_system(Teuchos::ParameterList& nlParams,
        const Teuchos::RCP<::NOX::Epetra::Interface::Required>& interface,
        ::NOX::Epetra::Vector& noxSoln, Teuchos::RCP<::NOX::Utils> utils);

    /// create convergence tests including testing framework
    Teuchos::RCP<::NOX::StatusTest::Combo> create_status_test(
        Teuchos::ParameterList& nlParams, Teuchos::RCP<::NOX::Epetra::Group> grp);

    //! connection of interface dofs for finite differences
    Teuchos::RCP<Epetra_CrsGraph> raw_graph_;

    //! counters on how many times the residuum was called in a time step
    /*!
      NOX knows different types of residuum calls depending on
      circumstances (normal, finite difference, matrix free
      jacobi). It is possible to do approximations depending on the
      type.
     */
    std::vector<int> counter_;

    //! number of residuum calculations per nonlinear solve in one time step
    std::vector<int> linsolvcount_;

    /// print parameters and stuff
    /*!
      \warning This variable is only valid during when the time loop runs.
     */
    Teuchos::RCP<::NOX::Utils> utils_;

   protected:
    int mfresitemax_;


    /// coupling of structure and fluid at the interface, with mortar.
    Teuchos::RCP<CORE::ADAPTER::CouplingMortar> coupsfm_;

    /// nodes at the fluid-structure interface match
    bool matchingnodes_;

    /// parameters handed in to NOX
    Teuchos::ParameterList noxparameterlist_;

    /// special debugging output
    Teuchos::RCP<UTILS::DebugWriter> debugwriter_;
  };

}  // namespace FSI

FOUR_C_NAMESPACE_CLOSE

#endif