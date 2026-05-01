/**
 * @brief Modern C++ implementation dengan RAII untuk ABI dan Calling Conventions
 */

#ifndef CCVM_FRONTEND_ABI_H
#define CCVM_FRONTEND_ABI_H

#include <vector>
#include <string>

namespace ccvm {
namespace frontend {

enum Register {
    RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP,
    R8, R9, R10, R11, R12, R13, R14, R15,
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    X0, X1, X2, X3, X4, X5, X6, X7, X8, X9,
    X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, X29, X30,
    V0, V1, V2, V3, V4, V5, V6, V7,
    A0, A1, A2, A3, A4, A5, A6, A7,
    T0, T1, T2, T3, T4, T5, T6,
    S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11,
    FA0, FA1, FA2, FA3, FA4, FA5, FA6, FA7
};

struct CallingConvention {
    std::vector<Register> integer_args;
    std::vector<Register> float_args;
    std::vector<Register> caller_saved;
    std::vector<Register> callee_saved;
    Register return_register;
    int stack_alignment;
    int red_zone_size;
};

class ABI {
public:
    // Microsoft x64 ABI (Windows)
    static CallingConvention get_microsoft_x64() {
        return CallingConvention {
            .integer_args = {RCX, RDX, R8, R9},
            .float_args = {XMM0, XMM1, XMM2, XMM3},
            .caller_saved = {RAX, RCX, RDX, R8, R9, R10, R11},
            .callee_saved = {RBX, RBP, RDI, RSI, R12, R13, R14, R15},
            .return_register = RAX,
            .stack_alignment = 16,
            .red_zone_size = 0  // No red zone in Windows
        };
    }
    
    // ARM64 AAPCS
    static CallingConvention get_aapcs64() {
        return CallingConvention {
            .integer_args = {X0, X1, X2, X3, X4, X5, X6, X7},
            .float_args = {V0, V1, V2, V3, V4, V5, V6, V7},
            .caller_saved = {X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12, X13, X14, X15, X16, X17},
            .callee_saved = {X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, X29, X30},
            .return_register = X0,
            .stack_alignment = 16,
            .red_zone_size = 0
        };
    }
    
    // RISC-V calling convention
    static CallingConvention get_riscv() {
        return CallingConvention {
            .integer_args = {A0, A1, A2, A3, A4, A5, A6, A7},
            .float_args = {FA0, FA1, FA2, FA3, FA4, FA5, FA6, FA7},
            .caller_saved = {T0, T1, T2, T3, T4, T5, T6, A0, A1, A2, A3, A4, A5, A6, A7},
            .callee_saved = {S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11},
            .return_register = A0,
            .stack_alignment = 16,
            .red_zone_size = 0
        };
    }
};

} // namespace frontend
} // namespace ccvm

#endif
