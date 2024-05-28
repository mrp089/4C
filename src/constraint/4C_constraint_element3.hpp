/*----------------------------------------------------------------------*/
/*! \file
\brief A 3D constraint element with no physics attached
\level 2


*----------------------------------------------------------------------*/

#ifndef FOUR_C_CONSTRAINT_ELEMENT3_HPP
#define FOUR_C_CONSTRAINT_ELEMENT3_HPP


#include "4C_config.hpp"

#include "4C_comm_parobjectfactory.hpp"
#include "4C_lib_element.hpp"
#include "4C_lib_elementtype.hpp"
#include "4C_lib_node.hpp"
#include "4C_linalg_fixedsizematrix.hpp"
#include "4C_linalg_serialdensematrix.hpp"

#include <Epetra_Vector.h>
#include <Teuchos_RCP.hpp>

FOUR_C_NAMESPACE_OPEN

namespace DRT
{
  class Discretization;

  namespace ELEMENTS
  {
    // forward declarations

    class ConstraintElement3Type : public DRT::ElementType
    {
     public:
      std::string Name() const override { return "ConstraintElement3Type"; }

      static ConstraintElement3Type& Instance();

      CORE::COMM::ParObject* Create(const std::vector<char>& data) override;

      Teuchos::RCP<DRT::Element> Create(const std::string eletype, const std::string eledistype,
          const int id, const int owner) override;

      Teuchos::RCP<DRT::Element> Create(const int id, const int owner) override;

      void nodal_block_information(
          DRT::Element* dwele, int& numdf, int& dimns, int& nv, int& np) override;

      CORE::LINALG::SerialDenseMatrix ComputeNullSpace(
          DRT::Node& node, const double* x0, const int numdof, const int dimnsp) override;

     private:
      static ConstraintElement3Type instance_;
    };

    /*!
     */
    class ConstraintElement3 : public DRT::Element
    {
     public:
      //! @name Friends

      //@}
      //! @name Constructors and destructors and related methods

      /*!
      \brief Standard Constructor

      */
      ConstraintElement3(int id,  ///< A unique global id
          int owner               ///< element owner
      );

      /*!
      \brief Copy Constructor

      Makes a deep copy of a Element

      */
      ConstraintElement3(const ConstraintElement3& old);

      /*!
      \brief Deep copy this instance of ConstraintElement3 and return pointer to the copy

      The Clone() method is used from the virtual base class Element in cases
      where the type of the derived class is unknown and a copy-ctor is needed

      */
      DRT::Element* Clone() const override;

      /*!
      \brief Get shape type of element
      */
      CORE::FE::CellType Shape() const override
      {
        FOUR_C_THROW("ConstraintElement3 has no shape!");
        return CORE::FE::CellType::dis_none;
      };

      /*!
      \brief Return unique ParObject id

      every class implementing ParObject needs a unique id defined at the
      top of this file.
      */
      int UniqueParObjectId() const override
      {
        return ConstraintElement3Type::Instance().UniqueParObjectId();
      }

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


      //@}

      //! @name Access methods


      /*!
      \brief Get number of degrees of freedom of a certain node
             (implements pure virtual DRT::Element)

      The element decides how many degrees of freedom its nodes must have.
      As this may vary along a simulation, the element can redecide the
      number of degrees of freedom per node along the way for each of it's nodes
      separately.
      */
      int NumDofPerNode(const DRT::Node& node) const override { return 3; }

      /*!
      \brief Get number of degrees of freedom per element
             (implements pure virtual DRT::Element)

      The element decides how many element degrees of freedom it has.
      It can redecide along the way of a simulation.

      \note Element degrees of freedom mentioned here are dofs that are visible
            at the level of the total system of equations. Purely internal
            element dofs that are condensed internally should NOT be considered.
      */
      int num_dof_per_element() const override { return 0; }

      DRT::ElementType& ElementType() const override { return ConstraintElement3Type::Instance(); }

      //@}

      //! @name Evaluation

      /*!
      \brief Evaluate an element

      \return 0 if successful, negative otherwise
      */
      int Evaluate(Teuchos::ParameterList& params,   ///< ParameterList for communication
          DRT::Discretization& discretization,       ///< discretization
          std::vector<int>& lm,                      ///< location vector
          CORE::LINALG::SerialDenseMatrix& elemat1,  ///< first matrix to be filled by element
          CORE::LINALG::SerialDenseMatrix& elemat2,  ///< second matrix to be filled by element
          CORE::LINALG::SerialDenseVector& elevec1,  ///< third matrix to be filled by element
          CORE::LINALG::SerialDenseVector& elevec2,  ///< first vector to be filled by element
          CORE::LINALG::SerialDenseVector& elevec3   ///< second vector to be filled by element
          ) override;


