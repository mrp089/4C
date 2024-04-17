/*----------------------------------------------------------------------*/
/*! \file

\brief Auxiliar routine to boolify integral Yes/No data


\level 0
*/

/*----------------------------------------------------------------------*/
/* definitions */

/*----------------------------------------------------------------------*/
/* headers */
#include "baci_inpar_boolifyparameters.hpp"

#include "baci_utils_exceptions.hpp"

FOUR_C_NAMESPACE_OPEN


/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
// Auxiliar routine to boolify integral Yes/No data
void INPUT::BoolifyValidInputParameters(
    Teuchos::ParameterList& list  ///< the valid input parameter list
)
{
  // collect parameter names with Yes/No entries
  // parse sub-lists as well
  std::vector<std::string> boolnames;  // collector
  for (Teuchos::ParameterList::ConstIterator i = list.begin(); i != list.end(); ++i)
  {
    if (list.isSublist(list.name(i)))
      BoolifyValidInputParameters(list.sublist(list.name(i)));
    else
    {
      if (list.isType<std::string>(list.name(i)))
      {
        const std::string value = list.get<std::string>(list.name(i));
        if ((value == "Yes") or (value == "YES") or (value == "yes") or (value == "No") or
            (value == "NO") or (value == "no"))
        {
          boolnames.push_back(list.name(i));
        }
      }
    }
  }

  // remove integral Yes/No parameters and replace them by Boolean
  for (std::vector<std::string>::iterator name = boolnames.begin(); name != boolnames.end(); ++name)
  {
    const std::string value = list.get<std::string>(*name);
    list.remove(*name);
    if ((value == "Yes") or (value == "YES") or (value == "yes"))
      list.set<bool>(*name, true);
    else if ((value == "No") or (value == "NO") or (value == "no"))
      list.set<bool>(*name, false);
    else
      dserror("Cannot deal with entry \"%s\"", value.c_str());
  }
}


/*----------------------------------------------------------------------*/

FOUR_C_NAMESPACE_CLOSE
