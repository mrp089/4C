/*----------------------------------------------------------------------*/
/*! \file

\brief prestress functionality in solid elements

\level 2

*----------------------------------------------------------------------*/

#include "baci_so3_prestress.hpp"

#include "baci_utils_exceptions.hpp"

FOUR_C_NAMESPACE_OPEN


DRT::ELEMENTS::PreStressType DRT::ELEMENTS::PreStressType::instance_;


/*----------------------------------------------------------------------*
 |  ctor (public)                                            mwgee 07/08|
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::PreStress::PreStress(const int numnode, const int ngp, const bool istet4)
    : ParObject(), isinit_(false), numnode_(numnode)
{
  // allocate history memory
  Fhist_ = Teuchos::rcp(new CORE::LINALG::SerialDenseMatrix(ngp, 9));
  if (!istet4)
    invJhist_ = Teuchos::rcp(new CORE::LINALG::SerialDenseMatrix(ngp, 9));
  else
    invJhist_ = Teuchos::rcp(new CORE::LINALG::SerialDenseMatrix(ngp, 12));

  // init the deformation gradient history
  CORE::LINALG::Matrix<3, 3> F(true);
  F(0, 0) = F(1, 1) = F(2, 2) = 1.0;
  for (int i = 0; i < NGP(); ++i) MatrixtoStorage(i, F, FHistory());
}

/*----------------------------------------------------------------------*
 |  copy-ctor (public)                                       mwgee 11/06|
 *----------------------------------------------------------------------*/
DRT::ELEMENTS::PreStress::PreStress(const DRT::ELEMENTS::PreStress& old)
    : ParObject(old),
      isinit_(old.isinit_),
      numnode_(old.numnode_),
      Fhist_(Teuchos::rcp(new CORE::LINALG::SerialDenseMatrix(old.FHistory()))),
      invJhist_(Teuchos::rcp(new CORE::LINALG::SerialDenseMatrix(old.JHistory())))
{
  return;
}


/*----------------------------------------------------------------------*
 |  Pack data                                                  (public) |
 |                                                            gee 02/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::PreStress::Pack(CORE::COMM::PackBuffer& data) const
{
  CORE::COMM::PackBuffer::SizeMarker sm(data);
  sm.Insert();

  // pack type of this instance of ParObject
  int type = UniqueParObjectId();
  AddtoPack(data, type);

  // pack isinit_
  AddtoPack(data, isinit_);

  // pack numnode_
  AddtoPack(data, numnode_);

  // pack Fhist_
  AddtoPack(data, *Fhist_);

  // pack invJhist_
  AddtoPack(data, *invJhist_);

  return;
}


/*----------------------------------------------------------------------*
 |  Unpack data                                                (public) |
 |                                                            gee 02/07 |
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::PreStress::Unpack(const std::vector<char>& data)
{
  std::vector<char>::size_type position = 0;

  CORE::COMM::ExtractAndAssertId(position, data, UniqueParObjectId());

  // extract isinit_
  isinit_ = ExtractInt(position, data);

  // extract numnode_
  ExtractfromPack(position, data, numnode_);

  // extract Fhist_
  ExtractfromPack(position, data, *Fhist_);

  // extract invJhist_
  ExtractfromPack(position, data, *invJhist_);

  if (position != data.size())
    dserror("Mismatch in size of data %d <-> %d", (int)data.size(), position);
  return;
}

int DRT::ELEMENTS::PreStress::UniqueParObjectId() const
{
  return PreStressType::Instance().UniqueParObjectId();
}

FOUR_C_NAMESPACE_CLOSE
