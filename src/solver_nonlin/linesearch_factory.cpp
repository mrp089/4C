/*----------------------------------------------------------------------------*/
/*!
\file linesearch_factory.cpp

<pre>
Maintainer: Matthias Mayr
            mayr@mhpc.mw.tum.de
            089 - 289-10362
</pre>
*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* headers */

// Teuchos
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>

// baci
#include "linesearch_backtracking.H"
#include "linesearch_base.H"
#include "linesearch_bruteforce.H"
#include "linesearch_factory.H"
#include "linesearch_fullstep.H"
#include "linesearch_linear.H"
#include "linesearch_polynomial.H"

#include "../drt_lib/drt_dserror.H"

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
NLNSOL::LineSearchFactory::LineSearchFactory()
{
  return;
}

/*----------------------------------------------------------------------------*/
Teuchos::RCP<NLNSOL::LineSearchBase>
NLNSOL::LineSearchFactory::Create(const Teuchos::ParameterList& params)
{
  const std::string lstype = params.get<std::string>("Line Search Type");
  if (lstype == "brute force")
  {
    return Teuchos::rcp(new NLNSOL::LineSearchBruteForce());
  }
  else if (lstype == "backtracking")
  {
    return Teuchos::rcp(new NLNSOL::LineSearchBacktracking());
  }
  else if (lstype == "polynomial2")
  {
    return Teuchos::rcp(new NLNSOL::LineSearchPolynomial());
  }
  else if (lstype == "full step")
  {
    return Teuchos::rcp(new NLNSOL::LineSearchFullStep());
  }
  else if (lstype == "linear")
  {
    return Teuchos::rcp(new NLNSOL::LineSearchLinear());
  }
  else
  {
    dserror("Unknown line search algorithm %s.", lstype.c_str());
  }

  return Teuchos::null;
}
