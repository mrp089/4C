#include "../linalg/linalg_gauss.H"

#include "cut_element.H"
#include "cut_mesh.H"
#include "cut_boundarycell.H"
#include "cut_volumecell.H"
#include "cut_kernel.H"
#include "cut_options.H"
#include "cut_triangulateFacet.H"

GEO::CUT::Facet::Facet( Mesh & mesh, const std::vector<Point*> & points, Side * side, bool cutsurface )
  : points_( points ),
    parentside_( side ),
    planar_( false ),
    planar_known_( false ),
    position_( cutsurface ? Point::oncutsurface : Point::undecided ),
    isPlanarComputed_( false )
{
  FindCornerPoints();

  if ( cutsurface )
  {
    for ( std::vector<Point*>::const_iterator i=points.begin(); i!=points.end(); ++i )
    {
      Point * p = *i;
      p->Position( Point::oncutsurface );
    }
  }
  else
  {
    // On multiple cuts there are facets on element sides that belong to an
    // old cut surface. Thus if all nodes are on a cut surface, the facet is
    // as well.
    bool allonsurface = true;
    for ( std::vector<Point*>::const_iterator i=points.begin(); i!=points.end(); ++i )
    {
      Point * p = *i;
      if ( p->Position()!=Point::oncutsurface )
      {
        allonsurface = false;
        break;
      }
    }
    if ( allonsurface )
    {
      // If my side has an id this facet is actually on a cut
      // surface. Otherwise it could be an inside or outside facet. The actual
      // decision does not matter much.
      if ( side->Id()>-1 )
      {
        position_ = Point::oncutsurface;
      }
      else
      {
        position_ = Point::undecided;
      }
    }
  }
  for ( std::vector<Point*>::const_iterator i=points.begin(); i!=points.end(); ++i )
  {
    Point * p = *i;
    p->Register( this );
  }
}

void GEO::CUT::Facet::Register( VolumeCell * cell )
{
  cells_.insert( cell );
  if ( cells_.size() > 2 )
  {
#ifdef DEBUGCUTLIBRARY
    {
      std::ofstream file( "volumecells.plot" );
      for ( plain_volumecell_set::iterator i=cells_.begin(); i!=cells_.end(); ++i )
      {
        VolumeCell * vc = *i;
        vc->Print( file );
      }
    }
#endif
    throw std::runtime_error( "too many volume cells at facet" );
  }
}

void GEO::CUT::Facet::DisconnectVolume( VolumeCell * cell )
{
  cells_.erase( cell );
}

int GEO::CUT::Facet::SideId()
{
  return parentside_->Id();
}

int GEO::CUT::Facet::PositionSideId()
{
  int sid = SideId();
  switch ( position_ )
  {
  case Point::undecided:
    throw std::runtime_error( "undecided facet position" );
  case Point::inside:
    if ( sid > -1 )
      return sid;
    else
      return Point::inside;
  case Point::outside:
    if ( sid > -1 )
      return sid;
    else
      return Point::outside;
  case Point::oncutsurface:
    if ( sid > -1 )
      return sid;
    else
      throw std::runtime_error( "cannot have facet on cut side without cut side" );
  }
  throw std::runtime_error( "unhandled position" );
}

void GEO::CUT::Facet::Coordinates( double * x )
{
  for ( std::vector<Point*>::const_iterator i=points_.begin(); i!=points_.end(); ++i )
  {
    Point * p = *i;
    p->Coordinates( x );
    x += 3;
  }
}

void GEO::CUT::Facet::CornerCoordinates( double * x )
{
  FindCornerPoints();
  for ( std::vector<Point*>::const_iterator i=corner_points_.begin(); i!=corner_points_.end(); ++i )
  {
    Point * p = *i;
    p->Coordinates( x );
    x += 3;
  }
}

void GEO::CUT::Facet::GetAllPoints( Mesh & mesh, PointSet & cut_points, bool dotriangulate )
{
  if ( IsPlanar( mesh, dotriangulate ) )
  {
    std::copy( points_.begin(), points_.end(), std::inserter( cut_points, cut_points.begin() ) );
    for ( plain_facet_set::iterator i=holes_.begin(); i!=holes_.end(); ++i )
    {
      Facet * h = *i;
      h->GetAllPoints( mesh, cut_points );
    }
  }
  else
  {
    for ( std::vector<std::vector<Point*> >::iterator i=triangulation_.begin();
          i!=triangulation_.end();
          ++i )
    {
      std::vector<Point*> & tri = *i;
      std::copy( tri.begin(), tri.end(),
                 std::inserter( cut_points, cut_points.begin() ) );
    }
  }
}

