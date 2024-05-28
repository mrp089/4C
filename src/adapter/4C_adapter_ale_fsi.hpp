/*----------------------------------------------------------------------------*/
/*! \file

 \brief FSI Wrapper for the ALE time integration

 \level 1

 */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

#ifndef FOUR_C_ADAPTER_ALE_FSI_HPP
#define FOUR_C_ADAPTER_ALE_FSI_HPP


/*----------------------------------------------------------------------------*/
/* header inclusions */
#include "4C_config.hpp"

#include "4C_adapter_ale_wrapper.hpp"

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------------*/
/* forward declarations */
namespace ALE
{
  namespace UTILS
  {
    class MapExtractor;
  }
}  // namespace ALE

/*----------------------------------------------------------------------------*/
/* class definitions */
namespace ADAPTER
{
  /*! \brief ALE Wrapper for FSI Problems
   *
   *  Provide FSI specific ALE functionalities here by overloading the respective
   *  routines from ADAPTER::AleWrapper
   *
   *  \sa ADAPTER::Ale, ADAPTER::AleWrapper
   *
   *  \author mayr.mt \date 10/2014
   */
  class AleFsiWrapper : public AleWrapper
  {
   public:
    //! @name Construction / Destruction
    //@{

    //! constructor
    explicit AleFsiWrapper(Teuchos::RCP<Ale> ale);

    //@}

    //! communicate object at the interface
    Teuchos::RCP<const ALE::UTILS::MapExtractor> Interface() const;

    //! apply interface displacements
    void apply_interface_displacements(Teuchos::RCP<const Epetra_Vector> idisp)
    {
      interface_->InsertFSICondVector(idisp, WriteAccessDispnp());
    }

    //! get Dirichlet map extractor
    Teuchos::RCP<const CORE::LINALG::MapExtractor> GetDBCMapExtractor() override
    {
      return AleWrapper::GetDBCMapExtractor();
    }

   private:
    Teuchos::RCP<ALE::UTILS::MapExtractor> interface_;

  };  // class AleFsiWrapper
}  // namespace ADAPTER

FOUR_C_NAMESPACE_CLOSE

#endif