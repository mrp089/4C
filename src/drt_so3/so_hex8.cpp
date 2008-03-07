/*!----------------------------------------------------------------------
\file so_hex8.cpp
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

#include "so_hex8.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_lib/drt_dserror.H"

/*----------------------------------------------------------------------*
 |  ctor (public)                                              maf 04/07|
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_hex8::So_hex8(int id, int owner) :
DRT::Element(id,element_so_hex8,owner),
data_()
{
  volume_.resize(0);
  surfaces_.resize(0);
  surfaceptrs_.resize(0);
  lines_.resize(0);
  lineptrs_.resize(0);
  kintype_ = soh8_totlag;
  eastype_ = soh8_easnone;
  neas_ = 0;
  donerewinding_ = false;
  thickvec_.resize(0);
  fiberdirection_.resize(0);
  return;
}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                         maf 04/07|
 |  id             (in)  this element's global id                       |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_hex8::So_hex8(const DRT::ELEMENTS::So_hex8& old) :
DRT::Element(old),
kintype_(old.kintype_),
eastype_(old.eastype_),
neas_(old.neas_),
data_(old.data_),
volume_(old.volume_),
surfaces_(old.surfaces_),
surfaceptrs_(old.surfaceptrs_),
lines_(old.lines_),
lineptrs_(old.lineptrs_),
donerewinding_(old.donerewinding_),
thickvec_(old.thickvec_),
fiberdirection_(old.fiberdirection_)
{
  return;
}

/*----------------------------------------------------------------------*
 |  Deep copy this instance of Solid3 and return pointer to it (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
DRT::Element* DRT::ELEMENTS::So_hex8::Clone() const
{
  DRT::ELEMENTS::So_hex8* newelement = new DRT::ELEMENTS::So_hex8(*this);
  return newelement;
}

/*----------------------------------------------------------------------*
 |                                                             (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
DRT::Element::DiscretizationType DRT::ELEMENTS::So_hex8::Shape() const
{
  return hex8;
}

/*----------------------------------------------------------------------*
 |  Pack data                                                  (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_hex8::Pack(vector<char>& data) const
{
  data.resize(0);

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data,type);
  // add base class Element
  vector<char> basedata(0);
  Element::Pack(basedata);
  AddtoPack(data,basedata);
  // kintype_
  AddtoPack(data,kintype_);
  // eastype_
  AddtoPack(data,eastype_);
  // neas_
  AddtoPack(data,neas_);
  // rewind flags
  AddtoPack(data,donerewinding_);
  // fiber related
  AddtoPack(data,thickvec_);
  AddtoPack(data,fiberdirection_);
  // data_
  vector<char> tmp(0);
  data_.Pack(tmp);
  AddtoPack(data,tmp);

  return;
}


/*----------------------------------------------------------------------*
 |  Unpack data                                                (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_hex8::Unpack(const vector<char>& data)
{
  int position = 0;
  // extract type
  int type = 0;
  ExtractfromPack(position,data,type);
  if (type != UniqueParObjectId()) dserror("wrong instance type data");
  // extract base class Element
  vector<char> basedata(0);
  ExtractfromPack(position,data,basedata);
  Element::Unpack(basedata);
  // kintype_
  ExtractfromPack(position,data,kintype_);
  // eastype_
  ExtractfromPack(position,data,eastype_);
  // neas_
  ExtractfromPack(position,data,neas_);
  // rewinding flags
  ExtractfromPack(position,data,donerewinding_);
  // fiber related
  ExtractfromPack(position,data,thickvec_);
  ExtractfromPack(position,data,fiberdirection_);
  // data_
  vector<char> tmp(0);
  ExtractfromPack(position,data,tmp);
  data_.Unpack(tmp);

  if (position != (int)data.size())
    dserror("Mismatch in size of data %d <-> %d",(int)data.size(),position);
  return;
}


/*----------------------------------------------------------------------*
 |  dtor (public)                                              maf 04/07|
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::So_hex8::~So_hex8()
{
  return;
}


/*----------------------------------------------------------------------*
 |  print this element (public)                                maf 04/07|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_hex8::Print(ostream& os) const
{
  os << "So_hex8 ";
  Element::Print(os);
  cout << endl;
  cout << data_;
  return;
}


/*----------------------------------------------------------------------*
 |  extrapolation of quantities at the GPs to the nodes      lw 02/08   |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_hex8::soh8_expol(Epetra_SerialDenseMatrix& stresses,
                                        Epetra_SerialDenseMatrix& nodalstresses)
{
  static Epetra_SerialDenseMatrix expol(NUMNOD_SOH8,NUMGPT_SOH8);
  static bool isfilled;

  if (isfilled==true)
  {
    nodalstresses.Multiply('N','N',1.0,expol,stresses,0.0);
  }
  else
  {
    double sq3=sqrt(3);
    expol(0,0)=1.25+0.75*sq3;
    expol(0,1)=-0.25-0.25*sq3;
    expol(0,2)=-0.25+0.25*sq3;
    expol(0,3)=-0.25-0.25*sq3;
    expol(0,4)=-0.25-0.25*sq3;
    expol(0,5)=-0.25+0.25*sq3;
    expol(0,6)=1.25-0.75*sq3;
    expol(0,7)=-0.25+0.25*sq3;
    expol(1,1)=1.25+0.75*sq3;
    expol(1,2)=-0.25-0.25*sq3;
    expol(1,3)=-0.25+0.25*sq3;
    expol(1,4)=-0.25+0.25*sq3;
    expol(1,5)=-0.25-0.25*sq3;
    expol(1,6)=-0.25+0.25*sq3;
    expol(1,7)=1.25-0.75*sq3;
    expol(2,2)=1.25+0.75*sq3;
    expol(2,3)=-0.25-0.25*sq3;
    expol(2,4)=1.25-0.75*sq3;
    expol(2,5)=-0.25+0.25*sq3;
    expol(2,6)=-0.25-0.25*sq3;
    expol(2,7)=-0.25+0.25*sq3;
    expol(3,3)=1.25+0.75*sq3;
    expol(3,4)=-0.25+0.25*sq3;
    expol(3,5)=1.25-0.75*sq3;
    expol(3,6)=-0.25+0.25*sq3;
    expol(3,7)=-0.25-0.25*sq3;
    expol(4,4)=1.25+0.75*sq3;
    expol(4,5)=-0.25-0.25*sq3;
    expol(4,6)=-0.25+0.25*sq3;
    expol(4,7)=-0.25-0.25*sq3;
    expol(5,5)=1.25+0.75*sq3;
    expol(5,6)=-0.25-0.25*sq3;
    expol(5,7)=-0.25+0.25*sq3;
    expol(6,6)=1.25+0.75*sq3;
    expol(6,7)=-0.25-0.25*sq3;
    expol(7,7)=1.25+0.75*sq3;

    for (int i=0;i<NUMNOD_SOH8;++i)
    {
      for (int j=0;j<i;++j)
      {
        expol(i,j)=expol(j,i);
      }
    }

    nodalstresses.Multiply('N','N',1.0,expol,stresses,0.0);

    isfilled = true;
  }
}

/*----------------------------------------------------------------------*
 |  allocate and return So_hex8Register (public)                maf 04/07|
 *----------------------------------------------------------------------*/
