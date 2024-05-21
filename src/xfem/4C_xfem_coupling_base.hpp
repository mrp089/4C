/*----------------------------------------------------------------------*/
/*! \file

\brief is the base for the different types of mesh and level-set based coupling conditions and
thereby builds the bridge between the xfluid class and the cut-library

\level 2

*/
/*----------------------------------------------------------------------*/


#ifndef FOUR_C_XFEM_COUPLING_BASE_HPP
#define FOUR_C_XFEM_COUPLING_BASE_HPP


#include "4C_config.hpp"

#include "4C_global_data.hpp"
#include "4C_inpar_xfem.hpp"
#include "4C_lib_discret.hpp"
#include "4C_utils_exceptions.hpp"

#include <Epetra_Vector.h>
#include <Teuchos_RCP.hpp>

#include <vector>

FOUR_C_NAMESPACE_OPEN

namespace DRT
{
  namespace ELEMENTS
  {
    // finally this parameter list should go and all interface relevant parameters should be stored
    // in the condition mangager or coupling objects
    class FluidEleParameterXFEM;
  }  // namespace ELEMENTS
}  // namespace DRT

namespace XFEM
{
  typedef std::pair<INPAR::XFEM::EleCouplingCondType, CORE::Conditions::Condition*> EleCoupCond;

  INPAR::XFEM::EleCouplingCondType CondType_stringToEnum(const std::string& condname);

  class CouplingBase
  {
   public:
    //! which boolean set operator used to combine current field with previous one
    enum LevelSetBooleanType
    {
      ls_none = 0,           // used for first Boundary condition level-setcoupling
      ls_cut = 1,            // latex: \cap:         Omega 1 \cap \Omega 2
      ls_union = 2,          // latex: \cup          Omega 1 \cup \Omega 2
      ls_difference = 3,     // latex: \backslash    Omega 1 - Omega 2
      ls_sym_difference = 4  // latex: \triangle     (Omega 1 - Omega 2) \cup (Omega 2 - \Omega 1)
    };

    //! constructor
    explicit CouplingBase(Teuchos::RCP<DRT::Discretization>& bg_dis,  ///< background discretization
        const std::string& cond_name,  ///< name of the condition, by which the derived cutter
                                       ///< discretization is identified
        Teuchos::RCP<DRT::Discretization>&
            cond_dis,  ///< full discretization from which the cutter discretization is derived
        const int coupling_id,  ///< id of composite of coupling conditions
        const double time,      ///< time
        const int step          ///< time step
    );

    //! destructor
    virtual ~CouplingBase() = default;
    virtual void SetDofSetCouplingMap(const std::map<std::string, int>& dofset_coupling_map)
    {
      dofset_coupling_map_ = dofset_coupling_map;
    }

    virtual void SetCouplingDofsets(){};

    int GetCouplingDofsetNds(const std::string& name)
    {
      if (not(dofset_coupling_map_.count(name) == 1))
        FOUR_C_THROW("%s -dofset not set in dofset_coupling_map for fluid dis!", name.c_str());

      return dofset_coupling_map_[name];
    }


    //! initialized the coupling object
    virtual void Init();

    //! setup the coupling object
    virtual void Setup();

    /// get the indicator state
    inline const bool& IsInit() const { return isinit_; };

    /// get the indicator state
    inline const bool& IsSetup() const { return issetup_; };

    /// Check if Init() and Setup() have been called, yet.
    inline void CheckInitSetup() const
    {
      if (!IsInit() or !IsSetup()) FOUR_C_THROW("Call Init() and Setup() first!");
    }

    /// Check if Init() has been called
    inline void CheckInit() const
    {
      if (not IsInit()) FOUR_C_THROW("Call Init() first!");
    }

    //! cutter dis should be loaded into the cut?
    virtual bool CutGeometry() { return true; }

    void SetTimeAndStep(const double time, const int step)
    {
      time_ = time;
      step_ = step;
    }

    void IncrementTimeAndStep(const double dt)
    {
      dt_ = dt;
      time_ += dt;
      step_ += 1;
    }

    void GetConditionByCouplingId(const std::vector<CORE::Conditions::Condition*>& mycond,
        const int coupling_id, std::vector<CORE::Conditions::Condition*>& mynewcond);

