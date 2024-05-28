/*----------------------------------------------------------------------*/
/*! \file

\brief Base element for interactions between a beam and a solid.

\level 3
*/


#include "4C_beaminteraction_beam_to_solid_pair_base.hpp"

#include "4C_beam3_euler_bernoulli.hpp"
#include "4C_beam3_kirchhoff.hpp"
#include "4C_beam3_reissner.hpp"
#include "4C_beaminteraction_beam_to_solid_visualization_output_writer_base.hpp"
#include "4C_beaminteraction_beam_to_solid_visualization_output_writer_visualization.hpp"
#include "4C_beaminteraction_geometry_pair_access_traits.hpp"
#include "4C_geometry_pair_element_evaluation_functions.hpp"
#include "4C_geometry_pair_scalar_types.hpp"

#include <Sacado.hpp>

FOUR_C_NAMESPACE_OPEN


/**
 *
 */
template <typename scalar_type, typename segments_scalar_type, typename beam, typename solid>
BEAMINTERACTION::BeamToSolidPairBase<scalar_type, segments_scalar_type, beam,
    solid>::BeamToSolidPairBase()
    : BeamContactPair(), line_to_3D_segments_()
{
}


/**
 *
 */
template <typename scalar_type, typename segments_scalar_type, typename beam, typename solid>
void BEAMINTERACTION::BeamToSolidPairBase<scalar_type, segments_scalar_type, beam, solid>::Setup()
{
  check_init();

  // Call setup of base class first.
  BeamContactPair::Setup();

  // Get the beam element data container
  ele1posref_ = GEOMETRYPAIR::InitializeElementData<beam, double>::Initialize(Element1());
  ele1pos_ = GEOMETRYPAIR::InitializeElementData<beam, scalar_type>::Initialize(Element1());

  // Set reference nodal positions (and tangents) for beam element
  for (unsigned int n = 0; n < beam::n_nodes_; ++n)
  {
    const DRT::Node* node = Element1()->Nodes()[n];
    for (int d = 0; d < 3; ++d)
      ele1posref_.element_position_(3 * beam::n_val_ * n + d) = node->X()[d];

    // tangents
    if (beam::n_val_ == 2)
    {
      CORE::LINALG::Matrix<3, 1> tan;
      const DRT::ElementType& eot = Element1()->ElementType();

      if (eot == DRT::ELEMENTS::Beam3rType::Instance())
      {
        const DRT::ELEMENTS::Beam3r* ele = dynamic_cast<const DRT::ELEMENTS::Beam3r*>(Element1());
        if (ele->hermite_centerline_interpolation())
          tan = ele->Tref()[n];
        else
          FOUR_C_THROW(
              "ERROR: Beam3tosolidmeshtying: beam::n_val_=2 detected for beam3r element w/o "
              "Hermite centerline");
      }
      else if (eot == DRT::ELEMENTS::Beam3kType::Instance())
      {
        const DRT::ELEMENTS::Beam3k* ele = dynamic_cast<const DRT::ELEMENTS::Beam3k*>(Element1());
        tan = ele->Tref()[n];
      }
      else if (eot == DRT::ELEMENTS::Beam3ebType::Instance())
      {
        const DRT::ELEMENTS::Beam3eb* ele = dynamic_cast<const DRT::ELEMENTS::Beam3eb*>(Element1());
        tan = ele->Tref()[n];
      }
      else
      {
        FOUR_C_THROW("ERROR: Beam3tosolidmeshtying: Invalid beam element type");
      }

      for (int d = 0; d < 3; ++d)
        ele1posref_.element_position_(3 * beam::n_val_ * n + d + 3) = tan(d, 0);
    }
  }

  // Initialize current nodal positions (and tangents) for beam element
  for (unsigned int i = 0; i < beam::n_dof_; i++) ele1pos_.element_position_(i) = 0.0;

  issetup_ = true;
}

/**
 *
 */
