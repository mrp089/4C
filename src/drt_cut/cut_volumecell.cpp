
#include "cut_volumecell.H"
#include "cut_boundarycell.H"
#include "cut_integrationcell.H"
#include "cut_facet.H"
#include "cut_tetmesh.H"
#include "cut_mesh.H"
#include "cut_options.H"
#include "cut_kernel.H"

#include "../../src/drt_fem_general/drt_utils_gausspoints.H"


int GEO::CUT::VolumeCell::hex8totet4[5][4] = {
  {0, 1, 3, 4},
  {1, 2, 3, 6},
  {4, 5, 1, 6},
  {6, 7, 3, 4},
  {1, 6, 3, 4}
};

int GEO::CUT::VolumeCell::wedge6totet4[3][4] = {
  {0, 1, 2, 3},
  {3, 4, 1, 5},
  {1, 5, 2, 3}
};


int GEO::CUT::VolumeCell::pyramid5totet4[2][4] = {
  {0, 1, 3, 4},
  {1, 2, 3, 4}
};


GEO::CUT::VolumeCell::VolumeCell( const plain_facet_set & facets,
                                  const std::map<std::pair<Point*, Point*>, plain_facet_set > & volume_lines,
                                  Element * element )
  : element_( element ),
    position_( Point::undecided ),
    facets_( facets )
{
  for ( plain_facet_set::const_iterator i=facets_.begin(); i!=facets_.end(); ++i )
  {
    Facet * f = *i;
    f->Register( this );
  }
}

void GEO::CUT::VolumeCell::Neighbors( Point * p,
                                      const plain_volumecell_set & cells,
                                      const plain_volumecell_set & done,
                                      plain_volumecell_set & connected,
                                      plain_element_set & elements )
{
  if ( done.count( this )==0 )
  {
    // this volume is included
    connected.insert( this );
    elements.insert( element_ );

    // Do the facets that include the point first. This ensures we choose the
    // right volumes (the ones attached to the point), if there are multiple
    // connections possible (we are faced with a thin structure cut.)

    for ( plain_facet_set::const_iterator i=facets_.begin(); i!=facets_.end(); ++i )
    {
      Facet * f = *i;
      if ( p==NULL or f->Contains( p ) )
      {
        f->Neighbors( p, cells, done, connected, elements );
      }
    }

    if ( p!=NULL )
    {
      for ( plain_facet_set::const_iterator i=facets_.begin(); i!=facets_.end(); ++i )
      {
        Facet * f = *i;
        if ( not f->Contains( p ) )
        {
          f->Neighbors( p, cells, done, connected, elements );
        }
      }
    }
  }
}

void GEO::CUT::VolumeCell::GetAllPoints( Mesh & mesh, PointSet & cut_points )
{
  for ( plain_facet_set::iterator i=facets_.begin(); i!=facets_.end(); ++i )
  {
    Facet * f = *i;
    f->GetAllPoints( mesh, cut_points );
  }
}

bool GEO::CUT::VolumeCell::Contains( Point * p )
{
  for ( plain_facet_set::const_iterator i=facets_.begin(); i!=facets_.end(); ++i )
  {
    Facet * f = *i;
    if ( f->Contains( p ) )
    {
      return true;
    }
  }
  return false;
}

void GEO::CUT::VolumeCell::CreateTet4IntegrationCells( Mesh & mesh,
                                                       const std::vector<std::vector<Point*> > & tets,
                                                       const std::map<Facet*, std::vector<Point*> > & sides_xyz )
{
  for ( std::vector<std::vector<Point*> >::const_iterator i=tets.begin();
        i!=tets.end();
        ++i )
  {
    const std::vector<Point*> & tet = *i;
    if ( tet.size()!=4 )
    {
      throw std::runtime_error( "tet expected" );
    }
    NewTet4Cell( mesh, tet );
  }

  for ( std::map<Facet*, std::vector<Point*> >::const_iterator i=sides_xyz.begin();
        i!=sides_xyz.end();
        ++i )
  {
    Facet * f = i->first;
    const std::vector<Point*> & points = i->second;

    std::size_t length = points.size();
    if ( length % 3 != 0 )
      throw std::runtime_error( "expect list of triangles" );

    length /= 3;
    std::vector<Point*> p( 3 );
    for ( std::size_t i=0; i<length; ++i )
    {
      std::copy( &points[3*i], &points[3*( i+1 )], &p[0] );
      //Tri3BoundaryCell::CreateCell( mesh, this, f, p );
      NewTri3Cell( mesh, f, p );
    }
  }
}

void GEO::CUT::VolumeCell::GetIntegrationCells( plain_integrationcell_set & cells )
{
  std::copy( integrationcells_.begin(), integrationcells_.end(), std::inserter( cells, cells.begin() ) );
}

