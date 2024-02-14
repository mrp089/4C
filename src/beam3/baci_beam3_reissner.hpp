/*-----------------------------------------------------------------------------------------------*/
/*! \file

\brief 3D nonlinear Reissner beam element

\level 2

*/
/*-----------------------------------------------------------------------------------------------*/

/* 3D nonlinear Reissner beam element of type II (according to "The interpolation
 * of rotations and its application to finite element models of geometrically exact
 * rods", Romero 2004)
 *
 * Attention: For this implementation, prescribed 3D rotation values have no
 * direct physical interpretation so far because DBC handling is always additive in BACI.
 * For 2D rotations, multiplicative and additive increments are identical and
 * rotations can be prescribed without problems. */

#ifndef BACI_BEAM3_REISSNER_HPP
#define BACI_BEAM3_REISSNER_HPP


#include "baci_config.hpp"

#include "baci_beam3_base.hpp"
#include "baci_beam3_spatial_discretization_utils.hpp"
#include "baci_discretization_fem_general_largerotations.hpp"
#include "baci_discretization_fem_general_utils_fem_shapefunctions.hpp"
#include "baci_discretization_fem_general_utils_integration.hpp"
#include "baci_lib_node.hpp"
#include "baci_linalg_serialdensematrix.hpp"
#include "baci_structure_new_elements_paramsinterface.hpp"
#include "baci_utils_fad.hpp"

#include <Sacado.hpp>

BACI_NAMESPACE_OPEN

typedef Sacado::Fad::DFad<double> FAD;

// forward declaration ...
namespace CORE::LINALG
{
  class SerialDenseMatrix;
}

namespace LARGEROTATIONS
{
  template <unsigned int numnodes, typename T>
  class TriadInterpolationLocalRotationVectors;
}

// #define BEAM3RCONSTSTOCHFORCE  //Flag in order to hold stochastic forces constant over the
// element
//  length and to only provide random numbers for the 3 translational DoFs (needed in order to
//  compare with beam3eb)

namespace DRT
{
  namespace ELEMENTS
  {
    class Beam3rType : public DRT::ElementType
    {
     public:
      std::string Name() const override { return "Beam3rType"; }

      static Beam3rType& Instance();

      CORE::COMM::ParObject* Create(const std::vector<char>& data) override;

      Teuchos::RCP<DRT::Element> Create(const std::string eletype, const std::string eledistype,
          const int id, const int owner) override;

      Teuchos::RCP<DRT::Element> Create(const int id, const int owner) override;

      int Initialize(DRT::Discretization& dis) override;

      void NodalBlockInformation(
          DRT::Element* dwele, int& numdf, int& dimns, int& nv, int& np) override;

      CORE::LINALG::SerialDenseMatrix ComputeNullSpace(
          DRT::Node& actnode, const double* x0, const int numdof, const int dimnsp) override;

      void SetupElementDefinition(
          std::map<std::string, std::map<std::string, INPUT::LineDefinition>>& definitions)
          override;

     private:
      static Beam3rType instance_;
    };

    /*!
    \brief 3D nonlinear Reissner beam element implemented according to the following sources:
    Jelenic, Crisfield, 1999, "Geometrically exact 3D beam theory: implementations of a
    strain-invariant finite element for statics and dynamics", Crisfield, Jelenic, 1999,
    "Objectivity of strain measures in the geometrically exact three dimensional beam theory and its
    finite element implementation", Romero, 2004, "The interpolation of rotations and its
    application to finite element models of geometrically exact rods", Crisfield, 2003, "Non-linear
    Finite Element Analysis of Solids and Structures", Volume 2

    */
    class Beam3r : public Beam3Base
    {
     public:
      //! purposes of numerical integration
      enum IntegrationPurpose
      {
        res_elastic_force,
        res_elastic_moment,
        res_inertia,
        res_damp_stoch,
        neumann_lineload
      };

     public:
      //! @name Friends
      friend class Beam3rType;

      //! @name Constructors and destructors and related methods

      /*!
      \brief Standard Constructor

      \param id    (in): A globally unique element id
      \param etype (in): Type of element
      \param owner (in): owner processor of the element
      */
      Beam3r(int id, int owner);

      /*!
      \brief Copy Constructor

      Makes a deep copy of a Element
      */
      Beam3r(const Beam3r& old);



      // don't want = operator
      Beam3r& operator=(const Beam3r& old);

      /*!
      \brief Deep copy this instance of Beam3r and return pointer to the copy

      The Clone() method is used by the virtual base class Element in cases
      where the type of the derived class is unknown and a copy-ctor is needed
    .
      */
      DRT::Element* Clone() const override;

      /*!
     \brief Get shape type of element
     */
      CORE::FE::CellType Shape() const override;

      /*!
      \brief Return unique ParObject id

      Every class implementing ParObject needs a unique id defined at the
      top of parobject.H
      */
      int UniqueParObjectId() const override { return Beam3rType::Instance().UniqueParObjectId(); }

      /*!
      \brief Pack this class so it can be communicated

      \ref Pack and \ref Unpack are used to communicate this element

      */
      void Pack(CORE::COMM::PackBuffer& data) const override;

      /*!
      \brief Unpack data from a char vector into this class

      \ref Pack and \ref Unpack are used to communicate this element

      */
      void Unpack(const std::vector<char>& data) override;

      DRT::ElementType& ElementType() const override { return Beam3rType::Instance(); }

      //@}

      /*!
      \brief Return number of lines to this element
      */
      int NumLine() const override { return 1; }

      /*!
      \brief Get vector of Teuchos::RCPs to the lines of this element
      */
      std::vector<Teuchos::RCP<DRT::Element>> Lines() override;

      /** \brief Get number of nodes used for centerline interpolation
       *
       *  \author grill
       *  \date 05/16 */
      inline int NumCenterlineNodes() const override
      {
        return centerline_hermite_ ? 2 : this->NumNode();
      }

      /** \brief find out whether given node is used for centerline interpolation
       *
       *  \author grill
       *  \date 10/16 */
      inline bool IsCenterlineNode(const DRT::Node& node) const override
      {
        if (!centerline_hermite_ or node.Id() == this->Nodes()[0]->Id() or
            node.Id() == this->Nodes()[1]->Id())
          return true;
        else
          return false;
      }

