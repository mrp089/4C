

#include <Teuchos_TimeMonitor.hpp>

#include "../drt_lib/drt_utils.H"
#include "../drt_lib/drt_globalproblem.H"

#include "../drt_geometry/integrationcell.H"
#include "../drt_geometry/geo_intersection.H"

#include "xfem_fluidwizard.H"
#include "xfem_fluiddofset.H"

#include "../drt_io/io_control.H"

/*-------------------------------------------------------------*
* The new cut algorithm used in xfluid and xfluidfluid         *
*--------------------------------------------------------------*/
void XFEM::FluidWizard::Cut(  bool include_inner,
                              const Epetra_Vector & idispcol,
                              bool parallel,
                              std::string VCellgausstype,     //Gauss point generation method for Volumecell
                              std::string BCellgausstype,     //Gauss point generation method for Boundarycell
                              bool positions )
{
#ifdef QHULL
  TEUCHOS_FUNC_TIME_MONITOR( "XFEM::FluidWizard::Cut" );

  if ( backdis_.Comm().MyPID() == 0 )
    std::cout << "\nXFEM::FluidWizard::Cut:" << std::flush;

  const double t_start = Teuchos::Time::wallTime();

  // set a new CutWizard based on the background discretization
  cut_ = Teuchos::rcp( new GEO::CutWizard( backdis_, false, 1 ) );
  cut_->SetFindPositions( positions );
  GEO::CutWizard & cw = *cut_;

  std::vector<int> lm;
  std::vector<double> mydisp;


  // fill the cutwizard cw with information:
  // build up the mesh_ (normal background mesh) and the cut_mesh_ (cutter mesh) created by the meshhandle:
  // DO NOT CHANGE THE ORDER of 1. and 2.
  // 1. Add CutSides (sides of the cutterdiscretization)
  //      -> Update the current position of all cutter-nodes dependent on displacement idispcol
  // 2. Add Elements (elements of the background discretization)

  // 1.
  int numcutelements = cutterdis_.NumMyColElements();
  for ( int lid = 0; lid < numcutelements; ++lid )
  {
    DRT::Element * element = cutterdis_.lColElement(lid);

    const int numnode = element->NumNode();
    DRT::Node ** nodes = element->Nodes();

    Epetra_SerialDenseMatrix xyze( 3, numnode );

    for ( int i=0; i < numnode; ++i )
    {
      DRT::Node & node = *nodes[i];

      lm.clear();
      mydisp.clear();
      cutterdis_.Dof(&node, lm);


      if(lm.size() == 3) // case for BELE3 boundary elements
      {
        DRT::UTILS::ExtractMyValues(idispcol,mydisp,lm);
      }
      else if(lm.size() == 4) // case for BELE3_4 boundary elements
      {
        // copy the first three entries for the displacement, the fourth entry should be zero if BELE3_4 is used for cutdis instead of BELE3
        std::vector<int> lm_red; // reduced local map
        lm_red.clear();
        for(int k=0; k< 3; k++) lm_red.push_back(lm[k]);

        DRT::UTILS::ExtractMyValues(idispcol,mydisp,lm_red);
      }
      else dserror("wrong number of dofs for node %i", lm.size());

      if (mydisp.size() != 3)
        dserror("we need 3 displacements here");


      LINALG::Matrix<3, 1> disp( &mydisp[0], true ); //cout << "disp " << disp << endl;
      LINALG::Matrix<3, 1> x( node.X() ); //cout << "x " << x << endl;

      // update x-position of cutter node for current time step (update with displacement)
      x.Update( 1, disp, 1 );

      std::copy( x.A(), x.A()+3, &xyze( 0, i ) );
    }

    // add the side of the cutter-discretization to the FluidWizard
    cw.AddCutSide( 0, element, xyze );
  }

  // 2. add background elements dependent on bounding box created by the CutSides in 1.
  int numbackelements = backdis_.NumMyColElements();
  for ( int k = 0; k < numbackelements; ++k )
  {
    DRT::Element * element = backdis_.lColElement( k );
    cw.AddElement( element );
  }

  // run the (parallel) Cut
  if(parallel)
  {
    cw.CutParallel( include_inner, VCellgausstype, BCellgausstype );
  }
  else
  {
    dserror("the non-parallel cutwizard does not support the DofsetNEW framework");
//    cw.Cut( include_inner, VCellgausstype, BCellgausstype );
  }

  // cleanup

  const double t_end = Teuchos::Time::wallTime()-t_start;
  if ( backdis_.Comm().MyPID() == 0 )
  {
    std::cout << "\n XFEM::FluidWizard::Cut: Success (" << t_end  <<  " secs)\n";
  }

  cw.DumpGmshNumDOFSets(include_inner);


  cw.PrintCellStats();
  cw.DumpGmshIntegrationCells();
  cw.DumpGmshVolumeCells( include_inner );

#else
  dserror( "QHULL needs to be defined to cut elements" );
#endif
}


