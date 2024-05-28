/*-----------------------------------------------------------------------*/
/*! \file
\brief A class for performing mortar search in 2D/3D based on binarytrees

\level 1

*/
/*---------------------------------------------------------------------*/

#include "4C_mortar_binarytree.hpp"

#include "4C_lib_discret.hpp"
#include "4C_linalg_fixedsizematrix.hpp"
#include "4C_mortar_element.hpp"
#include "4C_mortar_node.hpp"

FOUR_C_NAMESPACE_OPEN


/*----------------------------------------------------------------------*
 |  ctor BinaryTreeNode (public)                              popp 10/08|
 *----------------------------------------------------------------------*/
MORTAR::BinaryTreeNode::BinaryTreeNode(MORTAR::BinaryTreeNodeType type,
    DRT::Discretization& discret, Teuchos::RCP<BinaryTreeNode> parent, std::vector<int> elelist,
    const CORE::LINALG::SerialDenseMatrix& dopnormals, const int& kdop, const int& dim,
    const bool& useauxpos, const int layer,
    std::vector<std::vector<Teuchos::RCP<BinaryTreeNode>>>& streenodesmap,
    std::vector<std::vector<Teuchos::RCP<BinaryTreeNode>>>& mtreenodesmap,
    std::vector<std::vector<Teuchos::RCP<BinaryTreeNode>>>& sleafsmap,
    std::vector<std::vector<Teuchos::RCP<BinaryTreeNode>>>& mleafsmap)
    : MORTAR::BaseBinaryTreeNode::BaseBinaryTreeNode(
          discret, elelist, dopnormals, kdop, dim, useauxpos, layer),
      type_(type),
      parent_(parent),
      streenodesmap_(streenodesmap),
      mtreenodesmap_(mtreenodesmap),
      sleafsmap_(sleafsmap),
      mleafsmap_(mleafsmap)
{
  // keep the constructor clean
  return;
}


/*----------------------------------------------------------------------*
 | get communicator (public)                                  popp 10/08|
 *----------------------------------------------------------------------*/
const Epetra_Comm& MORTAR::BinaryTreeNode::Comm() const { return discret().Comm(); }

