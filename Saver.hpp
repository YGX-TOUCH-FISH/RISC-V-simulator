//
// Created by 18163 on 2021/6/28.
//

#ifndef RISK_V_SIMULATOR_SAVER_HPP
#define RISK_V_SIMULATOR_SAVER_HPP
//register, memory and etc.
namespace RA{
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

    class Memory {
        unsigned int mem[501000]{};
    public:
        explicit Memory() {
            for (int i = 0 ; i < 500000 ; ++i)
                mem[i] = 0;
        }
        ~Memory() = default;


        unsigned int &operator[](const unsigned int &foo) {
            return mem[foo];
        }

    };
    enum OrderType {
        lui, auipc,
        jal, jalr,
        beq, bne, blt, bge, bltu, bgeu,
        lb, lh, lw, lbu, lhu,
        sb, sh, sw,
        addi, slti, sltiu, xori, ori, andi, slli, srli, srai,
        add, sub, sll, slt, sltu, xorr, srl, sra, orr, andd,
        uninit
    };

    enum TYPE {
        R, I, S, B, U, J, N
    };
    class Order {
    public:
        unsigned opcode = 0;
        unsigned rd = 0;
        unsigned rs1 = 0;
        unsigned rs2 = 0;
        unsigned shamt = 0;
        unsigned xrd = 0;
        unsigned xrs1 = 0;
        unsigned xrs2 = 0;
        unsigned imm = 0;
        unsigned funct3 = 0;
        unsigned funct7 = 0;
        unsigned output = 0;
        unsigned pc = 0;//当前指令的pc值
        OrderType type = uninit;

        TYPE clas = N;
        bool isNop = true;
        bool jump = false;

        Order() = default;
        ~Order() = default;
        bool isLoad() const{
            return (type == lb) || (type == lh) || (type == lw) || (type == lbu) || (type == lhu);
        }
    };

}
#endif //RISK_V_SIMULATOR_SAVER_HPP