void GEO::CUT::Facet::AddHole( Facet * hole )
{
  for ( std::vector<Point*>::iterator i=hole->points_.begin(); i!=hole->points_.end(); ++i )
  {
    Point * p = *i;
    p->Register( this );
  }
  holes_.insert( hole );
}

bool GEO::CUT::Facet::IsPlanar( Mesh & mesh, bool dotriangulate )
{
  if ( dotriangulate )
  {
    //if ( not IsTriangulated() and points_.size() > 3 )
    if ( not IsTriangulated() and corner_points_.size() > 3 )
    {
      CreateTriangulation( mesh, points_ );
      planar_known_ = true;
      planar_ = false;
    }
  }

  if ( not planar_known_ )
  {
    planar_ = IsPlanar( mesh, points_ );
    planar_known_ = true;
  }

  return planar_;
}

bool GEO::CUT::Facet::IsPlanar( Mesh & mesh, const std::vector<Point*> & points )
{
  //TEUCHOS_FUNC_TIME_MONITOR( "GEO::CUT::Facet::IsPlanar" );

  if ( isPlanarComputed_ )
    return isPlanar_;


  LINALG::Matrix<3,1> x1;
  LINALG::Matrix<3,1> x2;
  LINALG::Matrix<3,1> x3;

  LINALG::Matrix<3,1> b1;
  LINALG::Matrix<3,1> b2;
  LINALG::Matrix<3,1> b3;

  unsigned i = Normal( points, x1, x2, x3, b1, b2, b3 );
  if ( i==0 ) // all on one line is ok
  {
    isPlanarComputed_ = true;
    isPlanar_ = true;
    return true;
  }

  LINALG::Matrix<3,3> A;
  std::copy( b1.A(), b1.A()+3, A.A() );
  std::copy( b2.A(), b2.A()+3, A.A()+3 );
  std::copy( b3.A(), b3.A()+3, A.A()+6 );

  //std::copy( points.begin(), points.end(), std::ostream_iterator<Point*>( std::cout, "; " ) );
  //std::cout << "\n";

  for ( ++i; i<points.size(); ++i )
  {
    Point * p = points[i];
    p->Coordinates( x3.A() );

    x3.Update( -1, x1, 1 );

    LINALG::Matrix<3,3> B;
    B = A;
    x2 = 0;
    double det = LINALG::gaussElimination<true, 3>( B, x3, x2 );
    if ( fabs( det ) < LINSOLVETOL )
    {
      throw std::runtime_error( "failed to find point position" );
    }

    if ( fabs( x2( 2 ) ) > PLANARTOL )
    {
      // there is one point that is not within the plain

      CreateTriangulation( mesh, points );

      isPlanarComputed_ = true;
      isPlanar_ = false;
      return false;
    }
  }
  isPlanarComputed_ = true;
  isPlanar_ = true;
  return true;
}

