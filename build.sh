#!/bin/bash

clang++ -fno-rtti -fno-exceptions test.cc $(../llvm-install/bin/llvm-config --cxxflags --ldflags --libs) -lclangFrontend     -lclangDriver     -lclangSerialization     -lclangParse     -lclangSema     -lclangAnalysis     -lclangRewrite     -lclangEdit     -lclangAST     -lclangLex     -lclangBasic     -lLLVMMC     -lLLVMSupport -O0 -g
