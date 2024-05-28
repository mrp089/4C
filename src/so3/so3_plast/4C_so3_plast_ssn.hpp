/*----------------------------------------------------------------------*/
/*! \file
\level 2
*/


/*----------------------------------------------------------------------*
 | definitions                                              seitz 07/13 |
 *----------------------------------------------------------------------*/
#ifndef FOUR_C_SO3_PLAST_SSN_HPP
#define FOUR_C_SO3_PLAST_SSN_HPP

/*----------------------------------------------------------------------*
 | headers                                                  seitz 07/13 |
 *----------------------------------------------------------------------*/
#include "4C_config.hpp"

#include "4C_discretization_fem_general_utils_gausspoints.hpp"
#include "4C_discretization_fem_general_utils_nurbs_shapefunctions.hpp"
#include "4C_linalg_serialdensematrix.hpp"
#include "4C_linalg_serialdensevector.hpp"
#include "4C_so3_plast_ssn_eletypes.hpp"
#include "4C_thermo_ele_impl_utils.hpp"

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*
 |                                                          seitz 07/13 |
 *----------------------------------------------------------------------*/
namespace DRT
{
  class Discretization;

  namespace UTILS
  {
    struct PlastSsnData;
  }

  namespace ELEMENTS
  {
    /*!
     * \brief EAS technology enhancement types of so_hex8
     *
     * Solid Hex8 has EAS enhancement of GL-strains to avoid locking.
     * New enum for so3_plast. Currently only for hex8
     */
    enum So3PlastEasType  // with meaningful value for matrix size info
    {
      soh8p_easnone,   //!< no EAS i.e. displacement based with tremendous locking
      soh8p_eassosh8,  //!< related to Solid-Shell, 7 parameters to alleviate
                       //!< in-plane (membrane) locking and main modes for Poisson-locking
      soh8p_easmild,   //!< 9 parameters consisting of modes to alleviate
                       //!< shear locking (bending) and main incompressibility modes
                       //!< (for Solid Hex8)
                       //!< The sosh18 also uses 9 eas parameters. Therefore, we can
                       //!< re-use this EASType here.
      soh8p_easfull,   //!< 21 parameters to prevent almost all locking modes.
                       //!< Equivalent to all 30 parameters to fully complete element
                       //!< with quadratic modes (see Andelfinger 1993 for details) and therefore
                       //!< also suitable for distorted elements.
                       //!< (for Solid Hex8)
      soh18p_eassosh18
    };

    //! A C++ version of a 3 dimensional solid element with modifications von Mises
    //! plasticity using a semi-smooth Newton method
    template <CORE::FE::CellType distype>
    class So3Plast : public virtual SoBase
    {
      //! @name Friends
      friend class SoHex8PlastType;
      friend class SoHex27PlastType;

     public:
      //@}
      //! @name Constructors and destructors and related methods


      //! Standard Constructor
      So3Plast(int id,  //!< (i) this element's global id
          int owner     //!< elements owner
      );

      //! Copy Constructor
      //! Makes a deep copy of a Element
      So3Plast(const So3Plast& old);

      //! Deep copy this instance of Solid3 and return pointer to the copy
      //!
      //! The Clone() method is used from the virtual base class Element in cases
      //! where the type of the derived class is unknown and a copy-ctor is needed
      DRT::Element* Clone() const override;

      //@}

      //! number of element nodes
      static constexpr int nen_ = CORE::FE::num_nodes<distype>;
      //! number of space dimensions
      static constexpr int nsd_ = 3;
      //! number of dofs per node
      static constexpr int numdofpernode_ = 3;
      //! total dofs per element
      static constexpr int numdofperelement_ = numdofpernode_ * nen_;
      //! number of strains/stresses
      static constexpr int numstr_ = 6;
      //! number of Gauss points per element (value is added in so3_thermo.cpp)
      int numgpt_{};
      //! static const is required for fixedsizematrices
      //! TODO maybe more beauty is possible
      static constexpr int numgpt_post = THR::DisTypeToSTRNumGaussPoints<distype>::nquad;


      //! @name Acess methods

      //! Return unique ParObject id
      //!
      //! every class implementing ParObject needs a unique id defined at the top of
      //! this file.
      int UniqueParObjectId() const override;

      bool HaveEAS() const override { return (eastype_ != soh8p_easnone); };

      //! Pack this class so it can be communicated
      //! Pack and \ref Unpack are used to communicate this element
      void Pack(CORE::COMM::PackBuffer& data) const override;

      //! Unpack data from a char vector into this class
      //! Pack and \ref Unpack are used to communicate this element
      void Unpack(const std::vector<char>& data) override;

      //! Get number of degrees of freedom of a certain node in case of multiple
      //! dofsets (implements pure virtual DRT::Element)
      //!
      //! The element decides how many degrees of freedom its nodes must have.
      int NumDofPerNode(const DRT::Node& node) const override { return nsd_; };

      //! Get number of degrees of freedom of this element
      int num_dof_per_element() const override { return 0; };

      //@}

      //! @name Access methods

      //! Print this element
      void Print(std::ostream& os) const override;

      //! return elementtype
      DRT::ElementType& ElementType() const override;

      //! return element shape
      CORE::FE::CellType Shape() const override { return distype; };

      /*!
      \brief Return number of volumes of this element
      */
      int NumVolume() const override;

      /*!
      \brief Return number of surfaces of this element
      */
      int NumSurface() const override;

      /*!
      \brief Return number of lines of this element
      */
      int NumLine() const override;

      /*!
      \brief Get vector of Teuchos::RCPs to the lines of this element

      */
      std::vector<Teuchos::RCP<DRT::Element>> Lines() override;

      /*!
      \brief Get vector of Teuchos::RCPs to the surfaces of this element

      */
      std::vector<Teuchos::RCP<DRT::Element>> Surfaces() override;

      //@}

      //! @name Input and Creation