/*----------------------------------------------------------------------*
 | Initialize tree (public)                                   popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTreeNode::InitializeTree(double& enlarge)
{
  // return if proc. has no elements!
  if (elelist().size() == 0) return;

  // calculate bounding volume
  CalculateSlabsDop();
  EnlargeGeometry(enlarge);

  // if current treenode is inner treenode
  if (type_ == SLAVE_INNER || type_ == MASTER_INNER)
  {
    // divide treenode
    DivideTreeNode();

    // check what to do with left child
    if (leftchild_->elelist().size() == 0)
    {
      FOUR_C_THROW("Processor has no leftchild elements.");
      return;
    }
    else
    {
      // if leftchild is slave leaf
      if (leftchild_->Type() == SLAVE_LEAF) sleafsmap_[0].push_back(leftchild_);
      // if leaftchild is master leaf
      if (leftchild_->Type() == MASTER_LEAF) mleafsmap_[0].push_back(leftchild_);

      // recursively initialize the whole tree
      leftchild_->InitializeTree(enlarge);
    }

    // check what to do with right child
    if (rightchild_->elelist().size() == 0)
    {
      FOUR_C_THROW("Processor has no rightchild elements.");
      return;
    }
    else
    {
      // if rightchild is slave leaf
      if (rightchild_->Type() == SLAVE_LEAF) sleafsmap_[1].push_back(rightchild_);
      // if rightchild is master leaf
      if (rightchild_->Type() == MASTER_LEAF) mleafsmap_[1].push_back(rightchild_);

      // recursively initialize the whole tree
      rightchild_->InitializeTree(enlarge);
    }
  }

  return;
}

/*----------------------------------------------------------------------*
 | Update slabs bottom up (public)                            popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTreeNode::UpdateSlabsBottomUp(double& enlarge)
{
  // if current treenode is inner node
  if (type_ == SLAVE_INNER || type_ == MASTER_INNER)
  {
    for (int k = 0; k < kdop() / 2; k++)
    {
      // for minimum
      if (leftchild_->slabs()(k, 0) <= rightchild_->slabs()(k, 0))
        slabs()(k, 0) = leftchild_->slabs()(k, 0);
      else
        slabs()(k, 0) = rightchild_->slabs()(k, 0);

      // for maximum
      if (leftchild_->slabs()(k, 1) >= rightchild_->slabs()(k, 1))
        slabs()(k, 1) = leftchild_->slabs()(k, 1);
      else
        slabs()(k, 1) = rightchild_->slabs()(k, 1);
    }
  }

  // if current treenode is leafnode
  if (type_ == SLAVE_LEAF || type_ == MASTER_LEAF)
  {
    CalculateSlabsDop();

    EnlargeGeometry(enlarge);

    // Prints Slabs to std::cout
    // PrintSlabs();
  }  // current treenode is leaf

  return;
}
/*----------------------------------------------------------------------*
 | Divide treenode (public)                                   popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTreeNode::DivideTreeNode()
{
  // map of elements belonging to left / right child treenode
  std::vector<int> leftelements(0);
  std::vector<int> rightelements(0);

  // if only 2 elements in Treenode, create new treenodes out of them
  if (elelist().size() == 2)
  {
    leftelements.push_back(elelist()[0]);
    rightelements.push_back(elelist()[1]);
  }
  // if more than 2 elements in Treenode
  else if (elelist().size() > 2)
  {
    // calculate splitting area (split along longest side)
    double lmax = 0.0;                                // max. length of sides of DOP
    int splittingnormal = -1;                         // defines side to split
    std::array<double, 3> xmedian = {0.0, 0.0, 0.0};  // coordinates of centroid

    for (int i = 0; i < kdop() / 2; ++i)
    {
      double lcurrent = abs(slabs()(i, 1) - slabs()(i, 0));
      if (lmax < lcurrent)
      {
        lmax = lcurrent;
        splittingnormal = i;
      }
    }

    // find median of centroid coordinates to divide area
    // coordinates of median
    for (int i = 0; i < dim(); ++i)
      xmedian[i] = slabs()(i, 1) - ((slabs()(i, 1) - slabs()(i, 0)) / 2);

    // compute d of ax+by+cz=d of splittingplane
    double d = xmedian[0] * dopnormals()(splittingnormal, 0) +
               xmedian[1] * dopnormals()(splittingnormal, 1) +
               xmedian[2] * dopnormals()(splittingnormal, 2);

    // split treenode into two parts
    for (int i = 0; i < ((int)elelist().size()); ++i)
    {
      bool isright = false;  // true, if element should be sorted into right treenode
      bool isleft = false;   // true, if element should be sorted into left treenode

      int gid = elelist()[i];
      DRT::Element* element = discret().gElement(gid);
      if (!element) FOUR_C_THROW("Cannot find element with gid %\n", gid);
      DRT::Node** nodes = element->Points();

      // vector of values of Hesse-Normalform of nodes of elements
      CORE::LINALG::SerialDenseVector axbycz;
      axbycz.resize(element->NumPoint());

      for (int k = 0; k < element->NumPoint(); ++k)
      {
        Node* mrtrnode = dynamic_cast<Node*>(nodes[k]);
        if (!mrtrnode) FOUR_C_THROW("Null pointer!");
        const auto& posnode = mrtrnode->X();

        // split along chosen area
        // ax+by+cz< or > d = criterion
        // compute ax+by+cz for chosen node
        if (dim() == 2)
          axbycz[k] = posnode[0] * dopnormals()(splittingnormal, 0) +
                      posnode[1] * dopnormals()(splittingnormal, 1);
        else if (dim() == 3)
          axbycz[k] = posnode[0] * dopnormals()(splittingnormal, 0) +
                      posnode[1] * dopnormals()(splittingnormal, 1) +
                      posnode[2] * dopnormals()(splittingnormal, 2);
        else
          FOUR_C_THROW("Problem dimension must be 2D or 3D!");

        if (axbycz[k] >= d) isright = true;
        if (axbycz[k] < d) isleft = true;
      }

      if (isright == false && isleft == false)
        FOUR_C_THROW("Current element could neither be sorted into left- or right-child node!");

      // if element is split through, it is sorted into left treenode
      if (isright == true && isleft == true) isright = false;

      // sort elements into child treenodes
      if (isright) rightelements.push_back(gid);
      if (isleft) leftelements.push_back(gid);
    }
  }

  // if treenode splitting algorithm was not able to divide treenode
  // successfully (i.e all elements are in one child treenode),
  // then just put one element into the other treenode
  if (leftelements.size() == 0 && rightelements.size() > 1)
  {
    leftelements.push_back(rightelements[rightelements.size() - 1]);
    rightelements.pop_back();
  }
  if (rightelements.size() == 0 && leftelements.size() > 1)
  {
    rightelements.push_back(leftelements[leftelements.size() - 1]);
    leftelements.pop_back();
  }

  // define type of newly created children treenodes
  if (elelist().size() >= 2)
  {
    // defines type of left and right TreeNode
    BinaryTreeNodeType lefttype = UNDEFINED;
    BinaryTreeNodeType righttype = UNDEFINED;

    // is the new left child treenode a leaf node?
    if (leftelements.size() == 1)
    {
      if (type_ == SLAVE_INNER)
        lefttype = SLAVE_LEAF;
      else if (type_ == MASTER_INNER)
        lefttype = MASTER_LEAF;
      else
        FOUR_C_THROW("Invalid TreeNodeType");
    }
    else
    {
      if (type_ == SLAVE_INNER)
        lefttype = SLAVE_INNER;
      else if (type_ == MASTER_INNER)
        lefttype = MASTER_INNER;
      else
        FOUR_C_THROW("Invalid TreeNodeType");
    }

    // is the new right child treenode a leaf node?
    if (rightelements.size() == 1)
    {
      if (type_ == SLAVE_INNER)
        righttype = SLAVE_LEAF;
      else if (type_ == MASTER_INNER)
        righttype = MASTER_LEAF;
      else
        FOUR_C_THROW("Invalid TreeNodeType");
    }
    else
    {
      if (type_ == SLAVE_INNER)
        righttype = SLAVE_INNER;
      else if (type_ == MASTER_INNER)
        righttype = MASTER_INNER;
      else
        FOUR_C_THROW("Invalid TreeNodeType");
    }

    // build left child treenode
    leftchild_ = Teuchos::rcp(new BinaryTreeNode(lefttype, discret(), Teuchos::rcp(this, false),
        leftelements, dopnormals(), kdop(), dim(), use_aux_pos(), (get_layer() + 1), streenodesmap_,
        mtreenodesmap_, sleafsmap_, mleafsmap_));

    // build right child treenode
    rightchild_ = Teuchos::rcp(new BinaryTreeNode(righttype, discret(), Teuchos::rcp(this, false),
        rightelements, dopnormals(), kdop(), dim(), use_aux_pos(), (get_layer() + 1),
        streenodesmap_, mtreenodesmap_, sleafsmap_, mleafsmap_));

    // update slave and mastertreenodes map
    // if parent treenode is slave
    if (type_ == SLAVE_INNER)
    {
      // if map of treenodes does not have enogh rows-->resize!
      if ((int)(streenodesmap_.size()) <= (get_layer() + 1))
        streenodesmap_.resize((get_layer() + 2));

      // put new pointers to children into map
      streenodesmap_[(get_layer() + 1)].push_back(leftchild_);
      streenodesmap_[(get_layer() + 1)].push_back(rightchild_);
    }

    // if parent treenode is master
    if (type_ == MASTER_INNER)
    {
      // if map of treenodes does not have enogh rows-->resize!
      if ((int)(mtreenodesmap_.size()) <= (get_layer() + 1))
        mtreenodesmap_.resize((get_layer() + 2));

      // put new pointers to children into map
      mtreenodesmap_[(get_layer() + 1)].push_back(leftchild_);
      mtreenodesmap_[(get_layer() + 1)].push_back(rightchild_);
    }
  }

  else
    FOUR_C_THROW("Only 1 or 0 elements in map-->TreeNode cannot be devided!!");

  return;
}

/*----------------------------------------------------------------------*
 | Print type of treenode to std::cout (public)               popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTreeNode::PrintType()
{
  switch (type_)
  {
    case SLAVE_INNER:
      std::cout << std::endl << "SLAVE_INNER ";
      break;
    case SLAVE_LEAF:
      std::cout << std::endl << "SLAVE_LEAF ";
      break;
    case MASTER_INNER:
      std::cout << std::endl << "MASTER_INNER ";
      break;
    case MASTER_LEAF:
      std::cout << std::endl << "MASTER_LEAF ";
      break;
    case NOSLAVE_ELEMENTS:
      std::cout << std::endl << "TreeNode contains no Slave-Elements=NO_SLAVEELEMENTS ";
      break;
    case NOMASTER_ELEMENTS:
      std::cout << std::endl << "TreeNode contains no Master-Elements=NO_MASTERELEMENTS ";
      break;
    case UNDEFINED:
      std::cout << std::endl << "UNDEFINED ";
      break;
    default:
      FOUR_C_THROW("Unknown MORTAR::BinaryTreeNodeType detected.");
      break;
  }

  return;
}

/*----------------------------------------------------------------------*
 |  ctor BinaryTree(public)                                   popp 10/08|
 *----------------------------------------------------------------------*/
