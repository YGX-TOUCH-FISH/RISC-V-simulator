//
// Created by 18163 on 2021/6/28.
//

#ifndef RISK_V_SIMULATOR_IO_HPP
#define RISK_V_SIMULATOR_IO_HPP
#include <iostream>
#include <fstream>
#include <sstream>
#include "Saver.hpp"
#include "Executer.hpp"
#include "exception.hpp"
//TODO hazard：
// //
// 结构冒险：同一时钟周期访问相同的硬件
// 寄存器的读写速度快，前半个时钟周期写，后半个时钟周期读
// 且设置独立的读写口
// //
// 数据冒险：这一条指令所需的数据还未被写入，但下一条指令需要该数据
// 1 插入nop指令
// 2 数据前递，由上一条指令的ALU结果传给下一条指令的ALU(Bypass)
// Load Use Hazard: 一条指令需要用到之前指令的访存结果，插入nop, forwarding
// //
// 控制冒险：下一次取指由上一条指令的结果确定
// 转移会使部分取指被废除
// 无条件间接转移：等待1clock至译码结束
// 条件转移：译码结束后，交由ALU比较(条件)。但如果条件是[是否相等]这种比较简单的条件，不需ALU，可在ID阶段结束后返回值

using namespace std;
namespace RA {
    class ioSystem {
    private:
        Memory memory;
        Register X;
        unsigned pc = 0;
        Order ID_EXE;
        Order EXE_MEM;
        Order MEM_WB;
        //两stage之间的指令状态
    private:
        unsigned read(unsigned offset, unsigned n) {
            unsigned ret = 0;
            for (int i = n-1 ; i >= 0 ; --i) {
                ret <<= 8;
                ret |= memory[offset+i];
            }
            return ret;
        }
        void write(unsigned msg, unsigned offset, int num){
            for (int i = 0 ; i < num ; ++i) {
                unsigned piece = cut(msg, i*8+7, i*8);
                memory[offset+i] = piece;
            }
        }
        unsigned IF(unsigned offset) {
            unsigned ret = 0;
            for (int i = 3 ; i >= 0 ; --i) {
                ret <<= 8;
                ret |= memory[offset+i];
            }
            return ret;
        }
        //Instrcution Fetch

