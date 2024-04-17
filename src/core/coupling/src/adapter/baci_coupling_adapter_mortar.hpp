/*----------------------------------------------------------------------*/
/*! \file

\brief A class providing coupling capabilities based on mortar methods

\level 2

*/
/*----------------------------------------------------------------------*/

#ifndef FOUR_C_COUPLING_ADAPTER_MORTAR_HPP
#define FOUR_C_COUPLING_ADAPTER_MORTAR_HPP

#include "baci_config.hpp"

#include "baci_coupling_adapter_base.hpp"
#include "baci_discretization_fem_general_shape_function_type.hpp"
#include "baci_utils_exceptions.hpp"

#include <Epetra_CrsMatrix.h>
#include <Epetra_Map.h>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>

FOUR_C_NAMESPACE_OPEN

// forward declarations
namespace CORE::LINALG
{
  class SparseMatrix;
}

namespace DRT
{
  class Discretization;
  class Element;
  class Node;
}  // namespace DRT

namespace MORTAR
{
  class Interface;
  class IntCell;
}  // namespace MORTAR

/// Couple non-matching interface meshes using mortar method
namespace CORE::ADAPTER
{ /*!
This is a generic class used to couple any non-matching meshes
(or more general: discretizations) at interfaces. The current
applications in BACI encompass FSI coupling algorithms (i.e. to
interpolate between fluid and structure fields at the interface)
and fluid mesh tying algorithms (i.e. to couple non-matching
Eulerian fluid meshes). All the hard work is actually done by
the MORTAR::Interface class (thus we use the mortar method).

The major part of this code is the Setup() method that gets the
non-matching interface meshes on input, initializes the mortar
interface and computes the so-called coupling matrices \f$D\f$ and \f$M\f$.

The actual coupling methods MasterToSlave() and SlaveToMaster()
just evaluate one simple equation each, i.e. primal variables
are projected from master to slave side via \f$D^{-1} M\f$ when
calling MasterToSlave(), and dual variables are projected from
slave to master side via \f$M^T D^{-T}\f$ when calling SlaveToMaster().

Whenever you want to add a new problem class, check whether you
can re-use one of the already existing Setup() methods. If not,
feel free to write your own tailored Setup() method.
*/
  class CouplingMortar : public CORE::ADAPTER::CouplingBase
  {
   public:
    /// Construct the CouplingMortar with basic parameters.
    CouplingMortar(int spatial_dimension, Teuchos::ParameterList mortar_coupling_params,
        Teuchos::ParameterList contact_dynamic_params,
        CORE::FE::ShapeFunctionType shape_function_type);

    /*! setup the machinery (generalized version)
     *
     *  \note
     *  - Master and slave discretizations are identical in case of sliding ALE or fluid/scatra
     * meshtying
     *  - ALE discretization is Teuchos::null in case of sliding ALE or fluid/scatra meshtying
     */
    void Setup(const Teuchos::RCP<DRT::Discretization>& masterdis,  ///< master discretization
        const Teuchos::RCP<DRT::Discretization>& slavedis,          ///< slave discretization
        const Teuchos::RCP<DRT::Discretization>& aledis,            ///< ALE discretization
        const std::vector<int>& coupleddof,  ///< vector defining coupled degrees of freedom
        const std::string& couplingcond,     ///< string for coupling condition
        const Epetra_Comm& comm,             ///< communicator
        const bool slavewithale = false,     ///< flag defining if slave is ALE
        const bool slidingale = false,       ///< flag indicating sliding ALE case
        const int nds_master = 0,            ///< master dofset number
        const int nds_slave = 0              ///< slave dofset number
    );

    /*! setup the machinery (generalized version)
     *
     *  \note
     *  - Master and slave discretizations are identical in case of sliding ALE or fluid/scatra
     * meshtying
     *  - ALE discretization is Teuchos::null in case of sliding ALE or fluid/scatra meshtying
     */
    void SetupInterface(
        const Teuchos::RCP<DRT::Discretization>& masterdis,  ///< master discretization
        const Teuchos::RCP<DRT::Discretization>& slavedis,   ///< slave discretization
        const std::vector<int>& coupleddof,  ///< vector defining coupled degrees of freedom
        const std::map<int, DRT::Node*>& mastergnodes,  ///< master nodes, including ghosted nodes
        const std::map<int, DRT::Node*>& slavegnodes,   ///< slave nodes, including ghosted nodes
        const std::map<int, Teuchos::RCP<DRT::Element>>& masterelements,  ///< master elements
        const std::map<int, Teuchos::RCP<DRT::Element>>& slaveelements,   ///< slave elements
        const Epetra_Comm& comm,                                          ///< communicator
        const bool slavewithale = false,  ///< flag defining if slave is ALE
        const bool slidingale = false,    ///< flag indicating sliding ALE case
        const int nds_master = 0,         ///< master dofset number
        const int nds_slave = 0           ///< slave dofset number
    );

    /// create integration cells
    virtual void EvaluateGeometry(std::vector<Teuchos::RCP<MORTAR::IntCell>>&
            intcells  //!< vector of mortar integration cells
    );