      //! Query names of element data to be visualized using BINIO
      //!
      //! The element fills the provided map with key names of
      //! visualization data the element wants to visualize AT THE CENTER
      //! of the element geometry. The values is supposed to be dimension of the
      //! data to be visualized. It can either be 1 (scalar), 3 (vector), 6 (sym. tensor)
      //! or 9 (nonsym. tensor)
      //!
      //! Example:
      //! \code
      //!   // Name of data is 'Owner', dimension is 1 (scalar value)
      //!   names.insert(std::pair<string,int>("Owner",1));
      //!   // Name of data is 'StressesXYZ', dimension is 6 (sym. tensor value)
      //!   names.insert(std::pair<string,int>("StressesXYZ",6));
      //! \endcode
      //!
      //!  names (out): On return, the derived class has filled names with key
      //!               names of data it wants to visualize and with int dimensions
      //!               of that data.
      void VisNames(std::map<std::string, int>& names) override;

      //!  Query data to be visualized using BINIO of a given name
      //!
      //! The method is supposed to call this base method to visualize the owner of
      //! the element.
      //! If the derived method recognizes a supported data name, it shall fill it
      //! with corresponding data.
      //! If it does NOT recognizes the name, it shall do nothing.
      //!
      //! warning: the method must not change size of data
      //!
      //!  name (in):   Name of data that is currently processed for visualization
      //! \param data (out):  data to be filled by element if element recognizes the name
      bool VisData(const std::string& name, std::vector<double>& data) override;

      //! read input for this element
      bool ReadElement(const std::string& eletype,  //!< so3plast(fbar)
          const std::string& eledistype,            //!< hex8,tet4,...
          INPUT::LineDefinition* linedef            //!< what parameters have to be read
          ) override;

      //@}

      //! @name Evaluation

      //! evaluate an element
      //! evaluate element stiffness, mass, internal forces, etc.
      //!
      //! if nullptr on input, the controlling method does not expect the element
      //!  to fill these matrices or vectors.
      //!
      //!  \return 0 if successful, negative otherwise
      int Evaluate(
          Teuchos::ParameterList&
              params,  //!< ParameterList for communication between control routine and elements
          DRT::Discretization& discretization,  //!< pointer to discretization for de-assembly
          DRT::Element::LocationArray& la,      //!< location array for de-assembly
          CORE::LINALG::SerialDenseMatrix&
              elemat1_epetra,  //!< (stiffness-)matrix to be filled by element.
          CORE::LINALG::SerialDenseMatrix&
              elemat2_epetra,  //!< (mass-)matrix to be filled by element.
          CORE::LINALG::SerialDenseVector&
              elevec1_epetra,  //!< (internal force-)vector to be filled by element
          CORE::LINALG::SerialDenseVector& elevec2_epetra,  //!< vector to be filled by element
          CORE::LINALG::SerialDenseVector& elevec3_epetra   //!< vector to be filled by element
          ) override;

      //@}

      /*!
      \brief Evaluate a Neumann boundary condition

      this method evaluates a surface Neumann condition on the solid3 element

      \param params (in/out)    : ParameterList for communication between control routine
                                  and elements
      \param discretization (in): A reference to the underlying discretization
      \param condition (in)     : The condition to be evaluated
      \param lm (in)            : location vector of this element
      \param elevec1 (out)      : vector to be filled by element. If nullptr on input,

      \return 0 if successful, negative otherwise
      */
      int evaluate_neumann(Teuchos::ParameterList& params, DRT::Discretization& discretization,
          CORE::Conditions::Condition& condition, std::vector<int>& lm,
          CORE::LINALG::SerialDenseVector& elevec1,
          CORE::LINALG::SerialDenseMatrix* elemat1 = nullptr) override;

      //! init the inverse of the jacobian and its determinant in the material
      //! configuration
      virtual void init_jacobian_mapping();

      // get parameter list from ssn_plast_manager
      virtual void ReadParameterList(Teuchos::RCP<Teuchos::ParameterList> plparams);

      void get_cauchy_n_dir_and_derivatives_at_xi(const CORE::LINALG::Matrix<3, 1>& xi,
          const std::vector<double>& disp, const CORE::LINALG::Matrix<3, 1>& n,
          const CORE::LINALG::Matrix<3, 1>& dir, double& cauchy_n_dir,
          CORE::LINALG::SerialDenseMatrix* d_cauchyndir_dd,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd2,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dn,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_ddir,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dxi,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_dn,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_ddir,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_dxi, const std::vector<double>* temp,
          CORE::LINALG::SerialDenseMatrix* d_cauchyndir_dT,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dT, const double* concentration,
          double* d_cauchyndir_dc) override;

      void HeatFlux(const std::vector<double>& temperature, const std::vector<double>& disp,
          const CORE::LINALG::Matrix<nsd_, 1>& xi, const CORE::LINALG::Matrix<nsd_, 1>& n,
          double& q, CORE::LINALG::SerialDenseMatrix* dq_dT, CORE::LINALG::SerialDenseMatrix* dq_dd,
          CORE::LINALG::Matrix<nsd_, 1>* dq_dn, CORE::LINALG::Matrix<nsd_, 1>* dq_dpxi,
          CORE::LINALG::SerialDenseMatrix* d2q_dT_dd, CORE::LINALG::SerialDenseMatrix* d2q_dT_dn,
          CORE::LINALG::SerialDenseMatrix* d2q_dT_dpxi);

      void HeatFlux(const std::vector<double>& temp, const std::vector<double>& disp,
          const CORE::LINALG::Matrix<2, 1>& xi, const CORE::LINALG::Matrix<2, 1>& n, double& q,
          CORE::LINALG::SerialDenseMatrix* dq_dT, CORE::LINALG::SerialDenseMatrix* dq_dd,
          CORE::LINALG::Matrix<2, 1>* dq_dn, CORE::LINALG::Matrix<2, 1>* dq_dpxi,
          CORE::LINALG::SerialDenseMatrix* d2q_dT_dd, CORE::LINALG::SerialDenseMatrix* d2q_dT_dn,
          CORE::LINALG::SerialDenseMatrix* d2q_dT_dpxi)
      {
        FOUR_C_THROW("wrong spatial dimension");
      }