MORTAR::BinaryTree::BinaryTree(DRT::Discretization& discret, Teuchos::RCP<Epetra_Map> selements,
    Teuchos::RCP<Epetra_Map> melements, int dim, double eps,
    INPAR::MORTAR::BinaryTreeUpdateType updatetype, bool useauxpos)
    : MORTAR::BaseBinaryTree(discret, dim, eps),
      selements_(selements),
      melements_(melements),
      updatetype_(updatetype),
      useauxpos_(useauxpos)
{
  // keep the constructor clean
  return;
}

/*----------------------------------------------------------------------*
 | initialize the binary tree                              schmidt 12/18|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::Init()
{
  // call initialization method of base class
  MORTAR::BaseBinaryTree::Init();

  init_internal_variables();

  // calculate minimal element length
  set_enlarge();

  //**********************************************************************
  // initialize binary tree root nodes
  //**********************************************************************
  // create element lists
  std::vector<int> slist;
  std::vector<int> mlist;

  for (int i = 0; i < selements_->NumMyElements(); ++i)
  {
    int gid = selements_->GID(i);
    slist.push_back(gid);
  }

  for (int i = 0; i < melements_->NumMyElements(); ++i)
  {
    int gid = melements_->GID(i);
    mlist.push_back(gid);
  }

  // check slave root node case
  if (slist.size() >= 2)
  {
    sroot_ = Teuchos::rcp(
        new BinaryTreeNode(MORTAR::SLAVE_INNER, discret(), sroot_, slist, dop_normals(), kdop(),
            dim(), useauxpos_, 0, streenodesmap_, mtreenodesmap_, sleafsmap_, mleafsmap_));

    // do initialization
    streenodesmap_[0].push_back(sroot_);
    sroot_->InitializeTree(enlarge());
  }
  else if (slist.size() == 1)
  {
    sroot_ =
        Teuchos::rcp(new BinaryTreeNode(MORTAR::SLAVE_LEAF, discret(), sroot_, slist, dop_normals(),
            kdop(), dim(), useauxpos_, 0, streenodesmap_, mtreenodesmap_, sleafsmap_, mleafsmap_));

    // trivial initialization
    streenodesmap_[0].push_back(sroot_);
    sleafsmap_[0].push_back(sroot_);
  }
  else
  {
    sroot_ = Teuchos::rcp(
        new BinaryTreeNode(MORTAR::NOSLAVE_ELEMENTS, discret(), sroot_, slist, dop_normals(),
            kdop(), dim(), useauxpos_, 0, streenodesmap_, mtreenodesmap_, sleafsmap_, mleafsmap_));

    // trivial initialization
    streenodesmap_[0].push_back(sroot_);
  }

  // check master root node case
  if (mlist.size() >= 2)
  {
    mroot_ = Teuchos::rcp(
        new BinaryTreeNode(MORTAR::MASTER_INNER, discret(), mroot_, mlist, dop_normals(), kdop(),
            dim(), useauxpos_, 0, streenodesmap_, mtreenodesmap_, sleafsmap_, mleafsmap_));

    // do initialization
    mtreenodesmap_[0].push_back(mroot_);
    mroot_->InitializeTree(enlarge());
  }
  else if (mlist.size() == 1)
  {
    mroot_ = Teuchos::rcp(
        new BinaryTreeNode(MORTAR::MASTER_LEAF, discret(), mroot_, mlist, dop_normals(), kdop(),
            dim(), useauxpos_, 0, streenodesmap_, mtreenodesmap_, sleafsmap_, mleafsmap_));

    // trivial initialization
    mtreenodesmap_[0].push_back(mroot_);
    mleafsmap_[0].push_back(mroot_);
  }
  else
  {
    mroot_ = Teuchos::rcp(
        new BinaryTreeNode(MORTAR::NOMASTER_ELEMENTS, discret(), mroot_, mlist, dop_normals(),
            kdop(), dim(), useauxpos_, 0, streenodesmap_, mtreenodesmap_, sleafsmap_, mleafsmap_));

    // trivial initialization / error
    mtreenodesmap_[0].push_back(mroot_);

    // no error: round robin loop can handle this case!
    // FOUR_C_THROW("No master element for Binarytree initialization on this processor");
  }

  /*
  // print binarytree to std::cout
  for (int k=0;k<Comm().NumProc();++k)
  {
    Comm().Barrier();
    if (Comm().MyPID()==k)
    {
      std::cout << "\n" << Comm().MyPID() << " Print tree with direct print function" << std::endl;
      std::cout <<"\n" <<Comm().MyPID()<< " Slave Tree:";
      print_tree(sroot_);
      std::cout <<"\n" <<Comm().MyPID()<< " Master Tree:";
      print_tree(mroot_);
    }
    Comm().Barrier();
  }

  for (int k=0;k<Comm().NumProc();++k)
  {
    Comm().Barrier();
    if (Comm().MyPID()==k)
    {
      std::cout << "\n" << Comm().MyPID() << " Print tree with print function of slave and master
  treemap" << std::endl; std::cout <<"\n" <<Comm().MyPID()<< " Slave Tree:";
      print_tree_of_map(streenodesmap_);
      std::cout <<"\n" <<Comm().MyPID()<< " Master Tree:";
      print_tree_of_map(mtreenodesmap_);
    }
    Comm().Barrier();
  }
  */

  return;
}