template <typename scalar_type, typename segments_scalar_type, typename beam, typename solid>
void BEAMINTERACTION::BeamToSolidPairBase<scalar_type, segments_scalar_type, beam,
    solid>::ResetState(const std::vector<double>& beam_centerline_dofvec,
    const std::vector<double>& solid_nodal_dofvec)
{
  // Set the current configuration of the beam element
  ele1pos_ = GEOMETRYPAIR::InitializeElementData<beam, scalar_type>::Initialize(Element1());
  for (unsigned int i = 0; i < beam::n_dof_; i++)
    ele1pos_.element_position_(i) = CORE::FADUTILS::HigherOrderFadValue<scalar_type>::apply(
        beam::n_dof_ + solid::n_dof_, i, beam_centerline_dofvec[i]);
}

/**
 *
 */
template <typename scalar_type, typename segments_scalar_type, typename beam, typename solid>
void BEAMINTERACTION::BeamToSolidPairBase<scalar_type, segments_scalar_type, beam, solid>::
    set_restart_displacement(const std::vector<std::vector<double>>& centerline_restart_vec_)
{
  // Call the parent method.
  BeamContactPair::set_restart_displacement(centerline_restart_vec_);
}

/**
 *
 */
template <typename scalar_type, typename segments_scalar_type, typename beam, typename solid>
void BEAMINTERACTION::BeamToSolidPairBase<scalar_type, segments_scalar_type, beam, solid>::Print(
    std::ostream& out) const
{
  check_init_setup();

  // Print some general information: Element IDs and dofvecs.
  out << "\n------------------------------------------------------------------------";
  out << "\nInstance of BeamToSolidPairBase"
      << "\nBeam EleGID:  " << Element1()->Id() << "\nSolid EleGID: " << Element2()->Id();

  out << "\n\nbeam dofvec: " << ele1pos_.element_position_;
  out << "\nn_segments: " << line_to_3D_segments_.size();
  out << "\n";
  out << "------------------------------------------------------------------------\n";
}

/**
 *
 */
template <typename scalar_type, typename segments_scalar_type, typename beam, typename solid>
void BEAMINTERACTION::BeamToSolidPairBase<scalar_type, segments_scalar_type, beam,
    solid>::print_summary_one_line_per_active_segment_pair(std::ostream& out) const
{
  check_init_setup();

  // Only display information if a segment exists for this pair.
  if (line_to_3D_segments_.size() == 0) return;

  // Display the number of segments and segment length.
  out << "beam ID " << Element1()->Id() << ", solid ID " << Element2()->Id() << ":";
  out << " n_segments = " << line_to_3D_segments_.size() << "\n";

  // Loop over segments and display information about them.
  for (unsigned int index_segment = 0; index_segment < line_to_3D_segments_.size(); index_segment++)
  {
    out << "    segment " << index_segment << ": ";
    out << "eta in [" << CORE::FADUTILS::CastToDouble(line_to_3D_segments_[index_segment].GetEtaA())
        << ", " << CORE::FADUTILS::CastToDouble(line_to_3D_segments_[index_segment].GetEtaB())
        << "]";
    out << ", Gauss points = "
        << line_to_3D_segments_[index_segment].get_number_of_projection_points();
    out << "\n";
  }
}

/**
 *
 */
template <typename scalar_type, typename segments_scalar_type, typename beam, typename solid>
void BEAMINTERACTION::BeamToSolidPairBase<scalar_type, segments_scalar_type, beam,
    solid>::evaluate_beam_position_double(const GEOMETRYPAIR::ProjectionPoint1DTo3D<double>&
                                              integration_point,
    CORE::LINALG::Matrix<3, 1, double>& r_beam, bool reference) const
{
  if (reference)
    GEOMETRYPAIR::EvaluatePosition<beam>(integration_point.GetEta(), ele1posref_, r_beam);
  else
    GEOMETRYPAIR::EvaluatePosition<beam>(integration_point.GetEta(),
        GEOMETRYPAIR::ElementDataToDouble<beam>::ToDouble(ele1pos_), r_beam);
}


/**
 * Explicit template initialization of template class.
 */
namespace BEAMINTERACTION
{
  using namespace GEOMETRYPAIR;

