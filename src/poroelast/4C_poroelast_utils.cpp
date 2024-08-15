/*----------------------------------------------------------------------*/
/*! \file

 \brief utility functions for porous media problems

\level 2

 */

#include "4C_poroelast_utils.hpp"

#include "4C_fem_condition_utils.hpp"
#include "4C_fem_discretization_faces.hpp"
#include "4C_fem_general_element_center.hpp"
#include "4C_fluid_ele_poro.hpp"
#include "4C_linalg_utils_densematrix_communication.hpp"
#include "4C_linalg_utils_sparse_algebra_create.hpp"
#include "4C_mat_fluidporo.hpp"
#include "4C_mat_structporo.hpp"
#include "4C_poroelast_base.hpp"
#include "4C_poroelast_monolithic.hpp"
#include "4C_poroelast_monolithicfluidsplit.hpp"
#include "4C_poroelast_monolithicmeshtying.hpp"
#include "4C_poroelast_monolithicsplit_nopenetration.hpp"
#include "4C_poroelast_monolithicstructuresplit.hpp"
#include "4C_poroelast_partitioned.hpp"
#include "4C_poroelast_utils_clonestrategy.hpp"
#include "4C_so3_poro_eletypes.hpp"
#include "4C_so3_poro_p1_eletypes.hpp"
#include "4C_solid_poro_3D_ele_pressure_based.hpp"
#include "4C_solid_poro_3D_ele_pressure_velocity_based.hpp"
#include "4C_w1_poro_eletypes.hpp"
#include "4C_w1_poro_p1_eletypes.hpp"

FOUR_C_NAMESPACE_OPEN

bool PoroElast::UTILS::IsPoroElement(const Core::Elements::Element* actele)
{
  // all poro elements need to be listed here
  return actele->element_type() == Discret::ELEMENTS::SoHex8PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::SolidPoroPressureBasedType::instance() or
         actele->element_type() ==
             Discret::ELEMENTS::SolidPoroPressureVelocityBasedType::instance() or
         actele->element_type() == Discret::ELEMENTS::SoTet4PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::SoTet10PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::SoHex27PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::SoNurbs27PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::WallTri3PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::WallQuad4PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::WallQuad9PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::WallNurbs4PoroType::instance() or
         actele->element_type() == Discret::ELEMENTS::WallNurbs9PoroType::instance() or
         IsPoroP1Element(actele);
}

bool PoroElast::UTILS::IsPoroP1Element(const Core::Elements::Element* actele)
{
  // all poro-p1 elements need to be listed here
  return actele->element_type() == Discret::ELEMENTS::SoHex8PoroP1Type::instance() or
         actele->element_type() == Discret::ELEMENTS::SoTet4PoroP1Type::instance() or
         actele->element_type() == Discret::ELEMENTS::WallQuad4PoroP1Type::instance() or
         actele->element_type() == Discret::ELEMENTS::WallTri3PoroP1Type::instance() or
         actele->element_type() == Discret::ELEMENTS::WallQuad9PoroP1Type::instance();
}

