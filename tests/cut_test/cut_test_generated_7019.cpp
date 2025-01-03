// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_cut_combintersection.hpp"
#include "4C_cut_levelsetintersection.hpp"
#include "4C_cut_meshintersection.hpp"
#include "4C_cut_options.hpp"
#include "4C_cut_side.hpp"
#include "4C_cut_tetmeshintersection.hpp"
#include "4C_cut_volumecell.hpp"
#include "4C_fem_general_utils_local_connectivity_matrices.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "cut_test_utils.hpp"

void test_generated_7019()
{
  Cut::MeshIntersection intersection;
  intersection.get_options().init_for_cuttests();  // use full cln
  std::vector<int> nids;

  int sidecount = 0;
  std::vector<double> lsvs(8);
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3375);
    tri3_xyze(0, 1) = -0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3377);
    tri3_xyze(0, 2) = -0.0555856;
    tri3_xyze(1, 2) = -0.0426524;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-194);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0816361;
    tri3_xyze(1, 0) = -0.0816361;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4861);
    tri3_xyze(0, 1) = 0.0773356;
    tri3_xyze(1, 1) = -0.0773356;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4863);
    tri3_xyze(0, 2) = 0.0678454;
    tri3_xyze(1, 2) = -0.0884179;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-487);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0773356;
    tri3_xyze(1, 0) = -0.0773356;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4863);
    tri3_xyze(0, 1) = 0.0546845;
    tri3_xyze(1, 1) = -0.0947164;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4613);
    tri3_xyze(0, 2) = 0.0678454;
    tri3_xyze(1, 2) = -0.0884179;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-487);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0546845;
    tri3_xyze(1, 0) = -0.0947164;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4613);
    tri3_xyze(0, 1) = 0.0577254;
    tri3_xyze(1, 1) = -0.0999834;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4611);
    tri3_xyze(0, 2) = 0.0678454;
    tri3_xyze(1, 2) = -0.0884179;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-487);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0577254;
    tri3_xyze(1, 0) = -0.0999834;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4611);
    tri3_xyze(0, 1) = 0.0816361;
    tri3_xyze(1, 1) = -0.0816361;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4861);
    tri3_xyze(0, 2) = 0.0678454;
    tri3_xyze(1, 2) = -0.0884179;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-487);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0773356;
    tri3_xyze(1, 0) = -0.0773356;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4863);
    tri3_xyze(0, 1) = 0.0729307;
    tri3_xyze(1, 1) = -0.0729307;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4865);
    tri3_xyze(0, 2) = 0.0641301;
    tri3_xyze(1, 2) = -0.083576;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-488);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0729307;
    tri3_xyze(1, 0) = -0.0729307;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4865);
    tri3_xyze(0, 1) = 0.0515698;
    tri3_xyze(1, 1) = -0.0893214;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4615);
    tri3_xyze(0, 2) = 0.0641301;
    tri3_xyze(1, 2) = -0.083576;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-488);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4615);
    tri3_xyze(0, 1) = 0.0546845;
    tri3_xyze(1, 1) = -0.0947164;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4613);
    tri3_xyze(0, 2) = 0.0641301;
    tri3_xyze(1, 2) = -0.083576;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-488);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0546845;
    tri3_xyze(1, 0) = -0.0947164;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4613);
    tri3_xyze(0, 1) = 0.0773356;
    tri3_xyze(1, 1) = -0.0773356;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4863);
    tri3_xyze(0, 2) = 0.0641301;
    tri3_xyze(1, 2) = -0.083576;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-488);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3615);
    tri3_xyze(0, 1) = -0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3617);
    tri3_xyze(0, 2) = -0.0603553;
    tri3_xyze(1, 2) = -0.0786566;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-239);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3617);
    tri3_xyze(0, 1) = -0.0684907;
    tri3_xyze(1, 1) = -0.0684907;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3367);
    tri3_xyze(0, 2) = -0.0603553;
    tri3_xyze(1, 2) = -0.0786566;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-239);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3617);
    tri3_xyze(0, 1) = -0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3619);
    tri3_xyze(0, 2) = -0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-240);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3619);
    tri3_xyze(0, 1) = -0.0640857;
    tri3_xyze(1, 1) = -0.0640857;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3369);
    tri3_xyze(0, 2) = -0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-240);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0684907;
    tri3_xyze(1, 0) = -0.0684907;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3367);
    tri3_xyze(0, 1) = -0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3617);
    tri3_xyze(0, 2) = -0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-240);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3619);
    tri3_xyze(0, 1) = -0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3621);
    tri3_xyze(0, 2) = -0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-241);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3621);
    tri3_xyze(0, 1) = -0.0597853;
    tri3_xyze(1, 1) = -0.0597853;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3371);
    tri3_xyze(0, 2) = -0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-241);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0640857;
    tri3_xyze(1, 0) = -0.0640857;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3369);
    tri3_xyze(0, 1) = -0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3619);
    tri3_xyze(0, 2) = -0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-241);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3621);
    tri3_xyze(0, 1) = -0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3623);
    tri3_xyze(0, 2) = -0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-242);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3623);
    tri3_xyze(0, 1) = -0.0556571;
    tri3_xyze(1, 1) = -0.0556571;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3373);
    tri3_xyze(0, 2) = -0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-242);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0556571;
    tri3_xyze(1, 0) = -0.0556571;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3373);
    tri3_xyze(0, 1) = -0.0597853;
    tri3_xyze(1, 1) = -0.0597853;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3371);
    tri3_xyze(0, 2) = -0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-242);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0597853;
    tri3_xyze(1, 0) = -0.0597853;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3371);
    tri3_xyze(0, 1) = -0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3621);
    tri3_xyze(0, 2) = -0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-242);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3623);
    tri3_xyze(0, 1) = -0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3625);
    tri3_xyze(0, 2) = -0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-243);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3625);
    tri3_xyze(0, 1) = -0.0517663;
    tri3_xyze(1, 1) = -0.0517663;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3375);
    tri3_xyze(0, 2) = -0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-243);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3375);
    tri3_xyze(0, 1) = -0.0556571;
    tri3_xyze(1, 1) = -0.0556571;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3373);
    tri3_xyze(0, 2) = -0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-243);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0556571;
    tri3_xyze(1, 0) = -0.0556571;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3373);
    tri3_xyze(0, 1) = -0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3623);
    tri3_xyze(0, 2) = -0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-243);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3625);
    tri3_xyze(0, 1) = -0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3627);
    tri3_xyze(0, 2) = -0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-244);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3627);
    tri3_xyze(0, 1) = -0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3377);
    tri3_xyze(0, 2) = -0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-244);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0481743;
    tri3_xyze(1, 0) = -0.0481743;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3377);
    tri3_xyze(0, 1) = -0.0517663;
    tri3_xyze(1, 1) = -0.0517663;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3375);
    tri3_xyze(0, 2) = -0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-244);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3375);
    tri3_xyze(0, 1) = -0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3625);
    tri3_xyze(0, 2) = -0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-244);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3627);
    tri3_xyze(0, 1) = -0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3629);
    tri3_xyze(0, 2) = -0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-245);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3629);
    tri3_xyze(0, 1) = -0.0449377;
    tri3_xyze(1, 1) = -0.0449377;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3379);
    tri3_xyze(0, 2) = -0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-245);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0449377;
    tri3_xyze(1, 0) = -0.0449377;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3379);
    tri3_xyze(0, 1) = -0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3377);
    tri3_xyze(0, 2) = -0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-245);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0481743;
    tri3_xyze(1, 0) = -0.0481743;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3377);
    tri3_xyze(0, 1) = -0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3627);
    tri3_xyze(0, 2) = -0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-245);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3629);
    tri3_xyze(0, 1) = -0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3631);
    tri3_xyze(0, 2) = -0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-246);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3631);
    tri3_xyze(0, 1) = -0.0421076;
    tri3_xyze(1, 1) = -0.0421076;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3381);
    tri3_xyze(0, 2) = -0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-246);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0449377;
    tri3_xyze(1, 0) = -0.0449377;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3379);
    tri3_xyze(0, 1) = -0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3629);
    tri3_xyze(0, 2) = -0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-246);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3631);
    tri3_xyze(0, 1) = -0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(3633);
    tri3_xyze(0, 2) = -0.0349258;
    tri3_xyze(1, 2) = -0.0455161;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-247);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0421076;
    tri3_xyze(1, 0) = -0.0421076;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3381);
    tri3_xyze(0, 1) = -0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3631);
    tri3_xyze(0, 2) = -0.0349258;
    tri3_xyze(1, 2) = -0.0455161;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-247);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3863);
    tri3_xyze(0, 1) = -0.0546845;
    tri3_xyze(1, 1) = -0.0947164;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3613);
    tri3_xyze(0, 2) = -0.0426494;
    tri3_xyze(1, 2) = -0.102965;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-287);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0546845;
    tri3_xyze(1, 0) = -0.0947164;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3613);
    tri3_xyze(0, 1) = -0.0577254;
    tri3_xyze(1, 1) = -0.0999834;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3611);
    tri3_xyze(0, 2) = -0.0426494;
    tri3_xyze(1, 2) = -0.102965;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-287);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3863);
    tri3_xyze(0, 1) = -0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3865);
    tri3_xyze(0, 2) = -0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-288);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3865);
    tri3_xyze(0, 1) = -0.0515698;
    tri3_xyze(1, 1) = -0.0893214;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3615);
    tri3_xyze(0, 2) = -0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-288);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3615);
    tri3_xyze(0, 1) = -0.0546845;
    tri3_xyze(1, 1) = -0.0947164;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3613);
    tri3_xyze(0, 2) = -0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-288);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0546845;
    tri3_xyze(1, 0) = -0.0947164;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3613);
    tri3_xyze(0, 1) = -0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3863);
    tri3_xyze(0, 2) = -0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-288);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3865);
    tri3_xyze(0, 1) = -0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3867);
    tri3_xyze(0, 2) = -0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-289);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3867);
    tri3_xyze(0, 1) = -0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3617);
    tri3_xyze(0, 2) = -0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-289);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3617);
    tri3_xyze(0, 1) = -0.0515698;
    tri3_xyze(1, 1) = -0.0893214;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3615);
    tri3_xyze(0, 2) = -0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-289);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3615);
    tri3_xyze(0, 1) = -0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3865);
    tri3_xyze(0, 2) = -0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-289);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3867);
    tri3_xyze(0, 1) = -0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3869);
    tri3_xyze(0, 2) = -0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-290);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3869);
    tri3_xyze(0, 1) = -0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3619);
    tri3_xyze(0, 2) = -0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-290);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3619);
    tri3_xyze(0, 1) = -0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3617);
    tri3_xyze(0, 2) = -0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-290);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3617);
    tri3_xyze(0, 1) = -0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3867);
    tri3_xyze(0, 2) = -0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-290);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3869);
    tri3_xyze(0, 1) = -0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3871);
    tri3_xyze(0, 2) = -0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-291);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3871);
    tri3_xyze(0, 1) = -0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3621);
    tri3_xyze(0, 2) = -0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-291);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3621);
    tri3_xyze(0, 1) = -0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3619);
    tri3_xyze(0, 2) = -0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-291);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3619);
    tri3_xyze(0, 1) = -0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3869);
    tri3_xyze(0, 2) = -0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-291);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3871);
    tri3_xyze(0, 1) = -0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3873);
    tri3_xyze(0, 2) = -0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-292);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3873);
    tri3_xyze(0, 1) = -0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3623);
    tri3_xyze(0, 2) = -0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-292);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3623);
    tri3_xyze(0, 1) = -0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3621);
    tri3_xyze(0, 2) = -0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-292);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3621);
    tri3_xyze(0, 1) = -0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3871);
    tri3_xyze(0, 2) = -0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-292);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3873);
    tri3_xyze(0, 1) = -0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3875);
    tri3_xyze(0, 2) = -0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-293);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3875);
    tri3_xyze(0, 1) = -0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3625);
    tri3_xyze(0, 2) = -0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-293);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3625);
    tri3_xyze(0, 1) = -0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3623);
    tri3_xyze(0, 2) = -0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-293);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3623);
    tri3_xyze(0, 1) = -0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3873);
    tri3_xyze(0, 2) = -0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-293);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3875);
    tri3_xyze(0, 1) = -0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3877);
    tri3_xyze(0, 2) = -0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-294);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3877);
    tri3_xyze(0, 1) = -0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3627);
    tri3_xyze(0, 2) = -0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-294);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3627);
    tri3_xyze(0, 1) = -0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3625);
    tri3_xyze(0, 2) = -0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-294);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3625);
    tri3_xyze(0, 1) = -0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3875);
    tri3_xyze(0, 2) = -0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-294);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3877);
    tri3_xyze(0, 1) = -0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3879);
    tri3_xyze(0, 2) = -0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-295);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3879);
    tri3_xyze(0, 1) = -0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3629);
    tri3_xyze(0, 2) = -0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-295);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3629);
    tri3_xyze(0, 1) = -0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3627);
    tri3_xyze(0, 2) = -0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-295);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3627);
    tri3_xyze(0, 1) = -0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3877);
    tri3_xyze(0, 2) = -0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-295);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3879);
    tri3_xyze(0, 1) = -0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3881);
    tri3_xyze(0, 2) = -0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-296);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3881);
    tri3_xyze(0, 1) = -0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3631);
    tri3_xyze(0, 2) = -0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-296);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3631);
    tri3_xyze(0, 1) = -0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3629);
    tri3_xyze(0, 2) = -0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-296);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3629);
    tri3_xyze(0, 1) = -0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3879);
    tri3_xyze(0, 2) = -0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-296);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3881);
    tri3_xyze(0, 1) = -0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(3883);
    tri3_xyze(0, 2) = -0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-297);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(3883);
    tri3_xyze(0, 1) = -0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(3633);
    tri3_xyze(0, 2) = -0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-297);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0280923;
    tri3_xyze(1, 0) = -0.0486573;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(3633);
    tri3_xyze(0, 1) = -0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3631);
    tri3_xyze(0, 2) = -0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-297);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3631);
    tri3_xyze(0, 1) = -0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3881);
    tri3_xyze(0, 2) = -0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-297);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(3883);
    tri3_xyze(0, 1) = -0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(3885);
    tri3_xyze(0, 2) = -0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-298);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(3885);
    tri3_xyze(0, 1) = -0.0267556;
    tri3_xyze(1, 1) = -0.046342;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(3635);
    tri3_xyze(0, 2) = -0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-298);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0267556;
    tri3_xyze(1, 0) = -0.046342;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(3635);
    tri3_xyze(0, 1) = -0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(3633);
    tri3_xyze(0, 2) = -0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-298);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0280923;
    tri3_xyze(1, 0) = -0.0486573;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(3633);
    tri3_xyze(0, 1) = -0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(3883);
    tri3_xyze(0, 2) = -0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-298);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(3885);
    tri3_xyze(0, 1) = -0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(3887);
    tri3_xyze(0, 2) = -0.0199346;
    tri3_xyze(1, 2) = -0.0481263;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-299);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0267556;
    tri3_xyze(1, 0) = -0.046342;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(3635);
    tri3_xyze(0, 1) = -0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(3885);
    tri3_xyze(0, 2) = -0.0199346;
    tri3_xyze(1, 2) = -0.0481263;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-299);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.01416e-15;
    tri3_xyze(1, 0) = -0.05;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4041);
    tri3_xyze(0, 1) = 1.02547e-15;
    tri3_xyze(1, 1) = -0.0503943;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4042);
    tri3_xyze(0, 2) = -0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-301);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02547e-15;
    tri3_xyze(1, 0) = -0.0503943;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4042);
    tri3_xyze(0, 1) = -0.013043;
    tri3_xyze(1, 1) = -0.0486771;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(3792);
    tri3_xyze(0, 2) = -0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-301);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.012941;
    tri3_xyze(1, 0) = -0.0482963;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(3791);
    tri3_xyze(0, 1) = 1.01416e-15;
    tri3_xyze(1, 1) = -0.05;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4041);
    tri3_xyze(0, 2) = -0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-301);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02547e-15;
    tri3_xyze(1, 0) = -0.0503943;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4139);
    tri3_xyze(0, 1) = 1.01416e-15;
    tri3_xyze(1, 1) = -0.05;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4041);
    tri3_xyze(0, 2) = -0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-302);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.01416e-15;
    tri3_xyze(1, 0) = -0.05;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4041);
    tri3_xyze(0, 1) = -0.012941;
    tri3_xyze(1, 1) = -0.0482963;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(3791);
    tri3_xyze(0, 2) = -0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-302);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(3889);
    tri3_xyze(0, 1) = 1.02547e-15;
    tri3_xyze(1, 1) = -0.0503943;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4139);
    tri3_xyze(0, 2) = -0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-302);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02547e-15;
    tri3_xyze(1, 0) = -0.0503943;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4042);
    tri3_xyze(0, 1) = 1.02606e-15;
    tri3_xyze(1, 1) = -0.0515708;
    tri3_xyze(2, 1) = 0.787566;
    nids.push_back(4045);
    tri3_xyze(0, 2) = -0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.790649;
    nids.push_back(-303);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02606e-15;
    tri3_xyze(1, 0) = -0.0515708;
    tri3_xyze(2, 0) = 0.787566;
    nids.push_back(4045);
    tri3_xyze(0, 1) = -0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.787566;
    nids.push_back(3795);
    tri3_xyze(0, 2) = -0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.790649;
    nids.push_back(-303);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(3792);
    tri3_xyze(0, 1) = 1.02547e-15;
    tri3_xyze(1, 1) = -0.0503943;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4042);
    tri3_xyze(0, 2) = -0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.790649;
    nids.push_back(-303);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02606e-15;
    tri3_xyze(1, 0) = -0.0515708;
    tri3_xyze(2, 0) = 0.787566;
    nids.push_back(4045);
    tri3_xyze(0, 1) = 1.01516e-15;
    tri3_xyze(1, 1) = -0.0535112;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4047);
    tri3_xyze(0, 2) = -0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-304);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.01516e-15;
    tri3_xyze(1, 0) = -0.0535112;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4047);
    tri3_xyze(0, 1) = -0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(3797);
    tri3_xyze(0, 2) = -0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-304);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.787566;
    nids.push_back(3795);
    tri3_xyze(0, 1) = 1.02606e-15;
    tri3_xyze(1, 1) = -0.0515708;
    tri3_xyze(2, 1) = 0.787566;
    nids.push_back(4045);
    tri3_xyze(0, 2) = -0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-304);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.01516e-15;
    tri3_xyze(1, 0) = -0.0535112;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4047);
    tri3_xyze(0, 1) = 1.02839e-15;
    tri3_xyze(1, 1) = -0.0561847;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4049);
    tri3_xyze(0, 2) = -0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-305);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02839e-15;
    tri3_xyze(1, 0) = -0.0561847;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4049);
    tri3_xyze(0, 1) = -0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(3799);
    tri3_xyze(0, 2) = -0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-305);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(3797);
    tri3_xyze(0, 1) = 1.01516e-15;
    tri3_xyze(1, 1) = -0.0535112;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4047);
    tri3_xyze(0, 2) = -0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-305);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02839e-15;
    tri3_xyze(1, 0) = -0.0561847;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4049);
    tri3_xyze(0, 1) = 1.03009e-15;
    tri3_xyze(1, 1) = -0.0595492;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4051);
    tri3_xyze(0, 2) = -0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-306);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03009e-15;
    tri3_xyze(1, 0) = -0.0595492;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4051);
    tri3_xyze(0, 1) = -0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(3801);
    tri3_xyze(0, 2) = -0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-306);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(3799);
    tri3_xyze(0, 1) = 1.02839e-15;
    tri3_xyze(1, 1) = -0.0561847;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4049);
    tri3_xyze(0, 2) = -0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-306);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03009e-15;
    tri3_xyze(1, 0) = -0.0595492;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4051);
    tri3_xyze(0, 1) = 1.03211e-15;
    tri3_xyze(1, 1) = -0.0635516;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4053);
    tri3_xyze(0, 2) = -0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-307);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03211e-15;
    tri3_xyze(1, 0) = -0.0635516;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4053);
    tri3_xyze(0, 1) = -0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(3803);
    tri3_xyze(0, 2) = -0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-307);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(3801);
    tri3_xyze(0, 1) = 1.03009e-15;
    tri3_xyze(1, 1) = -0.0595492;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4051);
    tri3_xyze(0, 2) = -0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-307);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03211e-15;
    tri3_xyze(1, 0) = -0.0635516;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4053);
    tri3_xyze(0, 1) = 1.0193e-15;
    tri3_xyze(1, 1) = -0.0681288;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4055);
    tri3_xyze(0, 2) = -0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-308);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0193e-15;
    tri3_xyze(1, 0) = -0.0681288;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4055);
    tri3_xyze(0, 1) = -0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(3805);
    tri3_xyze(0, 2) = -0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-308);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(3803);
    tri3_xyze(0, 1) = 1.03211e-15;
    tri3_xyze(1, 1) = -0.0635516;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4053);
    tri3_xyze(0, 2) = -0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-308);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0193e-15;
    tri3_xyze(1, 0) = -0.0681288;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4055);
    tri3_xyze(0, 1) = 1.03699e-15;
    tri3_xyze(1, 1) = -0.0732087;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4057);
    tri3_xyze(0, 2) = -0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-309);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03699e-15;
    tri3_xyze(1, 0) = -0.0732087;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4057);
    tri3_xyze(0, 1) = -0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(3807);
    tri3_xyze(0, 2) = -0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-309);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(3805);
    tri3_xyze(0, 1) = 1.0193e-15;
    tri3_xyze(1, 1) = -0.0681288;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4055);
    tri3_xyze(0, 2) = -0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-309);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03699e-15;
    tri3_xyze(1, 0) = -0.0732087;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4057);
    tri3_xyze(0, 1) = 1.03977e-15;
    tri3_xyze(1, 1) = -0.078711;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4059);
    tri3_xyze(0, 2) = -0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-310);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03977e-15;
    tri3_xyze(1, 0) = -0.078711;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4059);
    tri3_xyze(0, 1) = -0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(3809);
    tri3_xyze(0, 2) = -0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-310);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(3807);
    tri3_xyze(0, 1) = 1.03699e-15;
    tri3_xyze(1, 1) = -0.0732087;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4057);
    tri3_xyze(0, 2) = -0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-310);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03977e-15;
    tri3_xyze(1, 0) = -0.078711;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4059);
    tri3_xyze(0, 1) = 1.0615e-15;
    tri3_xyze(1, 1) = -0.0845492;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4061);
    tri3_xyze(0, 2) = -0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-311);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0615e-15;
    tri3_xyze(1, 0) = -0.0845492;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4061);
    tri3_xyze(0, 1) = -0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(3811);
    tri3_xyze(0, 2) = -0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-311);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(3809);
    tri3_xyze(0, 1) = 1.03977e-15;
    tri3_xyze(1, 1) = -0.078711;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4059);
    tri3_xyze(0, 2) = -0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-311);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0615e-15;
    tri3_xyze(1, 0) = -0.0845492;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4061);
    tri3_xyze(0, 1) = 1.06592e-15;
    tri3_xyze(1, 1) = -0.0906309;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4063);
    tri3_xyze(0, 2) = -0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-312);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.06592e-15;
    tri3_xyze(1, 0) = -0.0906309;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4063);
    tri3_xyze(0, 1) = -0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(3813);
    tri3_xyze(0, 2) = -0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-312);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(3811);
    tri3_xyze(0, 1) = 1.0615e-15;
    tri3_xyze(1, 1) = -0.0845492;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4061);
    tri3_xyze(0, 2) = -0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-312);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.06592e-15;
    tri3_xyze(1, 0) = -0.0906309;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4063);
    tri3_xyze(0, 1) = 1.02744e-15;
    tri3_xyze(1, 1) = -0.0968605;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4065);
    tri3_xyze(0, 2) = -0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-313);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02744e-15;
    tri3_xyze(1, 0) = -0.0968605;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4065);
    tri3_xyze(0, 1) = -0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(3815);
    tri3_xyze(0, 2) = -0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-313);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(3813);
    tri3_xyze(0, 1) = 1.06592e-15;
    tri3_xyze(1, 1) = -0.0906309;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4063);
    tri3_xyze(0, 2) = -0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-313);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02744e-15;
    tri3_xyze(1, 0) = -0.0968605;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4065);
    tri3_xyze(0, 1) = 1.05212e-15;
    tri3_xyze(1, 1) = -0.10314;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4067);
    tri3_xyze(0, 2) = -0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-314);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(3815);
    tri3_xyze(0, 1) = 1.02744e-15;
    tri3_xyze(1, 1) = -0.0968605;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4065);
    tri3_xyze(0, 2) = -0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-314);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0756e-15;
    tri3_xyze(1, 0) = -0.149606;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4089);
    tri3_xyze(0, 1) = 1.1091e-15;
    tri3_xyze(1, 1) = -0.15;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4091);
    tri3_xyze(0, 2) = -0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-326);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.1091e-15;
    tri3_xyze(1, 0) = -0.15;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4091);
    tri3_xyze(0, 1) = -0.0388229;
    tri3_xyze(1, 1) = -0.144889;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(3841);
    tri3_xyze(0, 2) = -0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-326);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.1091e-15;
    tri3_xyze(1, 0) = -0.15;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4091);
    tri3_xyze(0, 1) = 1.0756e-15;
    tri3_xyze(1, 1) = -0.149606;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4093);
    tri3_xyze(0, 2) = -0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-327);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0756e-15;
    tri3_xyze(1, 0) = -0.149606;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4093);
    tri3_xyze(0, 1) = -0.0387208;
    tri3_xyze(1, 1) = -0.144508;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(3843);
    tri3_xyze(0, 2) = -0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-327);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0388229;
    tri3_xyze(1, 0) = -0.144889;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(3841);
    tri3_xyze(0, 1) = 1.1091e-15;
    tri3_xyze(1, 1) = -0.15;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4091);
    tri3_xyze(0, 2) = -0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-327);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0756e-15;
    tri3_xyze(1, 0) = -0.149606;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4093);
    tri3_xyze(0, 1) = 1.075e-15;
    tri3_xyze(1, 1) = -0.148429;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4095);
    tri3_xyze(0, 2) = -0.0192843;
    tri3_xyze(1, 2) = -0.146479;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-328);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.075e-15;
    tri3_xyze(1, 0) = -0.148429;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4095);
    tri3_xyze(0, 1) = -0.0384163;
    tri3_xyze(1, 1) = -0.143372;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(3845);
    tri3_xyze(0, 2) = -0.0192843;
    tri3_xyze(1, 2) = -0.146479;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-328);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0387208;
    tri3_xyze(1, 0) = -0.144508;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(3843);
    tri3_xyze(0, 1) = 1.0756e-15;
    tri3_xyze(1, 1) = -0.149606;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4093);
    tri3_xyze(0, 2) = -0.0192843;
    tri3_xyze(1, 2) = -0.146479;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-328);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.075e-15;
    tri3_xyze(1, 0) = -0.148429;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4095);
    tri3_xyze(0, 1) = 1.00897e-15;
    tri3_xyze(1, 1) = -0.146489;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4097);
    tri3_xyze(0, 2) = -0.0190826;
    tri3_xyze(1, 2) = -0.144947;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-329);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00897e-15;
    tri3_xyze(1, 0) = -0.146489;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4097);
    tri3_xyze(0, 1) = -0.0379141;
    tri3_xyze(1, 1) = -0.141497;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(3847);
    tri3_xyze(0, 2) = -0.0190826;
    tri3_xyze(1, 2) = -0.144947;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-329);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0384163;
    tri3_xyze(1, 0) = -0.143372;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(3845);
    tri3_xyze(0, 1) = 1.075e-15;
    tri3_xyze(1, 1) = -0.148429;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4095);
    tri3_xyze(0, 2) = -0.0190826;
    tri3_xyze(1, 2) = -0.144947;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-329);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00897e-15;
    tri3_xyze(1, 0) = -0.146489;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4097);
    tri3_xyze(0, 1) = 1.00881e-15;
    tri3_xyze(1, 1) = -0.143815;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4099);
    tri3_xyze(0, 2) = -0.0187841;
    tri3_xyze(1, 2) = -0.142679;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-330);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00881e-15;
    tri3_xyze(1, 0) = -0.143815;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4099);
    tri3_xyze(0, 1) = -0.0372221;
    tri3_xyze(1, 1) = -0.138915;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(3849);
    tri3_xyze(0, 2) = -0.0187841;
    tri3_xyze(1, 2) = -0.142679;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-330);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0379141;
    tri3_xyze(1, 0) = -0.141497;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(3847);
    tri3_xyze(0, 1) = 1.00897e-15;
    tri3_xyze(1, 1) = -0.146489;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4097);
    tri3_xyze(0, 2) = -0.0187841;
    tri3_xyze(1, 2) = -0.142679;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-330);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00881e-15;
    tri3_xyze(1, 0) = -0.143815;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4099);
    tri3_xyze(0, 1) = 1.07097e-15;
    tri3_xyze(1, 1) = -0.140451;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4101);
    tri3_xyze(0, 2) = -0.0183934;
    tri3_xyze(1, 2) = -0.139712;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-331);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.07097e-15;
    tri3_xyze(1, 0) = -0.140451;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4101);
    tri3_xyze(0, 1) = -0.0363514;
    tri3_xyze(1, 1) = -0.135665;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3851);
    tri3_xyze(0, 2) = -0.0183934;
    tri3_xyze(1, 2) = -0.139712;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-331);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0372221;
    tri3_xyze(1, 0) = -0.138915;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(3849);
    tri3_xyze(0, 1) = 1.00881e-15;
    tri3_xyze(1, 1) = -0.143815;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4099);
    tri3_xyze(0, 2) = -0.0183934;
    tri3_xyze(1, 2) = -0.139712;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-331);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.07097e-15;
    tri3_xyze(1, 0) = -0.140451;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4101);
    tri3_xyze(0, 1) = 1.03865e-15;
    tri3_xyze(1, 1) = -0.136448;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4103);
    tri3_xyze(0, 2) = -0.0179167;
    tri3_xyze(1, 2) = -0.136091;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-332);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03865e-15;
    tri3_xyze(1, 0) = -0.136448;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4103);
    tri3_xyze(0, 1) = -0.0353155;
    tri3_xyze(1, 1) = -0.131799;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3853);
    tri3_xyze(0, 2) = -0.0179167;
    tri3_xyze(1, 2) = -0.136091;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-332);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0363514;
    tri3_xyze(1, 0) = -0.135665;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3851);
    tri3_xyze(0, 1) = 1.07097e-15;
    tri3_xyze(1, 1) = -0.140451;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4101);
    tri3_xyze(0, 2) = -0.0179167;
    tri3_xyze(1, 2) = -0.136091;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-332);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03865e-15;
    tri3_xyze(1, 0) = -0.136448;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4103);
    tri3_xyze(0, 1) = 1.03736e-15;
    tri3_xyze(1, 1) = -0.131871;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4105);
    tri3_xyze(0, 2) = -0.0173616;
    tri3_xyze(1, 2) = -0.131874;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-333);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03736e-15;
    tri3_xyze(1, 0) = -0.131871;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4105);
    tri3_xyze(0, 1) = -0.0341308;
    tri3_xyze(1, 1) = -0.127378;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3855);
    tri3_xyze(0, 2) = -0.0173616;
    tri3_xyze(1, 2) = -0.131874;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-333);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0353155;
    tri3_xyze(1, 0) = -0.131799;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3853);
    tri3_xyze(0, 1) = 1.03865e-15;
    tri3_xyze(1, 1) = -0.136448;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4103);
    tri3_xyze(0, 2) = -0.0173616;
    tri3_xyze(1, 2) = -0.131874;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-333);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03736e-15;
    tri3_xyze(1, 0) = -0.131871;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4105);
    tri3_xyze(0, 1) = 1.06407e-15;
    tri3_xyze(1, 1) = -0.126791;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4107);
    tri3_xyze(0, 2) = -0.0167367;
    tri3_xyze(1, 2) = -0.127128;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-334);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.06407e-15;
    tri3_xyze(1, 0) = -0.126791;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4107);
    tri3_xyze(0, 1) = -0.032816;
    tri3_xyze(1, 1) = -0.122471;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3857);
    tri3_xyze(0, 2) = -0.0167367;
    tri3_xyze(1, 2) = -0.127128;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-334);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0341308;
    tri3_xyze(1, 0) = -0.127378;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3855);
    tri3_xyze(0, 1) = 1.03736e-15;
    tri3_xyze(1, 1) = -0.131871;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4105);
    tri3_xyze(0, 2) = -0.0167367;
    tri3_xyze(1, 2) = -0.127128;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-334);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.06407e-15;
    tri3_xyze(1, 0) = -0.126791;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4107);
    tri3_xyze(0, 1) = 1.00743e-15;
    tri3_xyze(1, 1) = -0.121289;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4109);
    tri3_xyze(0, 2) = -0.016052;
    tri3_xyze(1, 2) = -0.121927;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-335);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00743e-15;
    tri3_xyze(1, 0) = -0.121289;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4109);
    tri3_xyze(0, 1) = -0.0313919;
    tri3_xyze(1, 1) = -0.117156;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3859);
    tri3_xyze(0, 2) = -0.016052;
    tri3_xyze(1, 2) = -0.121927;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-335);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.032816;
    tri3_xyze(1, 0) = -0.122471;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3857);
    tri3_xyze(0, 1) = 1.06407e-15;
    tri3_xyze(1, 1) = -0.126791;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4107);
    tri3_xyze(0, 2) = -0.016052;
    tri3_xyze(1, 2) = -0.121927;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-335);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00743e-15;
    tri3_xyze(1, 0) = -0.121289;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4109);
    tri3_xyze(0, 1) = 1.0327e-15;
    tri3_xyze(1, 1) = -0.115451;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4111);
    tri3_xyze(0, 2) = -0.0153182;
    tri3_xyze(1, 2) = -0.116353;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-336);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0327e-15;
    tri3_xyze(1, 0) = -0.115451;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4111);
    tri3_xyze(0, 1) = -0.0298809;
    tri3_xyze(1, 1) = -0.111517;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3861);
    tri3_xyze(0, 2) = -0.0153182;
    tri3_xyze(1, 2) = -0.116353;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-336);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0313919;
    tri3_xyze(1, 0) = -0.117156;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3859);
    tri3_xyze(0, 1) = 1.00743e-15;
    tri3_xyze(1, 1) = -0.121289;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4109);
    tri3_xyze(0, 2) = -0.0153182;
    tri3_xyze(1, 2) = -0.116353;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-336);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0327e-15;
    tri3_xyze(1, 0) = -0.115451;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4111);
    tri3_xyze(0, 1) = 1.05527e-15;
    tri3_xyze(1, 1) = -0.109369;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4113);
    tri3_xyze(0, 2) = -0.0145469;
    tri3_xyze(1, 2) = -0.110495;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-337);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05527e-15;
    tri3_xyze(1, 0) = -0.109369;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4113);
    tri3_xyze(0, 1) = -0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3863);
    tri3_xyze(0, 2) = -0.0145469;
    tri3_xyze(1, 2) = -0.110495;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-337);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0298809;
    tri3_xyze(1, 0) = -0.111517;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3861);
    tri3_xyze(0, 1) = 1.0327e-15;
    tri3_xyze(1, 1) = -0.115451;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4111);
    tri3_xyze(0, 2) = -0.0145469;
    tri3_xyze(1, 2) = -0.110495;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-337);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05527e-15;
    tri3_xyze(1, 0) = -0.109369;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4113);
    tri3_xyze(0, 1) = 1.05212e-15;
    tri3_xyze(1, 1) = -0.10314;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4115);
    tri3_xyze(0, 2) = -0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-338);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05212e-15;
    tri3_xyze(1, 0) = -0.10314;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4115);
    tri3_xyze(0, 1) = -0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3865);
    tri3_xyze(0, 2) = -0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-338);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3865);
    tri3_xyze(0, 1) = -0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3863);
    tri3_xyze(0, 2) = -0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-338);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3863);
    tri3_xyze(0, 1) = 1.05527e-15;
    tri3_xyze(1, 1) = -0.109369;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4113);
    tri3_xyze(0, 2) = -0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-338);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05212e-15;
    tri3_xyze(1, 0) = -0.10314;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4115);
    tri3_xyze(0, 1) = 1.04895e-15;
    tri3_xyze(1, 1) = -0.0968605;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4117);
    tri3_xyze(0, 2) = -0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-339);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.04895e-15;
    tri3_xyze(1, 0) = -0.0968605;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4117);
    tri3_xyze(0, 1) = -0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3867);
    tri3_xyze(0, 2) = -0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-339);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3867);
    tri3_xyze(0, 1) = -0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3865);
    tri3_xyze(0, 2) = -0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-339);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3865);
    tri3_xyze(0, 1) = 1.05212e-15;
    tri3_xyze(1, 1) = -0.10314;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4115);
    tri3_xyze(0, 2) = -0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-339);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.04895e-15;
    tri3_xyze(1, 0) = -0.0968605;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4117);
    tri3_xyze(0, 1) = 1.0458e-15;
    tri3_xyze(1, 1) = -0.0906309;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4119);
    tri3_xyze(0, 2) = -0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-340);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0458e-15;
    tri3_xyze(1, 0) = -0.0906309;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4119);
    tri3_xyze(0, 1) = -0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3869);
    tri3_xyze(0, 2) = -0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-340);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3869);
    tri3_xyze(0, 1) = -0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(3867);
    tri3_xyze(0, 2) = -0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-340);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(3867);
    tri3_xyze(0, 1) = 1.04895e-15;
    tri3_xyze(1, 1) = -0.0968605;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4117);
    tri3_xyze(0, 2) = -0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-340);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0458e-15;
    tri3_xyze(1, 0) = -0.0906309;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4119);
    tri3_xyze(0, 1) = 1.0615e-15;
    tri3_xyze(1, 1) = -0.0845492;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4121);
    tri3_xyze(0, 2) = -0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-341);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0615e-15;
    tri3_xyze(1, 0) = -0.0845492;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4121);
    tri3_xyze(0, 1) = -0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3871);
    tri3_xyze(0, 2) = -0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-341);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3871);
    tri3_xyze(0, 1) = -0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(3869);
    tri3_xyze(0, 2) = -0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-341);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(3869);
    tri3_xyze(0, 1) = 1.0458e-15;
    tri3_xyze(1, 1) = -0.0906309;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4119);
    tri3_xyze(0, 2) = -0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-341);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0615e-15;
    tri3_xyze(1, 0) = -0.0845492;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4121);
    tri3_xyze(0, 1) = 1.03977e-15;
    tri3_xyze(1, 1) = -0.078711;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4123);
    tri3_xyze(0, 2) = -0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-342);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03977e-15;
    tri3_xyze(1, 0) = -0.078711;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4123);
    tri3_xyze(0, 1) = -0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3873);
    tri3_xyze(0, 2) = -0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-342);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3873);
    tri3_xyze(0, 1) = -0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(3871);
    tri3_xyze(0, 2) = -0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-342);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(3871);
    tri3_xyze(0, 1) = 1.0615e-15;
    tri3_xyze(1, 1) = -0.0845492;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4121);
    tri3_xyze(0, 2) = -0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-342);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03977e-15;
    tri3_xyze(1, 0) = -0.078711;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4123);
    tri3_xyze(0, 1) = 1.02074e-15;
    tri3_xyze(1, 1) = -0.0732087;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4125);
    tri3_xyze(0, 2) = -0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-343);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02074e-15;
    tri3_xyze(1, 0) = -0.0732087;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4125);
    tri3_xyze(0, 1) = -0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3875);
    tri3_xyze(0, 2) = -0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-343);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3875);
    tri3_xyze(0, 1) = -0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(3873);
    tri3_xyze(0, 2) = -0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-343);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(3873);
    tri3_xyze(0, 1) = 1.03977e-15;
    tri3_xyze(1, 1) = -0.078711;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4123);
    tri3_xyze(0, 2) = -0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-343);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02074e-15;
    tri3_xyze(1, 0) = -0.0732087;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4125);
    tri3_xyze(0, 1) = 1.0193e-15;
    tri3_xyze(1, 1) = -0.0681288;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4127);
    tri3_xyze(0, 2) = -0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-344);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0193e-15;
    tri3_xyze(1, 0) = -0.0681288;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4127);
    tri3_xyze(0, 1) = -0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3877);
    tri3_xyze(0, 2) = -0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-344);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3877);
    tri3_xyze(0, 1) = -0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(3875);
    tri3_xyze(0, 2) = -0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-344);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(3875);
    tri3_xyze(0, 1) = 1.02074e-15;
    tri3_xyze(1, 1) = -0.0732087;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4125);
    tri3_xyze(0, 2) = -0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-344);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0193e-15;
    tri3_xyze(1, 0) = -0.0681288;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4127);
    tri3_xyze(0, 1) = 1.03211e-15;
    tri3_xyze(1, 1) = -0.0635516;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4129);
    tri3_xyze(0, 2) = -0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-345);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03211e-15;
    tri3_xyze(1, 0) = -0.0635516;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4129);
    tri3_xyze(0, 1) = -0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3879);
    tri3_xyze(0, 2) = -0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-345);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3879);
    tri3_xyze(0, 1) = -0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(3877);
    tri3_xyze(0, 2) = -0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-345);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(3877);
    tri3_xyze(0, 1) = 1.0193e-15;
    tri3_xyze(1, 1) = -0.0681288;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4127);
    tri3_xyze(0, 2) = -0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-345);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03211e-15;
    tri3_xyze(1, 0) = -0.0635516;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4129);
    tri3_xyze(0, 1) = 1.03009e-15;
    tri3_xyze(1, 1) = -0.0595492;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4131);
    tri3_xyze(0, 2) = -0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-346);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03009e-15;
    tri3_xyze(1, 0) = -0.0595492;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4131);
    tri3_xyze(0, 1) = -0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3881);
    tri3_xyze(0, 2) = -0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-346);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3881);
    tri3_xyze(0, 1) = -0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(3879);
    tri3_xyze(0, 2) = -0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-346);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(3879);
    tri3_xyze(0, 1) = 1.03211e-15;
    tri3_xyze(1, 1) = -0.0635516;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4129);
    tri3_xyze(0, 2) = -0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-346);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03009e-15;
    tri3_xyze(1, 0) = -0.0595492;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4131);
    tri3_xyze(0, 1) = 1.02839e-15;
    tri3_xyze(1, 1) = -0.0561847;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4133);
    tri3_xyze(0, 2) = -0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-347);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02839e-15;
    tri3_xyze(1, 0) = -0.0561847;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4133);
    tri3_xyze(0, 1) = -0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(3883);
    tri3_xyze(0, 2) = -0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-347);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(3883);
    tri3_xyze(0, 1) = -0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(3881);
    tri3_xyze(0, 2) = -0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-347);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(3881);
    tri3_xyze(0, 1) = 1.03009e-15;
    tri3_xyze(1, 1) = -0.0595492;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4131);
    tri3_xyze(0, 2) = -0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-347);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02839e-15;
    tri3_xyze(1, 0) = -0.0561847;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4133);
    tri3_xyze(0, 1) = 1.02704e-15;
    tri3_xyze(1, 1) = -0.0535112;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4135);
    tri3_xyze(0, 2) = -0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-348);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02704e-15;
    tri3_xyze(1, 0) = -0.0535112;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4135);
    tri3_xyze(0, 1) = -0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(3885);
    tri3_xyze(0, 2) = -0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-348);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(3885);
    tri3_xyze(0, 1) = -0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(3883);
    tri3_xyze(0, 2) = -0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-348);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(3883);
    tri3_xyze(0, 1) = 1.02839e-15;
    tri3_xyze(1, 1) = -0.0561847;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4133);
    tri3_xyze(0, 2) = -0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-348);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02704e-15;
    tri3_xyze(1, 0) = -0.0535112;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4135);
    tri3_xyze(0, 1) = 1.02606e-15;
    tri3_xyze(1, 1) = -0.0515708;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4137);
    tri3_xyze(0, 2) = -0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-349);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02606e-15;
    tri3_xyze(1, 0) = -0.0515708;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4137);
    tri3_xyze(0, 1) = -0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(3887);
    tri3_xyze(0, 2) = -0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-349);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(3887);
    tri3_xyze(0, 1) = -0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(3885);
    tri3_xyze(0, 2) = -0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-349);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(3885);
    tri3_xyze(0, 1) = 1.02704e-15;
    tri3_xyze(1, 1) = -0.0535112;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4135);
    tri3_xyze(0, 2) = -0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-349);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02606e-15;
    tri3_xyze(1, 0) = -0.0515708;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4137);
    tri3_xyze(0, 1) = 1.02547e-15;
    tri3_xyze(1, 1) = -0.0503943;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4139);
    tri3_xyze(0, 2) = -0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-350);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02547e-15;
    tri3_xyze(1, 0) = -0.0503943;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4139);
    tri3_xyze(0, 1) = -0.013043;
    tri3_xyze(1, 1) = -0.0486771;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(3889);
    tri3_xyze(0, 2) = -0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-350);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(3889);
    tri3_xyze(0, 1) = -0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(3887);
    tri3_xyze(0, 2) = -0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-350);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = -0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(3887);
    tri3_xyze(0, 1) = 1.02606e-15;
    tri3_xyze(1, 1) = -0.0515708;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4137);
    tri3_xyze(0, 2) = -0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-350);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.012941;
    tri3_xyze(1, 0) = -0.0482963;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4291);
    tri3_xyze(0, 1) = 0.013043;
    tri3_xyze(1, 1) = -0.0486771;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4292);
    tri3_xyze(0, 2) = 0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-351);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4292);
    tri3_xyze(0, 1) = 1.02547e-15;
    tri3_xyze(1, 1) = -0.0503943;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4042);
    tri3_xyze(0, 2) = 0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-351);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02547e-15;
    tri3_xyze(1, 0) = -0.0503943;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4042);
    tri3_xyze(0, 1) = 1.01416e-15;
    tri3_xyze(1, 1) = -0.05;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4041);
    tri3_xyze(0, 2) = 0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-351);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.01416e-15;
    tri3_xyze(1, 0) = -0.05;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4041);
    tri3_xyze(0, 1) = 0.012941;
    tri3_xyze(1, 1) = -0.0482963;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4291);
    tri3_xyze(0, 2) = 0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-351);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4389);
    tri3_xyze(0, 1) = 0.012941;
    tri3_xyze(1, 1) = -0.0482963;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4291);
    tri3_xyze(0, 2) = 0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-352);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.012941;
    tri3_xyze(1, 0) = -0.0482963;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4291);
    tri3_xyze(0, 1) = 1.01416e-15;
    tri3_xyze(1, 1) = -0.05;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4041);
    tri3_xyze(0, 2) = 0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-352);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.01416e-15;
    tri3_xyze(1, 0) = -0.05;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4041);
    tri3_xyze(0, 1) = 1.02547e-15;
    tri3_xyze(1, 1) = -0.0503943;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4139);
    tri3_xyze(0, 2) = 0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-352);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02547e-15;
    tri3_xyze(1, 0) = -0.0503943;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4139);
    tri3_xyze(0, 1) = 0.013043;
    tri3_xyze(1, 1) = -0.0486771;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4389);
    tri3_xyze(0, 2) = 0.00649599;
    tri3_xyze(1, 2) = -0.0493419;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-352);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4292);
    tri3_xyze(0, 1) = 0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.787566;
    nids.push_back(4295);
    tri3_xyze(0, 2) = 0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.790649;
    nids.push_back(-353);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.787566;
    nids.push_back(4295);
    tri3_xyze(0, 1) = 1.02606e-15;
    tri3_xyze(1, 1) = -0.0515708;
    tri3_xyze(2, 1) = 0.787566;
    nids.push_back(4045);
    tri3_xyze(0, 2) = 0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.790649;
    nids.push_back(-353);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02606e-15;
    tri3_xyze(1, 0) = -0.0515708;
    tri3_xyze(2, 0) = 0.787566;
    nids.push_back(4045);
    tri3_xyze(0, 1) = 1.02547e-15;
    tri3_xyze(1, 1) = -0.0503943;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4042);
    tri3_xyze(0, 2) = 0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.790649;
    nids.push_back(-353);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02547e-15;
    tri3_xyze(1, 0) = -0.0503943;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4042);
    tri3_xyze(0, 1) = 0.013043;
    tri3_xyze(1, 1) = -0.0486771;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4292);
    tri3_xyze(0, 2) = 0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.790649;
    nids.push_back(-353);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.787566;
    nids.push_back(4295);
    tri3_xyze(0, 1) = 0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4297);
    tri3_xyze(0, 2) = 0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-354);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4297);
    tri3_xyze(0, 1) = 1.01516e-15;
    tri3_xyze(1, 1) = -0.0535112;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4047);
    tri3_xyze(0, 2) = 0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-354);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.01516e-15;
    tri3_xyze(1, 0) = -0.0535112;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4047);
    tri3_xyze(0, 1) = 1.02606e-15;
    tri3_xyze(1, 1) = -0.0515708;
    tri3_xyze(2, 1) = 0.787566;
    nids.push_back(4045);
    tri3_xyze(0, 2) = 0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-354);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02606e-15;
    tri3_xyze(1, 0) = -0.0515708;
    tri3_xyze(2, 0) = 0.787566;
    nids.push_back(4045);
    tri3_xyze(0, 1) = 0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.787566;
    nids.push_back(4295);
    tri3_xyze(0, 2) = 0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-354);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4297);
    tri3_xyze(0, 1) = 0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4299);
    tri3_xyze(0, 2) = 0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-355);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4299);
    tri3_xyze(0, 1) = 1.02839e-15;
    tri3_xyze(1, 1) = -0.0561847;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4049);
    tri3_xyze(0, 2) = 0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-355);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02839e-15;
    tri3_xyze(1, 0) = -0.0561847;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4049);
    tri3_xyze(0, 1) = 1.01516e-15;
    tri3_xyze(1, 1) = -0.0535112;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4047);
    tri3_xyze(0, 2) = 0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-355);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.01516e-15;
    tri3_xyze(1, 0) = -0.0535112;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4047);
    tri3_xyze(0, 1) = 0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4297);
    tri3_xyze(0, 2) = 0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-355);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4299);
    tri3_xyze(0, 1) = 0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4301);
    tri3_xyze(0, 2) = 0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-356);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4301);
    tri3_xyze(0, 1) = 1.03009e-15;
    tri3_xyze(1, 1) = -0.0595492;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4051);
    tri3_xyze(0, 2) = 0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-356);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03009e-15;
    tri3_xyze(1, 0) = -0.0595492;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4051);
    tri3_xyze(0, 1) = 1.02839e-15;
    tri3_xyze(1, 1) = -0.0561847;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4049);
    tri3_xyze(0, 2) = 0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-356);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02839e-15;
    tri3_xyze(1, 0) = -0.0561847;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4049);
    tri3_xyze(0, 1) = 0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4299);
    tri3_xyze(0, 2) = 0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-356);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4301);
    tri3_xyze(0, 1) = 0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4303);
    tri3_xyze(0, 2) = 0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-357);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4303);
    tri3_xyze(0, 1) = 1.03211e-15;
    tri3_xyze(1, 1) = -0.0635516;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4053);
    tri3_xyze(0, 2) = 0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-357);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03211e-15;
    tri3_xyze(1, 0) = -0.0635516;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4053);
    tri3_xyze(0, 1) = 1.03009e-15;
    tri3_xyze(1, 1) = -0.0595492;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4051);
    tri3_xyze(0, 2) = 0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-357);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03009e-15;
    tri3_xyze(1, 0) = -0.0595492;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4051);
    tri3_xyze(0, 1) = 0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4301);
    tri3_xyze(0, 2) = 0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-357);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4303);
    tri3_xyze(0, 1) = 0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4305);
    tri3_xyze(0, 2) = 0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-358);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4305);
    tri3_xyze(0, 1) = 1.0193e-15;
    tri3_xyze(1, 1) = -0.0681288;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4055);
    tri3_xyze(0, 2) = 0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-358);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0193e-15;
    tri3_xyze(1, 0) = -0.0681288;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4055);
    tri3_xyze(0, 1) = 1.03211e-15;
    tri3_xyze(1, 1) = -0.0635516;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4053);
    tri3_xyze(0, 2) = 0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-358);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03211e-15;
    tri3_xyze(1, 0) = -0.0635516;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4053);
    tri3_xyze(0, 1) = 0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4303);
    tri3_xyze(0, 2) = 0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-358);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4305);
    tri3_xyze(0, 1) = 0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4307);
    tri3_xyze(0, 2) = 0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-359);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4307);
    tri3_xyze(0, 1) = 1.03699e-15;
    tri3_xyze(1, 1) = -0.0732087;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4057);
    tri3_xyze(0, 2) = 0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-359);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03699e-15;
    tri3_xyze(1, 0) = -0.0732087;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4057);
    tri3_xyze(0, 1) = 1.0193e-15;
    tri3_xyze(1, 1) = -0.0681288;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4055);
    tri3_xyze(0, 2) = 0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-359);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0193e-15;
    tri3_xyze(1, 0) = -0.0681288;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4055);
    tri3_xyze(0, 1) = 0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4305);
    tri3_xyze(0, 2) = 0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-359);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4307);
    tri3_xyze(0, 1) = 0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4309);
    tri3_xyze(0, 2) = 0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-360);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4309);
    tri3_xyze(0, 1) = 1.03977e-15;
    tri3_xyze(1, 1) = -0.078711;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4059);
    tri3_xyze(0, 2) = 0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-360);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03977e-15;
    tri3_xyze(1, 0) = -0.078711;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4059);
    tri3_xyze(0, 1) = 1.03699e-15;
    tri3_xyze(1, 1) = -0.0732087;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4057);
    tri3_xyze(0, 2) = 0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-360);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03699e-15;
    tri3_xyze(1, 0) = -0.0732087;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4057);
    tri3_xyze(0, 1) = 0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4307);
    tri3_xyze(0, 2) = 0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-360);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4309);
    tri3_xyze(0, 1) = 0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4311);
    tri3_xyze(0, 2) = 0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-361);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4311);
    tri3_xyze(0, 1) = 1.0615e-15;
    tri3_xyze(1, 1) = -0.0845492;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4061);
    tri3_xyze(0, 2) = 0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-361);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0615e-15;
    tri3_xyze(1, 0) = -0.0845492;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4061);
    tri3_xyze(0, 1) = 1.03977e-15;
    tri3_xyze(1, 1) = -0.078711;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4059);
    tri3_xyze(0, 2) = 0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-361);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03977e-15;
    tri3_xyze(1, 0) = -0.078711;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4059);
    tri3_xyze(0, 1) = 0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4309);
    tri3_xyze(0, 2) = 0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-361);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4311);
    tri3_xyze(0, 1) = 0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4313);
    tri3_xyze(0, 2) = 0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-362);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4313);
    tri3_xyze(0, 1) = 1.06592e-15;
    tri3_xyze(1, 1) = -0.0906309;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4063);
    tri3_xyze(0, 2) = 0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-362);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.06592e-15;
    tri3_xyze(1, 0) = -0.0906309;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4063);
    tri3_xyze(0, 1) = 1.0615e-15;
    tri3_xyze(1, 1) = -0.0845492;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4061);
    tri3_xyze(0, 2) = 0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-362);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0615e-15;
    tri3_xyze(1, 0) = -0.0845492;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4061);
    tri3_xyze(0, 1) = 0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4311);
    tri3_xyze(0, 2) = 0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-362);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4313);
    tri3_xyze(0, 1) = 0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4315);
    tri3_xyze(0, 2) = 0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-363);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4315);
    tri3_xyze(0, 1) = 1.02744e-15;
    tri3_xyze(1, 1) = -0.0968605;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4065);
    tri3_xyze(0, 2) = 0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-363);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02744e-15;
    tri3_xyze(1, 0) = -0.0968605;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4065);
    tri3_xyze(0, 1) = 1.06592e-15;
    tri3_xyze(1, 1) = -0.0906309;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4063);
    tri3_xyze(0, 2) = 0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-363);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.06592e-15;
    tri3_xyze(1, 0) = -0.0906309;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4063);
    tri3_xyze(0, 1) = 0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4313);
    tri3_xyze(0, 2) = 0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-363);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4315);
    tri3_xyze(0, 1) = 0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4317);
    tri3_xyze(0, 2) = 0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-364);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4317);
    tri3_xyze(0, 1) = 1.05212e-15;
    tri3_xyze(1, 1) = -0.10314;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4067);
    tri3_xyze(0, 2) = 0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-364);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05212e-15;
    tri3_xyze(1, 0) = -0.10314;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4067);
    tri3_xyze(0, 1) = 1.02744e-15;
    tri3_xyze(1, 1) = -0.0968605;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4065);
    tri3_xyze(0, 2) = 0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-364);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02744e-15;
    tri3_xyze(1, 0) = -0.0968605;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4065);
    tri3_xyze(0, 1) = 0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4315);
    tri3_xyze(0, 2) = 0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-364);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4317);
    tri3_xyze(0, 1) = 0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4319);
    tri3_xyze(0, 2) = 0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-365);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05212e-15;
    tri3_xyze(1, 0) = -0.10314;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4067);
    tri3_xyze(0, 1) = 0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4317);
    tri3_xyze(0, 2) = 0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-365);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0387208;
    tri3_xyze(1, 0) = -0.144508;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4339);
    tri3_xyze(0, 1) = 0.0388229;
    tri3_xyze(1, 1) = -0.144889;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4341);
    tri3_xyze(0, 2) = 0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-376);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0388229;
    tri3_xyze(1, 0) = -0.144889;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4341);
    tri3_xyze(0, 1) = 1.1091e-15;
    tri3_xyze(1, 1) = -0.15;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4091);
    tri3_xyze(0, 2) = 0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-376);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.1091e-15;
    tri3_xyze(1, 0) = -0.15;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4091);
    tri3_xyze(0, 1) = 1.0756e-15;
    tri3_xyze(1, 1) = -0.149606;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4089);
    tri3_xyze(0, 2) = 0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-376);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0388229;
    tri3_xyze(1, 0) = -0.144889;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4341);
    tri3_xyze(0, 1) = 0.0387208;
    tri3_xyze(1, 1) = -0.144508;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4343);
    tri3_xyze(0, 2) = 0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-377);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0387208;
    tri3_xyze(1, 0) = -0.144508;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4343);
    tri3_xyze(0, 1) = 1.0756e-15;
    tri3_xyze(1, 1) = -0.149606;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4093);
    tri3_xyze(0, 2) = 0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-377);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0756e-15;
    tri3_xyze(1, 0) = -0.149606;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4093);
    tri3_xyze(0, 1) = 1.1091e-15;
    tri3_xyze(1, 1) = -0.15;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4091);
    tri3_xyze(0, 2) = 0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-377);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.1091e-15;
    tri3_xyze(1, 0) = -0.15;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4091);
    tri3_xyze(0, 1) = 0.0388229;
    tri3_xyze(1, 1) = -0.144889;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4341);
    tri3_xyze(0, 2) = 0.0193859;
    tri3_xyze(1, 2) = -0.147251;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-377);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0387208;
    tri3_xyze(1, 0) = -0.144508;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4343);
    tri3_xyze(0, 1) = 0.0384163;
    tri3_xyze(1, 1) = -0.143372;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4345);
    tri3_xyze(0, 2) = 0.0192843;
    tri3_xyze(1, 2) = -0.146479;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-378);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0384163;
    tri3_xyze(1, 0) = -0.143372;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4345);
    tri3_xyze(0, 1) = 1.075e-15;
    tri3_xyze(1, 1) = -0.148429;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4095);
    tri3_xyze(0, 2) = 0.0192843;
    tri3_xyze(1, 2) = -0.146479;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-378);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.075e-15;
    tri3_xyze(1, 0) = -0.148429;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4095);
    tri3_xyze(0, 1) = 1.0756e-15;
    tri3_xyze(1, 1) = -0.149606;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4093);
    tri3_xyze(0, 2) = 0.0192843;
    tri3_xyze(1, 2) = -0.146479;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-378);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0756e-15;
    tri3_xyze(1, 0) = -0.149606;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4093);
    tri3_xyze(0, 1) = 0.0387208;
    tri3_xyze(1, 1) = -0.144508;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4343);
    tri3_xyze(0, 2) = 0.0192843;
    tri3_xyze(1, 2) = -0.146479;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-378);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0384163;
    tri3_xyze(1, 0) = -0.143372;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4345);
    tri3_xyze(0, 1) = 0.0379141;
    tri3_xyze(1, 1) = -0.141497;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4347);
    tri3_xyze(0, 2) = 0.0190826;
    tri3_xyze(1, 2) = -0.144947;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-379);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0379141;
    tri3_xyze(1, 0) = -0.141497;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4347);
    tri3_xyze(0, 1) = 1.00897e-15;
    tri3_xyze(1, 1) = -0.146489;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4097);
    tri3_xyze(0, 2) = 0.0190826;
    tri3_xyze(1, 2) = -0.144947;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-379);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00897e-15;
    tri3_xyze(1, 0) = -0.146489;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4097);
    tri3_xyze(0, 1) = 1.075e-15;
    tri3_xyze(1, 1) = -0.148429;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4095);
    tri3_xyze(0, 2) = 0.0190826;
    tri3_xyze(1, 2) = -0.144947;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-379);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.075e-15;
    tri3_xyze(1, 0) = -0.148429;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4095);
    tri3_xyze(0, 1) = 0.0384163;
    tri3_xyze(1, 1) = -0.143372;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4345);
    tri3_xyze(0, 2) = 0.0190826;
    tri3_xyze(1, 2) = -0.144947;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-379);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0379141;
    tri3_xyze(1, 0) = -0.141497;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4347);
    tri3_xyze(0, 1) = 0.0372221;
    tri3_xyze(1, 1) = -0.138915;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4349);
    tri3_xyze(0, 2) = 0.0187841;
    tri3_xyze(1, 2) = -0.142679;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-380);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0372221;
    tri3_xyze(1, 0) = -0.138915;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4349);
    tri3_xyze(0, 1) = 1.00881e-15;
    tri3_xyze(1, 1) = -0.143815;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4099);
    tri3_xyze(0, 2) = 0.0187841;
    tri3_xyze(1, 2) = -0.142679;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-380);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00881e-15;
    tri3_xyze(1, 0) = -0.143815;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4099);
    tri3_xyze(0, 1) = 1.00897e-15;
    tri3_xyze(1, 1) = -0.146489;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4097);
    tri3_xyze(0, 2) = 0.0187841;
    tri3_xyze(1, 2) = -0.142679;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-380);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00897e-15;
    tri3_xyze(1, 0) = -0.146489;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4097);
    tri3_xyze(0, 1) = 0.0379141;
    tri3_xyze(1, 1) = -0.141497;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4347);
    tri3_xyze(0, 2) = 0.0187841;
    tri3_xyze(1, 2) = -0.142679;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-380);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0372221;
    tri3_xyze(1, 0) = -0.138915;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4349);
    tri3_xyze(0, 1) = 0.0363514;
    tri3_xyze(1, 1) = -0.135665;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4351);
    tri3_xyze(0, 2) = 0.0183934;
    tri3_xyze(1, 2) = -0.139712;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-381);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0363514;
    tri3_xyze(1, 0) = -0.135665;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4351);
    tri3_xyze(0, 1) = 1.07097e-15;
    tri3_xyze(1, 1) = -0.140451;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4101);
    tri3_xyze(0, 2) = 0.0183934;
    tri3_xyze(1, 2) = -0.139712;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-381);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.07097e-15;
    tri3_xyze(1, 0) = -0.140451;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4101);
    tri3_xyze(0, 1) = 1.00881e-15;
    tri3_xyze(1, 1) = -0.143815;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4099);
    tri3_xyze(0, 2) = 0.0183934;
    tri3_xyze(1, 2) = -0.139712;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-381);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00881e-15;
    tri3_xyze(1, 0) = -0.143815;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4099);
    tri3_xyze(0, 1) = 0.0372221;
    tri3_xyze(1, 1) = -0.138915;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4349);
    tri3_xyze(0, 2) = 0.0183934;
    tri3_xyze(1, 2) = -0.139712;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-381);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0363514;
    tri3_xyze(1, 0) = -0.135665;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4351);
    tri3_xyze(0, 1) = 0.0353155;
    tri3_xyze(1, 1) = -0.131799;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4353);
    tri3_xyze(0, 2) = 0.0179167;
    tri3_xyze(1, 2) = -0.136091;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-382);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0353155;
    tri3_xyze(1, 0) = -0.131799;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4353);
    tri3_xyze(0, 1) = 1.03865e-15;
    tri3_xyze(1, 1) = -0.136448;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4103);
    tri3_xyze(0, 2) = 0.0179167;
    tri3_xyze(1, 2) = -0.136091;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-382);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03865e-15;
    tri3_xyze(1, 0) = -0.136448;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4103);
    tri3_xyze(0, 1) = 1.07097e-15;
    tri3_xyze(1, 1) = -0.140451;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4101);
    tri3_xyze(0, 2) = 0.0179167;
    tri3_xyze(1, 2) = -0.136091;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-382);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.07097e-15;
    tri3_xyze(1, 0) = -0.140451;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4101);
    tri3_xyze(0, 1) = 0.0363514;
    tri3_xyze(1, 1) = -0.135665;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4351);
    tri3_xyze(0, 2) = 0.0179167;
    tri3_xyze(1, 2) = -0.136091;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-382);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0353155;
    tri3_xyze(1, 0) = -0.131799;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4353);
    tri3_xyze(0, 1) = 0.0341308;
    tri3_xyze(1, 1) = -0.127378;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4355);
    tri3_xyze(0, 2) = 0.0173616;
    tri3_xyze(1, 2) = -0.131874;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-383);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0341308;
    tri3_xyze(1, 0) = -0.127378;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4355);
    tri3_xyze(0, 1) = 1.03736e-15;
    tri3_xyze(1, 1) = -0.131871;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4105);
    tri3_xyze(0, 2) = 0.0173616;
    tri3_xyze(1, 2) = -0.131874;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-383);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03736e-15;
    tri3_xyze(1, 0) = -0.131871;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4105);
    tri3_xyze(0, 1) = 1.03865e-15;
    tri3_xyze(1, 1) = -0.136448;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4103);
    tri3_xyze(0, 2) = 0.0173616;
    tri3_xyze(1, 2) = -0.131874;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-383);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03865e-15;
    tri3_xyze(1, 0) = -0.136448;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4103);
    tri3_xyze(0, 1) = 0.0353155;
    tri3_xyze(1, 1) = -0.131799;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4353);
    tri3_xyze(0, 2) = 0.0173616;
    tri3_xyze(1, 2) = -0.131874;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-383);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0341308;
    tri3_xyze(1, 0) = -0.127378;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4355);
    tri3_xyze(0, 1) = 0.032816;
    tri3_xyze(1, 1) = -0.122471;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4357);
    tri3_xyze(0, 2) = 0.0167367;
    tri3_xyze(1, 2) = -0.127128;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-384);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.032816;
    tri3_xyze(1, 0) = -0.122471;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4357);
    tri3_xyze(0, 1) = 1.06407e-15;
    tri3_xyze(1, 1) = -0.126791;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4107);
    tri3_xyze(0, 2) = 0.0167367;
    tri3_xyze(1, 2) = -0.127128;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-384);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.06407e-15;
    tri3_xyze(1, 0) = -0.126791;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4107);
    tri3_xyze(0, 1) = 1.03736e-15;
    tri3_xyze(1, 1) = -0.131871;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4105);
    tri3_xyze(0, 2) = 0.0167367;
    tri3_xyze(1, 2) = -0.127128;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-384);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03736e-15;
    tri3_xyze(1, 0) = -0.131871;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4105);
    tri3_xyze(0, 1) = 0.0341308;
    tri3_xyze(1, 1) = -0.127378;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4355);
    tri3_xyze(0, 2) = 0.0167367;
    tri3_xyze(1, 2) = -0.127128;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-384);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.032816;
    tri3_xyze(1, 0) = -0.122471;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4357);
    tri3_xyze(0, 1) = 0.0313919;
    tri3_xyze(1, 1) = -0.117156;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4359);
    tri3_xyze(0, 2) = 0.016052;
    tri3_xyze(1, 2) = -0.121927;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-385);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0313919;
    tri3_xyze(1, 0) = -0.117156;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4359);
    tri3_xyze(0, 1) = 1.00743e-15;
    tri3_xyze(1, 1) = -0.121289;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4109);
    tri3_xyze(0, 2) = 0.016052;
    tri3_xyze(1, 2) = -0.121927;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-385);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00743e-15;
    tri3_xyze(1, 0) = -0.121289;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4109);
    tri3_xyze(0, 1) = 1.06407e-15;
    tri3_xyze(1, 1) = -0.126791;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4107);
    tri3_xyze(0, 2) = 0.016052;
    tri3_xyze(1, 2) = -0.121927;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-385);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.06407e-15;
    tri3_xyze(1, 0) = -0.126791;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4107);
    tri3_xyze(0, 1) = 0.032816;
    tri3_xyze(1, 1) = -0.122471;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4357);
    tri3_xyze(0, 2) = 0.016052;
    tri3_xyze(1, 2) = -0.121927;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-385);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0313919;
    tri3_xyze(1, 0) = -0.117156;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4359);
    tri3_xyze(0, 1) = 0.0298809;
    tri3_xyze(1, 1) = -0.111517;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4361);
    tri3_xyze(0, 2) = 0.0153182;
    tri3_xyze(1, 2) = -0.116353;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-386);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0298809;
    tri3_xyze(1, 0) = -0.111517;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4361);
    tri3_xyze(0, 1) = 1.0327e-15;
    tri3_xyze(1, 1) = -0.115451;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4111);
    tri3_xyze(0, 2) = 0.0153182;
    tri3_xyze(1, 2) = -0.116353;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-386);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0327e-15;
    tri3_xyze(1, 0) = -0.115451;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4111);
    tri3_xyze(0, 1) = 1.00743e-15;
    tri3_xyze(1, 1) = -0.121289;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4109);
    tri3_xyze(0, 2) = 0.0153182;
    tri3_xyze(1, 2) = -0.116353;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-386);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.00743e-15;
    tri3_xyze(1, 0) = -0.121289;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4109);
    tri3_xyze(0, 1) = 0.0313919;
    tri3_xyze(1, 1) = -0.117156;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4359);
    tri3_xyze(0, 2) = 0.0153182;
    tri3_xyze(1, 2) = -0.116353;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-386);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0298809;
    tri3_xyze(1, 0) = -0.111517;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4361);
    tri3_xyze(0, 1) = 0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4363);
    tri3_xyze(0, 2) = 0.0145469;
    tri3_xyze(1, 2) = -0.110495;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-387);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4363);
    tri3_xyze(0, 1) = 1.05527e-15;
    tri3_xyze(1, 1) = -0.109369;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4113);
    tri3_xyze(0, 2) = 0.0145469;
    tri3_xyze(1, 2) = -0.110495;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-387);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05527e-15;
    tri3_xyze(1, 0) = -0.109369;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4113);
    tri3_xyze(0, 1) = 1.0327e-15;
    tri3_xyze(1, 1) = -0.115451;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4111);
    tri3_xyze(0, 2) = 0.0145469;
    tri3_xyze(1, 2) = -0.110495;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-387);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0327e-15;
    tri3_xyze(1, 0) = -0.115451;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4111);
    tri3_xyze(0, 1) = 0.0298809;
    tri3_xyze(1, 1) = -0.111517;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4361);
    tri3_xyze(0, 2) = 0.0145469;
    tri3_xyze(1, 2) = -0.110495;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-387);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4363);
    tri3_xyze(0, 1) = 0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4365);
    tri3_xyze(0, 2) = 0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-388);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4365);
    tri3_xyze(0, 1) = 1.05212e-15;
    tri3_xyze(1, 1) = -0.10314;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4115);
    tri3_xyze(0, 2) = 0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-388);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05212e-15;
    tri3_xyze(1, 0) = -0.10314;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4115);
    tri3_xyze(0, 1) = 1.05527e-15;
    tri3_xyze(1, 1) = -0.109369;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4113);
    tri3_xyze(0, 2) = 0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-388);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05527e-15;
    tri3_xyze(1, 0) = -0.109369;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4113);
    tri3_xyze(0, 1) = 0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4363);
    tri3_xyze(0, 2) = 0.0137503;
    tri3_xyze(1, 2) = -0.104444;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-388);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4365);
    tri3_xyze(0, 1) = 0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4367);
    tri3_xyze(0, 2) = 0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-389);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4367);
    tri3_xyze(0, 1) = 1.04895e-15;
    tri3_xyze(1, 1) = -0.0968605;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4117);
    tri3_xyze(0, 2) = 0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-389);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.04895e-15;
    tri3_xyze(1, 0) = -0.0968605;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4117);
    tri3_xyze(0, 1) = 1.05212e-15;
    tri3_xyze(1, 1) = -0.10314;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4115);
    tri3_xyze(0, 2) = 0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-389);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.05212e-15;
    tri3_xyze(1, 0) = -0.10314;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4115);
    tri3_xyze(0, 1) = 0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4365);
    tri3_xyze(0, 2) = 0.012941;
    tri3_xyze(1, 2) = -0.0982963;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-389);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4367);
    tri3_xyze(0, 1) = 0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4369);
    tri3_xyze(0, 2) = 0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-390);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4369);
    tri3_xyze(0, 1) = 1.0458e-15;
    tri3_xyze(1, 1) = -0.0906309;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4119);
    tri3_xyze(0, 2) = 0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-390);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0458e-15;
    tri3_xyze(1, 0) = -0.0906309;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4119);
    tri3_xyze(0, 1) = 1.04895e-15;
    tri3_xyze(1, 1) = -0.0968605;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4117);
    tri3_xyze(0, 2) = 0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-390);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.04895e-15;
    tri3_xyze(1, 0) = -0.0968605;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4117);
    tri3_xyze(0, 1) = 0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4367);
    tri3_xyze(0, 2) = 0.0121316;
    tri3_xyze(1, 2) = -0.0921486;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-390);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4369);
    tri3_xyze(0, 1) = 0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4371);
    tri3_xyze(0, 2) = 0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-391);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4371);
    tri3_xyze(0, 1) = 1.0615e-15;
    tri3_xyze(1, 1) = -0.0845492;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4121);
    tri3_xyze(0, 2) = 0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-391);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0615e-15;
    tri3_xyze(1, 0) = -0.0845492;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4121);
    tri3_xyze(0, 1) = 1.0458e-15;
    tri3_xyze(1, 1) = -0.0906309;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4119);
    tri3_xyze(0, 2) = 0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-391);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0458e-15;
    tri3_xyze(1, 0) = -0.0906309;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4119);
    tri3_xyze(0, 1) = 0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4369);
    tri3_xyze(0, 2) = 0.011335;
    tri3_xyze(1, 2) = -0.0860978;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-391);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4371);
    tri3_xyze(0, 1) = 0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4373);
    tri3_xyze(0, 2) = 0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-392);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4373);
    tri3_xyze(0, 1) = 1.03977e-15;
    tri3_xyze(1, 1) = -0.078711;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4123);
    tri3_xyze(0, 2) = 0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-392);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03977e-15;
    tri3_xyze(1, 0) = -0.078711;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4123);
    tri3_xyze(0, 1) = 1.0615e-15;
    tri3_xyze(1, 1) = -0.0845492;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4121);
    tri3_xyze(0, 2) = 0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-392);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0615e-15;
    tri3_xyze(1, 0) = -0.0845492;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4121);
    tri3_xyze(0, 1) = 0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4371);
    tri3_xyze(0, 2) = 0.0105637;
    tri3_xyze(1, 2) = -0.0802394;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-392);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4373);
    tri3_xyze(0, 1) = 0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4375);
    tri3_xyze(0, 2) = 0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-393);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4375);
    tri3_xyze(0, 1) = 1.02074e-15;
    tri3_xyze(1, 1) = -0.0732087;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4125);
    tri3_xyze(0, 2) = 0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-393);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02074e-15;
    tri3_xyze(1, 0) = -0.0732087;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4125);
    tri3_xyze(0, 1) = 1.03977e-15;
    tri3_xyze(1, 1) = -0.078711;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4123);
    tri3_xyze(0, 2) = 0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-393);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03977e-15;
    tri3_xyze(1, 0) = -0.078711;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4123);
    tri3_xyze(0, 1) = 0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4373);
    tri3_xyze(0, 2) = 0.00982993;
    tri3_xyze(1, 2) = -0.0746657;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-393);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4375);
    tri3_xyze(0, 1) = 0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4377);
    tri3_xyze(0, 2) = 0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-394);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4377);
    tri3_xyze(0, 1) = 1.0193e-15;
    tri3_xyze(1, 1) = -0.0681288;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4127);
    tri3_xyze(0, 2) = 0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-394);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0193e-15;
    tri3_xyze(1, 0) = -0.0681288;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4127);
    tri3_xyze(0, 1) = 1.02074e-15;
    tri3_xyze(1, 1) = -0.0732087;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4125);
    tri3_xyze(0, 2) = 0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-394);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02074e-15;
    tri3_xyze(1, 0) = -0.0732087;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4125);
    tri3_xyze(0, 1) = 0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4375);
    tri3_xyze(0, 2) = 0.00914521;
    tri3_xyze(1, 2) = -0.0694647;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-394);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4377);
    tri3_xyze(0, 1) = 0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4379);
    tri3_xyze(0, 2) = 0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-395);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4379);
    tri3_xyze(0, 1) = 1.03211e-15;
    tri3_xyze(1, 1) = -0.0635516;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4129);
    tri3_xyze(0, 2) = 0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-395);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03211e-15;
    tri3_xyze(1, 0) = -0.0635516;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4129);
    tri3_xyze(0, 1) = 1.0193e-15;
    tri3_xyze(1, 1) = -0.0681288;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4127);
    tri3_xyze(0, 2) = 0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-395);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.0193e-15;
    tri3_xyze(1, 0) = -0.0681288;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4127);
    tri3_xyze(0, 1) = 0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4377);
    tri3_xyze(0, 2) = 0.00852035;
    tri3_xyze(1, 2) = -0.0647185;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-395);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4379);
    tri3_xyze(0, 1) = 0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4381);
    tri3_xyze(0, 2) = 0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-396);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4381);
    tri3_xyze(0, 1) = 1.03009e-15;
    tri3_xyze(1, 1) = -0.0595492;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4131);
    tri3_xyze(0, 2) = 0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-396);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03009e-15;
    tri3_xyze(1, 0) = -0.0595492;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4131);
    tri3_xyze(0, 1) = 1.03211e-15;
    tri3_xyze(1, 1) = -0.0635516;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4129);
    tri3_xyze(0, 2) = 0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-396);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03211e-15;
    tri3_xyze(1, 0) = -0.0635516;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4129);
    tri3_xyze(0, 1) = 0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4379);
    tri3_xyze(0, 2) = 0.0079652;
    tri3_xyze(1, 2) = -0.0605017;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-396);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4381);
    tri3_xyze(0, 1) = 0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4383);
    tri3_xyze(0, 2) = 0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-397);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4383);
    tri3_xyze(0, 1) = 1.02839e-15;
    tri3_xyze(1, 1) = -0.0561847;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4133);
    tri3_xyze(0, 2) = 0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-397);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02839e-15;
    tri3_xyze(1, 0) = -0.0561847;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4133);
    tri3_xyze(0, 1) = 1.03009e-15;
    tri3_xyze(1, 1) = -0.0595492;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4131);
    tri3_xyze(0, 2) = 0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-397);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.03009e-15;
    tri3_xyze(1, 0) = -0.0595492;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4131);
    tri3_xyze(0, 1) = 0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4381);
    tri3_xyze(0, 2) = 0.00748853;
    tri3_xyze(1, 2) = -0.056881;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-397);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4383);
    tri3_xyze(0, 1) = 0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4385);
    tri3_xyze(0, 2) = 0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-398);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4385);
    tri3_xyze(0, 1) = 1.02704e-15;
    tri3_xyze(1, 1) = -0.0535112;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4135);
    tri3_xyze(0, 2) = 0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-398);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02704e-15;
    tri3_xyze(1, 0) = -0.0535112;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4135);
    tri3_xyze(0, 1) = 1.02839e-15;
    tri3_xyze(1, 1) = -0.0561847;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4133);
    tri3_xyze(0, 2) = 0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-398);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02839e-15;
    tri3_xyze(1, 0) = -0.0561847;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4133);
    tri3_xyze(0, 1) = 0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4383);
    tri3_xyze(0, 2) = 0.00709784;
    tri3_xyze(1, 2) = -0.0539135;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-398);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4385);
    tri3_xyze(0, 1) = 0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4387);
    tri3_xyze(0, 2) = 0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-399);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4387);
    tri3_xyze(0, 1) = 1.02606e-15;
    tri3_xyze(1, 1) = -0.0515708;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4137);
    tri3_xyze(0, 2) = 0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-399);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02606e-15;
    tri3_xyze(1, 0) = -0.0515708;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4137);
    tri3_xyze(0, 1) = 1.02704e-15;
    tri3_xyze(1, 1) = -0.0535112;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4135);
    tri3_xyze(0, 2) = 0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-399);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02704e-15;
    tri3_xyze(1, 0) = -0.0535112;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4135);
    tri3_xyze(0, 1) = 0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4385);
    tri3_xyze(0, 2) = 0.00679931;
    tri3_xyze(1, 2) = -0.0516459;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-399);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4387);
    tri3_xyze(0, 1) = 0.013043;
    tri3_xyze(1, 1) = -0.0486771;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4389);
    tri3_xyze(0, 2) = 0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-400);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4389);
    tri3_xyze(0, 1) = 1.02547e-15;
    tri3_xyze(1, 1) = -0.0503943;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4139);
    tri3_xyze(0, 2) = 0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-400);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02547e-15;
    tri3_xyze(1, 0) = -0.0503943;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4139);
    tri3_xyze(0, 1) = 1.02606e-15;
    tri3_xyze(1, 1) = -0.0515708;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4137);
    tri3_xyze(0, 2) = 0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-400);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 1.02606e-15;
    tri3_xyze(1, 0) = -0.0515708;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4137);
    tri3_xyze(0, 1) = 0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4387);
    tri3_xyze(0, 2) = 0.00659763;
    tri3_xyze(1, 2) = -0.050114;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-400);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.025;
    tri3_xyze(1, 0) = -0.0433013;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4541);
    tri3_xyze(0, 1) = 0.0251971;
    tri3_xyze(1, 1) = -0.0436427;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4542);
    tri3_xyze(0, 2) = 0.0190453;
    tri3_xyze(1, 2) = -0.0459793;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-401);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4292);
    tri3_xyze(0, 1) = 0.012941;
    tri3_xyze(1, 1) = -0.0482963;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4291);
    tri3_xyze(0, 2) = 0.0190453;
    tri3_xyze(1, 2) = -0.0459793;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-401);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.012941;
    tri3_xyze(1, 0) = -0.0482963;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4291);
    tri3_xyze(0, 1) = 0.025;
    tri3_xyze(1, 1) = -0.0433013;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4541);
    tri3_xyze(0, 2) = 0.0190453;
    tri3_xyze(1, 2) = -0.0459793;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-401);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0251971;
    tri3_xyze(1, 0) = -0.0436427;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4639);
    tri3_xyze(0, 1) = 0.025;
    tri3_xyze(1, 1) = -0.0433013;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4541);
    tri3_xyze(0, 2) = 0.0190453;
    tri3_xyze(1, 2) = -0.0459793;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-402);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.025;
    tri3_xyze(1, 0) = -0.0433013;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4541);
    tri3_xyze(0, 1) = 0.012941;
    tri3_xyze(1, 1) = -0.0482963;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4291);
    tri3_xyze(0, 2) = 0.0190453;
    tri3_xyze(1, 2) = -0.0459793;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-402);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.012941;
    tri3_xyze(1, 0) = -0.0482963;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4291);
    tri3_xyze(0, 1) = 0.013043;
    tri3_xyze(1, 1) = -0.0486771;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4389);
    tri3_xyze(0, 2) = 0.0190453;
    tri3_xyze(1, 2) = -0.0459793;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-402);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4389);
    tri3_xyze(0, 1) = 0.0251971;
    tri3_xyze(1, 1) = -0.0436427;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4639);
    tri3_xyze(0, 2) = 0.0190453;
    tri3_xyze(1, 2) = -0.0459793;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-402);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0267556;
    tri3_xyze(1, 0) = -0.046342;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4547);
    tri3_xyze(0, 1) = 0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4297);
    tri3_xyze(0, 2) = 0.0199346;
    tri3_xyze(1, 2) = -0.0481263;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-404);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4297);
    tri3_xyze(0, 1) = 0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.787566;
    nids.push_back(4295);
    tri3_xyze(0, 2) = 0.0199346;
    tri3_xyze(1, 2) = -0.0481263;
    tri3_xyze(2, 2) = 0.78458;
    nids.push_back(-404);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0267556;
    tri3_xyze(1, 0) = -0.046342;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4547);
    tri3_xyze(0, 1) = 0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4549);
    tri3_xyze(0, 2) = 0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-405);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0280923;
    tri3_xyze(1, 0) = -0.0486573;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4549);
    tri3_xyze(0, 1) = 0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4299);
    tri3_xyze(0, 2) = 0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-405);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4299);
    tri3_xyze(0, 1) = 0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4297);
    tri3_xyze(0, 2) = 0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-405);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.781594;
    nids.push_back(4297);
    tri3_xyze(0, 1) = 0.0267556;
    tri3_xyze(1, 1) = -0.046342;
    tri3_xyze(2, 1) = 0.781594;
    nids.push_back(4547);
    tri3_xyze(0, 2) = 0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.778753;
    nids.push_back(-405);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0280923;
    tri3_xyze(1, 0) = -0.0486573;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4549);
    tri3_xyze(0, 1) = 0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4551);
    tri3_xyze(0, 2) = 0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-406);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4551);
    tri3_xyze(0, 1) = 0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4301);
    tri3_xyze(0, 2) = 0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-406);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4301);
    tri3_xyze(0, 1) = 0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4299);
    tri3_xyze(0, 2) = 0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-406);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.775912;
    nids.push_back(4299);
    tri3_xyze(0, 1) = 0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4549);
    tri3_xyze(0, 2) = 0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-406);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4551);
    tri3_xyze(0, 1) = 0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4553);
    tri3_xyze(0, 2) = 0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-407);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4553);
    tri3_xyze(0, 1) = 0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4303);
    tri3_xyze(0, 2) = 0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-407);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4303);
    tri3_xyze(0, 1) = 0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4301);
    tri3_xyze(0, 2) = 0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-407);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4301);
    tri3_xyze(0, 1) = 0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4551);
    tri3_xyze(0, 2) = 0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-407);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4553);
    tri3_xyze(0, 1) = 0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4555);
    tri3_xyze(0, 2) = 0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-408);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4555);
    tri3_xyze(0, 1) = 0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4305);
    tri3_xyze(0, 2) = 0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-408);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4305);
    tri3_xyze(0, 1) = 0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4303);
    tri3_xyze(0, 2) = 0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-408);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4303);
    tri3_xyze(0, 1) = 0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4553);
    tri3_xyze(0, 2) = 0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-408);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4555);
    tri3_xyze(0, 1) = 0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4557);
    tri3_xyze(0, 2) = 0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-409);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4557);
    tri3_xyze(0, 1) = 0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4307);
    tri3_xyze(0, 2) = 0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-409);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4307);
    tri3_xyze(0, 1) = 0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4305);
    tri3_xyze(0, 2) = 0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-409);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4305);
    tri3_xyze(0, 1) = 0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4555);
    tri3_xyze(0, 2) = 0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-409);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4557);
    tri3_xyze(0, 1) = 0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4559);
    tri3_xyze(0, 2) = 0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-410);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4559);
    tri3_xyze(0, 1) = 0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4309);
    tri3_xyze(0, 2) = 0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-410);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4309);
    tri3_xyze(0, 1) = 0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4307);
    tri3_xyze(0, 2) = 0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-410);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4307);
    tri3_xyze(0, 1) = 0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4557);
    tri3_xyze(0, 2) = 0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-410);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4559);
    tri3_xyze(0, 1) = 0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4561);
    tri3_xyze(0, 2) = 0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-411);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4561);
    tri3_xyze(0, 1) = 0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4311);
    tri3_xyze(0, 2) = 0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-411);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4311);
    tri3_xyze(0, 1) = 0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4309);
    tri3_xyze(0, 2) = 0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-411);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4309);
    tri3_xyze(0, 1) = 0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4559);
    tri3_xyze(0, 2) = 0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-411);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4561);
    tri3_xyze(0, 1) = 0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4563);
    tri3_xyze(0, 2) = 0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-412);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4563);
    tri3_xyze(0, 1) = 0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4313);
    tri3_xyze(0, 2) = 0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-412);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4313);
    tri3_xyze(0, 1) = 0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4311);
    tri3_xyze(0, 2) = 0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-412);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4311);
    tri3_xyze(0, 1) = 0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4561);
    tri3_xyze(0, 2) = 0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-412);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4563);
    tri3_xyze(0, 1) = 0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4565);
    tri3_xyze(0, 2) = 0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-413);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4565);
    tri3_xyze(0, 1) = 0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4315);
    tri3_xyze(0, 2) = 0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-413);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4315);
    tri3_xyze(0, 1) = 0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4313);
    tri3_xyze(0, 2) = 0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-413);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4313);
    tri3_xyze(0, 1) = 0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4563);
    tri3_xyze(0, 2) = 0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-413);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4565);
    tri3_xyze(0, 1) = 0.0515698;
    tri3_xyze(1, 1) = -0.0893214;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4567);
    tri3_xyze(0, 2) = 0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-414);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4567);
    tri3_xyze(0, 1) = 0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4317);
    tri3_xyze(0, 2) = 0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-414);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4317);
    tri3_xyze(0, 1) = 0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4315);
    tri3_xyze(0, 2) = 0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-414);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4315);
    tri3_xyze(0, 1) = 0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4565);
    tri3_xyze(0, 2) = 0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-414);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4567);
    tri3_xyze(0, 1) = 0.0546845;
    tri3_xyze(1, 1) = -0.0947164;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4569);
    tri3_xyze(0, 2) = 0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-415);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0546845;
    tri3_xyze(1, 0) = -0.0947164;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4569);
    tri3_xyze(0, 1) = 0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4319);
    tri3_xyze(0, 2) = 0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-415);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4319);
    tri3_xyze(0, 1) = 0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4317);
    tri3_xyze(0, 2) = 0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-415);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4317);
    tri3_xyze(0, 1) = 0.0515698;
    tri3_xyze(1, 1) = -0.0893214;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4567);
    tri3_xyze(0, 2) = 0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-415);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0546845;
    tri3_xyze(1, 0) = -0.0947164;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4569);
    tri3_xyze(0, 1) = 0.0577254;
    tri3_xyze(1, 1) = -0.0999834;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4571);
    tri3_xyze(0, 2) = 0.0426494;
    tri3_xyze(1, 2) = -0.102965;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-416);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4319);
    tri3_xyze(0, 1) = 0.0546845;
    tri3_xyze(1, 1) = -0.0947164;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4569);
    tri3_xyze(0, 2) = 0.0426494;
    tri3_xyze(1, 2) = -0.102965;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-416);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.075;
    tri3_xyze(1, 0) = -0.129904;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4591);
    tri3_xyze(0, 1) = 0.0388229;
    tri3_xyze(1, 1) = -0.144889;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4341);
    tri3_xyze(0, 2) = 0.0568366;
    tri3_xyze(1, 2) = -0.137216;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-426);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0388229;
    tri3_xyze(1, 0) = -0.144889;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4341);
    tri3_xyze(0, 1) = 0.0387208;
    tri3_xyze(1, 1) = -0.144508;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4339);
    tri3_xyze(0, 2) = 0.0568366;
    tri3_xyze(1, 2) = -0.137216;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-426);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0748029;
    tri3_xyze(1, 0) = -0.129562;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4593);
    tri3_xyze(0, 1) = 0.0387208;
    tri3_xyze(1, 1) = -0.144508;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4343);
    tri3_xyze(0, 2) = 0.0568366;
    tri3_xyze(1, 2) = -0.137216;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-427);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0387208;
    tri3_xyze(1, 0) = -0.144508;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4343);
    tri3_xyze(0, 1) = 0.0388229;
    tri3_xyze(1, 1) = -0.144889;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4341);
    tri3_xyze(0, 2) = 0.0568366;
    tri3_xyze(1, 2) = -0.137216;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-427);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0388229;
    tri3_xyze(1, 0) = -0.144889;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4341);
    tri3_xyze(0, 1) = 0.075;
    tri3_xyze(1, 1) = -0.129904;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4591);
    tri3_xyze(0, 2) = 0.0568366;
    tri3_xyze(1, 2) = -0.137216;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-427);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0742146;
    tri3_xyze(1, 0) = -0.128543;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4595);
    tri3_xyze(0, 1) = 0.0384163;
    tri3_xyze(1, 1) = -0.143372;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4345);
    tri3_xyze(0, 2) = 0.0565386;
    tri3_xyze(1, 2) = -0.136496;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-428);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0384163;
    tri3_xyze(1, 0) = -0.143372;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4345);
    tri3_xyze(0, 1) = 0.0387208;
    tri3_xyze(1, 1) = -0.144508;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4343);
    tri3_xyze(0, 2) = 0.0565386;
    tri3_xyze(1, 2) = -0.136496;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-428);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0387208;
    tri3_xyze(1, 0) = -0.144508;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4343);
    tri3_xyze(0, 1) = 0.0748029;
    tri3_xyze(1, 1) = -0.129562;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4593);
    tri3_xyze(0, 2) = 0.0565386;
    tri3_xyze(1, 2) = -0.136496;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-428);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0732444;
    tri3_xyze(1, 0) = -0.126863;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4597);
    tri3_xyze(0, 1) = 0.0379141;
    tri3_xyze(1, 1) = -0.141497;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4347);
    tri3_xyze(0, 2) = 0.0559473;
    tri3_xyze(1, 2) = -0.135069;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-429);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0379141;
    tri3_xyze(1, 0) = -0.141497;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4347);
    tri3_xyze(0, 1) = 0.0384163;
    tri3_xyze(1, 1) = -0.143372;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4345);
    tri3_xyze(0, 2) = 0.0559473;
    tri3_xyze(1, 2) = -0.135069;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-429);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0384163;
    tri3_xyze(1, 0) = -0.143372;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4345);
    tri3_xyze(0, 1) = 0.0742146;
    tri3_xyze(1, 1) = -0.128543;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4595);
    tri3_xyze(0, 2) = 0.0559473;
    tri3_xyze(1, 2) = -0.135069;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-429);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0719077;
    tri3_xyze(1, 0) = -0.124548;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4599);
    tri3_xyze(0, 1) = 0.0372221;
    tri3_xyze(1, 1) = -0.138915;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4349);
    tri3_xyze(0, 2) = 0.0550721;
    tri3_xyze(1, 2) = -0.132956;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-430);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0372221;
    tri3_xyze(1, 0) = -0.138915;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4349);
    tri3_xyze(0, 1) = 0.0379141;
    tri3_xyze(1, 1) = -0.141497;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4347);
    tri3_xyze(0, 2) = 0.0550721;
    tri3_xyze(1, 2) = -0.132956;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-430);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0379141;
    tri3_xyze(1, 0) = -0.141497;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4347);
    tri3_xyze(0, 1) = 0.0732444;
    tri3_xyze(1, 1) = -0.126863;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4597);
    tri3_xyze(0, 2) = 0.0550721;
    tri3_xyze(1, 2) = -0.132956;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-430);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0702254;
    tri3_xyze(1, 0) = -0.121634;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4601);
    tri3_xyze(0, 1) = 0.0363514;
    tri3_xyze(1, 1) = -0.135665;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4351);
    tri3_xyze(0, 2) = 0.0539266;
    tri3_xyze(1, 2) = -0.13019;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-431);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0363514;
    tri3_xyze(1, 0) = -0.135665;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4351);
    tri3_xyze(0, 1) = 0.0372221;
    tri3_xyze(1, 1) = -0.138915;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4349);
    tri3_xyze(0, 2) = 0.0539266;
    tri3_xyze(1, 2) = -0.13019;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-431);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0372221;
    tri3_xyze(1, 0) = -0.138915;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4349);
    tri3_xyze(0, 1) = 0.0719077;
    tri3_xyze(1, 1) = -0.124548;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4599);
    tri3_xyze(0, 2) = 0.0539266;
    tri3_xyze(1, 2) = -0.13019;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-431);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0682242;
    tri3_xyze(1, 0) = -0.118168;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4603);
    tri3_xyze(0, 1) = 0.0353155;
    tri3_xyze(1, 1) = -0.131799;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4353);
    tri3_xyze(0, 2) = 0.0525291;
    tri3_xyze(1, 2) = -0.126816;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-432);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0353155;
    tri3_xyze(1, 0) = -0.131799;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4353);
    tri3_xyze(0, 1) = 0.0363514;
    tri3_xyze(1, 1) = -0.135665;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4351);
    tri3_xyze(0, 2) = 0.0525291;
    tri3_xyze(1, 2) = -0.126816;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-432);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0363514;
    tri3_xyze(1, 0) = -0.135665;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4351);
    tri3_xyze(0, 1) = 0.0702254;
    tri3_xyze(1, 1) = -0.121634;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4601);
    tri3_xyze(0, 2) = 0.0525291;
    tri3_xyze(1, 2) = -0.126816;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-432);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0659356;
    tri3_xyze(1, 0) = -0.114204;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4605);
    tri3_xyze(0, 1) = 0.0341308;
    tri3_xyze(1, 1) = -0.127378;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4355);
    tri3_xyze(0, 2) = 0.0509015;
    tri3_xyze(1, 2) = -0.122887;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-433);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0341308;
    tri3_xyze(1, 0) = -0.127378;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4355);
    tri3_xyze(0, 1) = 0.0353155;
    tri3_xyze(1, 1) = -0.131799;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4353);
    tri3_xyze(0, 2) = 0.0509015;
    tri3_xyze(1, 2) = -0.122887;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-433);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0353155;
    tri3_xyze(1, 0) = -0.131799;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4353);
    tri3_xyze(0, 1) = 0.0682242;
    tri3_xyze(1, 1) = -0.118168;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4603);
    tri3_xyze(0, 2) = 0.0509015;
    tri3_xyze(1, 2) = -0.122887;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-433);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0659356;
    tri3_xyze(1, 0) = -0.114204;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4605);
    tri3_xyze(0, 1) = 0.0633957;
    tri3_xyze(1, 1) = -0.109805;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4607);
    tri3_xyze(0, 2) = 0.0490695;
    tri3_xyze(1, 2) = -0.118464;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-434);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0633957;
    tri3_xyze(1, 0) = -0.109805;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4607);
    tri3_xyze(0, 1) = 0.032816;
    tri3_xyze(1, 1) = -0.122471;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4357);
    tri3_xyze(0, 2) = 0.0490695;
    tri3_xyze(1, 2) = -0.118464;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-434);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.032816;
    tri3_xyze(1, 0) = -0.122471;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4357);
    tri3_xyze(0, 1) = 0.0341308;
    tri3_xyze(1, 1) = -0.127378;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4355);
    tri3_xyze(0, 2) = 0.0490695;
    tri3_xyze(1, 2) = -0.118464;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-434);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0341308;
    tri3_xyze(1, 0) = -0.127378;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4355);
    tri3_xyze(0, 1) = 0.0659356;
    tri3_xyze(1, 1) = -0.114204;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4605);
    tri3_xyze(0, 2) = 0.0490695;
    tri3_xyze(1, 2) = -0.118464;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-434);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0633957;
    tri3_xyze(1, 0) = -0.109805;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4607);
    tri3_xyze(0, 1) = 0.0606445;
    tri3_xyze(1, 1) = -0.105039;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4609);
    tri3_xyze(0, 2) = 0.047062;
    tri3_xyze(1, 2) = -0.113618;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-435);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0606445;
    tri3_xyze(1, 0) = -0.105039;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4609);
    tri3_xyze(0, 1) = 0.0313919;
    tri3_xyze(1, 1) = -0.117156;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4359);
    tri3_xyze(0, 2) = 0.047062;
    tri3_xyze(1, 2) = -0.113618;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-435);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0313919;
    tri3_xyze(1, 0) = -0.117156;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4359);
    tri3_xyze(0, 1) = 0.032816;
    tri3_xyze(1, 1) = -0.122471;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4357);
    tri3_xyze(0, 2) = 0.047062;
    tri3_xyze(1, 2) = -0.113618;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-435);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.032816;
    tri3_xyze(1, 0) = -0.122471;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4357);
    tri3_xyze(0, 1) = 0.0633957;
    tri3_xyze(1, 1) = -0.109805;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4607);
    tri3_xyze(0, 2) = 0.047062;
    tri3_xyze(1, 2) = -0.113618;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-435);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0606445;
    tri3_xyze(1, 0) = -0.105039;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4609);
    tri3_xyze(0, 1) = 0.0577254;
    tri3_xyze(1, 1) = -0.0999834;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4611);
    tri3_xyze(0, 2) = 0.0449107;
    tri3_xyze(1, 2) = -0.108424;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-436);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0577254;
    tri3_xyze(1, 0) = -0.0999834;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4611);
    tri3_xyze(0, 1) = 0.0298809;
    tri3_xyze(1, 1) = -0.111517;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4361);
    tri3_xyze(0, 2) = 0.0449107;
    tri3_xyze(1, 2) = -0.108424;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-436);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0298809;
    tri3_xyze(1, 0) = -0.111517;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4361);
    tri3_xyze(0, 1) = 0.0313919;
    tri3_xyze(1, 1) = -0.117156;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4359);
    tri3_xyze(0, 2) = 0.0449107;
    tri3_xyze(1, 2) = -0.108424;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-436);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0313919;
    tri3_xyze(1, 0) = -0.117156;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4359);
    tri3_xyze(0, 1) = 0.0606445;
    tri3_xyze(1, 1) = -0.105039;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4609);
    tri3_xyze(0, 2) = 0.0449107;
    tri3_xyze(1, 2) = -0.108424;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-436);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0577254;
    tri3_xyze(1, 0) = -0.0999834;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4611);
    tri3_xyze(0, 1) = 0.0546845;
    tri3_xyze(1, 1) = -0.0947164;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4613);
    tri3_xyze(0, 2) = 0.0426494;
    tri3_xyze(1, 2) = -0.102965;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-437);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0546845;
    tri3_xyze(1, 0) = -0.0947164;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4613);
    tri3_xyze(0, 1) = 0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4363);
    tri3_xyze(0, 2) = 0.0426494;
    tri3_xyze(1, 2) = -0.102965;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-437);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4363);
    tri3_xyze(0, 1) = 0.0298809;
    tri3_xyze(1, 1) = -0.111517;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4361);
    tri3_xyze(0, 2) = 0.0426494;
    tri3_xyze(1, 2) = -0.102965;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-437);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0298809;
    tri3_xyze(1, 0) = -0.111517;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4361);
    tri3_xyze(0, 1) = 0.0577254;
    tri3_xyze(1, 1) = -0.0999834;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4611);
    tri3_xyze(0, 2) = 0.0426494;
    tri3_xyze(1, 2) = -0.102965;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-437);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0546845;
    tri3_xyze(1, 0) = -0.0947164;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4613);
    tri3_xyze(0, 1) = 0.0515698;
    tri3_xyze(1, 1) = -0.0893214;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4615);
    tri3_xyze(0, 2) = 0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-438);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4615);
    tri3_xyze(0, 1) = 0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4365);
    tri3_xyze(0, 2) = 0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-438);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4365);
    tri3_xyze(0, 1) = 0.0283068;
    tri3_xyze(1, 1) = -0.105642;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4363);
    tri3_xyze(0, 2) = 0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-438);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0283068;
    tri3_xyze(1, 0) = -0.105642;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4363);
    tri3_xyze(0, 1) = 0.0546845;
    tri3_xyze(1, 1) = -0.0947164;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4613);
    tri3_xyze(0, 2) = 0.0403139;
    tri3_xyze(1, 2) = -0.0973263;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-438);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4615);
    tri3_xyze(0, 1) = 0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4617);
    tri3_xyze(0, 2) = 0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-439);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4617);
    tri3_xyze(0, 1) = 0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4367);
    tri3_xyze(0, 2) = 0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-439);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4367);
    tri3_xyze(0, 1) = 0.0266945;
    tri3_xyze(1, 1) = -0.0996251;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4365);
    tri3_xyze(0, 2) = 0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-439);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0266945;
    tri3_xyze(1, 0) = -0.0996251;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4365);
    tri3_xyze(0, 1) = 0.0515698;
    tri3_xyze(1, 1) = -0.0893214;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4615);
    tri3_xyze(0, 2) = 0.037941;
    tri3_xyze(1, 2) = -0.0915976;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-439);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4617);
    tri3_xyze(0, 1) = 0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4619);
    tri3_xyze(0, 2) = 0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-440);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4619);
    tri3_xyze(0, 1) = 0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4369);
    tri3_xyze(0, 2) = 0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-440);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4369);
    tri3_xyze(0, 1) = 0.0250693;
    tri3_xyze(1, 1) = -0.09356;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4367);
    tri3_xyze(0, 2) = 0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-440);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0250693;
    tri3_xyze(1, 0) = -0.09356;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4367);
    tri3_xyze(0, 1) = 0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4617);
    tri3_xyze(0, 2) = 0.035568;
    tri3_xyze(1, 2) = -0.0858688;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-440);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4619);
    tri3_xyze(0, 1) = 0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4621);
    tri3_xyze(0, 2) = 0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-441);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4621);
    tri3_xyze(0, 1) = 0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4371);
    tri3_xyze(0, 2) = 0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-441);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4371);
    tri3_xyze(0, 1) = 0.023457;
    tri3_xyze(1, 1) = -0.0875428;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4369);
    tri3_xyze(0, 2) = 0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-441);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.023457;
    tri3_xyze(1, 0) = -0.0875428;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4369);
    tri3_xyze(0, 1) = 0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4619);
    tri3_xyze(0, 2) = 0.0332325;
    tri3_xyze(1, 2) = -0.0802303;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-441);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4621);
    tri3_xyze(0, 1) = 0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4623);
    tri3_xyze(0, 2) = 0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-442);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4623);
    tri3_xyze(0, 1) = 0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4373);
    tri3_xyze(0, 2) = 0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-442);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4373);
    tri3_xyze(0, 1) = 0.0218829;
    tri3_xyze(1, 1) = -0.0816682;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4371);
    tri3_xyze(0, 2) = 0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-442);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0218829;
    tri3_xyze(1, 0) = -0.0816682;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4371);
    tri3_xyze(0, 1) = 0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4621);
    tri3_xyze(0, 2) = 0.0309712;
    tri3_xyze(1, 2) = -0.0747712;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-442);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4623);
    tri3_xyze(0, 1) = 0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4625);
    tri3_xyze(0, 2) = 0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-443);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4625);
    tri3_xyze(0, 1) = 0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4375);
    tri3_xyze(0, 2) = 0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-443);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4375);
    tri3_xyze(0, 1) = 0.0203719;
    tri3_xyze(1, 1) = -0.076029;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4373);
    tri3_xyze(0, 2) = 0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-443);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0203719;
    tri3_xyze(1, 0) = -0.076029;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4373);
    tri3_xyze(0, 1) = 0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4623);
    tri3_xyze(0, 2) = 0.0288199;
    tri3_xyze(1, 2) = -0.0695774;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-443);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4625);
    tri3_xyze(0, 1) = 0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4627);
    tri3_xyze(0, 2) = 0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-444);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4627);
    tri3_xyze(0, 1) = 0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4377);
    tri3_xyze(0, 2) = 0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-444);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4377);
    tri3_xyze(0, 1) = 0.0189478;
    tri3_xyze(1, 1) = -0.0707141;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4375);
    tri3_xyze(0, 2) = 0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-444);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0189478;
    tri3_xyze(1, 0) = -0.0707141;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4375);
    tri3_xyze(0, 1) = 0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4625);
    tri3_xyze(0, 2) = 0.0268124;
    tri3_xyze(1, 2) = -0.0647308;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-444);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4627);
    tri3_xyze(0, 1) = 0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4629);
    tri3_xyze(0, 2) = 0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-445);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4629);
    tri3_xyze(0, 1) = 0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4379);
    tri3_xyze(0, 2) = 0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-445);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4379);
    tri3_xyze(0, 1) = 0.017633;
    tri3_xyze(1, 1) = -0.0658074;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4377);
    tri3_xyze(0, 2) = 0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-445);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.017633;
    tri3_xyze(1, 0) = -0.0658074;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4377);
    tri3_xyze(0, 1) = 0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4627);
    tri3_xyze(0, 2) = 0.0249804;
    tri3_xyze(1, 2) = -0.060308;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-445);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4629);
    tri3_xyze(0, 1) = 0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4631);
    tri3_xyze(0, 2) = 0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-446);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4631);
    tri3_xyze(0, 1) = 0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4381);
    tri3_xyze(0, 2) = 0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-446);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4381);
    tri3_xyze(0, 1) = 0.0164484;
    tri3_xyze(1, 1) = -0.0613861;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4379);
    tri3_xyze(0, 2) = 0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-446);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0164484;
    tri3_xyze(1, 0) = -0.0613861;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4379);
    tri3_xyze(0, 1) = 0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4629);
    tri3_xyze(0, 2) = 0.0233528;
    tri3_xyze(1, 2) = -0.0563786;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-446);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4631);
    tri3_xyze(0, 1) = 0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4633);
    tri3_xyze(0, 2) = 0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-447);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0280923;
    tri3_xyze(1, 0) = -0.0486573;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4633);
    tri3_xyze(0, 1) = 0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4383);
    tri3_xyze(0, 2) = 0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-447);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4383);
    tri3_xyze(0, 1) = 0.0154125;
    tri3_xyze(1, 1) = -0.0575201;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4381);
    tri3_xyze(0, 2) = 0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-447);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0154125;
    tri3_xyze(1, 0) = -0.0575201;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4381);
    tri3_xyze(0, 1) = 0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4631);
    tri3_xyze(0, 2) = 0.0219553;
    tri3_xyze(1, 2) = -0.0530047;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-447);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0280923;
    tri3_xyze(1, 0) = -0.0486573;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4633);
    tri3_xyze(0, 1) = 0.0267556;
    tri3_xyze(1, 1) = -0.046342;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4635);
    tri3_xyze(0, 2) = 0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-448);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0267556;
    tri3_xyze(1, 0) = -0.046342;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4635);
    tri3_xyze(0, 1) = 0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4385);
    tri3_xyze(0, 2) = 0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-448);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4385);
    tri3_xyze(0, 1) = 0.0145417;
    tri3_xyze(1, 1) = -0.0542702;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4383);
    tri3_xyze(0, 2) = 0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-448);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0145417;
    tri3_xyze(1, 0) = -0.0542702;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4383);
    tri3_xyze(0, 1) = 0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4633);
    tri3_xyze(0, 2) = 0.0208098;
    tri3_xyze(1, 2) = -0.0502394;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-448);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0267556;
    tri3_xyze(1, 0) = -0.046342;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4635);
    tri3_xyze(0, 1) = 0.0257854;
    tri3_xyze(1, 1) = -0.0446617;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4637);
    tri3_xyze(0, 2) = 0.0199346;
    tri3_xyze(1, 2) = -0.0481263;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-449);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0257854;
    tri3_xyze(1, 0) = -0.0446617;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4637);
    tri3_xyze(0, 1) = 0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4387);
    tri3_xyze(0, 2) = 0.0199346;
    tri3_xyze(1, 2) = -0.0481263;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-449);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4387);
    tri3_xyze(0, 1) = 0.0138497;
    tri3_xyze(1, 1) = -0.0516878;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4385);
    tri3_xyze(0, 2) = 0.0199346;
    tri3_xyze(1, 2) = -0.0481263;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-449);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0138497;
    tri3_xyze(1, 0) = -0.0516878;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4385);
    tri3_xyze(0, 1) = 0.0267556;
    tri3_xyze(1, 1) = -0.046342;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4635);
    tri3_xyze(0, 2) = 0.0199346;
    tri3_xyze(1, 2) = -0.0481263;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-449);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0257854;
    tri3_xyze(1, 0) = -0.0446617;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4637);
    tri3_xyze(0, 1) = 0.0251971;
    tri3_xyze(1, 1) = -0.0436427;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4639);
    tri3_xyze(0, 2) = 0.0193433;
    tri3_xyze(1, 2) = -0.0466988;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-450);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0251971;
    tri3_xyze(1, 0) = -0.0436427;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4639);
    tri3_xyze(0, 1) = 0.013043;
    tri3_xyze(1, 1) = -0.0486771;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4389);
    tri3_xyze(0, 2) = 0.0193433;
    tri3_xyze(1, 2) = -0.0466988;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-450);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.013043;
    tri3_xyze(1, 0) = -0.0486771;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4389);
    tri3_xyze(0, 1) = 0.0133475;
    tri3_xyze(1, 1) = -0.0498136;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4387);
    tri3_xyze(0, 2) = 0.0193433;
    tri3_xyze(1, 2) = -0.0466988;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-450);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0133475;
    tri3_xyze(1, 0) = -0.0498136;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4387);
    tri3_xyze(0, 1) = 0.0257854;
    tri3_xyze(1, 1) = -0.0446617;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4637);
    tri3_xyze(0, 2) = 0.0193433;
    tri3_xyze(1, 2) = -0.0466988;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-450);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0353553;
    tri3_xyze(1, 0) = -0.0353553;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4791);
    tri3_xyze(0, 1) = 0.0356341;
    tri3_xyze(1, 1) = -0.0356341;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(4792);
    tri3_xyze(0, 2) = 0.0302966;
    tri3_xyze(1, 2) = -0.0394834;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-451);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0251971;
    tri3_xyze(1, 0) = -0.0436427;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4542);
    tri3_xyze(0, 1) = 0.025;
    tri3_xyze(1, 1) = -0.0433013;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4541);
    tri3_xyze(0, 2) = 0.0302966;
    tri3_xyze(1, 2) = -0.0394834;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-451);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.025;
    tri3_xyze(1, 0) = -0.0433013;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4541);
    tri3_xyze(0, 1) = 0.0353553;
    tri3_xyze(1, 1) = -0.0353553;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4791);
    tri3_xyze(0, 2) = 0.0302966;
    tri3_xyze(1, 2) = -0.0394834;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-451);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0356341;
    tri3_xyze(1, 0) = -0.0356341;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4889);
    tri3_xyze(0, 1) = 0.0353553;
    tri3_xyze(1, 1) = -0.0353553;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4791);
    tri3_xyze(0, 2) = 0.0302966;
    tri3_xyze(1, 2) = -0.0394834;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-452);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0353553;
    tri3_xyze(1, 0) = -0.0353553;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4791);
    tri3_xyze(0, 1) = 0.025;
    tri3_xyze(1, 1) = -0.0433013;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4541);
    tri3_xyze(0, 2) = 0.0302966;
    tri3_xyze(1, 2) = -0.0394834;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-452);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.025;
    tri3_xyze(1, 0) = -0.0433013;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4541);
    tri3_xyze(0, 1) = 0.0251971;
    tri3_xyze(1, 1) = -0.0436427;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4639);
    tri3_xyze(0, 2) = 0.0302966;
    tri3_xyze(1, 2) = -0.0394834;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-452);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0251971;
    tri3_xyze(1, 0) = -0.0436427;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4639);
    tri3_xyze(0, 1) = 0.0356341;
    tri3_xyze(1, 1) = -0.0356341;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4889);
    tri3_xyze(0, 2) = 0.0302966;
    tri3_xyze(1, 2) = -0.0394834;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-452);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0421076;
    tri3_xyze(1, 0) = -0.0421076;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4801);
    tri3_xyze(0, 1) = 0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4551);
    tri3_xyze(0, 2) = 0.0349258;
    tri3_xyze(1, 2) = -0.0455161;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-456);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4551);
    tri3_xyze(0, 1) = 0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.775912;
    nids.push_back(4549);
    tri3_xyze(0, 2) = 0.0349258;
    tri3_xyze(1, 2) = -0.0455161;
    tri3_xyze(2, 2) = 0.773262;
    nids.push_back(-456);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0449377;
    tri3_xyze(1, 0) = -0.0449377;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4803);
    tri3_xyze(0, 1) = 0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4553);
    tri3_xyze(0, 2) = 0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-457);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4553);
    tri3_xyze(0, 1) = 0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4551);
    tri3_xyze(0, 2) = 0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-457);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.770611;
    nids.push_back(4551);
    tri3_xyze(0, 1) = 0.0421076;
    tri3_xyze(1, 1) = -0.0421076;
    tri3_xyze(2, 1) = 0.770611;
    nids.push_back(4801);
    tri3_xyze(0, 2) = 0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.768192;
    nids.push_back(-457);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0449377;
    tri3_xyze(1, 0) = -0.0449377;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4803);
    tri3_xyze(0, 1) = 0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4805);
    tri3_xyze(0, 2) = 0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-458);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0481743;
    tri3_xyze(1, 0) = -0.0481743;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4805);
    tri3_xyze(0, 1) = 0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4555);
    tri3_xyze(0, 2) = 0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-458);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4555);
    tri3_xyze(0, 1) = 0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4553);
    tri3_xyze(0, 2) = 0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-458);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.765773;
    nids.push_back(4553);
    tri3_xyze(0, 1) = 0.0449377;
    tri3_xyze(1, 1) = -0.0449377;
    tri3_xyze(2, 1) = 0.765773;
    nids.push_back(4803);
    tri3_xyze(0, 2) = 0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.763623;
    nids.push_back(-458);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0481743;
    tri3_xyze(1, 0) = -0.0481743;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4805);
    tri3_xyze(0, 1) = 0.0517663;
    tri3_xyze(1, 1) = -0.0517663;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4807);
    tri3_xyze(0, 2) = 0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-459);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4807);
    tri3_xyze(0, 1) = 0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4557);
    tri3_xyze(0, 2) = 0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-459);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4557);
    tri3_xyze(0, 1) = 0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4555);
    tri3_xyze(0, 2) = 0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-459);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.761474;
    nids.push_back(4555);
    tri3_xyze(0, 1) = 0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4805);
    tri3_xyze(0, 2) = 0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-459);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4807);
    tri3_xyze(0, 1) = 0.0556571;
    tri3_xyze(1, 1) = -0.0556571;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4809);
    tri3_xyze(0, 2) = 0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-460);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0556571;
    tri3_xyze(1, 0) = -0.0556571;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4809);
    tri3_xyze(0, 1) = 0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4559);
    tri3_xyze(0, 2) = 0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-460);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4559);
    tri3_xyze(0, 1) = 0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4557);
    tri3_xyze(0, 2) = 0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-460);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4557);
    tri3_xyze(0, 1) = 0.0517663;
    tri3_xyze(1, 1) = -0.0517663;
    tri3_xyze(2, 1) = 0.757784;
    nids.push_back(4807);
    tri3_xyze(0, 2) = 0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.756271;
    nids.push_back(-460);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0556571;
    tri3_xyze(1, 0) = -0.0556571;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4809);
    tri3_xyze(0, 1) = 0.0597853;
    tri3_xyze(1, 1) = -0.0597853;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4811);
    tri3_xyze(0, 2) = 0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-461);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0597853;
    tri3_xyze(1, 0) = -0.0597853;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4811);
    tri3_xyze(0, 1) = 0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4561);
    tri3_xyze(0, 2) = 0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-461);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4561);
    tri3_xyze(0, 1) = 0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4559);
    tri3_xyze(0, 2) = 0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-461);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.754759;
    nids.push_back(4559);
    tri3_xyze(0, 1) = 0.0556571;
    tri3_xyze(1, 1) = -0.0556571;
    tri3_xyze(2, 1) = 0.754759;
    nids.push_back(4809);
    tri3_xyze(0, 2) = 0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.753603;
    nids.push_back(-461);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0640857;
    tri3_xyze(1, 0) = -0.0640857;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4813);
    tri3_xyze(0, 1) = 0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4563);
    tri3_xyze(0, 2) = 0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-462);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4563);
    tri3_xyze(0, 1) = 0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4561);
    tri3_xyze(0, 2) = 0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-462);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.752447;
    nids.push_back(4561);
    tri3_xyze(0, 1) = 0.0597853;
    tri3_xyze(1, 1) = -0.0597853;
    tri3_xyze(2, 1) = 0.752447;
    nids.push_back(4811);
    tri3_xyze(0, 2) = 0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.751666;
    nids.push_back(-462);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0684907;
    tri3_xyze(1, 0) = -0.0684907;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4815);
    tri3_xyze(0, 1) = 0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4565);
    tri3_xyze(0, 2) = 0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-463);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4565);
    tri3_xyze(0, 1) = 0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4563);
    tri3_xyze(0, 2) = 0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-463);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.750886;
    nids.push_back(4563);
    tri3_xyze(0, 1) = 0.0640857;
    tri3_xyze(1, 1) = -0.0640857;
    tri3_xyze(2, 1) = 0.750886;
    nids.push_back(4813);
    tri3_xyze(0, 2) = 0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.750492;
    nids.push_back(-463);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4567);
    tri3_xyze(0, 1) = 0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4565);
    tri3_xyze(0, 2) = 0.0603553;
    tri3_xyze(1, 2) = -0.0786566;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-464);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.750099;
    nids.push_back(4565);
    tri3_xyze(0, 1) = 0.0684907;
    tri3_xyze(1, 1) = -0.0684907;
    tri3_xyze(2, 1) = 0.750099;
    nids.push_back(4815);
    tri3_xyze(0, 2) = 0.0603553;
    tri3_xyze(1, 2) = -0.0786566;
    tri3_xyze(2, 2) = 0.750099;
    nids.push_back(-464);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.101693;
    tri3_xyze(1, 0) = -0.101693;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4849);
    tri3_xyze(0, 1) = 0.0993137;
    tri3_xyze(1, 1) = -0.0993137;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4851);
    tri3_xyze(0, 2) = 0.0857849;
    tri3_xyze(1, 2) = -0.111797;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-481);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0993137;
    tri3_xyze(1, 0) = -0.0993137;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4851);
    tri3_xyze(0, 1) = 0.0702254;
    tri3_xyze(1, 1) = -0.121634;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4601);
    tri3_xyze(0, 2) = 0.0857849;
    tri3_xyze(1, 2) = -0.111797;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-481);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0993137;
    tri3_xyze(1, 0) = -0.0993137;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4851);
    tri3_xyze(0, 1) = 0.0964836;
    tri3_xyze(1, 1) = -0.0964836;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4853);
    tri3_xyze(0, 2) = 0.0835617;
    tri3_xyze(1, 2) = -0.1089;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-482);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0964836;
    tri3_xyze(1, 0) = -0.0964836;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4853);
    tri3_xyze(0, 1) = 0.0682242;
    tri3_xyze(1, 1) = -0.118168;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4603);
    tri3_xyze(0, 2) = 0.0835617;
    tri3_xyze(1, 2) = -0.1089;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-482);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0702254;
    tri3_xyze(1, 0) = -0.121634;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4601);
    tri3_xyze(0, 1) = 0.0993137;
    tri3_xyze(1, 1) = -0.0993137;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4851);
    tri3_xyze(0, 2) = 0.0835617;
    tri3_xyze(1, 2) = -0.1089;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-482);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0964836;
    tri3_xyze(1, 0) = -0.0964836;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4853);
    tri3_xyze(0, 1) = 0.093247;
    tri3_xyze(1, 1) = -0.093247;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4855);
    tri3_xyze(0, 2) = 0.0809726;
    tri3_xyze(1, 2) = -0.105526;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-483);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.093247;
    tri3_xyze(1, 0) = -0.093247;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4855);
    tri3_xyze(0, 1) = 0.0659356;
    tri3_xyze(1, 1) = -0.114204;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4605);
    tri3_xyze(0, 2) = 0.0809726;
    tri3_xyze(1, 2) = -0.105526;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-483);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0682242;
    tri3_xyze(1, 0) = -0.118168;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4603);
    tri3_xyze(0, 1) = 0.0964836;
    tri3_xyze(1, 1) = -0.0964836;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4853);
    tri3_xyze(0, 2) = 0.0809726;
    tri3_xyze(1, 2) = -0.105526;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-483);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.093247;
    tri3_xyze(1, 0) = -0.093247;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4855);
    tri3_xyze(0, 1) = 0.089655;
    tri3_xyze(1, 1) = -0.089655;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4857);
    tri3_xyze(0, 2) = 0.0780583;
    tri3_xyze(1, 2) = -0.101728;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-484);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.089655;
    tri3_xyze(1, 0) = -0.089655;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4857);
    tri3_xyze(0, 1) = 0.0633957;
    tri3_xyze(1, 1) = -0.109805;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4607);
    tri3_xyze(0, 2) = 0.0780583;
    tri3_xyze(1, 2) = -0.101728;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-484);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0659356;
    tri3_xyze(1, 0) = -0.114204;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4605);
    tri3_xyze(0, 1) = 0.093247;
    tri3_xyze(1, 1) = -0.093247;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4855);
    tri3_xyze(0, 2) = 0.0780583;
    tri3_xyze(1, 2) = -0.101728;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-484);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.089655;
    tri3_xyze(1, 0) = -0.089655;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4857);
    tri3_xyze(0, 1) = 0.0857642;
    tri3_xyze(1, 1) = -0.0857642;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4859);
    tri3_xyze(0, 2) = 0.0748649;
    tri3_xyze(1, 2) = -0.0975658;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-485);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0857642;
    tri3_xyze(1, 0) = -0.0857642;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4859);
    tri3_xyze(0, 1) = 0.0606445;
    tri3_xyze(1, 1) = -0.105039;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4609);
    tri3_xyze(0, 2) = 0.0748649;
    tri3_xyze(1, 2) = -0.0975658;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-485);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0606445;
    tri3_xyze(1, 0) = -0.105039;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4609);
    tri3_xyze(0, 1) = 0.0633957;
    tri3_xyze(1, 1) = -0.109805;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4607);
    tri3_xyze(0, 2) = 0.0748649;
    tri3_xyze(1, 2) = -0.0975658;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-485);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0633957;
    tri3_xyze(1, 0) = -0.109805;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4607);
    tri3_xyze(0, 1) = 0.089655;
    tri3_xyze(1, 1) = -0.089655;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4857);
    tri3_xyze(0, 2) = 0.0748649;
    tri3_xyze(1, 2) = -0.0975658;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-485);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0857642;
    tri3_xyze(1, 0) = -0.0857642;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4859);
    tri3_xyze(0, 1) = 0.0816361;
    tri3_xyze(1, 1) = -0.0816361;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4861);
    tri3_xyze(0, 2) = 0.0714426;
    tri3_xyze(1, 2) = -0.0931058;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-486);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0816361;
    tri3_xyze(1, 0) = -0.0816361;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4861);
    tri3_xyze(0, 1) = 0.0577254;
    tri3_xyze(1, 1) = -0.0999834;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4611);
    tri3_xyze(0, 2) = 0.0714426;
    tri3_xyze(1, 2) = -0.0931058;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-486);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0577254;
    tri3_xyze(1, 0) = -0.0999834;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4611);
    tri3_xyze(0, 1) = 0.0606445;
    tri3_xyze(1, 1) = -0.105039;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4609);
    tri3_xyze(0, 2) = 0.0714426;
    tri3_xyze(1, 2) = -0.0931058;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-486);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0606445;
    tri3_xyze(1, 0) = -0.105039;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4609);
    tri3_xyze(0, 1) = 0.0857642;
    tri3_xyze(1, 1) = -0.0857642;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4859);
    tri3_xyze(0, 2) = 0.0714426;
    tri3_xyze(1, 2) = -0.0931058;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-486);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0729307;
    tri3_xyze(1, 0) = -0.0729307;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4865);
    tri3_xyze(0, 1) = 0.0684907;
    tri3_xyze(1, 1) = -0.0684907;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4867);
    tri3_xyze(0, 2) = 0.0603553;
    tri3_xyze(1, 2) = -0.0786566;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-489);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0684907;
    tri3_xyze(1, 0) = -0.0684907;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4867);
    tri3_xyze(0, 1) = 0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4617);
    tri3_xyze(0, 2) = 0.0603553;
    tri3_xyze(1, 2) = -0.0786566;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-489);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4617);
    tri3_xyze(0, 1) = 0.0515698;
    tri3_xyze(1, 1) = -0.0893214;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4615);
    tri3_xyze(0, 2) = 0.0603553;
    tri3_xyze(1, 2) = -0.0786566;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-489);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515698;
    tri3_xyze(1, 0) = -0.0893214;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4615);
    tri3_xyze(0, 1) = 0.0729307;
    tri3_xyze(1, 1) = -0.0729307;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4865);
    tri3_xyze(0, 2) = 0.0603553;
    tri3_xyze(1, 2) = -0.0786566;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-489);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0684907;
    tri3_xyze(1, 0) = -0.0684907;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4867);
    tri3_xyze(0, 1) = 0.0640857;
    tri3_xyze(1, 1) = -0.0640857;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4869);
    tri3_xyze(0, 2) = 0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-490);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0640857;
    tri3_xyze(1, 0) = -0.0640857;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4869);
    tri3_xyze(0, 1) = 0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4619);
    tri3_xyze(0, 2) = 0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-490);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4619);
    tri3_xyze(0, 1) = 0.0484302;
    tri3_xyze(1, 1) = -0.0838836;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4617);
    tri3_xyze(0, 2) = 0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-490);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0484302;
    tri3_xyze(1, 0) = -0.0838836;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4617);
    tri3_xyze(0, 1) = 0.0684907;
    tri3_xyze(1, 1) = -0.0684907;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4867);
    tri3_xyze(0, 2) = 0.0565805;
    tri3_xyze(1, 2) = -0.0737372;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-490);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0640857;
    tri3_xyze(1, 0) = -0.0640857;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4869);
    tri3_xyze(0, 1) = 0.0597853;
    tri3_xyze(1, 1) = -0.0597853;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4871);
    tri3_xyze(0, 2) = 0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-491);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0597853;
    tri3_xyze(1, 0) = -0.0597853;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4871);
    tri3_xyze(0, 1) = 0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4621);
    tri3_xyze(0, 2) = 0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-491);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4621);
    tri3_xyze(0, 1) = 0.0453155;
    tri3_xyze(1, 1) = -0.0784887;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4619);
    tri3_xyze(0, 2) = 0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-491);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0453155;
    tri3_xyze(1, 0) = -0.0784887;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4619);
    tri3_xyze(0, 1) = 0.0640857;
    tri3_xyze(1, 1) = -0.0640857;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4869);
    tri3_xyze(0, 2) = 0.0528653;
    tri3_xyze(1, 2) = -0.0688954;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-491);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0597853;
    tri3_xyze(1, 0) = -0.0597853;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4871);
    tri3_xyze(0, 1) = 0.0556571;
    tri3_xyze(1, 1) = -0.0556571;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4873);
    tri3_xyze(0, 2) = 0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-492);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0556571;
    tri3_xyze(1, 0) = -0.0556571;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4873);
    tri3_xyze(0, 1) = 0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4623);
    tri3_xyze(0, 2) = 0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-492);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4623);
    tri3_xyze(0, 1) = 0.0422746;
    tri3_xyze(1, 1) = -0.0732217;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4621);
    tri3_xyze(0, 2) = 0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-492);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0422746;
    tri3_xyze(1, 0) = -0.0732217;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4621);
    tri3_xyze(0, 1) = 0.0597853;
    tri3_xyze(1, 1) = -0.0597853;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4871);
    tri3_xyze(0, 2) = 0.0492681;
    tri3_xyze(1, 2) = -0.0642075;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-492);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0556571;
    tri3_xyze(1, 0) = -0.0556571;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4873);
    tri3_xyze(0, 1) = 0.0517663;
    tri3_xyze(1, 1) = -0.0517663;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4875);
    tri3_xyze(0, 2) = 0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-493);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4875);
    tri3_xyze(0, 1) = 0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4625);
    tri3_xyze(0, 2) = 0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-493);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4625);
    tri3_xyze(0, 1) = 0.0393555;
    tri3_xyze(1, 1) = -0.0681658;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4623);
    tri3_xyze(0, 2) = 0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-493);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0393555;
    tri3_xyze(1, 0) = -0.0681658;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4623);
    tri3_xyze(0, 1) = 0.0556571;
    tri3_xyze(1, 1) = -0.0556571;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4873);
    tri3_xyze(0, 2) = 0.0458458;
    tri3_xyze(1, 2) = -0.0597474;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-493);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4875);
    tri3_xyze(0, 1) = 0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4877);
    tri3_xyze(0, 2) = 0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-494);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0481743;
    tri3_xyze(1, 0) = -0.0481743;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4877);
    tri3_xyze(0, 1) = 0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4627);
    tri3_xyze(0, 2) = 0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-494);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4627);
    tri3_xyze(0, 1) = 0.0366043;
    tri3_xyze(1, 1) = -0.0634006;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4625);
    tri3_xyze(0, 2) = 0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-494);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0366043;
    tri3_xyze(1, 0) = -0.0634006;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4625);
    tri3_xyze(0, 1) = 0.0517663;
    tri3_xyze(1, 1) = -0.0517663;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4875);
    tri3_xyze(0, 2) = 0.0426524;
    tri3_xyze(1, 2) = -0.0555856;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-494);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0481743;
    tri3_xyze(1, 0) = -0.0481743;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4877);
    tri3_xyze(0, 1) = 0.0449377;
    tri3_xyze(1, 1) = -0.0449377;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4879);
    tri3_xyze(0, 2) = 0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-495);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0449377;
    tri3_xyze(1, 0) = -0.0449377;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4879);
    tri3_xyze(0, 1) = 0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4629);
    tri3_xyze(0, 2) = 0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-495);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4629);
    tri3_xyze(0, 1) = 0.0340644;
    tri3_xyze(1, 1) = -0.0590013;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4627);
    tri3_xyze(0, 2) = 0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-495);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0340644;
    tri3_xyze(1, 0) = -0.0590013;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4627);
    tri3_xyze(0, 1) = 0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4877);
    tri3_xyze(0, 2) = 0.0397381;
    tri3_xyze(1, 2) = -0.0517877;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-495);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0449377;
    tri3_xyze(1, 0) = -0.0449377;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4879);
    tri3_xyze(0, 1) = 0.0421076;
    tri3_xyze(1, 1) = -0.0421076;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4881);
    tri3_xyze(0, 2) = 0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-496);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0421076;
    tri3_xyze(1, 0) = -0.0421076;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4881);
    tri3_xyze(0, 1) = 0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4631);
    tri3_xyze(0, 2) = 0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-496);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4631);
    tri3_xyze(0, 1) = 0.0317758;
    tri3_xyze(1, 1) = -0.0550373;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4629);
    tri3_xyze(0, 2) = 0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-496);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0317758;
    tri3_xyze(1, 0) = -0.0550373;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4629);
    tri3_xyze(0, 1) = 0.0449377;
    tri3_xyze(1, 1) = -0.0449377;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4879);
    tri3_xyze(0, 2) = 0.0371489;
    tri3_xyze(1, 2) = -0.0484134;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-496);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0421076;
    tri3_xyze(1, 0) = -0.0421076;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4881);
    tri3_xyze(0, 1) = 0.0397286;
    tri3_xyze(1, 1) = -0.0397286;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4883);
    tri3_xyze(0, 2) = 0.0349258;
    tri3_xyze(1, 2) = -0.0455161;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-497);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0397286;
    tri3_xyze(1, 0) = -0.0397286;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4883);
    tri3_xyze(0, 1) = 0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4633);
    tri3_xyze(0, 2) = 0.0349258;
    tri3_xyze(1, 2) = -0.0455161;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-497);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0280923;
    tri3_xyze(1, 0) = -0.0486573;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4633);
    tri3_xyze(0, 1) = 0.0297746;
    tri3_xyze(1, 1) = -0.0515711;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4631);
    tri3_xyze(0, 2) = 0.0349258;
    tri3_xyze(1, 2) = -0.0455161;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-497);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0297746;
    tri3_xyze(1, 0) = -0.0515711;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4631);
    tri3_xyze(0, 1) = 0.0421076;
    tri3_xyze(1, 1) = -0.0421076;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4881);
    tri3_xyze(0, 2) = 0.0349258;
    tri3_xyze(1, 2) = -0.0455161;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-497);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0397286;
    tri3_xyze(1, 0) = -0.0397286;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4883);
    tri3_xyze(0, 1) = 0.0378381;
    tri3_xyze(1, 1) = -0.0378381;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4885);
    tri3_xyze(0, 2) = 0.0331036;
    tri3_xyze(1, 2) = -0.0431415;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-498);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0378381;
    tri3_xyze(1, 0) = -0.0378381;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4885);
    tri3_xyze(0, 1) = 0.0267556;
    tri3_xyze(1, 1) = -0.046342;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4635);
    tri3_xyze(0, 2) = 0.0331036;
    tri3_xyze(1, 2) = -0.0431415;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-498);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0267556;
    tri3_xyze(1, 0) = -0.046342;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4635);
    tri3_xyze(0, 1) = 0.0280923;
    tri3_xyze(1, 1) = -0.0486573;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4633);
    tri3_xyze(0, 2) = 0.0331036;
    tri3_xyze(1, 2) = -0.0431415;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-498);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0280923;
    tri3_xyze(1, 0) = -0.0486573;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4633);
    tri3_xyze(0, 1) = 0.0397286;
    tri3_xyze(1, 1) = -0.0397286;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4883);
    tri3_xyze(0, 2) = 0.0331036;
    tri3_xyze(1, 2) = -0.0431415;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-498);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0378381;
    tri3_xyze(1, 0) = -0.0378381;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4885);
    tri3_xyze(0, 1) = 0.0364661;
    tri3_xyze(1, 1) = -0.0364661;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4887);
    tri3_xyze(0, 2) = 0.0317113;
    tri3_xyze(1, 2) = -0.041327;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-499);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0364661;
    tri3_xyze(1, 0) = -0.0364661;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4887);
    tri3_xyze(0, 1) = 0.0257854;
    tri3_xyze(1, 1) = -0.0446617;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4637);
    tri3_xyze(0, 2) = 0.0317113;
    tri3_xyze(1, 2) = -0.041327;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-499);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0257854;
    tri3_xyze(1, 0) = -0.0446617;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4637);
    tri3_xyze(0, 1) = 0.0267556;
    tri3_xyze(1, 1) = -0.046342;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4635);
    tri3_xyze(0, 2) = 0.0317113;
    tri3_xyze(1, 2) = -0.041327;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-499);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0267556;
    tri3_xyze(1, 0) = -0.046342;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4635);
    tri3_xyze(0, 1) = 0.0378381;
    tri3_xyze(1, 1) = -0.0378381;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4885);
    tri3_xyze(0, 2) = 0.0317113;
    tri3_xyze(1, 2) = -0.041327;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-499);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0364661;
    tri3_xyze(1, 0) = -0.0364661;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4887);
    tri3_xyze(0, 1) = 0.0356341;
    tri3_xyze(1, 1) = -0.0356341;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4889);
    tri3_xyze(0, 2) = 0.0307707;
    tri3_xyze(1, 2) = -0.0401011;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-500);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0356341;
    tri3_xyze(1, 0) = -0.0356341;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4889);
    tri3_xyze(0, 1) = 0.0251971;
    tri3_xyze(1, 1) = -0.0436427;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4639);
    tri3_xyze(0, 2) = 0.0307707;
    tri3_xyze(1, 2) = -0.0401011;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-500);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0251971;
    tri3_xyze(1, 0) = -0.0436427;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4639);
    tri3_xyze(0, 1) = 0.0257854;
    tri3_xyze(1, 1) = -0.0446617;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4637);
    tri3_xyze(0, 2) = 0.0307707;
    tri3_xyze(1, 2) = -0.0401011;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-500);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0257854;
    tri3_xyze(1, 0) = -0.0446617;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4637);
    tri3_xyze(0, 1) = 0.0364661;
    tri3_xyze(1, 1) = -0.0364661;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4887);
    tri3_xyze(0, 2) = 0.0307707;
    tri3_xyze(1, 2) = -0.0401011;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-500);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0433013;
    tri3_xyze(1, 0) = -0.025;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(5041);
    tri3_xyze(0, 1) = 0.0436427;
    tri3_xyze(1, 1) = -0.0251971;
    tri3_xyze(2, 1) = 0.793733;
    nids.push_back(5042);
    tri3_xyze(0, 2) = 0.0394834;
    tri3_xyze(1, 2) = -0.0302966;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-501);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0356341;
    tri3_xyze(1, 0) = -0.0356341;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(4792);
    tri3_xyze(0, 1) = 0.0353553;
    tri3_xyze(1, 1) = -0.0353553;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4791);
    tri3_xyze(0, 2) = 0.0394834;
    tri3_xyze(1, 2) = -0.0302966;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-501);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0353553;
    tri3_xyze(1, 0) = -0.0353553;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4791);
    tri3_xyze(0, 1) = 0.0433013;
    tri3_xyze(1, 1) = -0.025;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5041);
    tri3_xyze(0, 2) = 0.0394834;
    tri3_xyze(1, 2) = -0.0302966;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-501);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0436427;
    tri3_xyze(1, 0) = -0.0251971;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5139);
    tri3_xyze(0, 1) = 0.0433013;
    tri3_xyze(1, 1) = -0.025;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5041);
    tri3_xyze(0, 2) = 0.0394834;
    tri3_xyze(1, 2) = -0.0302966;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-502);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0433013;
    tri3_xyze(1, 0) = -0.025;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(5041);
    tri3_xyze(0, 1) = 0.0353553;
    tri3_xyze(1, 1) = -0.0353553;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(4791);
    tri3_xyze(0, 2) = 0.0394834;
    tri3_xyze(1, 2) = -0.0302966;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-502);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0353553;
    tri3_xyze(1, 0) = -0.0353553;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(4791);
    tri3_xyze(0, 1) = 0.0356341;
    tri3_xyze(1, 1) = -0.0356341;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4889);
    tri3_xyze(0, 2) = 0.0394834;
    tri3_xyze(1, 2) = -0.0302966;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-502);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0356341;
    tri3_xyze(1, 0) = -0.0356341;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4889);
    tri3_xyze(0, 1) = 0.0436427;
    tri3_xyze(1, 1) = -0.0251971;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5139);
    tri3_xyze(0, 2) = 0.0394834;
    tri3_xyze(1, 2) = -0.0302966;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-502);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.757784;
    nids.push_back(4807);
    tri3_xyze(0, 1) = 0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.761474;
    nids.push_back(4805);
    tri3_xyze(0, 2) = 0.0555856;
    tri3_xyze(1, 2) = -0.0426524;
    tri3_xyze(2, 2) = 0.759629;
    nids.push_back(-509);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.121634;
    tri3_xyze(1, 0) = -0.0702254;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(5101);
    tri3_xyze(0, 1) = 0.0993137;
    tri3_xyze(1, 1) = -0.0993137;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4851);
    tri3_xyze(0, 2) = 0.111797;
    tri3_xyze(1, 2) = -0.0857849;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-531);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0993137;
    tri3_xyze(1, 0) = -0.0993137;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4851);
    tri3_xyze(0, 1) = 0.101693;
    tri3_xyze(1, 1) = -0.101693;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4849);
    tri3_xyze(0, 2) = 0.111797;
    tri3_xyze(1, 2) = -0.0857849;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-531);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.118168;
    tri3_xyze(1, 0) = -0.0682242;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(5103);
    tri3_xyze(0, 1) = 0.0964836;
    tri3_xyze(1, 1) = -0.0964836;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4853);
    tri3_xyze(0, 2) = 0.1089;
    tri3_xyze(1, 2) = -0.0835617;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-532);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0964836;
    tri3_xyze(1, 0) = -0.0964836;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4853);
    tri3_xyze(0, 1) = 0.0993137;
    tri3_xyze(1, 1) = -0.0993137;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4851);
    tri3_xyze(0, 2) = 0.1089;
    tri3_xyze(1, 2) = -0.0835617;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-532);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0993137;
    tri3_xyze(1, 0) = -0.0993137;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4851);
    tri3_xyze(0, 1) = 0.121634;
    tri3_xyze(1, 1) = -0.0702254;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(5101);
    tri3_xyze(0, 2) = 0.1089;
    tri3_xyze(1, 2) = -0.0835617;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-532);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.114204;
    tri3_xyze(1, 0) = -0.0659356;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(5105);
    tri3_xyze(0, 1) = 0.093247;
    tri3_xyze(1, 1) = -0.093247;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4855);
    tri3_xyze(0, 2) = 0.105526;
    tri3_xyze(1, 2) = -0.0809726;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-533);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.093247;
    tri3_xyze(1, 0) = -0.093247;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4855);
    tri3_xyze(0, 1) = 0.0964836;
    tri3_xyze(1, 1) = -0.0964836;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4853);
    tri3_xyze(0, 2) = 0.105526;
    tri3_xyze(1, 2) = -0.0809726;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-533);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0964836;
    tri3_xyze(1, 0) = -0.0964836;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4853);
    tri3_xyze(0, 1) = 0.118168;
    tri3_xyze(1, 1) = -0.0682242;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(5103);
    tri3_xyze(0, 2) = 0.105526;
    tri3_xyze(1, 2) = -0.0809726;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-533);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.109805;
    tri3_xyze(1, 0) = -0.0633957;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(5107);
    tri3_xyze(0, 1) = 0.089655;
    tri3_xyze(1, 1) = -0.089655;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4857);
    tri3_xyze(0, 2) = 0.101728;
    tri3_xyze(1, 2) = -0.0780583;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-534);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.089655;
    tri3_xyze(1, 0) = -0.089655;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4857);
    tri3_xyze(0, 1) = 0.093247;
    tri3_xyze(1, 1) = -0.093247;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4855);
    tri3_xyze(0, 2) = 0.101728;
    tri3_xyze(1, 2) = -0.0780583;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-534);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.093247;
    tri3_xyze(1, 0) = -0.093247;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4855);
    tri3_xyze(0, 1) = 0.114204;
    tri3_xyze(1, 1) = -0.0659356;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(5105);
    tri3_xyze(0, 2) = 0.101728;
    tri3_xyze(1, 2) = -0.0780583;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-534);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.109805;
    tri3_xyze(1, 0) = -0.0633957;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(5107);
    tri3_xyze(0, 1) = 0.105039;
    tri3_xyze(1, 1) = -0.0606445;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(5109);
    tri3_xyze(0, 2) = 0.0975658;
    tri3_xyze(1, 2) = -0.0748649;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-535);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.105039;
    tri3_xyze(1, 0) = -0.0606445;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(5109);
    tri3_xyze(0, 1) = 0.0857642;
    tri3_xyze(1, 1) = -0.0857642;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4859);
    tri3_xyze(0, 2) = 0.0975658;
    tri3_xyze(1, 2) = -0.0748649;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-535);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0857642;
    tri3_xyze(1, 0) = -0.0857642;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4859);
    tri3_xyze(0, 1) = 0.089655;
    tri3_xyze(1, 1) = -0.089655;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4857);
    tri3_xyze(0, 2) = 0.0975658;
    tri3_xyze(1, 2) = -0.0748649;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-535);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.089655;
    tri3_xyze(1, 0) = -0.089655;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4857);
    tri3_xyze(0, 1) = 0.109805;
    tri3_xyze(1, 1) = -0.0633957;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(5107);
    tri3_xyze(0, 2) = 0.0975658;
    tri3_xyze(1, 2) = -0.0748649;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-535);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.105039;
    tri3_xyze(1, 0) = -0.0606445;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(5109);
    tri3_xyze(0, 1) = 0.0999834;
    tri3_xyze(1, 1) = -0.0577254;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(5111);
    tri3_xyze(0, 2) = 0.0931058;
    tri3_xyze(1, 2) = -0.0714426;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-536);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0999834;
    tri3_xyze(1, 0) = -0.0577254;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(5111);
    tri3_xyze(0, 1) = 0.0816361;
    tri3_xyze(1, 1) = -0.0816361;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4861);
    tri3_xyze(0, 2) = 0.0931058;
    tri3_xyze(1, 2) = -0.0714426;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-536);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0816361;
    tri3_xyze(1, 0) = -0.0816361;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4861);
    tri3_xyze(0, 1) = 0.0857642;
    tri3_xyze(1, 1) = -0.0857642;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4859);
    tri3_xyze(0, 2) = 0.0931058;
    tri3_xyze(1, 2) = -0.0714426;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-536);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0857642;
    tri3_xyze(1, 0) = -0.0857642;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4859);
    tri3_xyze(0, 1) = 0.105039;
    tri3_xyze(1, 1) = -0.0606445;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(5109);
    tri3_xyze(0, 2) = 0.0931058;
    tri3_xyze(1, 2) = -0.0714426;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-536);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0999834;
    tri3_xyze(1, 0) = -0.0577254;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(5111);
    tri3_xyze(0, 1) = 0.0947164;
    tri3_xyze(1, 1) = -0.0546845;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(5113);
    tri3_xyze(0, 2) = 0.0884179;
    tri3_xyze(1, 2) = -0.0678454;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-537);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0947164;
    tri3_xyze(1, 0) = -0.0546845;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(5113);
    tri3_xyze(0, 1) = 0.0773356;
    tri3_xyze(1, 1) = -0.0773356;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4863);
    tri3_xyze(0, 2) = 0.0884179;
    tri3_xyze(1, 2) = -0.0678454;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-537);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0773356;
    tri3_xyze(1, 0) = -0.0773356;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4863);
    tri3_xyze(0, 1) = 0.0816361;
    tri3_xyze(1, 1) = -0.0816361;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4861);
    tri3_xyze(0, 2) = 0.0884179;
    tri3_xyze(1, 2) = -0.0678454;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-537);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0816361;
    tri3_xyze(1, 0) = -0.0816361;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4861);
    tri3_xyze(0, 1) = 0.0999834;
    tri3_xyze(1, 1) = -0.0577254;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(5111);
    tri3_xyze(0, 2) = 0.0884179;
    tri3_xyze(1, 2) = -0.0678454;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-537);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0947164;
    tri3_xyze(1, 0) = -0.0546845;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(5113);
    tri3_xyze(0, 1) = 0.0893214;
    tri3_xyze(1, 1) = -0.0515698;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(5115);
    tri3_xyze(0, 2) = 0.083576;
    tri3_xyze(1, 2) = -0.0641301;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-538);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0893214;
    tri3_xyze(1, 0) = -0.0515698;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(5115);
    tri3_xyze(0, 1) = 0.0729307;
    tri3_xyze(1, 1) = -0.0729307;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4865);
    tri3_xyze(0, 2) = 0.083576;
    tri3_xyze(1, 2) = -0.0641301;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-538);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0729307;
    tri3_xyze(1, 0) = -0.0729307;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4865);
    tri3_xyze(0, 1) = 0.0773356;
    tri3_xyze(1, 1) = -0.0773356;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4863);
    tri3_xyze(0, 2) = 0.083576;
    tri3_xyze(1, 2) = -0.0641301;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-538);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0773356;
    tri3_xyze(1, 0) = -0.0773356;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4863);
    tri3_xyze(0, 1) = 0.0947164;
    tri3_xyze(1, 1) = -0.0546845;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(5113);
    tri3_xyze(0, 2) = 0.083576;
    tri3_xyze(1, 2) = -0.0641301;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-538);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0893214;
    tri3_xyze(1, 0) = -0.0515698;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(5115);
    tri3_xyze(0, 1) = 0.0838836;
    tri3_xyze(1, 1) = -0.0484302;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(5117);
    tri3_xyze(0, 2) = 0.0786566;
    tri3_xyze(1, 2) = -0.0603553;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-539);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0838836;
    tri3_xyze(1, 0) = -0.0484302;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(5117);
    tri3_xyze(0, 1) = 0.0684907;
    tri3_xyze(1, 1) = -0.0684907;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4867);
    tri3_xyze(0, 2) = 0.0786566;
    tri3_xyze(1, 2) = -0.0603553;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-539);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0684907;
    tri3_xyze(1, 0) = -0.0684907;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4867);
    tri3_xyze(0, 1) = 0.0729307;
    tri3_xyze(1, 1) = -0.0729307;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4865);
    tri3_xyze(0, 2) = 0.0786566;
    tri3_xyze(1, 2) = -0.0603553;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-539);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0729307;
    tri3_xyze(1, 0) = -0.0729307;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4865);
    tri3_xyze(0, 1) = 0.0893214;
    tri3_xyze(1, 1) = -0.0515698;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(5115);
    tri3_xyze(0, 2) = 0.0786566;
    tri3_xyze(1, 2) = -0.0603553;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-539);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0838836;
    tri3_xyze(1, 0) = -0.0484302;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(5117);
    tri3_xyze(0, 1) = 0.0784887;
    tri3_xyze(1, 1) = -0.0453155;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(5119);
    tri3_xyze(0, 2) = 0.0737372;
    tri3_xyze(1, 2) = -0.0565805;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-540);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0784887;
    tri3_xyze(1, 0) = -0.0453155;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(5119);
    tri3_xyze(0, 1) = 0.0640857;
    tri3_xyze(1, 1) = -0.0640857;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4869);
    tri3_xyze(0, 2) = 0.0737372;
    tri3_xyze(1, 2) = -0.0565805;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-540);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0640857;
    tri3_xyze(1, 0) = -0.0640857;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4869);
    tri3_xyze(0, 1) = 0.0684907;
    tri3_xyze(1, 1) = -0.0684907;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(4867);
    tri3_xyze(0, 2) = 0.0737372;
    tri3_xyze(1, 2) = -0.0565805;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-540);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0684907;
    tri3_xyze(1, 0) = -0.0684907;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(4867);
    tri3_xyze(0, 1) = 0.0838836;
    tri3_xyze(1, 1) = -0.0484302;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(5117);
    tri3_xyze(0, 2) = 0.0737372;
    tri3_xyze(1, 2) = -0.0565805;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-540);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0784887;
    tri3_xyze(1, 0) = -0.0453155;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(5119);
    tri3_xyze(0, 1) = 0.0732217;
    tri3_xyze(1, 1) = -0.0422746;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(5121);
    tri3_xyze(0, 2) = 0.0688954;
    tri3_xyze(1, 2) = -0.0528653;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-541);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0732217;
    tri3_xyze(1, 0) = -0.0422746;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(5121);
    tri3_xyze(0, 1) = 0.0597853;
    tri3_xyze(1, 1) = -0.0597853;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4871);
    tri3_xyze(0, 2) = 0.0688954;
    tri3_xyze(1, 2) = -0.0528653;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-541);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0597853;
    tri3_xyze(1, 0) = -0.0597853;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4871);
    tri3_xyze(0, 1) = 0.0640857;
    tri3_xyze(1, 1) = -0.0640857;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(4869);
    tri3_xyze(0, 2) = 0.0688954;
    tri3_xyze(1, 2) = -0.0528653;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-541);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0640857;
    tri3_xyze(1, 0) = -0.0640857;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(4869);
    tri3_xyze(0, 1) = 0.0784887;
    tri3_xyze(1, 1) = -0.0453155;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(5119);
    tri3_xyze(0, 2) = 0.0688954;
    tri3_xyze(1, 2) = -0.0528653;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-541);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0681658;
    tri3_xyze(1, 0) = -0.0393555;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(5123);
    tri3_xyze(0, 1) = 0.0556571;
    tri3_xyze(1, 1) = -0.0556571;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4873);
    tri3_xyze(0, 2) = 0.0642075;
    tri3_xyze(1, 2) = -0.0492681;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-542);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0556571;
    tri3_xyze(1, 0) = -0.0556571;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4873);
    tri3_xyze(0, 1) = 0.0597853;
    tri3_xyze(1, 1) = -0.0597853;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(4871);
    tri3_xyze(0, 2) = 0.0642075;
    tri3_xyze(1, 2) = -0.0492681;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-542);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0597853;
    tri3_xyze(1, 0) = -0.0597853;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(4871);
    tri3_xyze(0, 1) = 0.0732217;
    tri3_xyze(1, 1) = -0.0422746;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(5121);
    tri3_xyze(0, 2) = 0.0642075;
    tri3_xyze(1, 2) = -0.0492681;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-542);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0634006;
    tri3_xyze(1, 0) = -0.0366043;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(5125);
    tri3_xyze(0, 1) = 0.0517663;
    tri3_xyze(1, 1) = -0.0517663;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4875);
    tri3_xyze(0, 2) = 0.0597474;
    tri3_xyze(1, 2) = -0.0458458;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-543);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4875);
    tri3_xyze(0, 1) = 0.0556571;
    tri3_xyze(1, 1) = -0.0556571;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(4873);
    tri3_xyze(0, 2) = 0.0597474;
    tri3_xyze(1, 2) = -0.0458458;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-543);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0556571;
    tri3_xyze(1, 0) = -0.0556571;
    tri3_xyze(2, 0) = 0.845241;
    nids.push_back(4873);
    tri3_xyze(0, 1) = 0.0681658;
    tri3_xyze(1, 1) = -0.0393555;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(5123);
    tri3_xyze(0, 2) = 0.0597474;
    tri3_xyze(1, 2) = -0.0458458;
    tri3_xyze(2, 2) = 0.843729;
    nids.push_back(-543);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0590013;
    tri3_xyze(1, 0) = -0.0340644;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(5127);
    tri3_xyze(0, 1) = 0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4877);
    tri3_xyze(0, 2) = 0.0555856;
    tri3_xyze(1, 2) = -0.0426524;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-544);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0481743;
    tri3_xyze(1, 0) = -0.0481743;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4877);
    tri3_xyze(0, 1) = 0.0517663;
    tri3_xyze(1, 1) = -0.0517663;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(4875);
    tri3_xyze(0, 2) = 0.0555856;
    tri3_xyze(1, 2) = -0.0426524;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-544);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0517663;
    tri3_xyze(1, 0) = -0.0517663;
    tri3_xyze(2, 0) = 0.842216;
    nids.push_back(4875);
    tri3_xyze(0, 1) = 0.0634006;
    tri3_xyze(1, 1) = -0.0366043;
    tri3_xyze(2, 1) = 0.842216;
    nids.push_back(5125);
    tri3_xyze(0, 2) = 0.0555856;
    tri3_xyze(1, 2) = -0.0426524;
    tri3_xyze(2, 2) = 0.840371;
    nids.push_back(-544);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0550373;
    tri3_xyze(1, 0) = -0.0317758;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(5129);
    tri3_xyze(0, 1) = 0.0449377;
    tri3_xyze(1, 1) = -0.0449377;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4879);
    tri3_xyze(0, 2) = 0.0517877;
    tri3_xyze(1, 2) = -0.0397381;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-545);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0449377;
    tri3_xyze(1, 0) = -0.0449377;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4879);
    tri3_xyze(0, 1) = 0.0481743;
    tri3_xyze(1, 1) = -0.0481743;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(4877);
    tri3_xyze(0, 2) = 0.0517877;
    tri3_xyze(1, 2) = -0.0397381;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-545);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0481743;
    tri3_xyze(1, 0) = -0.0481743;
    tri3_xyze(2, 0) = 0.838526;
    nids.push_back(4877);
    tri3_xyze(0, 1) = 0.0590013;
    tri3_xyze(1, 1) = -0.0340644;
    tri3_xyze(2, 1) = 0.838526;
    nids.push_back(5127);
    tri3_xyze(0, 2) = 0.0517877;
    tri3_xyze(1, 2) = -0.0397381;
    tri3_xyze(2, 2) = 0.836377;
    nids.push_back(-545);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0550373;
    tri3_xyze(1, 0) = -0.0317758;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(5129);
    tri3_xyze(0, 1) = 0.0515711;
    tri3_xyze(1, 1) = -0.0297746;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(5131);
    tri3_xyze(0, 2) = 0.0484134;
    tri3_xyze(1, 2) = -0.0371489;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-546);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515711;
    tri3_xyze(1, 0) = -0.0297746;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(5131);
    tri3_xyze(0, 1) = 0.0421076;
    tri3_xyze(1, 1) = -0.0421076;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4881);
    tri3_xyze(0, 2) = 0.0484134;
    tri3_xyze(1, 2) = -0.0371489;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-546);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0421076;
    tri3_xyze(1, 0) = -0.0421076;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4881);
    tri3_xyze(0, 1) = 0.0449377;
    tri3_xyze(1, 1) = -0.0449377;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(4879);
    tri3_xyze(0, 2) = 0.0484134;
    tri3_xyze(1, 2) = -0.0371489;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-546);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0449377;
    tri3_xyze(1, 0) = -0.0449377;
    tri3_xyze(2, 0) = 0.834227;
    nids.push_back(4879);
    tri3_xyze(0, 1) = 0.0550373;
    tri3_xyze(1, 1) = -0.0317758;
    tri3_xyze(2, 1) = 0.834227;
    nids.push_back(5129);
    tri3_xyze(0, 2) = 0.0484134;
    tri3_xyze(1, 2) = -0.0371489;
    tri3_xyze(2, 2) = 0.831808;
    nids.push_back(-546);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515711;
    tri3_xyze(1, 0) = -0.0297746;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(5131);
    tri3_xyze(0, 1) = 0.0486573;
    tri3_xyze(1, 1) = -0.0280923;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(5133);
    tri3_xyze(0, 2) = 0.0455161;
    tri3_xyze(1, 2) = -0.0349258;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-547);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0486573;
    tri3_xyze(1, 0) = -0.0280923;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(5133);
    tri3_xyze(0, 1) = 0.0397286;
    tri3_xyze(1, 1) = -0.0397286;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4883);
    tri3_xyze(0, 2) = 0.0455161;
    tri3_xyze(1, 2) = -0.0349258;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-547);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0397286;
    tri3_xyze(1, 0) = -0.0397286;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4883);
    tri3_xyze(0, 1) = 0.0421076;
    tri3_xyze(1, 1) = -0.0421076;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(4881);
    tri3_xyze(0, 2) = 0.0455161;
    tri3_xyze(1, 2) = -0.0349258;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-547);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0421076;
    tri3_xyze(1, 0) = -0.0421076;
    tri3_xyze(2, 0) = 0.829389;
    nids.push_back(4881);
    tri3_xyze(0, 1) = 0.0515711;
    tri3_xyze(1, 1) = -0.0297746;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(5131);
    tri3_xyze(0, 2) = 0.0455161;
    tri3_xyze(1, 2) = -0.0349258;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-547);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0486573;
    tri3_xyze(1, 0) = -0.0280923;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(5133);
    tri3_xyze(0, 1) = 0.046342;
    tri3_xyze(1, 1) = -0.0267556;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(5135);
    tri3_xyze(0, 2) = 0.0431415;
    tri3_xyze(1, 2) = -0.0331036;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-548);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.046342;
    tri3_xyze(1, 0) = -0.0267556;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(5135);
    tri3_xyze(0, 1) = 0.0378381;
    tri3_xyze(1, 1) = -0.0378381;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4885);
    tri3_xyze(0, 2) = 0.0431415;
    tri3_xyze(1, 2) = -0.0331036;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-548);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0378381;
    tri3_xyze(1, 0) = -0.0378381;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4885);
    tri3_xyze(0, 1) = 0.0397286;
    tri3_xyze(1, 1) = -0.0397286;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(4883);
    tri3_xyze(0, 2) = 0.0431415;
    tri3_xyze(1, 2) = -0.0331036;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-548);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0397286;
    tri3_xyze(1, 0) = -0.0397286;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(4883);
    tri3_xyze(0, 1) = 0.0486573;
    tri3_xyze(1, 1) = -0.0280923;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(5133);
    tri3_xyze(0, 2) = 0.0431415;
    tri3_xyze(1, 2) = -0.0331036;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-548);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.046342;
    tri3_xyze(1, 0) = -0.0267556;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(5135);
    tri3_xyze(0, 1) = 0.0446617;
    tri3_xyze(1, 1) = -0.0257854;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5137);
    tri3_xyze(0, 2) = 0.041327;
    tri3_xyze(1, 2) = -0.0317113;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-549);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0446617;
    tri3_xyze(1, 0) = -0.0257854;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5137);
    tri3_xyze(0, 1) = 0.0364661;
    tri3_xyze(1, 1) = -0.0364661;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4887);
    tri3_xyze(0, 2) = 0.041327;
    tri3_xyze(1, 2) = -0.0317113;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-549);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0364661;
    tri3_xyze(1, 0) = -0.0364661;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4887);
    tri3_xyze(0, 1) = 0.0378381;
    tri3_xyze(1, 1) = -0.0378381;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(4885);
    tri3_xyze(0, 2) = 0.041327;
    tri3_xyze(1, 2) = -0.0317113;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-549);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0378381;
    tri3_xyze(1, 0) = -0.0378381;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(4885);
    tri3_xyze(0, 1) = 0.046342;
    tri3_xyze(1, 1) = -0.0267556;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(5135);
    tri3_xyze(0, 2) = 0.041327;
    tri3_xyze(1, 2) = -0.0317113;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-549);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0446617;
    tri3_xyze(1, 0) = -0.0257854;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5137);
    tri3_xyze(0, 1) = 0.0436427;
    tri3_xyze(1, 1) = -0.0251971;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5139);
    tri3_xyze(0, 2) = 0.0401011;
    tri3_xyze(1, 2) = -0.0307707;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-550);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0436427;
    tri3_xyze(1, 0) = -0.0251971;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5139);
    tri3_xyze(0, 1) = 0.0356341;
    tri3_xyze(1, 1) = -0.0356341;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(4889);
    tri3_xyze(0, 2) = 0.0401011;
    tri3_xyze(1, 2) = -0.0307707;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-550);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0356341;
    tri3_xyze(1, 0) = -0.0356341;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(4889);
    tri3_xyze(0, 1) = 0.0364661;
    tri3_xyze(1, 1) = -0.0364661;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(4887);
    tri3_xyze(0, 2) = 0.0401011;
    tri3_xyze(1, 2) = -0.0307707;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-550);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0364661;
    tri3_xyze(1, 0) = -0.0364661;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(4887);
    tri3_xyze(0, 1) = 0.0446617;
    tri3_xyze(1, 1) = -0.0257854;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5137);
    tri3_xyze(0, 2) = 0.0401011;
    tri3_xyze(1, 2) = -0.0307707;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-550);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0436427;
    tri3_xyze(1, 0) = -0.0251971;
    tri3_xyze(2, 0) = 0.793733;
    nids.push_back(5042);
    tri3_xyze(0, 1) = 0.0433013;
    tri3_xyze(1, 1) = -0.025;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5041);
    tri3_xyze(0, 2) = 0.0459793;
    tri3_xyze(1, 2) = -0.0190453;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-551);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0433013;
    tri3_xyze(1, 0) = -0.025;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(5041);
    tri3_xyze(0, 1) = 0.0482963;
    tri3_xyze(1, 1) = -0.012941;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5291);
    tri3_xyze(0, 2) = 0.0459793;
    tri3_xyze(1, 2) = -0.0190453;
    tri3_xyze(2, 2) = 0.796867;
    nids.push_back(-551);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0486771;
    tri3_xyze(1, 0) = -0.013043;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5389);
    tri3_xyze(0, 1) = 0.0482963;
    tri3_xyze(1, 1) = -0.012941;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5291);
    tri3_xyze(0, 2) = 0.0459793;
    tri3_xyze(1, 2) = -0.0190453;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-552);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0482963;
    tri3_xyze(1, 0) = -0.012941;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(5291);
    tri3_xyze(0, 1) = 0.0433013;
    tri3_xyze(1, 1) = -0.025;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5041);
    tri3_xyze(0, 2) = 0.0459793;
    tri3_xyze(1, 2) = -0.0190453;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-552);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0433013;
    tri3_xyze(1, 0) = -0.025;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(5041);
    tri3_xyze(0, 1) = 0.0436427;
    tri3_xyze(1, 1) = -0.0251971;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5139);
    tri3_xyze(0, 2) = 0.0459793;
    tri3_xyze(1, 2) = -0.0190453;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-552);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0436427;
    tri3_xyze(1, 0) = -0.0251971;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5139);
    tri3_xyze(0, 1) = 0.0486771;
    tri3_xyze(1, 1) = -0.013043;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5389);
    tri3_xyze(0, 2) = 0.0459793;
    tri3_xyze(1, 2) = -0.0190453;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-552);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.111517;
    tri3_xyze(1, 0) = -0.0298809;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(5361);
    tri3_xyze(0, 1) = 0.0999834;
    tri3_xyze(1, 1) = -0.0577254;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(5111);
    tri3_xyze(0, 2) = 0.108424;
    tri3_xyze(1, 2) = -0.0449107;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-586);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0999834;
    tri3_xyze(1, 0) = -0.0577254;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(5111);
    tri3_xyze(0, 1) = 0.105039;
    tri3_xyze(1, 1) = -0.0606445;
    tri3_xyze(2, 1) = 0.845241;
    nids.push_back(5109);
    tri3_xyze(0, 2) = 0.108424;
    tri3_xyze(1, 2) = -0.0449107;
    tri3_xyze(2, 2) = 0.846397;
    nids.push_back(-586);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.105642;
    tri3_xyze(1, 0) = -0.0283068;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(5363);
    tri3_xyze(0, 1) = 0.0947164;
    tri3_xyze(1, 1) = -0.0546845;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(5113);
    tri3_xyze(0, 2) = 0.102965;
    tri3_xyze(1, 2) = -0.0426494;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-587);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0947164;
    tri3_xyze(1, 0) = -0.0546845;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(5113);
    tri3_xyze(0, 1) = 0.0999834;
    tri3_xyze(1, 1) = -0.0577254;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(5111);
    tri3_xyze(0, 2) = 0.102965;
    tri3_xyze(1, 2) = -0.0426494;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-587);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0999834;
    tri3_xyze(1, 0) = -0.0577254;
    tri3_xyze(2, 0) = 0.847553;
    nids.push_back(5111);
    tri3_xyze(0, 1) = 0.111517;
    tri3_xyze(1, 1) = -0.0298809;
    tri3_xyze(2, 1) = 0.847553;
    nids.push_back(5361);
    tri3_xyze(0, 2) = 0.102965;
    tri3_xyze(1, 2) = -0.0426494;
    tri3_xyze(2, 2) = 0.848334;
    nids.push_back(-587);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0996251;
    tri3_xyze(1, 0) = -0.0266945;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(5365);
    tri3_xyze(0, 1) = 0.0893214;
    tri3_xyze(1, 1) = -0.0515698;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(5115);
    tri3_xyze(0, 2) = 0.0973263;
    tri3_xyze(1, 2) = -0.0403139;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-588);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0893214;
    tri3_xyze(1, 0) = -0.0515698;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(5115);
    tri3_xyze(0, 1) = 0.0947164;
    tri3_xyze(1, 1) = -0.0546845;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(5113);
    tri3_xyze(0, 2) = 0.0973263;
    tri3_xyze(1, 2) = -0.0403139;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-588);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0947164;
    tri3_xyze(1, 0) = -0.0546845;
    tri3_xyze(2, 0) = 0.849114;
    nids.push_back(5113);
    tri3_xyze(0, 1) = 0.105642;
    tri3_xyze(1, 1) = -0.0283068;
    tri3_xyze(2, 1) = 0.849114;
    nids.push_back(5363);
    tri3_xyze(0, 2) = 0.0973263;
    tri3_xyze(1, 2) = -0.0403139;
    tri3_xyze(2, 2) = 0.849508;
    nids.push_back(-588);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0838836;
    tri3_xyze(1, 0) = -0.0484302;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(5117);
    tri3_xyze(0, 1) = 0.0893214;
    tri3_xyze(1, 1) = -0.0515698;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(5115);
    tri3_xyze(0, 2) = 0.0915976;
    tri3_xyze(1, 2) = -0.037941;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-589);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0893214;
    tri3_xyze(1, 0) = -0.0515698;
    tri3_xyze(2, 0) = 0.849901;
    nids.push_back(5115);
    tri3_xyze(0, 1) = 0.0996251;
    tri3_xyze(1, 1) = -0.0266945;
    tri3_xyze(2, 1) = 0.849901;
    nids.push_back(5365);
    tri3_xyze(0, 2) = 0.0915976;
    tri3_xyze(1, 2) = -0.037941;
    tri3_xyze(2, 2) = 0.849901;
    nids.push_back(-589);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0542702;
    tri3_xyze(1, 0) = -0.0145417;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(5383);
    tri3_xyze(0, 1) = 0.0486573;
    tri3_xyze(1, 1) = -0.0280923;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(5133);
    tri3_xyze(0, 2) = 0.0530047;
    tri3_xyze(1, 2) = -0.0219553;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-597);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0486573;
    tri3_xyze(1, 0) = -0.0280923;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(5133);
    tri3_xyze(0, 1) = 0.0515711;
    tri3_xyze(1, 1) = -0.0297746;
    tri3_xyze(2, 1) = 0.829389;
    nids.push_back(5131);
    tri3_xyze(0, 2) = 0.0530047;
    tri3_xyze(1, 2) = -0.0219553;
    tri3_xyze(2, 2) = 0.826738;
    nids.push_back(-597);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0516878;
    tri3_xyze(1, 0) = -0.0138497;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(5385);
    tri3_xyze(0, 1) = 0.046342;
    tri3_xyze(1, 1) = -0.0267556;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(5135);
    tri3_xyze(0, 2) = 0.0502394;
    tri3_xyze(1, 2) = -0.0208098;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-598);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.046342;
    tri3_xyze(1, 0) = -0.0267556;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(5135);
    tri3_xyze(0, 1) = 0.0486573;
    tri3_xyze(1, 1) = -0.0280923;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(5133);
    tri3_xyze(0, 2) = 0.0502394;
    tri3_xyze(1, 2) = -0.0208098;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-598);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0486573;
    tri3_xyze(1, 0) = -0.0280923;
    tri3_xyze(2, 0) = 0.824088;
    nids.push_back(5133);
    tri3_xyze(0, 1) = 0.0542702;
    tri3_xyze(1, 1) = -0.0145417;
    tri3_xyze(2, 1) = 0.824088;
    nids.push_back(5383);
    tri3_xyze(0, 2) = 0.0502394;
    tri3_xyze(1, 2) = -0.0208098;
    tri3_xyze(2, 2) = 0.821247;
    nids.push_back(-598);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0516878;
    tri3_xyze(1, 0) = -0.0138497;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(5385);
    tri3_xyze(0, 1) = 0.0498136;
    tri3_xyze(1, 1) = -0.0133475;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5387);
    tri3_xyze(0, 2) = 0.0481263;
    tri3_xyze(1, 2) = -0.0199346;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-599);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0498136;
    tri3_xyze(1, 0) = -0.0133475;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5387);
    tri3_xyze(0, 1) = 0.0446617;
    tri3_xyze(1, 1) = -0.0257854;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5137);
    tri3_xyze(0, 2) = 0.0481263;
    tri3_xyze(1, 2) = -0.0199346;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-599);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0446617;
    tri3_xyze(1, 0) = -0.0257854;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5137);
    tri3_xyze(0, 1) = 0.046342;
    tri3_xyze(1, 1) = -0.0267556;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(5135);
    tri3_xyze(0, 2) = 0.0481263;
    tri3_xyze(1, 2) = -0.0199346;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-599);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.046342;
    tri3_xyze(1, 0) = -0.0267556;
    tri3_xyze(2, 0) = 0.818406;
    nids.push_back(5135);
    tri3_xyze(0, 1) = 0.0516878;
    tri3_xyze(1, 1) = -0.0138497;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(5385);
    tri3_xyze(0, 2) = 0.0481263;
    tri3_xyze(1, 2) = -0.0199346;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-599);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0498136;
    tri3_xyze(1, 0) = -0.0133475;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5387);
    tri3_xyze(0, 1) = 0.0486771;
    tri3_xyze(1, 1) = -0.013043;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5389);
    tri3_xyze(0, 2) = 0.0466988;
    tri3_xyze(1, 2) = -0.0193433;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-600);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0486771;
    tri3_xyze(1, 0) = -0.013043;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5389);
    tri3_xyze(0, 1) = 0.0436427;
    tri3_xyze(1, 1) = -0.0251971;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5139);
    tri3_xyze(0, 2) = 0.0466988;
    tri3_xyze(1, 2) = -0.0193433;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-600);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0436427;
    tri3_xyze(1, 0) = -0.0251971;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5139);
    tri3_xyze(0, 1) = 0.0446617;
    tri3_xyze(1, 1) = -0.0257854;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5137);
    tri3_xyze(0, 2) = 0.0466988;
    tri3_xyze(1, 2) = -0.0193433;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-600);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0446617;
    tri3_xyze(1, 0) = -0.0257854;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5137);
    tri3_xyze(0, 1) = 0.0498136;
    tri3_xyze(1, 1) = -0.0133475;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5387);
    tri3_xyze(0, 2) = 0.0466988;
    tri3_xyze(1, 2) = -0.0193433;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-600);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0503943;
    tri3_xyze(1, 0) = 1.96873e-17;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5639);
    tri3_xyze(0, 1) = 0.05;
    tri3_xyze(1, 1) = 0;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5541);
    tri3_xyze(0, 2) = 0.0493419;
    tri3_xyze(1, 2) = -0.00649599;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-602);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.05;
    tri3_xyze(1, 0) = 0;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(5541);
    tri3_xyze(0, 1) = 0.0482963;
    tri3_xyze(1, 1) = -0.012941;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5291);
    tri3_xyze(0, 2) = 0.0493419;
    tri3_xyze(1, 2) = -0.00649599;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-602);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0482963;
    tri3_xyze(1, 0) = -0.012941;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(5291);
    tri3_xyze(0, 1) = 0.0486771;
    tri3_xyze(1, 1) = -0.013043;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5389);
    tri3_xyze(0, 2) = 0.0493419;
    tri3_xyze(1, 2) = -0.00649599;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-602);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0486771;
    tri3_xyze(1, 0) = -0.013043;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5389);
    tri3_xyze(0, 1) = 0.0503943;
    tri3_xyze(1, 1) = 1.96873e-17;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5639);
    tri3_xyze(0, 2) = 0.0493419;
    tri3_xyze(1, 2) = -0.00649599;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-602);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0515708;
    tri3_xyze(1, 0) = 3.90641e-17;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5637);
    tri3_xyze(0, 1) = 0.0498136;
    tri3_xyze(1, 1) = -0.0133475;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5387);
    tri3_xyze(0, 2) = 0.0516459;
    tri3_xyze(1, 2) = -0.00679931;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-649);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0498136;
    tri3_xyze(1, 0) = -0.0133475;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5387);
    tri3_xyze(0, 1) = 0.0516878;
    tri3_xyze(1, 1) = -0.0138497;
    tri3_xyze(2, 1) = 0.818406;
    nids.push_back(5385);
    tri3_xyze(0, 2) = 0.0516459;
    tri3_xyze(1, 2) = -0.00679931;
    tri3_xyze(2, 2) = 0.81542;
    nids.push_back(-649);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0503943;
    tri3_xyze(1, 0) = 1.96873e-17;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5639);
    tri3_xyze(0, 1) = 0.0486771;
    tri3_xyze(1, 1) = -0.013043;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5389);
    tri3_xyze(0, 2) = 0.050114;
    tri3_xyze(1, 2) = -0.00659763;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-650);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0486771;
    tri3_xyze(1, 0) = -0.013043;
    tri3_xyze(2, 0) = 0.806267;
    nids.push_back(5389);
    tri3_xyze(0, 1) = 0.0498136;
    tri3_xyze(1, 1) = -0.0133475;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5387);
    tri3_xyze(0, 2) = 0.050114;
    tri3_xyze(1, 2) = -0.00659763;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-650);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0498136;
    tri3_xyze(1, 0) = -0.0133475;
    tri3_xyze(2, 0) = 0.812434;
    nids.push_back(5387);
    tri3_xyze(0, 1) = 0.0515708;
    tri3_xyze(1, 1) = 3.90641e-17;
    tri3_xyze(2, 1) = 0.812434;
    nids.push_back(5637);
    tri3_xyze(0, 2) = 0.050114;
    tri3_xyze(1, 2) = -0.00659763;
    tri3_xyze(2, 2) = 0.809351;
    nids.push_back(-650);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.0482963;
    tri3_xyze(1, 0) = 0.012941;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(8291);
    tri3_xyze(0, 1) = 0.05;
    tri3_xyze(1, 1) = 0;
    tri3_xyze(2, 1) = 0.8;
    nids.push_back(5541);
    tri3_xyze(0, 2) = 0.0493419;
    tri3_xyze(1, 2) = 0.00649599;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-652);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix tri3_xyze(3, 3);

    nids.clear();
    tri3_xyze(0, 0) = 0.05;
    tri3_xyze(1, 0) = 0;
    tri3_xyze(2, 0) = 0.8;
    nids.push_back(5541);
    tri3_xyze(0, 1) = 0.0503943;
    tri3_xyze(1, 1) = 1.96873e-17;
    tri3_xyze(2, 1) = 0.806267;
    nids.push_back(5639);
    tri3_xyze(0, 2) = 0.0493419;
    tri3_xyze(1, 2) = 0.00649599;
    tri3_xyze(2, 2) = 0.803133;
    nids.push_back(-652);
    intersection.add_cut_side(++sidecount, nids, tri3_xyze, Core::FE::CellType::tri3);
  }
  {
    Core::LinAlg::SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.05;
    hex8_xyze(1, 0) = -0.1;
    hex8_xyze(2, 0) = 0.75;
    nids.push_back(1862);
    hex8_xyze(0, 1) = 0.05;
    hex8_xyze(1, 1) = -0.05;
    hex8_xyze(2, 1) = 0.75;
    nids.push_back(1863);
    hex8_xyze(0, 2) = 0;
    hex8_xyze(1, 2) = -0.05;
    hex8_xyze(2, 2) = 0.75;
    nids.push_back(1874);
    hex8_xyze(0, 3) = 0;
    hex8_xyze(1, 3) = -0.1;
    hex8_xyze(2, 3) = 0.75;
    nids.push_back(1873);
    hex8_xyze(0, 4) = 0.05;
    hex8_xyze(1, 4) = -0.1;
    hex8_xyze(2, 4) = 0.8;
    nids.push_back(1983);
    hex8_xyze(0, 5) = 0.05;
    hex8_xyze(1, 5) = -0.05;
    hex8_xyze(2, 5) = 0.8;
    nids.push_back(1984);
    hex8_xyze(0, 6) = 0;
    hex8_xyze(1, 6) = -0.05;
    hex8_xyze(2, 6) = 0.8;
    nids.push_back(1995);
    hex8_xyze(0, 7) = 0;
    hex8_xyze(1, 7) = -0.1;
    hex8_xyze(2, 7) = 0.8;
    nids.push_back(1994);

    intersection.add_element(6919, nids, hex8_xyze, Core::FE::CellType::hex8);
  }

  {
    Core::LinAlg::SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.1;
    hex8_xyze(1, 0) = -0.1;
    hex8_xyze(2, 0) = 0.8;
    nids.push_back(1972);
    hex8_xyze(0, 1) = 0.1;
    hex8_xyze(1, 1) = -0.05;
    hex8_xyze(2, 1) = 0.8;
    nids.push_back(1973);
    hex8_xyze(0, 2) = 0.05;
    hex8_xyze(1, 2) = -0.05;
    hex8_xyze(2, 2) = 0.8;
    nids.push_back(1984);
    hex8_xyze(0, 3) = 0.05;
    hex8_xyze(1, 3) = -0.1;
    hex8_xyze(2, 3) = 0.8;
    nids.push_back(1983);
    hex8_xyze(0, 4) = 0.1;
    hex8_xyze(1, 4) = -0.1;
    hex8_xyze(2, 4) = 0.85;
    nids.push_back(2093);
    hex8_xyze(0, 5) = 0.1;
    hex8_xyze(1, 5) = -0.05;
    hex8_xyze(2, 5) = 0.85;
    nids.push_back(2094);
    hex8_xyze(0, 6) = 0.05;
    hex8_xyze(1, 6) = -0.05;
    hex8_xyze(2, 6) = 0.85;
    nids.push_back(2105);
    hex8_xyze(0, 7) = 0.05;
    hex8_xyze(1, 7) = -0.1;
    hex8_xyze(2, 7) = 0.85;
    nids.push_back(2104);

    intersection.add_element(7009, nids, hex8_xyze, Core::FE::CellType::hex8);
  }

  {
    Core::LinAlg::SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.05;
    hex8_xyze(1, 0) = -0.15;
    hex8_xyze(2, 0) = 0.8;
    nids.push_back(1982);
    hex8_xyze(0, 1) = 0.05;
    hex8_xyze(1, 1) = -0.1;
    hex8_xyze(2, 1) = 0.8;
    nids.push_back(1983);
    hex8_xyze(0, 2) = 0;
    hex8_xyze(1, 2) = -0.1;
    hex8_xyze(2, 2) = 0.8;
    nids.push_back(1994);
    hex8_xyze(0, 3) = 1.1091e-15;
    hex8_xyze(1, 3) = -0.15;
    hex8_xyze(2, 3) = 0.8;
    nids.push_back(1993);
    hex8_xyze(0, 4) = 0.05;
    hex8_xyze(1, 4) = -0.15;
    hex8_xyze(2, 4) = 0.85;
    nids.push_back(2103);
    hex8_xyze(0, 5) = 0.05;
    hex8_xyze(1, 5) = -0.1;
    hex8_xyze(2, 5) = 0.85;
    nids.push_back(2104);
    hex8_xyze(0, 6) = 0;
    hex8_xyze(1, 6) = -0.1;
    hex8_xyze(2, 6) = 0.85;
    nids.push_back(2115);
    hex8_xyze(0, 7) = 0;
    hex8_xyze(1, 7) = -0.15;
    hex8_xyze(2, 7) = 0.85;
    nids.push_back(2114);

    intersection.add_element(7018, nids, hex8_xyze, Core::FE::CellType::hex8);
  }

  {
    Core::LinAlg::SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.05;
    hex8_xyze(1, 0) = -0.1;
    hex8_xyze(2, 0) = 0.8;
    nids.push_back(1983);
    hex8_xyze(0, 1) = 0.05;
    hex8_xyze(1, 1) = -0.05;
    hex8_xyze(2, 1) = 0.8;
    nids.push_back(1984);
    hex8_xyze(0, 2) = 0;
    hex8_xyze(1, 2) = -0.05;
    hex8_xyze(2, 2) = 0.8;
    nids.push_back(1995);
    hex8_xyze(0, 3) = 0;
    hex8_xyze(1, 3) = -0.1;
    hex8_xyze(2, 3) = 0.8;
    nids.push_back(1994);
    hex8_xyze(0, 4) = 0.05;
    hex8_xyze(1, 4) = -0.1;
    hex8_xyze(2, 4) = 0.85;
    nids.push_back(2104);
    hex8_xyze(0, 5) = 0.05;
    hex8_xyze(1, 5) = -0.05;
    hex8_xyze(2, 5) = 0.85;
    nids.push_back(2105);
    hex8_xyze(0, 6) = 0;
    hex8_xyze(1, 6) = -0.05;
    hex8_xyze(2, 6) = 0.85;
    nids.push_back(2116);
    hex8_xyze(0, 7) = 0;
    hex8_xyze(1, 7) = -0.1;
    hex8_xyze(2, 7) = 0.85;
    nids.push_back(2115);

    intersection.add_element(7019, nids, hex8_xyze, Core::FE::CellType::hex8);
  }

  {
    Core::LinAlg::SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0.05;
    hex8_xyze(1, 0) = -0.05;
    hex8_xyze(2, 0) = 0.8;
    nids.push_back(1984);
    hex8_xyze(0, 1) = 0.05;
    hex8_xyze(1, 1) = -2.08167e-18;
    hex8_xyze(2, 1) = 0.8;
    nids.push_back(1985);
    hex8_xyze(0, 2) = 0;
    hex8_xyze(1, 2) = 0;
    hex8_xyze(2, 2) = 0.8;
    nids.push_back(1996);
    hex8_xyze(0, 3) = 0;
    hex8_xyze(1, 3) = -0.05;
    hex8_xyze(2, 3) = 0.8;
    nids.push_back(1995);
    hex8_xyze(0, 4) = 0.05;
    hex8_xyze(1, 4) = -0.05;
    hex8_xyze(2, 4) = 0.85;
    nids.push_back(2105);
    hex8_xyze(0, 5) = 0.05;
    hex8_xyze(1, 5) = -2.42861e-18;
    hex8_xyze(2, 5) = 0.85;
    nids.push_back(2106);
    hex8_xyze(0, 6) = 0;
    hex8_xyze(1, 6) = 0;
    hex8_xyze(2, 6) = 0.85;
    nids.push_back(2117);
    hex8_xyze(0, 7) = 0;
    hex8_xyze(1, 7) = -0.05;
    hex8_xyze(2, 7) = 0.85;
    nids.push_back(2116);

    intersection.add_element(7020, nids, hex8_xyze, Core::FE::CellType::hex8);
  }

  {
    Core::LinAlg::SerialDenseMatrix hex8_xyze(3, 8);

    nids.clear();
    hex8_xyze(0, 0) = 0;
    hex8_xyze(1, 0) = -0.1;
    hex8_xyze(2, 0) = 0.8;
    nids.push_back(1994);
    hex8_xyze(0, 1) = 0;
    hex8_xyze(1, 1) = -0.05;
    hex8_xyze(2, 1) = 0.8;
    nids.push_back(1995);
    hex8_xyze(0, 2) = -0.05;
    hex8_xyze(1, 2) = -0.05;
    hex8_xyze(2, 2) = 0.8;
    nids.push_back(2006);
    hex8_xyze(0, 3) = -0.05;
    hex8_xyze(1, 3) = -0.1;
    hex8_xyze(2, 3) = 0.8;
    nids.push_back(2005);
    hex8_xyze(0, 4) = 0;
    hex8_xyze(1, 4) = -0.1;
    hex8_xyze(2, 4) = 0.85;
    nids.push_back(2115);
    hex8_xyze(0, 5) = 0;
    hex8_xyze(1, 5) = -0.05;
    hex8_xyze(2, 5) = 0.85;
    nids.push_back(2116);
    hex8_xyze(0, 6) = -0.05;
    hex8_xyze(1, 6) = -0.05;
    hex8_xyze(2, 6) = 0.85;
    nids.push_back(2127);
    hex8_xyze(0, 7) = -0.05;
    hex8_xyze(1, 7) = -0.1;
    hex8_xyze(2, 7) = 0.85;
    nids.push_back(2126);

    intersection.add_element(7029, nids, hex8_xyze, Core::FE::CellType::hex8);
  }


  intersection.cut_test_cut(
      true, Cut::VCellGaussPts_DirectDivergence, Cut::BCellGaussPts_Tessellation);
  intersection.cut_finalize(
      true, Cut::VCellGaussPts_DirectDivergence, Cut::BCellGaussPts_Tessellation, false, true);
}