    /// Compute mortar matrices by using mortar interface using reference configuration
    virtual void Evaluate();

    /// Compute mortar matrices
    virtual void Evaluate(Teuchos::RCP<Epetra_Vector> idisp  ///< [in] ??
    );

    /// Compute mortar matrices (case of transferring same dofs on two different meshes)
    virtual void Evaluate(Teuchos::RCP<Epetra_Vector> idispma,  ///< [in] ??
        Teuchos::RCP<Epetra_Vector> idispsl                     ///< [in] ??
    );

    //! Compute mortar matrices after performing a mesh correction step
    virtual void EvaluateWithMeshRelocation(
        Teuchos::RCP<DRT::Discretization> slavedis,  ///< slave discretization
        Teuchos::RCP<DRT::Discretization> aledis,    ///< ALE discretization
        Teuchos::RCP<Epetra_Vector>& idisp,          ///< ALE displacements
        const Epetra_Comm& comm,                     ///< communicator
        bool slavewithale                            ///< flag defining if slave is ALE
    );

    //! Get the mortar interface itself
    Teuchos::RCP<MORTAR::Interface> Interface() const { return interface_; }

    //! Access to slave side mortar matrix \f$D\f$
    virtual Teuchos::RCP<CORE::LINALG::SparseMatrix> GetMortarMatrixD() const
    {
      if (D_ == Teuchos::null) dserror("D Matrix is null pointer!");
      return D_;
    };

    //! Access to inverse of slave side mortar matrix \f$D^{-1}\f$
    virtual Teuchos::RCP<CORE::LINALG::SparseMatrix> GetMortarMatrixDinv() const
    {
      if (Dinv_ == Teuchos::null) dserror("DInv Matrix is null pointer!");
      return Dinv_;
    };

    //! Access to master side mortar matrix \f$M\f$
    virtual Teuchos::RCP<CORE::LINALG::SparseMatrix> GetMortarMatrixM() const
    {
      if (M_ == Teuchos::null) dserror("M Matrix is null pointer!");
      return M_;
    };

    //! Access to mortar projection operator \f$P\f$
    virtual Teuchos::RCP<CORE::LINALG::SparseMatrix> GetMortarMatrixP() const
    {
      if (P_ == Teuchos::null) dserror("P Matrix is null pointer!");
      return P_;
    };

    /// @name Conversion between master and slave
    //@{

    /*! \brief Transfer a dof vector from master to slave
     *
     *  \return Slave vector
     */
    Teuchos::RCP<Epetra_Vector> MasterToSlave(
        Teuchos::RCP<Epetra_Vector> mv  ///< [in] master vector (to be transferred)
    ) const override;

    /*! \brief Transfer a dof vector from master to slave
     *
     *  \return Slave vector
     */
    Teuchos::RCP<Epetra_MultiVector> MasterToSlave(
        Teuchos::RCP<Epetra_MultiVector> mv  ///< [in] master vector (to be transferred)
    ) const override;

    /*! \brief Transfer a dof vector from master to slave (const version)
     *
     *  \return Slave vector
     */
    Teuchos::RCP<Epetra_Vector> MasterToSlave(
        Teuchos::RCP<const Epetra_Vector> mv  ///< [in] master vector (to be transferred)
    ) const override;

    /*! \brief Transfer a dof vector from master to slave (const version)
     *
     *  \return Slave vector
     */
    Teuchos::RCP<Epetra_MultiVector> MasterToSlave(
        Teuchos::RCP<const Epetra_MultiVector> mv  ///< [in] master vector (to be transferred)
    ) const override;

    /*! \brief Transfer a dof vector from slave to master
     *
     *  \return Master vector
     */
    Teuchos::RCP<Epetra_Vector> SlaveToMaster(
        Teuchos::RCP<Epetra_Vector> sv  ///< [in] slave vector (to be transferred)
    ) const override;

    /*! \brief Transfer a dof vector from slave to master
     *
     *  \return Master vector
     */
    Teuchos::RCP<Epetra_MultiVector> SlaveToMaster(
        Teuchos::RCP<Epetra_MultiVector> sv  ///< [in] slave vector (to be transferred)
    ) const override;

    /*! \brief Transfer a dof vector from slave to master (const version)
     *
     *  \return Master vector
     */
    Teuchos::RCP<Epetra_Vector> SlaveToMaster(
        Teuchos::RCP<const Epetra_Vector> sv  ///< [in] slave vector (to be transferred)
    ) const override;

    /*! \brief Transfer a dof vector from slave to master (const version)
     *
     *  \return Master vector
     */
    Teuchos::RCP<Epetra_MultiVector> SlaveToMaster(
        Teuchos::RCP<const Epetra_MultiVector> sv  ///< [in] slave vector (to be transferred)
    ) const override;

    /// transfer a dof vector from master to slave
    void MasterToSlave(
        Teuchos::RCP<const Epetra_MultiVector> mv,  ///< [in] master vector (to be transferred)
        Teuchos::RCP<Epetra_MultiVector> sv         ///< [out] slave vector (containing result)
    ) const override;