/*-------------------------------------------------------------------------------------------------------*
      Find the middle point (M) and join them with the corners of the facet to create triangles
      Works only for convex facets, because of the middle point calculation

                                           4   ________ 3
                                              /.     . \
                                             /  .   .   \
                                            /    . .     \
                                        5  /      . M     \ 2
                                           \     . .      /
                                            \   .   .    /
                                             \._______./
                                            0            1
      Whenever possible call SplitFacet() instead of triangulation as it is very effective
*--------------------------------------------------------------------------------------------------------*/
void GEO::CUT::Facet::CreateTriangulation( Mesh & mesh, const std::vector<Point*> & points )
{
#if 0 //old implementation of Ulli -- failed when --test=alex55 when called for directDivergence
  LINALG::Matrix<3,1> x1;
  LINALG::Matrix<3,1> x2;
  LINALG::Matrix<3,1> x3;

  LINALG::Matrix<3,1> b1;
//   LINALG::Matrix<3,1> b2;
//   LINALG::Matrix<3,1> b3;

  std::vector<Point*> pts( points );
  pts.push_back( points[0] );
  std::vector<std::vector<Point*> > lines;

  for ( unsigned pos = 0; pos<pts.size(); )
  {
    lines.push_back( std::vector<Point*>() );
    std::vector<Point*> * current_line = & lines.back();

    current_line->push_back( pts[pos++] );
    if ( pos==pts.size() )
      break;
    current_line->push_back( pts[pos++] );

    ( *current_line )[0]->Coordinates( x1.A() );
    ( *current_line )[1]->Coordinates( x2.A() );

    b1.Update( 1, x2, -1, x1, 0 );

    if ( b1.Norm2() < std::numeric_limits<double>::min() )
    {
      throw std::runtime_error( "same point in facet not supported" );
    }

    for ( ; pos<pts.size(); ++pos )
    {
      Point * p = pts[pos];
      p->Coordinates( x3.A() );

      x3.Update( -1, x1, 1 );

      double f = x3.Norm2() / b1.Norm2();
      x3.Update( -f, b1, 1 );
      if ( x3.Norm2() > TOLERANCE )
      {
        --pos;
        break;
      }
      current_line->push_back( p );
    }
  }

  if ( lines.size()<4 )
    throw std::runtime_error( "too few lines" );

  // Invent point in middle of surface. Surface needs to be convex to
  // allow all this.

  x1 = 0;
  x2 = 0;

  for ( std::vector<Point*>::const_iterator i=points.begin(); i!=points.end(); ++i )
  {
    Point * p = *i;
    p->Coordinates( x1.A() );
    x2.Update( 1, x1, 1 );
  }

  x2.Scale( 1./points.size() );

  Point * p1 = mesh.NewPoint( x2.A(), NULL, ParentSide() );
  p1->Position( Position() );
  p1->Register( this );
  //p1->Register( parentside_ );
  triangulation_.clear();

  for ( std::vector<std::vector<Point*> >::iterator i=lines.begin(); i!=lines.end(); ++i )
  {
    std::vector<Point*> & line = *i;
    triangulation_.push_back( std::vector<Point*>() );

    std::vector<Point*> & newtri = triangulation_.back();
    newtri.push_back( p1 );

    //std::copy( line.begin(), line.end(), std::back_inserter( triangulation_.back() ) );
    newtri.insert( newtri.end(), line.begin(), line.end() );

    //std::copy( line.begin(), line.end(), std::ostream_iterator<Point*>( std::cout, "; " ) );
    //std::cout << "\n";
  }
#else  //new simpler triangulation implementation (Sudhakar 04/12)
  std::vector<Point*> pts( points );

  //Find the middle point
  LINALG::Matrix<3,1> cur;
  LINALG::Matrix<3,1> avg(true); //fill with zeros
  for( std::vector<Point*>::iterator i=pts.begin();i!=pts.end();i++ )
  {
    Point* p1 = *i;
    p1->Coordinates(cur.A());
    avg.Update(1.0,cur,1.0);
  }
  avg.Scale(1.0/pts.size());
  Point * p_mid = mesh.NewPoint( avg.A(), NULL, ParentSide() );
  p_mid->Position( Position() );
  p_mid->Register( this );

  //form triangles and store them in triangulation_
  triangulation_.clear();
  std::vector<Point*> newtri(3);
  newtri[0] = p_mid;
  for( unsigned i=0;i<pts.size();i++ )
  {
    Point* pt1 = pts[i];
    Point* pt2 = pts[(i+1)%pts.size()];

    newtri[1] = pt1;
    newtri[2] = pt2;
    triangulation_.push_back(newtri);
  }
#endif
}

void GEO::CUT::Facet::GetNodalIds( Mesh & mesh, const std::vector<Point*> & points, std::vector<int> & nids )
{
  for ( std::vector<Point*>::const_iterator i=points.begin(); i!=points.end(); ++i )
  {
    Point * p = *i;
    Node * n = p->CutNode();
    if ( n==NULL )
    {
      plain_int_set point_id;
      point_id.insert( p->Id() );
      n = mesh.GetNode( point_id, p->X() );
    }
    nids.push_back( n->Id() );
  }
}

bool GEO::CUT::Facet::Equals( const std::vector<Point*> & my_points,
                              const std::vector<Point*> & facet_points )
{
  if ( my_points.size()!=facet_points.size() or holes_.size()>0 )
    return false;

  unsigned size = my_points.size();
  unsigned shift = std::find( my_points.begin(), my_points.end(), facet_points.front() ) - my_points.begin();

  bool forward_match = true;
  for ( unsigned i=0; i<size; ++i )
  {
    unsigned j = ( i+shift ) % size;
    if ( my_points[j] != facet_points[i] )
    {
      forward_match = false;
      break;
    }
  }
  if ( not forward_match )
  {
    for ( unsigned i=0; i<size; ++i )
    {
      unsigned j = ( shift+size-i ) % size;
      if ( my_points[j] != facet_points[i] )
      {
        return false;
      }
    }
  }
  return true;
}

