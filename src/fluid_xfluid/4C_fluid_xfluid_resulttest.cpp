/*----------------------------------------------------------------------*/
/*! \file

\brief xfem based fluid result tests

\level 0

 */
/*----------------------------------------------------------------------*/

#include "4C_fluid_xfluid_resulttest.hpp"

#include "4C_fem_discretization.hpp"
#include "4C_fem_general_node.hpp"
#include "4C_fluid_xfluid.hpp"
#include "4C_fluid_xfluid_fluid.hpp"
#include "4C_global_data.hpp"
#include "4C_io_linedefinition.hpp"

#include <string>

FOUR_C_NAMESPACE_OPEN


FLD::XFluidResultTest::XFluidResultTest(const FLD::XFluid& xfluid)
    : Core::UTILS::ResultTest("XFLUID"),
      discret_(xfluid.discret_),
      velnp_(xfluid.state_->velnp_),
      node_from_zero_(true)
{
}

FLD::XFluidResultTest::XFluidResultTest(const FLD::XFluidFluid& xfluid)
    : Core::UTILS::ResultTest("XFLUID"),
      discret_(xfluid.discret_),
      velnp_(xfluid.state_->velnp_),
      coupl_discret_(xfluid.embedded_fluid_->discretization()),
      coupl_velnp_(xfluid.embedded_fluid_->velnp()),
      node_from_zero_(false)
{
  // Todo: remove the "node_from_zero" flag for fluidfluid:
  // adapt the test cases!
}

void FLD::XFluidResultTest::test_node(
    const Core::IO::InputParameterContainer& container, int& nerr, int& test_count)
{
  // care for the case of multiple discretizations of the same field type
  std::string dis = container.get<std::string>("DIS");

  int node = container.get<int>("NODE");

  // Todo: remove!
  if (node_from_zero_) node -= 1;

  if (dis == discret_->name())
  {
    test_node(container, nerr, test_count, node, discret_, velnp_);
  }
  else if (dis == coupl_discret_->name())
  {
    test_node(container, nerr, test_count, node, coupl_discret_, coupl_velnp_);
  }
  else
    return;
}

void FLD::XFluidResultTest::test_node(const Core::IO::InputParameterContainer& container, int& nerr,
    int& test_count, int node, const Teuchos::RCP<const Core::FE::Discretization>& discret,
    const Teuchos::RCP<const Core::LinAlg::Vector<double>>& velnp)
{
  int havenode(discret->have_global_node(node));
  int isnodeofanybody(0);
  discret->get_comm().SumAll(&havenode, &isnodeofanybody, 1);

  if (isnodeofanybody == 0)
  {
    FOUR_C_THROW("Node %d does not belong to discretization %s", node + 1, discret->name().c_str());
  }
  else
  {
    if (discret->have_global_node(node))
    {
      Core::Nodes::Node* actnode = discret->g_node(node);

      if (actnode->owner() != discret->get_comm().MyPID()) return;

      double result = 0.;

      const Epetra_BlockMap& velnpmap = velnp->Map();

      std::string position = container.get<std::string>("QUANTITY");
      if (position == "velx")
      {
        result = (*velnp)[velnpmap.LID(discret->dof(0, actnode, 0))];
      }
      else if (position == "vely")
      {
        result = (*velnp)[velnpmap.LID(discret->dof(0, actnode, 1))];
      }
      else if (position == "velz")
      {
        result = (*velnp)[velnpmap.LID(discret->dof(0, actnode, 2))];
      }
      else if (position == "pressure")
      {
        result = (*velnp)[velnpmap.LID(discret->dof(0, actnode, 3))];
      }
      else
      {
        FOUR_C_THROW("Quantity '%s' not supported in ale testing", position.c_str());
      }

      nerr += compare_values(result, "NODE", container);
      test_count++;
    }
  }
}

FOUR_C_NAMESPACE_CLOSE