Teuchos::RCP<XFEM::FluidDofSet> XFEM::FluidWizard::DofSet(int maxNumMyReservedDofs)
{
  return Teuchos::rcp( new FluidDofSet( this , maxNumMyReservedDofs, backdis_ ) );
}


GEO::CutWizard & XFEM::FluidWizard::CutWizard()
{
  return *cut_;
}


GEO::CUT::ElementHandle * XFEM::FluidWizard::GetElement(
    DRT::Element * ele
)
{
  return cut_->GetElement( ele );
}


GEO::CUT::Node * XFEM::FluidWizard::GetNode(
    int nid
)
{
  return cut_->GetNode( nid );
}


void XFEM::FluidWizard::DumpGmshIntegrationCells( std::map< int, GEO::DomainIntCells > & domainintcells,
                                                  std::map< int, GEO::BoundaryIntCells > & boundaryintcells )
{
  std::string name = DRT::Problem::Instance()->OutputControlFile()->FileName();
  std::stringstream str;
  str << name
      << ".cells."
      << backdis_.Comm().MyPID()
      << ".pos";

  std::ofstream file( str.str().c_str() );

  file << "View \"IntegrationCells\" {\n";
  for ( std::map< int, GEO::DomainIntCells >::iterator i=domainintcells.begin();
        i!=domainintcells.end();
        ++i )
  {
    GEO::DomainIntCells & cells = i->second;
    for ( GEO::DomainIntCells::iterator i=cells.begin(); i!=cells.end(); ++i )
    {
      GEO::DomainIntCell & cell = *i;
      const LINALG::SerialDenseMatrix & xyz = cell.CellNodalPosXYZ();
      switch ( cell.Shape() )
      {
      case DRT::Element::hex8:
        file << "SH(";
        break;
      case DRT::Element::tet4:
        file << "SS(";
        break;
      case DRT::Element::wedge6:
        file << "SI(";
        break;
      case DRT::Element::pyramid5:
        file << "SP(";
        break;
      default:
        dserror( "distype unsupported" );
      }
      for ( int i=0; i<xyz.N(); ++i )
      {
        if ( i > 0 )
          file << ",";
        file << xyz( 0, i ) << "," << xyz( 1, i ) << "," << xyz( 2, i );
      }
      file << "){";
      for ( int i=0; i<xyz.N(); ++i )
      {
        if ( i > 0 )
          file << ",";
        file << cell.VolumeInPhysicalDomain();
      }
      file << "};\n";
    }
  }
  file << "};\n";

  file << "View \"BoundaryCells\" {\n";
  for ( std::map< int, GEO::BoundaryIntCells >::iterator i=boundaryintcells.begin();
        i!=boundaryintcells.end();
        ++i )
  {
    GEO::BoundaryIntCells & cells = i->second;
    for ( GEO::BoundaryIntCells::iterator i=cells.begin(); i!=cells.end(); ++i )
    {
      GEO::BoundaryIntCell & cell = *i;
      const LINALG::SerialDenseMatrix & xyz = cell.CellNodalPosXYZ();
      switch ( cell.Shape() )
      {
      case DRT::Element::tri3:
        file << "ST(";
        break;
      case DRT::Element::quad4:
        file << "SQ(";
        break;
      default:
        dserror( "distype unsupported" );
      }
      for ( int i=0; i<xyz.N(); ++i )
      {
        if ( i > 0 )
          file << ",";
        file << xyz( 0, i ) << "," << xyz( 1, i ) << "," << xyz( 2, i );
      }
      file << "){";
      for ( int i=0; i<xyz.N(); ++i )
      {
        if ( i > 0 )
          file << ",";
        file << cell.GetSurfaceEleGid();
      }
      file << "};\n";
    }
  }
  file << "};\n";
}