RefCountPtr<DRT::ElementRegister> DRT::ELEMENTS::So_hex8::ElementRegister() const
{
  return rcp(new DRT::ELEMENTS::Soh8Register(Type()));
}

  /*====================================================================*/
  /* 8-node hexhedra node topology*/
  /*--------------------------------------------------------------------*/
  /* parameter coordinates (r,s,t) of nodes
   * of biunit cube [-1,1]x[-1,1]x[-1,1]
   *  8-node hexahedron: node 0,1,...,7
   *                      t
   *                      |
   *             4========|================7
   *           //|        |               /||
   *          // |        |              //||
   *         //  |        |             // ||
   *        //   |        |            //  ||
   *       //    |        |           //   ||
   *      //     |        |          //    ||
   *     //      |        |         //     ||
   *     5=========================6       ||
   *    ||       |        |        ||      ||
   *    ||       |        o--------||---------s
   *    ||       |       /         ||      ||
   *    ||       0------/----------||------3
   *    ||      /      /           ||     //
   *    ||     /      /            ||    //
   *    ||    /      /             ||   //
   *    ||   /      /              ||  //
   *    ||  /      /               || //
   *    || /      r                ||//
   *    ||/                        ||/
   *     1=========================2
   *
   */
  /*====================================================================*/

/*----------------------------------------------------------------------*
 |  get vector of volumes (length 1) (public)                  maf 04/07|
 *----------------------------------------------------------------------*/
