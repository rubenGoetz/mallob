
#pragma once

#include "comm/mympi.hpp"
#include "app/palrup/palrup_constants.hpp"
#include "util/logger.hpp"
#include "util/params.hpp"
#include "util/assert.hpp"
#include "util/sys/fileutils.hpp"
#include "util/static_store.hpp"
#include "util/sys/subprocess.hpp"
#include <unistd.h>
#include <csignal>


class PalRupCaller {

private:
    const Parameters& _params;
    const int _global_num_workers;
    const std::string _cnf_path;
    const std::string _proofdir;
    const int _jobId;

    pid_t _pal_launcher_pid {-1};

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

        std::string palRupCall = "../lib/palrup/build/pal_launcher.sh" // Subprocess will prepend "build/"
            " -num-solvers=" + std::to_string(nbSolvers)
            + " -num-nodes=" + std::to_string(nbHosts)
            + " -num-procs-per-node=" + std::to_string(nbProcsPerHost)
            + " -proof-palrup=" + proofInputDir
            + " -proof-working=" + proofWorkingDir
            + " -log-dir=" + logDir
            + " -timeout=" + std::to_string(jwl > 0 ? jwl : LARGE_INT)
            + " -use-local-discs=" + std::to_string(palRupUseLocalDisks)
            + " -execution-dir=" + "lib/palrup"
            + " -formula-path=" + FileUtils::getAbsoluteFilePath(_cnf_path)
            + " -redist-strat=" + std::to_string(palRupStrat)
            + " -read-buffer-size=" + std::to_string(palRupReadBufferSize)
            + " -write-buffer-size=" + std::to_string(palRupWriteBufferSize)
            + " -merge-buffer-size=" + std::to_string(palRupMergeBufferSize)
            + " -q-size=" + std::to_string(palRupQSize)
            + " -q-alpha=" + std::to_string(palRupQAlpha)
            + " -palrup-binary=" + std::to_string(palRupBinary)
            + " -use-drup=" + std::to_string(palRupDrup)
            + " -convert=" + std::to_string(palRupConvert)
            + " -full-check=" + std::to_string(palRupCheck)
            + " -cleanup=" + std::to_string(palrupClean)
            + " -best-effort=" + std::to_string(palRupBestEffort);
        Subprocess subPalRup(_params, palRupCall, false);

        LOG(V4_VVER, "Calling PalRUP checker: %s\n", palRupCall.c_str());
        _pal_launcher_pid = subPalRup.start();
        LOG(V4_VVER, "PalRUP checker started, pid %i\n", _pal_launcher_pid);

        int retval = 1;
        LOG(V4_VVER, "wait for PalRUP checker to exit.\n");
        Process::didChildExit(_pal_launcher_pid, &retval, true);
        _pal_launcher_pid = -1;
        LOG(V4_VVER, "PalRUP checker returned, retval=%i\n", retval);

        if (retval != 0 && !FileUtils::isDirectory(proofWorkingDir + "/.DONE")) {
            FileUtils::create(fileFailure);
            return PALRUP_ERROR;
        }

        // Wait until checking is done
        while (!FileUtils::isDirectory(proofWorkingDir + "/.DONE"))
            usleep(10000);

        if (FileUtils::isRegularFile(fileSuccess)) {
            LOG(V2_INFO, "PalRUP VALIDATED UNSAT\n");
            return PALRUP_VALIDATED;
        }
        LOG(V4_VVER, "PalRUP return PALRUP_DONE = %i\n", PALRUP_DONE);
        return PALRUP_DONE;
#else
        return PALRUP_ERROR;
#endif
    }

    void interrupt() {
        if (_pal_launcher_pid > 0)
            Process::sendSignal(_pal_launcher_pid, SIGABRT);
    }

};