      /*!
      \brief Get number of degrees of freedom of a single node
      */
      int NumDofPerNode(const DRT::Node& node) const override
      {
        /* note: this is not necessarily the number of DOF assigned to this node by the
         * discretization finally, but only the number of DOF requested for this node by this
         * element; the discretization will finally assign the maximal number of DOF to this node
         * requested by any element connected to this node*/
        if (!centerline_hermite_)
          return 6;
        else
        {
          /* in case of Hermite centerline interpolation (so far always 3rd order = 2nodes), we have
           * 6 translational DOFs for the first two nodes and additionally 3 rotational DOFs for
           * each node */
          int dofpn_aux = 0;

          if (node.Id() == this->Nodes()[0]->Id() or node.Id() == this->Nodes()[1]->Id())
          {
            dofpn_aux = 9;
          }
          else
          {
            dofpn_aux = 3;
          }

          const int dofpn = dofpn_aux;
          return dofpn;
        }
      }

      /*!
      \brief Get number of degrees of freedom per element not including nodal degrees of freedom
      */
      int NumDofPerElement() const override { return 0; }

      /*!
      \brief Print this element
      */
      void Print(std::ostream& os) const override;

      /** \brief get centerline position at xi \in [-1,1] (element parameter space)
       *
       *  \author grill
       *  \date 06/16 */
      void GetPosAtXi(CORE::LINALG::Matrix<3, 1>& pos, const double& xi,
          const std::vector<double>& disp) const override;

      /** \brief get triad at xi \in [-1,1] (element parameter space)
       *
       *  \author grill
       *  \date 07/16 */
      void GetTriadAtXi(CORE::LINALG::Matrix<3, 3>& triad, const double& xi,
          const std::vector<double>& disp) const override;

      /** \brief get generalized interpolation matrix which yields the variation of the position and
       *         orientation at xi \in [-1,1] if multiplied with the vector of primary DoF
       * variations
       *
       *  \author grill
       *  \date 11/16 */
      void GetGeneralizedInterpolationMatrixVariationsAtXi(CORE::LINALG::SerialDenseMatrix& Ivar,
          const double& xi, const std::vector<double>& disp) const override;

      /** \brief get linearization of the product of (generalized interpolation matrix for
       * variations (see above) and applied force vector) with respect to the primary DoFs of this
       * element
       *
       *  \author grill
       *  \date 01/17 */
      void GetStiffmatResultingFromGeneralizedInterpolationMatrixAtXi(
          CORE::LINALG::SerialDenseMatrix& stiffmat, const double& xi,
          const std::vector<double>& disp,
          const CORE::LINALG::SerialDenseVector& force) const override
      {
        const unsigned int vpernode = this->HermiteCenterlineInterpolation() ? 2 : 1;
        const unsigned int nnodecl = this->NumCenterlineNodes();
        const unsigned int nnodetriad = this->NumNode();

        // safety check
        if ((unsigned int)stiffmat.numRows() != 3 * vpernode * nnodecl + 3 * nnodetriad or
            (unsigned int) stiffmat.numCols() != 3 * vpernode * nnodecl + 3 * nnodetriad)
          dserror("size mismatch! expected %dx%d matrix and got %dx%d",
              3 * vpernode * nnodecl + 3 * nnodetriad, 3 * vpernode * nnodecl + 3 * nnodetriad,
              stiffmat.numRows(), stiffmat.numCols());

        // nothing to do here since this term vanishes for Beam3r
        stiffmat.putScalar(0.0);
      }

      /** \brief get generalized interpolation matrix which yields the increments of the position
       * and orientation at xi \in [-1,1] if multiplied with the vector of primary DoF increments
       *
       *  \author grill
       *  \date 11/16 */
      void GetGeneralizedInterpolationMatrixIncrementsAtXi(CORE::LINALG::SerialDenseMatrix& Iinc,
          const double& xi, const std::vector<double>& disp) const override;

      /** \brief get unit tangent vector in reference configuration at i-th node of beam element
       * (element-internal numbering)
       *
       *  \author grill
       *  \date 06/16 */
      inline void GetRefTangentAtNode(
          CORE::LINALG::Matrix<3, 1>& Tref_i, const int& i) const override
      {
        if (not((unsigned)i < Tref().size()))
          dserror("asked for tangent at node index %d, but only %d centerline nodes existing", i,
              Tref().size());

        Tref_i = Tref()[i];
      }

      /*!
      \brief get tangent of centerline at all nodes in reference configuration
      */
      std::vector<CORE::LINALG::Matrix<3, 1>> Tref() const { return Tref_; }

      /*!
      \brief Get jacobiGP_ factor of first Gauss point for under-integration (constant over element
      length for linear Lagrange interpolation)
      */
      const double& GetJacobi() const { return jacobiGPelastf_[0]; }

      /** \brief get Jacobi factor ds/dxi(xi) at xi \in [-1;1]
       *
       *  \author grill
       *  \date 06/16 */
      double GetJacobiFacAtXi(const double& xi) const override;

      /*!
      \brief Get maximal bending curvature occurring in this element
      */
      const double& GetKappaMax() const { return Kmax_; }

      /** \brief Get material cross-section deformation measures, i.e. strain resultants
       *
       *  \author grill
       *  \date 04/17 */
      inline void GetMaterialStrainResultantsAtAllGPs(std::vector<double>& axial_strain_GPs,
          std::vector<double>& shear_strain_2_GPs, std::vector<double>& shear_strain_3_GPs,
          std::vector<double>& twist_GPs, std::vector<double>& curvature_2_GPs,
          std::vector<double>& curvature_3_GPs) const override
      {
        axial_strain_GPs = axial_strain_GP_elastf_;
        shear_strain_2_GPs = shear_strain_2_GP_elastf_;
        shear_strain_3_GPs = shear_strain_3_GP_elastf_;

        twist_GPs = twist_GP_elastm_;
        curvature_2_GPs = curvature_2_GP_elastm_;
        curvature_3_GPs = curvature_3_GP_elastm_;
      }

      /** \brief Get spatial cross-section stress resultants
       *
       *  \author grill
       *  \date 04/17 */
      inline void GetSpatialStressResultantsAtAllGPs(std::vector<double>& spatial_axial_force_GPs,
          std::vector<double>& spatial_shear_force_2_GPs,
          std::vector<double>& spatial_shear_force_3_GPs, std::vector<double>& spatial_torque_GPs,
          std::vector<double>& spatial_bending_moment_2_GPs,
          std::vector<double>& spatial_bending_moment_3_GPs) const override
      {
        GetSpatialForcesAtAllGPs(
            spatial_axial_force_GPs, spatial_shear_force_2_GPs, spatial_shear_force_3_GPs);

        GetSpatialMomentsAtAllGPs(
            spatial_torque_GPs, spatial_bending_moment_2_GPs, spatial_bending_moment_3_GPs);
      }