Teuchos::RCP<PoroElast::PoroBase> PoroElast::UTILS::CreatePoroAlgorithm(
    const Teuchos::ParameterList& timeparams, const Epetra_Comm& comm, bool setup_solver,
    Teuchos::RCP<Core::LinAlg::MapExtractor> porosity_splitter)
{
  Global::Problem* problem = Global::Problem::instance();

  // access the problem-specific parameter list
  const Teuchos::ParameterList& poroelastdyn = problem->poroelast_dynamic_params();

  //  problem->mortar_coupling_params()
  const auto coupling = Core::UTILS::IntegralValue<Inpar::PoroElast::SolutionSchemeOverFields>(
      poroelastdyn, "COUPALGO");

  // create an empty Poroelast::Algorithm instance
  Teuchos::RCP<PoroElast::PoroBase> poroalgo = Teuchos::null;

  switch (coupling)
  {
    case Inpar::PoroElast::Monolithic:
    {
      // create an PoroElast::Monolithic instance
      poroalgo = Teuchos::rcp(new PoroElast::Monolithic(comm, timeparams, porosity_splitter));
      break;
    }  // monolithic case
    case Inpar::PoroElast::Monolithic_structuresplit:
    {
      // create an PoroElast::MonolithicStructureSplit instance
      poroalgo = Teuchos::rcp(
          new PoroElast::MonolithicStructureSplit(comm, timeparams, porosity_splitter));
      break;
    }
    case Inpar::PoroElast::Monolithic_fluidsplit:
    {
      // create an PoroElast::MonolithicFluidSplit instance
      poroalgo =
          Teuchos::rcp(new PoroElast::MonolithicFluidSplit(comm, timeparams, porosity_splitter));
      break;
    }
    case Inpar::PoroElast::Monolithic_nopenetrationsplit:
    {
      // create an PoroElast::MonolithicSplitNoPenetration instance
      poroalgo = Teuchos::rcp(
          new PoroElast::MonolithicSplitNoPenetration(comm, timeparams, porosity_splitter));
      break;
    }
    case Inpar::PoroElast::Partitioned:
    {
      // create an PoroElast::Partitioned instance
      poroalgo = Teuchos::rcp(new PoroElast::Partitioned(comm, timeparams, porosity_splitter));
      break;
    }
    case Inpar::PoroElast::Monolithic_meshtying:
    {
      // create an PoroElast::MonolithicMeshtying instance
      poroalgo =
          Teuchos::rcp(new PoroElast::MonolithicMeshtying(comm, timeparams, porosity_splitter));
      break;
    }
    default:
      FOUR_C_THROW("Unknown solutiontype for poroelasticity: %d", coupling);
      break;
  }

  // setup solver (if needed)
  if (setup_solver) poroalgo->setup_solver();

  return poroalgo;
}


Teuchos::RCP<Core::LinAlg::MapExtractor> PoroElast::UTILS::BuildPoroSplitter(
    Teuchos::RCP<Core::FE::Discretization> dis)
{
  Teuchos::RCP<Core::LinAlg::MapExtractor> porositysplitter = Teuchos::null;

  // Loop through all elements on processor
  int locporop1 = std::count_if(
      dis->my_col_element_range().begin(), dis->my_col_element_range().end(), IsPoroP1Element);

  // Was at least one PoroP1 found on one processor?
  int glonumporop1 = 0;
  dis->get_comm().MaxAll(&locporop1, &glonumporop1, 1);
  // Yes, it was. Go ahead for all processors (even if they do not carry any PoroP1 elements)
  if (glonumporop1 > 0)
  {
    porositysplitter = Teuchos::rcp(new Core::LinAlg::MapExtractor());
    const int ndim = Global::Problem::instance()->n_dim();
    Core::LinAlg::CreateMapExtractorFromDiscretization(*dis, ndim, *porositysplitter);
  }

  return porositysplitter;
}

void PoroElast::UTILS::SetMaterialPointersMatchingGrid(
    Teuchos::RCP<const Core::FE::Discretization> sourcedis,
    Teuchos::RCP<const Core::FE::Discretization> targetdis)
{
  const int numelements = targetdis->num_my_col_elements();

  for (int i = 0; i < numelements; ++i)
  {
    Core::Elements::Element* targetele = targetdis->l_col_element(i);
    const int gid = targetele->id();

    Core::Elements::Element* sourceele = sourcedis->g_element(gid);

    // for coupling we add the source material to the target element and vice versa
    targetele->add_material(sourceele->material());
    sourceele->add_material(targetele->material());
  }
}