bool GEO::CUT::Facet::IsCutSide( Side * side )
{
  if ( parentside_==side )
    return false;

  int count = 0;
  for ( std::vector<Point*>::iterator i=points_.begin(); i!=points_.end(); ++i )
  {
    Point * p = *i;
    if ( p->IsCut( side ) )
    {
      count += 1;
      if ( count > 1 )
      {
        return true;
      }
    }
  }
  return false;
}

void GEO::CUT::Facet::Position( Point::PointPosition pos )
{

#ifdef DEBUGCUTLIBRARY
  // safety check, if the position of a facet changes from one side to the other
  if( (position_ == Point::inside and pos == Point::outside) or
      (position_ == Point::outside and pos == Point::inside) )
  {
    //this->Print(std::cout);
    dserror("Are you sure that you want to change the facet-position from inside to outside or vice versa?");
  }
#endif

  if( position_ == Point::undecided)
  {

    if ( position_ != pos )
    {
      position_ = pos;
      if ( pos==Point::outside or pos==Point::inside )
      {
        for ( std::vector<Point*>::iterator i=points_.begin(); i!=points_.end(); ++i )
        {
          Point * p = *i;
          if ( p->Position()==Point::undecided )
          {
            p->Position( pos );
          }
        }
        for ( plain_volumecell_set::iterator i=cells_.begin(); i!=cells_.end(); ++i )
        {
          VolumeCell * c = *i;
          c->Position( pos );
        }
      }
    }
  }
}

void GEO::CUT::Facet::GetLines( std::map<std::pair<Point*, Point*>, plain_facet_set > & lines )
{
#if 0
  // We are interested in the surrounding lines of the facet. Thus the
  // internal lines that result from a triangulation are not wanted
  // here. (There are no other facets connected to any of our internal lines.)
  if ( IsTriangulated() )
  {
    for ( std::vector<std::vector<Point*> >::iterator i=triangulation_.begin();
          i!=triangulation_.end();
          ++i )
    {
      std::vector<Point*> & points = *i;
      GetLines( points, lines );
    }
  }
  else
#endif
  {
    GetLines( points_, lines );

    // add hole lines but do not connect with parent facet
    for ( plain_facet_set::iterator i=holes_.begin(); i!=holes_.end(); ++i )
    {
      Facet * hole = *i;
      hole->GetLines( lines );
    }
  }
}

void GEO::CUT::Facet::GetLines( const std::vector<Point*> & points,
                                std::map<std::pair<Point*, Point*>, plain_facet_set > & lines )
{
  unsigned length = points.size();
  for ( unsigned i=0; i<length; ++i )
  {
    unsigned j = ( i+1 ) % length;

    Point * p1 = points[i];
    Point * p2 = points[j];

    if ( p1->Id() < p2->Id() )
    {
      lines[std::make_pair( p1, p2 )].insert( this );
    }
    else if ( p1->Id() > p2->Id() )
    {
      lines[std::make_pair( p2, p1 )].insert( this );
    }
    else
      dserror( "line creation with identical begin and end points\n" );

  }
}

bool GEO::CUT::Facet::IsLine( Point * p1, Point * p2 )
{
  if ( IsTriangulated() )
  {
    for ( std::vector<std::vector<Point*> >::iterator i=triangulation_.begin();
          i!=triangulation_.end();
          ++i )
    {
      std::vector<Point*> & points = *i;
      if ( IsLine( points, p1, p2 ) )
        return true;
    }
  }
  else
  {
    if ( IsLine( points_, p1, p2 ) )
      return true;
    for ( plain_facet_set::iterator i=holes_.begin(); i!=holes_.end(); ++i )
    {
      Facet * hole = *i;
      if ( hole->IsLine( p1, p2 ) )
        return true;
    }
  }
  return false;
}

bool GEO::CUT::Facet::IsLine( const std::vector<Point*> & points, Point * p1, Point * p2 )
{
  std::vector<Point*>::const_iterator i1 = std::find( points.begin(), points.end(), p1 );
  if ( i1!=points.end() )
  {
    std::vector<Point*>::const_iterator i2 = i1 + 1;
    if ( i2!=points.end() )
    {
      if ( *i2 == p2 )
      {
        return true;
      }
    }
    else
    {
      i2 = points.begin();
      if ( *i2 == p2 )
      {
        return true;
      }
    }
    if ( i1!=points.begin() )
    {
      i2 = i1 - 1;
      if ( *i2 == p2 )
      {
        return true;
      }
    }
    else
    {
      i2 = points.end() - 1;
      if ( *i2 == p2 )
      {
        return true;
      }
    }
  }
  return false;
}