/*----------------------------------------------------------------------*
 |  Initialize internal variables (private)                schmidt 01/19|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::init_internal_variables()
{
  // initialize sizes
  streenodesmap_.resize(1);
  mtreenodesmap_.resize(1);
  couplingmap_.resize(2);
  sleafsmap_.resize(2);
  mleafsmap_.resize(2);

  return;
}

/*----------------------------------------------------------------------*
 | clear found search elements from last eval.               farah 01/16|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::init_search_elements()
{
  // loop over all elements to reset candidates / search lists
  // (use standard slave column map)
  for (int i = 0; i < selements_->NumMyElements(); ++i)
  {
    int gid = selements_->GID(i);
    DRT::Element* ele = discret().gElement(gid);
    if (!ele) FOUR_C_THROW("Cannot find ele with gid %i", gid);
    MORTAR::Element* sele = dynamic_cast<MORTAR::Element*>(ele);

    sele->MoData().SearchElements().resize(0);
  }
}


/*----------------------------------------------------------------------*
 | evaluate search to get sele to mele info (public)         farah 01/16|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::evaluate_search()
{
  // We have to explicitly call the updating routine, i.e. update_tree_top_down() or
  // update_tree_bottom_up() before calling the search routine evaluate_search(...). For very large
  // mesh tying problems, the BottomUp version is faster and therefore preferable.

  // init search elements
  init_search_elements();

  // calculate minimal element length
  set_enlarge();

  // update binary tree according to update type
  switch (updatetype_)
  {
    case INPAR::MORTAR::binarytree_top_down:
      update_tree_top_down();
      break;
    case INPAR::MORTAR::binarytree_bottom_up:
      update_tree_bottom_up();
      break;
    default:
      FOUR_C_THROW("MORTAR::BinaryTreeUpdateType has to be bottom up or top down!");
      break;
  }

#ifdef MORTARGMSHCTN
  for (int i = 0; i < (int)(binarytree_->coupling_map().size()); i++)
    binarytree_->coupling_map()[i].clear();
  binarytree_->coupling_map().clear();
  binarytree_->coupling_map().resize(2);
#endif
  // evaluate search algorithm
  evaluate_search(sroot_, mroot_);

  return;
}


/*----------------------------------------------------------------------*
 | get communicator (public)                                  popp 10/08|
 *----------------------------------------------------------------------*/