      /** \brief Get spatial cross-section stress resultants
       *
       *  \author grill
       *  \date 04/17 */
      inline void GetSpatialForcesAtAllGPs(std::vector<double>& spatial_axial_force_GPs,
          std::vector<double>& spatial_shear_force_2_GPs,
          std::vector<double>& spatial_shear_force_3_GPs) const override
      {
        spatial_axial_force_GPs = spatial_x_force_GP_elastf_;
        spatial_shear_force_2_GPs = spatial_y_force_2_GP_elastf_;
        spatial_shear_force_3_GPs = spatial_z_force_3_GP_elastf_;
      }

      /** \brief Get spatial cross-section stress resultants
       *
       *  \author grill
       *  \date 04/17 */
      inline void GetSpatialMomentsAtAllGPs(std::vector<double>& spatial_torque_GPs,
          std::vector<double>& spatial_bending_moment_2_GPs,
          std::vector<double>& spatial_bending_moment_3_GPs) const override
      {
        spatial_torque_GPs = spatial_x_moment_GP_elastm_;
        spatial_bending_moment_2_GPs = spatial_y_moment_2_GP_elastm_;
        spatial_bending_moment_3_GPs = spatial_z_moment_3_GP_elastm_;
      }

      /** \brief Get material cross-section stress resultants
       *
       *  \author grill
       *  \date 04/17 */
      inline void GetMaterialStressResultantsAtAllGPs(std::vector<double>& material_axial_force_GPs,
          std::vector<double>& material_shear_force_2_GPs,
          std::vector<double>& material_shear_force_3_GPs, std::vector<double>& material_torque_GPs,
          std::vector<double>& material_bending_moment_2_GPs,
          std::vector<double>& material_bending_moment_3_GPs) const override
      {
        material_axial_force_GPs = material_axial_force_GP_elastf_;
        material_shear_force_2_GPs = material_shear_force_2_GP_elastf_;
        material_shear_force_3_GPs = material_shear_force_3_GP_elastf_;

        material_torque_GPs = material_torque_GP_elastm_;
        material_bending_moment_2_GPs = material_bending_moment_2_GP_elastm_;
        material_bending_moment_3_GPs = material_bending_moment_3_GP_elastm_;
      }

      /** \brief get access to the reference length
       *
       *  \author grill
       *  \date 05/16 */
      inline double RefLength() const override { return reflength_; }

      /*!
      \brief Get initial nodal rotation vectors
      */
      const std::vector<CORE::LINALG::Matrix<3, 1>>& InitialNodalRotVecs() const
      {
        return theta0node_;
      }

      /*!
      \brief Set bool indicating Hermite centerline interpolation
      */
      void SetCenterlineHermite(const bool yesno);

      //! computes the number of different random numbers required in each time step for generation
      //! of stochastic forces
      int HowManyRandomNumbersINeed() const override;

      //! sets up all geometric parameters (considering current position as reference configuration)
      /* nnodetriad: number of nodes used for interpolation of triad field
       * nnodecl: number of nodes used for interpolation of centerline
       * vpernode: interpolated values per centerline node (1: value (i.e. Lagrange), 2: value +
       * derivative of value (i.e. Hermite))*/
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void SetUpReferenceGeometry(
          const std::vector<double>& xrefe, const std::vector<double>& rotrefe);

      //@}

      //! @name Construction

      /*!
      \brief Read input for this element
      */
      bool ReadElement(const std::string& eletype, const std::string& distype,
          INPUT::LineDefinition* linedef) override;

      //@}


      //! @name Evaluation methods


      /*!
      \brief Evaluate this element

      \param params (in/out)    : ParameterList for communication between control routine
                                  and elements
      \param discretization (in): A reference to the underlying discretization
      \param lm (in)            : location vector of this element
      \param elemat1 (out)      : matrix to be filled by element depending on commands
                                  given in params
      \param elemat2 (out)      : matrix to be filled by element depending on commands
                                  given in params
      \param elevec1 (out)      : vector to be filled by element depending on commands
                                  given in params
      \param elevec2 (out)      : vector to be filled by element depending on commands
                                  given in params
      \param elevec3 (out)      : vector to be filled by element depending on commands
                                  given in params
      \return 0 if successful, negative otherwise
      */
      int Evaluate(Teuchos::ParameterList& params, DRT::Discretization& discretization,
          std::vector<int>& lm, CORE::LINALG::SerialDenseMatrix& elemat1,
          CORE::LINALG::SerialDenseMatrix& elemat2, CORE::LINALG::SerialDenseVector& elevec1,
          CORE::LINALG::SerialDenseVector& elevec2,
          CORE::LINALG::SerialDenseVector& elevec3) override;

      /*!
      \brief Evaluate a Neumann boundary condition

      \param params (in/out)    : ParameterList for communication between control routine
                                  and elements
      \param discretization (in): A reference to the underlying discretization
      \param condition (in)     : The condition to be evaluated
      \param lm (in)            : location vector of this element
      \param elevec1 (out)      : Force vector to be filled by element

      \return 0 if successful, negative otherwise
      */
      int EvaluateNeumann(Teuchos::ParameterList& params, DRT::Discretization& discretization,
          DRT::Condition& condition, std::vector<int>& lm, CORE::LINALG::SerialDenseVector& elevec1,
          CORE::LINALG::SerialDenseMatrix* elemat1 = nullptr) override;

      /*!
      \brief Evaluate PTC addition to stiffness for free Brownian motion

      An element derived from this class uses the Evaluate method to receive commands
      and parameters from some control routine in params and evaluates a  statistical Neumann
      boundary condition as used in the problem type STATISTICAL MECHANICS

      \param params (in/out)       : ParameterList for communication between control routine and
      elements \param vector<double> mydisp : current nodal displacement \param elemat1 (out) :
      artificial damping matrix to be filled by element

      \return 0 if successful, negative otherwise
      */
      template <unsigned int nnode>
      void EvaluatePTC(Teuchos::ParameterList& params, CORE::LINALG::SerialDenseMatrix& elemat1);

