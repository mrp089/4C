
#include <sstream>
#include <string>
#include <iostream>

#include "drt_parobjectregister.H"

#include "../drt_nurbs_discret/drt_control_point.H"
#include "../drt_meshfree_discret/drt_meshfree_node.H"
#include "../drt_beam2/beam2.H"
#include "../drt_beam2r/beam2r.H"
#include "../drt_beam3/beam3.H"
#include "../drt_beam3ii/beam3ii.H"
#include "../drt_beam3eb/beam3eb.H"
#include "../drt_beam3ebtor/beam3ebtor.H"
#include "../drt_beam3eb_anisotrop/beam3eb_anisotrop.H"
#include "../drt_rigidsphere/rigidsphere.H"
//#include "../drt_smoothrod/smoothrod.H"
#include "../drt_truss3/truss3.H"
#include "../drt_truss2/truss2.H"
#include "../drt_torsion3/torsion3.H"
#include "../drt_torsion2/torsion2.H"
#include "../drt_s8/shell8.H"
#include "../drt_scatra_ele/scatra_ele.H"
#include "../drt_meshfree_discret/meshfree_fluid_cell.H"
#include "../drt_meshfree_discret/meshfree_scatra_cell.H"
#include "../drt_meshfree_discret/drt_meshfree_multibin.H"
#include "../drt_particle/particle_node.H"
#include "../drt_fluid_ele/fluid_ele.H"
#include "../drt_fluid_ele/fluid_ele_poro.H"
#include "../drt_fluid_ele/fluid_ele_immersed.H"
#include "../drt_combust/combust3.H"
#include "../drt_ale2/ale2.H"
#include "../drt_ale2/ale2_nurbs.H"
#include "../drt_ale3/ale3.H"
#include "../drt_ale3/ale3_nurbs.H"
#include "../drt_bele3/bele3_4.H"
#include "../drt_bele3/bele3.H"
#include "../drt_bele3/vele3.H"
#include "../drt_bele3/bele2.H"
#include "../drt_constraint/constraint_element2.H"
#include "../drt_constraint/constraint_element3.H"
#include "../drt_w1/wall1.H"
#include "../drt_w1/wall1_nurbs.H"
#include "../drt_w1/wall1_poro_eletypes.H"
#include "../drt_w1/wall1_poro_p1_eletypes.H"
#include "../drt_w1/wall1_poro_p2_eletypes.H"
#include "../drt_w1/wall1_scatra.H"
#include "../drt_so3/so_hex8.H"
#include "../drt_so3/so_hex20.H"
#include "../drt_so3/so_hex27.H"
#include "../drt_so3/so_nurbs27.H"
#include "../drt_so3/so_sh8.H"
#include "../drt_so3/so_sh8p8.H"
#include "../drt_so3/so_tet4.H"
//#include "../drt_so3/so_ptet.H"
#include "../drt_so3/so_nstet.H"
#include "../drt_so3/so_nstet5.H"
#include "../drt_so3/so_tet10.H"
#include "../drt_so3/so_weg6.H"
#include "../drt_so3/so_shw6.H"
#include "../drt_so3/so_disp.H"
#include "../drt_so3/so_hex8p1j1.H"
#include "../drt_so3/so_hex8fbar.H"
#include "../drt_so3/so3_poro_eletypes.H"
#include "../drt_so3/so3_poro_p1_eletypes.H"
#include "../drt_so3/so3_scatra_eletypes.H"
#include "../drt_so3/so3_thermo_eletypes.H"
#include "../drt_so3/so3_ssn_plast_eletypes.H"
#include "../drt_so3/so3_ssn_plast_sosh8.H"
#include "../drt_thermo/thermo_element.H"
#include "../drt_mat/newtonianfluid.H"
#include "../drt_mat/stvenantkirchhoff.H"
#include "../drt_mat/thermostvenantkirchhoff.H"
#include "../drt_mat/thermoplasticlinelast.H"
#include "../drt_mat/micromaterial.H"
#include "../drt_mat/neohooke.H"
#include "../drt_mat/aaaneohooke.H"
#include "../drt_mat/aaaneohooke_stopro.H"
#include "../drt_mat/aaaraghavanvorp_damage.H"
#include "../drt_mat/aaa_mixedeffects.H"
#include "../drt_mat/aaagasser.H"
#include "../drt_mat/visconeohooke.H"
#include "../drt_mat/viscoanisotropic.H"
#include "../drt_mat/scatra_mat.H"
#include "../drt_mat/myocard.H"
#include "../drt_mat/ion.H"
#include "../drt_mat/mixfrac.H"
#include "../drt_mat/sutherland.H"
#include "../drt_mat/arrhenius_spec.H"
#include "../drt_mat/arrhenius_temp.H"
#include "../drt_mat/arrhenius_pv.H"
#include "../drt_mat/ferech_pv.H"
#include "../drt_mat/carreauyasuda.H"
#include "../drt_mat/modpowerlaw.H"
#include "../drt_mat/herschelbulkley.H"
#include "../drt_mat/yoghurt.H"
#include "../drt_mat/matlist.H"
#include "../drt_mat/elchmat.H"
#include "../drt_mat/elasthyper.H"
#include "../drt_mat/plasticelasthyper.H"
#include "../drt_mat/viscogenmax.H"
#include "../drt_mat/cnst_1d_art.H"
#include "../drt_mat/fourieriso.H"
#include "../drt_mat/growth_ip.H"
#include "../drt_mat/growth_scd.H"
#include "../drt_mat/scatra_growth_scd.H"
#include "../drt_mat/constraintmixture.H"
#include "../drt_mat/constraintmixture_history.H"
#include "../drt_mat/plasticlinelast.H"
#include "../drt_mat/robinson.H"
#include "../drt_mat/damage.H"
#include "../drt_mat/biofilm.H"
#include "../drt_mat/spring.H"
#include "../drt_mat/optimization_density.H"
#include "../drt_mat/structporo.H"
#include "../drt_mat/structporo_reaction.H"
#include "../drt_mat/fluidporo.H"
#include "../drt_mat/acoustic.H"
#include "../drt_mat/acoustic_visc.H"
#include "../drt_mortar/mortar_node.H"
#include "../drt_mortar/mortar_element.H"
#include "../drt_contact/contact_node.H"
#include "../drt_contact/friction_node.H"
#include "../drt_contact/contact_element.H"
#include "../drt_art_net/artery.H"
#include "../drt_red_airways/red_airway.H"
#include "../drt_opti/topopt_optimizer_ele.H"
#include "../drt_crack/dcohesive.H"
#include "../drt_acou/acou_ele.H"
#include "../drt_acou/acou_visc_ele.H"
#include "../drt_inv_analysis/smc_particle.H"