bool GEO::CUT::Facet::Contains( Point * p ) const
{
  if ( IsTriangulated() )
  {
    for ( std::vector<std::vector<Point*> >::const_iterator i=triangulation_.begin();
          i!=triangulation_.end();
          ++i )
    {
      const std::vector<Point*> & points = *i;
      if ( std::find( points.begin(), points.end(), p ) != points.end() )
        return true;
    }
  }
  else
  {
    if ( std::find( points_.begin(), points_.end(), p ) != points_.end() )
      return true;
    for ( plain_facet_set::const_iterator i=holes_.begin(); i!=holes_.end(); ++i )
    {
      Facet * hole = *i;
      if ( hole->Contains( p ) )
        return true;
    }
  }
  return false;
}

bool GEO::CUT::Facet::Contains( const std::vector<Point*> & side ) const
{
  for ( std::vector<Point*>::const_iterator i=side.begin(); i!=side.end(); ++i )
  {
    Point * p = *i;
    if ( not Contains( p ) )
    {
      return false;
    }
  }
  return true;
}

bool GEO::CUT::Facet::ContainsSome( const std::vector<Point*> & side ) const
{
  for ( std::vector<Point*>::const_iterator i=side.begin(); i!=side.end(); ++i )
  {
    Point * p = *i;
    if ( Contains( p ) )
    {
      return true;
    }
  }
  return false;
}

bool GEO::CUT::Facet::Touches( Facet * f )
{
  for ( std::vector<Point*>::iterator i=points_.begin(); i!=points_.end(); ++i )
  {
    Point * p = *i;
    if ( f->Contains( p ) )
    {
      return true;
    }
  }
  return false;
}

GEO::CUT::VolumeCell * GEO::CUT::Facet::Neighbor( VolumeCell * cell )
{
  if ( cells_.size() > 2 )
  {
#ifdef DEBUGCUTLIBRARY
    {
      std::ofstream file( "volumes.plot" );
      for ( plain_volumecell_set::iterator i=cells_.begin(); i!=cells_.end(); ++i )
      {
        VolumeCell * vc = *i;
        vc->Print( file );
      }
    }
#endif
    throw std::runtime_error( "can only have two neighbors" );
  }
  if ( cells_.count( cell )==0 )
    throw std::runtime_error( "not my neighbor" );
  for ( plain_volumecell_set::iterator i=cells_.begin(); i!=cells_.end(); ++i )
  {
    VolumeCell * vc = *i;
    if ( vc != cell )
    {
      return vc;
    }
  }
  return NULL;
}

void GEO::CUT::Facet::Neighbors( Point * p,
                                 const plain_volumecell_set & cells,
                                 const plain_volumecell_set & done,
                                 plain_volumecell_set & connected,
                                 plain_element_set & elements )
{
  for ( plain_volumecell_set::iterator i=cells_.begin(); i!=cells_.end(); ++i )
  {
    VolumeCell * c = *i;
    if ( cells.count( c )>0 )
    {
      if ( done.count( c )==0 and
           connected.count( c )==0 and
           elements.count( c->ParentElement() )==0 )
      {
        connected.insert( c );
        elements.insert( c->ParentElement() );
        c->Neighbors( p, cells, done, connected, elements );
      }
    }
  }
}


void GEO::CUT::Facet::Neighbors( Point * p,
                                 const plain_volumecell_set & cells,
                                 const plain_volumecell_set & done,
                                 plain_volumecell_set & connected)
{

  for ( plain_volumecell_set::iterator i=cells_.begin(); i!=cells_.end(); ++i )
  {

    VolumeCell * c = *i;
    if ( cells.count( c )>0 )
    {
      if ( done.count( c )==0 and
           connected.count( c )==0 )
      {
        connected.insert( c );
        c->Neighbors( p, cells, done, connected);
      }
    }
  }
}

bool GEO::CUT::Facet::Equals( DRT::Element::DiscretizationType distype )
{
  if ( holes_.size()==0 )
  {
    FindCornerPoints();
    switch ( distype )
    {
    case DRT::Element::quad4:
      return KERNEL::IsValidQuad4( corner_points_ );
    case DRT::Element::tri3:
      return KERNEL::IsValidTri3( corner_points_ );
    default:
      throw std::runtime_error( "unsupported distype requested" );
    }
  }
  return false;
}

