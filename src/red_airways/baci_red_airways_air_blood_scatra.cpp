/*---------------------------------------------------------------------*/
/*! \file

\brief Incomplete! - Purpose: Implements RedAirBloodScatra element


\level 3

*/
/*---------------------------------------------------------------------*/

#include "baci_io_linedefinition.hpp"
#include "baci_lib_discret.hpp"
#include "baci_red_airways_elementbase.hpp"
#include "baci_utils_exceptions.hpp"

FOUR_C_NAMESPACE_OPEN

using namespace CORE::FE;

DRT::ELEMENTS::RedAirBloodScatraType DRT::ELEMENTS::RedAirBloodScatraType::instance_;

DRT::ELEMENTS::RedAirBloodScatraType& DRT::ELEMENTS::RedAirBloodScatraType::Instance()
{
  return instance_;
}


CORE::COMM::ParObject* DRT::ELEMENTS::RedAirBloodScatraType::Create(const std::vector<char>& data)
{
  DRT::ELEMENTS::RedAirBloodScatra* object = new DRT::ELEMENTS::RedAirBloodScatra(-1, -1);
  object->Unpack(data);
  return object;
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::RedAirBloodScatraType::Create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == "RED_AIR_BLOOD_SCATRA")
  {
    Teuchos::RCP<DRT::Element> ele = Teuchos::rcp(new DRT::ELEMENTS::RedAirBloodScatra(id, owner));
    return ele;
  }
  return Teuchos::null;
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::RedAirBloodScatraType::Create(
    const int id, const int owner)
{
  Teuchos::RCP<DRT::Element> ele = Teuchos::rcp(new DRT::ELEMENTS::RedAirBloodScatra(id, owner));
  return ele;
}


void DRT::ELEMENTS::RedAirBloodScatraType::SetupElementDefinition(
    std::map<std::string, std::map<std::string, INPUT::LineDefinition>>& definitions)
{
  std::map<std::string, INPUT::LineDefinition>& defs = definitions["RED_AIR_BLOOD_SCATRA"];

  defs["LINE2"] = INPUT::LineDefinition::Builder()
                      .AddIntVector("LINE2", 2)
                      .AddNamedDouble("DiffusionCoefficient")
                      .AddNamedDouble("WallThickness")
                      .AddNamedDouble("PercentageOfDiffusionArea")
                      .Build();
}


/*----------------------------------------------------------------------*
 |  ctor (public)                                           ismail 05/13|
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::RedAirBloodScatra::RedAirBloodScatra(int id, int owner) : DRT::Element(id, owner) {}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                      ismail 05/13|
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::RedAirBloodScatra::RedAirBloodScatra(const DRT::ELEMENTS::RedAirBloodScatra& old)
    : DRT::Element(old), elemParams_(old.elemParams_), generation_(old.generation_)
{
}

/*----------------------------------------------------------------------*
 |  Deep copy this instance of RedAirBloodScatra and return pointer             |
 |  to it                                                      (public) |
 |                                                         ismail 05/13 |
 *----------------------------------------------------------------------*/
DRT::Element* DRT::ELEMENTS::RedAirBloodScatra::Clone() const
{
  DRT::ELEMENTS::RedAirBloodScatra* newelement = new DRT::ELEMENTS::RedAirBloodScatra(*this);
  return newelement;
}

/*----------------------------------------------------------------------*
 |                                                             (public) |
 |                                                         ismail 05/13 |
 *----------------------------------------------------------------------*/
CORE::FE::CellType DRT::ELEMENTS::RedAirBloodScatra::Shape() const
{
  switch (NumNode())
  {
    case 2:
      return CORE::FE::CellType::line2;
    case 3:
      return CORE::FE::CellType::line3;
    default:
      dserror("unexpected number of nodes %d", NumNode());
  }
}

/*----------------------------------------------------------------------*
 |  Pack data                                                  (public) |
 |                                                         ismail 05/13 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::RedAirBloodScatra::Pack(CORE::COMM::PackBuffer& data) const
{
  CORE::COMM::PackBuffer::SizeMarker sm(data);
  sm.Insert();

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data, type);

  // add base class Element
  Element::Pack(data);


  std::map<std::string, double>::const_iterator it;

  AddtoPack(data, (int)(elemParams_.size()));
  for (it = elemParams_.begin(); it != elemParams_.end(); it++)
  {
    AddtoPack(data, it->first);
    AddtoPack(data, it->second);
  }

  AddtoPack(data, generation_);

  return;
}


/*----------------------------------------------------------------------*
 |  Unpack data                                                (public) |
 |                                                         ismail 05/13 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::RedAirBloodScatra::Unpack(const std::vector<char>& data)
{
  std::vector<char>::size_type position = 0;

  CORE::COMM::ExtractAndAssertId(position, data, UniqueParObjectId());

  // extract base class Element
  std::vector<char> basedata(0);
  ExtractfromPack(position, data, basedata);
  Element::Unpack(basedata);

  std::map<std::string, double> it;
  int n = 0;

  ExtractfromPack(position, data, n);

  for (int i = 0; i < n; i++)
  {
    std::string name;
    double val;
    ExtractfromPack(position, data, name);
    ExtractfromPack(position, data, val);
    elemParams_[name] = val;
  }

  // extract generation
  ExtractfromPack(position, data, generation_);

  if (position != data.size())
    dserror("Mismatch in size of data %d <-> %d", (int)data.size(), position);

  return;
}



/*----------------------------------------------------------------------*
 |  print this element (public)                             ismail 05/13|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::RedAirBloodScatra::Print(std::ostream& os) const
{
  os << "RedAirBloodScatra ";
  Element::Print(os);

  return;
}

/*----------------------------------------------------------------------*
 |  Return names of visualization data                     ismail 05/13 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::RedAirBloodScatra::VisNames(std::map<std::string, int>& names)
{
  // Put the owner of this element into the file (use base class method for this)
  DRT::Element::VisNames(names);
}

/*----------------------------------------------------------------------*
 |  Return visualization data (public)                     ismail 02/10 |
 *----------------------------------------------------------------------*/
bool DRT::ELEMENTS::RedAirBloodScatra::VisData(const std::string& name, std::vector<double>& data)
{
  // Put the owner of this element into the file (use base class method for this)
  if (DRT::Element::VisData(name, data)) return true;

  return false;
}



/*----------------------------------------------------------------------*
 |  Get element parameters (public)                        ismail 04/10 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::RedAirBloodScatra::getParams(std::string name, double& var)
{
  std::map<std::string, double>::iterator it;
  it = elemParams_.find(name);
  if (it == elemParams_.end())
  {
    dserror("[%s] is not found with in the element variables", name.c_str());
    exit(1);
  }
  var = elemParams_[name];
}

/*----------------------------------------------------------------------*
 |  Get element parameters (public)                        ismail 03/11 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::RedAirBloodScatra::getParams(std::string name, int& var)
{
  if (name == "Generation")
  {
    var = generation_;
  }
  else
  {
    dserror("[%s] is not found with in the element INT variables", name.c_str());
    exit(1);
  }
}

/*----------------------------------------------------------------------*
 |  get vector of lines              (public)              ismail  02/13|
 *----------------------------------------------------------------------*/
std::vector<Teuchos::RCP<DRT::Element>> DRT::ELEMENTS::RedAirBloodScatra::Lines()
{
  dsassert(NumLine() == 1, "RED_AIRWAY element must have one and only one line");

  return {Teuchos::rcpFromRef(*this)};
}

FOUR_C_NAMESPACE_CLOSE
