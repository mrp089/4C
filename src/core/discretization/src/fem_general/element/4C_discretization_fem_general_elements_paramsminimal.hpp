/*----------------------------------------------------------------------*/
/*! \file

\brief Minimal implementation of the parameter interface for the element
<--> time integrator data exchange

\level 3


*/
/*----------------------------------------------------------------------*/

#ifndef FOUR_C_DISCRETIZATION_FEM_GENERAL_ELEMENTS_PARAMSMINIMAL_HPP
#define FOUR_C_DISCRETIZATION_FEM_GENERAL_ELEMENTS_PARAMSMINIMAL_HPP

#include "4C_config.hpp"

#include "4C_discretization_fem_general_elements_paramsinterface.hpp"

FOUR_C_NAMESPACE_OPEN

namespace CORE::Elements
{
  /*!
     \brief Minimal implementation of the parameter interface for the element <--> time integrator
     data exchange
    */
  class ParamsMinimal : public ParamsInterface
  {
   public:
    //! constructor
    ParamsMinimal() : ele_action_(none), total_time_(-1.0), delta_time_(-1.0){};

    //! @name Access general control parameters
    //! @{
    //! get the desired action type
    enum ActionType GetActionType() const override { return ele_action_; };

    //! get the current total time for the evaluate call
    double GetTotalTime() const override { return total_time_; };

    //! get the current time step
    double GetDeltaTime() const override { return delta_time_; };
    //! @}

    /*! @name set routines which are used to set the parameters of the data container
     *
     *  These functions are not allowed to be called by the elements! */
    //! @{
    //! set the action type
    inline void SetActionType(const enum CORE::Elements::ActionType& actiontype)
    {
      ele_action_ = actiontype;
    }

    //! set the total time for the evaluation call
    inline void SetTotalTime(const double& total_time) { total_time_ = total_time; }

    //! set the current time step for the evaluation call
    inline void SetDeltaTime(const double& dt) { delta_time_ = dt; }

    //! @}

   private:
    //! @name General element control parameters
    //! @{
    //! Current action type
    enum ActionType ele_action_;

    //! total time for the evaluation
    double total_time_;

    //! current time step for the evaluation
    double delta_time_;
    //! @}
  };  // class ParamsMinimal

}  // namespace CORE::Elements


FOUR_C_NAMESPACE_CLOSE

#endif