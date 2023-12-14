/*----------------------------------------------------------------------*/
/*! \file

\brief functions to create geometry pairs.

\level 1
*/


#include "baci_geometry_pair_factory.H"

#include "baci_geometry_pair_element.H"
#include "baci_geometry_pair_line_to_3D_evaluation_data.H"
#include "baci_geometry_pair_line_to_surface_evaluation_data.H"
#include "baci_geometry_pair_line_to_surface_gauss_point_projection.H"
#include "baci_geometry_pair_line_to_surface_segmentation.H"
#include "baci_geometry_pair_line_to_volume_gauss_point_projection.H"
#include "baci_geometry_pair_line_to_volume_segmentation.H"
#include "baci_geometry_pair_scalar_types.H"

BACI_NAMESPACE_OPEN

/**
 *
 */
template <typename scalar_type, typename line, typename volume>
Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToVolumeFactory(
    const DRT::Element* element1, const DRT::Element* element2,
    const Teuchos::RCP<GeometryEvaluationDataBase>& geometry_evaluation_data)
{
  // Cast the geometry evaluation data to the correct format.
  auto line_to_3d_evaluation_data =
      Teuchos::rcp_dynamic_cast<LineTo3DEvaluationData>(geometry_evaluation_data, true);

  // Get the strategy for line to volume interaction.
  INPAR::GEOMETRYPAIR::LineTo3DStrategy strategy = line_to_3d_evaluation_data->GetStrategy();

  // Create the class depending on the strategy.
  switch (strategy)
  {
    case INPAR::GEOMETRYPAIR::LineTo3DStrategy::
        gauss_point_projection_without_boundary_segmentation:
    case INPAR::GEOMETRYPAIR::LineTo3DStrategy::gauss_point_projection_boundary_segmentation:
      return Teuchos::rcp(
          new GeometryPairLineToVolumeGaussPointProjection<scalar_type, line, volume>(
              element1, element2, line_to_3d_evaluation_data));
    case INPAR::GEOMETRYPAIR::LineTo3DStrategy::segmentation:
      return Teuchos::rcp(new GeometryPairLineToVolumeSegmentation<scalar_type, line, volume>(
          element1, element2, line_to_3d_evaluation_data));
    default:
    {
      dserror(
          "The given geometry pair strategy is not universally valid. You might want to create "
          "your pair directly if you need certain features (for example cross section "
          "projection)!");
      return Teuchos::null;
    }
  }
}


/**
 * Explicit template initialization of factory function.
 */
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToVolumeFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_hex8>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToVolumeFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_hex20>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToVolumeFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_hex27>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToVolumeFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_tet4>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToVolumeFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_tet10>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToVolumeFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_nurbs27>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);


/**
 *
 */
template <typename scalar_type, typename line, typename surface>
Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory(
    const DRT::Element* element1, const DRT::Element* element2,
    const Teuchos::RCP<GeometryEvaluationDataBase>& geometry_evaluation_data)
{
  // Cast the geometry evaluation data to the correct format.
  auto line_to_surface_evaluation_data =
      Teuchos::rcp_dynamic_cast<LineToSurfaceEvaluationData>(geometry_evaluation_data, true);

  // Get the strategy for line to volume interaction.
  INPAR::GEOMETRYPAIR::LineTo3DStrategy strategy = line_to_surface_evaluation_data->GetStrategy();

  // Create the class depending on the strategy.
  switch (strategy)
  {
    case INPAR::GEOMETRYPAIR::LineTo3DStrategy::
        gauss_point_projection_without_boundary_segmentation:
    case INPAR::GEOMETRYPAIR::LineTo3DStrategy::gauss_point_projection_boundary_segmentation:
      return Teuchos::rcp(
          new GeometryPairLineToSurfaceGaussPointProjection<scalar_type, line, surface>(
              element1, element2, line_to_surface_evaluation_data));
    case INPAR::GEOMETRYPAIR::LineTo3DStrategy::segmentation:
      return Teuchos::rcp(new GeometryPairLineToSurfaceSegmentation<scalar_type, line, surface>(
          element1, element2, line_to_surface_evaluation_data));
    default:
    {
      dserror("The given geometry pair strategy is not valid.");
      return Teuchos::null;
    }
  }
}

/**
 *
 */
template <typename scalar_type, typename line, typename surface>
Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD(
    const DRT::Element* element1, const DRT::Element* element2,
    const Teuchos::RCP<GeometryEvaluationDataBase>& geometry_evaluation_data)
{
  // Create the internal pair as double.
  auto internal_geometry_pair_double =
      Teuchos::rcp_dynamic_cast<GeometryPairLineToSurface<double, line, surface>>(
          GeometryPairLineToSurfaceFactory<double, line, surface>(
              element1, element2, geometry_evaluation_data),
          true);

  // Create the wrapper.
  return Teuchos::rcp(new GeometryPairLineToSurfaceFADWrapper<scalar_type, line, surface>(
      element1, element2, internal_geometry_pair_double));
}


/**
 * Explicit template initialization of factory function.
 */
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair>
GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<double, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_tri3>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair>
GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<double, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_tri6>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_quad4>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_quad8>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_quad9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_nurbs9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);

template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_line2,
    GEOMETRYPAIR::t_tri3>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_line2,
    GEOMETRYPAIR::t_tri6>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_line2,
    GEOMETRYPAIR::t_quad4>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_line2,
    GEOMETRYPAIR::t_quad8>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_line2,
    GEOMETRYPAIR::t_quad9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_fixed_size_1st_order<GEOMETRYPAIR::t_line2,
        GEOMETRYPAIR::t_nurbs9>,
    GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_nurbs9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);

template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_tri3>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_tri6>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_quad4>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_quad8>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type, GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_quad9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_fixed_size<GEOMETRYPAIR::t_line2,
        GEOMETRYPAIR::t_nurbs9>,
    GEOMETRYPAIR::t_line2, GEOMETRYPAIR::t_nurbs9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);

template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_tri3>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_tri6>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_quad4>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_quad8>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_quad9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactory<
    double, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_nurbs9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);

template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_hermite,
    GEOMETRYPAIR::t_tri3>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_hermite,
    GEOMETRYPAIR::t_tri6>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_hermite,
    GEOMETRYPAIR::t_quad4>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_hermite,
    GEOMETRYPAIR::t_quad8>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_1st_order, GEOMETRYPAIR::t_hermite,
    GEOMETRYPAIR::t_quad9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_fixed_size_1st_order<GEOMETRYPAIR::t_hermite,
        GEOMETRYPAIR::t_nurbs9>,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_nurbs9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);

template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_tri3>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type, GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_tri6>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair>
GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<GEOMETRYPAIR::line_to_surface_patch_scalar_type,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_quad4>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair>
GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<GEOMETRYPAIR::line_to_surface_patch_scalar_type,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_quad8>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair>
GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<GEOMETRYPAIR::line_to_surface_patch_scalar_type,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_quad9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);
template Teuchos::RCP<GEOMETRYPAIR::GeometryPair> GEOMETRYPAIR::GeometryPairLineToSurfaceFactoryFAD<
    GEOMETRYPAIR::line_to_surface_patch_scalar_type_fixed_size<GEOMETRYPAIR::t_hermite,
        GEOMETRYPAIR::t_nurbs9>,
    GEOMETRYPAIR::t_hermite, GEOMETRYPAIR::t_nurbs9>(
    const DRT::Element*, const DRT::Element*, const Teuchos::RCP<GeometryEvaluationDataBase>&);

BACI_NAMESPACE_CLOSE
