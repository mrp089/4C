
//#include "../drt_geometry/intersection_templates.H"

#include "cut_position.H"
#include "cut_position2d.H"
#include "cut_intersection.H"
#include "cut_facet.H"
#include "cut_point_impl.H"
#include "cut_pointgraph.H"
#include "cut_pointcycle.H"
#include "cut_linesegment.H"
#include "cut_creator.H"

#include <string>
#include <stack>

#include "cut_side.H"

GEO::CUT::Edge * GEO::CUT::Side::FindEdge( Point * begin, Point * end )
{
  for ( std::vector<Edge*>::iterator i=edges_.begin(); i!=edges_.end(); ++i )
  {
    Edge * e = *i;
    if ( e->Matches( begin, end ) )
    {
      return e;
    }
  }
  return NULL;
}

bool GEO::CUT::Side::FindCutPoints( Mesh & mesh, Element * element, Side & other )
{
  bool cut = false;
  const std::vector<Edge*> & edges = Edges();
  for ( std::vector<Edge*>::const_iterator i=edges.begin(); i!=edges.end(); ++i )
  {
    Edge * e = *i;
    if ( e->FindCutPoints( mesh, element, *this, other ) )
    {
      cut = true;
    }
  }
  return cut;
}

bool GEO::CUT::Side::FindCutLines( Mesh & mesh, Element * element, Side & other )
{
#if 1
  bool cut = false;
  for ( std::vector<Line*>::iterator i=cut_lines_.begin(); i!=cut_lines_.end(); ++i )
  {
    Line * l = *i;
    if ( l->IsCut( this, &other ) )
    {
      l->AddElement( element );
      other.AddLine( l );
      cut = true;
    }
  }
  if ( cut )
  {
    return true;
  }
#endif

  std::set<Point*> cuts;
  GetCutPoints( element, other, cuts );

  switch ( cuts.size() )
  {
  case 0:
    return false;
  case 1:
  {
    std::set<Point*> reverse_cuts;
    other.GetCutPoints( element, *this, reverse_cuts );
    reverse_cuts.erase( *cuts.begin() );
    if ( reverse_cuts.size()==1 )
    {
      mesh.NewLine( *cuts.begin(), *reverse_cuts.begin(), this, &other, element );
      return true;
    }
    else if ( reverse_cuts.size()==0 )
    {
      // Touch of two edges. No lines to create?!
      return false;
    }
    else
    {
      throw std::runtime_error( "most peculiar cut" );
    }
  }
  case 2:
  {
    // The normal case. A straight cut.
    std::vector<Point*> c;
    c.reserve( 2 );
    c.assign( cuts.begin(), cuts.end() );
    mesh.NewLine( c[0], c[1], this, &other, element );
    return true;
  }
  default:
  {
    // More that two cut points shows a touch.
    //
    // If all nodes are catched and nothing else, the cut surface has hit this
    // surface exactly. No need to cut anything. However, the surface might be
    // required for integration.

    const std::vector<Node*> & nodes = Nodes();
    if ( cuts.size()==nodes.size() and AllOnNodes( cuts ) )
    {
      for ( unsigned i=0; i<nodes.size(); ++i )
      {
        unsigned j = ( i+1 ) % nodes.size();
        mesh.NewLine( nodes[i]->point(), nodes[j]->point(), this, &other, element );
      }
      return true;
    }
    return other.FindAmbiguousCutLines( mesh, element, *this, cuts );
  }
  }
}

void GEO::CUT::Side::CreateMissingLines( Creator & creator, Element * element )
{
  std::map<Point*, std::set<Point*> > pg;

  const std::vector<Line*> & cut_lines = CutLines();
  for ( std::vector<Line*>::const_iterator i=cut_lines.begin(); i!=cut_lines.end(); ++i )
  {
    Line * l = *i;
    if ( l->IsCut( element ) )
    {
      Point * p1 = l->BeginPoint();
      Point * p2 = l->EndPoint();
      pg[p1].insert( p2 );
      pg[p2].insert( p1 );
    }
  }

  if ( pg.size() > 2 )
  {
    // Needs to be a proper cycle. No gaps, no forks.

    std::vector<Point*> open;

    for ( std::map<Point*, std::set<Point*> >::iterator i=pg.begin();
          i!=pg.end();
          ++i )
    {
      Point * p = i->first;
      std::set<Point*> & row = i->second;
      if ( row.size() < 2 )
      {
        open.push_back( p );
      }
      else if ( row.size() > 2 )
      {
        throw std::runtime_error( "fork in line cycle" );
      }
    }

    if ( open.size() > 0 )
    {
      std::set<Point*> done;

      std::vector<Point*> open_side_points;
      open_side_points.reserve( 2 );

      const std::vector<Side*> & sides = element->Sides();
      for ( std::vector<Side*>::const_iterator i=sides.begin(); i!=sides.end(); ++i )
      {
        Side * s = *i;
        open_side_points.clear();

        for ( std::vector<Point*>::iterator i=open.begin(); i!=open.end(); ++i )
        {
          Point * p = *i;
          if ( p->IsCut( s ) )
          {
            open_side_points.push_back( p );
          }
        }

        if ( open_side_points.size()==2 )
        {
          creator.NewLine( open_side_points[0], open_side_points[1], s, this, element );
          done.insert( open_side_points[0] );
          done.insert( open_side_points[1] );
        }
#if 0
        else if ( open_side_points.size() > 0 )
        {
          throw std::runtime_error( "illegal number of open points on element side" );
        }
#endif
      }

      if ( done.size() != open.size() )
      {
        throw std::runtime_error( "failed to close open points" );
      }
    }
  }
}