DRT::Element** DRT::ELEMENTS::So_hex8::Volumes()
{
  volume_.resize(1);
  volume_[0] = this; //points to element itself
  return (DRT::Element**)&volume_[0];
}

 /*----------------------------------------------------------------------*
 |  get vector of surfaces (public)                             maf 04/07|
 |  surface normals always point outward                                 |
 *----------------------------------------------------------------------*/
DRT::Element** DRT::ELEMENTS::So_hex8::Surfaces()
{

  const int nsurf = NumSurface();
  surfaces_.resize(nsurf);
  surfaceptrs_.resize(nsurf);
  int nodeids[100];
  DRT::Node* nodes[100];

  nodeids[0] = NodeIds()[0];
  nodeids[1] = NodeIds()[3];
  nodeids[2] = NodeIds()[2];
  nodeids[3] = NodeIds()[1];
  nodes[0] = Nodes()[0];
  nodes[1] = Nodes()[3];
  nodes[2] = Nodes()[2];
  nodes[3] = Nodes()[1];
  surfaces_[0] =
    rcp(new DRT::ELEMENTS::Soh8Surface(0,Owner(),4,nodeids,nodes,this,0));
  surfaceptrs_[0] = surfaces_[0].get();

  nodeids[0] = NodeIds()[0];
  nodeids[1] = NodeIds()[1];
  nodeids[2] = NodeIds()[5];
  nodeids[3] = NodeIds()[4];
  nodes[0] = Nodes()[0];
  nodes[1] = Nodes()[1];
  nodes[2] = Nodes()[5];
  nodes[3] = Nodes()[4];
  surfaces_[1] =
    rcp(new DRT::ELEMENTS::Soh8Surface(1,Owner(),4,nodeids,nodes,this,1));
  surfaceptrs_[1] = surfaces_[1].get();

  nodeids[0] = NodeIds()[0];
  nodeids[1] = NodeIds()[4];
  nodeids[2] = NodeIds()[7];
  nodeids[3] = NodeIds()[3];
  nodes[0] = Nodes()[0];
  nodes[1] = Nodes()[4];
  nodes[2] = Nodes()[7];
  nodes[3] = Nodes()[3];
  surfaces_[2] =
    rcp(new DRT::ELEMENTS::Soh8Surface(2,Owner(),4,nodeids,nodes,this,2));
  surfaceptrs_[2] = surfaces_[2].get();

  nodeids[0] = NodeIds()[2];
  nodeids[1] = NodeIds()[3];
  nodeids[2] = NodeIds()[7];
  nodeids[3] = NodeIds()[6];
  nodes[0] = Nodes()[2];
  nodes[1] = Nodes()[3];
  nodes[2] = Nodes()[7];
  nodes[3] = Nodes()[6];
  surfaces_[3] =
    rcp(new DRT::ELEMENTS::Soh8Surface(3,Owner(),4,nodeids,nodes,this,3));
  surfaceptrs_[3] = surfaces_[3].get();

  nodeids[0] = NodeIds()[1];
  nodeids[1] = NodeIds()[2];
  nodeids[2] = NodeIds()[6];
  nodeids[3] = NodeIds()[5];
  nodes[0] = Nodes()[1];
  nodes[1] = Nodes()[2];
  nodes[2] = Nodes()[6];
  nodes[3] = Nodes()[5];
  surfaces_[4] =
    rcp(new DRT::ELEMENTS::Soh8Surface(4,Owner(),4,nodeids,nodes,this,4));
  surfaceptrs_[4] = surfaces_[4].get();

  nodeids[0] = NodeIds()[4];
  nodeids[1] = NodeIds()[5];
  nodeids[2] = NodeIds()[6];
  nodeids[3] = NodeIds()[7];
  nodes[0] = Nodes()[4];
  nodes[1] = Nodes()[5];
  nodes[2] = Nodes()[6];
  nodes[3] = Nodes()[7];
  surfaces_[5] =
    rcp(new DRT::ELEMENTS::Soh8Surface(5,Owner(),4,nodeids,nodes,this,5));
  surfaceptrs_[5] = surfaces_[5].get();

  return (DRT::Element**)(&(surfaceptrs_[0]));
}

