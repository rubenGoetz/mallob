
#pragma once

#include "optionslist.hpp"
#include "util/option.hpp"

// Application-specific program options for SAT solving.
// memberName                               short option name, long option name          default   min  max

OPTION_GROUP(grpAppPalrupCheck, "app/palrupcheck", "PalRUP checking options")
 OPT_STRING(palRupCheckWorkdir,         "palrup-check-dir", "",                      "",                        "Global (shared) working directory for PalRUP checkers")
 OPT_INT(palRupStrat,                   "palrup-strat", "",                          3, 1, 3,                   "Check PalRUP proof using redistribution strategy 1 to 3")
 OPT_INT(palRupReadBufferSize,          "palrup-read-buffer", "",                    16384, 0, LARGE_INT,       "PalRUP checker read buffer size in KiB")
 OPT_INT(palRupWriteBufferSize,         "palrup-write-buffer", "",                   16384, 0, LARGE_INT,       "PalRUP checker write buffer size in KiB")
 OPT_INT(palRupMergeBufferSize,         "palrup-merge-buffer", "",                   8192, 0, LARGE_INT,        "PalRUP checker merge buffer size in KiB")
 OPT_INT(palRupQSize,                   "palrup-q-size", "",                         409600, 0, LARGE_INT,      "PalRUP checker queue size in KiB")
 OPT_INT(palrupClean,                   "palrup-clean", "",                          1, 0, 2,                   "Clean up after PalRUP check. 0 performs no cleanup, 1 removes working directory, 2 additionaly removes proof")
 OPT_BOOL(palRupUseLocalDisks,          "palrup-use-local-disks", "",                false,                     "Expect PalRUP fragments to be stored on distributed disks")
 OPT_BOOL(palRupCheck,                  "palrup-check", "",                          false,                     "Check PalRUP proof immediately after its production")
 OPT_BOOL(palRupDrup,                   "palrup-drup", "",                           false,                     "Check proof in DRUP format")
 OPT_BOOL(palRupConvert,                "palrup-convert", "",                        false,                     "Convert proof from DRUP tu LRUP format")
 //OPT_BOOL(palRupBestEffort,             "palrup-best-effort", "",                    false,                     "Check LRUP if it exists, check DRUP else")     // TODO: implement
 OPT_FLOAT(palRupQAlpha,                "palrup-q-alpha", "",                        0.5, 0, 1,                 "PalRUP checker queue alpha")
