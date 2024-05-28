/*----------------------------------------------------------------------*/
/*! \file
 \brief implementation of the evaluation routines of the porofluidmultiphase element

   \level 3

 *----------------------------------------------------------------------*/

#ifndef FOUR_C_POROFLUIDMULTIPHASE_ELE_CALC_HPP
#define FOUR_C_POROFLUIDMULTIPHASE_ELE_CALC_HPP

#include "4C_config.hpp"

#include "4C_discretization_fem_general_utils_integration.hpp"
#include "4C_discretization_fem_general_utils_local_connectivity_matrices.hpp"
#include "4C_porofluidmultiphase_ele_action.hpp"
#include "4C_porofluidmultiphase_ele_calc_utils.hpp"
#include "4C_porofluidmultiphase_ele_interface.hpp"

FOUR_C_NAMESPACE_OPEN


namespace DRT
{
  namespace ELEMENTS
  {
    // forward declarations
    class PoroFluidMultiPhaseEleParameter;

    namespace POROFLUIDEVALUATOR
    {
      template <int, int>
      class EvaluatorInterface;
    }

    namespace POROFLUIDMANAGER
    {
      class PhaseManagerInterface;
      template <int, int>
      class VariableManagerInterface;
    }  // namespace POROFLUIDMANAGER

    /*!
    \brief implementation of evaluation routines of porous fluid multiphase  element

    This singleton class is responsible for evaluating boundary terms.
    It provides the method Evaluate(...) which performs the actual evaluation
    depending on the action provided by the global algorithm.


    \author vuong
    */
    template <CORE::FE::CellType distype>
    class PoroFluidMultiPhaseEleCalc : public PoroFluidMultiPhaseEleInterface
    {
     protected:
      /// protected constructor, since we are a Singleton.
      /// this constructor is called from a derived class
      /// -> therefore, it has to be protected instead of private
      PoroFluidMultiPhaseEleCalc(const int numdofpernode, const std::string& disname);

     public:
      //! Singleton access method
      static PoroFluidMultiPhaseEleCalc<distype>* Instance(
          const int numdofpernode, const std::string& disname);

      /*========================================================================*/
      //! @name static member variables
      /*========================================================================*/

      //! number of element nodes (nomenclature: T. Hughes, The finite element method)
      static constexpr int nen_ = CORE::FE::num_nodes<distype>;

      //! number of space dimensions
      static constexpr int nsd_ = CORE::FE::dim<distype>;

      //! number of components necessary to store second derivatives
      // 1 component  for nsd=1:  (N,xx)
      // 3 components for nsd=2:  (N,xx ; N,yy ; N,xy)
      // 6 components for nsd=3:  (N,xx ; N,yy ; N,zz ; N,xy ; N,xz ; N,yz)
      static constexpr int numderiv2_ = CORE::FE::DisTypeToNumDeriv2<distype>::numderiv2;

      //! element-type specific flag if second derivatives are needed
      static constexpr bool use2ndderiv_ =
          POROFLUIDMULTIPHASE::ELEUTILS::Use2ndDerivs<distype>::use;

      /// Evaluate the element
      /*!
        Generic virtual interface function. Called via base pointer.
       */
      int Evaluate(DRT::Element* ele, Teuchos::ParameterList& params,
          DRT::Discretization& discretization, DRT::Element::LocationArray& la,
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,
          std::vector<CORE::LINALG::SerialDenseVector*>& elevec) override;

     protected:
      /*========================================================================*/
      //! @name general framework
      /*========================================================================*/

      /// Setup element evaluation
      virtual int setup_calc(DRT::Element* ele, DRT::Discretization& discretization,
          const POROFLUIDMULTIPHASE::Action& action);

      //! evaluate action
      virtual int evaluate_action(DRT::Element* ele, Teuchos::ParameterList& params,
          DRT::Discretization& discretization, const POROFLUIDMULTIPHASE::Action& action,
          DRT::Element::LocationArray& la, std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,
          std::vector<CORE::LINALG::SerialDenseVector*>& elevec);

      //! extract element based or nodal values
      //  return extracted values of phinp
      virtual void extract_element_and_node_values(DRT::Element* ele,
          Teuchos::ParameterList& params, DRT::Discretization& discretization,
          DRT::Element::LocationArray& la);

      /// Setup element evaluation
      virtual void prepare_gauss_point_loop(
          DRT::Element* ele  ///< the element whose matrix is calculated
      );

      void gauss_point_loop(
          const CORE::FE::IntPointsAndWeights<nsd_>& intpoints,   ///< integration points
          DRT::Element* ele,                                      //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrixes to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la       //!< location array
      );

      void gauss_point_loop_od_struct(
          const CORE::FE::IntPointsAndWeights<nsd_>& intpoints,   ///< integration points
          DRT::Element* ele,                                      //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrixes to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la       //!< location array
      );

      void gauss_point_loop_od_scatra(
          const CORE::FE::IntPointsAndWeights<nsd_>& intpoints,   ///< integration points
          DRT::Element* ele,                                      //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrixes to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la       //!< location array
      );

