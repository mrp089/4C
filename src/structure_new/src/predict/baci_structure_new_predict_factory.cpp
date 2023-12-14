/*-----------------------------------------------------------*/
/*! \file


\brief Factory class to build predictor objects

\level 3

*/
/*-----------------------------------------------------------*/


#include "baci_structure_new_predict_factory.H"

// supported predictor classes
#include "baci_structure_new_predict_constdisvelaccpress.H"
#include "baci_structure_new_predict_tangdis.H"

BACI_NAMESPACE_OPEN


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
STR::PREDICT::Factory::Factory()
{
  // empty
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
Teuchos::RCP<STR::PREDICT::Generic> STR::PREDICT::Factory::BuildPredictor(
    const enum INPAR::STR::PredEnum& predType) const
{
  Teuchos::RCP<STR::PREDICT::Generic> predictor = Teuchos::null;

  switch (predType)
  {
    case INPAR::STR::pred_constdis:
    case INPAR::STR::pred_constvel:
    case INPAR::STR::pred_constacc:
    case INPAR::STR::pred_constdisvelacc:
    case INPAR::STR::pred_constdispres:
    case INPAR::STR::pred_constdisvelaccpres:
      predictor = Teuchos::rcp(new STR::PREDICT::ConstDisVelAccPress());
      break;
    case INPAR::STR::pred_tangdis:
    case INPAR::STR::pred_tangdis_constfext:
      predictor = Teuchos::rcp(new STR::PREDICT::TangDis());
      break;
    case INPAR::STR::pred_vague:
    default:
      dserror("Unknown predictor type!");
      break;
  }

  return predictor;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
Teuchos::RCP<STR::PREDICT::Generic> STR::PREDICT::BuildPredictor(
    const enum INPAR::STR::PredEnum& predType)
{
  Factory factory;
  return factory.BuildPredictor(predType);
}

BACI_NAMESPACE_CLOSE
