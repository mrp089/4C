/*----------------------------------------------------------------------*/
/*! \file
\brief Test for the CUT Library

\level 1

*----------------------------------------------------------------------*/
// This test was automatically generated by CUT::OUTPUT::GmshElementCutTest(),
// as the cut crashed for this configuration!

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "cut_test_utils.H"

#include "../../src/drt_cut/cut_side.H"
#include "../../src/drt_cut/cut_meshintersection.H"
#include "../../src/drt_cut/cut_levelsetintersection.H"
#include "../../src/drt_cut/cut_combintersection.H"
#include "../../src/drt_cut/cut_tetmeshintersection.H"
#include "../../src/drt_cut/cut_options.H"
#include "../../src/drt_cut/cut_volumecell.H"

#include "../../src/drt_fem_general/drt_utils_local_connectivity_matrices.H"

void test_bacigenerated_238343()
{
  GEO::CUT::MeshIntersection intersection;
  intersection.GetOptions().Init_for_Cuttests();  // use full cln
  std::vector<int> nids;

  int sidecount = 0;
  std::vector<double> lsvs(8);
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35967476430280587163;
    tri3_xyze(1, 0) = 0.14618067301976458983;
    tri3_xyze(2, 0) = -0.13985273439446022081;
    nids.push_back(3816);
    tri3_xyze(0, 1) = 0.35382432467570429369;
    tri3_xyze(1, 1) = 0.16132844557064748847;
    tri3_xyze(2, 1) = -0.13935078984092844068;
    nids.push_back(3818);
    tri3_xyze(0, 2) = 0.35089302897608382059;
    tri3_xyze(1, 2) = 0.16603269507076770517;
    tri3_xyze(2, 2) = -0.13326764788321598942;
    nids.push_back(-390);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35382432467570429369;
    tri3_xyze(1, 0) = 0.16132844557064748847;
    tri3_xyze(2, 0) = -0.13935078984092844068;
    nids.push_back(3818);
    tri3_xyze(0, 1) = 0.34154975075164428766;
    tri3_xyze(1, 1) = 0.18565168465957157529;
    tri3_xyze(2, 1) = -0.12665101811319384728;
    nids.push_back(3457);
    tri3_xyze(0, 2) = 0.35089302897608382059;
    tri3_xyze(1, 2) = 0.16603269507076770517;
    tri3_xyze(2, 2) = -0.13326764788321598942;
    nids.push_back(-390);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.34852327617418077388;
    tri3_xyze(1, 0) = 0.17096997703308713934;
    tri3_xyze(2, 0) = -0.12721604918428153219;
    nids.push_back(3455);
    tri3_xyze(0, 1) = 0.35967476430280587163;
    tri3_xyze(1, 1) = 0.14618067301976458983;
    tri3_xyze(2, 1) = -0.13985273439446022081;
    nids.push_back(3816);
    tri3_xyze(0, 2) = 0.35089302897608382059;
    tri3_xyze(1, 2) = 0.16603269507076770517;
    tri3_xyze(2, 2) = -0.13326764788321598942;
    nids.push_back(-390);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.34226269626628180731;
    tri3_xyze(1, 0) = 0.17958602181458011016;
    tri3_xyze(2, 0) = -0.15075004712383116567;
    nids.push_back(4255);
    tri3_xyze(0, 1) = 0.35082515461553009928;
    tri3_xyze(1, 1) = 0.16574018148498959047;
    tri3_xyze(2, 1) = -0.15110731046936637378;
    nids.push_back(4253);
    tri3_xyze(0, 2) = 0.35204784136082939439;
    tri3_xyze(1, 2) = 0.16026127477404145116;
    tri3_xyze(2, 2) = -0.15723040356078801794;
    nids.push_back(-473);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35082515461553009928;
    tri3_xyze(1, 0) = 0.16574018148498959047;
    tri3_xyze(2, 0) = -0.15110731046936637378;
    nids.push_back(4253);
    tri3_xyze(0, 1) = 0.36130443025033198712;
    tri3_xyze(1, 1) = 0.14064351170938668711;
    tri3_xyze(2, 1) = -0.16369264176047274018;
    nids.push_back(4614);
    tri3_xyze(0, 2) = 0.35204784136082939439;
    tri3_xyze(1, 2) = 0.16026127477404145116;
    tri3_xyze(2, 2) = -0.15723040356078801794;
    nids.push_back(-473);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35967476430280587163;
    tri3_xyze(1, 0) = 0.14618067301976458983;
    tri3_xyze(2, 0) = -0.13985273439446022081;
    nids.push_back(3816);
    tri3_xyze(0, 1) = 0.34852327617418077388;
    tri3_xyze(1, 1) = 0.17096997703308713934;
    tri3_xyze(2, 1) = -0.12721604918428153219;
    nids.push_back(3455);
    tri3_xyze(0, 2) = 0.35666287046434702601;
    tri3_xyze(1, 2) = 0.15089020335718952848;
    tri3_xyze(2, 2) = -0.13380353260109806302;
    nids.push_back(-389);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.36412085401098781556;
    tri3_xyze(1, 0) = 0.13608005166299699806;
    tri3_xyze(2, 0) = -0.15192674996553481859;
    nids.push_back(4179);
    tri3_xyze(0, 1) = 0.35382432467570429369;
    tri3_xyze(1, 1) = 0.16132844557064748847;
    tri3_xyze(2, 1) = -0.13935078984092844068;
    nids.push_back(3818);
    tri3_xyze(0, 2) = 0.36160884648086655258;
    tri3_xyze(1, 2) = 0.14103179227992093669;
    tri3_xyze(2, 2) = -0.14587176700181983535;
    nids.push_back(-430);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35382432467570429369;
    tri3_xyze(1, 0) = 0.16132844557064748847;
    tri3_xyze(2, 0) = -0.13935078984092844068;
    nids.push_back(3818);
    tri3_xyze(0, 1) = 0.35967476430280587163;
    tri3_xyze(1, 1) = 0.14618067301976458983;
    tri3_xyze(2, 1) = -0.13985273439446022081;
    nids.push_back(3816);
    tri3_xyze(0, 2) = 0.36160884648086655258;
    tri3_xyze(1, 2) = 0.14103179227992093669;
    tri3_xyze(2, 2) = -0.14587176700181983535;
    nids.push_back(-430);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35812447393829804909;
    tri3_xyze(1, 0) = 0.15119523765030246087;
    tri3_xyze(2, 0) = -0.15150583641078704811;
    nids.push_back(4181);
    tri3_xyze(0, 1) = 0.3675036689964701897;
    tri3_xyze(1, 1) = 0.12561483149485705435;
    tri3_xyze(2, 1) = -0.16403551629625762187;
    nids.push_back(4542);
    tri3_xyze(0, 2) = 0.35943943195015759517;
    tri3_xyze(1, 2) = 0.1457984405848839482;
    tri3_xyze(2, 2) = -0.15758532623422094598;
    nids.push_back(-472);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.36130443025033198712;
    tri3_xyze(1, 0) = 0.14064351170938668711;
    tri3_xyze(2, 0) = -0.16369264176047274018;
    nids.push_back(4614);
    tri3_xyze(0, 1) = 0.35082515461553009928;
    tri3_xyze(1, 1) = 0.16574018148498959047;
    tri3_xyze(2, 1) = -0.15110731046936637378;
    nids.push_back(4253);
    tri3_xyze(0, 2) = 0.35943943195015759517;
    tri3_xyze(1, 2) = 0.1457984405848839482;
    tri3_xyze(2, 2) = -0.15758532623422094598;
    nids.push_back(-472);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35082515461553009928;
    tri3_xyze(1, 0) = 0.16574018148498959047;
    tri3_xyze(2, 0) = -0.15110731046936637378;
    nids.push_back(4253);
    tri3_xyze(0, 1) = 0.35812447393829804909;
    tri3_xyze(1, 1) = 0.15119523765030246087;
    tri3_xyze(2, 1) = -0.15150583641078704811;
    nids.push_back(4181);
    tri3_xyze(0, 2) = 0.35943943195015759517;
    tri3_xyze(1, 2) = 0.1457984405848839482;
    tri3_xyze(2, 2) = -0.15758532623422094598;
    nids.push_back(-472);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.36412085401098781556;
    tri3_xyze(1, 0) = 0.13608005166299699806;
    tri3_xyze(2, 0) = -0.15192674996553481859;
    nids.push_back(4179);
    tri3_xyze(0, 1) = 0.35812447393829804909;
    tri3_xyze(1, 1) = 0.15119523765030246087;
    tri3_xyze(2, 1) = -0.15150583641078704811;
    nids.push_back(4181);
    tri3_xyze(0, 2) = 0.35569409558476522415;
    tri3_xyze(1, 2) = 0.15613948312708000876;
    tri3_xyze(2, 2) = -0.14541291362073555105;
    nids.push_back(-431);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35812447393829804909;
    tri3_xyze(1, 0) = 0.15119523765030246087;
    tri3_xyze(2, 0) = -0.15150583641078704811;
    nids.push_back(4181);
    tri3_xyze(0, 1) = 0.34670672971407068275;
    tri3_xyze(1, 1) = 0.17595419762437308764;
    tri3_xyze(2, 1) = -0.13886827826569192457;
    nids.push_back(3820);
    tri3_xyze(0, 2) = 0.35569409558476522415;
    tri3_xyze(1, 2) = 0.15613948312708000876;
    tri3_xyze(2, 2) = -0.14541291362073555105;
    nids.push_back(-431);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.34670672971407068275;
    tri3_xyze(1, 0) = 0.17595419762437308764;
    tri3_xyze(2, 0) = -0.13886827826569192457;
    nids.push_back(3820);
    tri3_xyze(0, 1) = 0.35382432467570429369;
    tri3_xyze(1, 1) = 0.16132844557064748847;
    tri3_xyze(2, 1) = -0.13935078984092844068;
    nids.push_back(3818);
    tri3_xyze(0, 2) = 0.35569409558476522415;
    tri3_xyze(1, 2) = 0.15613948312708000876;
    tri3_xyze(2, 2) = -0.14541291362073555105;
    nids.push_back(-431);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35382432467570429369;
    tri3_xyze(1, 0) = 0.16132844557064748847;
    tri3_xyze(2, 0) = -0.13935078984092844068;
    nids.push_back(3818);
    tri3_xyze(0, 1) = 0.36412085401098781556;
    tri3_xyze(1, 1) = 0.13608005166299699806;
    tri3_xyze(2, 1) = -0.15192674996553481859;
    nids.push_back(4179);
    tri3_xyze(0, 2) = 0.35569409558476522415;
    tri3_xyze(1, 2) = 0.15613948312708000876;
    tri3_xyze(2, 2) = -0.14541291362073555105;
    nids.push_back(-431);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35812447393829804909;
    tri3_xyze(1, 0) = 0.15119523765030246087;
    tri3_xyze(2, 0) = -0.15150583641078704811;
    nids.push_back(4181);
    tri3_xyze(0, 1) = 0.35082515461553009928;
    tri3_xyze(1, 1) = 0.16574018148498959047;
    tri3_xyze(2, 1) = -0.15110731046936637378;
    nids.push_back(4253);
    tri3_xyze(0, 2) = 0.34849914551977811961;
    tri3_xyze(1, 2) = 0.17070209456162438455;
    tri3_xyze(2, 2) = -0.14497695208872846129;
    nids.push_back(-432);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.34154975075164428766;
    tri3_xyze(1, 0) = 0.18565168465957157529;
    tri3_xyze(2, 0) = -0.12665101811319384728;
    nids.push_back(3457);
    tri3_xyze(0, 1) = 0.35382432467570429369;
    tri3_xyze(1, 1) = 0.16132844557064748847;
    tri3_xyze(2, 1) = -0.13935078984092844068;
    nids.push_back(3818);
    tri3_xyze(0, 2) = 0.34385788200282918492;
    tri3_xyze(1, 2) = 0.18066239163616210073;
    tri3_xyze(2, 2) = -0.13274709401536149977;
    nids.push_back(-391);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.33834022381121353629;
    tri3_xyze(1, 0) = 0.18991876148683251024;
    tri3_xyze(2, 0) = -0.13842638320906852645;
    nids.push_back(3892);
    tri3_xyze(0, 1) = 0.34670672971407068275;
    tri3_xyze(1, 1) = 0.17595419762437308764;
    tri3_xyze(2, 1) = -0.13886827826569192457;
    nids.push_back(3820);
    tri3_xyze(0, 2) = 0.34849914551977811961;
    tri3_xyze(1, 2) = 0.17070209456162438455;
    tri3_xyze(2, 2) = -0.14497695208872846129;
    nids.push_back(-432);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.34670672971407068275;
    tri3_xyze(1, 0) = 0.17595419762437308764;
    tri3_xyze(2, 0) = -0.13886827826569192457;
    nids.push_back(3820);
    tri3_xyze(0, 1) = 0.35812447393829804909;
    tri3_xyze(1, 1) = 0.15119523765030246087;
    tri3_xyze(2, 1) = -0.15150583641078704811;
    nids.push_back(4181);
    tri3_xyze(0, 2) = 0.34849914551977811961;
    tri3_xyze(1, 2) = 0.17070209456162438455;
    tri3_xyze(2, 2) = -0.14497695208872846129;
    nids.push_back(-432);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.3675036689964701897;
    tri3_xyze(1, 0) = 0.12561483149485705435;
    tri3_xyze(2, 0) = -0.16403551629625762187;
    nids.push_back(4542);
    tri3_xyze(0, 1) = 0.35812447393829804909;
    tri3_xyze(1, 1) = 0.15119523765030246087;
    tri3_xyze(2, 1) = -0.15150583641078704811;
    nids.push_back(4181);
    tri3_xyze(0, 2) = 0.36552646590360982071;
    tri3_xyze(1, 2) = 0.13075028651984690886;
    tri3_xyze(2, 2) = -0.15796317140901561249;
    nids.push_back(-471);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35812447393829804909;
    tri3_xyze(1, 0) = 0.15119523765030246087;
    tri3_xyze(2, 0) = -0.15150583641078704811;
    nids.push_back(4181);
    tri3_xyze(0, 1) = 0.36412085401098781556;
    tri3_xyze(1, 1) = 0.13608005166299699806;
    tri3_xyze(2, 1) = -0.15192674996553481859;
    nids.push_back(4179);
    tri3_xyze(0, 2) = 0.36552646590360982071;
    tri3_xyze(1, 2) = 0.13075028651984690886;
    tri3_xyze(2, 2) = -0.15796317140901561249;
    nids.push_back(-471);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.35382432467570429369;
    tri3_xyze(1, 0) = 0.16132844557064748847;
    tri3_xyze(2, 0) = -0.13935078984092844068;
    nids.push_back(3818);
    tri3_xyze(0, 1) = 0.34670672971407068275;
    tri3_xyze(1, 1) = 0.17595419762437308764;
    tri3_xyze(2, 1) = -0.13886827826569192457;
    nids.push_back(3820);
    tri3_xyze(0, 2) = 0.34385788200282918492;
    tri3_xyze(1, 2) = 0.18066239163616210073;
    tri3_xyze(2, 2) = -0.13274709401536149977;
    nids.push_back(-391);
    intersection.AddCutSide(++sidecount, nids, tri3_xyze, DRT::Element::tri3);
  }
  {
    Epetra_SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.37193092002679312147;
    hex8_xyze(1, 0) = 0.15602938346927186508;
    hex8_xyze(2, 0) = -0.1560526315789479701;
    nids.push_back(258931);
    hex8_xyze(0, 1) = 0.36605541926728601965;
    hex8_xyze(1, 1) = 0.16935527096264008096;
    hex8_xyze(2, 1) = -0.1560526315789479701;
    nids.push_back(258932);
    hex8_xyze(0, 2) = 0.35939986618969899101;
    hex8_xyze(1, 2) = 0.16627608421786482795;
    hex8_xyze(2, 2) = -0.1560526315789479701;
    nids.push_back(258962);
    hex8_xyze(0, 3) = 0.36516853966266960008;
    hex8_xyze(1, 3) = 0.15319248558801237814;
    hex8_xyze(2, 3) = -0.1560526315789479701;
    nids.push_back(258961);
    hex8_xyze(0, 4) = 0.37193092002679312147;
    hex8_xyze(1, 4) = 0.15602938346927180957;
    hex8_xyze(2, 4) = -0.13631578947368463983;
    nids.push_back(247143);
    hex8_xyze(0, 5) = 0.36605541926728601965;
    hex8_xyze(1, 5) = 0.16935527096264002545;
    hex8_xyze(2, 5) = -0.13631578947368463983;
    nids.push_back(247144);
    hex8_xyze(0, 6) = 0.35939986618969899101;
    hex8_xyze(1, 6) = 0.16627608421786477244;
    hex8_xyze(2, 6) = -0.13631578947368463983;
    nids.push_back(247174);
    hex8_xyze(0, 7) = 0.36516853966266960008;
    hex8_xyze(1, 7) = 0.15319248558801232263;
    hex8_xyze(2, 7) = -0.13631578947368463983;
    nids.push_back(247173);

    intersection.AddElement(238314, nids, hex8_xyze, DRT::Element::hex8);
  }

  {
    Epetra_SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.36516853966266965559;
    hex8_xyze(1, 0) = 0.15319248558801232263;
    hex8_xyze(2, 0) = -0.17578947368421127262;
    nids.push_back(270749);
    hex8_xyze(0, 1) = 0.35939986618969904653;
    hex8_xyze(1, 1) = 0.16627608421786477244;
    hex8_xyze(2, 1) = -0.17578947368421127262;
    nids.push_back(270750);
    hex8_xyze(0, 2) = 0.35274431311211196238;
    hex8_xyze(1, 2) = 0.16319689747308946393;
    hex8_xyze(2, 2) = -0.17578947368421127262;
    nids.push_back(270780);
    hex8_xyze(0, 3) = 0.35840615929854607868;
    hex8_xyze(1, 3) = 0.15035558770675280793;
    hex8_xyze(2, 3) = -0.17578947368421127262;
    nids.push_back(270779);
    hex8_xyze(0, 4) = 0.36516853966266960008;
    hex8_xyze(1, 4) = 0.15319248558801237814;
    hex8_xyze(2, 4) = -0.1560526315789479701;
    nids.push_back(258961);
    hex8_xyze(0, 5) = 0.35939986618969899101;
    hex8_xyze(1, 5) = 0.16627608421786482795;
    hex8_xyze(2, 5) = -0.1560526315789479701;
    nids.push_back(258962);
    hex8_xyze(0, 6) = 0.35274431311211190687;
    hex8_xyze(1, 6) = 0.16319689747308951944;
    hex8_xyze(2, 6) = -0.1560526315789479701;
    nids.push_back(258992);
    hex8_xyze(0, 7) = 0.35840615929854602317;
    hex8_xyze(1, 7) = 0.15035558770675286344;
    hex8_xyze(2, 7) = -0.1560526315789479701;
    nids.push_back(258991);

    intersection.AddElement(250043, nids, hex8_xyze, DRT::Element::hex8);
  }

  {
    Epetra_SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.36516853966266960008;
    hex8_xyze(1, 0) = 0.15319248558801232263;
    hex8_xyze(2, 0) = -0.13631578947368463983;
    nids.push_back(247173);
    hex8_xyze(0, 1) = 0.35939986618969899101;
    hex8_xyze(1, 1) = 0.16627608421786477244;
    hex8_xyze(2, 1) = -0.13631578947368463983;
    nids.push_back(247174);
    hex8_xyze(0, 2) = 0.35274431311211190687;
    hex8_xyze(1, 2) = 0.16319689747308946393;
    hex8_xyze(2, 2) = -0.13631578947368463983;
    nids.push_back(247204);
    hex8_xyze(0, 3) = 0.35840615929854602317;
    hex8_xyze(1, 3) = 0.15035558770675280793;
    hex8_xyze(2, 3) = -0.13631578947368463983;
    nids.push_back(247203);
    hex8_xyze(0, 4) = 0.36516853966266960008;
    hex8_xyze(1, 4) = 0.15319248558801232263;
    hex8_xyze(2, 4) = -0.11657894736842105976;
    nids.push_back(235385);
    hex8_xyze(0, 5) = 0.35939986618969899101;
    hex8_xyze(1, 5) = 0.16627608421786477244;
    hex8_xyze(2, 5) = -0.11657894736842105976;
    nids.push_back(235386);
    hex8_xyze(0, 6) = 0.35274431311211190687;
    hex8_xyze(1, 6) = 0.16319689747308946393;
    hex8_xyze(2, 6) = -0.11657894736842105976;
    nids.push_back(235416);
    hex8_xyze(0, 7) = 0.35840615929854602317;
    hex8_xyze(1, 7) = 0.15035558770675280793;
    hex8_xyze(2, 7) = -0.11657894736842105976;
    nids.push_back(235415);

    intersection.AddElement(226643, nids, hex8_xyze, DRT::Element::hex8);
  }

  {
    Epetra_SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.37046110302653800783;
    hex8_xyze(1, 0) = 0.13990915318291657909;
    hex8_xyze(2, 0) = -0.1560526315789479701;
    nids.push_back(258960);
    hex8_xyze(0, 1) = 0.36516853966266960008;
    hex8_xyze(1, 1) = 0.15319248558801237814;
    hex8_xyze(2, 1) = -0.1560526315789479701;
    nids.push_back(258961);
    hex8_xyze(0, 2) = 0.35840615929854602317;
    hex8_xyze(1, 2) = 0.15035558770675286344;
    hex8_xyze(2, 2) = -0.1560526315789479701;
    nids.push_back(258991);
    hex8_xyze(0, 3) = 0.36360071222975021143;
    hex8_xyze(1, 3) = 0.13731824293878847065;
    hex8_xyze(2, 3) = -0.1560526315789479701;
    nids.push_back(258990);
    hex8_xyze(0, 4) = 0.37046110302653800783;
    hex8_xyze(1, 4) = 0.13990915318291652358;
    hex8_xyze(2, 4) = -0.13631578947368463983;
    nids.push_back(247172);
    hex8_xyze(0, 5) = 0.36516853966266960008;
    hex8_xyze(1, 5) = 0.15319248558801232263;
    hex8_xyze(2, 5) = -0.13631578947368463983;
    nids.push_back(247173);
    hex8_xyze(0, 6) = 0.35840615929854602317;
    hex8_xyze(1, 6) = 0.15035558770675280793;
    hex8_xyze(2, 6) = -0.13631578947368463983;
    nids.push_back(247203);
    hex8_xyze(0, 7) = 0.36360071222975021143;
    hex8_xyze(1, 7) = 0.13731824293878841514;
    hex8_xyze(2, 7) = -0.13631578947368463983;
    nids.push_back(247202);

    intersection.AddElement(238342, nids, hex8_xyze, DRT::Element::hex8);
  }

  {
    Epetra_SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.36516853966266960008;
    hex8_xyze(1, 0) = 0.15319248558801237814;
    hex8_xyze(2, 0) = -0.1560526315789479701;
    nids.push_back(258961);
    hex8_xyze(0, 1) = 0.35939986618969899101;
    hex8_xyze(1, 1) = 0.16627608421786482795;
    hex8_xyze(2, 1) = -0.1560526315789479701;
    nids.push_back(258962);
    hex8_xyze(0, 2) = 0.35274431311211190687;
    hex8_xyze(1, 2) = 0.16319689747308951944;
    hex8_xyze(2, 2) = -0.1560526315789479701;
    nids.push_back(258992);
    hex8_xyze(0, 3) = 0.35840615929854602317;
    hex8_xyze(1, 3) = 0.15035558770675286344;
    hex8_xyze(2, 3) = -0.1560526315789479701;
    nids.push_back(258991);
    hex8_xyze(0, 4) = 0.36516853966266960008;
    hex8_xyze(1, 4) = 0.15319248558801232263;
    hex8_xyze(2, 4) = -0.13631578947368463983;
    nids.push_back(247173);
    hex8_xyze(0, 5) = 0.35939986618969899101;
    hex8_xyze(1, 5) = 0.16627608421786477244;
    hex8_xyze(2, 5) = -0.13631578947368463983;
    nids.push_back(247174);
    hex8_xyze(0, 6) = 0.35274431311211190687;
    hex8_xyze(1, 6) = 0.16319689747308946393;
    hex8_xyze(2, 6) = -0.13631578947368463983;
    nids.push_back(247204);
    hex8_xyze(0, 7) = 0.35840615929854602317;
    hex8_xyze(1, 7) = 0.15035558770675280793;
    hex8_xyze(2, 7) = -0.13631578947368463983;
    nids.push_back(247203);

    intersection.AddElement(238343, nids, hex8_xyze, DRT::Element::hex8);
  }

  {
    Epetra_SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.35939986618969899101;
    hex8_xyze(1, 0) = 0.16627608421786482795;
    hex8_xyze(2, 0) = -0.1560526315789479701;
    nids.push_back(258962);
    hex8_xyze(0, 1) = 0.35316260385729147941;
    hex8_xyze(1, 1) = 0.17914289055594112554;
    hex8_xyze(2, 1) = -0.1560526315789479701;
    nids.push_back(258963);
    hex8_xyze(0, 2) = 0.34662255563771193767;
    hex8_xyze(1, 2) = 0.17582542961971994733;
    hex8_xyze(2, 2) = -0.1560526315789479701;
    nids.push_back(258993);
    hex8_xyze(0, 3) = 0.35274431311211190687;
    hex8_xyze(1, 3) = 0.16319689747308951944;
    hex8_xyze(2, 3) = -0.1560526315789479701;
    nids.push_back(258992);
    hex8_xyze(0, 4) = 0.35939986618969899101;
    hex8_xyze(1, 4) = 0.16627608421786477244;
    hex8_xyze(2, 4) = -0.13631578947368463983;
    nids.push_back(247174);
    hex8_xyze(0, 5) = 0.35316260385729147941;
    hex8_xyze(1, 5) = 0.17914289055594107003;
    hex8_xyze(2, 5) = -0.13631578947368463983;
    nids.push_back(247175);
    hex8_xyze(0, 6) = 0.34662255563771193767;
    hex8_xyze(1, 6) = 0.17582542961971989182;
    hex8_xyze(2, 6) = -0.13631578947368463983;
    nids.push_back(247205);
    hex8_xyze(0, 7) = 0.35274431311211190687;
    hex8_xyze(1, 7) = 0.16319689747308946393;
    hex8_xyze(2, 7) = -0.13631578947368463983;
    nids.push_back(247204);

    intersection.AddElement(238344, nids, hex8_xyze, DRT::Element::hex8);
  }

  {
    Epetra_SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.35840615929854602317;
    hex8_xyze(1, 0) = 0.15035558770675286344;
    hex8_xyze(2, 0) = -0.1560526315789479701;
    nids.push_back(258991);
    hex8_xyze(0, 1) = 0.35274431311211190687;
    hex8_xyze(1, 1) = 0.16319689747308951944;
    hex8_xyze(2, 1) = -0.1560526315789479701;
    nids.push_back(258992);
    hex8_xyze(0, 2) = 0.34608876003452493375;
    hex8_xyze(1, 2) = 0.16011771072831426643;
    hex8_xyze(2, 2) = -0.1560526315789479701;
    nids.push_back(259022);
    hex8_xyze(0, 3) = 0.35164377893442255729;
    hex8_xyze(1, 3) = 0.14751868982549340426;
    hex8_xyze(2, 3) = -0.1560526315789479701;
    nids.push_back(259021);
    hex8_xyze(0, 4) = 0.35840615929854602317;
    hex8_xyze(1, 4) = 0.15035558770675280793;
    hex8_xyze(2, 4) = -0.13631578947368463983;
    nids.push_back(247203);
    hex8_xyze(0, 5) = 0.35274431311211190687;
    hex8_xyze(1, 5) = 0.16319689747308946393;
    hex8_xyze(2, 5) = -0.13631578947368463983;
    nids.push_back(247204);
    hex8_xyze(0, 6) = 0.34608876003452493375;
    hex8_xyze(1, 6) = 0.16011771072831421092;
    hex8_xyze(2, 6) = -0.13631578947368463983;
    nids.push_back(247234);
    hex8_xyze(0, 7) = 0.35164377893442255729;
    hex8_xyze(1, 7) = 0.14751868982549334874;
    hex8_xyze(2, 7) = -0.13631578947368463983;
    nids.push_back(247233);

    intersection.AddElement(238372, nids, hex8_xyze, DRT::Element::hex8);
  }

  intersection.Status();

  intersection.CutTest_Cut(
      true, INPAR::CUT::VCellGaussPts_DirectDivergence, INPAR::CUT::BCellGaussPts_Tessellation);
  intersection.Cut_Finalize(true, INPAR::CUT::VCellGaussPts_DirectDivergence,
      INPAR::CUT::BCellGaussPts_Tessellation, false, true);
}
