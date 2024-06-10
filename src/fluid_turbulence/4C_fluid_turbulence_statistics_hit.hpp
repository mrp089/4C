/*----------------------------------------------------------------------*/
/*! \file

\brief routines for homogeneous isotropic turbulence


\level 2

*/
/*----------------------------------------------------------------------*/

#ifndef FOUR_C_FLUID_TURBULENCE_STATISTICS_HIT_HPP
#define FOUR_C_FLUID_TURBULENCE_STATISTICS_HIT_HPP

#include "4C_config.hpp"

#include "4C_linalg_utils_sparse_algebra_create.hpp"

#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>

FOUR_C_NAMESPACE_OPEN

namespace Core::FE
{
  class Discretization;
}  // namespace Core::FE

namespace FLD
{
  class TurbulenceStatisticsHit
  {
   public:
    //! constructor: set-up sampling
    TurbulenceStatisticsHit(Teuchos::RCP<Core::FE::Discretization> actdis,
        Teuchos::ParameterList& params, const std::string& statistics_outfilename,
        const bool forced);

    //! destructor
    virtual ~TurbulenceStatisticsHit() = default;

    //! store scatra discretization if passive scalar is included
    virtual void StoreScatraDiscret(Teuchos::RCP<Core::FE::Discretization> scatradis)
    {
      scatradiscret_ = scatradis;
      return;
    };

    //! space and time (only forced but not decaying case) averaging
    //! get energy spectrum
    virtual void DoTimeSample(Teuchos::RCP<Epetra_Vector> velnp);
    //! version with scalar field
    virtual void DoScatraTimeSample(
        Teuchos::RCP<Epetra_Vector> velnp, Teuchos::RCP<Epetra_Vector> phinp);

    // evaluation of dissipation rate and rbvmm-related quantities
    virtual void EvaluateResiduals(std::map<std::string, Teuchos::RCP<Epetra_Vector>> statevecs);

    //! dump the result to file
    virtual void DumpStatistics(int step, bool multiple_records = false);
    //! version with scalar field
    virtual void dump_scatra_statistics(int step, bool multiple_records = false);

    //! reset sums and number of samples to zero
    virtual void ClearStatistics();
    //! version with scalar field
    virtual void clear_scatra_statistics();


   protected:
    //! sort criterium for double values up to a tolerance of 10-9
    class LineSortCriterion
    {
     public:
      bool operator()(const double& p1, const double& p2) const { return (p1 < p2 - 1E-9); }

     protected:
     private:
    };

    //! calculate the resolved energy for the given discretization
    //! and write to statistics file
    virtual void calculate_resolved_energy_decaying_turbulence();

    //! numerical integration via trapezoidal rule
    static double integrate_trapezoidal_rule(
        const double& x_1, const double& x_2, const double& y_1, const double& y_2)
    {
      const double value = 0.5 * (x_2 - x_1) * (y_2 + y_1);
      return value;
    }

    //! interpolation function
    static double interpolate(
        const double& x, const double& x_1, const double& x_2, const double& y_1, const double& y_2)
    {
      const double value = y_1 + (y_2 - y_1) / (x_2 - x_1) * (x - x_1);
      return value;
    }

    //! the discretisation (required for nodes, dofs etc;)
    Teuchos::RCP<Core::FE::Discretization> discret_;

    //! the scatra discretisation (required for nodes, dofs etc;)
    Teuchos::RCP<Core::FE::Discretization> scatradiscret_;

    //! parameter list
    Teuchos::ParameterList& params_;

    //! name of statistics output file, despite the ending
    const std::string statistics_outfilename_;

    //! specifies type
    enum SpecialFlow
    {
      decaying_homogeneous_isotropic_turbulence,
      forced_homogeneous_isotropic_turbulence
    } type_;

    //! number of resolved mode
    int nummodes_;

    //! vector of coordinates in one spatial direction (same for the other two directions)
    Teuchos::RCP<std::vector<double>> coordinates_;

    //! vector of wave numbers
    Teuchos::RCP<std::vector<double>> wavenumbers_;

    //! vector energy (sum over k=const)
    Teuchos::RCP<std::vector<double>> energyspectrum_;

    //! vector dissipation (sum over k=const)
    Teuchos::RCP<std::vector<double>> dissipationspectrum_;

    //! vector scalar variance (sum over k=const)
    Teuchos::RCP<std::vector<double>> scalarvariancespectrum_;

    //! sum over velocity vector
    Teuchos::RCP<std::vector<double>> sumvel_;

    //! sum over squares of velocity vector componetnts
    Teuchos::RCP<std::vector<double>> sumvelvel_;

    //! number of samples taken
    int numsamp_;

    //! time step size
    double dt_;

    //! kinematic viscosity
    double visc_;

    //! output steps for energy spectrum of decaying case
    Teuchos::RCP<std::vector<int>> outsteps_;

    //! toogle vectors: sums are computed by scalarproducts
    Teuchos::RCP<Epetra_Vector> toggleu_;
    Teuchos::RCP<Epetra_Vector> togglev_;
    Teuchos::RCP<Epetra_Vector> togglew_;
  };

  class TurbulenceStatisticsHitHDG : public TurbulenceStatisticsHit
  {
   public:
    //! constructor: set-up sampling
    TurbulenceStatisticsHitHDG(Teuchos::RCP<Core::FE::Discretization> actdis,
        Teuchos::ParameterList& params, const std::string& statistics_outfilename,
        const bool forced);


    //! store scatra discretization if passive scalar is included
    void StoreScatraDiscret(Teuchos::RCP<Core::FE::Discretization> scatradis) override
    {
      FOUR_C_THROW("not implemented for hdg");
      return;
    };

    //! space and time (only forced but not decaying case) averaging
    //! get energy spectrum
    void DoTimeSample(Teuchos::RCP<Epetra_Vector> velnp) override;
    //! version with scalar field
    void DoScatraTimeSample(
        Teuchos::RCP<Epetra_Vector> velnp, Teuchos::RCP<Epetra_Vector> phinp) override
    {
      FOUR_C_THROW("not implemented for hdg");
      return;
    }

    // evaluation of dissipation rate and rbvmm-related quantities
    void EvaluateResiduals(std::map<std::string, Teuchos::RCP<Epetra_Vector>> statevecs) override
    {
      FOUR_C_THROW("not implemented for hdg");
      return;
    };

    //! version with scalar field
    void dump_scatra_statistics(int step, bool multiple_records = false) override
    {
      FOUR_C_THROW("not implemented for hdg");
      return;
    };

    //! version with scalar field
    void clear_scatra_statistics() override
    {
      FOUR_C_THROW("not implemented for hdg");
      return;
    };


   protected:
    //! calculate the resolved energy for the given discretization
    //! and write to statistics file
    void calculate_resolved_energy_decaying_turbulence() override
    {
      FOUR_C_THROW("not implemented for hdg");
      return;
    }
  };

}  // namespace FLD

FOUR_C_NAMESPACE_CLOSE

#endif
