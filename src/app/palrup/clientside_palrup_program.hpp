
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

            nlohmann::json jsonJobBlueprint = {
                {"user", "internal"},
                {"name", ""},
                {"files", {_params.monoFilename(), _params.proofDirectory()}},
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
                bool palRupDrup = param_preset.find("-palrup-drup=1") != std::string::npos;
                jsonJob["name"] = "palrupchain-" + std::to_string(_desc.getId()) + "-" + std::to_string(count++);
                jsonJob["configuration"]["options"] = param_preset;

                // execute PalRUP
                LOG(V4_VVER, "Execute PalRUP preset %c: %s\n", symbol, jsonJob.dump().c_str());
                jsonJobResult = APIRegistry::get().processBlocking(jsonJob);
                result = jsonJobResult["result"]["resultcode"];
                LOG(V4_VVER, "PalRUP preset %c result: %i\n", symbol, result);

                if (result == PALRUP_ERROR) {
                    LOG(V0_CRIT, "PalRUP job chain failed at %c!\n", symbol);
                    break;
                }

                // rename result file for next iteration
                auto logDir = FileUtils::getAbsoluteFilePath(_params.logDirectory());
                auto fileSuccess = logDir + "/" + SUCCESS_FILE_BASE_NAME + (palRupDrup ? DRUP_FILE_ENDING : LRUP_FILE_ENDING);
                auto fileFailure = logDir + "/" + FAILURE_FILE_BASE_NAME + (palRupDrup ? DRUP_FILE_ENDING : LRUP_FILE_ENDING);
                if (FileUtils::exists(fileSuccess)){
                    std::string new_file_name = fileSuccess + "." + _seq.get_remaining_sequence();
                    LOG(V4_VVER, "Rename PalRUP result file %s -> %s", fileSuccess.c_str(), new_file_name.c_str());
                    std::rename(fileSuccess.c_str(), new_file_name.c_str());
                }
                if (FileUtils::exists(fileFailure)) {
                    std::string new_file_name = fileFailure + "." + _seq.get_remaining_sequence();
                    LOG(V4_VVER, "Rename PalRUP result file %s -> %s", fileFailure.c_str(), new_file_name.c_str());
                    std::rename(fileFailure.c_str(), new_file_name.c_str());
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

