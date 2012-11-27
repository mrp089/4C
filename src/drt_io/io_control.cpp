/*----------------------------------------------------------------------*/
/*!
 * \file io_control.cpp
\brief output control

<pre>
Maintainer: Ulrich Kuettler
            kuettler@lnm.mw.tum.de
            http://www.lnm.mw.tum.de/Members/kuettler
            089 - 289-15238
</pre>
*/
/*----------------------------------------------------------------------*/


#include "io_control.H"
#include "io_pstream.H"
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <Epetra_MpiComm.h>
#include <vector>
#include <iostream>
#include <sstream>

#include "../drt_lib/drt_dserror.H"

extern "C" {
#include "compile_settings.h"      // for printing current revision number
}

#include "../pss_full/pss_cpp.h" // access to legacy parser module

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
IO::OutputControl::OutputControl(const Epetra_Comm& comm,
                                 std::string problemtype,
                                 std::string spatial_approx,
                                 std::string inputfile,
                                 std::string outputname,
                                 int ndim,
                                 int restart,
                                 int filesteps,
                                 int create_controlfile)
  : problemtype_(problemtype),
    inputfile_(inputfile),
    ndim_(ndim),
    filename_(outputname),
    restartname_(outputname),
    filesteps_(filesteps),
    create_controlfile_(create_controlfile)
{
  if (restart)
  {
    if (comm.MyPID()==0)
    {
      int number = 0;
      size_t pos = filename_.rfind('-');
      if (pos!=string::npos)
      {
        number = atoi(filename_.substr(pos+1).c_str());
        filename_ = filename_.substr(0,pos);
      }

      for (;;)
      {
        number += 1;
        std::stringstream name;
        name << filename_ << "-" << number << ".control";
        std::ifstream file(name.str().c_str());
        if (not file)
        {
          filename_ = name.str();
          filename_ = filename_.substr(0,filename_.length()-8);
          std::cout << "restart with new output file: "
                    << filename_
                    << "\n";
          break;
        }
      }
    }

    if (comm.NumProc()>1)
    {
      int length = filename_.length();
      std::vector<int> name(filename_.begin(),filename_.end());
      int err = comm.Broadcast(&length, 1, 0);
      if (err)
        dserror("communication error");
      name.resize(length);
      err = comm.Broadcast(&name[0], length, 0);
      if (err)
        dserror("communication error");
      filename_.assign(name.begin(),name.end());
    }
  }

  if (comm.MyPID()==0)
  {
    std::stringstream name;
    name << filename_ << ".control";
    controlfile_.open(name.str().c_str(),std::ios_base::out);
    if (not controlfile_)
      dserror("could not open control file '%s' for writing", name.str().c_str());

    time_t time_value;
    time_value = time(NULL);

    char hostname[31];
    struct passwd *user_entry;
    user_entry = getpwuid(getuid());
    gethostname(hostname, 30);

    controlfile_ << "# baci output control file\n"
                 << "# created by "
                 << user_entry->pw_name
                 << " on " << hostname << " at " << ctime(&time_value)
                 << "# using code revision " << (CHANGEDREVISION+0) << " \n\n"
                 << "input_file = \"" << inputfile << "\"\n"
                 << "problem_type = \"" << problemtype << "\"\n"
                 << "spatial_approximation = \"" << spatial_approx << "\"\n"
                 << "ndim = " << ndim << "\n"
                 << "\n";

    // insert back reference
    if (restart)
    {
      size_t pos = outputname.rfind('/');
      controlfile_ << "restarted_run = \""
                   << ((pos!=string::npos) ? outputname.substr(pos+1) : outputname)
                   << "\"\n\n";
    }

    controlfile_ << std::flush;
  }
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
IO::OutputControl::OutputControl(const Epetra_Comm& comm,
                                 std::string problemtype,
                                 std::string spatial_approx,
                                 std::string inputfile,
                                 std::string restartname,
                                 std::string outputname,
                                 int ndim,
                                 int restart,
                                 int filesteps,
                                 int create_controlfile,
                                 bool adaptname)
  : problemtype_(problemtype),
    inputfile_(inputfile),
    ndim_(ndim),
    filename_(outputname),
    restartname_(restartname),
    filesteps_(filesteps),
    create_controlfile_(create_controlfile)
{
  if (restart)
  {
    if (comm.MyPID()==0 && adaptname == true)
    {
      // check whether filename_ includes a dash and in case separate the number at the end
      int number = 0;
      size_t pos = filename_.rfind('-');
      if (pos!=string::npos)
      {
        number = atoi(filename_.substr(pos+1).c_str());
        filename_ = filename_.substr(0,pos);
      }

      // either add or increase the number in the end or just set the new name for the control file
      for (;;)
      {
        // if no number is found and the control file name does not yet exist -> create it
        if (number == 0)
        {
          std::stringstream name;
          name << filename_ << ".control";
          std::ifstream file(name.str().c_str());
          if (not file)
          {
            std::cout << "restart with new output file: "
                      << filename_
                      << std::endl;
            break;
          }
        }
        // a number was found or the file does already exist -> set number correctly and add it
        number += 1;
        std::stringstream name;
        name << filename_ << "-" << number << ".control";
        std::ifstream file(name.str().c_str());
        if (not file)
        {
          filename_ = name.str();
          filename_ = filename_.substr(0,filename_.length()-8);
          std::cout << "restart with new output file: "
                    << filename_
                    << std::endl;
          break;
        }
      }

    }

    if (comm.NumProc()>1)
    {
      int length = filename_.length();
      std::vector<int> name(filename_.begin(),filename_.end());
      int err = comm.Broadcast(&length, 1, 0);
      if (err)
        dserror("communication error");
      name.resize(length);
      err = comm.Broadcast(&name[0], length, 0);
      if (err)
        dserror("communication error");
      filename_.assign(name.begin(),name.end());
    }
  }

  if (comm.MyPID()==0)
  {
    std::stringstream name;
    name << filename_ << ".control";
    if(create_controlfile_)
    {
    controlfile_.open(name.str().c_str(),std::ios_base::out);
    if (not controlfile_)
      dserror("could not open control file '%s' for writing", name.str().c_str());
    }

    time_t time_value;
    time_value = time(NULL);

    char hostname[31];
    struct passwd *user_entry;
    user_entry = getpwuid(getuid());
    gethostname(hostname, 30);
    if(create_controlfile_)
    {
      controlfile_ << "# baci output control file\n"
                   << "# created by "
                   << user_entry->pw_name
                   << " on " << hostname << " at " << ctime(&time_value)
                   << "# using code revision " << (CHANGEDREVISION+0) << " \n\n"
                   << "input_file = \"" << inputfile << "\"\n"
                   << "problem_type = \"" << problemtype << "\"\n"
                   << "spatial_approximation = \"" << spatial_approx << "\"\n"
                   << "ndim = " << ndim << "\n"
                   << "\n";

      // insert back reference
      if (restart && create_controlfile_)
      {
        controlfile_ << "restarted_run = \""
                     << restartname_
                     << "\"\n\n";
      }

      controlfile_ << std::flush;
    }
  }
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IO::OutputControl::OverwriteResultFile()
{
  controlfile_.close();
  std::stringstream name;
  name << filename_ << ".control";
  controlfile_.open(name.str().c_str(),std::ios_base::out);
  if (not controlfile_)
    dserror("could not open control file '%s' for writing", name.str().c_str());

  time_t time_value;
  time_value = time(NULL);

  char hostname[31];
  struct passwd *user_entry;
  user_entry = getpwuid(getuid());
  gethostname(hostname, 30);

  controlfile_ << "# baci output control file\n"
               << "# created by "
               << user_entry->pw_name
               << " on " << hostname << " at " << ctime(&time_value)
               << "# using code revision " << (CHANGEDREVISION+0) << " \n\n"
               << "input_file = \"" << inputfile_ << "\"\n"
               << "problem_type = \"" << problemtype_ << "\"\n"
               << "spatial_approximation = \"" << "Polynomial" << "\"\n"
               << "ndim = " << ndim_ << "\n"
               << "\n";

  controlfile_ << std::flush;
}


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IO::OutputControl::NewResultFile(int numb_run)
{
  if (filename_.rfind("_run_")!=string::npos)
  {
    size_t pos = filename_.rfind("_run_");
    if (pos==string::npos)
      dserror("inconsistent file name");
    filename_ = filename_.substr(0, pos);
  }

  std::stringstream name;
  name << filename_ << "_run_"<< numb_run;
  filename_ = name.str();
  name << ".control";
  controlfile_.close();
  controlfile_.open(name.str().c_str(),std::ios_base::out);
  if (not controlfile_)
    dserror("could not open control file '%s' for writing", name.str().c_str());

  time_t time_value;
  time_value = time(NULL);

  char hostname[31];
  struct passwd *user_entry;
  user_entry = getpwuid(getuid());
  gethostname(hostname, 30);

  controlfile_ << "# baci output control file\n"
               << "# created by "
               << user_entry->pw_name
               << " on " << hostname << " at " << ctime(&time_value)
               << "# using code revision " << (CHANGEDREVISION+0) << " \n\n"
               << "input_file = \"" << inputfile_ << "\"\n"
               << "problem_type = \"" << problemtype_ << "\"\n"
               << "spatial_approximation = \"" << "Polynomial" << "\"\n"
               << "ndim = " << ndim_ << "\n"
               << "\n";

  controlfile_ << std::flush;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void IO::OutputControl::NewResultFile(string name_appendix, int numb_run)
{
  std::stringstream name;
  name  << name_appendix;
  name << "_run_"<< numb_run;
  filename_ = name.str();
  name << ".control";

  controlfile_.close();
  bool b = controlfile_.fail();
  IO::cout << b << IO::endl;
  controlfile_.open(name.str().c_str(),std::ios_base::out);
  if (not controlfile_)
    dserror("could not open control file '%s' for writing", name.str().c_str());

  time_t time_value;
  time_value = time(NULL);

  char hostname[31];
  struct passwd *user_entry;
  user_entry = getpwuid(getuid());
  gethostname(hostname, 30);

  controlfile_ << "# baci output control file\n"
               << "# created by "
               << user_entry->pw_name
               << " on " << hostname << " at " << ctime(&time_value)
               << "# using code revision " << (CHANGEDREVISION+0) << " \n\n"
               << "input_file = \"" << inputfile_ << "\"\n"
               << "problem_type = \"" << problemtype_ << "\"\n"
               << "spatial_approximation = \"" << "Polynomial" << "\"\n"
               << "ndim = " << ndim_ << "\n"
               << "\n";

  controlfile_ << std::flush;
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
IO::InputControl::InputControl(std::string filename, const bool serial)
  : filename_(filename)
{
  std::stringstream name;
  name << filename << ".control";

  if (!serial)
    parse_control_file(&table_, name.str().c_str(), MPI_COMM_WORLD);
  else
    parse_control_file_serial(&table_, name.str().c_str());
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
IO::InputControl::InputControl(std::string filename, const Epetra_Comm& comm)
  : filename_(filename)
{
  std::stringstream name;
  name << filename << ".control";

  // works for parallel, as well as serial applications because we only
  // have an Epetra_MpiComm
  const Epetra_MpiComm& epetrampicomm = dynamic_cast<const Epetra_MpiComm&>(comm);
  if (!&epetrampicomm)
    dserror("ERROR: casting Epetra_Comm -> Epetra_MpiComm failed");
  const MPI_Comm lcomm = epetrampicomm.GetMpiComm();

  parse_control_file(&table_, name.str().c_str(), lcomm);
}
/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
IO::InputControl::~InputControl()
{
  destroy_map(&table_);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
IO::ErrorFileControl::ErrorFileControl(const Epetra_Comm& comm,
                                       const std::string outputname,
                                       int create_errorfiles)
  : filename_(outputname),
    errfile_(NULL)
{
  // create error file name
  {
    std::ostringstream mypid;
    mypid << comm.MyPID();
    errname_ = filename_ + mypid.str() + ".err";
  }

   //open error files (one per processor)
  //int create_errorfiles =0;
  if (create_errorfiles)
  {
    errfile_ = fopen(errname_.c_str(), "w");
    if (errfile_ == NULL)
      dserror("Opening of output file %s failed\n", errname_.c_str());
  }

  // inform user
  if (comm.MyPID() == 0)
    IO::cout << "errors are reported to " <<  errname_.c_str() << IO::endl;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
IO::ErrorFileControl::~ErrorFileControl()
{
  if (errfile_ != NULL) fclose(errfile_);
}

