
#pragma once

#include "comm/sysstate.hpp"

#define SYSSTATE_BUSYRATIO 0
#define SYSSTATE_NUMJOBS 1
#define SYSSTATE_GLOBALMEM 2
#define SYSSTATE_NUMHOPS 3
#define SYSSTATE_SPAWNEDREQUESTS 4
#define SYSSTATE_NUMDESIRES 5
#define SYSSTATE_NUMFULFILLEDDESIRES 6
#define SYSSTATE_SUMDESIRELATENCIES 7

typedef SysState<8> WorkerSysState;