    void Status(const int coupling_idx, const int side_start_gid);


    std::string DisNameToString(Teuchos::RCP<DRT::Discretization> dis)
    {
      if (dis == Teuchos::null) return "---";

      return dis->Name();
    }

    std::string TypeToStringForPrint(const INPAR::XFEM::EleCouplingCondType& type)
    {
      if (type == INPAR::XFEM::CouplingCond_SURF_FSI_PART)
        return "XFSI Partitioned";
      else if (type == INPAR::XFEM::CouplingCond_SURF_FSI_MONO)
        return "XFSI Monolithic";
      else if (type == INPAR::XFEM::CouplingCond_SURF_FPI_MONO)
        return "XFPI Monolithic";
      else if (type == INPAR::XFEM::CouplingCond_SURF_FLUIDFLUID)
        return "FLUID-FLUID Coupling";
      else if (type == INPAR::XFEM::CouplingCond_LEVELSET_WEAK_DIRICHLET)
        return "WEAK DIRICHLET BC / LS";
      else if (type == INPAR::XFEM::CouplingCond_LEVELSET_NEUMANN)
        return "NEUMANN BC        / LS";
      else if (type == INPAR::XFEM::CouplingCond_LEVELSET_NAVIER_SLIP)
        return "NAVIER SLIP BC    / LS";
      else if (type == INPAR::XFEM::CouplingCond_LEVELSET_TWOPHASE)
        return "TWO-PHASE Coupling";
      else if (type == INPAR::XFEM::CouplingCond_LEVELSET_COMBUSTION)
        return "COMBUSTION Coupling";
      else if (type == INPAR::XFEM::CouplingCond_SURF_WEAK_DIRICHLET)
        return "WEAK DIRICHLET BC / MESH";
      else if (type == INPAR::XFEM::CouplingCond_SURF_NEUMANN)
        return "NEUMANN BC        / MESH";
      else if (type == INPAR::XFEM::CouplingCond_SURF_NAVIER_SLIP)
        return "NAVIER SLIP BC    / MESH";
      else if (type == INPAR::XFEM::CouplingCond_SURF_NAVIER_SLIP_TWOPHASE)
        return "NAVIER SLIP TWOPHASE BC    / MESH";
      else
        FOUR_C_THROW("unsupported coupling condition type %i", type);

      return "UNKNOWN";
    }

    std::string AveragingToStringForPrint(const INPAR::XFEM::AveragingStrategy& strategy)
    {
      if (strategy == INPAR::XFEM::Xfluid_Sided)
        return "XFLUID-sided averaging";
      else if (strategy == INPAR::XFEM::Embedded_Sided)
        return "EMBEDDED-sided averaging";
      else if (strategy == INPAR::XFEM::Mean)
        return "MEAN averaging";
      else if (strategy == INPAR::XFEM::Harmonic)
        return "HARMONIC averaging";
      else if (strategy == INPAR::XFEM::invalid)
        return "INVALID";
      else
        FOUR_C_THROW("unsupported averaging strategy %i", strategy);

      return "UNKNOWN";
    }


    const EleCoupCond& GetCouplingCondition(
        const int gid  ///< global element element id w.r.t cutter discretization (bgele->Id for
                       ///< LevelsetCoupling cut and side-Id for MeshCoupling)
    )
    {
      int lid = cutter_dis_->ElementColMap()->LID(gid);
      return cutterele_conds_[lid];
    }

    //! get the coupling element (equal to the side for xfluid-sided, mesh-based coupling)
    virtual DRT::Element* GetCouplingElement(
        const int eid  ///< global side element id w.r.t coupling discretization (background element
                       ///< eid for levelset couplings)
    )
    {
      return (coupl_dis_ != Teuchos::null) ? coupl_dis_->gElement(eid) : nullptr;
    }

    virtual const std::string& GetName() { return coupl_name_; }

    Teuchos::RCP<DRT::Discretization> GetCutterDis() { return cutter_dis_; }
    Teuchos::RCP<DRT::Discretization> GetCouplingDis() { return coupl_dis_; }
    Teuchos::RCP<DRT::Discretization> GetCondDis() { return cond_dis_; }

    INPAR::XFEM::AveragingStrategy GetAveragingStrategy() { return averaging_strategy_; }

