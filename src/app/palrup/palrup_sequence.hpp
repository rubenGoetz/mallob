
#pragma once

#include <string>
#include <algorithm>

#include "util/logger.hpp"
#include <cassert>


class PalRupSequence {
    std::string _symbol_stack = "";

public:
    enum Symbols {
        LRUP_CHECK = 'l',
        DRUP_CHECK = 'd',
        DRUP_CONVERT = 'c',
        DRUP_CONVERT_CHECK = 'D',
        //BEST_EFFORT = 'b',
        //BEST_EFFORT_CONVERT = 'B',
        //LRUP_TRIM = 't'

        DONE = '\0'
    };

    PalRupSequence(const std::string s) {
        for (int i = s.length() - 1; i >= 0; i--)
            switch (s[i]) {
                case LRUP_CHECK:
                case DRUP_CHECK:
                case DRUP_CONVERT:
                case DRUP_CONVERT_CHECK:
                //case BEST_EFFORT:
                //case BEST_EFFORT_CONVERT:
                //case LRUP_TRIM:
                    _symbol_stack.push_back(s[i]);
                    break;
                default:
                    LOG(V1_WARN, "PalRUP job sequence contains forbidden symbol %c\n", s[i]);
                    break;
            }
    };
    ~PalRupSequence() = default;

    char next() {
        if (_symbol_stack.empty())
            return Symbols::DONE;
        return _symbol_stack.back();
    };
    void pop() {
        if (!_symbol_stack.empty())
            _symbol_stack.pop_back();
    };

    std::string get_remaining_sequence() {
        std::string rev = _symbol_stack;
        std::reverse(rev.begin(), rev.end());
        return rev;
    };

    static bool is_valid_symbol(char symbol) {
        switch (symbol) {
            case Symbols::LRUP_CHECK:
            case Symbols::DRUP_CHECK:
            case Symbols::DRUP_CONVERT:
            case Symbols::DRUP_CONVERT_CHECK:
            //case Symbols::BEST_EFFORT:
            //case Symbols::BEST_EFFORT_CONVERT:
            //case Symbols::LRUP_TRIM:
                return true;
            default:
                return false;
        }
    };

    static std::string get_param_preset(char symbol) {
        assert(is_valid_symbol(symbol));
        switch (symbol) {
            case Symbols::LRUP_CHECK:
                return "-palrup-check=1 -palrup-drup=0 -palrup-convert=0";
            case Symbols::DRUP_CHECK:
                return "-palrup-check=1 -palrup-drup=1 -palrup-convert=0";
            case Symbols::DRUP_CONVERT:
                return "-palrup-check=0 -palrup-drup=1 -palrup-convert=1";
            case Symbols::DRUP_CONVERT_CHECK:
                return "-palrup-check=1 -palrup-drup=1 -palrup-convert=1";
            //case Symbols::BEST_EFFORT:
            //    return "-palrup-check=1 -palrup-best-effort=1 -palrup-convert=0";
            //case Symbols::BEST_EFFORT_CONVERT:
            //    return "-palrup-check=1 -palrup-best-effort=1 -palrup-convert=1";
            //case Symbols::LRUP_TRIM:
                // TODO
            default:
                return "";
        }
    }

};
