
#pragma once

#include "optionslist.hpp"

// Application-specific program options for PalRUP job chains.
// memberName                               short option name, long option name          default   min  max

OPTION_GROUP(grpAppPalrup, "app/palrup", "PalRUP meta app options")
 OPT_STRING(palRupSequence,     "palrup-sequence", "",      "",                     "Defines a sequence of PalRUP-Check jobs to be executed consecutively. Executed after solving, if the application is SAT and the result unsatifsiable.")
 OPT_FLOAT(palRupDrupFactor,    "palrup-drup-factor", "",   1., 0., LARGE_INT,      "Timeout factor for DRUP jobs compared to other jobs")