    virtual void PrepareSolve(){};

    virtual bool HasMovingInterface() = 0;

    virtual void EvaluateCouplingConditions(CORE::LINALG::Matrix<3, 1>& ivel,
        CORE::LINALG::Matrix<3, 1>& itraction, const CORE::LINALG::Matrix<3, 1>& x,
        const CORE::Conditions::Condition* cond)
    {
      FOUR_C_THROW("EvaluateCouplingConditions should be implemented by derived class");
    };

    virtual void EvaluateCouplingConditions(CORE::LINALG::Matrix<3, 1>& ivel,
        CORE::LINALG::Matrix<6, 1>& itraction, const CORE::LINALG::Matrix<3, 1>& x,
        const CORE::Conditions::Condition* cond)
    {
      FOUR_C_THROW("EvaluateCouplingConditions should be implemented by derived class");
    };

    virtual void EvaluateCouplingConditionsOldState(CORE::LINALG::Matrix<3, 1>& ivel,
        CORE::LINALG::Matrix<3, 1>& itraction, const CORE::LINALG::Matrix<3, 1>& x,
        const CORE::Conditions::Condition* cond)
    {
      FOUR_C_THROW("EvaluateCouplingConditionsOldState should be implemented by derived class");
    };

    /// set material pointer for coupling slave side
    virtual void GetInterfaceSlaveMaterial(
        DRT::Element* actele, Teuchos::RCP<CORE::MAT::Material>& mat)
    {
      mat = Teuchos::null;
    }

    /// get the sliplength for the specific coupling condition
    virtual void GetSlipCoefficient(double& slipcoeff, const CORE::LINALG::Matrix<3, 1>& x,
        const CORE::Conditions::Condition* cond)
    {
      slipcoeff = 0.0;
    }

