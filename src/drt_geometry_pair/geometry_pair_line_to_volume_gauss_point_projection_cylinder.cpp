/*!incomplete
\file geometry_pair_line_to_volume_gauss_point_projection_cylinder.cpp

\brief Line to volume interaction with simple Gauss point projection and boundary segmentation.

\level 1
\maintainer Ivo Steinbrecher
*/


#include "geometry_pair_line_to_volume_gauss_point_projection_cylinder.H"
#include "geometry_pair_element_types.H"
#include "geometry_pair_evaluation_data_global.H"
#include "geometry_pair_line_to_volume_evaluation_data.H"
#include "geometry_pair_utility_classes.H"

#include "../drt_lib/drt_element.H"
#include "../drt_fem_general/drt_utils_integration.H"

#include <math.h>


#define radius 0.1


/**
 *
 */
template <typename scalar_type, typename line, typename volume>
void GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<scalar_type, line,
    volume>::Setup()
{
  // Call Setup on the base class.
  GeometryPairLineToVolume<scalar_type, line, volume>::Setup();

  // Check if a projection tracking vector exists for this line element. If not a new one is
  // created.
  int line_element_id = this->Element1()->Id();
  std::map<int, std::vector<bool>>& projection_tracker =
      this->EvaluationData()->LineToVolumeEvaluationData()->GetGaussPointProjectionTrackerMutable();

  if (projection_tracker.find(line_element_id) == projection_tracker.end())
  {
    int n_gauss_points =
        this->EvaluationData()->LineToVolumeEvaluationData()->GetNumberOfGaussPoints() *
        this->EvaluationData()->LineToVolumeEvaluationData()->GetGaussPointsCircumfence();
    std::vector<bool> new_tracking_vector;
    new_tracking_vector.resize(n_gauss_points, false);
    projection_tracker[line_element_id] = new_tracking_vector;
  }
}


/**
 *
 */
template <typename scalar_type, typename line, typename volume>
void GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<scalar_type, line,
    volume>::PreEvaluateCylinder(const LINALG::TMatrix<scalar_type, line::n_dof_, 1>& q_line,
    const LINALG::TMatrix<scalar_type, volume::n_dof_, 1>& q_volume,
    std::vector<GEOMETRYPAIR::ProjectionPointVolumeToVolume<scalar_type>>&
        cylinder_to_volume_points) const
{
  // Check if the element is initialized.
  this->CheckInitSetup();

  // Get the Gauss point projection tracker for this line element.
  std::vector<bool>& line_projection_tracker = GetLineProjectionVectorMutable();

  // Gauss rule.
  DRT::UTILS::IntegrationPoints1D gauss_points_axis =
      this->EvaluationData()->LineToVolumeEvaluationData()->GetGaussPoints();
  unsigned int n_gauss_points_axis =
      this->EvaluationData()->LineToVolumeEvaluationData()->GetNumberOfGaussPoints();
  unsigned int n_gauss_points_circ =
      this->EvaluationData()->LineToVolumeEvaluationData()->GetGaussPointsCircumfence();

  // Initilaize variables for the projection.
  scalar_type eta;
  double alpha;
  LINALG::TMatrix<scalar_type, 3, 1> r_beam;
  LINALG::TMatrix<scalar_type, 3, 1> xi_beam;
  LINALG::TMatrix<scalar_type, 3, 1> xi_solid;
  ProjectionResult projection_result;
  cylinder_to_volume_points.clear();

  // Loop over Gauss points and check if they project to this volume.
  for (unsigned int index_gp_axis = 0; index_gp_axis < n_gauss_points_axis; index_gp_axis++)
  {
    for (unsigned int index_gp_circ = 0; index_gp_circ < n_gauss_points_circ; index_gp_circ++)
    {
      // Index of the current Gauss point in the tracking vector.
      unsigned int index_gp = index_gp_axis * n_gauss_points_circ + index_gp_circ;

      // Only check points that do not already have a valid projection.
      if (line_projection_tracker[index_gp] == false)
      {
        // Centerline coordinate.
        eta = gauss_points_axis.qxg[index_gp_axis][0];

        // Get the spatial position of the beam centerline.
        GEOMETRYPAIR::EvaluatePosition<line>(eta, q_line, r_beam, this->Element1());

        // Add the in crossection position.
        alpha = 2. * M_PI / double(n_gauss_points_circ) * index_gp_circ;
        r_beam(1) += radius * cos(alpha);
        r_beam(2) += radius * sin(alpha);

        // Parameter coordinates on the beam.
        r_beam(0) = eta;
        r_beam(1) = cos(alpha);
        r_beam(2) = sin(alpha);

        this->ProjectPointToVolume(r_beam, q_volume, xi_solid, projection_result);
        if (projection_result == ProjectionResult::projection_found_valid)
        {
          // Valid Gauss point was found, add to this segment and set tracking point to true.
          cylinder_to_volume_points.push_back(ProjectionPointVolumeToVolume<scalar_type>(xi_beam,
              xi_solid, gauss_points_axis.qwgt[index_gp_axis] * 2. / double(n_gauss_points_circ)));
          line_projection_tracker[index_gp] = true;
        }
      }
    }
  }
}


