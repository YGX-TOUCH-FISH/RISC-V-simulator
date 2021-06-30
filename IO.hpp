//
// Created by 18163 on 2021/6/28.
//

#ifndef RISK_V_SIMULATOR_IO_HPP
#define RISK_V_SIMULATOR_IO_HPP
//used for read Machine Order from stdin, write Ret by stdout.
#include <iostream>
#include <fstream>
#include <sstream>
#include "Saver.hpp"
#include "Executer.hpp"
#include "exception.hpp"
using namespace std;
namespace RA {
    class ioSystem {
    private:
        Memory memory;
        //用32位的unsigned int存储memory, memory[i]储存了1个字节的信息
        Register X;
        unsigned int pc = 0;
    private:
        //high-pos is behind
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

        unsigned fetch(unsigned offset) {
            //[offset], .. [offset+3]
            unsigned ret = 0;
            for (int i = 3 ; i >= 0 ; --i) {
                ret <<= 8;
                ret |= memory[offset+i];
            }
            return ret;
        }//fetch 4 memory everytime, change direction

        bool Execute(unsigned Order) {
            if (Order == 0x0ff00513) {
                cout << (((unsigned)X[10]) & 255u);
                exit(0);
            }
            unsigned int order(Order), opcode(0), funct3(0), funct7(0), rd(0), rs1(0), rs2(0), offset(0), mem(0);
            opcode = cut(order, 6, 0);
            if (opcode == 55) {
                //Load Upper Immediate
                // 将立即数imm加载到寄存器x[rd]的高20位
                rd = cut(order, 11, 7);
                X[rd] = cut(order, 31, 12)<<12;
            }
            //0110111
            else if (opcode == 23) {
                //AUIPC
                rd = cut(order, 11, 7);
                X[rd] = pc + (cut(order, 31, 12)<<12);
            }
            //0010111
            else if (opcode == 111) {
                //Jump And Link
                rd = cut(order, 11, 7);
//                if (rd == 0) throw syntax_error();
                unsigned p1(cut(order, 31, 31));//offset 20
                unsigned p2(cut(order, 30, 21));//offset 10-1
                unsigned p3(cut(order, 20, 20));//offset 11
                unsigned p4(cut(order, 19, 12));//offset 19-12

                if (rd != 0) X[rd] = pc + 4;
                unsigned imm = (p1<<20)|(p4<<12)|(p3<<11)|(p2<<1);
                pc += sign_ext(imm, 20);
                return true;
            }
            //1101111
            else if (opcode == 103) {
                //Jump And Link Register
                rd = cut(order, 11, 7);
                rs1 = cut(order, 19, 15);
                unsigned t = pc + 4;
                unsigned imm = sign_ext(cut(order, 31, 20), 11);
                pc = (X[rs1]+ imm)&(-1);
                if (rd != 0) X[rd] = t;
                return true;
            }
            //1100111
            else if (opcode == 99) {
                unsigned offset1(cut(order, 31, 31));//offset 12
                unsigned offset2(cut(order, 30, 25));//offset 10-5
                unsigned offset3(cut(order, 11, 8));//offset 4-1
                unsigned offset4(cut(order, 7, 7));//offset 11
                offset = (offset1<<12)|(offset2<<5)|(offset3<<1)|(offset4<<11);
                offset = sign_ext(offset, 12);

                funct3 = cut(order, 14, 12);
                rs1 = cut(order, 19, 15);
                rs2 = cut(order, 24, 20);

                if (funct3 == 0) {
                    if (X[rs1] == X[rs2]) {
                        pc += offset;
                        return true;
                    }
                }//beq
                else if (funct3 == 1) {
                    if (X[rs1] != X[rs2]) {
                        pc += offset;
                        return true;
                    }
                }//bne
                else if (funct3 == 4) {
                    //视为二进制补码：signed
                    if ((signed)X[rs1] < (signed)X[rs2]) {
                        pc += offset;
                        return true;
                    }
                }//blt
                else if (funct3 == 5) {
                    if ((signed)X[rs1] >= (signed)X[rs2]) {
                        pc += offset;
                        return true;
                    }
                }//bge
                else if (funct3 == 6) {
                    if (X[rs1] < X[rs2]) {
                        pc += offset;
                        return true;
                    }
                }//bltu
                else if (funct3 == 7) {
                    if (X[rs1] >= X[rs2]) {
                        pc += offset;
                        return true;
                    }
                }//BGEU
                else throw syntax_error();
            }
            //1100011
            else if (opcode == 3) {
                rd = cut(order, 11, 7);
                funct3 = cut(order, 14, 12);
                rs1 = cut(order, 19, 15);
                offset = X[rs1] + sign_ext(cut(order, 31, 20), 11);
                if (rd == 0) throw syntax_error();
                if (funct3 == 0) {
                    mem = read(offset, 1);
                    X[rd] = sign_ext(mem, 7);
                    //Load Byte
                }
                else if (funct3 == 1) {
                    mem = read(offset, 2);
                    X[rd] = sign_ext(mem, 15);
                    //Load Halfword
                }
                else if (funct3 == 2) {
                    X[rd] = read(offset, 4);
                    //Load Word
                }
                else if (funct3 == 4) {
                    X[rd] = read(offset, 1);
                    //Load Byte Unsigned
                }
                else if (funct3 == 5) {
                    X[rd] = read(offset, 2);
                    //Load Halfword Unsigned
                }
                else throw unknown_field();
            }
            //0000011
            else if (opcode == 35) {
                funct3 = cut(order, 14, 12);
                rs1 = cut(order, 19, 15);
                rs2 = cut(order, 24, 20);
                unsigned offset1(cut(order, 31, 25)), offset2(cut(order, 11, 7));
                offset1 <<= 5;
                offset = X[rs1] + sign_ext(offset1|offset2, 11);
                if (funct3 == 0) {
                    //Save Byte
                    unsigned byt(cut(X[rs2], 7, 0));
                    write(byt, offset, 1);
                }
                else if (funct3 == 1) {
                    //Save Halfword
                    unsigned byt(cut(X[rs2], 15, 0));
                    write(byt, offset, 2);
                }
                else if (funct3 == 2) {
                    //Save Word
                    unsigned byt = X[rs2];
                    write(byt, offset, 4);
                }
                else throw unknown_field();
            }
            //0100011
            else if (opcode == 19) {
                rd = cut(order, 11, 7);
                funct3 = cut(order, 14, 12);
                rs1 = cut(order, 19, 15);
                if (rd == 0) throw syntax_error();
                if (funct3 == 0) {
                    //addi
                    unsigned imm = sign_ext(cut(order, 31, 20), 11);
                    X[rd] = X[rs1] + imm;
                }
                else if (funct3 == 2) {
                    //slti: set if less than immediate
                    unsigned immediate = sign_ext(cut(order, 31, 20), 11);
                    if (X[rs1] < (signed)immediate) X[rd] = 1;
                    else X[rd] = 0;
                }
                else if (funct3 == 3) {
                    //sltiu: set if less than immediate, unsigned
                    unsigned immediate = sign_ext(cut(order, 31, 20), 11);
                    if (X[rs1] < immediate) X[rd] = 1;
                    else X[rd] = 0;
                }
                else if (funct3 == 4) {
                    //Exclusive-Or immediate
                    unsigned immediate = sign_ext(cut(order, 31, 20), 11);
                    X[rd] = X[rs1]^immediate;
                }
                else if (funct3 == 6) {
                    //OR immediate
                    unsigned immediate = sign_ext(cut(order, 31, 20), 11);
                    X[rd] = X[rs1]|immediate;
                }
                else if (funct3 == 7) {
                    //And immediate
                    unsigned immediate = sign_ext(cut(order, 31, 20), 11);
                    X[rd] = X[rs1]&immediate;
                }
                else if (funct3 == 1) {
                    //shift left logical immediate
                    unsigned shamt = cut(order, 25, 20);
                    if (cut(shamt, 5, 5) != 0) throw syntax_error();
                    X[rd] = X[rs1]<<shamt;
                }
                else if (funct3 == 5) {
                    unsigned flag = cut(order, 31, 25);
                    if (flag == 0) {
                        //shift right logical immediate
                        unsigned shamt = cut(order, 25, 20);
                        if (cut(shamt, 5, 5) != 0) throw syntax_error();
                        X[rd] = X[rs1]>>shamt;
                    }
                    else if (flag == 1<<5) {
                        //shift right arithmetic immediate
                        unsigned shamt = cut(order, 25, 20);
                        if (cut(shamt, 5, 5) != 0) throw syntax_error();

                        if (X[rs1]>>31 == 0) X[rd] = X[rs1]>>shamt;
                        else {
                            unsigned a = (1<<shamt)-1, b = X[rs1]>>shamt;
                            a<<=(32-shamt);
                            X[rd] = a|b;
                        }
                    }
                    else throw invalid_visit();
                }
            }
            //0010011
            else if (opcode == 51) {
                rd = cut(order, 11, 7);
                funct3 = cut(order, 14, 12);
                rs1 = cut(order, 19, 15);
                rs2 = cut(order, 24, 20);
                funct7 = cut(order, 31, 25);
                if (rd == 0) throw syntax_error();
                if (funct3 == 0 && funct7 == 0) {
                    //add
                    X[rd] = X[rs1]+X[rs2];
                }
                else if (funct3 == 0 && funct7 == 32) {
                    //sub
                    X[rd] = X[rs1]-X[rs2];
                }
                else if (funct3 == 1) {
                    //sll
                    unsigned shamt = cut(order, 25, 20);
                    if (cut(shamt, 5, 5) != 0) throw invalid_visit();
                    X[rd] = X[rs1] << shamt;
                }
                else if (funct3 == 2) {
                    //slt
                    if ((signed)X[rs1] < (signed)X[rs2]) X[rd] = 1;
                    else X[rd] = 0;
                }
                else if (funct3 == 3) {
                    //sltu
                    if (X[rs1] < X[rs2]) X[rd] = 1;
                    else X[rd] = 0;
                }
                else if (funct3 == 4) {
                    //xor
                    X[rd] = X[rs1]^X[rs2];
                }
                else if (funct3 == 5 && funct7 == 0) {
                    //srl
                    unsigned shamt = cut(X[rs2], 4, 0);
                    X[rd] = X[rs1]>>shamt;
                }
                else if (funct3 == 5 && funct7 == 32) {
                    //sra
                    unsigned shamt = cut(order, 25, 20);
                    X[rd] = sign_ext(X[rs1] >>(signed)shamt, 31-(signed)shamt);
                }
                else if (funct3 == 6) {
                    //or
                    X[rd] = X[rs1]|X[rs2];
                }
                else if (funct3 == 7) {
                    //and
                    X[rd] = X[rs1]&X[rs2];
                }
            }
            //0110011

            else throw syntax_error();
            return false;
            //TODO
            // 注意，装入目的寄存器如果为x0，将会产生一个异常
            // 所有的memory访问都是通过load/store指令
            // 其它指令都是在寄存器之间或者寄存器和立即数之间进行运算，比如加法指令，减法指令等等
        }
    public:
        ioSystem() = default;
        ~ioSystem() = default;
        void run() {
            string str, record;
            while (getline(cin, str)) {
                if (str[0] == '@') {
                    record = str.substr(1);
                    pc = HEXtoBIN(record);
                    //choose地址一定是4的倍数
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
            while (true) {
                unsigned order = fetch(pc);
                try {
                    if (!Execute(order)) pc+=4;
                    //Execute false: 无跳转
                    //Execute true: 有跳转
                } catch (...) {
                    exit(1);
                }
            }
        }
    };
}
#endif //RISK_V_SIMULATOR_IO_HPP
