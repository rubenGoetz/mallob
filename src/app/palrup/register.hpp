
#pragma once

#include "app/app_message_subscription.hpp"
#include "app/app_registry.hpp"
#include "app/palrup/clientside_palrup_program.hpp"

void register_mallob_app_palrup() {

    app_registry::AppEntry entry;
    entry.key = "PALRUP";
    entry.copyrightInformation = "by Dominik Schreiber and Ruben Götz\n";
    entry.type = app_registry::AppEntry::CLIENT_SIDE;

    entry.reader = [](const Parameters& params, const std::vector<std::string>& files, JobDescription& desc) {
        auto chkcnf = files[0];
        auto chkdir = files.size() > 1 ? files[1] : params.proofDirectory();
        desc.setAppConfigurationEntry("__chkcnf", chkcnf);
        desc.setAppConfigurationEntry("__chkproofdir", chkdir);
        desc.beginInitialization(0);
        desc.endInitialization();
        StaticStore<std::string>::insert("chkcnf-#" + std::to_string(desc.getId()), chkcnf);
        StaticStore<std::string>::insert("chkdir-#" + std::to_string(desc.getId()), chkdir);
        return true;
    };

    entry.clientSideProgramCreator = [](const Parameters& params, APIConnector& api, JobDescription& desc) {
        return new ClientSidePalrupProgram(params, api, desc);
    };

    entry.solutionFormatter = [](const Parameters& params, const JobResult& result, const JobProcessingStatistics& stat) {
        auto json = nlohmann::json::array();
        auto model = result.copySolution();
        json = std::move(model);

        return json;
    };

    entry.optionChecker = [](const Parameters& params, auto& vec) {
        if (!params.palRupSequence.isSet())
            vec.push_back({
                &params.palRupSequence,
                "PalRUP requires to specify a job Sequence."
            });
        else
            for ( char c : params.palRupSequence() )
                if (!PalRupSequence::is_valid_symbol(c))
                    vec.push_back({
                        &params.palRupSequence,
                        "Symbol " + std::string(1, c) + " is not valid in a PalRUP sequence"
                    });

        return vec.empty();
    };

    app_registry::registerApplication(entry);
}