        pair<Order, bool> ID(unsigned order) {
            Order ret;
            if (order == 0x0ff00513) {
                cout << (((unsigned)X[10]) & 255u);
                exit(0);
            }
            ret.opcode = cut(order, 6, 0);

            if (ret.opcode == 55) {
                ret.rd = cut(order, 11, 7);
                ret.xrd = X[ret.rd];
                ret.imm = cut(order, 31, 12)<<12;
                ret.type = lui;
                return make_pair(ret, false);
            }
            else if (ret.opcode == 23) {
                ret.rd = cut(order, 11, 7);
                ret.xrd = X[ret.rd];
                ret.imm = cut(order, 31, 12)<<12;
                ret.type = auipc;
                return make_pair(ret, false);
            }
            else if (ret.opcode == 111) {
                ret.rd = cut(order, 11, 7);
                ret.xrd = X[ret.rd];
                ret.type = jal;
                unsigned p1(cut(order, 31, 31));//offset 20
                unsigned p2(cut(order, 30, 21));//offset 10-1
                unsigned p3(cut(order, 20, 20));//offset 11
                unsigned p4(cut(order, 19, 12));//offset 19-12
                ret.imm = (p1<<20)|(p4<<12)|(p3<<11)|(p2<<1);
                ret.imm = sign_ext(ret.imm, 20);
                return make_pair(ret, true);
            }
            else if (ret.opcode == 103) {
                ret.rd = cut(order, 11, 7);
                ret.xrd = X[ret.rd];
                ret.rs1 = cut(order, 19, 15);
                ret.xrs1 = X[ret.rs1];
                ret.type = jalr;
                ret.imm = sign_ext(cut(order, 31, 20), 11);
                return make_pair(ret, true);
            }
            else if (ret.opcode == 99) {
                unsigned p1(cut(order, 31, 31));//offset 12
                unsigned p2(cut(order, 30, 25));//offset 10-5
                unsigned p3(cut(order, 11, 8));//offset 4-1
                unsigned p4(cut(order, 7, 7));//offset 11

                ret.imm = (p1<<12)|(p2<<5)|(p3<<1)|(p4<<11);
                ret.imm = sign_ext(ret.imm, 12);

                ret.funct3 = cut(order, 14, 12);
                ret.rs1 = cut(order, 19, 15);
                ret.rs2 = cut(order, 24, 20);
                ret.xrs1 = X[ret.rs1];
                ret.xrs2 = X[ret.rs2];
                switch (ret.funct3) {
                    case 0: ret.type = beq; break;
                    case 1: ret.type = bne; break;
                    case 4: ret.type = blt; break;
                    case 5: ret.type = bge; break;
                    case 6: ret.type = bltu; break;
                    case 7: ret.type = bgeu; break;
                }
                return make_pair(ret, false);
            }
            else if (ret.opcode == 3) {
                ret.rd = cut(order, 11, 7);
                ret.xrd = X[ret.rd];
                ret.funct3 = cut(order, 14, 12);
                ret.rs1 = cut(order, 19, 15);
                ret.xrs1 = X[ret.rs1];
                ret.imm = sign_ext(cut(order, 31, 20), 11);
                switch (ret.funct3) {
                    case 0: ret.type = lb; break;
                    case 1: ret.type = lh; break;
                    case 2: ret.type = lw; break;
                    case 4: ret.type = lbu;break;
                    case 5: ret.type = lhu;break;
                }
                return make_pair(ret, false);
            }
            else if (ret.opcode == 35) {
                ret.funct3 = cut(order, 14, 12);
                ret.rs1 = cut(order, 19, 15);
                ret.xrs1 = X[ret.rs1];
                ret.rs2 = cut(order, 24, 20);
                ret.xrs2 = X[ret.rs2];
                unsigned p1(cut(order, 31, 25));
                unsigned p2(cut(order, 11, 7));
                ret.imm = (p1<<5)|p2;
                ret.imm = sign_ext(ret.imm, 11);
                switch (ret.funct3) {
                    case 0: ret.type = sb;break;
                    case 1: ret.type = sh;break;
                    case 2: ret.type = sw;break;
                }
                return make_pair(ret, false);
            }
            else if (ret.opcode == 19) {
                ret.rd = cut(order, 11, 7);
                ret.xrd = X[ret.rd];
                ret.funct3 = cut(order, 14, 12);
                ret.rs1 = cut(order, 19, 15);
                ret.xrs1 = X[ret.rs1];
                ret.imm = sign_ext(cut(order, 31, 20), 11);
                ret.shamt = cut(order, 25, 20);
                ret.funct7 = cut(order, 31, 25);
                switch (ret.funct3) {
                    case 0:ret.type = addi;break;
                    case 1:ret.type = slli;break;
                    case 2:ret.type = slti;break;
                    case 3:ret.type = sltiu;break;
                    case 4:ret.type = xori;break;
                    case 5: {
                        if (ret.funct7 == 0) ret.type = srli;
                        else ret.type = srai;
                        break;
                    }
                    case 6:ret.type = ori;break;
                    case 7:ret.type = andi;break;
                }
                return make_pair(ret, false);
            }
            else if (ret.opcode == 51) {
                ret.rd = cut(order, 11, 7);
                ret.xrd = X[ret.rd];
                ret.rs1 = cut(order, 19, 15);
                ret.xrs1 = X[ret.rs1];
                ret.rs2 = cut(order, 24, 20);
                ret.xrs2 = X[ret.rs2];
                ret.funct3 = cut(order, 14, 12);
                ret.funct7 = cut(order, 31, 25);
                switch (ret.funct3) {
                    case 0: {
                        if (ret.funct7 == 0) ret.type = add;
                        else ret.type = sub;
                        break;
                    }
                    case 1: ret.type = sll;break;
                    case 2: ret.type = slt;break;
                    case 3: ret.type = sltu;break;
                    case 4: ret.type = xorr;break;
                    case 5: {
                        if (ret.funct7 == 0)  ret.type = srl;
                        else ret.type = sra;
                        break;
                    }
                    case 6: ret.type = orr;break;
                    case 7: ret.type = andd;break;
                }
                return make_pair(ret, false);
            }
            else throw syntax_error();
        }
        //分解order,计算imm、寄存器的值、进行符号扩展
        //TODO 计算关于pc的possible branch值...(risk)
        //获取寄存器的值
        //Instruction Decode