      virtual void set_is_nitsche_contact_ele(bool val)
      {
        is_nitsche_contact_ = val;
        if (is_nitsche_contact_)
        {
          cauchy_.resize(numgpt_);
          cauchy_deriv_.resize(numgpt_);
          if (tsi_) cauchy_deriv_T_.resize(numgpt_);
        }
      }
      //@}

     protected:
      //! don't want = operator
      So3Plast& operator=(const So3Plast& old) = delete;

      //! number of plastic variables at each gauss point
      enum PlSpinType
      {
        zerospin = 5,  //!< 5 parameters for zero plastic spin (symmetric traceless tensor)
        plspin = 8,    //!< 8 parameters for zero plastic spin (non-symmetric traceless tensor)
      };

      //! vector of coordinates of current integration point in reference coordinates
      std::vector<CORE::LINALG::Matrix<nsd_, 1>> xsi_;
      //! Gauss point weights
      std::vector<double> wgt_;

      //! @name plasticity related stuff


      //! Calculate nonlinear stiffness and mass matrix with condensed plastic matrices
      virtual void nln_stiffmass(std::vector<double>& disp,  // current displacements
          std::vector<double>& vel,                          // current velocities
          std::vector<double>& temperature,                  // current temperatures
          CORE::LINALG::Matrix<numdofperelement_, numdofperelement_>*
              stiffmatrix,  // element stiffness matrix
          CORE::LINALG::Matrix<numdofperelement_, numdofperelement_>*
              massmatrix,                                         // element mass matrix
          CORE::LINALG::Matrix<numdofperelement_, 1>* force,      // element internal force vector
          CORE::LINALG::Matrix<numgpt_post, numstr_>* elestress,  // stresses at GP
          CORE::LINALG::Matrix<numgpt_post, numstr_>* elestrain,  // strains at GP
          Teuchos::ParameterList& params,         // algorithmic parameters e.g. time
          const INPAR::STR::StressType iostress,  // stress output option
          const INPAR::STR::StrainType iostrain   // strain output option
      );

      //! Calculate the coupling matrix K_dT for monolithic TSI
      virtual void nln_kd_t_tsi(CORE::LINALG::Matrix<numdofperelement_, nen_>*
                                    k_dT,  //!< (o): mechanical thermal stiffness term at current gp
          Teuchos::ParameterList& params);

      // Add plastic increment of converged state to plastic history for nonlinear kinematics
      virtual void update_plastic_deformation_nln(PlSpinType spintype);

      //@}

      //! calculate nonlinear B-operator
      virtual void calculate_bop(CORE::LINALG::Matrix<numstr_, numdofperelement_>* bop,
          const CORE::LINALG::Matrix<nsd_, nsd_>* defgrd,
          const CORE::LINALG::Matrix<nsd_, nen_>* N_XYZ, const int gp);

      /// Extrapolate Gauss-point data to nodes and store results in elevectors
      /// todo: Unfortunately, there is no universal extrapolation function in all base elements,
      /// i.e. the hex8 elements would call soh8_expol, hex27 would call soh27_expol...
      /// For now, we copy the extrapolation function here and allow the use only for hex8 elements
      template <unsigned int num_cols>
      void soh8_expol(CORE::LINALG::Matrix<numgpt_post, num_cols>& data,  ///< gp data
          Epetra_MultiVector& expolData);                                 ///< nodal data

      // EAS element techonolgy *******************************************
      //! Initialize data for EAS (once)
      virtual void eas_init();

      //! setup EAS for each evaluation
      virtual void eas_setup();

      // evaluate EAS shape functions
      virtual void eas_shape(const int gp);

      //! add eas strains to GL strains
      virtual void eas_enhance_strains();


      //! @name plasticity related stuff

      /*! \brief Calculate the deformation gradient that is consistent
       *         with modified (e.g. EAS) GL strain tensor.
       *         Expensive (two polar decomposition), but required, if
       *         the material evaluation is based on the deformation
       *         gradient rather than the GL strain tensor (e.g. plasticity).
       *
       * \param defgrd_disp  (in)  : displacement-based deformation gradient
       * \param glstrain_mod (in)  : modified GL strain tensor (strain-like Voigt notation)
       * \param defgrd_mod   (out) : consistent modified deformation gradient
       */
      virtual void calc_consistent_defgrd();


      /*! \brief Evaluate the NCP function and the linearization
       *         and condense the additional degrees of freedom
       *         into the stiffness matrix block.
       *         Required data from the element evaluate is handed in,
       *         with potential EAS matrices or F-bar linearizations.
       *         The template argument "spinype" decides, if
       *         the additional evolution equation for the plastic
       *         spin is linearized and solved for.
       */
      template <int spintype>
      void condense_plasticity(const CORE::LINALG::Matrix<nsd_, nsd_>& defgrd,
          const CORE::LINALG::Matrix<nsd_, nsd_>& deltaLp,
          const CORE::LINALG::Matrix<numstr_, numdofperelement_>& bop,
          const CORE::LINALG::Matrix<nsd_, nen_>* N_XYZ,
          const CORE::LINALG::Matrix<numstr_, 1>* RCG, const double detJ_w, const int gp,
          const double temp, Teuchos::ParameterList& params,
          CORE::LINALG::Matrix<numdofperelement_, 1>* force,
          CORE::LINALG::Matrix<numdofperelement_, numdofperelement_>* stiffmatrix,
          const CORE::LINALG::SerialDenseMatrix* M = nullptr,
          CORE::LINALG::SerialDenseMatrix* Kda = nullptr,
          std::vector<CORE::LINALG::SerialDenseVector>* dHda = nullptr,
          const double* f_bar_factor = nullptr,
          const CORE::LINALG::Matrix<numdofperelement_, 1>* htensor = nullptr);

