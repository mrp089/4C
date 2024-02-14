/*! \file

\brief Implementation of routines for calculation of a coupled solid-scatra element with templated
solid formulation

\level 1
*/

#include "baci_solid_scatra_ele_calc.hpp"

#include "baci_discretization_fem_general_cell_type.hpp"
#include "baci_discretization_fem_general_cell_type_traits.hpp"
#include "baci_lib_discret.hpp"
#include "baci_mat_so3_material.hpp"
#include "baci_solid_ele_calc_displacement_based.hpp"
#include "baci_solid_ele_calc_lib.hpp"
#include "baci_solid_ele_calc_lib_integration.hpp"
#include "baci_solid_ele_calc_lib_io.hpp"
#include "baci_solid_ele_interface_serializable.hpp"
#include "baci_utils_exceptions.hpp"

#include <Teuchos_ParameterList.hpp>

#include <optional>

BACI_NAMESPACE_OPEN

namespace
{
  template <CORE::FE::CellType celltype>
  inline static constexpr int num_str = CORE::FE::dim<celltype>*(CORE::FE::dim<celltype> + 1) / 2;

  template <CORE::FE::CellType celltype>
  CORE::LINALG::Matrix<num_str<celltype>, 1> EvaluateDMaterialStressDScalar(
      MAT::So3Material& solid_material,
      const CORE::LINALG::Matrix<CORE::FE::dim<celltype>, CORE::FE::dim<celltype>>&
          deformation_gradient,
      const CORE::LINALG::Matrix<num_str<celltype>, 1>& gl_strain, Teuchos::ParameterList& params,
      const int gp, const int eleGID)
  {
    CORE::LINALG::Matrix<num_str<celltype>, 1> dStressDScalar(true);

    // The derivative of the solid stress w.r.t. the scalar is implemented in the normal material
    // Evaluate call by not passing the linearization matrix.
    solid_material.Evaluate(
        &deformation_gradient, &gl_strain, params, &dStressDScalar, nullptr, gp, eleGID);

    return dStressDScalar;
  }

  template <CORE::FE::CellType celltype>
  void PrepareScatraQuantityInParameterList(const DRT::Discretization& discretization,
      const DRT::Element::LocationArray& la,
      const DRT::ELEMENTS::ElementNodes<celltype>& element_nodes, const std::string& field_name,
      const int field_index, const int num_scalars,
      const CORE::FE::GaussIntegration& gauss_integration, Teuchos::ParameterList& params,
      const std::string& target_name)
  {
    dsassert(discretization.HasState(field_index, field_name),
        "Could not find the requested field in the discretization.");

    // the material expects the gp-quantities at the Gauss points in a rcp-std::vector
    auto quantity_at_gp = Teuchos::rcp(new std::vector<std::vector<double>>(
        gauss_integration.NumPoints(), std::vector<double>(num_scalars, 0.0)));

    // get quantitiy from discretization
    Teuchos::RCP<const Epetra_Vector> quantitites_np =
        discretization.GetState(field_index, field_name);
    if (quantitites_np == Teuchos::null)
      dserror("Cannot get state vector '%s' ", field_name.c_str());

    // extract my values
    auto my_quantities = std::vector<double>(la[field_index].lm_.size(), 0.0);
    DRT::UTILS::ExtractMyValues(*quantitites_np, my_quantities, la[field_index].lm_);

    // element vector for k-th scalar
    std::vector<CORE::LINALG::Matrix<CORE::FE::num_nodes<celltype>, 1>> element_quantity(
        num_scalars);
    for (int k = 0; k < num_scalars; ++k)
      for (int i = 0; i < CORE::FE::num_nodes<celltype>; ++i)
        (element_quantity[k])(i, 0) = my_quantities.at(num_scalars * i + k);

    DRT::ELEMENTS::ForEachGaussPoint(element_nodes, gauss_integration,
        [&](const CORE::LINALG::Matrix<CORE::FE::dim<celltype>, 1>& xi,
            const DRT::ELEMENTS::ShapeFunctionsAndDerivatives<celltype>& shape_functions,
            const DRT::ELEMENTS::JacobianMapping<celltype>& jacobian_mapping,
            double integration_factor, int gp)
        {
          // concentrations at current gauss point
          std::vector<double> conc_gp_k(num_scalars, 0.0);

          for (int k = 0; k < num_scalars; ++k)
          {
            // identical shapefunctions for displacements and temperatures
            conc_gp_k[k] = shape_functions.shapefunctions_.Dot(element_quantity[k]);
          }

          (*quantity_at_gp)[gp] = conc_gp_k;
        });

    params.set<Teuchos::RCP<std::vector<std::vector<double>>>>(target_name, quantity_at_gp);
  };