/**
 *
 */
template <typename scalar_type, typename line, typename volume>
void GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<scalar_type, line,
    volume>::EvaluateCylinder(const LINALG::TMatrix<scalar_type, line::n_dof_, 1>& q_line,
    const LINALG::TMatrix<scalar_type, volume::n_dof_, 1>& q_volume,
    std::vector<GEOMETRYPAIR::ProjectionPointVolumeToVolume<scalar_type>>&
        cylinder_to_volume_points) const
{
#if 0
  // Check if the element is initialized.
  this->CheckInitSetup();

  // Only zero one segments are expected.
  if (segments.size() > 1)
    dserror(
        "There should be zero or one segments for the Gauss point projection method. The actual "
        "value is %d!",
        segments.size());

  // Check if one point projected in PreEvaluate.
  if (segments.size() == 1 && segments[0].GetNumberOfProjectionPoints() > 0)
  {
    // Flag if segmentation is needed.
    bool need_segmentation = false;

    // Check if all Gauss points projected for this line.
    const std::vector<bool>& line_projection_tracker = GetLineProjectionVectorMutable();
    for (auto const& projects : line_projection_tracker)
      if (!projects) need_segmentation = true;

    if (need_segmentation)
    {
      // Segmentation is needed. First get the intersection points with the volume.
      std::vector<ProjectionPointLineToVolume<scalar_type>> intersection_points;
      this->IntersectLineWithVolume(q_line, q_volume, intersection_points);

      // This algorithm only works if one intersection point was found.
      if (intersection_points.size() != 1)
        dserror("In the segmentation case we expect exactly one found intersection point. Got: %d!",
            intersection_points.size());

      // Get the limits of the segmented line.
      scalar_type eta_a, eta_b, eta_intersection_point, eta_first_gauss_point;
      eta_intersection_point = intersection_points[0].GetEta();
      eta_first_gauss_point = segments[0].GetProjectionPoints()[0].GetEta();
      if (eta_intersection_point < eta_first_gauss_point)
      {
        eta_a = eta_intersection_point;
        eta_b = 1.;
      }
      else
      {
        eta_a = -1.;
        eta_b = eta_intersection_point;
      }

      // Reproject the Gauss points on the segmented line.
      segments[0] = LineSegment<scalar_type>(eta_a, eta_b);
      this->ProjectGaussPointsOnSegmentToVolume(q_line, q_volume,
          this->EvaluationData()->LineToVolumeEvaluationData()->GetGaussPoints(), segments[0]);
    }
  }
#endif
}


/**
 *
 */
template <typename scalar_type, typename line, typename volume>
std::vector<bool>& GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<scalar_type,
    line, volume>::GetLineProjectionVectorMutable() const
{
  // Get the Gauss point projection tracker for this line element.
  int line_element_id = this->Element1()->Id();
  std::map<int, std::vector<bool>>& projection_tracker =
      this->EvaluationData()->LineToVolumeEvaluationData()->GetGaussPointProjectionTrackerMutable();
  return projection_tracker[line_element_id];
}


/**
 * Explicit template initialization of template class.
 */
template class GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<double,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_hex8>;
template class GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<double,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_hex20>;
template class GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<double,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_hex27>;
template class GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<double,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_tet4>;
template class GEOMETRYPAIR::GeometryPairLineToVolumeGaussPointProjectionCylinder<double,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_tet10>;
