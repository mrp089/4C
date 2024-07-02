/*----------------------------------------------------------------------------*/
/*! \file
\brief Fast time monitor. E. g. to measure the element evaluation times.


\level 3
*/
/*----------------------------------------------------------------------------*/

#include "4C_contact_aug_timemonitor.hpp"

#include "4C_utils_exceptions.hpp"

#include <Epetra_Comm.h>
#include <Teuchos_Time.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <typename EnumClass>
CONTACT::Aug::TimeMonitor<EnumClass>::TimeMonitor()
{
  static_assert(std::is_same<unsigned, typename std::underlying_type<EnumClass>::type>::value,
      "The template ENUM_CLASS must use UNSIGNED INT as underlying type!");

  timings_.resize(static_cast<unsigned>(EnumClass::MAX_TIME_ID), std::make_pair(-1.0, 0.0));
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <typename EnumClass>
void CONTACT::Aug::TimeMonitor<EnumClass>::TimeMonitor::reset()
{
  std::fill(timings_.begin(), timings_.end(), std::make_pair(-1.0, 0.0));
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <typename EnumClass>
void CONTACT::Aug::TimeMonitor<EnumClass>::start(const EnumClass id)
{
  timings_[static_cast<unsigned>(id)].first = Teuchos::Time::wallTime();
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <typename EnumClass>
void CONTACT::Aug::TimeMonitor<EnumClass>::stop(const EnumClass id)
{
  std::pair<double, double>& begin_time = timings_[static_cast<unsigned>(id)];
  if (begin_time.first == -1.0) FOUR_C_THROW("Call start() first!");

  double& accumulated_time = begin_time.second;
  last_incr_ = Teuchos::Time::wallTime() - begin_time.first;
  accumulated_time += last_incr_;

  begin_time.first = -1.0;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <typename EnumClass>
double CONTACT::Aug::TimeMonitor<EnumClass>::getMyTotalTime() const
{
  double my_total_time = 0.0;
  for (auto& t : timings_) my_total_time += t.second;

  return my_total_time;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
template <typename EnumClass>
void CONTACT::Aug::TimeMonitor<EnumClass>::write(std::ostream& os)
{
  int mypid = 0;
  if (comm_) mypid = comm_->MyPID();

  if (mypid == 0)
  {
    os << std::string(100, '=') << std::endl;
    os << "CONTACT::Aug::TimeMonitor - Final Overview:\n";
  }
  for (unsigned i = 0; i < static_cast<unsigned>(EnumClass::MAX_TIME_ID); ++i)
  {
    double gtime = 0.0;
    if (comm_)
      comm_->SumAll(&timings_[i].second, &gtime, 1);
    else
      gtime = timings_[i].second;

    if (gtime == 0.0) continue;

    std::string name = TimeID2Str(static_cast<EnumClass>(i));
    if (mypid == 0)
    {
      os << std::string(100, '-') << "\n";
      os << "TOTAL - " << std::left << std::setw(72) << name << ": " << std::scientific
         << std::setprecision(5) << gtime << " [sec.]\n";
    }

    if (comm_)
    {
      const int numproc = comm_->NumProc();
      std::vector<double> lproc_timings(numproc, 0.0);
      lproc_timings[mypid] = timings_[i].second;

      std::vector<double> gproc_timings(numproc, 0.0);
      comm_->SumAll(lproc_timings.data(), gproc_timings.data(), numproc);

      if (mypid == 0)
      {
        for (int p = 0; p < numproc; ++p)
        {
          os << "proc #" << std::right << std::setw(3) << p << " - " << std::left << std::setw(68)
             << name << ": " << std::scientific << std::setprecision(5) << gproc_timings[p]
             << " [sec.]\n"
             << std::left;
        }
      }
    }
  }
  if (mypid == 0) os << std::string(100, '=') << std::endl;

  // wait till everything is done
  if (comm_) comm_->Barrier();
}

/*----------------------------------------------------------------------------*/
template class CONTACT::Aug::TimeMonitor<CONTACT::Aug::TimeID>;
template class CONTACT::Aug::TimeMonitor<CONTACT::Aug::GlobalTimeID>;

FOUR_C_NAMESPACE_CLOSE