    /// transfer a dof vector from slave to master
    void SlaveToMaster(
        Teuchos::RCP<const Epetra_MultiVector> sv,  ///< [in] slave vector (to be transferred)
        Teuchos::RCP<Epetra_MultiVector> mv         ///< [out] master vector (containing result)
    ) const override;

    //@}

    /** \name Coupled maps */
    //@{

    /// Get the interface dof row map of the master side
    Teuchos::RCP<const Epetra_Map> MasterDofMap() const override { return pmasterdofrowmap_; }

    /// Get the interface dof row map of the slave side
    Teuchos::RCP<const Epetra_Map> SlaveDofMap() const override { return pslavedofrowmap_; }

    //@}

    /** \name Condensation methods */
    //@{

    /// do condensation of Lagrange multiplier and slave-sided dofs
    void MortarCondensation(
        Teuchos::RCP<CORE::LINALG::SparseMatrix>& k,  ///< in:  tangent matrix w/o condensation
                                                      ///< out: tangent matrix w/  condensation
        Teuchos::RCP<Epetra_Vector>& rhs              ///< in:  rhs vector     w/o condensation
                                                      ///< out: rhs vector     w/  condensation
    ) const;

    /// recover slave-sided dofs
    void MortarRecover(Teuchos::RCP<CORE::LINALG::SparseMatrix>& k,  ///< in: tangent matrix
        Teuchos::RCP<Epetra_Vector>& inc  ///< in:  solution vector     w/o condensation
                                          ///< out: solution vector     w/  condensation
    ) const;

    //@}

   protected:
    /// Create mortar projection operator \f$P=D{^1}M\f$
    virtual void CreateP();

    /*! \brief Check if slave dofs have Dirichlet constraints
     *
     *  Slave DOFs are not allowed to carry Dirichlet boundary conditions to
     *  avoid over-constraining the problem, cf. [1]
     *
     *  <h3> References </h3>
     *  [1] Puso, M and Laursen, TA: Mesh tying on curved interfaces in 3D,
     *      Engineering Computation, 20:305-319 (2003)
     */
    void CheckSlaveDirichletOverlap(
        const Teuchos::RCP<DRT::Discretization>& slavedis,  ///< [in] Slave discretization
        const Epetra_Comm& comm                             ///< [in] Communicator
    );

    /// back transformation to initial parallel distribution
    void MatrixRowColTransform();

    /// check setup call
    const bool& IsSetup() const { return issetup_; };

    /// check init and setup call
    virtual void CheckSetup() const
    {
      if (!IsSetup()) dserror("Call Setup() first!");
    }

   private:
    /// perform mesh relocation
    void MeshRelocation(Teuchos::RCP<DRT::Discretization> slavedis,  ///< [in] Slave discretization
        Teuchos::RCP<DRT::Discretization> aledis,                    ///< [in] ALE discretization
        Teuchos::RCP<const Epetra_Map>
            masterdofrowmap,  ///< [in] DOF row map of master discretization
        Teuchos::RCP<const Epetra_Map>
            slavedofrowmap,                  ///< [in] DOF row map of slave discretization
        Teuchos::RCP<Epetra_Vector>& idisp,  ///< [in] ALE displacements
        const Epetra_Comm& comm,             ///< [in] Communicator
        bool slavewithale                    ///< [in] Flag defining if slave is ALE
    );

   protected:
    //! Spatial dimension of problem
    int spatial_dimension_;

    //! Parameters for mortar coupling
    Teuchos::ParameterList mortar_coupling_params_;
    //! Parameters for contact dynamic
    Teuchos::ParameterList contact_dynamic_params_;

    //! Shape functions used in coupled discretizations
    CORE::FE::ShapeFunctionType shape_function_type_;

    /// Check for setup
    bool issetup_;

    /// Interface
    Teuchos::RCP<MORTAR::Interface> interface_;

    /// Map of master row dofs (after parallel redist.)
    Teuchos::RCP<const Epetra_Map> masterdofrowmap_;

    /// Map of slave row dofs  (after parallel redist.)
    Teuchos::RCP<const Epetra_Map> slavedofrowmap_;

    /// Map of master row dofs (before parallel redist.)
    Teuchos::RCP<const Epetra_Map> pmasterdofrowmap_;

    /// Map of slave row dofs  (before parallel redist.)
    Teuchos::RCP<const Epetra_Map> pslavedofrowmap_;

    /// Slave side mortar matrix \f$D\f$
    Teuchos::RCP<CORE::LINALG::SparseMatrix> D_;

    /// Inverse \f$D^{-1}\f$ of slave side mortar matrix \f$D\f$
    Teuchos::RCP<CORE::LINALG::SparseMatrix> Dinv_;

    /// Master side mortar matrix \f$M\f$
    Teuchos::RCP<CORE::LINALG::SparseMatrix> M_;

    /// Mortar projection operator \f$P=D^{-1}M\f$
    Teuchos::RCP<CORE::LINALG::SparseMatrix> P_;
  };
}  // namespace CORE::ADAPTER

FOUR_C_NAMESPACE_CLOSE

#endif
