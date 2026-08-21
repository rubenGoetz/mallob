
#pragma once

#include "app/app_message_subscription.hpp"
#include "app/app_registry.hpp"
#include "app/palrupcheck/palrupcheck_job.hpp"
#include "data/job_processing_statistics.hpp"

void register_mallob_app_palrupcheck() {

    app_registry::AppEntry entry;
    entry.key = "PALRUPCHECK";
    entry.type = app_registry::AppEntry::DISTRIBUTED;
    entry.copyrightInformation = "by Dominik Schreiber and Ruben Götz\n";

    entry.optionChecker = [](const Parameters& params, auto& vec) {
        if (!params.regularProcessDistribution() || params.processesPerHost() == 0) {
            vec.push_back({
                &params.regularProcessDistribution,
                "PalRUP-check requires to specify a valid number of processes per host (-rpa=1 -pph=<k>)."
            });
        }
        if (!params.logDirectory.isSet()) {
            vec.push_back({
                &params.logDirectory,
                "PalRUP-check requires to specify a log directory."
            });
        }
        if (!params.proofDirectory.isSet()) {
            vec.push_back({
                &params.proofDirectory,
                "PalRUP-check requires to specify a proof directory."
            });
        }
        if (!params.palRupCheckWorkdir.isSet()) {
            vec.push_back({
                &params.palRupCheckWorkdir,
                "PalRUP-check requires to specify a shared working directory."
            });
        }
        if (params.palRupConvert() && !params.palRupDrup()) {
            vec.push_back({
                &params.palRupConvertRecheck,
                "PalRUP-check requires to specify the PadRUP format when converting."
            });
        }
        if (params.palRupConvertRecheck() && !params.palRupConvert()) {
            vec.push_back({
                &params.palRupConvertRecheck,
                "PalRUP-check requires to enable PalRUP to PadRUP convertion for a recheck."
            });
        }
        return vec.empty();
    };

    entry.reader = [](const Parameters& params, const std::vector<std::string>& files, JobDescription& desc) {
        auto chkcnf = files[0];
        auto chkdir = files.size() > 1 ? files[1] : params.proofDirectory();
        desc.setAppConfigurationEntry("__chkcnf", chkcnf);
        desc.setAppConfigurationEntry("__chkproofdir", chkdir);
        desc.beginInitialization(0);
        desc.endInitialization();
        StaticStore<std::string>::insert("chkcnf-#" + std::to_string(desc.getId()), chkcnf);
        StaticStore<std::string>::insert("chkdir-#" + std::to_string(desc.getId()), chkdir);
        if (files.size() > 2)
            StaticStore<bool>::insert("done-#" + std::to_string(desc.getId()), true);
        return true;
    };

    entry.creator = [](const Parameters& params, const Job::JobSetup& setup, AppMessageTable& table) -> Job* {
        return new PalrupCheckJob(params, setup, table);
    };

    entry.solutionFormatter = [](const Parameters& params, const JobResult& result, const JobProcessingStatistics& stat) {
        auto json = nlohmann::json::array();
        auto model = result.copySolution();
        json = std::move(model);
        //std::stringstream modelString;
        //modelString << "c parse_time " << stat.parseTime << "\n";
        //modelString << "c process_time " << stat.processingTime << "\n";
        //modelString << "c total_response_time " << stat.totalResponseTime << "\n";
        //json.push_back(modelString.str());
        return json;
    };

    entry.cleaner = [](const Parameters& params) {
        if (!params.palRupCheckWorkdir().empty()) {
            // Remove all files created by previous checker and directories if empty
            FileUtils::rmrf(params.palRupCheckWorkdir() + "/.cleanup");
            FileUtils::rmrf(params.palRupCheckWorkdir() + "/.unsat_found");
            for (auto file : FileUtils::glob(params.palRupCheckWorkdir() + "/.pal_launcher.*.lock"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.palRupCheckWorkdir() + "/*/*/.check_ok"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.palRupCheckWorkdir() + "/*/*/.done"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.palRupCheckWorkdir() + "/*/*/.valid"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.palRupCheckWorkdir() + "/*/*/out.palrup_import"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.palRupCheckWorkdir() + "/*/*/out.palrup_proxy"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.palRupCheckWorkdir() + "/*/*"))
                if (FileUtils::isDirectory(file))
                    FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.palRupCheckWorkdir() + "/*"))
                if (FileUtils::isDirectory(file))    
                    FileUtils::rm(file);

            FileUtils::rm(params.palRupCheckWorkdir());
        }

        if (!params.logDirectory().empty()) {
            FileUtils::rmrf(params.logDirectory() + "/palrup_pals");
            FileUtils::rmrf(params.logDirectory() + "/padrup_pals");
            for (auto file : FileUtils::glob(params.logDirectory() + "/*.palrup"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.logDirectory() + "/*.padrup"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.logDirectory() + "/*/palrup.out"))
                FileUtils::rm(file);
            for (auto file : FileUtils::glob(params.logDirectory() + "/*/padrup.out"))
                FileUtils::rm(file);
        }
    };

    entry.epilog = [](const Parameters& params, const JobResult& result) {
        auto cnfPathOpt = StaticStore<std::string>::extractMaybe("chkcnf-#" + std::to_string(result.id));
        auto chkPathOpt = StaticStore<std::string>::extractMaybe("chkdir-#" + std::to_string(result.id));
        auto isDone = StaticStore<bool>::extractMaybe("done-#" + std::to_string(result.id));
        if (params.palRupConvertRecheck()
            && cnfPathOpt.has_value() && chkPathOpt.has_value()
            && result.result == UNSAT && !isDone.has_value()) {
            auto cnfPath = cnfPathOpt.value();
            auto chkPath = chkPathOpt.value();
            nlohmann::json jsonJob = {
                {"user", "internal"},
                {"name", "palrupchk-re-" + std::to_string(result.id)},
                {"files", {cnfPath, chkPath, cnfPath}},     // third file marks end of palrup-job-chain. TODO: make elegant
                {"priority", 1.000},
                {"application", "PALRUPCHECK"},
                {"incremental", false}
            };
            auto jsonPalrupResult = APIRegistry::get().processBlocking(jsonJob);
        }
    };

    app_registry::registerApplication(entry);
}