unsigned GEO::CUT::Facet::Normal( const std::vector<Point*> & points,
                                  LINALG::Matrix<3,1> & x1,
                                  LINALG::Matrix<3,1> & x2,
                                  LINALG::Matrix<3,1> & x3,
                                  LINALG::Matrix<3,1> & b1,
                                  LINALG::Matrix<3,1> & b2,
                                  LINALG::Matrix<3,1> & b3 )
{
  unsigned pointsize = points.size();
  if ( pointsize < 3 )
    return 0;

  points[0]->Coordinates( x1.A() );
  points[1]->Coordinates( x2.A() );

  b1.Update( 1, x2, -1, x1, 0 );
  b1.Scale( 1./b1.Norm2() );

  if ( b1.Norm2() < std::numeric_limits<double>::min() )
    throw std::runtime_error( "same point in facet not supported" );

  bool found = false;
  unsigned i=2;
  for ( ; i<pointsize; ++i )
  {
    Point * p = points[i];
    p->Coordinates( x3.A() );

    b2.Update( 1, x3, -1, x1, 0 );
    b2.Scale( 1./b2.Norm2() );

    // cross product to get the normal at the point
    b3( 0 ) = b1( 1 )*b2( 2 ) - b1( 2 )*b2( 1 );
    b3( 1 ) = b1( 2 )*b2( 0 ) - b1( 0 )*b2( 2 );
    b3( 2 ) = b1( 0 )*b2( 1 ) - b1( 1 )*b2( 0 );

    if ( b3.Norm2() > PLANARTOL )
    {
      found = true;
      break;
    }
  }
  if ( not found )
  {
    // all on one line, no normal
    return 0;
  }

  b3.Scale( 1./b3.Norm2() );
  return i;
}

void GEO::CUT::Facet::TriangulationPoints( PointSet & points )
{
  for ( std::vector<std::vector<Point*> >::const_iterator i=triangulation_.begin();
        i!=triangulation_.end();
        ++i )
  {
    const std::vector<Point*> & tri = *i;
    std::copy( tri.begin(), tri.end(), std::inserter( points, points.begin() ) );
  }
}

void GEO::CUT::Facet::NewTri3Cell( Mesh & mesh, VolumeCell * volume, const std::vector<Point*> & points, plain_boundarycell_set & bcells )
{
  BoundaryCell * bc = mesh.NewTri3Cell( volume, this, points );
  bcells.insert( bc );
}


void GEO::CUT::Facet::NewQuad4Cell( Mesh & mesh, VolumeCell * volume, const std::vector<Point*> & points, plain_boundarycell_set & bcells )
{
  if ( mesh.CreateOptions().GenQuad4() )
  {
    BoundaryCell * bc = mesh.NewQuad4Cell( volume, this, points );
    bcells.insert( bc );
  }
  else
  { // split into two tri3 cells
    std::vector<Point*> tri3_points = points;
    tri3_points.pop_back(); // erase the last (fourth) point to obtain points (1,2,3)
    BoundaryCell * bc = mesh.NewTri3Cell( volume, this, tri3_points );
    bcells.insert( bc );
    tri3_points.erase( tri3_points.begin()+1 ); // erase the second point (1,3)
    tri3_points.push_back( points.back() ); // add the last point again (1,3,4)
    bc = mesh.NewTri3Cell( volume, this, tri3_points );
    bcells.insert( bc );
  }
}

void GEO::CUT::Facet::NewArbitraryCell( Mesh & mesh, VolumeCell * volume, const std::vector<Point*> & points,
    plain_boundarycell_set & bcells, const DRT::UTILS::GaussIntegration& gp, const LINALG::Matrix<3,1>& normal )
{
  BoundaryCell* bc = mesh.NewArbitraryCell(volume, this, points, gp, normal);
  bcells.insert( bc );
}

/// unused, see comment
void GEO::CUT::Facet::GetBoundaryCells( plain_boundarycell_set & bcells )
{
  if ( cells_.size()==0 )
    throw std::runtime_error( "no volume cells" );

  dserror("do not use this function at the moment -> Read comment!");

  // when asking the facet for boundary cells is it questionable which cells you want to have,
  // for Tesselation boundary cells are stored for each volumecell (inside and outside) independently,
  // the f->GetBoundaryCells then return the bcs just for the first vc which is fine.
  // For DirectDivergence bcs are created just for outside vcs and therefore the return of f->GetBoundaryCells does not work properly
  // as it can happen that the first vc of the facet is an inside vc which does not store the bcs.
  // We have to restructure the storage of bcs. bcs should be stored unique! for each cut-facet and if necessary also for non-cut facets
  // between elements. The storage of boundary-cells to the volume-cells is not right way to do this!

  // TODO: we should not ask the volume-cells for boundary cells!!!

  // it is sufficient to take the boundary cells only from one of the two adjacent volume cells
  VolumeCell * vc = *cells_.begin();

  const plain_boundarycell_set & vbcells = vc->BoundaryCells();
  for ( plain_boundarycell_set::const_iterator i=vbcells.begin();
        i!=vbcells.end();
        ++i )
  {
    BoundaryCell * bc = *i;
    if ( bc->GetFacet()==this )
    {
      bcells.insert( bc );
    }
  }
}

