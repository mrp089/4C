/*!----------------------------------------------------------------------
\file so_shw6.cpp
\brief

<pre>
Maintainer: Moritz Frenzel
            frenzel@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15240
</pre>

*----------------------------------------------------------------------*/
#ifdef D_SOLID3
#ifdef CCADISCRET

#include "so_shw6.H"
#include "so_weg6.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_lib/drt_linedefinition.H"

using namespace DRT::UTILS;


DRT::ELEMENTS::So_shw6Type DRT::ELEMENTS::So_shw6Type::instance_;


DRT::ParObject* DRT::ELEMENTS::So_shw6Type::Create( const std::vector<char> & data )
{
  DRT::ELEMENTS::So_shw6* object =
    new DRT::ELEMENTS::So_shw6(-1,-1);
  object->Unpack(data);
  return object;
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::So_shw6Type::Create( const string eletype,
                                                            const string eledistype,
                                                            const int id,
                                                            const int owner )
{
  if ( eletype=="SOLIDSHW6" )
  {
    Teuchos::RCP<DRT::Element> ele = rcp(new DRT::ELEMENTS::So_shw6(id,owner));
    return ele;
  }
  return Teuchos::null;
}


Teuchos::RCP<DRT::Element> DRT::ELEMENTS::So_shw6Type::Create( const int id, const int owner )
{
  Teuchos::RCP<DRT::Element> ele = rcp(new DRT::ELEMENTS::So_shw6(id,owner));
  return ele;
}


void DRT::ELEMENTS::So_shw6Type::NodalBlockInformation( DRT::Element * dwele, int & numdf, int & dimns, int & nv, int & np )
{
  numdf = 3;
  dimns = 6;
  nv = 3;
}

void DRT::ELEMENTS::So_shw6Type::ComputeNullSpace( DRT::Discretization & dis, std::vector<double> & ns, const double * x0, int numdf, int dimns )
{
  DRT::UTILS::ComputeStructure3DNullSpace( dis, ns, x0, numdf, dimns );
}

void DRT::ELEMENTS::So_shw6Type::SetupElementDefinition( std::map<std::string,std::map<std::string,DRT::INPUT::LineDefinition> > & definitions )
{
  std::map<std::string,DRT::INPUT::LineDefinition>& defs = definitions["SOLIDSHW6"];

  defs["WEDGE6"]
    .AddIntVector("WEDGE6",6)
    .AddNamedInt("MAT")
    .AddNamedString("KINEM")
    .AddNamedString("EAS")
    .AddOptionalTag("OPTORDER")
    .AddOptionalNamedDoubleVector("RAD",3)
    .AddOptionalNamedDoubleVector("AXI",3)
    .AddOptionalNamedDoubleVector("CIR",3)
    ;
}


/*----------------------------------------------------------------------*
 |  ctor (public)                                              maf 04/07|
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_shw6::So_shw6(int id, int owner) :
DRT::ELEMENTS::So_weg6(id,owner)
{
  eastype_ = soshw6_easnone;
  neas_ = 0;
  optimal_parameterspace_map_ = false;
  nodes_rearranged_ = false;
  return;
}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                         maf 04/07|
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_shw6::So_shw6(const DRT::ELEMENTS::So_shw6& old) :
DRT::ELEMENTS::So_weg6(old)
{
  return;
}

/*----------------------------------------------------------------------*
 |  Deep copy this instance of Solid3 and return pointer to it (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
DRT::Element* DRT::ELEMENTS::So_shw6::Clone() const
{
  DRT::ELEMENTS::So_shw6* newelement = new DRT::ELEMENTS::So_shw6(*this);
  return newelement;
}


/*----------------------------------------------------------------------*
 |  Pack data                                                  (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_shw6::Pack(vector<char>& data) const
{
  data.resize(0);

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data,type);
  // add base class So_weg6 Element
  vector<char> basedata(0);
  DRT::ELEMENTS::So_weg6::Pack(basedata);
  AddtoPack(data,basedata);
  // eastype_
  AddtoPack(data,eastype_);
  // neas_
  AddtoPack(data,neas_);
  // reordering
  AddtoPack(data,optimal_parameterspace_map_);
  AddtoPack(data,nodes_rearranged_);

  return;
}


/*----------------------------------------------------------------------*
 |  Unpack data                                                (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_shw6::Unpack(const vector<char>& data)
{
  vector<char>::size_type position = 0;
  // extract type
  int type = 0;
  ExtractfromPack(position,data,type);
  if (type != UniqueParObjectId()) dserror("wrong instance type data");
  // extract base class So_weg6 Element
  vector<char> basedata(0);
  ExtractfromPack(position,data,basedata);
  DRT::ELEMENTS::So_weg6::Unpack(basedata);
  // eastype_
  ExtractfromPack(position,data,eastype_);
  // neas_
  ExtractfromPack(position,data,neas_);
  // reordering
  ExtractfromPack(position,data,optimal_parameterspace_map_);
  ExtractfromPack(position,data,nodes_rearranged_);

  if (position != data.size())
    dserror("Mismatch in size of data %d <-> %d",(int)data.size(),position);
  return;
}


/*----------------------------------------------------------------------*
 |  dtor (public)                                              maf 04/07|
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_shw6::~So_shw6()
{
  return;
}


/*----------------------------------------------------------------------*
 |  print this element (public)                                maf 04/07|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_shw6::Print(ostream& os) const
{
  os << "So_shw6 ";
  Element::Print(os);
  cout << endl;
  cout << data_;
  return;
}


#endif  // #ifdef CCADISCRET
#endif  // #ifdef D_SOLID3