        pair<Order, bool> EXE(Order order) {
            Order ret(order);
            bool jump = false;
            switch (ret.type) {
                case lui: {
                    ret.xrd = ret.imm;
                    break;
                }
                case auipc: {
                    ret.xrd = pc + ret.imm;
                    break;
                }
                case jal: {
                    if (ret.rd != 0) ret.xrd = pc+4;
                    pc += ret.imm;
                    jump = true;
                    break;
                }
                case jalr: {
                    unsigned t = pc+4;
                    pc = (ret.xrs1+ret.imm)&(-1);
                    if (ret.rd != 0) ret.xrd = t;
                    jump = true;
                    break;
                }
                case beq: {
                    if (ret.xrs1 == ret.xrs2) pc += ret.imm, jump = true;
                    break;
                }
                case bne: {
                    if (ret.xrs1 != ret.xrs2) pc += ret.imm, jump = true;
                    break;
                }
                case blt: {
                    if ((signed)ret.xrs1 < (signed)ret.xrs2) pc += ret.imm, jump = true;
                    break;
                }
                case bge: {
                    if ((signed)ret.xrs1 >= (signed)ret.xrs2) pc += ret.imm, jump = true;
                    break;
                }
                case bltu: {
                    if (ret.xrs1 < ret.xrs2) pc += ret.imm, jump = true;
                    break;
                }
                case bgeu: {
                    if (ret.xrs1 >= ret.xrs2) pc += ret.imm, jump = true;
                    break;
                }
                //
                case lb: {
                    ret.output = ret.xrs1 + ret.imm;
                    break;
                }
                case lh: {
                    ret.output = ret.xrs1 + ret.imm;
                    break;
                }
                case lw: {
                    ret.output = ret.xrs1 + ret.imm;
                    break;
                }
                case lbu: {
                    ret.output = ret.xrs1 + ret.imm;
                    break;
                }
                case lhu: {
                    ret.output = ret.xrs1 + ret.imm;
                    break;
                }
                case sb: {
                    ret.output = ret.xrs1 + ret.imm;
                    break;
                }
                case sh: {
                    ret.output = ret.xrs1 + ret.imm;
                    break;
                }
                case sw: {
                    ret.output = ret.xrs1 + ret.imm;
                    break;
                }
                //访存：计算出地址
                case addi: {
                    ret.xrd = ret.xrs1+ret.imm;
                    break;
                }
                case slti: {
                    if (ret.xrs1 < (signed)ret.imm) ret.xrd = 1;
                    else ret.xrd = 0;
                    break;
                }
                case sltiu: {
                    if (ret.xrs1 < ret.imm) ret.xrd = 1;
                    else ret.xrd = 0;
                    break;
                }
                case xori: {
                    ret.xrd = ret.xrs1^ret.imm;
                    break;
                }
                case ori: {
                    ret.xrd = ret.xrs1|ret.imm;
                    break;
                }
                case andi: {
                    ret.xrd = ret.xrs1&ret.imm;
                    break;
                }
                case slli: {
                    ret.xrd = ret.xrs1<<ret.shamt;
                    break;
                }
                case srli: {
                    ret.xrd = ret.xrs1>>ret.shamt;
                    break;
                }
                case srai: {
                    if (ret.xrs1>>31 == 0) ret.xrd = ret.xrs1>>ret.shamt;
                    else {
                        unsigned a = (1<<ret.shamt)-1, b = ret.xrs1>>ret.shamt;
                        a<<=(32-ret.shamt);
                        ret.xrd = a|b;
                    }
                    break;
                }
                case add: {
                    ret.xrd = ret.xrs1 + ret.xrs2;
                    break;
                }
                case sub: {
                    ret.xrd = ret.xrs1 - ret.xrs2;
                    break;
                }
                case sll: {
                    ret.xrd = ret.rs1<<ret.shamt;
                    break;
                }
                case slt: {
                    if ((signed)ret.xrs1 < (signed)ret.xrs2) ret.xrd = 1;
                    else ret.xrd = 0;
                    break;
                }
                case sltu: {
                    if (ret.xrs1 < ret.xrs2) ret.xrd = 1;
                    else ret.xrd = 0;
                    break;
                }
                case xorr: {
                    ret.xrd = ret.xrs1^ret.xrs2;
                    break;
                }
                case srl: {
                    unsigned shmt = cut(ret.xrs2, 4, 0);
                    ret.xrd = ret.xrs1>>shmt;
                    break;
                }
                case sra: {
                    unsigned shmt = cut(ret.xrs2, 4, 0);
                    ret.xrd = ret.xrs1>>shmt;
                    ret.xrd = sign_ext(ret.xrd, 31-shmt);
                    break;
                }
                case orr: {
                    ret.xrd = ret.xrs1|ret.xrs2;
                    break;
                }
                case andd: {
                    ret.xrd = ret.xrs1&ret.xrs2;
                    break;
                }
            }
            return make_pair(ret, jump);
        }
        //算数指令：完成运算
        //访存指令：计算出对应地址
        //Execute

