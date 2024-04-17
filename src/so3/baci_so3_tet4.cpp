/*----------------------------------------------------------------------*/
/*! \file

\brief Solid Tet4 element

\level 1


*----------------------------------------------------------------------*/

#include "baci_so3_tet4.hpp"

#include "baci_comm_utils_factory.hpp"
#include "baci_discretization_fem_general_utils_fem_shapefunctions.hpp"
#include "baci_fiber_nodal_fiber_holder.hpp"
#include "baci_fiber_node.hpp"
#include "baci_fiber_utils.hpp"
#include "baci_global_data.hpp"
#include "baci_io_linedefinition.hpp"
#include "baci_lib_discret.hpp"
#include "baci_mat_so3_material.hpp"
#include "baci_so3_line.hpp"
#include "baci_so3_nullspace.hpp"
#include "baci_so3_prestress.hpp"
#include "baci_so3_prestress_service.hpp"
#include "baci_so3_surface.hpp"
#include "baci_so3_utils.hpp"
#include "baci_utils_exceptions.hpp"

#include <Teuchos_StandardParameterEntryValidators.hpp>

FOUR_C_NAMESPACE_OPEN


DRT::ELEMENTS::So_tet4Type DRT::ELEMENTS::So_tet4Type::instance_;

DRT::ELEMENTS::So_tet4Type& DRT::ELEMENTS::So_tet4Type::Instance() { return instance_; }

//------------------------------------------------------------------------
CORE::COMM::ParObject* DRT::ELEMENTS::So_tet4Type::Create(const std::vector<char>& data)
{
  auto* object = new DRT::ELEMENTS::So_tet4(-1, -1);
  object->Unpack(data);
  return object;
}


//------------------------------------------------------------------------
Teuchos::RCP<DRT::Element> DRT::ELEMENTS::So_tet4Type::Create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == GetElementTypeString())
  {
    Teuchos::RCP<DRT::Element> ele = Teuchos::rcp(new DRT::ELEMENTS::So_tet4(id, owner));
    return ele;
  }
  return Teuchos::null;
}


//------------------------------------------------------------------------
Teuchos::RCP<DRT::Element> DRT::ELEMENTS::So_tet4Type::Create(const int id, const int owner)
{
  Teuchos::RCP<DRT::Element> ele = Teuchos::rcp(new DRT::ELEMENTS::So_tet4(id, owner));
  return ele;
}


//------------------------------------------------------------------------
void DRT::ELEMENTS::So_tet4Type::NodalBlockInformation(
    DRT::Element* dwele, int& numdf, int& dimns, int& nv, int& np)
{
  numdf = 3;
  dimns = 6;
  nv = 3;
}

//------------------------------------------------------------------------
CORE::LINALG::SerialDenseMatrix DRT::ELEMENTS::So_tet4Type::ComputeNullSpace(
    DRT::Node& node, const double* x0, const int numdof, const int dimnsp)
{
  return ComputeSolid3DNullSpace(node, x0);
}

//------------------------------------------------------------------------
void DRT::ELEMENTS::So_tet4Type::SetupElementDefinition(
    std::map<std::string, std::map<std::string, INPUT::LineDefinition>>& definitions)
{
  std::map<std::string, INPUT::LineDefinition>& defs = definitions[GetElementTypeString()];

  defs["TET4"] = INPUT::LineDefinition::Builder()
                     .AddIntVector("TET4", 4)
                     .AddNamedInt("MAT")
                     .AddNamedString("KINEM")
                     .AddOptionalNamedDoubleVector("RAD", 3)
                     .AddOptionalNamedDoubleVector("AXI", 3)
                     .AddOptionalNamedDoubleVector("CIR", 3)
                     .AddOptionalNamedDoubleVector("FIBER1", 3)
                     .AddOptionalNamedDoubleVector("FIBER2", 3)
                     .AddOptionalNamedDoubleVector("FIBER3", 3)
                     .AddOptionalNamedDouble("GROWTHTRIG")
                     .Build();
}

