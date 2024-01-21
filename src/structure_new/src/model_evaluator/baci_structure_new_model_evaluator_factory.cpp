/*-----------------------------------------------------------*/
/*! \file

\brief Factory to create the desired model evaluators.


\level 3

*/
/*-----------------------------------------------------------*/


#include "baci_structure_new_model_evaluator_factory.H"

#include "baci_beamcontact_str_model_evaluator_beaminteraction_old.H"
#include "baci_beaminteraction_str_model_evaluator.H"
#include "baci_browniandyn_str_model_evaluator.H"
#include "baci_cardiovascular0d_structure_new_model_evaluator.H"
#include "baci_constraint_framework_model_evaluator.H"
#include "baci_global_data.H"
#include "baci_inpar_structure.H"
#include "baci_struct_ale_str_model_evaluator.H"
#include "baci_structure_new_model_evaluator_contact.H"
#include "baci_structure_new_model_evaluator_lagpenconstraint.H"
#include "baci_structure_new_model_evaluator_meshtying.H"
#include "baci_structure_new_model_evaluator_springdashpot.H"
#include "baci_structure_new_model_evaluator_structure.H"

BACI_NAMESPACE_OPEN


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
STR::MODELEVALUATOR::Factory::Factory()
{
  // empty constructor
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
Teuchos::RCP<STR::ModelEvaluator::Map> STR::MODELEVALUATOR::Factory::BuildModelEvaluators(
    const std::set<enum INPAR::STR::ModelType>& modeltypes,
    const Teuchos::RCP<STR::MODELEVALUATOR::Generic>& coupling_model_ptr) const
{
  // create a new standard map
  Teuchos::RCP<STR::ModelEvaluator::Map> model_map = Teuchos::rcp(new STR::ModelEvaluator::Map());

  std::set<enum INPAR::STR::ModelType>::const_iterator mt_iter;
  for (mt_iter = modeltypes.begin(); mt_iter != modeltypes.end(); ++mt_iter)
  {
    switch (*mt_iter)
    {
      case INPAR::STR::model_structure:
        (*model_map)[*mt_iter] = BuildStructureModelEvaluator();
        break;
      case INPAR::STR::model_springdashpot:
        (*model_map)[*mt_iter] = Teuchos::rcp(new STR::MODELEVALUATOR::SpringDashpot());
        break;
      case INPAR::STR::model_browniandyn:
        (*model_map)[*mt_iter] = Teuchos::rcp(new STR::MODELEVALUATOR::BrownianDyn());
        break;
      case INPAR::STR::model_beaminteraction:
        (*model_map)[*mt_iter] = Teuchos::rcp(new STR::MODELEVALUATOR::BeamInteraction());
        break;
      case INPAR::STR::model_contact:
      {
        (*model_map)[*mt_iter] = BuildContactModelEvaluator();
        break;
      }
      case INPAR::STR::model_beam_interaction_old:
        (*model_map)[*mt_iter] = Teuchos::rcp(new STR::MODELEVALUATOR::BeamInteractionOld());
        break;
      case INPAR::STR::model_lag_pen_constraint:
        (*model_map)[*mt_iter] = Teuchos::rcp(new STR::MODELEVALUATOR::LagPenConstraint());
        break;
      case INPAR::STR::model_cardiovascular0d:
        (*model_map)[*mt_iter] = Teuchos::rcp(new STR::MODELEVALUATOR::Cardiovascular0D());
        break;
      case INPAR::STR::model_monolithic_coupling:
      {
        if (coupling_model_ptr.is_null())
          dserror("The monolithic coupling model evaluator is not defined.");
        (*model_map)[*mt_iter] = coupling_model_ptr;
        break;
      }
      case INPAR::STR::model_partitioned_coupling:
      {
        if (coupling_model_ptr.is_null())
          dserror("The partitioned coupling model evaluator is not defined.");
        (*model_map)[*mt_iter] = coupling_model_ptr;
        break;
      }
      case INPAR::STR::model_basic_coupling:
      {
        if (coupling_model_ptr.is_null())
          dserror("The basic coupling model evaluator is not defined.");
        (*model_map)[*mt_iter] = coupling_model_ptr;
        break;
      }
      case INPAR::STR::model_meshtying:
        (*model_map)[*mt_iter] = Teuchos::rcp(new STR::MODELEVALUATOR::Meshtying());
        break;
      case INPAR::STR::model_constraints:
        (*model_map)[*mt_iter] = Teuchos::rcp(new STR::MODELEVALUATOR::Constraints());
        break;
      default:
        dserror("Not yet implemented!");
        break;
    }
  }

  return model_map;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
Teuchos::RCP<STR::MODELEVALUATOR::Generic>
STR::MODELEVALUATOR::Factory::BuildContactModelEvaluator() const
{
  return Teuchos::rcp(new STR::MODELEVALUATOR::Contact());
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
Teuchos::RCP<STR::MODELEVALUATOR::Generic>
STR::MODELEVALUATOR::Factory::BuildStructureModelEvaluator() const
{
  Teuchos::RCP<STR::MODELEVALUATOR::Generic> structure_model_ptr = Teuchos::null;
  ProblemType probtype = DRT::Problem::Instance()->GetProblemType();
  switch (probtype)
  {
    case ProblemType::struct_ale:
    {
      structure_model_ptr = Teuchos::rcp(new STR::MODELEVALUATOR::StructAle());
      break;
    }
    default:
    {
      structure_model_ptr = Teuchos::rcp(new STR::MODELEVALUATOR::Structure());
      break;
    }
  }
  return structure_model_ptr;
}


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
Teuchos::RCP<STR::ModelEvaluator::Map> STR::MODELEVALUATOR::BuildModelEvaluators(
    const std::set<enum INPAR::STR::ModelType>& modeltypes,
    const Teuchos::RCP<STR::MODELEVALUATOR::Generic>& coupling_model_ptr)
{
  Factory factory;
  return factory.BuildModelEvaluators(modeltypes, coupling_model_ptr);
}

BACI_NAMESPACE_CLOSE
