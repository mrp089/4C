/*----------------------------------------------------------------------*/
/*! \file

\brief pyramid shaped solid element

\level 1


*----------------------------------------------------------------------*/

#include "baci_so3_pyramid5fbar.H"

#include "baci_io_linedefinition.H"
#include "baci_lib_discret.H"
#include "baci_so3_nullspace.H"
#include "baci_so3_prestress.H"
#include "baci_so3_prestress_service.H"
#include "baci_so3_utils.H"
#include "baci_utils_exceptions.H"

BACI_NAMESPACE_OPEN

DRT::ELEMENTS::So_pyramid5fbarType DRT::ELEMENTS::So_pyramid5fbarType::instance_;


DRT::ELEMENTS::So_pyramid5fbarType& DRT::ELEMENTS::So_pyramid5fbarType::Instance()
{
  return instance_;
}

CORE::COMM::ParObject* DRT::ELEMENTS::So_pyramid5fbarType::Create(const std::vector<char>& data)
{
  auto* object = new DRT::ELEMENTS::So_pyramid5fbar(-1, -1);
  object->Unpack(data);
  return object;
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::So_pyramid5fbarType::Create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == GetElementTypeString())
  {
    Teuchos::RCP<DRT::Element> ele = Teuchos::rcp(new DRT::ELEMENTS::So_pyramid5fbar(id, owner));
    return ele;
  }
  return Teuchos::null;
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::So_pyramid5fbarType::Create(const int id, const int owner)
{
  Teuchos::RCP<DRT::Element> ele = Teuchos::rcp(new DRT::ELEMENTS::So_pyramid5fbar(id, owner));
  return ele;
}


void DRT::ELEMENTS::So_pyramid5fbarType::NodalBlockInformation(
    DRT::Element* dwele, int& numdf, int& dimns, int& nv, int& np)
{
  numdf = 3;
  dimns = 6;
  nv = 3;
  np = 0;
}

CORE::LINALG::SerialDenseMatrix DRT::ELEMENTS::So_pyramid5fbarType::ComputeNullSpace(
    DRT::Node& node, const double* x0, const int numdof, const int dimnsp)
{
  return ComputeSolid3DNullSpace(node, x0);
}

void DRT::ELEMENTS::So_pyramid5fbarType::SetupElementDefinition(
    std::map<std::string, std::map<std::string, INPUT::LineDefinition>>& definitions)
{
  std::map<std::string, INPUT::LineDefinition>& defs = definitions[GetElementTypeString()];

  defs["PYRAMID5"] = INPUT::LineDefinition::Builder()
                         .AddIntVector("PYRAMID5", 5)
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


/*----------------------------------------------------------------------*
 |  ctor (public)                                           seitz 03/15 |
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_pyramid5fbar::So_pyramid5fbar(int id, int owner)
    : DRT::ELEMENTS::So_pyramid5(id, owner)
{
  Teuchos::RCP<const Teuchos::ParameterList> params =
      GLOBAL::Problem::Instance()->getParameterList();
  if (params != Teuchos::null)
  {
    DRT::ELEMENTS::UTILS::ThrowErrorFDMaterialTangent(
        GLOBAL::Problem::Instance()->StructuralDynamicParams(), GetElementTypeString());
  }

  if (PRESTRESS::IsMulf(pstype_))
    prestress_ = Teuchos::rcp(new DRT::ELEMENTS::PreStress(NUMNOD_SOP5, NUMGPT_SOP5 + 1));
  return;
}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                      seitz 03/15 |
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_pyramid5fbar::So_pyramid5fbar(const DRT::ELEMENTS::So_pyramid5fbar& old)
    : DRT::ELEMENTS::So_pyramid5(old)
{
  return;
}

/*----------------------------------------------------------------------*
 |  Deep copy this instance of Solid3 and return pointer to it (public) |
 |                                                          seitz 03/15 |
 *----------------------------------------------------------------------*/
DRT::Element* DRT::ELEMENTS::So_pyramid5fbar::Clone() const
{
  auto* newelement = new DRT::ELEMENTS::So_pyramid5fbar(*this);
  return newelement;
}

/*----------------------------------------------------------------------*
 |  Pack data                                                  (public) |
 |                                                          seitz 03/15 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_pyramid5fbar::Pack(CORE::COMM::PackBuffer& data) const
{
  CORE::COMM::PackBuffer::SizeMarker sm(data);
  sm.Insert();

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data, type);
  // add base class So_pyramid5 Element
  DRT::ELEMENTS::So_pyramid5::Pack(data);

  return;
}

/*----------------------------------------------------------------------*
 |  Unpack data                                                (public) |
 |                                                          seitz 03/15 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_pyramid5fbar::Unpack(const std::vector<char>& data)
{
  std::vector<char>::size_type position = 0;

  CORE::COMM::ExtractAndAssertId(position, data, UniqueParObjectId());

  // extract base class So_pyramid5 Element
  std::vector<char> basedata(0);
  ExtractfromPack(position, data, basedata);
  DRT::ELEMENTS::So_pyramid5::Unpack(basedata);

  if (position != data.size())
    dserror("Mismatch in size of data %d <-> %d", (int)data.size(), position);
  return;
}



/*----------------------------------------------------------------------*
 |  print this element (public)                              seitz 03/15 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_pyramid5fbar::Print(std::ostream& os) const
{
  os << "So_pyramid5fbar ";
  Element::Print(os);
  std::cout << std::endl;
  return;
}

BACI_NAMESPACE_CLOSE