      void recover_plasticity_and_eas(const CORE::LINALG::Matrix<numdofperelement_, 1>* res_d,
          const CORE::LINALG::Matrix<nen_, 1>* res_t = nullptr);

      void recover_eas(const CORE::LINALG::Matrix<numdofperelement_, 1>* res_d,
          const CORE::LINALG::Matrix<nen_, 1>* res_T = nullptr);

      template <int spintype>
      void recover_plasticity(const CORE::LINALG::Matrix<numdofperelement_, 1>* res_d, const int gp,
          const double* res_t = nullptr);

      void reduce_eas_step(const double new_step_length, const double old_step_length);
      void reduce_plasticity_step(
          const double new_step_length, const double old_step_length, const int gp);

      virtual void build_delta_lp(const int gp);

      /*! \brief Return if plastic spin is solved for.
       */
      virtual bool have_plastic_spin();


      //! calculate internal elastic energy
      double calc_int_energy(
          std::vector<double>& disp, std::vector<double>& temp, Teuchos::ParameterList& params);

      //@}

      /*!
       * @brief Evaluate Cauchy stress contracted with the normal vector and another direction
       * vector at given point in parameter space and calculate linearizations
       *
       * @param[in] xi            position in parameter space xi
       * @param[in] disp          vector of displacements
       * @param[in] n             normal vector (\f[\mathbf{n}\f])
       * @param[in] dir           direction vector (\f[\mathbf{v}\f]), can be either normal or
       *                          tangential vector
       * @param[out] cauchy_n_dir  cauchy stress tensor contracted using the vectors n and dir
       *    (\f[ \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v} \f])
       * @param[out] d_cauchyndir_dd  derivative of cauchy_n_dir w.r.t. displacements
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{d}} \f])
       * @param[out] d2_cauchyndir_dd2  second derivative of cauchy_n_dir w.r.t. displacements
       *    (\f[ \frac{ \mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{d}^2 } \f])
       * @param[out] d2_cauchyndir_dd_dn  second derivative of cauchy_n_dir w.r.t. displacements and
       *                                  vector n
       *    (\f[ \frac{\mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} {\mathrm{d}
       *    \mathbf{d} \mathrm{d} \mathbf{n} } \f])
       * @param[out] d2_cauchyndir_dd_ddir  second derivative of cauchy_n_dir w.r.t. displacements
       *                                    and direction vector v
       *    (\f[ \frac{\mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} {\mathrm{d}
       *    \mathbf{d} \mathrm{d} \mathbf{v} } \f])
       * @param[out] d2_cauchyndir_dd_dxi  second derivative of cauchy_n_dir w.r.t. displacements
       *                                   and local parameter coordinate xi
       *    (\f[ \frac{\mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} {\mathrm{d}
       *    \mathbf{d} \mathrm{d} \mathbf{\xi} } \f])
       * @param[out] d_cauchyndir_dn   derivative of cauchy_n_dir w.r.t. vector n
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{n}} \f])
       * @param[out] d_cauchyndir_ddir  derivative of cauchy_n_dir w.r.t. direction vector v
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{v}} \f])
       * @param[out] d_cauchyndir_dxi  derivative of cauchy_n_dir w.r.t. local parameter coord. xi
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{\xi}} \f])
       * @param[in] temp                temperature
       * @param[out] d_cauchyndir_dT    derivative of cauchy_n_dir w.r.t. temperature
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    T} \f])
       * @param[out] d2_cauchyndir_dd_dT  second derivative of cauchy_n_dir w.r.t. displacements
       *                                   and temperature
       *    (\f[ \frac{\mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} {\mathrm{d}
       *    \mathbf{d} \mathrm{d} T } \f])
       *
       * @note At the moment this method is only used for the nitsche contact formulation
       */
      virtual void get_cauchy_n_dir_and_derivatives_at_xi_plast(
          const CORE::LINALG::Matrix<3, 1>& xi, const std::vector<double>& disp,
          const CORE::LINALG::Matrix<3, 1>& n, const CORE::LINALG::Matrix<3, 1>& dir,
          double& cauchy_n_dir, CORE::LINALG::SerialDenseMatrix* d_cauchyndir_dd,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd2,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dn,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_ddir,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dxi,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_dn,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_ddir,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_dxi, const std::vector<double>* temp,
          CORE::LINALG::SerialDenseMatrix* d_cauchyndir_dT,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dT);

