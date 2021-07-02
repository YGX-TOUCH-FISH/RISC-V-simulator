//
// Created by 18163 on 2021/6/28.
//
#include <iostream>
#include "IO.hpp"
using namespace std;
//TODO
// 5-stage pipeline
int main() {
    RA::ioSystem System;
//    freopen("../testcases/pi.data", "r", stdin);
    System.input();
    System.run();
    return 0;
}