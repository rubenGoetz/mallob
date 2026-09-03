
#pragma once

#include "app/app_registry.hpp"
#include "app/palrup/palrup_sequence.hpp"
#include "app/palrupcheck/palrup_caller.hpp"
#include "app/palrup/palrup_constants.hpp"


struct ClientSidePalrupProgram : public app_registry::ClientSideProgram {

private:
    PalRupSequence _seq;
    int count = 0;
    const Parameters& _params;
    const JobDescription& _desc;

public:
    ClientSidePalrupProgram(const Parameters params, APIConnector& api, const JobDescription& desc) :
        app_registry::ClientSideProgram(), _seq(params.palRupSequence()),
        _params(params), _desc(desc) {

        function = [&]() {
            LOG(V4_VVER, "Execute PalRUP sequence %s\n", _seq.get_remaining_sequence().c_str());

            // Create job blueprint
            auto proofCnfOpt = StaticStore<std::string>::extractMaybe("chkcnf-#" + std::to_string(_desc.getId()));
            auto proofDirOpt = StaticStore<std::string>::extractMaybe("chkdir-#" + std::to_string(desc.getId()));
            nlohmann::json jsonJobBlueprint = {
                {"user", "internal"},
                {"name", ""},
                {"files", {
                    proofCnfOpt.has_value() ? proofCnfOpt.value() : _params.monoFilename(),
                    proofDirOpt.has_value() ? proofDirOpt.value() : _params.proofDirectory()}
                },
                {"priority", 1.000},
                {"application", "PALRUPCHECK"},
                {"incremental", false}
            };
            nlohmann::json jsonJobResult;
        
            int result = PALRUP_ERROR;
            for (char symbol = _seq.next(); symbol != PalRupSequence::Symbols::DONE; symbol = _seq.next()) {
                // Build next PalRUP job
                auto jsonJob = jsonJobBlueprint;
                auto param_preset = PalRupSequence::get_param_preset(symbol);
                auto logDir = FileUtils::getAbsoluteFilePath(_params.logDirectory()) + "/palrup_logs." + _seq.get_remaining_sequence();
                auto workingDir = FileUtils::getAbsoluteFilePath(_params.palRupCheckWorkdir()) + "/" + _seq.get_remaining_sequence();
                bool palRupDrup = param_preset.find("-palrup-drup=1") != std::string::npos;
                jsonJob["name"] = "palrupchain-" + std::to_string(_desc.getId()) + "-" + std::to_string(count++);
                // TODO: add timeout extention for drup
                jsonJob["configuration"]["options"] = param_preset +
                                                      " -log=" + logDir + 
                                                      " -palrup-check-dir=" + workingDir;

                // execute PalRUP
                LOG(V4_VVER, "Execute PalRUP preset %c: %s\n", symbol, jsonJob.dump().c_str());
                jsonJobResult = APIRegistry::get().processBlocking(jsonJob);
                result = jsonJobResult["result"]["resultcode"];
                LOG(V4_VVER, "PalRUP preset %c result: %i\n", symbol, result);

                if (result == PALRUP_ERROR) {
                    LOG(V0_CRIT, "PalRUP job chain failed at %c!\n", symbol);
                    break;
                }

                _seq.pop();
            }
            
            JobResult res;
            res.id = _desc.getId();
            res.revision = 0;
            res.result = result;
            return res;
        };
    };

    ~ClientSidePalrupProgram() = default;
};