const Epetra_Comm& MORTAR::BinaryTree::comm() const { return discret().Comm(); }

/*----------------------------------------------------------------------*
 | Find min. length of master and slave elements (public)     popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::set_enlarge()
{
  double lmin = 1.0e12;

  // calculate mininmal length of slave elements
  for (int i = 0; i < selements_->NumMyElements(); ++i)
  {
    int gid = selements_->GID(i);
    DRT::Element* element = discret().gElement(gid);
    if (!element) FOUR_C_THROW("Cannot find element with gid %\n", gid);
    MORTAR::Element* mrtrelement = dynamic_cast<MORTAR::Element*>(element);
    double mincurrent = mrtrelement->MinEdgeSize();
    if (mincurrent < lmin) lmin = mincurrent;
  }

  // calculate minimal length of master elements
  for (int i = 0; i < melements_->NumMyElements(); ++i)
  {
    int gid = melements_->GID(i);
    DRT::Element* element = discret().gElement(gid);
    if (!element) FOUR_C_THROW("Cannot find element with gid %\n", gid);
    MORTAR::Element* mrtrelement = dynamic_cast<MORTAR::Element*>(element);
    double mincurrent = mrtrelement->MinEdgeSize();
    if (mincurrent < lmin) lmin = mincurrent;
  }

  if (lmin <= 0.0) FOUR_C_THROW("Minimal element length <= 0!");

  // set the class variables
  enlarge() = eps() * lmin;

  return;
}

/*----------------------------------------------------------------------*
 | Print tree (public)                                        popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::print_tree(Teuchos::RCP<BinaryTreeNode> treenode)
{
  // if treenode has no elements (NOSLAVE_ELEMENTS,NOMASTER_ELEMENTS)
  if (treenode->Type() == NOSLAVE_ELEMENTS || treenode->Type() == NOMASTER_ELEMENTS)
  {
    std::cout << "\n" << comm().MyPID() << " Tree has no element to print";
    return;
  }
  std::cout << "\n"
            << comm().MyPID() << " Tree at layer: " << treenode->get_layer() << " Elements: ";
  for (int i = 0; i < (int)(treenode->elelist().size()); i++)
    std::cout << " " << treenode->elelist()[i];

  // while treenode is inner node
  if (treenode->Type() == SLAVE_INNER || treenode->Type() == MASTER_INNER)
  {
    print_tree(treenode->Leftchild());
    print_tree(treenode->Rightchild());
  }

  return;
}

/*----------------------------------------------------------------------*
 | Print tree with treenodesmap_ (public)                     popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::print_tree_of_map(
    std::vector<std::vector<Teuchos::RCP<BinaryTreeNode>>>& treenodesmap)
{
  // print tree, elements listet in brackets (), belong to one treenode!
  for (int i = 0; i < (int)(treenodesmap.size()); i++)
  {
    std::cout << "\n" << comm().MyPID() << " Tree at layer: " << i << " Elements: ";
    for (int k = 0; k < (int)(treenodesmap[i].size()); k++)
    {
      Teuchos::RCP<BinaryTreeNode> currentnode = treenodesmap[i][k];
      std::cout << " (";
      for (int l = 0; l < (int)(currentnode->elelist().size()); l++)
      {
        std::cout << currentnode->elelist()[l] << " ";
      }
      std::cout << ") ";
    }
  }

  return;
}

/*----------------------------------------------------------------------*
 | Update tree topdown (public)                               popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::evaluate_update_tree_top_down(Teuchos::RCP<BinaryTreeNode> treenode)
{
  // if no slave element on proc-->return
  if (treenode->elelist().size() == 0) return;

  treenode->CalculateSlabsDop();
  treenode->EnlargeGeometry(enlarge());

  if (treenode->Type() == SLAVE_INNER || treenode->Type() == MASTER_INNER)
  {
    evaluate_update_tree_top_down(treenode->Leftchild());
    evaluate_update_tree_top_down(treenode->Rightchild());
  }

  return;
}

/*----------------------------------------------------------------------*
 | Update tree bottom up based on list (public)               popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::evaluate_update_tree_bottom_up(
    std::vector<std::vector<Teuchos::RCP<BinaryTreeNode>>>& treenodesmap)
{
  // update tree bottom up (for every treelayer)
  for (int i = ((int)(treenodesmap.size() - 1)); i >= 0; i = i - 1)
  {
    for (int j = 0; j < (int)(treenodesmap[i].size()); j++)
      treenodesmap[i][j]->UpdateSlabsBottomUp(enlarge());
  }

  return;
}

/*----------------------------------------------------------------------*
 | Search for contact (public)                                popp 10/08|
 *----------------------------------------------------------------------*/
