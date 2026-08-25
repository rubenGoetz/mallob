
#pragma once

#include "comm/mympi.hpp"
#include "app/palrup/palrup_constants.hpp"
#include "util/logger.hpp"
#include "util/params.hpp"
#include "util/assert.hpp"
#include "util/sys/fileutils.hpp"
#include "util/static_store.hpp"
#include <unistd.h>


class PalRupCaller {

private:
    const Parameters& _params;
    const int _global_num_workers;
    const std::string _cnf_path;
    const std::string _proofdir;
    const int _jobId;

public:
    PalRupCaller(const Parameters& params, int globalNumWorkers, const std::string& cnfPath, const std::string& proofDir, const int jobId) :
        _params(params), _global_num_workers(globalNumWorkers), _cnf_path(cnfPath), _proofdir(proofDir), _jobId(jobId) {}

    PalRupResult callBlocking() {

#if MALLOB_APP_PALRUPCHECK
        assert(_params.regularProcessDistribution());
        assert(_params.logDirectory.isSet());
        assert(_params.proofDirectory.isSet());
        assert(_params.palRupCheckWorkdir.isSet());

        const int nbProcsPerHost = _params.processesPerHost();
        const int nbHosts = _global_num_workers / nbProcsPerHost;
        const int nbSolvers = _params.numThreadsPerProcess() * _global_num_workers;
        const int jwl = _params.jobWallclockLimit();
        const int palRupStrat = _params.palRupStrat();
        const int palRupReadBufferSize = _params.palRupReadBufferSize();
        const int palRupWriteBufferSize = _params.palRupWriteBufferSize();
        const int palRupMergeBufferSize = _params.palRupMergeBufferSize();
        const int palRupQSize = _params.palRupQSize();
        const int palrupClean = _params.palrupClean();
        const bool palRupBinary = _params.palRupBinary();
        const bool palRupUseLocalDisks = _params.palRupUseLocalDisks();
        const bool palRupDrup = _params.palRupDrup();
        const bool palRupConvert = _params.palRupConvert();
        const bool palRupCheck = _params.palRupCheck();
        const bool palRupBestEffort = _params.palRupBestEffort();
        const float palRupQAlpha = _params.palRupQAlpha();
        const std::string proofInputDir = FileUtils::getAbsoluteFilePath(_proofdir);
        const std::string proofWorkingDir = FileUtils::getAbsoluteFilePath(_params.palRupCheckWorkdir());
        const std::string logDir = FileUtils::getAbsoluteFilePath(_params.logDirectory());
        FileUtils::mkdir(proofWorkingDir);

        auto fileSuccess = logDir + "/" + SUCCESS_FILE_NAME;
        auto fileFailure = logDir + "/" + FAILURE_FILE_NAME;
        if (FileUtils::isRegularFile(fileSuccess) && palRupCheck) {
            LOG(V0_CRIT, "[ERROR] PalRUP success file exists before starting a checker!\n");
            return PALRUP_ERROR;
        }
        if (FileUtils::isRegularFile(fileFailure) && palRupCheck) {
            LOG(V0_CRIT, "[ERROR] PalRUP failure file discovered immediately\n");
            return PALRUP_ERROR;
        }

        std::string palRupCall = "cd lib/palrup;"
            " NUM_SOLVERS=" + std::to_string(nbSolvers)
            + " NUM_NODES=" + std::to_string(nbHosts)
            + " NUM_PROCS_PER_NODE=" + std::to_string(nbProcsPerHost)
            // FIXME replace monoFilename with path to *this specific job's* description
            + " FORMULA_PATH=\"" + FileUtils::getAbsoluteFilePath(_cnf_path) + "\""
            + " PROOF_PALRUP=\"" + proofInputDir + "\""
            + " PROOF_WORKING=\"" + proofWorkingDir + "\""
            + " LOG_DIR=\"" + logDir + "\""
            + " TIMEOUT=" + std::to_string(jwl > 0 ? jwl : 9999999)
            + " REDIST_STRAT=\"" + std::to_string(palRupStrat) + "\""
            + " READ_BUFFER_SIZE=\"" + std::to_string(palRupReadBufferSize) + "\""
            + " WRITE_BUFFER_SIZE=\"" + std::to_string(palRupWriteBufferSize) + "\""
            + " MERGE_BUFFER_SIZE=\"" + std::to_string(palRupMergeBufferSize) + "\""
            + " Q_SIZE=\"" + std::to_string(palRupQSize) + "\""
            + " Q_ALPHA=\"" + std::to_string(palRupQAlpha) + "\""
            + " PALRUP_BINARY=\"" + std::to_string(palRupBinary) + "\""
            + " USE_LOCAL_DISKS=\"" + std::to_string(palRupUseLocalDisks) + "\""
            + " USE_DRUP=\"" + std::to_string(palRupDrup) + "\""
            + " CONVERT=\"" + std::to_string(palRupConvert) + "\""
            + " FULL_CHECK=\"" + std::to_string(palRupCheck) +"\""
            + " CLEANUP=\"" + std::to_string(palrupClean) + "\""
            + " BEST_EFFORT=\"" + std::to_string(palRupBestEffort) + "\""
            + " bash build/pal_launcher.sh";

        LOG(V4_VVER, "Calling PalRUP checker: %s\n", palRupCall.c_str());
        const int retval = system(palRupCall.c_str());
        LOG(V4_VVER, "PalRUP checker returned, retval=%i\n", retval);

        if (retval != 0) {
            FileUtils::create(fileFailure);
            return PALRUP_ERROR;
        }
        if (FileUtils::isRegularFile(fileSuccess)) {
            LOG(V2_INFO, "PalRUP VALIDATED UNSAT\n");
            return PALRUP_VALIDATED;
        }
        return PALRUP_DONE;
#else
        return PALRUP_ERROR;
#endif
    }
};