      //! calculation of nonlinear stiffness and mass matrix
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void CalcInternalAndInertiaForcesAndStiff(
          CORE::LINALG::Matrix<3 * vpernode * nnodecl, 1, double>& disp_totlag_centerline,
          std::vector<CORE::LINALG::Matrix<4, 1, double>>& Qnode,
          CORE::LINALG::SerialDenseMatrix* stiffmatrix, CORE::LINALG::SerialDenseMatrix* massmatrix,
          CORE::LINALG::SerialDenseVector* force, CORE::LINALG::SerialDenseVector* inertia_force);

      //@}

      /** \brief add indices of those DOFs of a given node that are positions
       *
       *  \author grill
       *  \date 07/16 */
      inline void PositionDofIndices(
          std::vector<int>& posdofs, const DRT::Node& node) const override
      {
        if ((not centerline_hermite_) or
            (node.Id() == this->Nodes()[0]->Id() or node.Id() == this->Nodes()[1]->Id()))
        {
          posdofs.push_back(0);
          posdofs.push_back(1);
          posdofs.push_back(2);
        }
        return;
      }

      /** \brief add indices of those DOFs of a given node that are tangents (in the case of Hermite
       * interpolation)
       *
       *  \author grill
       *  \date 07/16 */
      inline void TangentDofIndices(
          std::vector<int>& tangdofs, const DRT::Node& node) const override
      {
        if (centerline_hermite_ and
            (node.Id() == this->Nodes()[0]->Id() or node.Id() == this->Nodes()[1]->Id()))
        {
          tangdofs.push_back(6);
          tangdofs.push_back(7);
          tangdofs.push_back(8);
        }
        return;
      }

      /** \brief add indices of those DOFs of a given node that are rotation DOFs (non-additive
       * rotation vectors)
       *
       *  \author grill
       *  \date 07/16 */
      inline void RotationVecDofIndices(
          std::vector<int>& rotvecdofs, const DRT::Node& node) const override
      {
        if ((not centerline_hermite_) or
            (node.Id() == this->Nodes()[0]->Id() or node.Id() == this->Nodes()[1]->Id()))
        {
          rotvecdofs.push_back(3);
          rotvecdofs.push_back(4);
          rotvecdofs.push_back(5);
        }
        else
        {
          rotvecdofs.push_back(0);
          rotvecdofs.push_back(1);
          rotvecdofs.push_back(2);
        }
        return;
      }

      /** \brief add indices of those DOFs of a given node that are 1D rotation DOFs
       *         (planar rotations are additive, e.g. in case of relative twist DOF of beam3k with
       * rotvec=false)
       *
       *  \author grill
       *  \date 07/16 */
      inline void Rotation1DDofIndices(
          std::vector<int>& twistdofs, const DRT::Node& node) const override
      {
        return;
      }

      /** \brief add indices of those DOFs of a given node that represent norm of tangent vector
       *         (additive, e.g. in case of beam3k with rotvec=true)
       *
       *  \author grill
       *  \date 07/16 */
      inline void TangentLengthDofIndices(
          std::vector<int>& tangnormdofs, const DRT::Node& node) const override
      {
        return;
      }

      /** \brief get element local indices of those Dofs that are used for centerline interpolation
       *
       *  \author grill
       *  \date 12/16 */
      inline void CenterlineDofIndicesOfElement(
          std::vector<unsigned int>& centerlinedofindices) const override
      {
        // nnodecl: number of nodes used for interpolation of centerline
        // vpernode: number of interpolated values per centerline node (1: value (i.e. Lagrange), 2:
        // value + derivative of value (i.e. Hermite))
        const unsigned int vpernode = this->HermiteCenterlineInterpolation() ? 2 : 1;
        const unsigned int nnodecl = this->NumCenterlineNodes();

        const unsigned int dofperclnode = 3 * vpernode;
        const unsigned int dofpertriadnode = 3;
        const unsigned int dofpercombinode =
            dofperclnode +
            dofpertriadnode;  // if node is used for centerline and triad interpolation

        centerlinedofindices.resize(dofperclnode * nnodecl, 0);

        for (unsigned int inodecl = 0; inodecl < nnodecl; ++inodecl)
        {
          // position Dofs: always node-local indices 0,1,2
          for (unsigned int idof = 0; idof < 3; ++idof)
            centerlinedofindices[dofperclnode * inodecl + idof] = dofpercombinode * inodecl + idof;
          // tangent Dofs (if needed): node-local indices 6,7,8
          for (unsigned int idof = 6; idof < dofpercombinode; ++idof)
            centerlinedofindices[dofperclnode * inodecl + idof - 3] =
                dofpercombinode * inodecl + idof;
        }
      }

      /** \brief extract values for those Dofs relevant for centerline-interpolation from total
       * state vector
       *
       *  \author grill
       *  \date 11/16 */
      void ExtractCenterlineDofValuesFromElementStateVector(const std::vector<double>& dofvec,
          std::vector<double>& dofvec_centerline, bool add_reference_values = false) const override;

      //! get internal (elastic) energy of element
      double GetInternalEnergy() const override { return Eint_; };

      //! get kinetic energy of element
      double GetKineticEnergy() const override { return Ekin_; };

     private:
      //! @name Internal calculation methods

