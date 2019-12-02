#pragma once

#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <string>

namespace commun {

    using std::string;
    using namespace eosio;

    static inline asset asset_from_string(const string& s) {
        char* fract_ptr = NULL;
        char* symbol_ptr = NULL;
        const char* int_ptr = s.c_str();
    
        int precision = 0;
        int64_t fract_part = 0;
        int64_t int_part = std::strtoll(int_ptr, &fract_ptr, 10);
        check(fract_ptr != int_ptr, "wrong asset string format");
        check(('0' <= *int_ptr && *int_ptr <= '9') || *int_ptr == '-', "wrong asset string format");
        const char* lead_digit = *int_ptr == '-' ? int_ptr+1 : int_ptr;
        check(*lead_digit != '0' || (int_part == 0 && (fract_ptr-lead_digit)==1), "wrong asset string format");
    
        if (*fract_ptr == '.') {
            ++fract_ptr;
            fract_part = std::strtoll(fract_ptr, &symbol_ptr, 10);
            check(symbol_ptr != fract_ptr, "wrong asset string format");
            check('0' <= *fract_ptr && *fract_ptr <= '9', "wrong asset string format");
            precision = symbol_ptr - fract_ptr;
            if (*int_ptr == '-') fract_part = -fract_part;
        } else {
            symbol_ptr = fract_ptr;
        }
    
        check(*symbol_ptr == ' ', "wrong asset string format");
        ++symbol_ptr;
    
        int64_t p10 = 1;
        for (int i = 0; i < precision; i++, p10 *= 10);
    
        int128_t amount = (int128_t)int_part * p10 + fract_part;
        check(-asset::max_amount <= amount && amount <= asset::max_amount, "asset amount overflow");
        check(amount != 0 || *int_ptr != '-', "wrong asset string format");

        return asset(amount, symbol(symbol_ptr, precision));
    }
} /// namespace commun