/*----------------------------------------------------------------------*
 |  get vector of lines (public)                               maf 04/07|
 *----------------------------------------------------------------------*/
DRT::Element** DRT::ELEMENTS::So_hex8::Lines()
{
  const int nline = NumLine();
  lines_.resize(nline);
  lineptrs_.resize(nline);
  int nodeids[100];
  DRT::Node* nodes[100];

  nodeids[0] = NodeIds()[0];
  nodeids[1] = NodeIds()[1];
  nodes[0] = Nodes()[0];
  nodes[1] = Nodes()[1];
  lines_[0] =
    rcp(new DRT::ELEMENTS::Soh8Line(0,Owner(),2,nodeids,nodes,this,0));
  lineptrs_[0] = lines_[0].get();

  nodeids[0] = NodeIds()[1];
  nodeids[1] = NodeIds()[2];
  nodes[0] = Nodes()[1];
  nodes[1] = Nodes()[2];
  lines_[1] =
    rcp(new DRT::ELEMENTS::Soh8Line(1,Owner(),2,nodeids,nodes,this,1));
  lineptrs_[1] = lines_[1].get();

  nodeids[0] = NodeIds()[2];
  nodeids[1] = NodeIds()[3];
  nodes[0] = Nodes()[2];
  nodes[1] = Nodes()[3];
  lines_[2] =
    rcp(new DRT::ELEMENTS::Soh8Line(2,Owner(),2,nodeids,nodes,this,2));
  lineptrs_[2] = lines_[2].get();

  nodeids[0] = NodeIds()[3];
  nodeids[1] = NodeIds()[0];
  nodes[0] = Nodes()[3];
  nodes[1] = Nodes()[0];
  lines_[3] =
    rcp(new DRT::ELEMENTS::Soh8Line(3,Owner(),2,nodeids,nodes,this,3));
  lineptrs_[3] = lines_[3].get();

  nodeids[0] = NodeIds()[0];
  nodeids[1] = NodeIds()[4];
  nodes[0] = Nodes()[0];
  nodes[1] = Nodes()[4];
  lines_[4] =
    rcp(new DRT::ELEMENTS::Soh8Line(4,Owner(),2,nodeids,nodes,this,4));
  lineptrs_[4] = lines_[4].get();

  nodeids[0] = NodeIds()[1];
  nodeids[1] = NodeIds()[5];
  nodes[0] = Nodes()[1];
  nodes[1] = Nodes()[5];
  lines_[5] =
    rcp(new DRT::ELEMENTS::Soh8Line(5,Owner(),2,nodeids,nodes,this,5));
  lineptrs_[5] = lines_[5].get();

  nodeids[0] = NodeIds()[2];
  nodeids[1] = NodeIds()[6];
  nodes[0] = Nodes()[2];
  nodes[1] = Nodes()[6];
  lines_[6] =
    rcp(new DRT::ELEMENTS::Soh8Line(6,Owner(),2,nodeids,nodes,this,6));
  lineptrs_[6] = lines_[6].get();

  nodeids[0] = NodeIds()[3];
  nodeids[1] = NodeIds()[7];
  nodes[0] = Nodes()[3];
  nodes[1] = Nodes()[7];
  lines_[7] =
    rcp(new DRT::ELEMENTS::Soh8Line(7,Owner(),2,nodeids,nodes,this,7));
  lineptrs_[7] = lines_[7].get();

  nodeids[0] = NodeIds()[4];
  nodeids[1] = NodeIds()[5];
  nodes[0] = Nodes()[4];
  nodes[1] = Nodes()[5];
  lines_[8] =
    rcp(new DRT::ELEMENTS::Soh8Line(8,Owner(),2,nodeids,nodes,this,8));
  lineptrs_[8] = lines_[8].get();

  nodeids[0] = NodeIds()[5];
  nodeids[1] = NodeIds()[6];
  nodes[0] = Nodes()[5];
  nodes[1] = Nodes()[6];
  lines_[9] =
    rcp(new DRT::ELEMENTS::Soh8Line(9,Owner(),2,nodeids,nodes,this,9));
  lineptrs_[9] = lines_[9].get();

  nodeids[0] = NodeIds()[6];
  nodeids[1] = NodeIds()[7];
  nodes[0] = Nodes()[6];
  nodes[1] = Nodes()[7];
  lines_[10] =
    rcp(new DRT::ELEMENTS::Soh8Line(10,Owner(),2,nodeids,nodes,this,10));
  lineptrs_[10] = lines_[10].get();

  nodeids[0] = NodeIds()[7];
  nodeids[1] = NodeIds()[4];
  nodes[0] = Nodes()[7];
  nodes[1] = Nodes()[4];
  lines_[11] =
    rcp(new DRT::ELEMENTS::Soh8Line(11,Owner(),2,nodeids,nodes,this,11));
  lineptrs_[11] = lines_[11].get();
  return (DRT::Element**)(&(lineptrs_[0]));
}