void GEO::CUT::Facet::TestFacetArea( double tolerance )
{
  if ( OnCutSide() and cells_.size() > 1 )
  {
    std::vector<double> area;
    area.reserve( 2 );
    for ( plain_volumecell_set::iterator i=cells_.begin(); i!=cells_.end(); ++i )
    {
      VolumeCell * vc = *i;

      area.push_back( 0. );
      double & a = area.back();

      const plain_boundarycell_set & vbcells = vc->BoundaryCells();
      for ( plain_boundarycell_set::const_iterator i=vbcells.begin();
            i!=vbcells.end();
            ++i )
      {
        BoundaryCell * bc = *i;
        if ( bc->GetFacet()==this )
        {
          a += bc->Area();
        }
      }
    }
    if ( area.size() != 2 )
    {
      throw std::runtime_error( "expect two volume cells at facet" );
    }
    double diff = area[0] - area[1];
    if ( fabs( diff ) >= tolerance )
    {
      std::stringstream str;
      str << "area mismatch: a1=" << area[0]
          << " a2=" << area[1]
          << " diff=" << diff;
      //throw std::runtime_error( str.str() );
      std::cout << "WARNING: " << str.str() << "\n";
    }
  }
}

void GEO::CUT::Facet::FindCornerPoints()
{
  if ( corner_points_.size()==0 )
  {
#if 1
    corner_points_ = points_;
#else
    KERNEL::FindCornerPoints( points_, corner_points_ );
#endif
  }
}

void GEO::CUT::Facet::Print( std::ostream & stream )
{
  stream << "# Facet: " << "numpoints " << points_.size()
         << "\n# ";
  std::copy( points_.begin(), points_.end(), std::ostream_iterator<Point*>( stream, " " ) );
  stream << "\n";
  if ( points_.size() > 0 )
  {
    LINALG::Matrix<3,1> middle;
    LINALG::Matrix<3,1> x;

    middle = 0;
    for ( std::vector<Point*>::iterator i=points_.begin(); i!=points_.end(); ++i )
    {
      Point * p = *i;
      p->Coordinates( x.A() );
      middle.Update( 1, x, 1 );
      //p->Plot( stream );
    }
    middle.Scale( 1./points_.size() );
    for ( unsigned i=0; i<=points_.size(); ++i )
    {
      Point * p = points_[i % points_.size()];
      p->Coordinates( x.A() );
      x.Update( -1, middle, 1 );
      x.Scale( 0.8 );
      x.Update( 1, middle, 1 );
      stream << std::setprecision( 10 ) << x( 0, 0 ) << " "
             << std::setprecision( 10 ) << x( 1, 0 ) << " "
             << std::setprecision( 10 ) << x( 2, 0 ) << " "
             << std::setprecision( 10 ) << p->X()[0] << " "
             << std::setprecision( 10 ) << p->X()[1] << " "
             << std::setprecision( 10 ) << p->X()[2] << " "
             << "# " << p->Id()
             << "\n";
    }
    stream << "\n\n";

    for ( plain_facet_set::iterator i=holes_.begin(); i!=holes_.end(); ++i )
    {
      Facet * hole = *i;
      hole->Print( stream );
    }
  }
}

bool GEO::CUT::Facet::IsTriangle( const std::vector<Point*> & tri ) const
{
  if ( tri.size()!=3 )
    throw std::runtime_error( "three points expected" );

  return points_.size()==3 and not IsTriangulated() and not HasHoles() and Contains( tri );
}

bool GEO::CUT::Facet::IsTriangulatedSide( const std::vector<Point*> & tri ) const
{
  if ( tri.size()!=3 )
    throw std::runtime_error( "three points expected" );

  for ( std::vector<std::vector<Point*> >::const_iterator i=triangulation_.begin();
        i!=triangulation_.end();
        ++i )
  {
    const std::vector<Point*> & t = *i;
    bool found = true;
    for ( std::vector<Point*>::const_iterator i=tri.begin(); i!=tri.end(); ++i )
    {
      Point * p = *i;
      if ( std::find( t.begin(), t.end(), p )==t.end() )
      {
        found = false;
        break;
      }
    }
    if ( found )
    {
      return true;
    }
  }
  return false;
}