      /*!
       * @brief Evaluate Cauchy stress contracted with the normal vector and another direction
       * vector at given point in parameter space and calculate linearizations
       *
       * @param[in] xi            position in parameter space xi
       * @param[in] disp          vector of displacements
       * @param[in] n             normal vector (\f[\mathbf{n}\f])
       * @param[in] dir           direction vector (\f[\mathbf{v}\f]), can be either normal or
       *                          tangential vector
       * @param[out] cauchy_n_dir  cauchy stress tensor contracted using the vectors n and dir
       *                           (\f[ \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v} \f])
       * @param[out] d_cauchyndir_dd    derivative of cauchy_n_dir w.r.t. displacements
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{d}} \f])
       * @param[out] d2_cauchyndir_dd2  second derivative of cauchy_n_dir w.r.t. displacements
       *    (\f[ \frac{ \mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{d}^2 } \f])
       * @param[out] d2_cauchyndir_dd_dn  second derivative of cauchy_n_dir w.r.t. displacements and
       *                                  vector n
       *    (\f[ \frac{\mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} {\mathrm{d}
       *    \mathbf{d} \mathrm{d} \mathbf{n} } \f])
       * @param[out] d2_cauchyndir_dd_ddir  second derivative of cauchy_n_dir w.r.t. displacements
       *                                    and direction vector v
       *    (\f[ \frac{\mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} {\mathrm{d}
       *    \mathbf{d} \mathrm{d} \mathbf{v} } \f])
       * @param[out] d2_cauchyndir_dd_dxi  second derivative of cauchy_n_dir w.r.t. displacements
       *                                   and local parameter coordinate xi
       *    (\f[ \frac{\mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} {\mathrm{d}
       *    \mathbf{d} \mathrm{d} \mathbf{\xi} } \f])
       * @param[out] d_cauchyndir_dn   derivative of cauchy_n_dir w.r.t. vector n
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{n}} \f])
       * @param[out] d_cauchyndir_ddir  derivative of cauchy_n_dir w.r.t. direction vector v
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{v}} \f])
       * @param[out] d_cauchyndir_dxi  derivative of cauchy_n_dir w.r.t. local parameter coord. xi
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    \mathbf{\xi}} \f])
       * @param[in] temp                temperature
       * @param[out] d_cauchyndir_dT    derivative of cauchy_n_dir w.r.t. temperature
       *    (\f[ \frac{ \mathrm{d} \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}} { \mathrm{d}
       *    T} \f])
       * @param[out] d2_cauchyndir_dd_dT  second derivative of cauchy_n_dir w.r.t. displacements
       *                                   and temperature
       *    (\f[ \frac{\mathrm{d}^2 \mathbf{\sigma} \cdot \mathbf{n} \cdot \mathbf{v}}{\mathrm{d}
       *    \mathbf{d} \mathrm{d} T } \f])
       *
       * @note At the moment this method is only used for the nitsche contact formulation
       */
      virtual void get_cauchy_n_dir_and_derivatives_at_xi_elast(
          const CORE::LINALG::Matrix<3, 1>& xi, const std::vector<double>& disp,
          const CORE::LINALG::Matrix<3, 1>& n, const CORE::LINALG::Matrix<3, 1>& dir,
          double& cauchy_n_dir, CORE::LINALG::SerialDenseMatrix* d_cauchyndir_dd,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd2,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dn,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_ddir,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dxi,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_dn,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_ddir,
          CORE::LINALG::Matrix<3, 1>* d_cauchyndir_dxi, const std::vector<double>* temp,
          CORE::LINALG::SerialDenseMatrix* d_cauchyndir_dT,
          CORE::LINALG::SerialDenseMatrix* d2_cauchyndir_dd_dT);

      virtual void output_strains(const int gp,
          const INPAR::STR::StrainType iostrain,                 // strain output option
          CORE::LINALG::Matrix<numgpt_post, numstr_>* elestrain  // strains at GP
      );

      virtual void output_stress(const int gp,
          const INPAR::STR::StressType iostress,                 // strain output option
          CORE::LINALG::Matrix<numgpt_post, numstr_>* elestress  // strains at GP
      );

      virtual void kinematics(const int gp = -1);

      virtual void integrate_mass_matrix(
          const int gp, CORE::LINALG::Matrix<numdofperelement_, numdofperelement_>& mass);

      virtual void integrate_stiff_matrix(const int gp,
          CORE::LINALG::Matrix<numdofperelement_, numdofperelement_>& stiff,
          CORE::LINALG::SerialDenseMatrix& Kda);

      virtual void integrate_force(const int gp, CORE::LINALG::Matrix<numdofperelement_, 1>& force);

      virtual void integrate_thermo_gp(const int gp, CORE::LINALG::SerialDenseVector& dHda);


      // algoirthmic parameters
      bool fbar_{};

      // plasticity ********************************************
      // Kbb^-1 at each Gauss point for recovery of inner variables
      std::vector<CORE::LINALG::SerialDenseMatrix> KbbInv_;

      // Kbd at each Gauss point for recovery of inner variables
      std::vector<CORE::LINALG::SerialDenseMatrix> Kbd_;

      // f_b at each Gauss point for recovery of inner variables
      std::vector<CORE::LINALG::SerialDenseVector> fbeta_;

      // plastic flow at each Gauss point at last Newton iteration
      std::vector<CORE::LINALG::SerialDenseVector> dDp_last_iter_;

      // increment of plastic flow over last Newton step (needed for Newton line search)
      std::vector<CORE::LINALG::SerialDenseVector> dDp_inc_;

      PlSpinType plspintype_;

      // line search parameter (old step length)
      double old_step_length_{};

      // EAS element technology ************************************
      Teuchos::RCP<CORE::LINALG::SerialDenseMatrix> KaaInv_;
      Teuchos::RCP<CORE::LINALG::SerialDenseMatrix> Kad_;
      Teuchos::RCP<CORE::LINALG::SerialDenseMatrix> KaT_;
      Teuchos::RCP<CORE::LINALG::Matrix<numdofperelement_, nen_>> KdT_eas_;
      Teuchos::RCP<CORE::LINALG::SerialDenseVector> feas_;
      Teuchos::RCP<std::vector<CORE::LINALG::SerialDenseMatrix>> Kba_;
      Teuchos::RCP<CORE::LINALG::SerialDenseVector> alpha_eas_;
      Teuchos::RCP<CORE::LINALG::SerialDenseVector> alpha_eas_last_timestep_;
      Teuchos::RCP<CORE::LINALG::SerialDenseVector> alpha_eas_delta_over_last_timestep_;
      Teuchos::RCP<CORE::LINALG::SerialDenseVector> alpha_eas_inc_;
      DRT::ELEMENTS::So3PlastEasType eastype_;
      int neas_{};

      // TSI ******************************************************
      bool tsi_{};

      /// derivative of the internal force vector w.r.t. temperature at this GP
      /// derivative w.r.t. gp temperature is sufficient as the gp temperature depends linearily
      /// on the nodal values.
      Teuchos::RCP<std::vector<CORE::LINALG::Matrix<numdofperelement_, 1>>> dFintdT_;

      /// derivative of NCP w.r.t. temperatures at gp
      Teuchos::RCP<std::vector<CORE::LINALG::SerialDenseVector>> KbT_;

      /// Temperature at each gp in last Newton iteration
      /// this is needed for the recovery of the plastic flow using KbT_
      /// As the gp temperature depends linearly on the nodal temperature dofs
      /// a scalar value is sufficient
      Teuchos::RCP<std::vector<double>> temp_last_;