      /** \brief compute internal (i.e. elastic) force, stiffness matrix, inertia force and mass
       * matrix
       *
       *  \author grill
       *  \date 12/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void CalcInternalAndInertiaForcesAndStiff(Teuchos::ParameterList& params,
          std::vector<double>& disp, CORE::LINALG::SerialDenseMatrix* stiffmatrix,
          CORE::LINALG::SerialDenseMatrix* massmatrix, CORE::LINALG::SerialDenseVector* force,
          CORE::LINALG::SerialDenseVector* inertia_force);

      /** \brief compute internal (i.e. elastic) force and stiffness matrix
       *
       *  \author grill
       *  \date 12/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode, typename T>
      void CalcInternalForceAndStiff(
          const CORE::LINALG::Matrix<3 * vpernode * nnodecl, 1, T>& disp_totlag_centerline,
          const std::vector<CORE::LINALG::Matrix<4, 1, T>>& Qnode,
          CORE::LINALG::SerialDenseMatrix* stiffmatrix,
          CORE::LINALG::Matrix<3 * vpernode * nnodecl + 3 * nnodetriad, 1, T>& internal_force);

      /** \brief calculate inertia force and mass matrix
       *
       *  \author grill
       *  \date 11/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void CalcInertiaForceAndMassMatrix(
          const CORE::LINALG::Matrix<3 * vpernode * nnodecl, 1, double>& disp_totlag_centerline,
          const std::vector<CORE::LINALG::Matrix<4, 1, double>>& Qnode,
          CORE::LINALG::SerialDenseMatrix* massmatrix,
          CORE::LINALG::SerialDenseVector* inertia_force);

      /** \brief get Jacobi factor ds/dxi(xi) at xi \in [-1;1]
       *
       *  \author grill
       *  \date 08/16 */
      template <unsigned int nnodecl, unsigned int vpernode>
      double GetJacobiFacAtXi(const double& xi) const
      {
        /* ||dr_0/ds(xi)||=1 because s is arc-length parameter => ||dr_0/dxi(xi)|| * dxi/ds(xi) = 1
         * => JacobiFac(xi) = ds/dxi(xi) = ||dr_0/dxi(xi)|| */
        CORE::LINALG::Matrix<3, 1> r0_xi;
        CORE::LINALG::Matrix<1, vpernode * nnodecl, double> N_i_xi;
        CORE::LINALG::Matrix<3 * nnodecl * vpernode, 1> disp_centerline_ref;

        // fill disp_centerline_ref with reference nodal centerline positions and tangents
        for (unsigned int node = 0; node < nnodecl; node++)
        {
          for (unsigned int dim = 0; dim < 3; ++dim)
          {
            disp_centerline_ref(3 * vpernode * node + dim) = Nodes()[node]->X()[dim];
            if (vpernode == 2)
              disp_centerline_ref(3 * vpernode * node + 3 + dim) = (Tref_[node])(dim);
          }
        }

        DRT::UTILS::BEAM::EvaluateShapeFunctionDerivsAtXi<nnodecl, vpernode>(
            xi, N_i_xi, this->Shape(), this->RefLength());
        this->Calc_r_xi<nnodecl, vpernode, double>(disp_centerline_ref, N_i_xi, r0_xi);

        return r0_xi.Norm2();
      }

      /*!
       \brief Get triad (three unit base vectors) at given parameter coordinate xi
      */
      template <unsigned int nnodetriad, typename T>
      void GetTriadAtXi(CORE::LINALG::Matrix<3, 3, T>& triad, const double& xi,
          const std::vector<CORE::LINALG::Matrix<4, 1, T>>& Qnode) const
      {
        // create object of triad interpolation scheme
        Teuchos::RCP<LARGEROTATIONS::TriadInterpolationLocalRotationVectors<nnodetriad, T>>
            triad_interpolation_scheme_ptr = Teuchos::rcp(
                new LARGEROTATIONS::TriadInterpolationLocalRotationVectors<nnodetriad, T>());

        // reset scheme with nodal quaternions
        triad_interpolation_scheme_ptr->Reset(Qnode);

        triad_interpolation_scheme_ptr->GetInterpolatedTriadAtXi(triad, xi);
      }

      /** \brief get generalized interpolation matrix which yields the variation of the position and
       *         orientation at xi \in [-1,1] if multiplied with the vector of primary DoF
       * variations
       *
       *  \author grill
       *  \date 11/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void GetGeneralizedInterpolationMatrixVariationsAtXi(
          CORE::LINALG::Matrix<6, 3 * vpernode * nnodecl + 3 * nnodetriad, double>& Ivar,
          const double& xi) const;

      /** \brief get generalized interpolation matrix which yields the increments of the position
       * and orientation at xi \in [-1,1] if multiplied with the vector of primary DoF increments
       *
       *  \author grill
       *  \date 11/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void GetGeneralizedInterpolationMatrixIncrementsAtXi(
          CORE::LINALG::Matrix<6, 3 * vpernode * nnodecl + 3 * nnodetriad, double>& Iinc,
          const double& xi, const std::vector<double>& disp) const;

      //! compute material curvature at certain Gauss point according to Crisfield 1999, eq. (4.9)
      template <typename T>
      void computeK(const CORE::LINALG::Matrix<3, 1, T>& Psi_l,
          const CORE::LINALG::Matrix<3, 1, T>& Psi_l_s,
          const CORE::LINALG::Matrix<3, 1, double>& Kref, CORE::LINALG::Matrix<3, 1, T>& K) const
      {
        K.Clear();

        /* Calculation of material curvature vector according to Crisfield 1999, eq. (4.2) (this
         * equation has been derived for a different beam element formulation but is also valid for
         * the element type considered here),
         * or Jelenic 1999, paragraph on page 153 between NOTE 5 and NOTE 6*/
        CORE::LINALG::Matrix<3, 3, T> Tinv(true);
        Tinv = CORE::LARGEROTATIONS::Tinvmatrix<T>(Psi_l);
        // It is important to use the transposed matrix Tinv^T instead of Tinv (these two only
        // differ in one of three terms)
        K.MultiplyTN(Tinv, Psi_l_s);

        // mechanically relevant curvature is current curvature minus curvature in reference
        // position
        for (int i = 0; i < 3; ++i) K(i) -= Kref(i);
      }

      //! compute convected strain at certain Gauss point with according to Crisfield 1999, eq.
      //! (3.4)
      template <typename T>
      void computeGamma(const CORE::LINALG::Matrix<3, 1, T>& r_s,
          const CORE::LINALG::Matrix<3, 3, T>& Lambda,
          const CORE::LINALG::Matrix<3, 1, double>& Gammaref,
          CORE::LINALG::Matrix<3, 1, T>& Gamma) const
      {
        Gamma.Clear();

        // convected strain gamma according to Crisfield 1999, eq. (3.4)
        Gamma.MultiplyTN(Lambda, r_s);

        /* In contrary to Crisfield 1999, eq. (3.4), the current implementation allows for initial
         * values of the vector gammaref which has also a second and a third component, i.e. it
         * allows for initial shear deformation. This is the case, when the initial triad at the
         * evaluation point is not parallel to the centerline tangent vector at this point. The
         * geometrically exact beam theory does in general allow for such initial triads if they are
         * considered consistently in the reference strains. While it is standard to assume
         * vanishing initial shear strains in the space-continuous setting, the possibility of
         * initial shear strains might be advantageous for the spatially discretized problem: For
         * curved initial geometries, the nodal triad had to be determined such that the resulting
         * interpolated triad at the Gauss point would be tangential to the centerline tangent at
         * this point resulting from the centerline interpolation. In order to avoid this additional
         * effort and to allow for an independent prescription of the nodal triads (e.g. prescribed
         * by an analytical geometry definition), the approach of considering arbitrary initial
         * shear angles at the Gauss points is applied here.*/
        for (int i = 0; i < 3; ++i) Gamma(i) -= Gammaref(i);
      }