    std::map<INPAR::XFEM::CoupTerm, std::pair<bool, double>>& GetConfigurationmap(
        double& kappa_m,                          //< fluid sided weighting
        double& visc_m,                           //< master sided dynamic viscosity
        double& visc_s,                           //< slave sided dynamic viscosity
        double& density_m,                        //< master sided density
        double& visc_stab_tang,                   //< viscous tangential NIT Penalty scaling
        double& full_stab,                        //< full NIT Penalty scaling
        const CORE::LINALG::Matrix<3, 1>& x,      //< Position x
        const CORE::Conditions::Condition* cond,  //< Condition
        DRT::Element* ele,                        //< Element
        DRT::Element* bele,                       //< Boundary Element
        double* funct,  //< local shape function for Gauss Point (from fluid element)
        double* derxy,  //< local derivatives of shape function for Gauss Point (from fluid element)
        CORE::LINALG::Matrix<3, 1>& rst_slave,  //< local coord of gp on slave boundary element
        CORE::LINALG::Matrix<3, 1>& normal,     //< normal at gp
        CORE::LINALG::Matrix<3, 1>& vel_m,      //< master velocity at gp
        double* fulltraction  //< precomputed fsi traction (sigmaF n + gamma relvel)
    )
    {
      UpdateConfigurationMap_GP(kappa_m, visc_m, visc_s, density_m, visc_stab_tang, full_stab, x,
          cond, ele, bele, funct, derxy, rst_slave, normal, vel_m, fulltraction);
#ifdef FOUR_C_ENABLE_ASSERTIONS
      // Do some safety checks, every combination which is not handled correct by the element level
      // should be caught here ... As this check is done for every Gausspoint, just do this in DEBUG
      // Version ... Feel free the add more checks ...

      // In Case we do just use Penalty or Adjoint we should still set the scaling on both, to
      // guarantee we the the correct constraint!
      if (((configuration_map_.at(INPAR::XFEM::F_Adj_Col).first &&
               !configuration_map_.at(INPAR::XFEM::F_Pen_Col).first) ||
              (!configuration_map_.at(INPAR::XFEM::F_Adj_Col).first &&
                  configuration_map_.at(INPAR::XFEM::F_Pen_Col).first)) &&
          fabs(configuration_map_.at(INPAR::XFEM::F_Adj_Col).second -
               configuration_map_.at(INPAR::XFEM::F_Pen_Col).second) > 1e-16)
        FOUR_C_THROW(
            "%s: You should set Scalings for Adjoint and Penalty Column, even if just one is used, "
            "as we support at the moment just equal penalty and adjoint consistent constraints!",
            cond_name_.c_str());
      if (((configuration_map_.at(INPAR::XFEM::X_Adj_Col).first &&
               !configuration_map_.at(INPAR::XFEM::X_Pen_Col).first) ||
              (!configuration_map_.at(INPAR::XFEM::X_Adj_Col).first &&
                  configuration_map_.at(INPAR::XFEM::X_Pen_Col).first)) &&
          fabs(configuration_map_.at(INPAR::XFEM::X_Adj_Col).second -
               configuration_map_.at(INPAR::XFEM::X_Pen_Col).second) > 1e-16)
        FOUR_C_THROW(
            "%s: You should set Scalings for Adjoint and Penalty Column, even if just one is used, "
            "as we support at the moment just equal penalty and adjoint consistent constraints!",
            cond_name_.c_str());
      if (((configuration_map_.at(INPAR::XFEM::F_Adj_n_Col).first &&
               !configuration_map_.at(INPAR::XFEM::F_Pen_n_Col).first) ||
              (!configuration_map_.at(INPAR::XFEM::F_Adj_n_Col).first &&
                  configuration_map_.at(INPAR::XFEM::F_Pen_n_Col).first)) &&
          fabs(configuration_map_.at(INPAR::XFEM::F_Adj_n_Col).second -
               configuration_map_.at(INPAR::XFEM::F_Pen_n_Col).second) > 1e-16)
        FOUR_C_THROW(
            "%s: You should set Scalings for Adjoint and Penalty Column, even if just one is used, "
            "as we support at the moment just equal penalty and adjoint consistent constraints!",
            cond_name_.c_str());
      if (((configuration_map_.at(INPAR::XFEM::X_Adj_n_Col).first &&
               !configuration_map_.at(INPAR::XFEM::X_Pen_n_Col).first) ||
              (!configuration_map_.at(INPAR::XFEM::X_Adj_n_Col).first &&
                  configuration_map_.at(INPAR::XFEM::X_Pen_n_Col).first)) &&
          fabs(configuration_map_.at(INPAR::XFEM::X_Adj_n_Col).second -
               configuration_map_.at(INPAR::XFEM::X_Pen_n_Col).second) > 1e-16)
        FOUR_C_THROW(
            "%s: You should set Scalings for Adjoint and Penalty Column, even if just one is used, "
            "as we support at the moment just equal penalty and adjoint consistent constraints!",
            cond_name_.c_str());
      if (((configuration_map_.at(INPAR::XFEM::F_Adj_t_Col).first &&
               !configuration_map_.at(INPAR::XFEM::F_Pen_t_Col).first) ||
              (!configuration_map_.at(INPAR::XFEM::F_Adj_t_Col).first &&
                  configuration_map_.at(INPAR::XFEM::F_Pen_t_Col).first)) &&
          fabs(configuration_map_.at(INPAR::XFEM::F_Adj_t_Col).second -
               configuration_map_.at(INPAR::XFEM::F_Pen_t_Col).second) > 1e-16)
        FOUR_C_THROW(
            "%s: You should set Scalings for Adjoint and Penalty Column, even if just one is used, "
            "as we support at the moment just equal penalty and adjoint consistent constraints!",
            cond_name_.c_str());
      if (((configuration_map_.at(INPAR::XFEM::X_Adj_t_Col).first &&
               !configuration_map_.at(INPAR::XFEM::X_Pen_t_Col).first) ||
              (!configuration_map_.at(INPAR::XFEM::X_Adj_t_Col).first &&
                  configuration_map_.at(INPAR::XFEM::X_Pen_t_Col).first)) &&
          fabs(configuration_map_.at(INPAR::XFEM::X_Adj_t_Col).second -
               configuration_map_.at(INPAR::XFEM::X_Pen_t_Col).second) > 1e-16)
        FOUR_C_THROW(
            "%s: You should set Scalings for Adjoint and Penalty Column, even if just one is used, "
            "as we support at the moment just equal penalty and adjoint consistent constraints!",
            cond_name_.c_str());

      // At the moment you cannot use different consistent constraints between Adjoint and Penalty
      // terms
      // Check if we need a more general implementation (If constraints between Adjoint and Penalty
      // are not the same!)
      if (fabs(configuration_map_.at(INPAR::XFEM::F_Adj_Col).second -
               configuration_map_.at(INPAR::XFEM::F_Pen_Col).second) > 1e-16 ||
          fabs(configuration_map_.at(INPAR::XFEM::F_Adj_n_Col).second -
               configuration_map_.at(INPAR::XFEM::F_Pen_n_Col).second) > 1e-16 ||
          fabs(configuration_map_.at(INPAR::XFEM::F_Adj_t_Col).second -
               configuration_map_.at(INPAR::XFEM::F_Pen_t_Col).second) > 1e-16 ||
          fabs(configuration_map_.at(INPAR::XFEM::X_Adj_Col).second -
               configuration_map_.at(INPAR::XFEM::X_Pen_Col).second) > 1e-16 ||
          fabs(configuration_map_.at(INPAR::XFEM::X_Adj_n_Col).second -
               configuration_map_.at(INPAR::XFEM::X_Pen_n_Col).second) > 1e-16 ||
          fabs(configuration_map_.at(INPAR::XFEM::X_Adj_t_Col).second -
               configuration_map_.at(INPAR::XFEM::X_Pen_t_Col).second) > 1e-16)
      {
        std::cout << "F_Adj_Col/F_Pen_Col: " << configuration_map_.at(INPAR::XFEM::F_Adj_Col).second
                  << "/" << configuration_map_.at(INPAR::XFEM::F_Pen_Col).second << std::endl;
        std::cout << "F_Adj_n_Col/F_Pen_n_Col: "
                  << configuration_map_.at(INPAR::XFEM::F_Adj_n_Col).second << "/"
                  << configuration_map_.at(INPAR::XFEM::F_Pen_n_Col).second << std::endl;
        std::cout << "F_Adj_t_Col/F_Pen_t_Col: "
                  << configuration_map_.at(INPAR::XFEM::F_Adj_t_Col).second << "/"
                  << configuration_map_.at(INPAR::XFEM::F_Pen_t_Col).second << std::endl;
        std::cout << "X_Adj_Col/X_Pen_Col: " << configuration_map_.at(INPAR::XFEM::X_Adj_Col).second
                  << "/" << configuration_map_.at(INPAR::XFEM::X_Pen_Col).second << std::endl;
        std::cout << "X_Adj_n_Col/X_Pen_n_Col: "
                  << configuration_map_.at(INPAR::XFEM::X_Adj_n_Col).second << "/"
                  << configuration_map_.at(INPAR::XFEM::X_Pen_n_Col).second << std::endl;
        std::cout << "X_Adj_t_Col/X_Pen_t_Col: "
                  << configuration_map_.at(INPAR::XFEM::X_Adj_t_Col).second << "/"
                  << configuration_map_.at(INPAR::XFEM::X_Pen_t_Col).second << std::endl;
        FOUR_C_THROW(
            "%s: Your consistent constraint for Penalty and Adjoint term is not equal, go to "
            "element level and split up velint_diff_ for penalty and adjoint!",
            cond_name_.c_str());
      }

#endif
      return configuration_map_;
    }

