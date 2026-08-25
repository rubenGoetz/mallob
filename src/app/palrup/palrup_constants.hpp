
#pragma once

#define SUCCESS_FILE_BASE_NAME "success"
#define FAILURE_FILE_BASE_NAME "failure"

#define LRUP_FILE_ENDING ".palrup"
#define DRUP_FILE_ENDING ".padrup"

enum PalRupResult {PALRUP_VALIDATED = 20,
                   PALRUP_ERROR = 10,
                   PALRUP_DONE = 0};