      // Cauchy stress for Nitsche contact **************************
      bool is_nitsche_contact_{};
      std::vector<CORE::LINALG::Matrix<numstr_, 1>> cauchy_;
      std::vector<CORE::LINALG::Matrix<numstr_, numdofperelement_>> cauchy_deriv_;
      std::vector<CORE::LINALG::Matrix<numstr_, nen_>> cauchy_deriv_T_;

      // Static members for faster evaluation **************************
      static std::pair<bool, CORE::LINALG::Matrix<nen_, 1>> shapefunct_;
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nen_>> deriv_;
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nsd_>> invJ_;
      static std::pair<bool, double> detJ_;
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nen_>> N_XYZ_;  // N_XYZ = J^-1 * N_rst
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nsd_>>
          defgrd_;  //     deformation gradient consistent with displacements
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nsd_>>
          defgrd_mod_;  // deformation gradient consistent with displacements + element technology
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nsd_>> rcg_;
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nsd_>>
          delta_Lp_;  // plastic velocity increment over this time step
      static std::pair<bool, CORE::LINALG::Matrix<numstr_, numdofperelement_>> bop_;
      static std::pair<bool, CORE::LINALG::Matrix<numstr_, 1>> pk2_;
      static std::pair<bool, CORE::LINALG::Matrix<numstr_, numstr_>> cmat_;

      static std::pair<bool, CORE::LINALG::Matrix<nen_, nsd_>>
          xrefe_;  // X, material coord. of element
      static std::pair<bool, CORE::LINALG::Matrix<nen_, nsd_>>
          xcurr_;  // x, current  coord. of element
      static std::pair<bool, CORE::LINALG::Matrix<nen_, nsd_>>
          xcurr_rate_;  // xdot, rate of current  coord. of element
      static std::pair<bool, CORE::LINALG::Matrix<nen_, 1>>
          etemp_;  // vector of the current element temperatures

      // Static members for Fbar related stuff **************************
      static std::pair<bool, double> detF_;
      static std::pair<bool, double> detF_0_;
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nsd_>> inv_defgrd_;
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nsd_>> inv_defgrd_0_;
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nen_>> N_XYZ_0_;
      static std::pair<bool, CORE::LINALG::Matrix<numstr_, 1>> rcg_vec_;  // strain-like
      static std::pair<bool, double> f_bar_fac_;
      static std::pair<bool, CORE::LINALG::Matrix<numdofperelement_, 1>> htensor_;

      // Static members for EAS related stuff **************************
      // transformation matrix T0, maps M-matrix evaluated at origin
      // between local element coords and global coords
      // here we already get the inverse transposed T0
      static std::pair<bool, CORE::LINALG::Matrix<numstr_, numstr_>> T0invT_;
      static std::pair<bool, CORE::LINALG::Matrix<nsd_, nsd_>> jac_0_;
      static std::pair<bool, double> det_jac_0_;
      static std::pair<bool, CORE::LINALG::SerialDenseMatrix> M_eas_;  // EAS matrix M at current GP

      // NURBS-specific
      static std::pair<bool, CORE::LINALG::Matrix<nen_, 1>> weights_;
      static std::pair<bool, std::vector<CORE::LINALG::SerialDenseVector>> knots_;

      virtual void invalid_gp_data()
      {
        shapefunct_.first = false;
        deriv_.first = false;
        invJ_.first = false;
        detJ_.first = false;
        N_XYZ_.first = false;
        defgrd_.first = false;
        defgrd_mod_.first = false;
        rcg_.first = false;
        delta_Lp_.first = false;
        bop_.first = false;
        detF_.first = false;
        f_bar_fac_.first = false;
        htensor_.first = false;
        inv_defgrd_.first = false;
        rcg_vec_.first = false;
        M_eas_.first = false;
        pk2_.first = false;
        cmat_.first = false;
      }

      void invalid_ele_data()
      {
        xrefe_.first = false;
        xcurr_.first = false;
        xcurr_rate_.first = false;
        etemp_.first = false;
        detF_0_.first = false;
        inv_defgrd_0_.first = false;
        N_XYZ_0_.first = false;
        T0invT_.first = false;
        jac_0_.first = false;
        det_jac_0_.first = false;
        weights_.first = false;
        knots_.first = false;
      }

      inline const CORE::LINALG::Matrix<nen_, 1>& weights() const
      {
        FOUR_C_ASSERT(weights_.first, "weights_ not valid");
        return weights_.second;
      }
      inline CORE::LINALG::Matrix<nen_, 1>& set_weights()
      {
        weights_.first = true;
        return weights_.second;
      }

      inline const std::vector<CORE::LINALG::SerialDenseVector>& knots() const
      {
        FOUR_C_ASSERT(knots_.first, "weights_ not valid");
        return knots_.second;
      }
      inline std::vector<CORE::LINALG::SerialDenseVector>& set_knots()
      {
        knots_.first = true;
        return knots_.second;
      }

      void fill_position_arrays(const std::vector<double>& disp,  // current displacements
          const std::vector<double>& vel,                         // current velocities
          const std::vector<double>& temp                         // current temperatures
      )
      {
        for (int i = 0; i < nen_; ++i)
        {
          for (int d = 0; d < nsd_; ++d)
          {
            xrefe_.second(i, d) = Nodes()[i]->X()[d];
            xcurr_.second(i, d) = Nodes()[i]->X()[d] + disp[i * numdofpernode_ + d];
            if (!vel.empty()) xcurr_rate_.second(i, d) = vel[i * numdofpernode_ + d];
          }
          if (!temp.empty()) etemp_.second(i) = temp[i];
        }
        xrefe_.first = true;
        xcurr_.first = true;
        if (!vel.empty()) xcurr_rate_.first = true;
        if (!temp.empty()) etemp_.first = true;
      }

      inline const CORE::LINALG::Matrix<nen_, nsd_>& xrefe()
      {
        FOUR_C_ASSERT(xrefe_.first == true, "xrefe not valid");
        return xrefe_.second;
      }

