/*----------------------------------------------------------------------------*/
/*! \file
\brief A 2D wall element for solid-part of porous medium using p1 (mixed) approach including scatra
functionality

\level 2


*/
/*---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*!

 \brief a 2D wall element for solid-part of porous medium using p1 (mixed) approach including scatra
functionality

 \level 2

*----------------------------------------------------------------------*/


#ifndef FOUR_C_W1_PORO_P1_SCATRA_HPP
#define FOUR_C_W1_PORO_P1_SCATRA_HPP

#include "baci_config.hpp"

#include "baci_inpar_scatra.hpp"
#include "baci_w1_poro_p1.hpp"

FOUR_C_NAMESPACE_OPEN

namespace DRT
{
  // forward declarations
  class Discretization;

  namespace ELEMENTS
  {
    /*!
    \brief A C++ version of a 2 dimensional solid element with modifications for porous media using
    p1 (mixed) approach including scatra functionality

    */
    template <CORE::FE::CellType distype>
    class Wall1_PoroP1Scatra : public Wall1_PoroP1<distype>
    {
      typedef DRT::ELEMENTS::Wall1_PoroP1<distype> my;

     public:
      //@}
      //! @name Constructors and destructors and related methods

      /*!
      \brief Standard Constructor

      \param id : A unique global id
      \param owner : elements owner
      */
      Wall1_PoroP1Scatra(int id, int owner);

      /*!
      \brief Copy Constructor

      Makes a deep copy of a Element

      */
      Wall1_PoroP1Scatra(const Wall1_PoroP1Scatra& old);


      //@}

      //! @name Acess methods

      /*!
      \brief Deep copy this instance of Solid3 and return pointer to the copy

      The Clone() method is used from the virtual base class Element in cases
      where the type of the derived class is unknown and a copy-ctor is needed

      */
      DRT::Element* Clone() const override;

      /*!
      \brief Return unique ParObject id

      every class implementing ParObject needs a unique id defined at the
      top of this file.
      */
      int UniqueParObjectId() const override;

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

      //! @name Access methods

      /*!
      \brief Print this element
      */
      void Print(std::ostream& os) const override;

      DRT::ElementType& ElementType() const override;

      //@}

      //! @name Input and Creation

      /*!
      \brief Read input for this element
      */
      bool ReadElement(const std::string& eletype, const std::string& eledistype,
          INPUT::LineDefinition* linedef) override;

      /// @name params
      /// return SCATRA::ImplType
      const INPAR::SCATRA::ImplType& ImplType() const { return impltype_; };

     private:
      //! scalar transport implementation type (physics)
      INPAR::SCATRA::ImplType impltype_;

     protected:
      //! don't want = operator
      Wall1_PoroP1Scatra& operator=(const Wall1_PoroP1Scatra& old);
    };
  }  // namespace ELEMENTS
}  // namespace DRT


FOUR_C_NAMESPACE_CLOSE

#endif