    virtual void GmshOutput(const std::string& filename_base, const int step,
        const int gmsh_step_diff, const bool gmsh_debug_out_screen){};

    /// get viscosity of the master fluid
    void GetViscosityMaster(DRT::Element* xfele,  ///< xfluid ele
        double& visc_m);                          ///< viscosity mastersided

    /// get scaling of the master side for penalty (viscosity, E-modulus for solids)
    virtual void GetPenaltyScalingSlave(DRT::Element* coup_ele,  ///< xfluid ele
        double& penscaling_s)                                    ///< penalty scaling slavesided
    {
      FOUR_C_THROW("GetPenaltyScalingSlave not implemented for this coupling object!");
    }

    /// get weighting paramters
    void GetAverageWeights(DRT::Element* xfele,  ///< xfluid ele
        DRT::Element* coup_ele,                  ///< coup_ele ele
        double& kappa_m,                         ///< Weight parameter (parameter +/master side)
        double& kappa_s,                         ///< Weight parameter (parameter -/slave  side)
        bool& non_xfluid_coupling);

    /// get coupling specific weighting paramters (should be overload, whenever required)
    virtual void GetCouplingSpecificAverageWeights(DRT::Element* xfele,  ///< xfluid ele
        DRT::Element* coup_ele,                                          ///< coup_ele ele
        double& kappa_m)  ///< Weight parameter (parameter +/master side)
    {
      FOUR_C_THROW(
          "XFEM::CouplingBase: GetCouplingSpecificAverageWeights not implemented for this coupling "
          "object!");
    }

