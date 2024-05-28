/*----------------------------------------------------------------------*/
/*! \file

\brief provides the basic functionality for cutting a mesh with a level set function


\level 2
 *------------------------------------------------------------------------------------------------*/
#include "4C_cut_levelsetintersection.hpp"

#include "4C_cut_levelsetside.hpp"
#include "4C_cut_side.hpp"

#include <Teuchos_TimeMonitor.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
CORE::GEO::CUT::LevelSetIntersection::LevelSetIntersection(
    const Epetra_Comm& comm, bool create_side)
    : ParentIntersection(comm.MyPID()), side_(Teuchos::null), comm_(&comm)
{
  if (create_side) AddCutSide(1);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
CORE::GEO::CUT::LevelSetIntersection::LevelSetIntersection(int myrank, bool create_side)
    : ParentIntersection(myrank), side_(Teuchos::null), comm_(nullptr)
{
  if (create_side) AddCutSide(1);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void CORE::GEO::CUT::LevelSetIntersection::AddCutSide(int levelset_sid)
{
  if (!side_.is_null()) FOUR_C_THROW("currently only one levelset-side is supported");

  // create the levelset-side
  side_ = Teuchos::rcp(Side::CreateLevelSetSide(levelset_sid));
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
CORE::GEO::CUT::ElementHandle* CORE::GEO::CUT::LevelSetIntersection::add_element(int eid,
    const std::vector<int>& nids, const CORE::LINALG::SerialDenseMatrix& xyz,
    CORE::FE::CellType distype, const double* lsv, const bool lsv_only_plus_domain,
    const bool& check_lsv)
{
  int numnode = nids.size();
  if (numnode != xyz.numCols()) FOUR_C_THROW("node coordinate number mismatch");

  bool ltz = false;
  bool gtz = false;

  if (check_lsv)
  {
    // make sure element has LSV +ve and -ve in one of its nodes
    // ensures this is a cut element
    for (int i = 0; i < numnode; ++i)
    {
      if (lsv[i] <= REFERENCETOL) ltz = true;
      if (lsv[i] >= -REFERENCETOL) gtz = true;
    }
  }

  /* add all cut elements (different signs of levelset values) OR
   * if only plus domain is a physical field we have to add also
   * elements with only negative values (as they are not allowed to
   * carry DOFS at the end) */
  if ((not check_lsv) or (ltz and gtz) or (lsv_only_plus_domain and ltz))
  {
    // add all nodes to mesh
    for (int i = 0; i < numnode; ++i)
    {
      NormalMesh().GetNode(nids[i], &xyz(0, i), lsv[i]);
    }

    // create element
    return mesh_.create_element(eid, nids, distype);
  }

  return nullptr;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void CORE::GEO::CUT::LevelSetIntersection::Cut_Mesh(bool screenoutput)
{
  TEUCHOS_FUNC_TIME_MONITOR("CORE::GEO::CUT --- 1/3 --- Cut");

  Mesh& m = NormalMesh();

  double t_diff = 0.0;
  {
    const double t_start = Teuchos::Time::wallTime();
    if (myrank_ == 0 and screenoutput)
      std::cout << "\n\t * 1/6 Cut ......................" << std::flush;

    m.Cut(*side_);

    if (myrank_ == 0 and screenoutput)
    {
      if (comm_) comm_->Barrier();
      t_diff = Teuchos::Time::wallTime() - t_start;

      printf("success! ( %10.4e secs )", t_diff);
    }
  }

  {
    const double t_start = Teuchos::Time::wallTime();
    if (myrank_ == 0 and screenoutput)
      std::cout << "\n\t * 2/6 MakeCutLines ............." << std::flush;

    m.MakeCutLines();

    if (myrank_ == 0 and screenoutput)
    {
      if (comm_) comm_->Barrier();
      t_diff = Teuchos::Time::wallTime() - t_start;

      printf("success! ( %10.4e secs )", t_diff);
    }
  }

  {
    const double t_start = Teuchos::Time::wallTime();
    if (myrank_ == 0 and screenoutput)
      std::cout << "\n\t * 3/6 MakeFacets ..............." << std::flush;

    m.MakeFacets();

    if (myrank_ == 0 and screenoutput)
    {
      if (comm_) comm_->Barrier();
      t_diff = Teuchos::Time::wallTime() - t_start;

      printf("success! ( %10.4e secs )", t_diff);
    }
  }

  {
    const double t_start = Teuchos::Time::wallTime();
    if (myrank_ == 0 and screenoutput)
      std::cout << "\n\t * 4/6 MakeVolumeCells .........." << std::flush;

    m.MakeVolumeCells();

    if (myrank_ == 0 and screenoutput)
    {
      if (comm_) comm_->Barrier();
      t_diff = Teuchos::Time::wallTime() - t_start;

      printf("success! ( %10.4e secs )", t_diff);
    }
  }
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void CORE::GEO::CUT::LevelSetIntersection::Cut(
    bool include_inner, bool screenoutput, INPAR::CUT::VCellGaussPts VCellGP)
{
  // ###########################################################################
  //  STEP 1/3 CUT THE MESH
  // ###########################################################################
  Cut_Mesh(screenoutput);

  // ###########################################################################
  //  STEP 2/3 ASSIGN DOFS
  // ###########################################################################

  Mesh& m = NormalMesh();

  if (options_.FindPositions())
  {
    m.FindLSNodePositions();

    //=====================================================================

    m.FindNodalDOFSets(include_inner);

    //=====================================================================
  }
  // #############################################################################
  //  STEP 3/3 FINALIZE, ASSIGN INTEGRATIONRULES
  // #############################################################################

  double t_diff = 0.0;
  {
    const double t_start = Teuchos::Time::wallTime();
    if (myrank_ == 0 and screenoutput)
      std::cout << "\n\t * 5/6 create_integration_cells ..." << std::flush;

    if (VCellGP == INPAR::CUT::VCellGaussPts_Tessellation)
      m.create_integration_cells(0);
    else
      m.direct_divergence_gauss_rule(true, INPAR::CUT::BCellGaussPts_Tessellation);

    if (myrank_ == 0 and screenoutput)
    {
      if (comm_) comm_->Barrier();
      t_diff = Teuchos::Time::wallTime() - t_start;

      printf("success! ( %10.4e secs )", t_diff);
    }
  }

  {
    const double t_start = Teuchos::Time::wallTime();
    if (myrank_ == 0 and screenoutput)
      std::cout << "\n\t * 6/6 TestElementVolume ........" << std::flush;

    m.TestElementVolume(true, VCellGP);

    if (myrank_ == 0 and screenoutput)
    {
      if (comm_) comm_->Barrier();
      t_diff = Teuchos::Time::wallTime() - t_start;

      printf("success! ( %10.4e secs )\n\n", t_diff);
    }
  }
  // ######################################################################################

}  // CORE::GEO::CUT::LevelSetIntersection::Cut

FOUR_C_NAMESPACE_CLOSE