      /*!
      \brief Evaluate a Neumann boundary condition

      Since the element has no physics attached this method will give a FOUR_C_THROW

      \return 0 if successful, negative otherwise
      */
      int evaluate_neumann(Teuchos::ParameterList& params,  ///< ParameterList for communication
          DRT::Discretization& discretization,              ///< discretization
          CORE::Conditions::Condition& condition,           ///< Neumann condition to evaluate
          std::vector<int>& lm,                             ///< location vector
          CORE::LINALG::SerialDenseVector& elevec1,         ///< vector to be filled by element
          CORE::LINALG::SerialDenseMatrix* elemat1 = nullptr) override;


     private:
      //! action parameters recognized by constraint element
      enum ActionType
      {
        none,
        calc_MPC_stiff,
        calc_MPC_state
      };

      //! vector of surfaces of this element (length 1)
      std::vector<DRT::Element*> surface_;

      // don't want = operator
      ConstraintElement3& operator=(const ConstraintElement3& old);

      /*!
      \brief Create matrix with material configuration for 3 dimensions and 4 nodes
      */
      inline void material_configuration(
          CORE::LINALG::Matrix<4, 3>& x  ///< nodal coords in material frame
      ) const
      {
        const int numnode = 4;
        const int numdim = 3;
        for (int i = 0; i < numnode; ++i)
        {
          for (int j = 0; j < numdim; ++j)
          {
            x(i, j) = Nodes()[i]->X()[j];
          }
        }
        return;
      }

      /*!
      \brief Create matrix with spatial configuration for 3 dimensions and 4 nodes
      */
      inline void spatial_configuration(
          CORE::LINALG::Matrix<4, 3>& x,  ///< nodal coords in spatial frame
          const std::vector<double> disp  ///< displacements
      ) const
      {
        const int numnode = 4;
        const int numdim = 3;
        for (int i = 0; i < numnode; ++i)
        {
          for (int j = 0; j < numdim; ++j)
          {
            x(i, j) = Nodes()[i]->X()[j] + disp[i * numdim + j];
          }
        }
        return;
      }

      /*!
      \brief Create matrix with spatial configuration for 3 dimensions and 2 nodes
      */
      inline void spatial_configuration(
          CORE::LINALG::Matrix<2, 3>& x,  ///< nodal coords in spatial frame
          const std::vector<double> disp  ///< displacements
      ) const
      {
        const int numnode = 2;
        const int numdim = 3;
        for (int i = 0; i < numnode; ++i)
        {
          for (int j = 0; j < numdim; ++j)
          {
            x(i, j) = Nodes()[i]->X()[j] + disp[i * numdim + j];
          }
        }
        return;
      }

      /// compute normal for 3D case using the first three nodes to specify a plane
      void compute_normal(const CORE::LINALG::Matrix<4, 3>& xc,  ///< nodal coords in spatial frame
          CORE::LINALG::Matrix<3, 1>& elenormal                  ///< resulting element normal
      );

      /// Compute normal distance between plane and fourth node
      double compute_normal_dist(
          const CORE::LINALG::Matrix<4, 3>& xc,        ///< nodal coords in spatial frame
          const CORE::LINALG::Matrix<3, 1>& elenormal  ///< element normal
      );

      /// Compute first derivative of normal distance with respect to the nodal displacements
      void compute_first_deriv(
          const CORE::LINALG::Matrix<4, 3>& xc,        ///< nodal coords in spatial frame
          CORE::LINALG::SerialDenseVector& elevector,  ///< vector to store results into
          const CORE::LINALG::Matrix<3, 1>& elenormal  ///< element normal
      );

      /// Compute first derivative of normal distance with respect to the nodal displacements
      void compute_second_deriv(
          const CORE::LINALG::Matrix<4, 3>& xc,        ///< nodal coords in spatial frame
          CORE::LINALG::SerialDenseMatrix& elematrix,  ///< vector to store results into
          const CORE::LINALG::Matrix<3, 1>& elenormal  ///< element normal
      );

      /// Compute difference of nodal displacement to masternode in given direction
      double compute_weighted_distance(
          const std::vector<double> disp,   ///< displacement vector of current node and masternode
          const std::vector<double> direct  ///< direction to weight with
      );

      /// Compute first derivatives nodal displacement to masternode in given direction
      void compute_first_deriv_weighted_distance(
          CORE::LINALG::SerialDenseVector& elevector,  ///< vector to store results into
          const std::vector<double> direct             ///< direction to weight with
      );

      /// Compute difference of spatial configuration to masternode in given direction
      double compute_weighted_distance(
          const CORE::LINALG::Matrix<2, 3> disp, const std::vector<double> direct);
    };  // class ConstraintElement3


    //=======================================================================
    //=======================================================================
    //=======================================================================
    //=======================================================================



  }  // namespace ELEMENTS
}  // namespace DRT


FOUR_C_NAMESPACE_CLOSE

#endif