    /// compute viscous part of Nitsche's penalty term scaling for Nitsche's method
    void Get_ViscPenalty_Stabfac(DRT::Element* xfele,  ///< xfluid ele
        DRT::Element* coup_ele,                        ///< coup_ele ele
        const double& kappa_m,      ///< Weight parameter (parameter +/master side)
        const double& kappa_s,      ///< Weight parameter (parameter -/slave  side)
        const double& inv_h_k,      ///< the inverse characteristic element length h_k
        double& NIT_visc_stab_fac,  ///< viscous part of Nitsche's penalty term
        double&
            NIT_visc_stab_fac_tang,    ///< viscous part of Nitsche's penalty term in tang direction
        const double& NITStabScaling,  ///< prefactor of Nitsche's scaling in normal direction
        const double&
            NITStabScalingTang,  ///< prefactor of Nitsche's scaling in tangential direction
        const bool& IsPseudo2D,  ///< is this a pseudo 2d problem
        const INPAR::XFEM::ViscStabTraceEstimate
            ViscStab_TraceEstimate  ///< trace estimate for visc stab fac
    );

    /// compute viscous part of Nitsche's penalty term scaling for Nitsche's method
    void Get_ViscPenalty_Stabfac(DRT::Element* xfele,  ///< xfluid ele
        DRT::Element* coup_ele,                        ///< coup_ele ele
        const double& kappa_m,  ///< Weight parameter (parameter +/master side)
        const double& kappa_s,  ///< Weight parameter (parameter -/slave  side)
        const double& inv_h_k,  ///< the inverse characteristic element length h_k
        const DRT::ELEMENTS::FluidEleParameterXFEM*
            params,                 ///< parameterlist which specifies interface configuration
        double& NIT_visc_stab_fac,  ///< viscous part of Nitsche's penalty term
        double&
            NIT_visc_stab_fac_tang  ///< viscous part of Nitsche's penalty term in tang direction
    );


   protected:
    virtual void SetCouplingName()
    {
      coupl_name_ =
          cond_name_;  // the standard case are equal name of condition and coupling object
    }

    virtual void SetConditionsToCopy(){};

    virtual void SetCutterDiscretization(){};

    virtual void SetConditionSpecificParameters(){};

    virtual void SetElementConditions();

    void SetAveragingStrategy();

    void SetCouplingDiscretization();

    virtual void PrepareCutterOutput(){};

    virtual void DoConditionSpecificSetup(){};

    //! set the configuration map up for the specific coupling object
    virtual void SetupConfigurationMap(){};

    //! Updates configurationmap for specific Gausspoint
    virtual void UpdateConfigurationMap_GP(double& kappa_m,  //< fluid sided weighting
        double& visc_m,                                      //< master sided dynamic viscosity
        double& visc_s,                                      //< slave sided dynamic viscosity
        double& density_m,                                   //< master sided density
        double& visc_stab_tang,                   //< viscous tangential NIT Penalty scaling
        double& full_stab,                        //< full NIT Penalty scaling
        const CORE::LINALG::Matrix<3, 1>& x,      //< Position x in global coordinates
        const CORE::Conditions::Condition* cond,  //< Condition
        DRT::Element* ele,                        //< Element
        DRT::Element* bele,                       //< Boundary Element
        double* funct,  //< local shape function for Gauss Point (from fluid element)
        double* derxy,  //< local derivatives of shape function for Gauss Point (from fluid element)
        CORE::LINALG::Matrix<3, 1>& rst_slave,  //< local coord of gp on slave boundary element
        CORE::LINALG::Matrix<3, 1>& normal,     //< normal at gp
        CORE::LINALG::Matrix<3, 1>& vel_m,      //< master velocity at gp
        double* fulltraction  //< precomputed fsi traction (sigmaF n + gamma relvel)
    ){};