      //! push forward material stress vector and constitutive matrix to their spatial counterparts
      //! by rotation matrix Lambda
      template <typename T>
      void pushforward(const CORE::LINALG::Matrix<3, 3, T>& Lambda,
          const CORE::LINALG::Matrix<3, 1, T>& stress_mat,
          const CORE::LINALG::Matrix<3, 3, T>& C_mat, CORE::LINALG::Matrix<3, 1, T>& stress_spatial,
          CORE::LINALG::Matrix<3, 3, T>& c_spatial) const;

      /** \brief compute analytic linearization (i.e. stiffness matrix) of element force vector
       *         resulting from internal elastic forces at a certain Gauss point
       *
       *  \author grill
       *  \date 12/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void CalcStiffmatAnalyticForceContributions(CORE::LINALG::SerialDenseMatrix& stiffmatrix,
          const CORE::LINALG::Matrix<3, 1, double>& stressn,
          const CORE::LINALG::Matrix<3, 3, double>& cn,
          const CORE::LINALG::Matrix<3, 3, double>& r_s_hat,
          const LARGEROTATIONS::TriadInterpolationLocalRotationVectors<nnodetriad, double>&
              triad_intpol,
          const CORE::LINALG::Matrix<1, nnodetriad, double>& I_i,
          const CORE::LINALG::Matrix<1, vpernode * nnodecl, double>& H_i_xi, const double wgt,
          const double jacobifactor) const;

      /** \brief compute analytic linearization (i.e. stiffness matrix) of element force vector
       *         resulting from internal elastic forces at a certain Gauss point
       *
       *  \author grill
       *  \date 12/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      inline void CalcStiffmatAnalyticForceContributions(
          CORE::LINALG::SerialDenseMatrix& stiffmatrix,
          const CORE::LINALG::Matrix<3, 1, Sacado::Fad::DFad<double>>& stressn,
          const CORE::LINALG::Matrix<3, 3, Sacado::Fad::DFad<double>>& cn,
          const CORE::LINALG::Matrix<3, 3, Sacado::Fad::DFad<double>>& r_s_hat,
          const LARGEROTATIONS::TriadInterpolationLocalRotationVectors<nnodetriad,
              Sacado::Fad::DFad<double>>& triad_intpol,
          const CORE::LINALG::Matrix<1, nnodetriad, double>& I_i,
          const CORE::LINALG::Matrix<1, vpernode * nnodecl, double>& H_i_xi, const double wgt,
          const double jacobifactor) const
      {
        // this is a dummy because in case that the pre-calculated values are of type Fad,
        // we use automatic differentiation and consequently there is no need for analytic stiffmat
      }

      /** \brief compute analytic linearization (i.e. stiffness matrix) of element force vector
       *         resulting from internal elastic moments at a certain Gauss point
       *
       *  \author grill
       *  \date 12/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void CalcStiffmatAnalyticMomentContributions(CORE::LINALG::SerialDenseMatrix& stiffmatrix,
          const CORE::LINALG::Matrix<3, 1, double>& stressm,
          const CORE::LINALG::Matrix<3, 3, double>& cm,
          const LARGEROTATIONS::TriadInterpolationLocalRotationVectors<nnodetriad, double>&
              triad_intpol,
          const CORE::LINALG::Matrix<3, 1, double>& Psi_l,
          const CORE::LINALG::Matrix<3, 1, double>& Psi_l_s,
          const CORE::LINALG::Matrix<1, nnodetriad, double>& I_i,
          const CORE::LINALG::Matrix<1, nnodetriad, double>& I_i_xi, const double wgt,
          const double jacobifactor) const;

      /** \brief compute analytic linearization (i.e. stiffness matrix) of element force vector
       *         resulting from internal elastic moments at a certain Gauss point
       *
       *  \author grill
       *  \date 12/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      inline void CalcStiffmatAnalyticMomentContributions(
          CORE::LINALG::SerialDenseMatrix& stiffmatrix,
          const CORE::LINALG::Matrix<3, 1, Sacado::Fad::DFad<double>>& stressm,
          const CORE::LINALG::Matrix<3, 3, Sacado::Fad::DFad<double>>& cm,
          const LARGEROTATIONS::TriadInterpolationLocalRotationVectors<nnodetriad,
              Sacado::Fad::DFad<double>>& triad_intpol,
          const CORE::LINALG::Matrix<3, 1, Sacado::Fad::DFad<double>>& Psi_l,
          const CORE::LINALG::Matrix<3, 1, Sacado::Fad::DFad<double>>& Psi_l_s,
          const CORE::LINALG::Matrix<1, nnodetriad, double>& I_i,
          const CORE::LINALG::Matrix<1, nnodetriad, double>& I_i_xi, const double wgt,
          const double jacobifactor) const
      {
        // this is a dummy because in case that the pre-calculated values are of type Fad,
        // we use automatic differentiation and consequently there is no need for analytic stiffmat
      }

      /** \brief compute linearization (i.e. stiffness matrix) of a given force vector
       *         using automatic differentiation based on Sacado::Fad package
       *
       *  \author grill
       *  \date 12/16 */
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void CalcStiffmatAutomaticDifferentiation(CORE::LINALG::SerialDenseMatrix& stiffmatrix,
          const std::vector<CORE::LINALG::Matrix<4, 1, double>>& Qnode,
          CORE::LINALG::Matrix<3 * vpernode * nnodecl + 3 * nnodetriad, 1,
              Sacado::Fad::DFad<double>>
              forcevec) const;

      //! calculation of thermal (i.e. stochastic) and damping forces according to Brownian dynamics
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void CalcBrownianForcesAndStiff(Teuchos::ParameterList& params, std::vector<double>& vel,
          std::vector<double>& disp, CORE::LINALG::SerialDenseMatrix* stiffmatrix,
          CORE::LINALG::SerialDenseVector* force);