unsigned GEO::CUT::Facet::NumPoints()
{
  unsigned numpoints = points_.size();
  if ( IsTriangulated() )
    return numpoints + 1;
  for ( plain_facet_set::iterator i=holes_.begin(); i!=holes_.end(); ++i )
  {
    Facet * hole = *i;
    numpoints += hole->NumPoints();
  }
  return numpoints;
}

GEO::CUT::Point * GEO::CUT::Facet::OtherPoint( Point * p1, Point * p2 )
{
  Point * result = NULL;
  if ( not HasHoles() and not IsTriangulated() and Points().size()==3 )
  {
    for ( std::vector<Point*>::iterator i=points_.begin(); i!=points_.end(); ++i )
    {
      Point * p = *i;
      if ( p != p1 and p != p2 )
      {
        if ( result == NULL )
        {
          result = p;
        }
        else
        {
          throw std::runtime_error( "point not unique" );
        }
      }
    }
  }
  else
  {
    throw std::runtime_error( "plain triangular facet required" );
  }
  return result;
}

/*-----------------------------------------------------------------------------------------------------------*
  return the local coordinates of corner points with respect to the given element
  if shadow=true, then the mapping is w.r. to the parent quad element from which this element is derived
*------------------------------------------------------------------------------------------------------------*/
void GEO::CUT::Facet::CornerPointsLocal( Element *elem1, std::vector<std::vector<double> > & cornersLocal, bool shadow )
{
  //TEUCHOS_FUNC_TIME_MONITOR( "GEO::CUT::Facet::CornerPointsLocal" );

  //TODO: this routine is expensive
  cornersLocal.clear();

  const std::vector<Point*> & corners = CornerPoints();
  int mm=0;

  for(std::vector<Point*>::const_iterator k=corners.begin();k!=corners.end();k++)
  {
      std::vector<double> pt_local;
      const Point* po = *k;
      const double * coords = po->X();
      LINALG::Matrix<3,1> glo,loc;

      glo(0,0) = coords[0];
      glo(1,0) = coords[1];
      glo(2,0) = coords[2];

      //TODO: do we really need call a Newton here?
      if( shadow and elem1->isShadow() )
        elem1->LocalCoordinatesQuad( glo, loc );
      else
        elem1->LocalCoordinates(glo,loc);

      pt_local.push_back(loc(0,0));
      pt_local.push_back(loc(1,0));
      pt_local.push_back(loc(2,0));

      cornersLocal.push_back(pt_local);
      mm++;
  }
}

/*-----------------------------------------------------------------------*
          Split the facet into a number of tri and quad cells
*------------------------------------------------------------------------*/
 void GEO::CUT::Facet::SplitFacet( const std::vector<Point*> & points )
 {
   TriangulateFacet tf( points );
   tf.SplitFacet();
   splitCells_ = tf.GetSplitCells();
 }

 /*-----------------------------------------------------------------------*

 *------------------------------------------------------------------------*/
 std::ostream & operator<<( std::ostream & stream, GEO::CUT::Facet & f )
 {
   stream << "facet: {";
   if ( f.IsTriangulated() )
   {
     const std::vector<std::vector<GEO::CUT::Point*> > & triangulation = f.Triangulation();
     for ( std::vector<std::vector<GEO::CUT::Point*> >::const_iterator i=triangulation.begin();
           i!=triangulation.end();
           ++i )
     {
       const std::vector<GEO::CUT::Point*> & tri = *i;
       stream << "{";
       for ( std::vector<GEO::CUT::Point*>::const_iterator i=tri.begin(); i!=tri.end(); ++i )
       {
       GEO::CUT::Point * p = *i;
         p->Print( stream );
         stream << ",";
       }
       stream << "},";
     }
   }
   else
   {
     const std::vector<GEO::CUT::Point*> & points = f.Points();
     for ( std::vector<GEO::CUT::Point*>::const_iterator i=points.begin(); i!=points.end(); ++i )
     {
       GEO::CUT::Point * p = *i;
       p->Print( stream );
       stream << ",";
     }
     if ( f.HasHoles() )
     {
       const GEO::CUT::plain_facet_set & holes = f.Holes();
       for ( GEO::CUT::plain_facet_set::const_iterator i=holes.begin(); i!=holes.end(); ++i )
       {
       GEO::CUT::Facet & h = **i;
         stream << h;
       }
     }
   }
   stream << "}";
   return stream;
 }