    virtual void InitStateVectors(){};

    virtual void PerpareCutterOutput(){};

    void EvaluateDirichletFunction(CORE::LINALG::Matrix<3, 1>& ivel,
        const CORE::LINALG::Matrix<3, 1>& x, const CORE::Conditions::Condition* cond, double time);

    void EvaluateNeumannFunction(CORE::LINALG::Matrix<3, 1>& itraction,
        const CORE::LINALG::Matrix<3, 1>& x, const CORE::Conditions::Condition* cond, double time);

    void EvaluateNeumannFunction(CORE::LINALG::Matrix<6, 1>& itraction,
        const CORE::LINALG::Matrix<3, 1>& x, const CORE::Conditions::Condition* cond, double time);

    void EvaluateFunction(std::vector<double>& final_values, const double* x,
        const CORE::Conditions::Condition* cond, const double time);

    void EvaluateScalarFunction(double& final_value, const double* x, const double& val,
        const CORE::Conditions::Condition* cond, const double time);

    //! @name Sets up a projection matrix
    /*!
    \brief Is utilized for seperating Dirichlet and Neumann conditions
     */
    template <class M1, class M2>
    inline void SetupProjectionMatrix(M1& proj_matrix, const M2& normal)
    {
      double n_j = 0.0;
      for (unsigned int j = 0; j < nsd_; ++j)
      {
        n_j = normal(j, 0);
        for (unsigned int i = 0; i < nsd_; ++i)
        {
          proj_matrix(i, j) = ((i == j) ? 1.0 : 0.0) - normal(i, 0) * n_j;
        }
      }
      return;
    }

    ///< number of spatial dimensions
    size_t nsd_;

    ///< background discretization
    Teuchos::RCP<DRT::Discretization> bg_dis_;

    ///------------------------
    // CUTTER-DISCRETIZATION specific member
    ///------------------------

    ///< name of the condition, by which the derived cutter discretization is identified
    std::string cond_name_;

    ///< discretization from which the cutter discretization is derived
    Teuchos::RCP<DRT::Discretization> cond_dis_;

    ///< id of composite of coupling conditions
    const int coupling_id_;

    ///< discretization w.r.t which the interface is described and w.r.t which the state vectors
    ///< describing the interface position are defined (bgdis for LevelSetCoupling and boundary dis
    ///< for MeshCoupling)
    Teuchos::RCP<DRT::Discretization> cutter_dis_;

    ///< pairs of condition type and pointer to CORE::Conditions::Condition for all column elements
    ///< of the cutter discretization (bgdis for LevelSetCoupling and boundary dis for MeshCoupling)
    std::vector<EleCoupCond> cutterele_conds_;

    std::vector<std::string>
        conditions_to_copy_;  ///< list of conditions that will be copied to the new discretization
                              ///< and used to set for each cutter element

    //! Output specific
    Teuchos::RCP<IO::DiscretizationWriter> cutter_output_;


    ///------------------------
    // Coupling-DISCRETIZATION specific member
    ///------------------------

    ///< discretization with which the background discretization is coupled (structural dis, fluid
    ///< dis, poro dis, scatra dis, boundary dis), Teuchos::null in case that no coupling terms but
    ///< only boundary terms are evaluated
    Teuchos::RCP<DRT::Discretization> coupl_dis_;

    // TODO: be aware of the fact, that accesing the coupling object via Name is unsafe, it assumes
    // that only one coupling of that type is available < name of the mesh/levelset coupling object
    std::string coupl_name_;

    ///< averaging strategy, type of weighting
    INPAR::XFEM::AveragingStrategy averaging_strategy_;

    int myrank_;

    double dt_;  ///< current time step size

    double time_;

    int step_;

    ///< map which configures element level (which terms are evaluated & scaled with which value)
    std::map<INPAR::XFEM::CoupTerm, std::pair<bool, double>> configuration_map_;

    bool issetup_;

    bool isinit_;

    std::map<std::string, int> dofset_coupling_map_;

   private:
    //! Initializes configurationmap to zero (non-virtual)
    void InitConfigurationMap();
  };

}  // namespace XFEM

FOUR_C_NAMESPACE_CLOSE

#endif