/*----------------------------------------------------------------------***
 |  ctor (public)                                              maf 04/07|
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_tet4::So_tet4(int id, int owner)
    : So_base(id, owner),
      // material_(0),
      V_(-1.0),
      pstype_(INPAR::STR::PreStress::none),
      pstime_(0.0),
      time_(0.0)
{
  Teuchos::RCP<const Teuchos::ParameterList> params =
      GLOBAL::Problem::Instance()->getParameterList();
  if (params != Teuchos::null)
  {
    pstype_ = PRESTRESS::GetType();
    pstime_ = PRESTRESS::GetPrestressTime();

    DRT::ELEMENTS::UTILS::ThrowErrorFDMaterialTangent(
        GLOBAL::Problem::Instance()->StructuralDynamicParams(), GetElementTypeString());
  }
  if (PRESTRESS::IsMulf(pstype_))
    prestress_ = Teuchos::rcp(new DRT::ELEMENTS::PreStress(NUMNOD_SOTET4, NUMGPT_SOTET4, true));
}

/*----------------------------------------------------------------------***
 |  copy-ctor (public)                                         maf 04/07|
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_tet4::So_tet4(const DRT::ELEMENTS::So_tet4& old)
    : So_base(old),
      // material_(old.material_),
      V_(old.V_),
      pstype_(old.pstype_),
      pstime_(old.pstime_),
      time_(old.time_)
{
  if (PRESTRESS::IsMulf(pstype_))
    prestress_ = Teuchos::rcp(new DRT::ELEMENTS::PreStress(*(old.prestress_)));
}

/*----------------------------------------------------------------------***
 |  Deep copy this instance of Solid3 and return pointer to it (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
DRT::Element* DRT::ELEMENTS::So_tet4::Clone() const
{
  auto* newelement = new DRT::ELEMENTS::So_tet4(*this);
  return newelement;
}

/*----------------------------------------------------------------------***
 |                                                             (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
CORE::FE::CellType DRT::ELEMENTS::So_tet4::Shape() const { return CORE::FE::CellType::tet4; }

/*----------------------------------------------------------------------***
 |  Pack data                                                  (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_tet4::Pack(CORE::COMM::PackBuffer& data) const
{
  CORE::COMM::PackBuffer::SizeMarker sm(data);
  sm.Insert();

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data, type);
  // add base class Element
  So_base::Pack(data);
  // ngp_
  // AddtoPack(data,ngp_,3*sizeof(int));
  // material_
  // AddtoPack(data,material_);

  // V_
  AddtoPack(data, V_);

  // Pack prestress
  AddtoPack(data, static_cast<int>(pstype_));
  AddtoPack(data, pstime_);
  AddtoPack(data, time_);
  if (PRESTRESS::IsMulf(pstype_))
  {
    CORE::COMM::ParObject::AddtoPack(data, *prestress_);
  }
}


/*----------------------------------------------------------------------***
 |  Unpack data                                                (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_tet4::Unpack(const std::vector<char>& data)
{
  std::vector<char>::size_type position = 0;

  CORE::COMM::ExtractAndAssertId(position, data, UniqueParObjectId());

  // extract base class Element
  std::vector<char> basedata(0);
  ExtractfromPack(position, data, basedata);
  So_base::Unpack(basedata);
  // ngp_
  // ExtractfromPack(position,data,ngp_,3*sizeof(int));
  // material_
  // ExtractfromPack(position,data,material_);
  // V_
  ExtractfromPack(position, data, V_);

  // Extract prestress
  pstype_ = static_cast<INPAR::STR::PreStress>(ExtractInt(position, data));
  ExtractfromPack(position, data, pstime_);
  ExtractfromPack(position, data, time_);
  if (PRESTRESS::IsMulf(pstype_))
  {
    std::vector<char> tmpprestress(0);
    ExtractfromPack(position, data, tmpprestress);
    if (prestress_ == Teuchos::null)
      prestress_ = Teuchos::rcp(new DRT::ELEMENTS::PreStress(NUMNOD_SOTET4, NUMGPT_SOTET4, true));
    prestress_->Unpack(tmpprestress);
  }

  if (position != data.size())
    dserror("Mismatch in size of data %d <-> %d", (int)data.size(), position);
  return;
}


/*----------------------------------------------------------------------***
 |  print this element (public)                                maf 04/07|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_tet4::Print(std::ostream& os) const
{
  os << "So_tet4 ";
  Element::Print(os);
  std::cout << std::endl;
  return;
}

/*====================================================================*/
/* 4-node tetrahedra node topology*/
/*--------------------------------------------------------------------*/
/* parameter coordinates (ksi1, ksi2, ksi3) of nodes
 * of a common tetrahedron [0,1]x[0,1]x[0,1]
 *  4-node hexahedron: node 0,1,...,3
 *
 * -----------------------
 *- this is the numbering used in GiD & EXODUS!!
 *      3-
 *      |\ ---
 *      |  \    ---
 *      |    \      ---
 *      |      \        -2
 *      |        \       /\
 *      |          \   /   \
 *      |            X      \
 *      |          /   \     \
 *      |        /       \    \
 *      |      /           \   \
 *      |    /               \  \
 *      |  /                   \ \
 *      |/                       \\
 *      0--------------------------1
 */