        Order MEM(Order order) {
            Order ret(order);
            switch (ret.type) {
                case lb: {
                    ret.xrd = sign_ext(read(ret.output, 1), 7);
                    break;
                }
                case lh: {
                    ret.xrd = sign_ext(read(ret.output, 2), 15);
                    break;
                }
                case lw: {
                    ret.xrd = read(ret.output, 4);
                    break;
                }
                case lbu: {
                    ret.xrd = read(ret.output, 1);
                    break;
                }
                case lhu: {
                    ret.xrd = read(ret.output, 2);
                    break;
                }
                case sb: {
                    write(cut(ret.xrs2, 7, 0), ret.output, 1);
                    break;
                }
                case sh: {
                    write(cut(ret.xrs2, 15, 0), ret.output, 2);
                    break;
                }
                case sw: {
                    write(ret.xrs2, ret.output, 4);
                    break;
                }
            }
            return ret;
        }
        //仅对访存指令进行操作
        //Memory Access

        Order WB(Order order) {
            Order ret(order);
            switch (ret.type) {
                case lui: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case auipc: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case jal: {
                    if (ret.rd != 0) X[ret.rd] = ret.xrd;
                    break;
                }
                case jalr: {
                    if (ret.rd != 0) X[ret.rd] = ret.xrd;
                    break;
                }
                case lb: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case lh: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case lw: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case lbu: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case lhu: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case addi: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case slti: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case sltiu: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case xori: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case ori: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case andi: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case slli: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case srli: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case srai: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case add: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case sub: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case sll: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case slt: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case sltu: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case xorr: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case srl: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case sra: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case orr: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
                case andd: {
                    X[ret.rd] = ret.xrd;
                    break;
                }
            }
            return ret;
        }
        //对于需要写回的指令
        //Write Back
    public:
        ioSystem() = default;
        ~ioSystem() = default;
        void run() {
            while (true) {
                try {
                    bool jump = false, jump1 = false, jump2 = false;
                    unsigned order = IF(pc);
                    if (pc == 4384) {
                        int x = 1;
                    }
                    unsigned pc_update = pc+4;
                    pair<Order, bool> IDret = ID(order);
                    ID_EXE = IDret.first;
                    jump1 = IDret.second;
                    pair<Order, bool> EXEret = EXE(ID_EXE);
                    EXE_MEM = EXEret.first;
                    jump2 = EXEret.second;
                    MEM_WB = MEM(EXE_MEM);
                    WB(MEM_WB);

                    if (!jump1 && !jump2) pc = pc_update;
                } catch (...) {
                    exit(-1);
                }
            }
        }
        void input() {
            string str, record;
            while (getline(cin, str)) {
                if (str[0] == '@') {
                    record = str.substr(1);
                    pc = HEXtoBIN(record);
                }
                else {
                    string section;
                    stringstream ss(str);
                    while (ss >> section) {
                        unsigned mem = HEXtoBIN(section);
                        memory[pc] = mem;
                        section = "";
                        ++pc;
                    }
                }
            }
            pc = 0;
        }
    };
}
#endif //RISK_V_SIMULATOR_IO_HPP
