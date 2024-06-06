/*----------------------------------------------------------------------*/
/*! \file

\brief Methods for spring and dashpot constraints / boundary conditions:

\level 2


*----------------------------------------------------------------------*/

#ifndef FOUR_C_CONSTRAINT_SPRINGDASHPOT_MANAGER_HPP
#define FOUR_C_CONSTRAINT_SPRINGDASHPOT_MANAGER_HPP

#include "4C_config.hpp"

#include <Epetra_Operator.h>
#include <Epetra_RowMatrix.h>
#include <Epetra_Vector.h>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>

FOUR_C_NAMESPACE_OPEN

// forward declarations
namespace Discret
{
  class Discretization;
}

namespace Core::LinAlg
{
  class SparseMatrix;
}  // namespace Core::LinAlg

namespace Core::IO
{
  class DiscretizationWriter;
  class DiscretizationReader;
}  // namespace Core::IO

namespace CONSTRAINTS
{
  class SpringDashpot;

  class SpringDashpotManager
  {
   public:
    /*!
      \brief Constructor
    */
    SpringDashpotManager(Teuchos::RCP<Discret::Discretization> dis);

    /*!
     \brief Return if there are spring dashpots
    */
    bool HaveSpringDashpot() const { return havespringdashpot_; };

    //! add contribution of spring dashpot BC to residual vector and stiffness matrix
    void stiffness_and_internal_forces(Teuchos::RCP<Core::LinAlg::SparseMatrix> stiff,
        Teuchos::RCP<Epetra_Vector> fint, Teuchos::RCP<Epetra_Vector> disn,
        Teuchos::RCP<Epetra_Vector> veln, Teuchos::ParameterList parlist);

    //! update for each new time step
    void Update();

    //! output of gap, normal, and nodal stiffness
    void Output(Teuchos::RCP<Core::IO::DiscretizationWriter> output,
        Teuchos::RCP<Discret::Discretization> discret, Teuchos::RCP<Epetra_Vector> disp);

    //! output of prestressing offset for restart
    void output_restart(Teuchos::RCP<Core::IO::DiscretizationWriter> output,
        Teuchos::RCP<Discret::Discretization> discret, Teuchos::RCP<Epetra_Vector> disp);

    /*!
     \brief Read restart information
    */
    void read_restart(Core::IO::DiscretizationReader& reader, const double& time);

    //! reset spring after having done a MULF prestressing update (mhv 12/2015)
    void ResetPrestress(Teuchos::RCP<Epetra_Vector> disold);

   private:
    Teuchos::RCP<Discret::Discretization> actdisc_;     ///< standard discretization
    std::vector<Teuchos::RCP<SpringDashpot>> springs_;  ///< all spring dashpot instances

    bool havespringdashpot_;  ///< are there any spring dashpot BCs at all?
    int n_conds_;             ///< number of spring dashpot conditions
  };                          // class
}  // namespace CONSTRAINTS
FOUR_C_NAMESPACE_CLOSE

#endif