/*====================================================================*/


/*----------------------------------------------------------------------*
|  get vector of surfaces (public)                             maf 04/07|
|  surface normals always point outward                                 |
*----------------------------------------------------------------------*/
std::vector<Teuchos::RCP<DRT::Element>> DRT::ELEMENTS::So_tet4::Surfaces()
{
  return CORE::COMM::ElementBoundaryFactory<StructuralSurface, DRT::Element>(
      CORE::COMM::buildSurfaces, *this);
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
std::vector<double> DRT::ELEMENTS::So_tet4::ElementCenterRefeCoords()
{
  // update element geometry
  DRT::Node** nodes = Nodes();
  CORE::LINALG::Matrix<NUMNOD_SOTET4, NUMDIM_SOTET4> xrefe;  // material coord. of element
  for (int i = 0; i < NUMNOD_SOTET4; ++i)
  {
    const auto& x = nodes[i]->X();
    xrefe(i, 0) = x[0];
    xrefe(i, 1) = x[1];
    xrefe(i, 2) = x[2];
  }
  const CORE::FE::CellType distype = Shape();
  CORE::LINALG::Matrix<NUMNOD_SOTET4, 1> funct;
  // Centroid of a tet with (0,1)(0,1)(0,1) is (0.25, 0.25, 0.25)
  CORE::FE::shape_function_3D(funct, 0.25, 0.25, 0.25, distype);
  CORE::LINALG::Matrix<1, NUMDIM_SOTET4> midpoint;
  // midpoint.Multiply('T','N',1.0,funct,xrefe,0.0);
  midpoint.MultiplyTN(funct, xrefe);
  std::vector<double> centercoords(3);
  centercoords[0] = midpoint(0, 0);
  centercoords[1] = midpoint(0, 1);
  centercoords[2] = midpoint(0, 2);
  return centercoords;
}

/*----------------------------------------------------------------------***++
 |  get vector of lines (public)                               maf 04/07|
 *----------------------------------------------------------------------*/
std::vector<Teuchos::RCP<DRT::Element>> DRT::ELEMENTS::So_tet4::Lines()
{
  return CORE::COMM::ElementBoundaryFactory<StructuralLine, DRT::Element>(
      CORE::COMM::buildLines, *this);
}

/*----------------------------------------------------------------------*
 |  Return names of visualization data (public)                 st 01/10|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_tet4::VisNames(std::map<std::string, int>& names)
{
  SolidMaterial()->VisNames(names);

  return;
}

/*----------------------------------------------------------------------*
 |  Return visualization data (public)                          st 01/10|
 *----------------------------------------------------------------------*/
bool DRT::ELEMENTS::So_tet4::VisData(const std::string& name, std::vector<double>& data)
{
  // Put the owner of this element into the file (use base class method for this)
  if (DRT::Element::VisData(name, data)) return true;

  return SolidMaterial()->VisData(name, data, NUMGPT_SOTET4, this->Id());
}

/*----------------------------------------------------------------------*
 |  Call post setup routine of the materials                            |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_tet4::MaterialPostSetup(Teuchos::ParameterList& params)
{
  if (DRT::FIBER::UTILS::HaveNodalFibers<CORE::FE::CellType::tet4>(Nodes()))
  {
    // This element has fiber nodes.
    // Interpolate fibers to the Gauss points and pass them to the material

    // Get shape functions
    static const std::vector<CORE::LINALG::Matrix<NUMNOD_SOTET4, 1>> shapefcts =
        so_tet4_1gp_shapefcts();

    // add fibers to the ParameterList
    // ParameterList does not allow to store a std::vector, so we have to add every gp fiber
    // with a separate key. To keep it clean, It is added to a sublist.
    DRT::FIBER::NodalFiberHolder fiberHolder;

    // Do the interpolation
    DRT::FIBER::UTILS::ProjectFibersToGaussPoints<CORE::FE::CellType::tet4>(
        Nodes(), shapefcts, fiberHolder);

    params.set("fiberholder", fiberHolder);
  }

  // Call super post setup
  So_base::MaterialPostSetup(params);

  // Cleanup ParameterList to not carry all fibers the whole simulation
  // do not throw an error if key does not exist.
  params.remove("fiberholder", false);
}

FOUR_C_NAMESPACE_CLOSE
