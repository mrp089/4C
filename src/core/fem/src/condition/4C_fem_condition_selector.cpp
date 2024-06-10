/*----------------------------------------------------------------------*/
/*! \file

\brief Split conditions into map extractors

\level 1


*/
/*----------------------------------------------------------------------*/


#include "4C_fem_condition_selector.hpp"

#include "4C_fem_condition.hpp"
#include "4C_fem_discretization.hpp"
#include "4C_linalg_utils_sparse_algebra_create.hpp"
#include "4C_utils_exceptions.hpp"

FOUR_C_NAMESPACE_OPEN


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Core::Conditions::ConditionSelector::ConditionSelector(
    const Core::FE::Discretization& dis, std::string condname)
    : dis_(dis)
{
  dis.GetCondition(condname, conds_);
  std::sort(conds_.begin(), conds_.end());
}


/*----------------------------------------------------------------------*
 | construct a selector from a given vector of conditions    fang 07/16 |
 *----------------------------------------------------------------------*/
Core::Conditions::ConditionSelector::ConditionSelector(
    const Core::FE::Discretization& dis,  //!< discretization
    const std::vector<Condition*>& conds  //!< given vector of conditions
    )
    : dis_(dis), conds_(conds)
{
  std::sort(conds_.begin(), conds_.end());
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
bool Core::Conditions::ConditionSelector::SelectDofs(
    Core::Nodes::Node* node, std::set<int>& conddofset)
{
  bool found = false;

  // put all conditioned dofs into conddofset
  if (ContainsNode(node->Id()))
  {
    std::vector<int> dof = discretization().Dof(0, node);
    for (unsigned k = 0; k < dof.size(); ++k)
    {
      // test for dof position
      if (ContainsDof(dof[k], k))
      {
        conddofset.insert(dof[k]);
        found = true;
      }
    }
  }
  return found;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
bool Core::Conditions::ConditionSelector::ContainsNode(int ngid)
{
  for (const auto& cond : conds_)
  {
    if (cond->ContainsNode(ngid))
    {
      return true;
    }
  }
  return false;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Core::Conditions::MultiConditionSelector::MultiConditionSelector() : overlapping_(false) {}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void Core::Conditions::MultiConditionSelector::SetupExtractor(const Core::FE::Discretization& dis,
    const Epetra_Map& fullmap, Core::LinAlg::MultiMapExtractor& extractor)
{
  setup_cond_dof_sets(dis);

  // Find all non-conditioned dofs by subtracting all conditioned ones.

  std::set<int> otherdofset(
      fullmap.MyGlobalElements(), fullmap.MyGlobalElements() + fullmap.NumMyElements());

  for (auto& conddofset : conddofset_)
  {
    for (const auto& dof : conddofset)
    {
      otherdofset.erase(dof);
    }
  }

  // Setup all maps. The "other" map goes first so it becomes the zeroth map
  // of the MultiMapExtractor.

  std::vector<Teuchos::RCP<const Epetra_Map>> maps;
  maps.reserve(conddofset_.size() + 1);

  maps.emplace_back(Core::LinAlg::CreateMap(otherdofset, dis.Comm()));
  for (auto& conddofset : conddofset_)
  {
    maps.emplace_back(Core::LinAlg::CreateMap(conddofset, dis.Comm()));
  }

  // MultiMapExtractor setup

  extractor.Setup(fullmap, maps);
}



/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void Core::Conditions::MultiConditionSelector::setup_cond_dof_sets(
    const Core::FE::Discretization& dis)
{
  // we get as many sets as we have selectors
  conddofset_.resize(selectors_.size());

  // for each owned node
  int numrownodes = dis.NumMyRowNodes();
  for (int i = 0; i < numrownodes; ++i)
  {
    Core::Nodes::Node* node = dis.lRowNode(i);

    // test each selector
    for (unsigned j = 0; j < selectors_.size(); ++j)
    {
      ConditionSelector& conds = *selectors_[j];

      // if the selector applies, we are done
      if (conds.SelectDofs(node, conddofset_[j]))
        if (!overlapping_) break;
    }
  }
}

FOUR_C_NAMESPACE_CLOSE