/*----------------------------------------------------------------------*
 |  Return names of visualization data (public)                maf 01/08|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_hex8::VisNames(map<string,int>& names)
{
  // Put the owner of this element into the file (use base class method for this)
  DRT::Element::VisNames(names);

  // element fiber direction vector
  string fibervecname = "FiberVec";
  names[fibervecname] = 3;

  return;
}

/*----------------------------------------------------------------------*
 |  Return visualization data (public)                         maf 01/08|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::So_hex8::VisData(const string& name, vector<double>& data)
{
  // Put the owner of this element into the file (use base class method for this)
  DRT::Element::VisData(name,data);

  // these are the names so_hex8 recognizes, do nothing for everything else
  if (name != "FiberVec") return;

  // check sizes
  if ((name == "FiberVec") && ((int)data.size()!=3)) dserror("FiberVec size mismatch ");

  if (name == "FiberVec"){
    data = fiberdirection_;
  }
  else dserror("weirdo impossible case????");

  return;
}



//=======================================================================
//=======================================================================
//=======================================================================
//=======================================================================

/*----------------------------------------------------------------------*
 |  ctor (public)                                              maf 04/07|
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::Soh8Register::Soh8Register(DRT::Element::ElementType etype) :
ElementRegister(etype)
{
  return;
}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                         maf 04/07|
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::Soh8Register::Soh8Register(
                               const DRT::ELEMENTS::Soh8Register& old) :
ElementRegister(old)
{
  return;
}

/*----------------------------------------------------------------------*
 |  Deep copy this instance return pointer to it               (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::Soh8Register* DRT::ELEMENTS::Soh8Register::Clone() const
{
  return new DRT::ELEMENTS::Soh8Register(*this);
}

/*----------------------------------------------------------------------*
 |  Pack data                                                  (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Soh8Register::Pack(vector<char>& data) const
{
  data.resize(0);

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data,type);
  // add base class ElementRegister
  vector<char> basedata(0);
  ElementRegister::Pack(basedata);
  AddtoPack(data,basedata);

  return;
}


/*----------------------------------------------------------------------*
 |  Unpack data                                                (public) |
 |                                                            maf 04/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Soh8Register::Unpack(const vector<char>& data)
{
  int position = 0;
  // extract type
  int type = 0;
  ExtractfromPack(position,data,type);
  if (type != UniqueParObjectId()) dserror("wrong instance type data");
  // base class ElementRegister
  vector<char> basedata(0);
  ExtractfromPack(position,data,basedata);
  ElementRegister::Unpack(basedata);

  if (position != (int)data.size())
    dserror("Mismatch in size of data %d <-> %d",(int)data.size(),position);
  return;
}


/*----------------------------------------------------------------------*
 |  dtor (public)                                              maf 04/07|
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::Soh8Register::~Soh8Register()
{
  return;
}

/*----------------------------------------------------------------------*
 |  print (public)                                             maf 04/07|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Soh8Register::Print(ostream& os) const
{
  os << "Soh8Register ";
  ElementRegister::Print(os);
  return;
}

#endif  // #ifdef CCADISCRET
#endif  // #ifdef D_SOLID3