      //! update (total) displacement vector and set nodal triads (as quaternions)
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode, typename T>
      void UpdateDispTotLagAndNodalTriads(const std::vector<double>& disp,
          CORE::LINALG::Matrix<3 * vpernode * nnodecl, 1, T>& disp_totlag_centerline,
          std::vector<CORE::LINALG::Matrix<4, 1, T>>& Q_i);

      //! set differentiation variables for automatic differentiation via FAD
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode>
      void SetAutomaticDifferentiationVariables(
          CORE::LINALG::Matrix<3 * vpernode * nnodecl, 1, FAD>& disp_totlag_centerline,
          std::vector<CORE::LINALG::Matrix<4, 1, FAD>>& Q_i) const;

      //! extract those Dofs relevant for centerline-interpolation from total state vector
      template <unsigned int nnodecl, unsigned int vpernode, typename T>
      void ExtractCenterlineDofValuesFromElementStateVector(const std::vector<double>& dofvec,
          CORE::LINALG::Matrix<3 * vpernode * nnodecl, 1, T>& dofvec_centerline,
          bool add_reference_values = false) const;

      //! extract those Dofs relevant for triad interpolation from total state vector
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode, typename T>
      void ExtractRotVecDofValues(const std::vector<double>& dofvec,
          std::vector<CORE::LINALG::Matrix<3, 1, T>>& dofvec_rotvec) const;

      //! extract those Dofs relevant for triad interpolation from total state vector
      void ExtractRotVecDofValues(const std::vector<double>& dofvec,
          std::vector<CORE::LINALG::Matrix<3, 1, double>>& dofvec_rotvec) const;

      //! compute nodal triads from current nodal rotation vectors ("displacement", i.e. relative
      //! rotations)
      template <unsigned int nnodetriad, typename T>
      void GetNodalTriadsFromDispTheta(
          const std::vector<CORE::LINALG::Matrix<3, 1, double>>& disptheta,
          std::vector<CORE::LINALG::Matrix<4, 1, T>>& Qnode) const;

     public:
      //! compute nodal triads either from current nodal rotation vectors ("displacement", i.e.
      //! relative rotations)
      //  or from full displacement state vector of the element (decision based on size of given STL
      //  vector)
      template <unsigned int nnodetriad, typename T>
      void GetNodalTriadsFromFullDispVecOrFromDispTheta(
          const std::vector<T>& dispvec, std::vector<CORE::LINALG::Matrix<4, 1, T>>& Qnode) const;

     private:
      //! compute vector with nnodetriad elements, who represent the 3x3-matrix-shaped interpolation
      //  function \tilde{I}^nnode at a certain point xi according to (3.18), Jelenic 1999
      template <unsigned int nnodetriad, typename T>
      void ComputeGeneralizedNodalRotationInterpolationMatrixFromNodalTriads(
          const std::vector<CORE::LINALG::Matrix<4, 1, T>>& Qnode, const double xi,
          std::vector<CORE::LINALG::Matrix<3, 3, T>>& Itilde) const;

      //! lump mass matrix
      template <unsigned int nnode>
      void lumpmass(CORE::LINALG::SerialDenseMatrix* massmatrix);

     public:
      //! determine Gauss rule from required type of integration and parameter list
      CORE::FE::GaussRule1D MyGaussRule(const IntegrationPurpose intpurpose) const;

      //@}

     private:
      //! @name Methods for Brownian dynamics simulations

      //! computes rotational damping forces and stiffness
      template <unsigned int nnodetriad, unsigned int nnodecl, unsigned int vpernode,
          unsigned int ndim>
      void EvaluateRotationalDamping(Teuchos::ParameterList& params,  //!< parameter list
          const std::vector<CORE::LINALG::Matrix<4, 1, double>>& Qnode,
          CORE::LINALG::SerialDenseMatrix* stiffmatrix,  //!< element stiffness matrix
          CORE::LINALG::SerialDenseVector* force);       //!< element internal force vector

      //! computes translational damping forces and stiffness
      template <unsigned int nnodecl, unsigned int vpernode,
          unsigned int ndim>  // number of nodes, number of dimensions of embedding space, number of
                              // degrees of freedom per node
      void EvaluateTranslationalDamping(Teuchos::ParameterList& params,  //!< parameter list
          const CORE::LINALG::Matrix<ndim * vpernode * nnodecl, 1, double>& vel_centerline,
          const CORE::LINALG::Matrix<ndim * vpernode * nnodecl, 1, double>& disp_totlag_centerline,
          CORE::LINALG::SerialDenseMatrix* stiffmatrix,   //!< element stiffness matrix
          CORE::LINALG::SerialDenseVector* force) const;  //!< element internal force vector

      //! computes stochastic translational forces and resulting stiffness
      template <unsigned int nnodecl, unsigned int vpernode, unsigned int ndim,
          unsigned int randompergauss>
      void EvaluateStochasticForces(Teuchos::ParameterList& params,  //!< parameter list
          const CORE::LINALG::Matrix<ndim * vpernode * nnodecl, 1, double>& disp_totlag_centerline,
          CORE::LINALG::SerialDenseMatrix* stiffmatrix,   //!< element stiffness matrix
          CORE::LINALG::SerialDenseVector* force) const;  //!< element internal force vector

      //@}
      //! computes modified Jacobian for PTC
      void CalcStiffContributionsPTC(CORE::LINALG::SerialDenseMatrix& elemat1);

     private:
      //! storing temporary stiffness matrix for element based scaling operator in PTC method
      CORE::LINALG::SerialDenseMatrix stiff_ptc_;

      //! bool storing whether automatic differentiation shall be used for this element evaluation
      bool useFAD_;

      //! variable saving whether element has already been initialized (then isinit_ == true)
      bool isinit_;


      //! initial length of the element
      double reflength_;

      //! rotational pseudovectors at nodes in reference configuration
      std::vector<CORE::LINALG::Matrix<3, 1>> theta0node_;

      //! vector holding current tangent at the centerline nodes Todo needed?
      std::vector<CORE::LINALG::Matrix<3, 1>> Tcurrnode_;

      //! initial material curvature at Gauss points for elasticity (corresponding to \Lambda_0^t
      //! \Labmda'_0 in eq. (3.5), Crisfield 1999
      std::vector<CORE::LINALG::Matrix<3, 1>> KrefGP_;

