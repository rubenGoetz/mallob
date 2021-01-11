
#ifndef DOMPASCH_CONSOLE_H
#define DOMPASCH_CONSOLE_H

#include <string>
#include <fstream>
#include <stdarg.h>
#include <iostream>
#include <ostream>
#include <ctime>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <cstdlib>

#include "util/sys/fileutils.hpp"
#include "util/sys/timer.hpp"
#include "util/sys/threading.hpp"

#define V0_CRIT 0
#define V1_WARN 1
#define V2_INFO 2
#define V3_VERB 3
#define V4_VVER 4
#define V5_DEBG 5

#define LOG_ADD_DESTRANK 8
#define LOG_ADD_SRCRANK 16

#define LOG_NO_PREFIX 32

class Logger {

private:
    // Taken from https://stackoverflow.com/a/17469726
    enum Code {
        FG_DEFAULT = 39, 
        FG_BLACK = 30, 
        FG_RED = 31, 
        FG_GREEN = 32, 
        FG_YELLOW = 33,
        FG_BLUE = 34, 
        FG_MAGENTA = 35, 
        FG_CYAN = 36, 
        FG_LIGHT_GRAY = 37, 
        FG_DARK_GRAY = 90, 
        FG_LIGHT_RED = 91, 
        FG_LIGHT_GREEN = 92, 
        FG_LIGHT_YELLOW = 93, 
        FG_LIGHT_BLUE = 94, 
        FG_LIGHT_MAGENTA = 95, 
        FG_LIGHT_CYAN = 96, 
        FG_WHITE = 97, 
        BG_RED = 41, 
        BG_GREEN = 42, 
        BG_BLUE = 44, 
        BG_DEFAULT = 49
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };

// Singleton for main console instance
private:
    static Logger _main_instance;
    Logger() {}
    Logger(const Logger& other) = delete;
    Logger& operator=(const Logger& other) = delete;
public:
    static void init(int rank, int verbosity, bool coloredOutput, bool quiet, bool cPrefix, std::string logDir=".") {
        _main_instance._rank = rank;
        _main_instance._verbosity = std::min(7, verbosity);
        _main_instance._colored_output = coloredOutput;
        _main_instance._quiet = quiet;
        _main_instance._c_prefix = cPrefix;

        // Create logging directory as necessary
        _main_instance._log_directory = (logDir.size() == 0 ? "." : logDir) + "/" + std::to_string(rank) + "/";
        int status = FileUtils::mkdir(_main_instance._log_directory);
        if (status != 0) {
            _main_instance.log(V0_CRIT, "ERROR while trying to create / access log directory \"%s\"", _main_instance._log_directory.c_str());
        }
    
        // Open logging files
        _main_instance._log_filename = _main_instance._log_directory + "log" + std::string(".") + std::to_string(rank);
        _main_instance._log_cfile = fopen(_main_instance._log_filename.c_str(), "a");
        if (_main_instance._log_cfile == nullptr) {
            _main_instance.log(V0_CRIT, "ERROR while trying to open log file \"%s\"", _main_instance._log_filename.c_str());
        }
    }
    static Logger& getMainInstance() {
        return _main_instance;
    }
    Logger(Logger&& other) :
        _log_directory(std::move(other._log_directory)), _log_filename(std::move(other._log_filename)), 
        _line_prefix(std::move(other._line_prefix)), _log_cfile(other._log_cfile), _rank(other._rank), 
        _verbosity(other._verbosity), _colored_output(other._colored_output), _quiet(other._quiet), 
        _c_prefix(other._c_prefix) {
        
        other._log_cfile = nullptr;
    }
    Logger& operator=(Logger&& other) {
        _log_directory = std::move(other._log_directory);
        _log_filename = std::move(other._log_filename);
        _line_prefix = std::move(other._line_prefix);
        _log_cfile = other._log_cfile;
        _rank = other._rank;
        _verbosity = other._verbosity;
        _colored_output = other._colored_output;
        _quiet = other._quiet;
        _c_prefix = other._c_prefix;

        other._log_cfile = nullptr;
        return *this;
    }
    ~Logger() {
        flush();
        if (_log_cfile != nullptr) fclose(_log_cfile);
    }

// Usual class members
private:
    std::string _log_directory;
    std::string _log_filename;
    std::string _line_prefix;
    FILE* _log_cfile = nullptr;
    int _rank;
    int _verbosity = 2;
    bool _colored_output = false;
    bool _quiet = false;
    bool _c_prefix = false;

public:
    std::string getLogFilename() {
        return _log_filename;
    }