void PoroElast::UTILS::create_volume_ghosting(Core::FE::Discretization& idiscret)
{
  //**********************************************************************
  // Prerequisites of this funtion:
  // All Contact Elements need a set parent_id_ (member of faceelement!) before
  // calling CreateInterfaceGhosting as this id will be communicated to all
  // processors! Otherwise any information which connects face and volume
  // element is lost! (Parent Element Pointer is not communicated)
  //**********************************************************************

  // We get the discretizations from the global problem, as the contact does not have
  // both structural and porofluid discretization, but we should guarantee consistent ghosting!

  Global::Problem* problem = Global::Problem::instance();

  std::vector<Teuchos::RCP<Core::FE::Discretization>> voldis;
  voldis.push_back(problem->get_dis("structure"));
  voldis.push_back(problem->get_dis("porofluid"));

  const Epetra_Map* ielecolmap = idiscret.element_col_map();

  for (auto& voldi : voldis)
  {
    // 1 Ghost all Volume Element + Nodes,for all ghosted mortar elements!
    std::vector<int> rdata;

    // Fill rdata with existing colmap

    const Epetra_Map* elecolmap = voldi->element_col_map();
    const Teuchos::RCP<Epetra_Map> allredelecolmap =
        Core::LinAlg::AllreduceEMap(*voldi->element_row_map());

    for (int i = 0; i < elecolmap->NumMyElements(); ++i)
    {
      int gid = elecolmap->GID(i);
      rdata.push_back(gid);
    }

    // Find elements, which are ghosted on the interface but not in the volume discretization
    for (int i = 0; i < ielecolmap->NumMyElements(); ++i)
    {
      int gid = ielecolmap->GID(i);

      Core::Elements::Element* ele = idiscret.g_element(gid);
      if (!ele) FOUR_C_THROW("ERROR: Cannot find element with gid %", gid);
      auto* faceele = dynamic_cast<Core::Elements::FaceElement*>(ele);

      int volgid = 0;
      if (!faceele)
        FOUR_C_THROW("Cast to FaceElement failed!");
      else
        volgid = faceele->parent_element_id();

      // Ghost the parent element additionally
      if (elecolmap->LID(volgid) == -1 &&
          allredelecolmap->LID(volgid) !=
              -1)  // Volume discretization has not Element on this proc but on another
        rdata.push_back(volgid);
    }

    // re-build element column map
    Teuchos::RCP<Epetra_Map> newelecolmap = Teuchos::rcp(
        new Epetra_Map(-1, static_cast<int>(rdata.size()), rdata.data(), 0, voldi->get_comm()));
    rdata.clear();

    // redistribute the volume discretization according to the
    // new (=old) element column layout & and ghost also nodes!
    voldi->extended_ghosting(*newelecolmap, true, true, true, false);  // no check!!!
  }

  // 2 Material pointers need to be reset after redistribution.
  PoroElast::UTILS::SetMaterialPointersMatchingGrid(voldis[0], voldis[1]);

  // 3 Reconnect Face Element -- Porostructural Parent Element Pointers!
  PoroElast::UTILS::reconnect_parent_pointers(idiscret, *voldis[0], &(*voldis[1]));

  // 4 In case we use
  Teuchos::RCP<Core::FE::DiscretizationFaces> facediscret =
      Teuchos::rcp_dynamic_cast<Core::FE::DiscretizationFaces>(voldis[1]);
  if (facediscret != Teuchos::null) facediscret->fill_complete_faces(true, true, true, true);
}

void PoroElast::UTILS::reconnect_parent_pointers(Core::FE::Discretization& idiscret,
    Core::FE::Discretization& voldiscret, Core::FE::Discretization* voldiscret2)
{
  const Epetra_Map* ielecolmap = idiscret.element_col_map();
  const Epetra_Map* elecolmap = voldiscret.element_col_map();

  for (int i = 0; i < ielecolmap->NumMyElements(); ++i)
  {
    int gid = ielecolmap->GID(i);

    Core::Elements::Element* ele = idiscret.g_element(gid);
    if (!ele) FOUR_C_THROW("ERROR: Cannot find element with gid %", gid);

    auto* faceele = dynamic_cast<Core::Elements::FaceElement*>(ele);

    if (!faceele) FOUR_C_THROW("Cast to FaceElement failed!");
    SetSlaveAndMaster(voldiscret, voldiscret2, elecolmap, faceele);
  }
}