bool GEO::CUT::Side::AllOnNodes( const std::set<Point*> & points )
{
  const std::vector<Node*> & nodes = Nodes();
  for ( std::set<Point*>::const_iterator i=points.begin(); i!=points.end(); ++i )
  {
    Point * p = *i;
    if ( not p->NodalPoint( nodes ) )
    {
      return false;
    }
  }
  return true;
}

void GEO::CUT::Side::GetCutPoints( Element * element, Side & other, std::set<Point*> & cuts )
{
  const std::vector<Edge*> & edges = Edges();
  for ( std::vector<Edge*>::const_iterator i=edges.begin(); i!=edges.end(); ++i )
  {
    Edge * e = *i;
    e->GetCutPoints( element, *this, other, cuts );
  }
}

void GEO::CUT::Side::AddLine( Line* cut_line )
{
  if ( std::find( cut_lines_.begin(), cut_lines_.end(), cut_line )==cut_lines_.end() )
  {
    cut_lines_.push_back( cut_line );
  }
}

GEO::CUT::Facet * GEO::CUT::Side::FindFacet( const std::vector<Point*> & facet_points )
{
  for ( std::vector<Facet*>::const_iterator i=facets_.begin(); i!=facets_.end(); ++i )
  {
    Facet * f = *i;
    if ( f->Equals( facet_points ) )
    {
      return f;
    }
  }
  return NULL;
}

bool GEO::CUT::Side::FindAmbiguousCutLines( Mesh & mesh, Element * element, Side & side, const std::set<Point*> & cut )
{
  return false;
}

void GEO::CUT::Side::GetBoundaryCells( std::set<GEO::CUT::BoundaryCell*> & bcells )
{
  for ( std::vector<Facet*>::iterator i=facets_.begin(); i!=facets_.end(); ++i )
  {
    Facet * f = *i;
    f->GetBoundaryCells( bcells );
  }
}

void GEO::CUT::Side::MakeOwnedSideFacets( Mesh & mesh, Element * element, std::set<Facet*> & facets )
{
  if ( facets_.size()==0 )
  {
    PointGraph point_graph( mesh, element, this, true );

    for ( PointGraph::iterator i=point_graph.begin(); i!=point_graph.end(); ++i )
    {
      const std::vector<Point*> & points = *i;

      Facet * f = mesh.NewFacet( points, this, IsCutSide() );
      if ( f==NULL )
        throw std::runtime_error( "failed to create facet" );
      facets_.push_back( f );
    }
  }

  std::copy( facets_.begin(), facets_.end(), std::inserter( facets, facets.begin() ) );
}

void GEO::CUT::Side::MakeSideCutFacets( Mesh & mesh, Element * element, std::set<Facet*> & facets )
{
  LineSegmentList lsl;

  std::set<Line*> cut_lines;

  for ( std::vector<Line*>::const_iterator i=cut_lines_.begin(); i!=cut_lines_.end(); ++i )
  {
    Line * l = *i;
    if ( l->IsCut( element ) and
         not OnEdge( l->BeginPoint() ) and
         not OnEdge( l->EndPoint() ) )
    {
      cut_lines.insert( l );
    }
  }

  lsl.Create( mesh, element, this, cut_lines, false );

  const std::vector<Teuchos::RCP<LineSegment> > & segments = lsl.Segments();
  for ( std::vector<Teuchos::RCP<LineSegment> >::const_iterator i=segments.begin(); i!=segments.end(); ++i )
  {
    LineSegment & ls = **i;
    if ( ls.IsClosed() )
    {
      const std::vector<Point*> & facet_points = ls.Points();
      Facet * f = FindFacet( facet_points );
      if ( f==NULL )
      {
        // If we have a hole and multiple cuts we have to test which facet the
        // hole belongs to. Not supported now.
        if ( facets_.size()!=1 )
        {
          throw std::runtime_error( "expect side with one (uncut) facet" );
        }
        Facet * hole = mesh.NewFacet( facet_points, this, false );
        facets_[0]->AddHole( hole );
      }
    }
  }
}

