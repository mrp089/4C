/*---------------------------------------------------------------------*/
/*! \file

\brief Create and handle integrationcells

\level 3


*----------------------------------------------------------------------*/
#include "baci_cut_integrationcell.H"

#include "baci_cut_boundarycell.H"
#include "baci_cut_facet.H"
#include "baci_cut_mesh.H"
#include "baci_cut_output.H"
#include "baci_cut_position.H"
#include "baci_cut_volumecell.H"
#include "baci_discretization_geometry_element_volume.H"


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
bool CORE::GEO::CUT::IntegrationCell::Contains(CORE::LINALG::Matrix<3, 1>& x)
{
  switch (this->Shape())
  {
    case ::DRT::Element::tet4:
    {
      // find element local position of gauss point
      return Contains<3, ::DRT::Element::tet4>(x);
    }
    case ::DRT::Element::hex8:
    {
      return Contains<3, ::DRT::Element::hex8>(x);
    }
    default:
    {
      dserror("unknown type of integration cell ");
      break;
    }
  }

  return false;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <unsigned probdim, ::DRT::Element::DiscretizationType celltype>
bool CORE::GEO::CUT::IntegrationCell::Contains(CORE::LINALG::Matrix<probdim, 1>& x)
{
  const int ncn = CORE::DRT::UTILS::DisTypeToNumNodePerEle<celltype>::numNodePerElement;

  CORE::LINALG::Matrix<probdim, ncn> coords(xyz_);

  Teuchos::RCP<CORE::GEO::CUT::Position> pos =
      CORE::GEO::CUT::Position::Create(coords, x, celltype);
  pos->Compute();

  return pos->WithinLimits();
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void CORE::GEO::CUT::IntegrationCell::DumpGmsh(std::ofstream& file, int* value)
{
  OUTPUT::GmshCellDump(file, Shape(), xyz_, &position_, value);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
double CORE::GEO::CUT::IntegrationCell::Volume() const
{
  return CORE::GEO::ElementVolume(Shape(), xyz_);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
int CORE::GEO::CUT::Line2IntegrationCell::CubatureDegree(
    ::DRT::Element::DiscretizationType elementshape) const
{
  // not 100% sure what this value really means, but 4 seems more than sufficient.
  return 4;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
int CORE::GEO::CUT::Tri3IntegrationCell::CubatureDegree(
    ::DRT::Element::DiscretizationType elementshape) const
{
  return 4;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
int CORE::GEO::CUT::Quad4IntegrationCell::CubatureDegree(
    ::DRT::Element::DiscretizationType elementshape) const
{
  return 4;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
int CORE::GEO::CUT::Hex8IntegrationCell::CubatureDegree(
    ::DRT::Element::DiscretizationType elementshape) const
{
  switch (elementshape)
  {
    case ::DRT::Element::hex8:
      return 6;
    case ::DRT::Element::hex20:
      return 15;
    case ::DRT::Element::hex27:
      return 15;
    case ::DRT::Element::tet4:
      return 6;
    case ::DRT::Element::tet10:
      return 6;
    case ::DRT::Element::wedge6:
      return 6;
    case ::DRT::Element::wedge15:
      return 14;
    case ::DRT::Element::pyramid5:
      return 6;
    default:
      dserror("no rule defined for this element type");
      exit(EXIT_FAILURE);
  }
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
int CORE::GEO::CUT::Tet4IntegrationCell::CubatureDegree(
    ::DRT::Element::DiscretizationType elementshape) const
{
  switch (elementshape)
  {
    case ::DRT::Element::hex8:
      return 6;
    case ::DRT::Element::hex20:
      return 15;
    case ::DRT::Element::hex27:
      return 15;
    case ::DRT::Element::tet4:
      return 6;
    case ::DRT::Element::tet10:
      return 7;
    case ::DRT::Element::wedge6:
      return 6;
    case ::DRT::Element::wedge15:
      return 14;
    case ::DRT::Element::pyramid5:
      return 6;
    default:
      dserror("no rule defined for this element type");
      exit(EXIT_FAILURE);
  }
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
int CORE::GEO::CUT::Wedge6IntegrationCell::CubatureDegree(
    ::DRT::Element::DiscretizationType elementshape) const
{
  return 4;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
int CORE::GEO::CUT::Pyramid5IntegrationCell::CubatureDegree(
    ::DRT::Element::DiscretizationType elementshape) const
{
  return 4;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void CORE::GEO::CUT::IntegrationCell::Print(std::ostream& stream) const
{
  stream << "--- integration cell ( address: " << std::setw(10) << this << " )\n";
  stream << "pos = " << Point::PointPosition2String(Position()) << " "
         << "shape = " << ::DRT::DistypeToString(Shape()) << " "
         << "volume = " << Volume() << "\n";
  for (unsigned i = 0; i < points_.size(); ++i)
  {
    (points_)[i]->Print(stream);
    stream << "\n";
  }
}