void PoroElast::UTILS::SetSlaveAndMaster(const Core::FE::Discretization& voldiscret,
    const Core::FE::Discretization* voldiscret2, const Epetra_Map* elecolmap,
    Core::Elements::FaceElement* faceele)
{
  int volgid = faceele->parent_element_id();

  if (elecolmap->LID(volgid) == -1)  // Volume discretization has not Element
    FOUR_C_THROW("create_volume_ghosting: Element %d does not exist on this Proc!", volgid);

  Core::Elements::Element* vele = voldiscret.g_element(volgid);
  if (!vele) FOUR_C_THROW("ERROR: Cannot find element with gid %", volgid);
  faceele->set_parent_master_element(vele, faceele->face_parent_number());

  if (voldiscret2)
  {
    const Epetra_Map* elecolmap2 = voldiscret2->element_col_map();
    if (elecolmap2->LID(volgid) == -1)  // Volume discretization has not Element
      faceele->set_parent_slave_element(nullptr, -1);
    else
    {
      vele = voldiscret2->g_element(volgid);
      if (!vele) FOUR_C_THROW("ERROR: Cannot find element with gid %", volgid);
      faceele->set_parent_slave_element(vele, faceele->face_parent_number());
    }
  }
}

void PoroElast::PrintLogo()
{
  std::cout << "This is a Porous Media problem" << std::endl;
  std::cout << "       .--..--..--..--..--..--. " << std::endl;
  std::cout << "      .'  \\  (`._   (_)     _   \\ " << std::endl;
  std::cout << "     .'    |  '._)         (_)  | " << std::endl;
  std::cout << "     \\ _.')\\      .----..---.   / " << std::endl;
  std::cout << "     |(_.'  |    /    .-\\-.  \\  | " << std::endl;
  std::cout << "     \\     0|    |   ( O| O) | o| " << std::endl;
  std::cout << "      |  _  |  .--.____.'._.-.  | " << std::endl;
  std::cout << "      \\ (_) | o         -` .-`  | " << std::endl;
  std::cout << "       |    \\   |`-._ _ _ _ _\\ / " << std::endl;
  std::cout << "       \\    |   |  `. |_||_|   | " << std::endl;
  std::cout << "       | o  |    \\_      \\     |     -.   .-. " << std::endl;
  std::cout << "       |.-.  \\     `--..-'   O |     `.`-' .' " << std::endl;
  std::cout << "     _.'  .' |     `-.-'      /-.__   ' .-' " << std::endl;
  std::cout << "   .' `-.` '.|='=.='=.='=.='=|._/_ `-'.' " << std::endl;
  std::cout << "   `-._  `.  |________/\\_____|    `-.' " << std::endl;
  std::cout << "      .'   ).| '=' '='\\/ '=' | " << std::endl;
  std::cout << "      `._.`  '---------------' " << std::endl;
  std::cout << "            //___\\   //___\\ " << std::endl;
  std::cout << "              ||       || " << std::endl;
  std::cout << "              ||_.-.   ||_.-. " << std::endl;
  std::cout << "             (_.--__) (_.--__) " << std::endl;
}

double PoroElast::UTILS::calculate_vector_norm(
    const enum Inpar::PoroElast::VectorNorm norm, const Teuchos::RCP<const Epetra_Vector> vect)
{
  // L1 norm
  // norm = sum_0^i vect[i]
  if (norm == Inpar::PoroElast::norm_l1)
  {
    double vectnorm;
    vect->Norm1(&vectnorm);
    return vectnorm;
  }
  // L2/Euclidian norm
  // norm = sqrt{sum_0^i vect[i]^2 }
  else if (norm == Inpar::PoroElast::norm_l2)
  {
    double vectnorm;
    vect->Norm2(&vectnorm);
    return vectnorm;
  }
  // RMS norm
  // norm = sqrt{sum_0^i vect[i]^2 }/ sqrt{length_vect}
  else if (norm == Inpar::PoroElast::norm_rms)
  {
    double vectnorm;
    vect->Norm2(&vectnorm);
    return vectnorm / sqrt((double)vect->GlobalLength());
  }
  // infinity/maximum norm
  // norm = max( vect[i] )
  else if (norm == Inpar::PoroElast::norm_inf)
  {
    double vectnorm;
    vect->NormInf(&vectnorm);
    return vectnorm;
  }
  // norm = sum_0^i vect[i]/length_vect
  else if (norm == Inpar::PoroElast::norm_l1_scaled)
  {
    double vectnorm;
    vect->Norm1(&vectnorm);
    return vectnorm / ((double)vect->GlobalLength());
  }
  else
  {
    FOUR_C_THROW("Cannot handle vector norm");
    return 0;
  }
}