  // Beam-to-volume pairs
  template class BeamToSolidPairBase<line_to_volume_scalar_type<t_hermite, t_hex8>, double,
      t_hermite, t_hex8>;
  template class BeamToSolidPairBase<line_to_volume_scalar_type<t_hermite, t_hex20>, double,
      t_hermite, t_hex20>;
  template class BeamToSolidPairBase<line_to_volume_scalar_type<t_hermite, t_hex27>, double,
      t_hermite, t_hex27>;
  template class BeamToSolidPairBase<line_to_volume_scalar_type<t_hermite, t_tet4>, double,
      t_hermite, t_tet4>;
  template class BeamToSolidPairBase<line_to_volume_scalar_type<t_hermite, t_tet10>, double,
      t_hermite, t_tet10>;
  template class BeamToSolidPairBase<line_to_volume_scalar_type<t_hermite, t_nurbs27>, double,
      t_hermite, t_nurbs27>;

  // Beam-to-surface pairs
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_line2, t_quad4>, double, t_line2,
      t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_line2, t_quad8>, double, t_line2,
      t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_line2, t_quad9>, double, t_line2,
      t_quad9>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_line2, t_tri3>, double, t_line2,
      t_tri3>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_line2, t_tri6>, double, t_line2,
      t_tri6>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_line2, t_nurbs9>, double,
      t_line2, t_nurbs9>;

  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_line2, t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_line2, t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_line2, t_quad9>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_line2, t_tri3>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_line2, t_tri6>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size<t_line2, t_nurbs9>, double, t_line2, t_nurbs9>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_fixed_size<t_line2, t_hex8>,
      double, t_line2, t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_fixed_size<t_line2, t_hex20>,
      double, t_line2, t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_fixed_size<t_line2, t_hex27>,
      double, t_line2, t_quad9>;

  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_line2, t_tri3>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_line2, t_tri6>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_line2, t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_line2, t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_line2, t_quad9>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size_1st_order<t_line2, t_nurbs9>,
      line_to_surface_patch_scalar_type_fixed_size_1st_order<t_line2, t_nurbs9>, t_line2, t_nurbs9>;

  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_line2, t_tri3>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_line2, t_tri6>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_line2, t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_line2, t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_line2, t_quad9>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size<t_line2, t_nurbs9>,
      line_to_surface_patch_scalar_type_fixed_size<t_line2, t_nurbs9>, t_line2, t_nurbs9>;

  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_hermite, t_quad4>, double,
      t_hermite, t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_hermite, t_quad8>, double,
      t_hermite, t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_hermite, t_quad9>, double,
      t_hermite, t_quad9>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_hermite, t_tri3>, double,
      t_hermite, t_tri3>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_hermite, t_tri6>, double,
      t_hermite, t_tri6>;
  template class BeamToSolidPairBase<line_to_surface_scalar_type<t_hermite, t_nurbs9>, double,
      t_hermite, t_nurbs9>;

  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_hermite, t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_hermite, t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_hermite, t_quad9>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_hermite, t_tri3>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type, double, t_hermite, t_tri6>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size<t_hermite, t_nurbs9>, double, t_hermite,
      t_nurbs9>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size<t_hermite, t_hex8>, double, t_hermite, t_quad4>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size<t_hermite, t_hex20>, double, t_hermite, t_quad8>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size<t_hermite, t_hex27>, double, t_hermite, t_quad9>;

  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_hermite, t_tri3>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_hermite, t_tri6>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_hermite, t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_hermite, t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type_1st_order,
      line_to_surface_patch_scalar_type_1st_order, t_hermite, t_quad9>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size_1st_order<t_hermite, t_nurbs9>,
      line_to_surface_patch_scalar_type_fixed_size_1st_order<t_hermite, t_nurbs9>, t_hermite,
      t_nurbs9>;

  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_hermite, t_tri3>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_hermite, t_tri6>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_hermite, t_quad4>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_hermite, t_quad8>;
  template class BeamToSolidPairBase<line_to_surface_patch_scalar_type,
      line_to_surface_patch_scalar_type, t_hermite, t_quad9>;
  template class BeamToSolidPairBase<
      line_to_surface_patch_scalar_type_fixed_size<t_hermite, t_nurbs9>,
      line_to_surface_patch_scalar_type_fixed_size<t_hermite, t_nurbs9>, t_hermite, t_nurbs9>;
}  // namespace BEAMINTERACTION

FOUR_C_NAMESPACE_CLOSE