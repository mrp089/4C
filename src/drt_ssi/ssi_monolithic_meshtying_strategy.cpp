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

#include "../linalg/linalg_blocksparsematrix.H"
#include "../linalg/linalg_matrixtransform.H"
#include "../linalg/linalg_utils_sparse_algebra_assemble.H"

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
SSI::MeshtyingStrategyBase::MeshtyingStrategyBase(const SSI::SSIMono& ssi_mono)
    : mapstructurecondensed_(
          ssi_mono.SSIInterfaceMeshtying() ? ssi_mono.MapStructureCondensed() : Teuchos::null),
      mapstructureslave_(
          ssi_mono.SSIInterfaceMeshtying() ? ssi_mono.MapsCoupStruct()->Map(1) : Teuchos::null),
      mapstructureslave3domainintersection_(
          (ssi_mono.SSIInterfaceMeshtying() and ssi_mono.Meshtying3DomainIntersection())
              ? ssi_mono.MapsCoupStruct3DomainIntersection()->Map(1)
              : Teuchos::null),
      meshtying_3_domain_intersection_(
          ssi_mono.SSIInterfaceMeshtying() and ssi_mono.Meshtying3DomainIntersection()),
      slave_side_converter_(
          ssi_mono.SSIInterfaceMeshtying() ? ssi_mono.SlaveSideConverter() : Teuchos::null),
      ssi_mono_(ssi_mono)
{
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
SSI::MeshtyingStrategySparse::MeshtyingStrategySparse(const SSI::SSIMono& ssi_mono)
    : MeshtyingStrategyBase(ssi_mono)
{
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
SSI::MeshtyingStrategyBlock::MeshtyingStrategyBlock(const SSI::SSIMono& ssi_mono)
    : MeshtyingStrategyBase(ssi_mono), position_structure_(-1)
{
  position_structure_ = SSIMono().GetBlockPositions(SSI::Subproblem::structure)->at(0);

  // safety check
  if (position_structure_ == -1) dserror("Cannot get position of structure block");
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
SSI::MeshtyingStrategyBlockSparse::MeshtyingStrategyBlockSparse(const SSI::SSIMono& ssi_mono)
    : MeshtyingStrategyBlock(ssi_mono)
{
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
SSI::MeshtyingStrategyBlockBlock::MeshtyingStrategyBlockBlock(const SSI::SSIMono& ssi_mono)
    : MeshtyingStrategyBlock(ssi_mono)
{
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void SSI::MeshtyingStrategyBase::ApplyMeshtyingToStructureMatrix(
    LINALG::SparseMatrix& ssi_structure_matrix,
    Teuchos::RCP<const LINALG::SparseMatrix> structure_matrix)
{
  /* Transform and assemble the structure matrix into the ssi structure matrix block by block:
   * S_m: structure interior and master side dofs
   * S_ss: structure slave surface dofs
   * S_sl: structure slave line dofs
   *
   *       S_m  S_ss  S_sl
   *       --------------
   * S_m  |  a |  b |  c |
   * S_ss |  d |  e |  f |
   * S_sl |  g |  h |  i |
   *       --------------
   */
  // assemble derivatives of interior & master dofs w.r.t. interior & master dofs (block a)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureCondensed(),
      *MapStructureCondensed(), 1.0, nullptr, nullptr, ssi_structure_matrix, true, true);

  // assemble derivatives of surface slave dofs w.r.t. master & interior dofs (block d)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureSlave(),
      *MapStructureCondensed(), 1.0, &StructureSlaveConverter(), nullptr, ssi_structure_matrix,
      true, true);

  // assemble derivatives of master & interior w.r.t. surface slave dofs (block b)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureCondensed(),
      *MapStructureSlave(), 1.0, nullptr, &StructureSlaveConverter(), ssi_structure_matrix, true,
      true);

  // assemble derivatives of surface slave dofs w.r.t. surface slave dofs (block e)
  LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureSlave(),
      *MapStructureSlave(), 1.0, &StructureSlaveConverter(), &StructureSlaveConverter(),
      ssi_structure_matrix, true, true);

  if (Meshtying3DomainIntersection())
  {
    // assemble derivatives of line slave dofs w.r.t. master & interior (block g)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix,
        *MapStructureSlave3DomainIntersection(), *MapStructureCondensed(), 1.0,
        &StructureSlaveConverter3DomainIntersection(), nullptr, ssi_structure_matrix, true, true);

    // assemble derivatives of master & interior w.r.t. line slave dofs (block c)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureCondensed(),
        *MapStructureSlave3DomainIntersection(), 1.0, nullptr,
        &StructureSlaveConverter3DomainIntersection(), ssi_structure_matrix, true, true);

    // assemble derivatives of line slave dof w.r.t. line slave dofs (block i)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix,
        *MapStructureSlave3DomainIntersection(), *MapStructureSlave3DomainIntersection(), 1.0,
        &StructureSlaveConverter3DomainIntersection(),
        &StructureSlaveConverter3DomainIntersection(), ssi_structure_matrix, true, true);

    // assemble derivatives of surface slave dofs w.r.t. line slave dofs (block f)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix, *MapStructureSlave(),
        *MapStructureSlave3DomainIntersection(), 1.0, &StructureSlaveConverter(),
        &StructureSlaveConverter3DomainIntersection(), ssi_structure_matrix, true, true);

    // assemble derivatives of line slave dofs w.r.t. surface slave dofs (block h)
    LINALG::MatrixLogicalSplitAndTransform()(*structure_matrix,
        *MapStructureSlave3DomainIntersection(), *MapStructureSlave(), 1.0,
        &StructureSlaveConverter3DomainIntersection(), &StructureSlaveConverter(),
        ssi_structure_matrix, true, true);
  }

  FinalizeMeshtyingStructureMatrix(ssi_structure_matrix);
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
Epetra_Vector SSI::MeshtyingStrategyBase::ApplyMeshtyingToStructureRHS(
    Teuchos::RCP<const Epetra_Vector> structure_rhs)
{
  // make copy of structure right-hand side vector
  Epetra_Vector rhs_structure(*structure_rhs);

  // transform slave-side part of structure right-hand side vector to master side
  const auto rhs_structure_only_slave_dofs =
      SSIMono().MapsCoupStruct()->ExtractVector(rhs_structure, 1);

  const auto rhs_structure_only_master_dofs =
      SSIMono().InterfaceCouplingAdapterStructure()->SlaveToMaster(rhs_structure_only_slave_dofs);

  auto rhs_structure_master =
      SSIMono().MapsCoupStruct()->InsertVector(rhs_structure_only_master_dofs, 2);

  if (Meshtying3DomainIntersection())
  {
    const auto rhs_structure_3_domain_intersection_only_slave_dofs =
        SSIMono().MapsCoupStruct3DomainIntersection()->ExtractVector(rhs_structure, 1);

    const auto rhs_structure_3_domain_intersection_only_master_dofs =
        SSIMono().InterfaceCouplingAdapterStructure3DomainIntersection()->SlaveToMaster(
            rhs_structure_3_domain_intersection_only_slave_dofs);

    const auto rhs_structure_3_domain_intersection_master =
        SSIMono().MapsCoupStruct3DomainIntersection()->InsertVector(
            rhs_structure_3_domain_intersection_only_master_dofs, 2);

    rhs_structure_master->Update(1.0, *rhs_structure_3_domain_intersection_master, 1.0);
  }

  // locsys manager of structure
  const auto& locsysmanager_structure = SSIMono().StructureField()->LocsysManager();

  // apply pseudo Dirichlet conditions to master-side part of structure right-hand side vector
  const auto zeros_structure_master = Teuchos::rcp(new Epetra_Vector(rhs_structure_master->Map()));

  if (locsysmanager_structure != Teuchos::null)
    locsysmanager_structure->RotateGlobalToLocal(rhs_structure_master);

  LINALG::ApplyDirichlettoSystem(rhs_structure_master, zeros_structure_master,
      *SSIMono().StructureField()->GetDBCMapExtractor()->CondMap());

  if (locsysmanager_structure != Teuchos::null)
    locsysmanager_structure->RotateLocalToGlobal(rhs_structure_master);

  // assemble master-side part of structure right-hand side vector
  rhs_structure.Update(1.0, *rhs_structure_master, 1.0);

  // zero out slave-side part of structure right-hand side vector
  SSIMono().MapsCoupStruct()->PutScalar(rhs_structure, 1, 0.0);
  if (Meshtying3DomainIntersection())
    SSIMono().MapsCoupStruct3DomainIntersection()->PutScalar(rhs_structure, 1, 0.0);

  return rhs_structure;
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
    slavemaps = LINALG::MultiMapExtractor::MergeMaps({SSIMono().MapsCoupStruct()->Map(1),
        SSIMono().MapsCoupStruct3DomainIntersection()->Map(1)});
  }
  else
    slavemaps = SSIMono().MapsCoupStruct()->Map(1);

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
Teuchos::RCP<SSI::MeshtyingStrategyBase> SSI::BuildMeshtyingStrategy(const SSI::SSIMono& ssi_mono,
    LINALG::MatrixType matrixtype_ssi, LINALG::MatrixType matrixtype_scatra)
{
  Teuchos::RCP<SSI::MeshtyingStrategyBase> meshtying_strategy = Teuchos::null;

  switch (matrixtype_ssi)
  {
    case LINALG::MatrixType::block_field:
    {
      switch (matrixtype_scatra)
      {
        case LINALG::MatrixType::block_condition:
        case LINALG::MatrixType::block_condition_dof:
        {
          meshtying_strategy = Teuchos::rcp(new SSI::MeshtyingStrategyBlockBlock(ssi_mono));
          break;
        }
        case LINALG::MatrixType::sparse:
        {
          meshtying_strategy = Teuchos::rcp(new SSI::MeshtyingStrategyBlockSparse(ssi_mono));
          break;
        }

        default:
        {
          dserror("unknown matrix type of ScaTra field");
          break;
        }
      }
      break;
    }
    case LINALG::MatrixType::sparse:
    {
      meshtying_strategy = Teuchos::rcp(new SSI::MeshtyingStrategySparse(ssi_mono));
      break;
    }
    default:
    {
      dserror("unknown matrix type of SSI problem");
      break;
    }
  }

  return meshtying_strategy;
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
ADAPTER::CouplingSlaveConverter& SSI::MeshtyingStrategyBase::StructureSlaveConverter() const
{
  return slave_side_converter_->InterfaceCouplingAdapterStructureSlaveConverter();
}

/*-------------------------------------------------------------------------*
 *-------------------------------------------------------------------------*/
ADAPTER::CouplingSlaveConverter&
SSI::MeshtyingStrategyBase::StructureSlaveConverter3DomainIntersection() const
{
  return slave_side_converter_
      ->InterfaceCouplingAdapterStructureSlaveConverter3DomainIntersection();
}