std::string DRT::ParObjectList()
{
  std::stringstream s;

  s << DRT::ContainerType::Instance().Name() << " "
    << DRT::ConditionObjectType::Instance().Name() << " "
    << DRT::NodeType::Instance().Name() << " "
    << DRT::NURBS::ControlPointType::Instance().Name() << " "
    << PARTICLE::ParticleNodeType::Instance().Name() << " "
    << DRT::MESHFREE::MeshfreeNodeType::Instance().Name() << " "
    << DRT::MESHFREE::MeshfreeMultiBinType::Instance().Name() << " "
    << DRT::ELEMENTS::Beam2Type::Instance().Name() << " "
    << DRT::ELEMENTS::Beam2rType::Instance().Name() << " "
    << DRT::ELEMENTS::Beam3Type::Instance().Name() << " "
    << DRT::ELEMENTS::Beam3iiType::Instance().Name() << " "
    << DRT::ELEMENTS::Beam3ebType::Instance().Name() << " "
    << DRT::ELEMENTS::Beam3ebtorType::Instance().Name() << " "
    << DRT::ELEMENTS::Beam3ebanisotropType::Instance().Name() << " "
    << DRT::ELEMENTS::RigidsphereType::Instance().Name() << " "
//    << DRT::ELEMENTS::SmoothrodType::Instance().Name() << " "
    << DRT::ELEMENTS::Truss3Type::Instance().Name() << " "
    << DRT::ELEMENTS::Truss2Type::Instance().Name() << " "
    << DRT::ELEMENTS::Torsion3Type::Instance().Name() << " "
    << DRT::ELEMENTS::Torsion2Type::Instance().Name() << " "
    << DRT::ELEMENTS::Shell8Type::Instance().Name() << " "
    << DRT::ELEMENTS::Wall1Type::Instance().Name() << " "
    << DRT::ELEMENTS::WallQuad4PoroType::Instance().Name() << " "
    << DRT::ELEMENTS::WallQuad4PoroP1Type::Instance().Name() << " "
    << DRT::ELEMENTS::WallQuad4PoroP2Type::Instance().Name() << " "
    << DRT::ELEMENTS::WallQuad9PoroType::Instance().Name() << " "
    << DRT::ELEMENTS::WallQuad9PoroP1Type::Instance().Name() << " "
    << DRT::ELEMENTS::WallQuad9PoroP2Type::Instance().Name() << " "
    << DRT::ELEMENTS::WallNurbs4PoroType::Instance().Name() << " "
    << DRT::ELEMENTS::WallNurbs9PoroType::Instance().Name() << " "
    << DRT::ELEMENTS::NURBS::Wall1NurbsType::Instance().Name() << " "
    << DRT::ELEMENTS::Wall1ScatraType::Instance().Name() << " "
    << DRT::ELEMENTS::Combust3Type::Instance().Name() << " "
    << DRT::ELEMENTS::FluidType::Instance().Name() << " "
    << DRT::ELEMENTS::FluidTypeImmersed::Instance().Name() << " "
    << DRT::ELEMENTS::FluidPoroEleType::Instance().Name() << " "
    << DRT::ELEMENTS::FluidBoundaryType::Instance().Name() << " "
    << DRT::ELEMENTS::FluidPoroBoundaryType::Instance().Name() << " "
    << DRT::ELEMENTS::MeshfreeFluidType::Instance().Name() << " "
    << DRT::ELEMENTS::MeshfreeFluidBoundaryType::Instance().Name() << " "
    << DRT::ELEMENTS::Ale3Type::Instance().Name() << " "
    << DRT::ELEMENTS::NURBS::Ale3_NurbsType::Instance().Name() << " "
    << DRT::ELEMENTS::Ale2Type::Instance().Name() << " "
    << DRT::ELEMENTS::NURBS::Ale2_NurbsType::Instance().Name() << " "
    << DRT::ELEMENTS::Bele2Type::Instance().Name() << " "
    << DRT::ELEMENTS::Bele3Type::Instance().Name() << " "
    << DRT::ELEMENTS::Bele3_4Type::Instance().Name() << " "
    << DRT::ELEMENTS::Vele3Type::Instance().Name() << " "
    << DRT::ELEMENTS::NStetType::Instance().Name() << " "
    << DRT::ELEMENTS::NStet5Type::Instance().Name() << " "
    << DRT::ELEMENTS::NURBS::So_nurbs27Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_nurbs27PoroType::Instance().Name() << " "
//    << DRT::ELEMENTS::PtetType::Instance().Name() << " "
    << DRT::ELEMENTS::SoDispType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_Hex8P1J1Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8fbarType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8fbarThermoType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8PoroType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8PoroP1Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8ScatraType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8ThermoType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8PlastType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex8Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex20Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex27Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex27PoroType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex27ThermoType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex20ThermoType::Instance().Name() << " "
    << DRT::ELEMENTS::So_hex27PlastType::Instance().Name() << " "
    << DRT::ELEMENTS::So_sh8Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_sh8PlastType::Instance().Name() << " "
    << DRT::ELEMENTS::So_sh8p8Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_shw6Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_tet10Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_tet10PoroType::Instance().Name() << " "
    << DRT::ELEMENTS::So_tet10ScatraType::Instance().Name() << " "
    << DRT::ELEMENTS::So_tet4Type::Instance().Name() << " "
    << DRT::ELEMENTS::So_tet4PoroType::Instance().Name() << " "
    << DRT::ELEMENTS::So_tet4ScatraType::Instance().Name() << " "
    << DRT::ELEMENTS::So_tet4ThermoType::Instance().Name() << " "
    << DRT::ELEMENTS::So_tet10ThermoType::Instance().Name() << " "
    << DRT::ELEMENTS::So_weg6Type::Instance().Name() << " "
    << DRT::ELEMENTS::ArteryType::Instance().Name() << " "
    << DRT::ELEMENTS::RedAirwayType::Instance().Name() << " "
    << DRT::ELEMENTS::RedAcinusType::Instance().Name() << " "
    << DRT::ELEMENTS::RedInterAcinarDepType::Instance().Name() << " "
    << DRT::ELEMENTS::RedAirBloodScatraType::Instance().Name() << " "
    << DRT::ELEMENTS::RedAirBloodScatraLine3Type::Instance().Name() << " "
    << DRT::ELEMENTS::ConstraintElement2Type::Instance().Name() << " "
    << DRT::ELEMENTS::ConstraintElement3Type::Instance().Name() << " "
    << DRT::ELEMENTS::TransportType::Instance().Name() << " "
    << DRT::ELEMENTS::MeshfreeTransportType::Instance().Name() << " "
    << DRT::ELEMENTS::MeshfreeTransportBoundaryType::Instance().Name() << " "
    << DRT::ELEMENTS::TopOptType::Instance().Name() << " "
    << DRT::ELEMENTS::ThermoType::Instance().Name() << " "
    << DRT::ELEMENTS::DcohesiveType::Instance().Name() << " "
    << DRT::ELEMENTS::AcouType::Instance().Name() << " "
    << DRT::ELEMENTS::AcouViscType::Instance().Name() << " "
    << DRT::ELEMENTS::AcouBoundaryType::Instance().Name() << " "
    << DRT::ELEMENTS::AcouViscBoundaryType::Instance().Name() << " "
    << DRT::ELEMENTS::AcouIntFaceType::Instance().Name() << " "
    << DRT::ELEMENTS::AcouViscIntFaceType::Instance().Name() << " "
    << MAT::Cnst_1d_artType::Instance().Name() << " "
    << MAT::AAAgasserType::Instance().Name() << " "
    << MAT::AAAneohookeType::Instance().Name() << " "
    << MAT::AAAneohooke_stoproType::Instance().Name() << " "
    << MAT::AAAraghavanvorp_damageType::Instance().Name() << " "
    << MAT::AAA_mixedeffectsType::Instance().Name() << " "
    << MAT::ArrheniusPVType::Instance().Name() << " "
    << MAT::ArrheniusSpecType::Instance().Name() << " "
    << MAT::ArrheniusTempType::Instance().Name() << " "
    << MAT::BiofilmType::Instance().Name() << " "
    << MAT::CarreauYasudaType::Instance().Name() << " "
    << MAT::ConstraintMixtureType::Instance().Name() << " "
    << MAT::ConstraintMixtureHistoryType::Instance().Name() << " "
    << MAT::ElastHyperType::Instance().Name() << " "
    << MAT::PlasticElastHyperType::Instance().Name() << " "
    << MAT::ViscoGenMaxType::Instance().Name() << " "
    << MAT::FerEchPVType::Instance().Name() << " "
    << MAT::FluidPoroType::Instance().Name() << " "
    << MAT::FourierIsoType::Instance().Name() << " "
    << MAT::GrowthType::Instance().Name() << " "
    << MAT::GrowthScdType::Instance().Name() << " "
    << MAT::ScatraGrowthScdType::Instance().Name() << " "
    << MAT::HerschelBulkleyType::Instance().Name() << " "
    << MAT::IonType::Instance().Name() << " "
    << MAT::MatListType::Instance().Name() << " "
    << MAT::ElchMatType::Instance().Name() << " "
    << MAT::MicroMaterialType::Instance().Name() << " "
    << MAT::MixFracType::Instance().Name() << " "
    << MAT::ModPowerLawType::Instance().Name() << " "
    << MAT::MyocardType::Instance().Name() << " "
    << MAT::NeoHookeType::Instance().Name() << " "
    << MAT::NewtonianFluidType::Instance().Name() << " "
    << MAT::StructPoroType::Instance().Name() << " "
    << MAT::StructPoroReactionType::Instance().Name() << " "
    << MAT::ScatraMatType::Instance().Name() << " "
    << MAT::StVenantKirchhoffType::Instance().Name() << " "
    << MAT::SutherlandType::Instance().Name() << " "
    << MAT::ThermoStVenantKirchhoffType::Instance().Name() << " "
    << MAT::ThermoPlasticLinElastType::Instance().Name() << " "
    << MAT::ViscoAnisotropicType::Instance().Name() << " "
    << MAT::ViscoNeoHookeType::Instance().Name() << " "
    << MAT::YoghurtType::Instance().Name() << " "
    << MAT::SpringType::Instance().Name() << " "
    << MAT::PlasticLinElastType::Instance().Name() << " "
    << MAT::RobinsonType::Instance().Name() << " "
    << MAT::DamageType::Instance().Name() << " "
    << MAT::TopOptDensType::Instance().Name() << " "
    << MAT::AcousticMatType::Instance().Name() << " "
    << MAT::AcousticViscMatType::Instance().Name() << " "
    << MORTAR::MortarNodeType::Instance().Name() << " "
    << MORTAR::MortarElementType::Instance().Name() << " "
    << CONTACT::CoNodeType::Instance().Name() << " "
    << CONTACT::FriNodeType::Instance().Name() << " "
    << CONTACT::CoElementType::Instance().Name() << " "
    // only compile this on the workstation as kaisers boost version is outdated an cant run this code
#if (BOOST_MAJOR_VERSION == 1) && (BOOST_MINOR_VERSION >= 47)
    << STR::INVANA::SMCParticleType::Instance().Name() << " "
#else
 // no code here
#endif

    ;
  return s.str();
}


void PrintParObjectList()
{
  std::cout << "defined parobject types: " << DRT::ParObjectList() << "\n";
}