      inline const CORE::LINALG::Matrix<nen_, nsd_>& xcurr()
      {
        FOUR_C_ASSERT(xcurr_.first, "xcurr_ not valid");
        return xcurr_.second;
      }

      inline const CORE::LINALG::Matrix<nen_, nsd_>& xcurr_rate()
      {
        FOUR_C_ASSERT(xcurr_rate_.first, "xcurr_rate_ not valid");
        return xcurr_rate_.second;
      }

      inline const CORE::LINALG::Matrix<nen_, 1>& temp()
      {
        FOUR_C_ASSERT(etemp_.first, "etemp not valid");
        return etemp_.second;
      }

      inline const CORE::LINALG::Matrix<nen_, 1>& shape_function() const
      {
        FOUR_C_ASSERT(shapefunct_.first, "shape function not valid");
        return shapefunct_.second;
      }
      inline CORE::LINALG::Matrix<nen_, 1>& set_shape_function()
      {
        shapefunct_.first = true;
        return shapefunct_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nen_>& deriv_shape_function() const
      {
        FOUR_C_ASSERT(deriv_.first, "deriv shape function not valid");
        return deriv_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nen_>& set_deriv_shape_function()
      {
        deriv_.first = true;
        return deriv_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nen_>& deriv_shape_function_xyz() const
      {
        FOUR_C_ASSERT(N_XYZ_.first, "deriv shape function not valid");
        return N_XYZ_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nen_>& set_deriv_shape_function_xyz()
      {
        N_XYZ_.first = true;
        return N_XYZ_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nsd_>& inv_j() const
      {
        FOUR_C_ASSERT(invJ_.first, "invJ_ not valid");
        return invJ_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nsd_>& set_inv_j()
      {
        invJ_.first = true;
        return invJ_.second;
      }

      inline const double& det_j() const
      {
        FOUR_C_ASSERT(detJ_.first, "detJ_ not valid");
        return detJ_.second;
      }
      inline double& set_det_j()
      {
        detJ_.first = true;
        return detJ_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nsd_>& defgrd() const
      {
        FOUR_C_ASSERT(defgrd_.first, "defgrd_ not valid");
        return defgrd_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nsd_>& set_defgrd()
      {
        defgrd_.first = true;
        return defgrd_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nsd_>& defgrd_mod() const
      {
        FOUR_C_ASSERT(defgrd_mod_.first, "defgrd_mod_ not valid");
        return defgrd_mod_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nsd_>& set_defgrd_mod()
      {
        defgrd_mod_.first = true;
        return defgrd_mod_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nsd_>& rcg() const
      {
        FOUR_C_ASSERT(rcg_.first, "rcg_ not valid");
        return rcg_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nsd_>& set_rcg()
      {
        rcg_.first = true;
        return rcg_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nsd_>& delta_lp() const
      {
        FOUR_C_ASSERT(delta_Lp_.first, "delta_Lp_ not valid");
        return delta_Lp_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nsd_>& set_delta_lp()
      {
        delta_Lp_.first = true;
        return delta_Lp_.second;
      }

      inline const CORE::LINALG::Matrix<numstr_, numdofperelement_>& bop() const
      {
        FOUR_C_ASSERT(bop_.first, "bop_ not valid");
        return bop_.second;
      }
      inline CORE::LINALG::Matrix<numstr_, numdofperelement_>& set_bop()
      {
        bop_.first = true;
        return bop_.second;
      }

      inline const CORE::LINALG::Matrix<numstr_, 1>& p_k2() const
      {
        FOUR_C_ASSERT(pk2_.first, "pk2_ not valid");
        return pk2_.second;
      }
      inline CORE::LINALG::Matrix<numstr_, 1>& set_p_k2()
      {
        pk2_.first = true;
        return pk2_.second;
      }

      inline const CORE::LINALG::Matrix<numstr_, numstr_>& cmat() const
      {
        FOUR_C_ASSERT(cmat_.first, "cmat_ not valid");
        return cmat_.second;
      }
      inline CORE::LINALG::Matrix<numstr_, numstr_>& set_cmat()
      {
        cmat_.first = true;
        return cmat_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nen_>& deriv_shape_function_xyz_0() const
      {
        FOUR_C_ASSERT(N_XYZ_0_.first, "deriv shape function not valid");
        return N_XYZ_0_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nen_>& set_deriv_shape_function_xyz_0()
      {
        N_XYZ_0_.first = true;
        return N_XYZ_0_.second;
      }

      inline const double& det_f() const
      {
        FOUR_C_ASSERT(detF_.first, "detF_ not valid");
        return detF_.second;
      }
      inline double& set_det_f()
      {
        detF_.first = true;
        return detF_.second;
      }

      inline const double& det_f_0() const
      {
        FOUR_C_ASSERT(detF_0_.first, "detF_0_ not valid");
        return detF_0_.second;
      }
      inline double& set_det_f_0()
      {
        detF_0_.first = true;
        return detF_0_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nsd_>& inv_defgrd() const
      {
        FOUR_C_ASSERT(inv_defgrd_.first, "inv_defgrd_ not valid");
        return inv_defgrd_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nsd_>& set_inv_defgrd()
      {
        inv_defgrd_.first = true;
        return inv_defgrd_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nsd_>& inv_defgrd_0() const
      {
        FOUR_C_ASSERT(inv_defgrd_0_.first, "inv_defgrd_0_ not valid");
        return inv_defgrd_0_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nsd_>& set_inv_defgrd_0()
      {
        inv_defgrd_0_.first = true;
        return inv_defgrd_0_.second;
      }

      inline const CORE::LINALG::Matrix<nsd_, nsd_>& jac_0() const
      {
        FOUR_C_ASSERT(jac_0_.first, "jac_0_ not valid");
        return jac_0_.second;
      }
      inline CORE::LINALG::Matrix<nsd_, nsd_>& set_jac_0()
      {
        jac_0_.first = true;
        return jac_0_.second;
      }

      inline const double& det_jac_0() const
      {
        FOUR_C_ASSERT(det_jac_0_.first, "det_jac_0_ not valid");
        return det_jac_0_.second;
      }
      inline double& set_det_jac_0()
      {
        det_jac_0_.first = true;
        return det_jac_0_.second;
      }

      inline const CORE::LINALG::Matrix<numstr_, 1>& rc_gvec() const
      {
        FOUR_C_ASSERT(rcg_vec_.first, "rcg_vec_ not valid");
        return rcg_vec_.second;
      }
      inline CORE::LINALG::Matrix<numstr_, 1>& set_rc_gvec()
      {
        rcg_vec_.first = true;
        return rcg_vec_.second;
      }

      inline const double& fbar_fac() const
      {
        FOUR_C_ASSERT(f_bar_fac_.first, "f_bar_fac_ not valid");
        return f_bar_fac_.second;
      }
      inline double& set_fbar_fac()
      {
        f_bar_fac_.first = true;
        return f_bar_fac_.second;
      }

      inline const CORE::LINALG::Matrix<numdofperelement_, 1>& htensor() const
      {
        FOUR_C_ASSERT(htensor_.first, "htensor_ not valid");
        return htensor_.second;
      }
      inline CORE::LINALG::Matrix<numdofperelement_, 1>& set_htensor()
      {
        htensor_.first = true;
        return htensor_.second;
      }

      void evaluate_center()
      {
        // element coordinate derivatives at centroid
        static CORE::LINALG::Matrix<nsd_, nen_> N_rst_0(false);
        CORE::FE::shape_function_3D_deriv1(N_rst_0, 0.0, 0.0, 0.0, CORE::FE::CellType::hex8);

        // inverse jacobian matrix at centroid
        set_jac_0().Multiply(N_rst_0, xrefe());
        static CORE::LINALG::Matrix<nsd_, nsd_> invJ_0;
        set_det_jac_0() = invJ_0.Invert(jac_0());
        // material derivatives at centroid
        set_deriv_shape_function_xyz_0().Multiply(invJ_0, N_rst_0);

        // deformation gradient and its determinant at centroid
        static CORE::LINALG::Matrix<3, 3> defgrd_0(false);
        defgrd_0.MultiplyTT(xcurr(), deriv_shape_function_xyz_0());
        set_det_f_0() = set_inv_defgrd_0().Invert(defgrd_0);
      }

      void setup_fbar_gp()
      {
        if (det_f() < 0. || det_f_0() < 0.) FOUR_C_THROW("element distortion too large");
        set_fbar_fac() = pow(det_f_0() / det_f(), 1. / 3.);
        set_defgrd_mod().Update(set_fbar_fac(), defgrd());
        set_htensor().Clear();

        for (int n = 0; n < numdofperelement_; n++)
          for (int i = 0; i < 3; i++)
            set_htensor()(n) += inv_defgrd_0()(i, n % 3) * deriv_shape_function_xyz_0()(i, n / 3) -
                                inv_defgrd()(i, n % 3) * deriv_shape_function_xyz()(i, n / 3);
      }

      inline const CORE::LINALG::Matrix<numstr_, numstr_>& t0inv_t() const
      {
        FOUR_C_ASSERT(T0invT_.first, "T0invT_ not valid");
        return T0invT_.second;
      }
      inline CORE::LINALG::Matrix<numstr_, numstr_>& set_t0inv_t()
      {
        T0invT_.first = true;
        return T0invT_.second;
      }

      inline const CORE::LINALG::SerialDenseMatrix& m_eas() const
      {
        FOUR_C_ASSERT(M_eas_.first, "M_eas_ not valid");
        return M_eas_.second;
      }
      inline CORE::LINALG::SerialDenseMatrix& set_m_eas()
      {
        M_eas_.first = true;
        return M_eas_.second;
      }

      inline void evaluate_shape(const CORE::LINALG::Matrix<3, 1>& xi)
      {
        if (distype == CORE::FE::CellType::nurbs27)
          CORE::FE::NURBS::nurbs_get_3D_funct_deriv(
              set_shape_function(), set_deriv_shape_function(), xi, knots(), weights(), distype);
        else
          CORE::FE::shape_function<distype>(xi, set_shape_function());
      }

      inline void evaluate_shape_deriv(const CORE::LINALG::Matrix<3, 1>& xi)
      {
        if (distype == CORE::FE::CellType::nurbs27)
          CORE::FE::NURBS::nurbs_get_3D_funct_deriv(
              set_shape_function(), set_deriv_shape_function(), xi, knots(), weights(), distype);
        else
          CORE::FE::shape_function_deriv1<distype>(xi, set_deriv_shape_function());
      }

      void get_nurbs_ele_info(DRT::Discretization* dis = nullptr);


    };  // class So3Plast

    template <DRT::ELEMENTS::So3PlastEasType eastype>
    struct PlastEasTypeToNumEas
    {
    };
    template <>
    struct PlastEasTypeToNumEas<DRT::ELEMENTS::soh8p_easmild>
    {
      static constexpr int neas = 9;
    };
    template <>
    struct PlastEasTypeToNumEas<DRT::ELEMENTS::soh8p_easfull>
    {
      static constexpr int neas = 21;
    };
    template <>
    struct PlastEasTypeToNumEas<DRT::ELEMENTS::soh8p_eassosh8>
    {
      static constexpr int neas = 7;
    };
    template <>
    struct PlastEasTypeToNumEas<DRT::ELEMENTS::soh18p_eassosh18>
    {
      static constexpr int neas = 9;
    };
    template <>
    struct PlastEasTypeToNumEas<DRT::ELEMENTS::soh8p_easnone>
    {
      static constexpr int neas = 0;
    };

    int PlastEasTypeToNumEasV(DRT::ELEMENTS::So3PlastEasType et);

  }  // namespace ELEMENTS
}  // namespace DRT

/*----------------------------------------------------------------------*/
FOUR_C_NAMESPACE_CLOSE

#endif