    void mergeJobLogs(int jobId) {
        int status = FileUtils::mergeFiles(_log_filename + "#" + std::to_string(jobId) + ".*", _log_directory + "jobs." + std::to_string(_rank), true);
        if (status != 0) {
            log(V1_WARN, "WARN: Could not merge logs, exit code: %i\n", status);
        }
    }

    Logger copy(const std::string& linePrefix, const std::string& filenameSuffix, int verbosityOffset = 0) const {
        Logger c;
        c._log_directory = _log_directory;
        c._log_filename = _log_filename + filenameSuffix;
        c._log_cfile = fopen(c._log_filename.c_str(), "a");
        if (c._log_cfile == nullptr) {
            log(V0_CRIT, "ERROR while trying to open child log file \"%s\"", c._log_filename.c_str());
        }
        c._line_prefix = _line_prefix + " " + linePrefix;
        c._verbosity = _verbosity + verbosityOffset;
        c._rank = _rank;
        c._colored_output = _colored_output;
        c._quiet = _quiet;
        c._c_prefix = _c_prefix;
        return c;
    }

    void log(unsigned int options, const char* str, ...) const {
        va_list args;
        va_start(args, str);
        log(args, options, str);
        va_end(args);
    }

    bool fail(unsigned int options, const char* str, ...) const {
        va_list args;
        va_start(args, str);
        log(args, options, str);
        va_end(args);
        return false;
    }

    void flush() const {
        if (!_quiet) fflush(stdout);
        if (_log_cfile != nullptr) fflush(_log_cfile);
    }

    static std::string floatToStr(double num, int precision) {
        std::ostringstream oss;
        oss << std::fixed;
        oss << std::setprecision(precision);
        oss << num;
        return oss.str();
    }

    friend void log(int options, const char* str, ...);
    friend bool log_return_false(const char* str, ...);

private:

    void log(va_list& args, unsigned int options, const char* str) const {

        int verbosity = options & 7;
        if (verbosity > _verbosity) return;
        bool prefix = (options & LOG_NO_PREFIX) == 0;
        bool withDestRank = (options & LOG_ADD_DESTRANK) != 0;
        bool withSrcRank = (options & LOG_ADD_SRCRANK) != 0;
        
        int otherRank = -1;
        if (withDestRank || withSrcRank) {
            otherRank = va_arg(args, int);
        }

        // Colored output, if applicable
        if (!_quiet && _colored_output) {
            if (verbosity == V0_CRIT) {
                std::cout << Modifier(Code::FG_LIGHT_RED);
            } else if (verbosity == V1_WARN) {
                std::cout << Modifier(Code::FG_YELLOW);
            } else if (verbosity == V2_INFO) {
                std::cout << Modifier(Code::FG_WHITE);
            } else {
                std::cout << Modifier(Code::FG_LIGHT_GRAY);
            }
        }

        // Timestamp and node rank
        if (prefix) {
            // Relative time to program start
            float elapsedRel = Timer::elapsedSeconds();
            if (_c_prefix) {
                if (!_quiet) printf("c ");
                if (_log_cfile != nullptr) fprintf(_log_cfile, "c ");
            }
            if (!_quiet) printf("%.3f %i%s ", elapsedRel, _rank, _line_prefix.c_str());
            if (_log_cfile != nullptr) fprintf(_log_cfile, "%.3f %i%s ", elapsedRel, _rank, _line_prefix.c_str());
        }

        // logging message
        va_list argsCopy; va_copy(argsCopy, args); // retrieve copy of "args"
        if (!_quiet) vprintf(str, args); // consume original args
        if (_log_cfile != nullptr) vfprintf(_log_cfile, str, argsCopy); // consume copied args
        if (otherRank >= 0) {
            auto arrowStr = withDestRank ? "=>" : "<=";
            if (!_quiet) printf(" %s [%i]\n", arrowStr, otherRank);
            if (_log_cfile != nullptr) fprintf(_log_cfile, " %s [%i]\n", arrowStr, otherRank);
        }
        va_end(argsCopy); // destroy copy

        // Reset terminal colors
        if (!_quiet && _colored_output) {
            std::cout << Modifier(Code::FG_DEFAULT);
        }
    }
};

void log(int options, const char* str, ...);
bool log_return_false(const char* str, ...);

#endif