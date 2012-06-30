#
# Build against a Clang 3 installation.
#

LLVM_DIR=/home/rprichard/llvm-install

INCLUDEPATH += $${LLVM_DIR}/include
QMAKE_CXXFLAGS += -fPIC -fvisibility-inlines-hidden -fno-rtti -fno-exceptions -pthread -Wno-unused-parameter -Wno-strict-aliasing
DEFINES += _GNU_SOURCE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS __STDC_LIMIT_MACROS
LIBS += \
    -ldl \
    -L$${LLVM_DIR}/lib \
    -lLLVMAsmParser \
    -lLLVMInstrumentation \
    -lLLVMLinker \
    -lLLVMArchive \
    -lLLVMBitReader \
    -lLLVMDebugInfo \
    -lLLVMJIT \
    -lLLVMipo \
    -lLLVMVectorize \
    -lLLVMBitWriter \
    -lLLVMTableGen \
    -lLLVMXCoreCodeGen \
    -lLLVMXCoreDesc \
    -lLLVMXCoreInfo \
    -lLLVMX86AsmParser \
    -lLLVMX86CodeGen \
    -lLLVMX86Disassembler \
    -lLLVMX86Desc \
    -lLLVMX86Info \
    -lLLVMX86AsmPrinter \
    -lLLVMX86Utils \
    -lLLVMSparcCodeGen \
    -lLLVMSparcDesc \
    -lLLVMSparcInfo \
    -lLLVMPowerPCCodeGen \
    -lLLVMPowerPCDesc \
    -lLLVMPowerPCInfo \
    -lLLVMPowerPCAsmPrinter \
    -lLLVMNVPTXCodeGen \
    -lLLVMNVPTXDesc \
    -lLLVMNVPTXInfo \
    -lLLVMNVPTXAsmPrinter \
    -lLLVMMSP430CodeGen \
    -lLLVMMSP430Desc \
    -lLLVMMSP430Info \
    -lLLVMMSP430AsmPrinter \
    -lLLVMMBlazeDisassembler \
    -lLLVMMBlazeAsmParser \
    -lLLVMMBlazeCodeGen \
    -lLLVMMBlazeDesc \
    -lLLVMMBlazeInfo \
    -lLLVMMBlazeAsmPrinter \
    -lLLVMMipsDisassembler \
    -lLLVMMipsCodeGen \
    -lLLVMMipsAsmParser \
    -lLLVMMipsDesc \
    -lLLVMMipsInfo \
    -lLLVMMipsAsmPrinter \
    -lLLVMHexagonCodeGen \
    -lLLVMHexagonAsmPrinter \
    -lLLVMHexagonDesc \
    -lLLVMHexagonInfo \
    -lLLVMCppBackendCodeGen \
    -lLLVMCppBackendInfo \
    -lLLVMCellSPUCodeGen \
    -lLLVMCellSPUDesc \
    -lLLVMCellSPUInfo \
    -lLLVMARMDisassembler \
    -lLLVMARMAsmParser \
    -lLLVMARMCodeGen \
    -lLLVMSelectionDAG \
    -lLLVMAsmPrinter \
    -lLLVMARMDesc \
    -lLLVMARMInfo \
    -lLLVMARMAsmPrinter \
    -lLLVMMCDisassembler \
    -lLLVMMCParser \
    -lLLVMInterpreter \
    -lLLVMCodeGen \
    -lLLVMScalarOpts \
    -lLLVMInstCombine \
    -lLLVMTransformUtils \
    -lLLVMipa \
    -lLLVMAnalysis \
    -lLLVMMCJIT \
    -lLLVMRuntimeDyld \
    -lLLVMExecutionEngine \
    -lLLVMTarget \
    -lLLVMMC \
    -lLLVMObject \
    -lLLVMCore \
    -lLLVMSupport
LIBS += \
    -lclangFrontend \
    -lclangDriver \
    -lclangSerialization \
    -lclangParse \
    -lclangSema \
    -lclangAnalysis \
    -lclangRewrite \
    -lclangEdit \
    -lclangAST \
    -lclangLex \
    -lclangBasic
LIBS += \
    -lLLVMMC \
    -lLLVMSupport
