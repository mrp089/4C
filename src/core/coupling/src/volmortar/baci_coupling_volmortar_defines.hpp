/*----------------------------------------------------------------------*/
/*! \file

\level 1


*----------------------------------------------------------------------*/

#ifndef BACI_COUPLING_VOLMORTAR_DEFINES_HPP
#define BACI_COUPLING_VOLMORTAR_DEFINES_HPP

#include "baci_config.hpp"

BACI_NAMESPACE_OPEN

/************************************************************************/
/* Mortar algorithm parameters                                          */
/************************************************************************/

// MORTAR INTEGRATION
#define VOLMORTARINTTOL 1.0e-12 /* tolerance for assembling gp-values*/

// GEOMETRIC TOLERANCES
#define VOLMORTARELETOL 1.0e-12
#define VOLMORTARCUTTOL 1.0e-12
#define VOLMORTARCUT2TOL -1.0e-12

BACI_NAMESPACE_CLOSE

#endif  // COUPLING_VOLMORTAR_DEFINES_H