void PoroElast::UTILS::PoroMaterialStrategy::assign_material2_to1(
    const Coupling::VolMortar::VolMortarCoupl* volmortar, Core::Elements::Element* ele1,
    const std::vector<int>& ids_2, Teuchos::RCP<Core::FE::Discretization> dis1,
    Teuchos::RCP<Core::FE::Discretization> dis2)
{
  // call default assignment
  Coupling::VolMortar::UTILS::DefaultMaterialStrategy::assign_material2_to1(
      volmortar, ele1, ids_2, dis1, dis2);

  // default strategy: take material of element with closest center in reference coordinates
  Core::Elements::Element* ele2 = nullptr;
  double mindistance = 1e10;
  {
    std::vector<double> centercoords1 = Core::FE::element_center_refe_coords(*ele1);

    for (int id_2 : ids_2)
    {
      Core::Elements::Element* actele2 = dis2->g_element(id_2);
      std::vector<double> centercoords2 = Core::FE::element_center_refe_coords(*actele2);

      Core::LinAlg::Matrix<3, 1> diffcoords(true);

      for (int j = 0; j < 3; ++j) diffcoords(j, 0) = centercoords1[j] - centercoords2[j];

      if (diffcoords.norm2() - mindistance < 1e-16)
      {
        mindistance = diffcoords.norm2();
        ele2 = actele2;
      }
    }
  }

  // if Bele is a fluid element
  auto* fluid = dynamic_cast<Discret::ELEMENTS::FluidPoro*>(ele2);
  if (fluid != nullptr)
  {
    // Copy Initial Porosity from StructPoro Material to FluidPoro Material
    static_cast<Mat::PAR::FluidPoro*>(fluid->material()->parameter())
        ->set_initial_porosity(
            Teuchos::rcp_static_cast<Mat::StructPoro>(ele1->material())->init_porosity());
  }
  else
  {
    FOUR_C_THROW("ERROR: Unsupported element type '%s'", typeid(*ele2).name());
  }
}

void PoroElast::UTILS::PoroMaterialStrategy::assign_material1_to2(
    const Coupling::VolMortar::VolMortarCoupl* volmortar, Core::Elements::Element* ele2,
    const std::vector<int>& ids_1, Teuchos::RCP<Core::FE::Discretization> dis1,
    Teuchos::RCP<Core::FE::Discretization> dis2)
{
  // call default assignment
  Coupling::VolMortar::UTILS::DefaultMaterialStrategy::assign_material1_to2(
      volmortar, ele2, ids_1, dis1, dis2);

  // if no corresponding element found -> leave
  if (ids_1.empty()) return;

  // default strategy: take material of element with closest center in reference coordinates
  Core::Elements::Element* ele1 = nullptr;
  double mindistance = 1e10;
  {
    std::vector<double> centercoords2 = Core::FE::element_center_refe_coords(*ele2);

    for (int id_1 : ids_1)
    {
      Core::Elements::Element* actele1 = dis1->g_element(id_1);
      std::vector<double> centercoords1 = Core::FE::element_center_refe_coords(*actele1);

      Core::LinAlg::Matrix<3, 1> diffcoords(true);

      for (int j = 0; j < 3; ++j) diffcoords(j, 0) = centercoords1[j] - centercoords2[j];

      if (diffcoords.norm2() - mindistance < 1e-16)
      {
        mindistance = diffcoords.norm2();
        ele1 = actele1;
      }
    }
  }

  // if Aele is a so3_base element
  auto* so_base = dynamic_cast<Discret::ELEMENTS::SoBase*>(ele1);

  // if Bele is a fluid element
  auto* fluid = dynamic_cast<Discret::ELEMENTS::FluidPoro*>(ele2);
  if (fluid != nullptr)
  {
    if (so_base)
    {
      fluid->set_kinematic_type(so_base->kinematic_type());
    }
    else
      FOUR_C_THROW("ERROR: ele1 is not a solid element");
  }
  else
  {
    FOUR_C_THROW("ERROR: Unsupported element type '%s'", typeid(*ele2).name());
  }
}

FOUR_C_NAMESPACE_CLOSE
