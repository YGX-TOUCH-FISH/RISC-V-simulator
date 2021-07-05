//
// Created by 18163 on 2021/6/28.
//

#ifndef RISK_V_SIMULATOR_EXECUTER_HPP
#define RISK_V_SIMULATOR_EXECUTER_HPP
//used for execute
#include <algorithm>
using namespace std;
namespace RA {
    unsigned HEXtoBIN(const string &HEXstr) {
        //TODO convert HEX number to unsigned int number
        unsigned ret = 0;
        for (int i = 0 ; i < HEXstr.length() ; ++i) {
            ret *= 16;
            if ('0' <= HEXstr[i] && HEXstr[i] <= '9') ret += HEXstr[i]-'0';
            else ret += HEXstr[i]-'A'+10;
        }
        return ret;
    }
    unsigned cut(unsigned x, unsigned h, unsigned l) {
        unsigned int s = h-l+1;
        return (x & ((1<<s)-1)<<l) >> l;
    }
    unsigned sign_ext(unsigned x, unsigned upbd) {
        //有符号扩展
        unsigned int s = 31-upbd;
        int y = x<<s;
        unsigned z = y>>s;
        return z;
    }
}
#endif //RISK_V_SIMULATOR_EXECUTER_HPP