  template <CORE::FE::CellType celltype>
  void PrepareScatraQuantitiesInParameterList(const DRT::Element& element,
      const DRT::Discretization& discretization, const DRT::Element::LocationArray& la,
      const DRT::ELEMENTS::ElementNodes<celltype>& element_nodes,
      const CORE::FE::GaussIntegration& gauss_integration, Teuchos::ParameterList& params)
  {
    if (la.Size() > 1)
    {
      // prepare data from the scatra-field
      if (discretization.HasState(1, "scalarfield"))
      {
        const int num_scalars = discretization.NumDof(1, element.Nodes()[0]);
        PrepareScatraQuantityInParameterList(discretization, la, element_nodes, "scalarfield", 1,
            num_scalars, gauss_integration, params, "gp_conc");
      }

      // additionally prepare temperature-filed if available
      if (discretization.NumDofSets() == 3 && discretization.HasState(2, "tempfield"))
      {
        PrepareScatraQuantityInParameterList(discretization, la, element_nodes, "tempfield", 2, 1,
            gauss_integration, params, "gp_temp");
      }
    }
  }
}  // namespace

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::SolidScatraEleCalc()
    : stiffness_matrix_integration_(
          CreateGaussIntegration<celltype>(GetGaussRuleStiffnessMatrix<celltype>())),
      mass_matrix_integration_(CreateGaussIntegration<celltype>(GetGaussRuleMassMatrix<celltype>()))
{
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::Pack(CORE::COMM::PackBuffer& data) const
{
  SolidFormulation::Pack(history_data_, data);
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::Unpack(std::vector<char>::size_type& position, const std::vector<char>& data)
{
  SolidFormulation::Unpack(position, data, history_data_);
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::EvaluateNonlinearForceStiffnessMass(const DRT::Element& ele,
    MAT::So3Material& solid_material, const DRT::Discretization& discretization,
    const DRT::Element::LocationArray& la, Teuchos::ParameterList& params,
    CORE::LINALG::SerialDenseVector* force_vector,
    CORE::LINALG::SerialDenseMatrix* stiffness_matrix, CORE::LINALG::SerialDenseMatrix* mass_matrix)
{
  // Create views to SerialDenseMatrices
  std::optional<CORE::LINALG::Matrix<num_dof_per_ele_, num_dof_per_ele_>> stiff{};
  std::optional<CORE::LINALG::Matrix<num_dof_per_ele_, num_dof_per_ele_>> mass{};
  std::optional<CORE::LINALG::Matrix<num_dof_per_ele_, 1>> force{};
  if (stiffness_matrix != nullptr) stiff.emplace(*stiffness_matrix, true);
  if (mass_matrix != nullptr) mass.emplace(*mass_matrix, true);
  if (force_vector != nullptr) force.emplace(*force_vector, true);

  const ElementNodes<celltype> nodal_coordinates =
      EvaluateElementNodes<celltype>(ele, discretization, la[0].lm_);

  // prepare scatra data in the parameter list
  PrepareScatraQuantitiesInParameterList(
      ele, discretization, la, nodal_coordinates, stiffness_matrix_integration_, params);

  bool equal_integration_mass_stiffness =
      CompareGaussIntegration(mass_matrix_integration_, stiffness_matrix_integration_);

  EvaluateCentroidCoordinatesAndAddToParameterList(nodal_coordinates, params);

  const PreparationData preparation_data =
      SolidFormulation::Prepare(ele, nodal_coordinates, history_data_);

  double element_mass = 0.0;
  double element_volume = 0.0;
  ForEachGaussPoint(nodal_coordinates, stiffness_matrix_integration_,
      [&](const CORE::LINALG::Matrix<DETAIL::num_dim<celltype>, 1>& xi,
          const ShapeFunctionsAndDerivatives<celltype>& shape_functions,
          const JacobianMapping<celltype>& jacobian_mapping, double integration_factor, int gp)
      {
        EvaluateGPCoordinatesAndAddToParameterList(nodal_coordinates, shape_functions, params);
        SolidFormulation::Evaluate(ele, nodal_coordinates, xi, shape_functions, jacobian_mapping,
            preparation_data, history_data_,
            [&](const CORE::LINALG::Matrix<CORE::FE::dim<celltype>, CORE::FE::dim<celltype>>&
                    deformation_gradient,
                const CORE::LINALG::Matrix<num_str_, 1>& gl_strain, const auto& linearization)
            {
              const Stress<celltype> stress = EvaluateMaterialStress<celltype>(
                  solid_material, deformation_gradient, gl_strain, params, gp, ele.Id());

              if (force.has_value())
              {
                SolidFormulation::AddInternalForceVector(linearization, stress, integration_factor,
                    preparation_data, history_data_, *force);
              }

              if (stiff.has_value())
              {
                SolidFormulation::AddStiffnessMatrix(linearization, jacobian_mapping, stress,
                    integration_factor, preparation_data, history_data_, *stiff);
              }

              if (mass.has_value())
              {
                if (equal_integration_mass_stiffness)
                {
                  AddMassMatrix(
                      shape_functions, integration_factor, solid_material.Density(gp), *mass);
                }
                else
                {
                  element_mass += solid_material.Density(gp) * integration_factor;
                  element_volume += integration_factor;
                }
              }
            });
      });

  if (mass.has_value() && !equal_integration_mass_stiffness)
  {
    // integrate mass matrix
    dsassert(element_mass > 0, "It looks like the element mass is 0.0");
    ForEachGaussPoint<celltype>(nodal_coordinates, mass_matrix_integration_,
        [&](const CORE::LINALG::Matrix<CORE::FE::dim<celltype>, 1>& xi,
            const ShapeFunctionsAndDerivatives<celltype>& shape_functions,
            const JacobianMapping<celltype>& jacobian_mapping, double integration_factor, int gp) {
          AddMassMatrix(shape_functions, integration_factor, element_mass / element_volume, *mass);
        });
  }
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::EvaluateDStressDScalar(const DRT::Element& ele, MAT::So3Material& solid_material,
    const DRT::Discretization& discretization, const DRT::Element::LocationArray& la,
    Teuchos::ParameterList& params, CORE::LINALG::SerialDenseMatrix& stiffness_matrix_dScalar)
{
  const int scatra_column_stride = std::invoke(
      [&]()
      {
        if (params.isParameter("numscatradofspernode"))
        {
          return params.get<int>("numscatradofspernode");
        }
        return 1;
      });


  const ElementNodes<celltype> nodal_coordinates =
      EvaluateElementNodes<celltype>(ele, discretization, la[0].lm_);

  // prepare scatra data in the parameter list
  PrepareScatraQuantitiesInParameterList(
      ele, discretization, la, nodal_coordinates, stiffness_matrix_integration_, params);

  EvaluateCentroidCoordinatesAndAddToParameterList(nodal_coordinates, params);

  const PreparationData preparation_data =
      SolidFormulation::Prepare(ele, nodal_coordinates, history_data_);

  ForEachGaussPoint(nodal_coordinates, stiffness_matrix_integration_,
      [&](const CORE::LINALG::Matrix<DETAIL::num_dim<celltype>, 1>& xi,
          const ShapeFunctionsAndDerivatives<celltype>& shape_functions,
          const JacobianMapping<celltype>& jacobian_mapping, double integration_factor, int gp)
      {
        EvaluateGPCoordinatesAndAddToParameterList(nodal_coordinates, shape_functions, params);
        SolidFormulation::Evaluate(ele, nodal_coordinates, xi, shape_functions, jacobian_mapping,
            preparation_data, history_data_,
            [&](const CORE::LINALG::Matrix<CORE::FE::dim<celltype>, CORE::FE::dim<celltype>>&
                    deformation_gradient,
                const CORE::LINALG::Matrix<num_str_, 1>& gl_strain, const auto& linearization)
            {
              CORE::LINALG::Matrix<6, 1> dSdc = EvaluateDMaterialStressDScalar<celltype>(
                  solid_material, deformation_gradient, gl_strain, params, gp, ele.Id());

              // linear B-opeartor
              const CORE::LINALG::Matrix<DETAILS::num_str<celltype>,
                  CORE::FE::num_nodes<celltype> * CORE::FE::dim<celltype>>
                  bop = SolidFormulation::GetLinearBOperator(linearization);

              constexpr int num_dof_per_ele =
                  CORE::FE::dim<celltype> * CORE::FE::num_nodes<celltype>;

              // Assemble matrix
              // k_dS = B^T . dS/dc * detJ * N * w(gp)
              CORE::LINALG::Matrix<num_dof_per_ele, 1> BdSdc(true);
              BdSdc.MultiplyTN(integration_factor, bop, dSdc);

              // loop over rows
              for (int rowi = 0; rowi < num_dof_per_ele; ++rowi)
              {
                const double BdSdc_rowi = BdSdc(rowi, 0);
                // loop over columns
                for (int coli = 0; coli < CORE::FE::num_nodes<celltype>; ++coli)
                {
                  stiffness_matrix_dScalar(rowi, coli * scatra_column_stride) +=
                      BdSdc_rowi * shape_functions.shapefunctions_(coli, 0);
                }
              }
            });
      });
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::Recover(const DRT::Element& ele, const DRT::Discretization& discretization,
    const DRT::Element::LocationArray& la, Teuchos::ParameterList& params)
{
  // nothing needs to be done for simple displacement based elements
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::Update(const DRT::Element& ele, MAT::So3Material& solid_material,
    const DRT::Discretization& discretization, const DRT::Element::LocationArray& la,
    Teuchos::ParameterList& params)
{
  const ElementNodes<celltype> nodal_coordinates =
      EvaluateElementNodes<celltype>(ele, discretization, la[0].lm_);

  // prepare scatra data in the parameter list
  PrepareScatraQuantitiesInParameterList(
      ele, discretization, la, nodal_coordinates, stiffness_matrix_integration_, params);

  EvaluateCentroidCoordinatesAndAddToParameterList(nodal_coordinates, params);

  const PreparationData preparation_data =
      SolidFormulation::Prepare(ele, nodal_coordinates, history_data_);

  DRT::ELEMENTS::ForEachGaussPoint(nodal_coordinates, stiffness_matrix_integration_,
      [&](const CORE::LINALG::Matrix<DETAIL::num_dim<celltype>, 1>& xi,
          const ShapeFunctionsAndDerivatives<celltype>& shape_functions,
          const JacobianMapping<celltype>& jacobian_mapping, double integration_factor, int gp)
      {
        EvaluateGPCoordinatesAndAddToParameterList(nodal_coordinates, shape_functions, params);
        SolidFormulation::Evaluate(ele, nodal_coordinates, xi, shape_functions, jacobian_mapping,
            preparation_data, history_data_,
            [&](const CORE::LINALG::Matrix<CORE::FE::dim<celltype>, CORE::FE::dim<celltype>>&
                    deformation_gradient,
                const CORE::LINALG::Matrix<num_str_, 1>& gl_strain, const auto& linearization)
            { solid_material.Update(deformation_gradient, gp, params, ele.Id()); });
      });
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
double DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::CalculateInternalEnergy(const DRT::Element& ele, MAT::So3Material& solid_material,
    const DRT::Discretization& discretization, const DRT::Element::LocationArray& la,
    Teuchos::ParameterList& params)
{
  const ElementNodes<celltype> nodal_coordinates =
      EvaluateElementNodes<celltype>(ele, discretization, la[0].lm_);

  // prepare scatra data in the parameter list
  PrepareScatraQuantitiesInParameterList(
      ele, discretization, la, nodal_coordinates, stiffness_matrix_integration_, params);

  EvaluateCentroidCoordinatesAndAddToParameterList(nodal_coordinates, params);

  const PreparationData preparation_data =
      SolidFormulation::Prepare(ele, nodal_coordinates, history_data_);

  double intenergy = 0;
  DRT::ELEMENTS::ForEachGaussPoint(nodal_coordinates, stiffness_matrix_integration_,
      [&](const CORE::LINALG::Matrix<DETAIL::num_dim<celltype>, 1>& xi,
          const ShapeFunctionsAndDerivatives<celltype>& shape_functions,
          const JacobianMapping<celltype>& jacobian_mapping, double integration_factor, int gp)
      {
        EvaluateGPCoordinatesAndAddToParameterList(nodal_coordinates, shape_functions, params);
        SolidFormulation::Evaluate(ele, nodal_coordinates, xi, shape_functions, jacobian_mapping,
            preparation_data, history_data_,
            [&](const CORE::LINALG::Matrix<CORE::FE::dim<celltype>, CORE::FE::dim<celltype>>&
                    deformation_gradient,
                const CORE::LINALG::Matrix<num_str_, 1>& gl_strain, const auto& linearization)
            {
              double psi = 0.0;
              solid_material.StrainEnergy(gl_strain, psi, gp, ele.Id());
              intenergy += psi * integration_factor;
            });
      });

  return intenergy;
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::CalculateStress(const DRT::Element& ele, MAT::So3Material& solid_material,
    const StressIO& stressIO, const StrainIO& strainIO, const DRT::Discretization& discretization,
    const DRT::Element::LocationArray& la, Teuchos::ParameterList& params)
{
  std::vector<char>& serialized_stress_data = stressIO.mutable_data;
  std::vector<char>& serialized_strain_data = strainIO.mutable_data;
  CORE::LINALG::SerialDenseMatrix stress_data(stiffness_matrix_integration_.NumPoints(), num_str_);
  CORE::LINALG::SerialDenseMatrix strain_data(stiffness_matrix_integration_.NumPoints(), num_str_);

  const ElementNodes<celltype> nodal_coordinates =
      EvaluateElementNodes<celltype>(ele, discretization, la[0].lm_);

  // prepare scatra data in the parameter list
  PrepareScatraQuantitiesInParameterList(
      ele, discretization, la, nodal_coordinates, stiffness_matrix_integration_, params);

  EvaluateCentroidCoordinatesAndAddToParameterList(nodal_coordinates, params);

  const PreparationData preparation_data =
      SolidFormulation::Prepare(ele, nodal_coordinates, history_data_);

  DRT::ELEMENTS::ForEachGaussPoint(nodal_coordinates, stiffness_matrix_integration_,
      [&](const CORE::LINALG::Matrix<DETAIL::num_dim<celltype>, 1>& xi,
          const ShapeFunctionsAndDerivatives<celltype>& shape_functions,
          const JacobianMapping<celltype>& jacobian_mapping, double integration_factor, int gp)
      {
        EvaluateGPCoordinatesAndAddToParameterList(nodal_coordinates, shape_functions, params);
        SolidFormulation::Evaluate(ele, nodal_coordinates, xi, shape_functions, jacobian_mapping,
            preparation_data, history_data_,
            [&](const CORE::LINALG::Matrix<CORE::FE::dim<celltype>, CORE::FE::dim<celltype>>&
                    deformation_gradient,
                const CORE::LINALG::Matrix<num_str_, 1>& gl_strain, const auto& linearization)
            {
              const Stress<celltype> stress = EvaluateMaterialStress<celltype>(
                  solid_material, deformation_gradient, gl_strain, params, gp, ele.Id());

              AssembleStrainTypeToMatrixRow<celltype>(
                  gl_strain, deformation_gradient, strainIO.type, strain_data, gp);
              AssembleStressTypeToMatrixRow(
                  deformation_gradient, stress, stressIO.type, stress_data, gp);
            });
      });

  Serialize(stress_data, serialized_stress_data);
  Serialize(strain_data, serialized_strain_data);
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::Setup(MAT::So3Material& solid_material, INPUT::LineDefinition* linedef)
{
  solid_material.Setup(stiffness_matrix_integration_.NumPoints(), linedef);
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::MaterialPostSetup(const DRT::Element& ele, MAT::So3Material& solid_material)
{
  Teuchos::ParameterList params{};

  // Check if element has fiber nodes, if so interpolate fibers to Gauss Points and add to params
  InterpolateFibersToGaussPointsAndAddToParameterList<celltype>(
      stiffness_matrix_integration_, ele, params);

  // Call PostSetup of material
  solid_material.PostSetup(params, ele.Id());
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::InitializeGaussPointDataOutput(const DRT::Element& ele,
    const MAT::So3Material& solid_material,
    STR::MODELEVALUATOR::GaussPointDataOutputManager& gp_data_output_manager) const
{
  dsassert(ele.IsParamsInterface(),
      "This action type should only be called from the new time integration framework!");

  AskAndAddQuantitiesToGaussPointDataOutput(
      stiffness_matrix_integration_.NumPoints(), solid_material, gp_data_output_manager);
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::EvaluateGaussPointDataOutput(const DRT::Element& ele,
    const MAT::So3Material& solid_material,
    STR::MODELEVALUATOR::GaussPointDataOutputManager& gp_data_output_manager) const
{
  dsassert(ele.IsParamsInterface(),
      "This action type should only be called from the new time integration framework!");

  CollectAndAssembleGaussPointDataOutput<celltype>(
      stiffness_matrix_integration_, solid_material, ele, gp_data_output_manager);
}

template <CORE::FE::CellType celltype, typename SolidFormulation, typename PreparationData,
    typename HistoryData>
void DRT::ELEMENTS::SolidScatraEleCalc<celltype, SolidFormulation, PreparationData,
    HistoryData>::ResetToLastConverged(const DRT::Element& ele, MAT::So3Material& solid_material)
{
  solid_material.ResetStep();
}

template <CORE::FE::CellType... celltypes>
struct VerifyPackable
{
  static constexpr bool are_all_packable =
      (DRT::ELEMENTS::IsPackable<DRT::ELEMENTS::SolidScatraEleCalc<celltypes,
              DRT::ELEMENTS::DisplacementBasedFormulation<celltypes>,
              DRT::ELEMENTS::DisplacementBasedPreparationData,
              DRT::ELEMENTS::DisplacementBasedHistoryData>*> &&
          ...);

  static constexpr bool are_all_unpackable =
      (DRT::ELEMENTS::IsUnpackable<DRT::ELEMENTS::SolidScatraEleCalc<celltypes,
              DRT::ELEMENTS::DisplacementBasedFormulation<celltypes>,
              DRT::ELEMENTS::DisplacementBasedPreparationData,
              DRT::ELEMENTS::DisplacementBasedHistoryData>*> &&
          ...);

  void StaticAsserts() const
  {
    static_assert(are_all_packable);
    static_assert(are_all_unpackable);
  }
};

template struct VerifyPackable<CORE::FE::CellType::hex8, CORE::FE::CellType::hex27,
    CORE::FE::CellType::tet4, CORE::FE::CellType::tet10>;

// explicit instantiations of template classes
// for displacement based formulation
template class DRT::ELEMENTS::SolidScatraEleCalc<CORE::FE::CellType::hex8,
    DRT::ELEMENTS::DisplacementBasedFormulation<CORE::FE::CellType::hex8>,
    DRT::ELEMENTS::DisplacementBasedPreparationData, DRT::ELEMENTS::DisplacementBasedHistoryData>;
template class DRT::ELEMENTS::SolidScatraEleCalc<CORE::FE::CellType::hex27,
    DRT::ELEMENTS::DisplacementBasedFormulation<CORE::FE::CellType::hex27>,
    DRT::ELEMENTS::DisplacementBasedPreparationData, DRT::ELEMENTS::DisplacementBasedHistoryData>;
template class DRT::ELEMENTS::SolidScatraEleCalc<CORE::FE::CellType::tet4,
    DRT::ELEMENTS::DisplacementBasedFormulation<CORE::FE::CellType::tet4>,
    DRT::ELEMENTS::DisplacementBasedPreparationData, DRT::ELEMENTS::DisplacementBasedHistoryData>;
template class DRT::ELEMENTS::SolidScatraEleCalc<CORE::FE::CellType::tet10,
    DRT::ELEMENTS::DisplacementBasedFormulation<CORE::FE::CellType::tet10>,
    DRT::ELEMENTS::DisplacementBasedPreparationData, DRT::ELEMENTS::DisplacementBasedHistoryData>;

BACI_NAMESPACE_CLOSE