      //! initial axial tension (always zero) and shear deformation at Gauss points for elasticity
      //! (corresponding to \Lambda_0^t rprime_0 - (1,0,0) )
      std::vector<CORE::LINALG::Matrix<3, 1>> GammarefGP_;


      //! Vector holding value of Jacobi determinant for each Gauss point of integration purpose
      //! res_elastic_force
      std::vector<double> jacobiGPelastf_;

      //! Vector holding value of Jacobi determinant for each Gauss point of integration purpose
      //! res_elastic_moment
      std::vector<double> jacobiGPelastm_;

      //! Vector holding value of Jacobi determinant for each Gauss point of integration purpose
      //! res_inertia
      std::vector<double> jacobiGPmass_;

      //! Vector holding value of Jacobi determinant for each Gauss point of integration purpose
      //! res_damp_stoch
      std::vector<double> jacobiGPdampstoch_;

      //! Vector holding value of Jacobi determinant for each Gauss point of integration purpose
      //! neumann_lineload
      std::vector<double> jacobiGPneumannline_;


      //  //! nodal triads in quaternion form at the end of the preceding time step
      std::vector<CORE::LINALG::Matrix<4, 1>> Qconvnode_;
      //! nodal triads in quaternion during the current iteration step
      std::vector<CORE::LINALG::Matrix<4, 1>> Qnewnode_;

      //************** begin: Class variables required for element-based Lie-group time integration
      //*******************************
      //! triads at Gauss points for exact integration in quaternion at the end of the preceding
      //! time step (required for computation of angular velocity)
      std::vector<CORE::LINALG::Matrix<4, 1>> QconvGPmass_;
      //! current triads at Gauss points for exact integration in quaternion (required for
      //! computation of angular velocity)
      std::vector<CORE::LINALG::Matrix<4, 1>> QnewGPmass_;
      //! spatial angular velocity vector at Gauss points for exact integration at the end of the
      //! preceding time step (required for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> wconvGPmass_;
      //! current spatial angular velocity vector at Gauss points for exact integration (required
      //! for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> wnewGPmass_;
      //! spatial angular acceleration vector at Gauss points for exact integration at the end of
      //! the preceding time step (required for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> aconvGPmass_;
      //! current spatial angular acceleration vector at Gauss points for exact integration
      //! (required for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> anewGPmass_;
      //! modified spatial angular acceleration vector (according to gen-alpha time integration) at
      //! Gauss points for exact integration at the end of the preceding time step (required for
      //! computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> amodconvGPmass_;
      //! current modified spatial angular acceleration vector (according to gen-alpha time
      //! integration) at Gauss points for exact integration (required for computation of inertia
      //! terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> amodnewGPmass_;
      //! translational acceleration vector at Gauss points for exact integration at the end of the
      //! preceding time step (required for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> rttconvGPmass_;
      //! current translational acceleration vector at Gauss points for exact integration (required
      //! for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> rttnewGPmass_;
      //! modified translational acceleration vector at Gauss points for exact integration at the
      //! end of the preceding time step (required for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> rttmodconvGPmass_;
      //! modified current translational acceleration vector at Gauss points for exact integration
      //! (required for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> rttmodnewGPmass_;
      //! translational velocity vector at Gauss points for exact integration at the end of the
      //! preceding time step (required for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> rtconvGPmass_;
      //! current translational velocity vector at Gauss points for exact integration (required for
      //! computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> rtnewGPmass_;
      //! translational displacement vector at Gauss points for exact integration at the end of the
      //! preceding time step (required for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> rconvGPmass_;
      //! current translational displacement vector at Gauss points for exact integration (required
      //! for computation of inertia terms)
      std::vector<CORE::LINALG::Matrix<3, 1>> rnewGPmass_;
      //************** end: Class variables required for element-based Lie-group time integration
      //*******************************//

      //! triads at Gauss points for integration of damping/stochastic forces in quaternion form at
      //! the end of the preceding time step (required for computation of angular velocity)
      std::vector<CORE::LINALG::Matrix<4, 1>> QconvGPdampstoch_;
      //! current triads at Gauss points for integration of damping/stochastic forces in quaternion
      //! form (required for computation of angular velocity)
      std::vector<CORE::LINALG::Matrix<4, 1>> QnewGPdampstoch_;

      //!@name variables only needed/used for output purposes. Note: No need to pack and unpack
      //! @{

      //! internal (elastic) energy of element
      double Eint_;

      //! kinetic energy of element
      double Ekin_;

      //! kinetic energy from rotational dofs part1
      double Ekintorsion_;

      //! kinetic energy from rotational dofs part2
      double Ekinbending_;

      //! kinetic energy from translational dofs
      double Ekintrans_;

      //! angular momentum of the element
      CORE::LINALG::Matrix<3, 1> L_;

      //! linear momentum of the element
      CORE::LINALG::Matrix<3, 1> P_;

      //! norm of maximal bending curvature occurring in this element Todo obsolete?
      double Kmax_;

      //! strain resultant values at GPs
      std::vector<double> axial_strain_GP_elastf_;
      std::vector<double> shear_strain_2_GP_elastf_;
      std::vector<double> shear_strain_3_GP_elastf_;

      std::vector<double> twist_GP_elastm_;
      std::vector<double> curvature_2_GP_elastm_;
      std::vector<double> curvature_3_GP_elastm_;

      //! material stress resultant values at GPs
      std::vector<double> material_axial_force_GP_elastf_;
      std::vector<double> material_shear_force_2_GP_elastf_;
      std::vector<double> material_shear_force_3_GP_elastf_;

      std::vector<double> material_torque_GP_elastm_;
      std::vector<double> material_bending_moment_2_GP_elastm_;
      std::vector<double> material_bending_moment_3_GP_elastm_;

      //! spatial stress resultant values at GPs
      std::vector<double> spatial_x_force_GP_elastf_;
      std::vector<double> spatial_y_force_2_GP_elastf_;
      std::vector<double> spatial_z_force_3_GP_elastf_;

      std::vector<double> spatial_x_moment_GP_elastm_;
      std::vector<double> spatial_y_moment_2_GP_elastm_;
      std::vector<double> spatial_z_moment_3_GP_elastm_;

      //! @}
    };

    // << operator
    std::ostream& operator<<(std::ostream& os, const DRT::Element& ele);


  }  // namespace ELEMENTS
}  // namespace DRT


BACI_NAMESPACE_CLOSE

#endif