      //! calculate matrix and rhs. Here the whole thing is hidden.
      void gauss_point_loop(DRT::Element* ele,                    //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrixes to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la       //!< location array
      );

      //! evaluate at all Gauss points and average
      void gauss_point_loop_average(DRT::Element* ele,            //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrices to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la       //!< location array
      );

      //! calculate off-diagonal fluid-struct-coupling matrix. Here the whole thing is hidden.
      void gauss_point_loop_od_struct(DRT::Element* ele,          //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrixes to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la       //!< location array
      );

      //! calculate off-diagonal fluid-scatra-coupling matrix. Here the whole thing is hidden.
      void gauss_point_loop_od_scatra(DRT::Element* ele,          //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrixes to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la       //!< location array
      );

      //! evaluate shape functions and their derivatives at current integration point
      double eval_shape_func_and_derivs_at_int_point(
          const CORE::FE::IntPointsAndWeights<nsd_>& intpoints,  //!< integration points
          const int iquad                                        //!< id of current Gauss point
      );

      //! evaluate shape functions and their derivatives at current integration point
      double eval_shape_func_and_derivs_in_parameter_space();

      // Compute Jacobian (determinant of deformation gradient) at node 'inode'
      void compute_jacobian_at_node(const int inode  //!< local node number
      );

      /*========================================================================*/
      //! @name routines for additional element evaluations (called from evaluate_action)
      /*========================================================================*/

      //! loop over nodes and evaluate element
      void node_loop(DRT::Element* ele,                           //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrixes to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la,      //!< location array
          const bool jacobian_needed            //!< necessary to compute Jacobian at node
      );

      //! evaluate just the element
      void evaluate_only_element(DRT::Element* ele,               //!< current element
          std::vector<CORE::LINALG::SerialDenseMatrix*>& elemat,  //!< element matrixes to calculate
          std::vector<CORE::LINALG::SerialDenseVector*>&
              elevec,                           //!< element rhs vectors to calculate
          DRT::Discretization& discretization,  //!< discretization
          DRT::Element::LocationArray& la       //!< location array
      );

     private:
      /*========================================================================*/
      //! @name can be very useful
      /*========================================================================*/

      //! element
      DRT::Element* ele_;

      /*========================================================================*/
      //! @name dofs and nodes
      /*========================================================================*/

      //! number of dof per node (= number of fluid phases + number of volume fractions)
      const int totalnumdofpernode_;

      //! number of fluid phases
      int numfluidphases_;

      /*========================================================================*/
      //! @name parameter lists
      /*========================================================================*/

      //! pointer to general scalar transport parameter class
      DRT::ELEMENTS::PoroFluidMultiPhaseEleParameter* para_;

      /*========================================================================*/
      //! @name Galerkin approximation and related
      /*========================================================================*/

      //! coordinates of current integration point in reference coordinates
      CORE::LINALG::Matrix<nsd_, 1> xsi_;
      //! initial node coordinates
      CORE::LINALG::Matrix<nsd_, nen_> xyze0_;
      //! current node coordinates
      CORE::LINALG::Matrix<nsd_, nen_> xyze_;
      //! array for shape functions
      CORE::LINALG::Matrix<nen_, 1> funct_;
      //! array for shape function derivatives w.r.t r,s,t
      CORE::LINALG::Matrix<nsd_, nen_> deriv_;
      //! array for second derivatives of shape function w.r.t r,s,t
      CORE::LINALG::Matrix<numderiv2_, nen_> deriv2_;
      //! global derivatives of shape functions w.r.t x,y,z
      CORE::LINALG::Matrix<nsd_, nen_> derxy_;
      //! global second derivatives of shape functions w.r.t x,y,z
      CORE::LINALG::Matrix<numderiv2_, nen_> derxy2_;

      //! transposed jacobian "dx/ds"
      CORE::LINALG::Matrix<nsd_, nsd_> xjm_;
      //! inverse of transposed jacobian "ds/dx"
      CORE::LINALG::Matrix<nsd_, nsd_> xij_;
      //! determinant of jacobian "dx/ds"
      double det_;
      //! determinant of deformation gradient "dx/dX"
      double j_;

      /*========================================================================*/
      //! @name scalar degrees of freedom and related
      /*========================================================================*/
      //! manager class for variables
      Teuchos::RCP<POROFLUIDMANAGER::VariableManagerInterface<nsd_, nen_>> variablemanager_;

      //! manager class for handling phases and corresponding DOFs
      Teuchos::RCP<POROFLUIDMANAGER::PhaseManagerInterface> phasemanager_;

      //! manager class for evaluation
      Teuchos::RCP<POROFLUIDEVALUATOR::EvaluatorInterface<nsd_, nen_>> evaluator_;
    };

  }  // namespace ELEMENTS
}  // namespace DRT

FOUR_C_NAMESPACE_CLOSE

#endif