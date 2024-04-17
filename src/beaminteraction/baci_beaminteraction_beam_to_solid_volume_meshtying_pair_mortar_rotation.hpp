/*----------------------------------------------------------------------*/
/*! \file

\brief Meshtying element for rotational meshtying between a 3D beam and a 3D solid element.

\level 3
*/
// End doxygen header.


#ifndef FOUR_C_BEAMINTERACTION_BEAM_TO_SOLID_VOLUME_MESHTYING_PAIR_MORTAR_ROTATION_HPP
#define FOUR_C_BEAMINTERACTION_BEAM_TO_SOLID_VOLUME_MESHTYING_PAIR_MORTAR_ROTATION_HPP


#include "baci_config.hpp"

#include "baci_beaminteraction_beam_to_solid_volume_meshtying_pair_mortar.hpp"

FOUR_C_NAMESPACE_OPEN


// Forward declarations.
namespace INPAR
{
  namespace BEAMTOSOLID
  {
    enum class BeamToSolidRotationCoupling;
  }
}  // namespace INPAR
namespace LARGEROTATIONS
{
  template <unsigned int numnodes, typename T>
  class TriadInterpolationLocalRotationVectors;
}  // namespace LARGEROTATIONS


namespace BEAMINTERACTION
{
  /**
   * \brief Class for beam to solid rotational meshtying.
   * @param beam Type from GEOMETRYPAIR::ElementDiscretization... representing the beam.
   * @param solid Type from GEOMETRYPAIR::ElementDiscretization... representing the solid.
   * @param mortar Type from BEAMINTERACTION::ElementDiscretization... representing the mortar shape
   * functions for displacement coupling.
   * @param mortar_rot Type from BEAMINTERACTION::ElementDiscretization... representing the mortar
   * shape functions for rotational coupling.
   */
  template <typename beam, typename solid, typename mortar, typename mortar_rot>
  class BeamToSolidVolumeMeshtyingPairMortarRotation
      : public BeamToSolidVolumeMeshtyingPairMortar<beam, solid, mortar>
  {
   protected:
    //! Shortcut to the base class.
    using base_class = BeamToSolidVolumeMeshtyingPairMortar<beam, solid, mortar>;

    //! Type to be used for scalar AD variables.
    using scalar_type = typename base_class::scalar_type;

    //! FAD type to evaluate the rotational coupling terms. The first 3 entries are the values of
    //! psi_beam, the following entries are the discrete solid DOFs.
    using scalar_type_rot_1st = typename Sacado::Fad::SLFad<double, 3 + solid::n_dof_>;
    using scalar_type_rot_2nd =
        typename CORE::FADUTILS::HigherOrderFadType<2, scalar_type_rot_1st>::type;

    //! Number of rotational DOF for the SR beams;
    static constexpr unsigned int n_dof_rot_ = 9;
    static constexpr unsigned int n_dof_pair_ = n_dof_rot_ + solid::n_dof_;

   public:
    /**
     * \brief Standard Constructor
     */
    BeamToSolidVolumeMeshtyingPairMortarRotation();

    /**
     * \brief Evaluate the global matrices and vectors resulting from mortar coupling. (derived)
     */
    void EvaluateAndAssembleMortarContributions(const DRT::Discretization& discret,
        const BeamToSolidMortarManager* mortar_manager, CORE::LINALG::SparseMatrix& global_G_B,
        CORE::LINALG::SparseMatrix& global_G_S, CORE::LINALG::SparseMatrix& global_FB_L,
        CORE::LINALG::SparseMatrix& global_FS_L, Epetra_FEVector& global_constraint,
        Epetra_FEVector& global_kappa, Epetra_FEVector& global_lambda_active,
        const Teuchos::RCP<const Epetra_Vector>& displacement_vector) override;

    /**
     * \brief Evaluate the pair and directly assemble it into the global force vector and stiffness
     * matrix (derived).
     */
    void EvaluateAndAssemble(const DRT::Discretization& discret,
        const BeamToSolidMortarManager* mortar_manager,
        const Teuchos::RCP<Epetra_FEVector>& force_vector,
        const Teuchos::RCP<CORE::LINALG::SparseMatrix>& stiffness_matrix,
        const Epetra_Vector& global_lambda, const Epetra_Vector& displacement_vector) override;

   private:
    /**
     * \brief Evaluate the constraint vector and the coupling matrices.
     */
    void EvaluateRotationalCouplingTerms(
        const INPAR::BEAMTOSOLID::BeamToSolidRotationCoupling& rot_coupling_type,
        const GEOMETRYPAIR::ElementData<solid, scalar_type_rot_1st>& q_solid,
        const LARGEROTATIONS::TriadInterpolationLocalRotationVectors<3, double>&
            triad_interpolation_scheme,
        const LARGEROTATIONS::TriadInterpolationLocalRotationVectors<3, double>&
            ref_triad_interpolation_scheme,
        CORE::LINALG::Matrix<mortar_rot::n_dof_, 1, double>& local_g,
        CORE::LINALG::Matrix<mortar_rot::n_dof_, n_dof_rot_, double>& local_G_B,
        CORE::LINALG::Matrix<mortar_rot::n_dof_, solid::n_dof_, double>& local_G_S,
        CORE::LINALG::Matrix<n_dof_rot_, mortar_rot::n_dof_, double>& local_FB_L,
        CORE::LINALG::Matrix<solid::n_dof_, mortar_rot::n_dof_, double>& local_FS_L,
        CORE::LINALG::Matrix<mortar_rot::n_dof_, 1, double>& local_kappa) const;

    /**
     * \brief Evaluate the stiffness contributions of this pair.
     */
    void EvaluateRotationalCouplingStiffTerms(
        const INPAR::BEAMTOSOLID::BeamToSolidRotationCoupling& rot_coupling_type,
        const GEOMETRYPAIR::ElementData<solid, scalar_type_rot_2nd>& q_solid,
        CORE::LINALG::Matrix<mortar_rot::n_dof_, 1, double>& lambda_rot,
        const LARGEROTATIONS::TriadInterpolationLocalRotationVectors<3, double>&
            triad_interpolation_scheme,
        const LARGEROTATIONS::TriadInterpolationLocalRotationVectors<3, double>&
            ref_triad_interpolation_scheme,
        CORE::LINALG::Matrix<n_dof_rot_, n_dof_rot_, double>& local_stiff_BB,
        CORE::LINALG::Matrix<n_dof_rot_, solid::n_dof_, double>& local_stiff_BS,
        CORE::LINALG::Matrix<solid::n_dof_, n_dof_rot_, double>& local_stiff_SB,
        CORE::LINALG::Matrix<solid::n_dof_, solid::n_dof_, double>& local_stiff_SS) const;
  };
}  // namespace BEAMINTERACTION

FOUR_C_NAMESPACE_CLOSE

#endif
