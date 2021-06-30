//
// Created by 18163 on 2021/6/28.
//

#ifndef RISK_V_SIMULATOR_SAVER_HPP
#define RISK_V_SIMULATOR_SAVER_HPP
//register, memory and etc.
namespace RA{
    //32位寄存器
    class Register {
        unsigned int reg[32]={0};
    public:
        Register() {
            for (int i = 0 ; i < 32 ; ++i)
                reg[i] = 0;
        }
        ~Register() = default;
        unsigned int &operator[](const unsigned int &foo) {
            return reg[foo];
        }
    };

    //内存池
    class Memory {
        unsigned int *mem;
    public:
        explicit Memory(int size = 1<<20) {
            mem = new unsigned int [size];
            for (int i = 0 ; i < size ; ++i)
                mem[i] = 0;
        }
        ~Memory() {
            delete []mem;
        }


        unsigned int &operator[](const unsigned int &foo) {
            return mem[foo];
        }

    };
}
#endif //RISK_V_SIMULATOR_SAVER_HPP
