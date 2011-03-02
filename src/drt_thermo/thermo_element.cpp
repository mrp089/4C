/*!
\file thermo_element.cpp
\brief

<pre>
Maintainer: Caroline Danowski
            danowski@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15253
</pre>
*/

/*----------------------------------------------------------------------*
 |  definitions                                                gjb 01/08|
 *----------------------------------------------------------------------*/
#ifdef CCADISCRET
#ifdef D_THERMO

/*----------------------------------------------------------------------*
 |  headers                                                    gjb 01/08|
 *----------------------------------------------------------------------*/
#include "thermo_element.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_lib/drt_dserror.H"
#include "../drt_lib/drt_globalproblem.H"
// material headers
#include "../drt_mat/fourieriso.H"
#include "../drt_mat/thermostvenantkirchhoff.H"
#include "../drt_lib/drt_linedefinition.H"

using namespace DRT::UTILS;

DRT::ELEMENTS::ThermoType DRT::ELEMENTS::ThermoType::instance_;


DRT::ParObject* DRT::ELEMENTS::ThermoType::Create( const std::vector<char> & data )
{
  DRT::ELEMENTS::Thermo* object =
    new DRT::ELEMENTS::Thermo(-1,-1);
  object->Unpack(data);
  return object;
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::ThermoType::Create( const string eletype,
                                                            const string eledistype,
                                                            const int id,
                                                            const int owner )
{
  if ( eletype=="THERMO" )
  {
    Teuchos::RCP<DRT::Element> ele = rcp(new DRT::ELEMENTS::Thermo(id,owner));
    return ele;
  }
  return Teuchos::null;
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::ThermoType::Create( const int id, const int owner )
{
  Teuchos::RCP<DRT::Element> ele = rcp(new DRT::ELEMENTS::Thermo(id,owner));
  return ele;
}


void DRT::ELEMENTS::ThermoType::NodalBlockInformation( Element * dwele, int & numdf, int & dimns, int & nv, int & np )
{
}

void DRT::ELEMENTS::ThermoType::ComputeNullSpace( DRT::Discretization & dis, std::vector<double> & ns, const double * x0, int numdf, int dimns )
{
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::ThermoBoundaryType::Create( const int id, const int owner )
{
  //return rcp(new DRT::ELEMENTS::ThermoBoundary(id,owner));
  return Teuchos::null;
}


void DRT::ELEMENTS::ThermoType::SetupElementDefinition( std::map<std::string,std::map<std::string,DRT::INPUT::LineDefinition> > & definitions )
{
  std::map<std::string,DRT::INPUT::LineDefinition>& defs = definitions["THERMO"];

  defs["HEX8"]
    .AddIntVector("HEX8",8)
    .AddNamedInt("MAT")
    ;

  defs["HEX20"]
    .AddIntVector("HEX20",20)
    .AddNamedInt("MAT")
    ;

  defs["HEX27"]
    .AddIntVector("HEX27",27)
    .AddNamedInt("MAT")
    ;

  defs["TET4"]
    .AddIntVector("TET4",4)
    .AddNamedInt("MAT")
    ;

  defs["TET10"]
    .AddIntVector("TET10",10)
    .AddNamedInt("MAT")
    ;

  defs["WEDGE6"]
    .AddIntVector("WEDGE6",6)
    .AddNamedInt("MAT")
    ;

  defs["WEDGE15"]
    .AddIntVector("WEDGE15",15)
    .AddNamedInt("MAT")
    ;

  defs["PYRAMID5"]
    .AddIntVector("PYRAMID5",5)
    .AddNamedInt("MAT")
    ;

  defs["QUAD4"]
    .AddIntVector("QUAD4",4)
    .AddNamedInt("MAT")
    ;

  defs["QUAD8"]
    .AddIntVector("QUAD8",8)
    .AddNamedInt("MAT")
    ;

  defs["QUAD9"]
    .AddIntVector("QUAD9",9)
    .AddNamedInt("MAT")
    ;

  defs["TRI3"]
    .AddIntVector("TRI3",3)
    .AddNamedInt("MAT")
    ;

  defs["TRI6"]
    .AddIntVector("TRI6",6)
    .AddNamedInt("MAT")
    ;

  defs["NURBS4"]
    .AddIntVector("NURBS4",4)
    .AddNamedInt("MAT")
    ;

  defs["NURBS9"]
    .AddIntVector("NURBS9",9)
    .AddNamedInt("MAT")
    ;

  defs["LINE2"]
    .AddIntVector("LINE2",2)
    .AddNamedInt("MAT")
    ;

  defs["LINE3"]
    .AddIntVector("LINE3",3)
    .AddNamedInt("MAT")
    ;
}


DRT::ELEMENTS::ThermoBoundaryType DRT::ELEMENTS::ThermoBoundaryType::instance_;


/*----------------------------------------------------------------------*
 |  ctor (public)                                            dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::Thermo::Thermo(int id, int owner) :
DRT::Element(id,owner),
data_(),
distype_(dis_none)
{
  return;
}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                       dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::Thermo::Thermo(const DRT::ELEMENTS::Thermo& old) :
DRT::Element(old),
data_(old.data_),
distype_(old.distype_)
{
  return;
}

/*----------------------------------------------------------------------*
 |  Deep copy this instance of Thermo and return pointer to it (public) |
 |                                                           dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::Element* DRT::ELEMENTS::Thermo::Clone() const
{
  DRT::ELEMENTS::Thermo* newelement = new DRT::ELEMENTS::Thermo(*this);
  return newelement;
}

/*----------------------------------------------------------------------*
 |  Return the shape of a Thermo element                      (public)  |
 |                                                           dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::Element::DiscretizationType DRT::ELEMENTS::Thermo::Shape() const
{
  return distype_;
}

/*----------------------------------------------------------------------*
 |  Pack data                                                  (public) |
 |                                                           dano 09/09 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Thermo::Pack(DRT::PackBuffer& data) const
{
  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data,type);
  // add base class Element
  Element::Pack(data);
  // distype
  AddtoPack(data,distype_);

  // data_
  data_.Pack(data);

  return;
}

/*----------------------------------------------------------------------*
 |  Unpack data                                                (public) |
 |                                                           dano 09/09 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Thermo::Unpack(const std::vector<char>& data)
{
  vector<char>::size_type position = 0;
  // extract type
  int type = 0;
  ExtractfromPack(position,data,type);
  dsassert(type == UniqueParObjectId(), "wrong instance type data");
  // extract base class Element
  std::vector<char> basedata(0);
  ExtractfromPack(position,data,basedata);
  Element::Unpack(basedata);
  // distype
  ExtractfromPack(position,data,distype_);

  std::vector<char> tmp(0);
  ExtractfromPack(position,data,tmp);
  data_.Unpack(tmp);

  if (position != data.size())
    dserror("Mismatch in size of data %d <-> %d",(int)data.size(),position);
  return;
}

/*----------------------------------------------------------------------*
 |  dtor (public)                                            dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::Thermo::~Thermo()
{
  return;
}

/*----------------------------------------------------------------------*
 |  print this element (public)                              dano 09/09 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Thermo::Print(std::ostream& os) const
{
  os << "Thermo element";
  Element::Print(os);
  cout << endl;
  cout << "DiscretizationType:  "<<distype_<< endl;
  cout << endl;
  cout << "Number DOF per Node: "<<numdofpernode_<< endl;
  cout << endl;
  cout << data_;
  return;
}

/*----------------------------------------------------------------------*
 |  get vector of lines            (public)                  dano 09/09 |
 *----------------------------------------------------------------------*/
std::vector<Teuchos::RCP<DRT::Element> > DRT::ELEMENTS::Thermo::Lines()
{
  // do NOT store line or surface elements inside the parent element
  // after their creation.
  // Reason: if a Redistribute() is performed on the discretization,
  // stored node ids and node pointers owned by these boundary elements might
  // have become illegal and you will get a nice segmentation fault ;-)

  // so we have to allocate new line elements:
  if (NumLine() > 1) // 3D and 2D
    return DRT::UTILS::ElementBoundaryFactory<ThermoBoundary,Thermo>(DRT::UTILS::buildLines,this);
  else
  {
    // 1D (we return the element itself)
    std::vector<Teuchos::RCP<Element> > lines(1);
    lines[0]= Teuchos::rcp(this, false);
    return lines;
  }
}

/*----------------------------------------------------------------------*
 |  get vector of surfaces (public)                          dano 09/09 |
 *----------------------------------------------------------------------*/
std::vector<Teuchos::RCP<DRT::Element> > DRT::ELEMENTS::Thermo::Surfaces()
{
  // do NOT store line or surface elements inside the parent element
  // after their creation.
  // Reason: if a Redistribute() is performed on the discretization,
  // stored node ids and node pointers owned by these boundary elements might
  // have become illegal and you will get a nice segmentation fault ;-)

  // so we have to allocate new surface elements:
  if (NumSurface() > 1) // 3D
    return DRT::UTILS::ElementBoundaryFactory<ThermoBoundary,Thermo>(DRT::UTILS::buildSurfaces,this);
  else if (NumSurface() == 1)
  {
    // 2D (we return the element itself)
    std::vector<Teuchos::RCP<Element> > surfaces(1);
    surfaces[0]= Teuchos::rcp(this, false);
    return surfaces;
  }
  else
  {
    // 1D
    dserror("Surfaces() for 1D-Thermo element not implemented");
    return DRT::Element::Surfaces();
  }
}

/*----------------------------------------------------------------------*
 |  get vector of volumes (length 1) (public)                dano 09/09|
 *----------------------------------------------------------------------*/
std::vector<Teuchos::RCP<DRT::Element> > DRT::ELEMENTS::Thermo::Volumes()
{
  if (NumVolume() == 1)
  {
    std::vector<Teuchos::RCP<Element> > volumes(1);
    volumes[0]= Teuchos::rcp(this, false);
    return volumes;
  }
  else
  {
    dserror("Surfaces() for 1D-/2D-Thermo element not implemented");
    return DRT::Element::Volumes();
  }
}

/*----------------------------------------------------------------------*
 |  Return names of visualization data (public)               dano 09/09|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Thermo::VisNames(std::map<std::string,int>& names)
{
  // Put the owner of this element into the file (use base class method for this)
  DRT::Element::VisNames(names);

  // see whether we have additional data for visualization in our container
  for (int k=0 ;k<numdofpernode_; k++)
  {
    std::ostringstream temp;
    temp << k;
  } // loop over temperatures

  return;
}

/*----------------------------------------------------------------------*
 |  Return visualization data (public)                        dano 09/09|
 *----------------------------------------------------------------------*/
bool DRT::ELEMENTS::Thermo::VisData(
  const std::string& name,
  std::vector<double>& data
  )
{
  // Put the owner of this element into the file (use base class method for this)
  if(DRT::Element::VisData(name,data))
    return true;

  for (int k = 0 ;k<numdofpernode_; k++)
  {
    std::ostringstream temp;
    temp << k;
      {
      if ((int)data.size()!=1) dserror("size mismatch");
      const double value = data_.GetDouble(name);
      data[0] = value;
      return true;
    }
  } // loop over temperatures

  return false;
}


//=======================================================================
//=======================================================================
//=======================================================================
//=======================================================================


/*----------------------------------------------------------------------*
 |  ctor (public)                                            dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::ThermoBoundary::ThermoBoundary(
  int id,
  int owner,
  int nnode,
  const int* nodeids,
  DRT::Node** nodes,
  DRT::ELEMENTS::Thermo* parent,
  const int lbeleid
  ) :
  DRT::Element(id,owner),
  parent_(parent),
  lbeleid_(lbeleid)
{
  SetNodeIds(nnode,nodeids);
  BuildNodalPointers(nodes);
  return;
}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                       dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::ThermoBoundary::ThermoBoundary(const DRT::ELEMENTS::ThermoBoundary& old) :
DRT::Element(old),
parent_(old.parent_),
lbeleid_(old.lbeleid_)
{
  return;
}

/*----------------------------------------------------------------------*
 |  Deep copy this instance return pointer to it    (public) dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::Element* DRT::ELEMENTS::ThermoBoundary::Clone() const
{
  DRT::ELEMENTS::ThermoBoundary* newelement = new DRT::ELEMENTS::ThermoBoundary(*this);
  return newelement;
}

/*----------------------------------------------------------------------*
 |  Return shape of this element                   (public)  dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::Element::DiscretizationType DRT::ELEMENTS::ThermoBoundary::Shape() const
{
  switch (NumNode())
  {
  case 2: return line2;
  case 3:
    if ((parent_->Shape() == quad8) || (parent_->Shape() == quad9))
      return line3;
    else
      return tri3;
  case 4: return quad4;
  case 6: return tri6;
  case 8: return quad8;
  case 9: return quad9;
  default:
    dserror("unexpected number of nodes %d", NumNode());
  }
  return dis_none;
}

/*----------------------------------------------------------------------*
 |  Pack data (public)                                       dano 09/09 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::ThermoBoundary::Pack(std::vector<char>& data) const
{
  dserror("This ThermoBoundary element does not support communication");

  return;
}

/*----------------------------------------------------------------------*
 |  Unpack data (public)                                     dano 09/09 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::ThermoBoundary::Unpack(const std::vector<char>& data)
{
  dserror("This ThermoBoundary element does not support communication");
  return;
}

/*----------------------------------------------------------------------*
 |  dtor (public)                                            dano 09/09 |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::ThermoBoundary::~ThermoBoundary()
{
  return;
}

/*----------------------------------------------------------------------*
 |  print this element (public)                              dano 09/09 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::ThermoBoundary::Print(std::ostream& os) const
{
  os << "ThermoBoundary ";
  Element::Print(os);
  return;
}

/*----------------------------------------------------------------------*
 |  get vector of lines (public)                             dano 09/09 |
 *----------------------------------------------------------------------*/
std::vector<Teuchos::RCP<DRT::Element> > DRT::ELEMENTS::ThermoBoundary::Lines()
{
  // do NOT store line or surface elements inside the parent element
  // after their creation.
  // Reason: if a Redistribute() is performed on the discretization,
  // stored node ids and node pointers owned by these boundary elements might
  // have become illegal and you will get a nice segmentation fault ;-)

  // so we have to allocate new line elements:
  dserror("Lines of ThermoBoundary not implemented");
  std::vector<Teuchos::RCP<DRT::Element> > lines(0);
  return lines;
}

/*----------------------------------------------------------------------*
 |  get vector of lines (public)                             dano 09/09 |
 *----------------------------------------------------------------------*/
std::vector<Teuchos::RCP<DRT::Element> > DRT::ELEMENTS::ThermoBoundary::Surfaces()
{
  // do NOT store line or surface elements inside the parent element
  // after their creation.
  // Reason: if a Redistribute() is performed on the discretization,
  // stored node ids and node pointers owned by these boundary elements might
  // have become illegal and you will get a nice segmentation fault ;-)

  // so we have to allocate new surface elements:
  dserror("Surfaces of ThermoBoundary not implemented");
  std::vector<Teuchos::RCP<DRT::Element> > surfaces(0);
  return surfaces;
}

/*----------------------------------------------------------------------*/
#endif  // #ifdef D_THERMO
#endif  // #ifdef CCADISCRET
