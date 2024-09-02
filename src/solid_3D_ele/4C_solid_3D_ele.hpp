/*! \file

\brief Declaration of the solid element

This file contains the element-specific service routines such as
Pack, Unpack, NumDofPerNode etc.

\level 1
*/

#ifndef FOUR_C_SOLID_3D_ELE_HPP
#define FOUR_C_SOLID_3D_ELE_HPP

#include "4C_config.hpp"

#include "4C_fem_general_element.hpp"
#include "4C_fem_general_elementtype.hpp"
#include "4C_inpar_structure.hpp"
#include "4C_linalg_serialdensematrix.hpp"
#include "4C_solid_3D_ele_calc_eas.hpp"
#include "4C_solid_3D_ele_calc_lib_nitsche.hpp"
#include "4C_solid_3D_ele_factory.hpp"
#include "4C_structure_new_elements_paramsinterface.hpp"

#include <memory>

FOUR_C_NAMESPACE_OPEN

namespace Mat
{
  class So3Material;
}
namespace Discret::ELEMENTS
{
  // forward declaration
  class SolidEleCalcInterface;

  class SolidType : public Core::Elements::ElementType
  {
   public:
    void setup_element_definition(
        std::map<std::string, std::map<std::string, Input::LineDefinition>>& definitions) override;

    Teuchos::RCP<Core::Elements::Element> create(const std::string eletype,
        const std::string elecelltype, const int id, const int owner) override;

    Teuchos::RCP<Core::Elements::Element> create(const int id, const int owner) override;

    Core::Communication::ParObject* create(Core::Communication::UnpackBuffer& buffer) override;

    [[nodiscard]] std::string name() const override { return "SolidType"; }

    void nodal_block_information(
        Core::Elements::Element* dwele, int& numdf, int& dimns, int& nv, int& np) override;

    Core::LinAlg::SerialDenseMatrix compute_null_space(
        Core::Nodes::Node& node, const double* x0, const int numdof, const int dimnsp) override;

    static SolidType& instance();

   private:
    static SolidType instance_;

  };  // class SolidType

  class Solid : public Core::Elements::Element
  {
    friend class SolidType;

   public:
    //! @name Constructors and destructors and related methods
    //!@{

    /*!
    \brief Standard Constructor

    \param id    (in): A globally unique element id
    \param owner (in): owner processor of the element
    */
    Solid(int id, int owner);

    //!@}

    [[nodiscard]] Core::Elements::Element* clone() const override;

    [[nodiscard]] int unique_par_object_id() const override
    {
      return SolidType::instance().unique_par_object_id();
    };

    void pack(Core::Communication::PackBuffer& data) const override;

    void unpack(Core::Communication::UnpackBuffer& buffer) override;

    [[nodiscard]] Core::Elements::ElementType& element_type() const override
    {
      return SolidType::instance();
    }

    [[nodiscard]] Core::FE::CellType shape() const override { return celltype_; };

    void set_kinematic_type(Inpar::Solid::KinemType kintype)
    {
      solid_ele_property_.kintype = kintype;
    }

    [[nodiscard]] virtual Teuchos::RCP<Mat::So3Material> solid_material(int nummat = 0) const;

    [[nodiscard]] int num_line() const override;

    [[nodiscard]] int num_surface() const override;

    [[nodiscard]] int num_volume() const override;

    std::vector<Teuchos::RCP<Core::Elements::Element>> lines() override;

    std::vector<Teuchos::RCP<Core::Elements::Element>> surfaces() override;

    [[nodiscard]] int num_dof_per_node(const Core::Nodes::Node& node) const override { return 3; }

    [[nodiscard]] int num_dof_per_element() const override { return 0; }

    bool read_element(const std::string& eletype, const std::string& celltype,
        const Core::IO::InputParameterContainer& container) override;

