/*----------------------------------------------------------------------*/
/*! \file
\brief Mesh tying strategy for monolithic SSI

\level 2

 */
/*----------------------------------------------------------------------*/

#include "ssi_monolithic_meshtying_strategy.H"

#include "ssi_monolithic.H"
#include "ssi_utils.H"
#include "Epetra_Map.h"

#include "../drt_adapter/ad_str_ssiwrapper.H"
#include "../drt_adapter/adapter_coupling.H"

#include "../drt_lib/drt_locsys.H"

#include "../drt_scatra/scatra_timint_meshtying_strategy_s2i.H"

#include "../linalg/linalg_blocksparsematrix.H"
#include "../linalg/linalg_matrixtransform.H"
#include "../linalg/linalg_utils_sparse_algebra_assemble.H"

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
SSI::MeshtyingStrategyBase::MeshtyingStrategyBase(const bool is_scatra_manifold,
    const bool meshtying_3_domain_intersection, Teuchos::RCP<SSI::UTILS::SSIMaps> ssi_maps,
    Teuchos::RCP<const SSI::UTILS::SSIStructureMeshTying> ssi_structure_meshtying)
    : temp_scatra_struct_mat_(Teuchos::null),
      temp_scatramanifold_struct_mat_(Teuchos::null),
      temp_struct_scatra_mat_(Teuchos::null),
      is_scatra_manifold_(is_scatra_manifold),
      meshtying_3_domain_intersection_(meshtying_3_domain_intersection),
      ssi_maps_(ssi_maps),
      ssi_structure_meshtying_(std::move(ssi_structure_meshtying))
{
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
SSI::MeshtyingStrategySparse::MeshtyingStrategySparse(const bool is_scatra_manifold,
    const bool meshtying_3_domain_intersection, Teuchos::RCP<SSI::UTILS::SSIMaps> ssi_maps,
    Teuchos::RCP<const SSI::UTILS::SSIStructureMeshTying> ssi_structure_meshtying)
    : MeshtyingStrategyBase(
          is_scatra_manifold, meshtying_3_domain_intersection, ssi_maps, ssi_structure_meshtying)
{
  temp_scatra_struct_mat_ =
      SSI::UTILS::SSIMatrices::SetupSparseMatrix(SSIMaps()->ScaTraDofRowMap());

  if (IsScaTraManifold())
  {
    temp_scatramanifold_struct_mat_ =
        SSI::UTILS::SSIMatrices::SetupSparseMatrix(SSIMaps()->ScaTraManifoldDofRowMap());
  }

  temp_struct_scatra_mat_ =
      SSI::UTILS::SSIMatrices::SetupSparseMatrix(SSIMaps()->StructureDofRowMap());
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
SSI::MeshtyingStrategyBlock::MeshtyingStrategyBlock(const bool is_scatra_manifold,
    const bool meshtying_3_domain_intersection, Teuchos::RCP<SSI::UTILS::SSIMaps> ssi_maps,
    Teuchos::RCP<const SSI::UTILS::SSIStructureMeshTying> ssi_structure_meshtying)
    : MeshtyingStrategyBase(
          is_scatra_manifold, meshtying_3_domain_intersection, ssi_maps, ssi_structure_meshtying),
      block_position_scatra_(Teuchos::null),
      block_position_scatra_manifold_(Teuchos::null),
      position_structure_(-1)
{
  temp_scatra_struct_mat_ = SSI::UTILS::SSIMatrices::SetupBlockMatrix(
      SSIMaps()->BlockMapScaTra(), SSIMaps()->BlockMapStructure());

  if (IsScaTraManifold())
  {
    temp_scatramanifold_struct_mat_ = SSI::UTILS::SSIMatrices::SetupBlockMatrix(
        SSIMaps()->BlockMapScaTraManifold(), SSIMaps()->BlockMapStructure());
  }

  temp_struct_scatra_mat_ = SSI::UTILS::SSIMatrices::SetupBlockMatrix(
      SSIMaps()->BlockMapStructure(), SSIMaps()->BlockMapScaTra());

  block_position_scatra_ = SSIMaps()->GetBlockPositions(SSI::Subproblem::scalar_transport);
  position_structure_ = SSIMaps()->GetBlockPositions(SSI::Subproblem::structure)->at(0);
  if (IsScaTraManifold())
    block_position_scatra_manifold_ = SSIMaps()->GetBlockPositions(SSI::Subproblem::manifold);

  // safety checks
  if (block_position_scatra_ == Teuchos::null) dserror("Cannot get position of scatra blocks");
  if (position_structure_ == -1) dserror("Cannot get position of structure block");
  if (IsScaTraManifold() and block_position_scatra_manifold_ == Teuchos::null)
    dserror("Cannot get position of scatra manifold blocks");
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void SSI::MeshtyingStrategyBase::ApplyMeshtyingToStructureMatrix(
    LINALG::SparseMatrix& ssi_structure_matrix,
    Teuchos::RCP<const LINALG::SparseMatrix> structure_matrix)
{
  /* Transform and assemble the structure matrix into the ssi structure matrix block by block:
   * S_i:  structure interior dofs
   * S_m:  structure master side dofs
   * S_ss: structure slave surface dofs
   * S_sl: structure slave line dofs
   *
   *      | S_i | S_m | S_ss| S_sl|
   *      |-----|-----|-----|-----|
   * S_i  |  a  |  b  |  c  |  d  |
   * S_m  |  e  |  f  |  g  |  -  |
   * S_ss |  h  |  i  |  j  |  k  |
   * S_sl |  l  |  -  |  m  |  n  |
   *      -------------------------
   */
  // assemble derivatives of interior dofs w.r.t. interior dofs (block a)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureInterior(),
      *MapStructureInterior(), 1.0, nullptr, nullptr, ssi_structure_matrix, true, true);

  // assemble derivatives of interior dofs w.r.t. master dofs (block b)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureInterior(),
      *MapStructureMaster(), 1.0, nullptr, nullptr, ssi_structure_matrix, true, true);

  // assemble derivatives of master dofs w.r.t. interior dofs (block e)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureMaster(),
      *MapStructureInterior(), 1.0, nullptr, nullptr, ssi_structure_matrix, true, true);

  // assemble derivatives of master dofs w.r.t. master dofs (block f)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureMaster(),
      *MapStructureMaster(), 1.0, nullptr, nullptr, ssi_structure_matrix, true, true);

  // assemble derivatives of surface slave dofs w.r.t. interior dofs (block h)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureSlave(),
      *MapStructureInterior(), 1.0, &StructureSlaveConverter(), nullptr, ssi_structure_matrix, true,
      true);

  // assemble derivatives of surface slave dofs w.r.t. master dofs (block i)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureSlave(),
      *MapStructureMaster(), 1.0, &StructureSlaveConverter(), nullptr, ssi_structure_matrix, true,
      true);

  // assemble derivatives of interior w.r.t. surface slave dofs (block c)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureInterior(),
      *MapStructureSlave(), 1.0, nullptr, &StructureSlaveConverter(), ssi_structure_matrix, true,
      true);

  // assemble derivatives of master w.r.t. surface slave dofs (block g)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureMaster(),
      *MapStructureSlave(), 1.0, nullptr, &StructureSlaveConverter(), ssi_structure_matrix, true,
      true);

  // assemble derivatives of surface slave dofs w.r.t. surface slave dofs (block j)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureSlave(),
      *MapStructureSlave(), 1.0, &StructureSlaveConverter(), &StructureSlaveConverter(),
      ssi_structure_matrix, true, true);

  if (Meshtying3DomainIntersection())
  {
    // assemble derivatives of line slave dofs w.r.t. interior dofs (block l)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix,
        *MapStructureSlave3DomainIntersection(), *MapStructureInterior(), 1.0,
        &StructureSlaveConverter3DomainIntersection(), nullptr, ssi_structure_matrix, true, true);

    // assemble derivatives of interior dofs w.r.t. line slave dofs (block d)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureInterior(),
        *MapStructureSlave3DomainIntersection(), 1.0, nullptr,
        &StructureSlaveConverter3DomainIntersection(), ssi_structure_matrix, true, true);

    // assemble derivatives of line slave dofs w.r.t. line slave dofs (block n)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix,
        *MapStructureSlave3DomainIntersection(), *MapStructureSlave3DomainIntersection(), 1.0,
        &StructureSlaveConverter3DomainIntersection(),
        &StructureSlaveConverter3DomainIntersection(), ssi_structure_matrix, true, true);

    // assemble derivatives of surface slave dofs w.r.t. line slave dofs (block k)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureSlave(),
        *MapStructureSlave3DomainIntersection(), 1.0, &StructureSlaveConverter(),
        &StructureSlaveConverter3DomainIntersection(), ssi_structure_matrix, true, true);

    // assemble derivatives of line slave dofs w.r.t. surface slave dofs (block m)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix,
        *MapStructureSlave3DomainIntersection(), *MapStructureSlave(), 1.0,
        &StructureSlaveConverter3DomainIntersection(), &StructureSlaveConverter(),
        ssi_structure_matrix, true, true);
  }

  FinalizeMeshtyingStructureMatrix(ssi_structure_matrix);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void SSI::MeshtyingStrategyBase::ApplyMeshtyingToXXXStructure(
    LINALG::SparseMatrix& ssi_xxx_structure_matrix,
    const LINALG::SparseMatrix& xxx_structure_matrix)
{
  // uncomplete matrix first
  ssi_xxx_structure_matrix.UnComplete();

  // assemble derivatives of x w.r.t. structure interior dofs
  LINALG::MatrixLogicalSplitAndTransform()(xxx_structure_matrix, xxx_structure_matrix.RangeMap(),
      *MapStructureInterior(), 1.0, nullptr, nullptr, ssi_xxx_structure_matrix, true, true);

  // assemble derivatives of x w.r.t. structure master dofs
  LINALG::MatrixLogicalSplitAndTransform()(xxx_structure_matrix, xxx_structure_matrix.RangeMap(),
      *MapStructureMaster(), 1.0, nullptr, nullptr, ssi_xxx_structure_matrix, true, true);

  // assemble derivatives of x w.r.t. structure surface slave dofs
  LINALG::MatrixLogicalSplitAndTransform()(xxx_structure_matrix, xxx_structure_matrix.RangeMap(),
      *MapStructureSlave(), 1.0, nullptr, &StructureSlaveConverter(), ssi_xxx_structure_matrix,
      true, true);

  if (Meshtying3DomainIntersection())
  {
    // assemble derivatives of x w.r.t. structure line slave dofs
    LINALG::MatrixLogicalSplitAndTransform()(xxx_structure_matrix, xxx_structure_matrix.RangeMap(),
        *MapStructureSlave3DomainIntersection(), 1.0, nullptr,
        &StructureSlaveConverter3DomainIntersection(), ssi_xxx_structure_matrix, true, true);
  }
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Epetra_Vector SSI::MeshtyingStrategyBase::ApplyMeshtyingToStructureRHS(
    Teuchos::RCP<const Epetra_Vector> structure_rhs)
{
  // make copy of structure right-hand side vector
  Epetra_Vector rhs_structure(*structure_rhs);

  // transform slave-side part of structure right-hand side vector to master side
  const auto rhs_structure_only_slave_dofs = MapsCoupStruct()->ExtractVector(rhs_structure, 1);

  const auto rhs_structure_only_master_dofs =
      SSIStructureMeshtying()->InterfaceCouplingAdapterStructure()->SlaveToMaster(
          rhs_structure_only_slave_dofs);

  auto rhs_structure_master = MapsCoupStruct()->InsertVector(rhs_structure_only_master_dofs, 2);

  if (Meshtying3DomainIntersection())
  {
    const auto rhs_structure_3_domain_intersection_only_slave_dofs =
        MapsCoupStruct3DomainIntersection()->ExtractVector(rhs_structure, 1);

    const auto rhs_structure_3_domain_intersection_only_master_dofs =
        SSIStructureMeshtying()
            ->InterfaceCouplingAdapterStructure3DomainIntersection()
            ->SlaveToMaster(rhs_structure_3_domain_intersection_only_slave_dofs);

    const auto rhs_structure_3_domain_intersection_master =
        MapsCoupStruct3DomainIntersection()->InsertVector(
            rhs_structure_3_domain_intersection_only_master_dofs, 2);

    rhs_structure_master->Update(1.0, *rhs_structure_3_domain_intersection_master, 1.0);
  }

  // assemble master-side part of structure right-hand side vector
  rhs_structure.Update(1.0, *rhs_structure_master, 1.0);

  // zero out slave-side part of structure right-hand side vector
  MapsCoupStruct()->PutScalar(rhs_structure, 1, 0.0);
  if (Meshtying3DomainIntersection())
    MapsCoupStruct3DomainIntersection()->PutScalar(rhs_structure, 1, 0.0);

  return rhs_structure;
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
void SSI::MeshtyingStrategySparse::ApplyMeshtyingToScatraManifoldStructure(
    Teuchos::RCP<LINALG::SparseOperator> manifold_structure_matrix, const bool do_uncomplete)
{
  temp_scatramanifold_struct_mat_->Zero();
  auto temp_scatramanifold_struct_sparse_matrix =
      LINALG::CastToSparseMatrixAndCheckSuccess(temp_scatramanifold_struct_mat_);
  auto manifold_structure_sparse_matrix =
      LINALG::CastToSparseMatrixAndCheckSuccess(manifold_structure_matrix);

  // apply mesh tying contributions to temp matrix and complete it
  ApplyMeshtyingToXXXStructure(
      *temp_scatramanifold_struct_sparse_matrix, *manifold_structure_sparse_matrix);
  temp_scatramanifold_struct_sparse_matrix->Complete(
      *SSIMaps()->StructureDofRowMap(), *SSIMaps()->ScaTraManifoldDofRowMap());

  if (do_uncomplete) manifold_structure_sparse_matrix->UnComplete();
  manifold_structure_sparse_matrix->Add(*temp_scatramanifold_struct_sparse_matrix, false, 1.0, 0.0);
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
void SSI::MeshtyingStrategyBlock::ApplyMeshtyingToScatraManifoldStructure(
    Teuchos::RCP<LINALG::SparseOperator> manifold_structure_matrix, const bool do_uncomplete)
{
  temp_scatramanifold_struct_mat_->Zero();
  auto temp_scatramanifold_struct_block_matrix =
      LINALG::CastToBlockSparseMatrixBaseAndCheckSuccess(temp_scatramanifold_struct_mat_);
  auto manifold_structure_matrix_block_matrix =
      LINALG::CastToBlockSparseMatrixBaseAndCheckSuccess(manifold_structure_matrix);

  // apply mesh tying contributions to temp matrix blocks and complete the resulting matrix
  for (int iblock = 0; iblock < static_cast<int>(BlockPositionScaTraManifold()->size()); ++iblock)
  {
    ApplyMeshtyingToXXXStructure(temp_scatramanifold_struct_block_matrix->Matrix(iblock, 0),
        manifold_structure_matrix_block_matrix->Matrix(iblock, 0));
  }
  temp_scatramanifold_struct_block_matrix->Complete();

  if (do_uncomplete) manifold_structure_matrix_block_matrix->UnComplete();
  manifold_structure_matrix_block_matrix->Add(
      *temp_scatramanifold_struct_block_matrix, false, 1.0, 0.0);
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
void SSI::MeshtyingStrategySparse::ApplyMeshtyingToScatraStructure(
    Teuchos::RCP<LINALG::SparseOperator> scatra_structure_matrix, const bool do_uncomplete)
{
  temp_scatra_struct_mat_->Zero();
  auto temp_scatra_struct_domain_sparse_matrix =
      LINALG::CastToSparseMatrixAndCheckSuccess(temp_scatra_struct_mat_);
  auto scatra_structure_matrix_sparse =
      LINALG::CastToSparseMatrixAndCheckSuccess(scatra_structure_matrix);

  ApplyMeshtyingToXXXStructure(
      *temp_scatra_struct_domain_sparse_matrix, *scatra_structure_matrix_sparse);
  temp_scatra_struct_domain_sparse_matrix->Complete(
      *SSIMaps()->StructureDofRowMap(), *SSIMaps()->ScaTraDofRowMap());

  if (do_uncomplete) scatra_structure_matrix_sparse->UnComplete();
  scatra_structure_matrix_sparse->Add(*temp_scatra_struct_domain_sparse_matrix, false, 1.0, 0.0);
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
void SSI::MeshtyingStrategyBlock::ApplyMeshtyingToScatraStructure(
    Teuchos::RCP<LINALG::SparseOperator> scatra_structure_matrix, const bool do_uncomplete)
{
  temp_scatra_struct_mat_->Zero();
  auto temp_scatra_struct_domain_block_sparse_matrix =
      LINALG::CastToBlockSparseMatrixBaseAndCheckSuccess(temp_scatra_struct_mat_);
  auto scatra_structure_matrix_block_sparse =
      LINALG::CastToBlockSparseMatrixBaseAndCheckSuccess(scatra_structure_matrix);

  // apply mesh tying for all blocks
  for (int iblock = 0; iblock < static_cast<int>(BlockPositionScaTra()->size()); ++iblock)
  {
    ApplyMeshtyingToXXXStructure(temp_scatra_struct_domain_block_sparse_matrix->Matrix(iblock, 0),
        scatra_structure_matrix_block_sparse->Matrix(iblock, 0));
  }
  temp_scatra_struct_domain_block_sparse_matrix->Complete();

  if (do_uncomplete) scatra_structure_matrix_block_sparse->UnComplete();
  scatra_structure_matrix_block_sparse->Add(
      *temp_scatra_struct_domain_block_sparse_matrix, false, 1.0, 0.0);
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
void SSI::MeshtyingStrategySparse::ApplyMeshtyingToStructureScatra(
    Teuchos::RCP<LINALG::SparseOperator> structure_scatra_matrix, const bool do_uncomplete)
{
  temp_struct_scatra_mat_->Zero();
  auto temp_struct_scatra_sparse_matrix =
      LINALG::CastToSparseMatrixAndCheckSuccess(temp_struct_scatra_mat_);
  auto struct_scatra_sparse_matrix =
      LINALG::CastToSparseMatrixAndCheckSuccess(structure_scatra_matrix);

  // apply mesh tying contributions to temp matrix and complete it
  ApplyMeshtyingToStructureXXX(*temp_struct_scatra_sparse_matrix, *struct_scatra_sparse_matrix);
  temp_struct_scatra_sparse_matrix->Complete(
      *SSIMaps()->ScaTraDofRowMap(), *SSIMaps()->StructureDofRowMap());

  if (do_uncomplete) struct_scatra_sparse_matrix->UnComplete();
  struct_scatra_sparse_matrix->Add(*temp_struct_scatra_sparse_matrix, false, 1.0, 0.0);
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
void SSI::MeshtyingStrategyBlock::ApplyMeshtyingToStructureScatra(
    Teuchos::RCP<LINALG::SparseOperator> structure_scatra_matrix, const bool do_uncomplete)
{
  temp_struct_scatra_mat_->Zero();
  auto temp_struct_scatra_block_matrix =
      LINALG::CastToBlockSparseMatrixBaseAndCheckSuccess(temp_struct_scatra_mat_);
  auto structure_scatra_block_matrix =
      LINALG::CastToBlockSparseMatrixBaseAndCheckSuccess(structure_scatra_matrix);

  // apply mesh tying contributions to temp matrix blocks and complete the resulting matrix
  for (int iblock = 0; iblock < static_cast<int>(BlockPositionScaTra()->size()); ++iblock)
  {
    ApplyMeshtyingToStructureXXX(temp_struct_scatra_block_matrix->Matrix(0, iblock),
        structure_scatra_block_matrix->Matrix(0, iblock));
  }
  temp_struct_scatra_block_matrix->Complete();

  if (do_uncomplete) structure_scatra_block_matrix->UnComplete();
  structure_scatra_block_matrix->Add(*temp_struct_scatra_block_matrix, false, 1.0, 0.0);
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void SSI::MeshtyingStrategyBase::ApplyMeshtyingToStructureXXX(
    LINALG::SparseMatrix& ssi_structure_xxx_matrix,
    const LINALG::SparseMatrix& structure_xxx_matrix)
{
  // assemble derivatives of structure interior dofs w.r.t. scatra dofs
  LINALG::MatrixLogicalSplitAndTransform()(structure_xxx_matrix, *MapStructureInterior(),
      structure_xxx_matrix.DomainMap(), 1.0, nullptr, nullptr, ssi_structure_xxx_matrix, true,
      true);

  // assemble derivatives of structure master dofs w.r.t. scatra dofs
  LINALG::MatrixLogicalSplitAndTransform()(structure_xxx_matrix, *MapStructureMaster(),
      structure_xxx_matrix.DomainMap(), 1.0, nullptr, nullptr, ssi_structure_xxx_matrix, true,
      true);

  // assemble derivatives of structure surface slave dofs & interior dofs w.r.t. scatra dofs
  LINALG::MatrixLogicalSplitAndTransform()(structure_xxx_matrix, *MapStructureSlave(),
      structure_xxx_matrix.DomainMap(), 1.0, &StructureSlaveConverter(), nullptr,
      ssi_structure_xxx_matrix, true, true);

  if (Meshtying3DomainIntersection())
  {
    // assemble derivatives of structure surface line dofs & interior dofs w.r.t. scatra dofs
    LINALG::MatrixLogicalSplitAndTransform()(structure_xxx_matrix,
        *MapStructureSlave3DomainIntersection(), structure_xxx_matrix.DomainMap(), 1.0,
        &StructureSlaveConverter3DomainIntersection(), nullptr, ssi_structure_xxx_matrix, true,
        true);
  }
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
void SSI::MeshtyingStrategyBase::FinalizeMeshtyingStructureMatrix(
    LINALG::SparseMatrix& ssi_structure_matrix)
{
  // map for slave side structure degrees of freedom
  Teuchos::RCP<const Epetra_Map> slavemaps = Teuchos::null;
  if (Meshtying3DomainIntersection())
  {
    slavemaps = LINALG::MultiMapExtractor::MergeMaps(
        {MapStructureSlave(), MapStructureSlave3DomainIntersection()});
  }
  else
    slavemaps = MapStructureSlave();

  // subject slave-side rows of structure system matrix to pseudo Dirichlet conditions to finalize
  // structure mesh tying
  const double one(1.0);
  for (int doflid_slave = 0; doflid_slave < slavemaps->NumMyElements(); ++doflid_slave)
  {
    // extract global ID of current slave-side row
    const int dofgid_slave = slavemaps->GID(doflid_slave);
    if (dofgid_slave < 0) dserror("Local ID not found!");

    // apply pseudo Dirichlet conditions to filled matrix, i.e., to local row and column indices
    if (ssi_structure_matrix.Filled())
    {
      const int rowlid_slave = ssi_structure_matrix.RowMap().LID(dofgid_slave);
      if (rowlid_slave < 0) dserror("Global ID not found!");
      if (ssi_structure_matrix.EpetraMatrix()->ReplaceMyValues(
              rowlid_slave, 1, &one, &rowlid_slave))
        dserror("ReplaceMyValues failed!");
    }

    // apply pseudo Dirichlet conditions to unfilled matrix, i.e., to global row and column indices
    else if (ssi_structure_matrix.EpetraMatrix()->InsertGlobalValues(
                 dofgid_slave, 1, &one, &dofgid_slave))
      dserror("InsertGlobalValues failed!");
  }
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Teuchos::RCP<SSI::MeshtyingStrategyBase> SSI::BuildMeshtyingStrategy(const bool is_scatra_manifold,
    const LINALG::MatrixType matrixtype_scatra, const bool meshtying_3_domain_intersection,
    Teuchos::RCP<SSI::UTILS::SSIMaps> ssi_maps,
    Teuchos::RCP<const SSI::UTILS::SSIStructureMeshTying> ssi_structure_meshtying)
{
  Teuchos::RCP<SSI::MeshtyingStrategyBase> meshtying_strategy = Teuchos::null;

  switch (matrixtype_scatra)
  {
    case LINALG::MatrixType::block_condition:
    case LINALG::MatrixType::block_condition_dof:
    {
      meshtying_strategy = Teuchos::rcp(new SSI::MeshtyingStrategyBlock(
          is_scatra_manifold, meshtying_3_domain_intersection, ssi_maps, ssi_structure_meshtying));
      break;
    }
    case LINALG::MatrixType::sparse:
    {
      meshtying_strategy = Teuchos::rcp(new SSI::MeshtyingStrategySparse(
          is_scatra_manifold, meshtying_3_domain_intersection, ssi_maps, ssi_structure_meshtying));
      break;
    }

    default:
    {
      dserror("unknown matrix type of ScaTra field");
      break;
    }
  }

  return meshtying_strategy;
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
ADAPTER::CouplingSlaveConverter& SSI::MeshtyingStrategyBase::StructureSlaveConverter() const
{
  return SSIStructureMeshtying()
      ->SlaveSideConverter()
      ->InterfaceCouplingAdapterStructureSlaveConverter();
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
ADAPTER::CouplingSlaveConverter&
SSI::MeshtyingStrategyBase::StructureSlaveConverter3DomainIntersection() const
{
  return SSIStructureMeshtying()
      ->SlaveSideConverter()
      ->InterfaceCouplingAdapterStructureSlaveConverter3DomainIntersection();
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Teuchos::RCP<const LINALG::MultiMapExtractor> SSI::MeshtyingStrategyBase::MapsCoupStruct() const
{
  return SSIStructureMeshtying()->SSIMeshTyingMaps()->MapsCoupStruct();
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Map> SSI::MeshtyingStrategyBase::MapStructureInterior() const
{
  return SSIStructureMeshtying()->SSIMeshTyingMaps()->MapStructureInterior();
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Map> SSI::MeshtyingStrategyBase::MapStructureMaster() const
{
  return SSIStructureMeshtying()->SSIMeshTyingMaps()->MapStructureMaster();
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Map> SSI::MeshtyingStrategyBase::MapStructureSlave() const
{
  return SSIStructureMeshtying()->SSIMeshTyingMaps()->MapStructureSlave();
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Teuchos::RCP<const LINALG::MultiMapExtractor>
SSI::MeshtyingStrategyBase::MapsCoupStruct3DomainIntersection() const
{
  return SSIStructureMeshtying()->SSIMeshTyingMaps()->MapsCoupStruct3DomainIntersection();
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Teuchos::RCP<const Epetra_Map> SSI::MeshtyingStrategyBase::MapStructureSlave3DomainIntersection()
    const
{
  return SSIStructureMeshtying()->SSIMeshTyingMaps()->MapStructureSlave3DomainIntersection();
}