void MORTAR::BinaryTree::evaluate_search(
    Teuchos::RCP<BinaryTreeNode> streenode, Teuchos::RCP<BinaryTreeNode> mtreenode)
{
  // tree needs to be updated before running contact search!

  // if there are no elements
  if (streenode->Type() == NOSLAVE_ELEMENTS || mtreenode->Type() == NOMASTER_ELEMENTS) return;

  // check if tree nodes intercept (they only intercept if ALL slabs intercept!)
  int nintercepts = 0;

  for (int i = 0; i < kdop() / 2; ++i)
  {
    if (streenode->slabs()(i, 0) <= mtreenode->slabs()(i, 0))
    {
      if (streenode->slabs()(i, 1) >= mtreenode->slabs()(i, 0))
        nintercepts++;
      else if (streenode->slabs()(i, 1) >= mtreenode->slabs()(i, 1))
        nintercepts++;
    }
    else if (streenode->slabs()(i, 0) >= mtreenode->slabs()(i, 0))
    {
      if (mtreenode->slabs()(i, 1) >= streenode->slabs()(i, 1))
        nintercepts++;
      else if (mtreenode->slabs()(i, 1) >= streenode->slabs()(i, 0))
        nintercepts++;
    }
  }

  // tree nodes intercept
  if (nintercepts == kdop() / 2)
  {
    // slave and master tree nodes are inner nodes
    if (streenode->Type() == SLAVE_INNER && mtreenode->Type() == MASTER_INNER)
    {
      evaluate_search(streenode->Leftchild(), mtreenode->Leftchild());
      evaluate_search(streenode->Leftchild(), mtreenode->Rightchild());
      evaluate_search(streenode->Rightchild(), mtreenode->Leftchild());
      evaluate_search(streenode->Rightchild(), mtreenode->Rightchild());
    }

    // slave tree node is inner, master tree node is leaf
    if (streenode->Type() == SLAVE_INNER && mtreenode->Type() == MASTER_LEAF)
    {
      evaluate_search(streenode->Leftchild(), mtreenode);
      evaluate_search(streenode->Rightchild(), mtreenode);
    }

    // slave tree node is leaf,  master tree node is inner
    if (streenode->Type() == SLAVE_LEAF && mtreenode->Type() == MASTER_INNER)
    {
      evaluate_search(streenode, mtreenode->Leftchild());
      evaluate_search(streenode, mtreenode->Rightchild());
    }

    // both tree nodes are leaf --> feasible pair
    if (streenode->Type() == SLAVE_LEAF && mtreenode->Type() == MASTER_LEAF)
    {
      int sgid = (int)streenode->elelist()[0];  // global id of slave element
      int mgid = (int)mtreenode->elelist()[0];  // global id of master element
      DRT::Element* element = discret().gElement(sgid);
      MORTAR::Element* selement = dynamic_cast<MORTAR::Element*>(element);
      selement->AddSearchElements(mgid);
    }
  }

#ifdef MORTARGMSHCTN  // for plotting contacting treenodes
  if (streenode->Type() == SLAVE_LEAF && mtreenode->Type() == MASTER_LEAF &&
      nintercepts == kdop_ / 2)
  {
    couplingmap_[0].push_back(streenode);
    couplingmap_[1].push_back(mtreenode);
  }
#endif  // #ifdef MORTARGMSHCTN

  return;
}

FOUR_C_NAMESPACE_CLOSE