    int evaluate(Teuchos::ParameterList& params, Core::FE::Discretization& discretization,
        std::vector<int>& lm, Core::LinAlg::SerialDenseMatrix& elemat1,
        Core::LinAlg::SerialDenseMatrix& elemat2, Core::LinAlg::SerialDenseVector& elevec1,
        Core::LinAlg::SerialDenseVector& elevec2,
        Core::LinAlg::SerialDenseVector& elevec3) override;

    int evaluate_neumann(Teuchos::ParameterList& params, Core::FE::Discretization& discretization,
        Core::Conditions::Condition& condition, std::vector<int>& lm,
        Core::LinAlg::SerialDenseVector& elevec1,
        Core::LinAlg::SerialDenseMatrix* elemat1 = nullptr) override;

    Teuchos::RCP<Core::Elements::ParamsInterface> params_interface_ptr() override
    {
      return interface_ptr_;
    }

    [[nodiscard]] inline bool is_solid_params_interface() const
    {
      return (not interface_ptr_.is_null());
    }


    [[nodiscard]] inline bool is_params_interface() const override
    {
      return (not interface_ptr_.is_null());
    }

    [[nodiscard]] inline FourC::Solid::ELEMENTS::ParamsInterface& params_interface() const
    {
      if (not is_params_interface()) FOUR_C_THROW("The interface ptr is not set!");
      return *interface_ptr_;
    }

    [[nodiscard]] inline FourC::Solid::ELEMENTS::ParamsInterface& get_solid_params_interface() const
    {
      return params_interface();
    }


    void set_params_interface_ptr(const Teuchos::ParameterList& p) override;

    [[nodiscard]] bool have_eas() const
    {
      return solid_ele_property_.element_technology == ElementTechnology::eas_full ||
             solid_ele_property_.element_technology == ElementTechnology::eas_mild ||
             solid_ele_property_.element_technology == ElementTechnology::shell_eas;
    }

    void vis_names(std::map<std::string, int>& names) override;

    void set_integration_rule(const Core::FE::GaussIntegration& integration_rule);

    bool vis_data(const std::string& name, std::vector<double>& data) override;

    /*!
     * @brief Evaluate the Cauchy stress at @p xi with the normal vector @p n in the direction @p
     * dir and compute the linearizations w.r.t. all input parameters (disp, xi, n and dir)
     *
     * @tparam dim (in) : Dimension of the element
     * @param disp (in) : Nodal displacements of the element
     * @param xi (in) : Position of the point in the parameter space of the element
     * @param n (in) : Normal vector
     * @param dir (in) : Direction of the Cauchy stress
     * @param linearizations (in/out) : A struct containing the needed linearizations (which might
     * are null)
     * @return double (out) : The scalar Cauchy stress value
     */
    template <int dim>
    double get_normal_cauchy_stress_at_xi(const std::vector<double>& disp,
        const Core::LinAlg::Matrix<dim, 1>& xi, const Core::LinAlg::Matrix<dim, 1>& n,
        const Core::LinAlg::Matrix<dim, 1>& dir, CauchyNDirLinearizations<dim>& linearizations);

    void for_each_gauss_point(Core::FE::Discretization& discretization, std::vector<int>& lm,
        const std::function<void(Mat::So3Material&, double integration_factor, int gp)>& integrator)
        const;

   private:
    //! cell type
    Core::FE::CellType celltype_ = Core::FE::CellType::dis_none;

    //! solid element properties
    SolidElementProperties solid_ele_property_{};

    //! interface pointer for data exchange between the element and the time integrator.
    Teuchos::RCP<FourC::Solid::ELEMENTS::ParamsInterface> interface_ptr_;

    //! element calculation holding one of the implemented variants
    SolidCalcVariant solid_calc_variant_;

    //! flag, whether the post setup of materials is already called
    bool material_post_setup_ = false;

  };  // class Solid

}  // namespace Discret::ELEMENTS

FOUR_C_NAMESPACE_CLOSE

#endif