void GEO::CUT::Side::MakeInternalFacets( Mesh & mesh, Element * element, std::set<Facet*> & facets )
{
  LineSegmentList lsl;
  lsl.Create( mesh, element, this, false );

  const std::vector<Teuchos::RCP<LineSegment> > & segments = lsl.Segments();

  for ( unsigned i=0; i<segments.size(); ++i )
  {
    LineSegment & ls = *segments[i];

    if ( not ls.IsClosed() )
    {
      //throw std::runtime_error( "expect one closed cut" );

      // Assume this is a cut along one of our edges. So this side is not
      // responsible.
      return;
    }

    Side * s = ls.OnSide( element );
    if ( s!=NULL )
    {
      const std::vector<Point*> & facet_points = ls.Points();
      Facet * f = s->FindFacet( facet_points );
      if ( f!=NULL )
      {
        f->ExchangeSide( this, true );
        facets.insert( f );
        facets_.push_back( f );
      }
      else
      {
        //throw std::runtime_error( "must have matching facet on side" );

        // multiple facets on one cut side within one elemenet: this is a
        // levelset case
        Facet * f = mesh.NewFacet( facet_points, this, true );
        facets.insert( f );
        facets_.push_back( f );
      }
    }
    else
    {
      // insert new internal facet
      const std::vector<Point*> & facet_points = ls.Points();
      Facet * f = mesh.NewFacet( facet_points, this, true );
      facets.insert( f );
      facets_.push_back( f );
    }
  }
}

bool GEO::CUT::Side::OnSide( const std::set<Point*, PointPidLess> & points )
{
  if ( nodes_.size()==points.size() )
  {
    for ( std::vector<Node*>::iterator i=nodes_.begin();
          i!=nodes_.end();
          ++i )
    {
      Node * n = *i;
      if ( points.count( n->point() )==0 )
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool GEO::CUT::Side::OnEdge( Point * point )
{
  for ( std::vector<Edge*>::const_iterator i=edges_.begin(); i!=edges_.end(); ++i )
  {
    Edge * e = *i;
    if ( point->IsCut( e ) )
    {
      return true;
    }
  }
  return false;
}

bool GEO::CUT::Side::OnEdge( Line * line )
{
  for ( std::vector<Edge*>::const_iterator i=edges_.begin(); i!=edges_.end(); ++i )
  {
    Edge * e = *i;
    if ( line->OnEdge( e ) )
    {
      return true;
    }
  }
  return false;
}

bool GEO::CUT::Side::HaveCommonEdge( Side & side )
{
  const std::vector<Edge*> & other_edges = side.Edges();
  for ( std::vector<Edge*>::const_iterator i=edges_.begin(); i!=edges_.end(); ++i )
  {
    Edge * e = *i;
    if ( std::find( other_edges.begin(), other_edges.end(), e )!=other_edges.end() )
    {
      return true;
    }
  }
  return false;
}

void GEO::CUT::Side::Print()
{
  std::cout << "[ ";
  for ( std::vector<Edge*>::iterator i=edges_.begin(); i!=edges_.end(); ++i )
  {
    Edge * e = *i;
    e->Print();
    std::cout << " ; ";
  }
  std::cout << " ]";
}

GEO::CUT::Node * GEO::CUT::Side::OnNode( const LINALG::Matrix<3,1> & x )
{
  LINALG::Matrix<3,1> nx;
  for ( std::vector<Node*>::iterator i=nodes_.begin(); i!=nodes_.end(); ++i )
  {
    Node * n = *i;
    n->Coordinates( nx.A() );
    nx.Update( -1, x, 1 );
    if ( nx.Norm2() < MINIMALTOL )
    {
      return n;
    }
  }
  return NULL;
}

bool GEO::CUT::Side::IsCut()
{
  if ( facets_.size()>1 )
    return true;
  if ( facets_[0]->OnCutSide() )
    return true;
  return false;
}

bool GEO::CUT::ConcreteSide<DRT::Element::tri3>::LocalCoordinates( const LINALG::Matrix<3,1> & xyz, LINALG::Matrix<3,1> & rst )
{
  Position2d<DRT::Element::tri3> pos( *this, xyz );
  bool success = pos.Compute();
  if ( not success )
  {
//     throw std::runtime_error( "global point not within element" );
  }
  rst = pos.LocalCoordinates();
  return success;
}

bool GEO::CUT::ConcreteSide<DRT::Element::quad4>::LocalCoordinates( const LINALG::Matrix<3,1> & xyz, LINALG::Matrix<3,1> & rst )
{
  Position2d<DRT::Element::quad4> pos( *this, xyz );
  bool success = pos.Compute();
  if ( not success )
  {
//     throw std::runtime_error( "global point not within element" );
  }
  rst = pos.LocalCoordinates();
  return success;
}