void GEO::CUT::VolumeCell::GetBoundaryCells( std::map<int, std::vector<GEO::CUT::BoundaryCell*> > & bcells )
{
  for ( plain_boundarycell_set::iterator i=bcells_.begin(); i!=bcells_.end(); ++i )
  {
    BoundaryCell * bc = *i;
    Facet * f = bc->GetFacet();
    int sid = f->SideId();
    if ( sid > -1 )
    {
      bcells[sid].push_back( bc );
    }
  }
}

void GEO::CUT::VolumeCell::ConnectNodalDOFSets( bool include_inner )
{
  if ( not include_inner and Position()!=Point::outside )
    return;

  const std::vector<Node*> & nodes = element_->Nodes();
  nodaldofset_.reserve( nodes.size() );

  for ( std::vector<Node*>::const_iterator i=nodes.begin();
        i!=nodes.end();
        ++i )
  {
    Node * n = *i;
    nodaldofset_.push_back( n->DofSetNumber( this ) );
  }
}

void GEO::CUT::VolumeCell::Position( Point::PointPosition position )
{
  if ( position_ != position )
  {
    position_ = position;

    for ( plain_facet_set::const_iterator i=facets_.begin(); i!=facets_.end(); ++i )
    {
      Facet * f = *i;
      Point::PointPosition fp = f->Position();
      if ( fp==Point::undecided )
      {
        f->Position( position );
      }
    }
  }
}

void GEO::CUT::VolumeCell::Print( std::ostream & stream )
{
  stream << "# VolumeCell: "
         << position_ << " "
         << facets_.size() << " "
         << integrationcells_.size() << " "
         << bcells_.size()
         << "\n";
  for ( plain_facet_set::iterator i=facets_.begin(); i!=facets_.end(); ++i )
  {
    Facet * f = *i;
    f->Print( stream );
  }
}

void GEO::CUT::VolumeCell::NewBoundaryCell( Mesh & mesh, DRT::Element::DiscretizationType shape, Facet * f, const std::vector<Point*> & x )
{
  switch ( shape )
  {
  case DRT::Element::tri3:
    NewTri3Cell( mesh, f, x );
    break;
  case DRT::Element::quad4:
    NewQuad4Cell( mesh, f, x );
    break;
  default:
    throw std::runtime_error( "unknown shape" );
  }
}

void GEO::CUT::VolumeCell::NewTri3Cell( Mesh & mesh, Facet * f, const std::vector<Point*> & x )
{
  f->NewTri3Cell( mesh, this, x, bcells_ );
}

void GEO::CUT::VolumeCell::NewQuad4Cell( Mesh & mesh, Facet * f, const std::vector<Point*> & x )
{
  f->NewQuad4Cell( mesh, this, x, bcells_ );
}

double GEO::CUT::VolumeCell::Volume()
{
  double volume = 0;
  for ( plain_integrationcell_set::iterator i=integrationcells_.begin(); i!=integrationcells_.end(); ++i )
  {
    IntegrationCell * ic = *i;
    volume += ic->Volume();
  }
  return volume;
}

int GEO::CUT::VolumeCell::NumGaussPoints( DRT::Element::DiscretizationType shape )
{
  int numgp = 0;

  for ( plain_integrationcell_set::const_iterator i=integrationcells_.begin(); i!=integrationcells_.end(); ++i )
  {
    IntegrationCell * ic = *i;

    // Create (unmodified) gauss points for integration cell with requested
    // polynomial order. This is supposed to be fast, since there is a cache.
    DRT::UTILS::GaussIntegration gi( ic->Shape(), ic->CubatureDegree( shape ) );

    // we just need the number of points per cell
    numgp += gi.NumPoints();
  }

  return numgp;
}

void GEO::CUT::VolumeCell::NewIntegrationCell( Mesh & mesh, DRT::Element::DiscretizationType shape, const std::vector<Point*> & x )
{
  switch ( shape )
  {
  case DRT::Element::hex8:
    NewHex8Cell( mesh, x );
    break;
  case DRT::Element::tet4:
    NewTet4Cell( mesh, x );
    break;
  case DRT::Element::wedge6:
    NewWedge6Cell( mesh, x );
    break;
  case DRT::Element::pyramid5:
    NewPyramid5Cell( mesh, x );
    break;
  default:
    throw std::runtime_error( "unknown shape" );
  }
}

void GEO::CUT::VolumeCell::NewHex8Cell( Mesh & mesh, const std::vector<Point*> & points )
{
  Point::PointPosition position = Position();
  if ( mesh.CreateOptions().GenHex8() )
  {
    integrationcells_.insert( mesh.NewHex8Cell( position, points, this ) );
  }
  else
  {
    std::vector<Point*> tet4_points( 4 );
    for ( int i=0; i<5; ++i )
    {
      SetTetPoints( hex8totet4[i], points, tet4_points );
      integrationcells_.insert( mesh.NewTet4Cell( position, tet4_points, this ) );
    }
  }
}

GEO::CUT::IntegrationCell * GEO::CUT::VolumeCell::NewTet4Cell( Mesh & mesh, const std::vector<Point*> & points )
{
  Point::PointPosition position = Position();
  IntegrationCell * ic = mesh.NewTet4Cell( position, points, this );
  integrationcells_.insert( ic );
  return ic;
}

void GEO::CUT::VolumeCell::NewWedge6Cell( Mesh & mesh, const std::vector<Point*> & points )
{
  Point::PointPosition position = Position();
  if ( mesh.CreateOptions().GenWedge6() )
  {
    integrationcells_.insert( mesh.NewWedge6Cell( position, points, this ) );
  }
  else
  {
    std::vector<Point*> tet4_points( 4 );
    for ( int i=0; i<3; ++i )
    {
      SetTetPoints( wedge6totet4[i], points, tet4_points );
      integrationcells_.insert( mesh.NewTet4Cell( position, tet4_points, this ) );
    }
  }
}

void GEO::CUT::VolumeCell::NewPyramid5Cell( Mesh & mesh, const std::vector<Point*> & points )
{
  Point::PointPosition position = Position();
  if ( mesh.CreateOptions().GenPyramid5() )
  {
    integrationcells_.insert( mesh.NewPyramid5Cell( position, points, this ) );
  }
  else
  {
    std::vector<Point*> tet4_points( 4 );
    for ( int i=0; i<2; ++i )
    {
      SetTetPoints( pyramid5totet4[i], points, tet4_points );
      integrationcells_.insert( mesh.NewTet4Cell( position, tet4_points, this ) );
    }
  }
}

void GEO::CUT::VolumeCell::SimplifyIntegrationCells( Mesh & mesh )
{
  // do whatever can be done to get simpler cells
  //

  std::map<int, std::vector<Facet*> > side_facets;

  for ( plain_facet_set::iterator i=facets_.begin(); i!=facets_.end(); ++i )
  {
    Facet * f = *i;
    if ( f->OnCutSide() )
    {
      side_facets[f->SideId()].push_back( f );
    }
  }

  for ( std::map<int, std::vector<Facet*> >::iterator i=side_facets.begin();
        i!=side_facets.end();
        ++i )
  {
    int sideid = i->first;
    std::vector<Facet*> & facets = i->second;
    std::vector<BoundaryCell*> bcs;
    sorted_vector<std::pair<Point*, Point*> > lines;
    for ( plain_boundarycell_set::iterator i=bcells_.begin(); i!=bcells_.end(); ++i )
    {
      BoundaryCell * bc = *i;
      if ( bc->GetFacet()->SideId()==sideid )
      {
        const Cycle & cycle = bc->PointCycle();
        cycle.Add( lines );
        bcs.push_back( bc );
      }
    }
    if ( bcs.size() > 1 )
    {
      Cycle cycle;
      if ( Cycle::MakeCycle( lines, cycle ) )
      {
        std::vector<Point*> corner_points;
        DRT::Element::DiscretizationType shape = KERNEL::CalculateShape( cycle(), corner_points );

        if ( shape!=DRT::Element::dis_none )
        {
          for ( std::vector<BoundaryCell*>::iterator i=bcs.begin(); i!=bcs.end(); ++i )
          {
            BoundaryCell * bc = *i;
            bcells_.erase( bc );
            bc->Clear();
          }
          switch ( shape )
          {
          case DRT::Element::quad4:
            // the facet is too small, but it knows the right side
            if ( mesh.CreateOptions().GenQuad4() )
            {
              mesh.NewQuad4Cell( this, facets[0], corner_points );
            }
            else
            {
              std::vector<Point*> tri3_points = corner_points;
              tri3_points.pop_back();
              mesh.NewTri3Cell( this, facets[0], tri3_points );
              tri3_points.erase( tri3_points.begin()+1 );
              tri3_points.push_back( corner_points.back() );
              mesh.NewTri3Cell( this, facets[0], tri3_points );
            }
            break;
          case DRT::Element::tri3:
            // the facet is too small, but it knows the right side
            mesh.NewTri3Cell( this, facets[0], corner_points );
            break;
          default:
            throw std::runtime_error( "unsupported boundary cell type" );
          }
        }
#if 0
        std::cout << "found cycle with " << cycle.size()
                  << " points on cut side " << sideid
                  << " out of " << numbc
                  << " boundary cells: shape=" << shape
                  << " with " << line_points.size()
                  << " points\n";
#endif
      }
    }
  }
}
