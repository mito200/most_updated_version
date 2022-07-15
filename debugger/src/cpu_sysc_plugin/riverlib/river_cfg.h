// 
//  Copyright 2022 Sergey Khabarov, sergeykhbr@gmail.com
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
// 
//      http://www.apache.org/licenses/LICENSE-2.0
// 
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// 
#pragma once

#include <systemc.h>
#include "../ambalib/types_amba.h"

namespace debugger {

// 
// 2**Number of CPU slots in the clusters. Some of them could be unavailable
// 
static const int CFG_LOG2_CPU_MAX = 2;
static const int CFG_CPU_MAX = (1 << CFG_LOG2_CPU_MAX);

static const sc_uint<32> CFG_VENDOR_ID = 0x000000F1;
static const sc_uint<32> CFG_IMPLEMENTATION_ID = 0x20191123;
static const bool CFG_HW_FPU_ENABLE = true;

static const int RISCV_ARCH = 64;

static const int CFG_CPU_ADDR_BITS = CFG_BUS_ADDR_WIDTH;
static const int CFG_CPU_ID_BITS = 1;
static const int CFG_CPU_USER_BITS = 1;

// 
// Branch Predictor Branch Target Buffer (BTB) size
// 
static const int CFG_BTB_SIZE = 8;
// Branch predictor depth. It is better when it is equal to the pipeline depth excluding fetcher.
// Let it be equal to the decoder's history depth
static const int CFG_BP_DEPTH = 5;

// 
// Decoded instructions history buffer size in Decoder
// 
static const int CFG_DEC_DEPTH = (CFG_BP_DEPTH - 3);        // requested, fetching, fetched

// Power-on start address can be free changed
static const sc_uint<CFG_CPU_ADDR_BITS> CFG_RESET_VECTOR = 0x10000;

// Valid size 0..16
static const int CFG_PROGBUF_REG_TOTAL = 16;
// Must be at least 2 to support RV64I
static const int CFG_DATA_REG_TOTAL = 4;
// Total number of dscratch registers
static const int CFG_DSCRATCH_REG_TOTAL = 2;
// Number of elements each 2*CFG_ADDR_WIDTH in stack trace buffer:
static const int CFG_LOG2_STACK_TRACE_ADDR = 5;
static const int STACK_TRACE_BUF_SIZE = (1 << CFG_LOG2_STACK_TRACE_ADDR);

// 
// ICacheLru config (16 KB by default)
// 
static const int CFG_ILOG2_BYTES_PER_LINE = 5;              // [4:0] 32 Bytes = 4x8 B log2(Bytes per line)
static const int CFG_ILOG2_LINES_PER_WAY = 7;
static const int CFG_ILOG2_NWAYS = 2;

// Derivatives I$ constants:
static const int ICACHE_BYTES_PER_LINE = (1 << CFG_ILOG2_BYTES_PER_LINE);
static const int ICACHE_LINES_PER_WAY = (1 << CFG_ILOG2_LINES_PER_WAY);
static const int ICACHE_WAYS = (1 << CFG_ILOG2_NWAYS);
static const int ICACHE_LINE_BITS = (8 * ICACHE_BYTES_PER_LINE);

// Information: To define the CACHE SIZE in Bytes use the following:
static const int ICACHE_SIZE_BYTES = (ICACHE_WAYS * (ICACHE_LINES_PER_WAY * ICACHE_BYTES_PER_LINE));

static const int ITAG_FL_TOTAL = 1;


// 
// DCacheLru config (16 KB by default)
// 
static const int CFG_DLOG2_BYTES_PER_LINE = 5;              // [4:0] 32 Bytes = 4x8 B log2(Bytes per line)
static const int CFG_DLOG2_LINES_PER_WAY = 7;               // 7=16KB; 8=32KB; ..
static const int CFG_DLOG2_NWAYS = 2;

// Derivatives D$ constants:
static const int DCACHE_BYTES_PER_LINE = (1 << CFG_DLOG2_BYTES_PER_LINE);
static const int DCACHE_LINES_PER_WAY = (1 << CFG_DLOG2_LINES_PER_WAY);
static const int DCACHE_WAYS = (1 << CFG_DLOG2_NWAYS);

static const int DCACHE_LINE_BITS = (8 * DCACHE_BYTES_PER_LINE);

// Information: To define the CACHE SIZE in Bytes use the following:
static const int DCACHE_SIZE_BYTES = (DCACHE_WAYS * (DCACHE_LINES_PER_WAY * DCACHE_BYTES_PER_LINE));

static const int TAG_FL_VALID = 0;                          // always 0
static const int DTAG_FL_DIRTY = 1;
static const int DTAG_FL_SHARED = 2;
static const int DTAG_FL_RESERVED = 3;
static const int DTAG_FL_TOTAL = 4;


// 
// L1 cache common parameters (suppose I$ and D$ have the same size)
// 
static const int L1CACHE_BYTES_PER_LINE = DCACHE_BYTES_PER_LINE;
static const int L1CACHE_LINE_BITS = (8 * DCACHE_BYTES_PER_LINE);

static const int SNOOP_REQ_TYPE_READDATA = 0;               // 0=check flags; 1=data transfer
static const int SNOOP_REQ_TYPE_READCLEAN = 1;              // 0=do nothing; 1=read and invalidate line
static const int SNOOP_REQ_TYPE_BITS = 2;


// 
// L2 cache config (River 16 KB by default, Wasserfall 64 KB)
// 
static const int CFG_L2_LOG2_BYTES_PER_LINE = 5;            // [4:0] 32 Bytes = 4x8 B log2(Bytes per line)
static const int CFG_L2_LOG2_LINES_PER_WAY = 7;             // 7=16KB; 8=32KB; 9=64KB, ..
static const int CFG_L2_LOG2_NWAYS = 2;

// Derivatives D$ constants:
static const int L2CACHE_BYTES_PER_LINE = (1 << CFG_L2_LOG2_BYTES_PER_LINE);
static const int L2CACHE_LINES_PER_WAY = (1 << CFG_L2_LOG2_LINES_PER_WAY);
static const int L2CACHE_WAYS = (1 << CFG_L2_LOG2_NWAYS);

static const int L2CACHE_LINE_BITS = (8 * L2CACHE_BYTES_PER_LINE);
static const int L2CACHE_SIZE_BYTES = (L2CACHE_WAYS * (L2CACHE_LINES_PER_WAY * L2CACHE_BYTES_PER_LINE));

static const int L2TAG_FL_DIRTY = 1;
static const int L2TAG_FL_TOTAL = 2;

static const int L2_REQ_TYPE_WRITE = 0;
static const int L2_REQ_TYPE_CACHED = 1;
static const int L2_REQ_TYPE_UNIQUE = 2;
static const int L2_REQ_TYPE_SNOOP = 3;                     // Use data received through snoop channel (no memory request)
static const int L2_REQ_TYPE_BITS = 4;

// MPU config
static const int CFG_MPU_TBL_WIDTH = 3;                     // [1:0]  log2(MPU_TBL_SIZE)
static const int CFG_MPU_TBL_SIZE = (1 << CFG_MPU_TBL_WIDTH);

static const int CFG_MPU_FL_WR = 0;
static const int CFG_MPU_FL_RD = 1;
static const int CFG_MPU_FL_EXEC = 2;
static const int CFG_MPU_FL_CACHABLE = 3;
static const int CFG_MPU_FL_ENA = 4;
static const int CFG_MPU_FL_TOTAL = 5;

// MMU config. Fetch and Data pathes have its own MMU block
static const int CFG_MMU_TLB_AWIDTH = 9;                    // TLB memory address bus width
static const int CFG_MMU_TLB_SIZE = (1 << CFG_MMU_TLB_AWIDTH);// Number of PTE entries in a table
static const int CFG_MMU_PTE_DWIDTH = ((2 * RISCV_ARCH) - 12);// PTE entry size in bits
static const int CFG_MMU_PTE_DBYTES = (CFG_MMU_PTE_DWIDTH / 8);// PTE entry size in bytes


enum EnumMemopSize {
    MEMOP_1B = 0,
    MEMOP_2B = 1,
    MEMOP_4B = 2,
    MEMOP_8B = 3
};

// Integer Registers specified by ISA
static const int REG_ZERO = 0;
static const int REG_RA = 1;                                // [1] Return address
static const int REG_SP = 2;                                // [2] Stack pointer
static const int REG_GP = 3;                                // [3] Global pointer
static const int REG_TP = 4;                                // [4] Thread pointer
static const int REG_T0 = 5;                                // [5] Temporaries 0 s3
static const int REG_T1 = 6;                                // [6] Temporaries 1 s4
static const int REG_T2 = 7;                                // [7] Temporaries 2 s5
static const int REG_S0 = 8;                                // [8] s0/fp Saved register/frame pointer
static const int REG_S1 = 9;                                // [9] Saved register 1
static const int REG_A0 = 10;                               // [10] Function argumentes 0
static const int REG_A1 = 11;                               // [11] Function argumentes 1
static const int REG_A2 = 12;                               // [12] Function argumentes 2
static const int REG_A3 = 13;                               // [13] Function argumentes 3
static const int REG_A4 = 14;                               // [14] Function argumentes 4
static const int REG_A5 = 15;                               // [15] Function argumentes 5
static const int REG_A6 = 16;                               // [16] Function argumentes 6
static const int REG_A7 = 17;                               // [17] Function argumentes 7
static const int REG_S2 = 18;                               // [18] Saved register 2
static const int REG_S3 = 19;                               // [19] Saved register 3
static const int REG_S4 = 20;                               // [20] Saved register 4
static const int REG_S5 = 21;                               // [21] Saved register 5
static const int REG_S6 = 22;                               // [22] Saved register 6
static const int REG_S7 = 23;                               // [23] Saved register 7
static const int REG_S8 = 24;                               // [24] Saved register 8
static const int REG_S9 = 25;                               // [25] Saved register 9
static const int REG_S10 = 26;                              // [26] Saved register 10
static const int REG_S11 = 27;                              // [27] Saved register 11
static const int REG_T3 = 28;                               // [28]
static const int REG_T4 = 29;                               // [29]
static const int REG_T5 = 30;                               // [30]
static const int REG_T6 = 31;                               // [31]

static const int REG_F0 = 0;                                // ft0 temporary register
static const int REG_F1 = 1;                                // ft1
static const int REG_F2 = 2;                                // ft2
static const int REG_F3 = 3;                                // ft3
static const int REG_F4 = 4;                                // ft4
static const int REG_F5 = 5;                                // ft5
static const int REG_F6 = 6;                                // ft6
static const int REG_F7 = 7;                                // ft7
static const int REG_F8 = 8;                                // fs0 saved register
static const int REG_F9 = 9;                                // fs1
static const int REG_F10 = 10;                              // fa0 argument/return value
static const int REG_F11 = 11;                              // fa1 argument/return value
static const int REG_F12 = 12;                              // fa2 argument register
static const int REG_F13 = 13;                              // fa3
static const int REG_F14 = 14;                              // fa4
static const int REG_F15 = 15;                              // fa5
static const int REG_F16 = 16;                              // fa6
static const int REG_F17 = 17;                              // fa7
static const int REG_F18 = 18;                              // fs2 saved register
static const int REG_F19 = 19;                              // fs3
static const int REG_F20 = 20;                              // fs4
static const int REG_F21 = 21;                              // fs5
static const int REG_F22 = 22;                              // fs6
static const int REG_F23 = 23;                              // fs7
static const int REG_F24 = 24;                              // fs8
static const int REG_F25 = 25;                              // fs9
static const int REG_F26 = 26;                              // fs10
static const int REG_F27 = 27;                              // fs11
static const int REG_F28 = 28;                              // ft8 temporary register
static const int REG_F29 = 29;                              // ft9
static const int REG_F30 = 30;                              // ft10
static const int REG_F31 = 31;                              // ft11

static const int INTREGS_TOTAL = 32;
static const int FPUREGS_OFFSET = INTREGS_TOTAL;
static const int FPUREGS_TOTAL = 32;

static const int REGS_BUS_WIDTH = 6;
static const int REGS_TOTAL = (1 << REGS_BUS_WIDTH);        // INTREGS_TOTAL + FPUREGS_TOTAL

// CSR[11:10] indicate whether the register is read/write (00, 01, or 10) or read-only (11)
// CSR[9:8] encode the lowest privilege level that can access the CSR
// @{
// FPU Accrued Exceptions fields from FCSR
static const uint16_t CSR_fflags = 0x001;
// FPU dynamic Rounding Mode fields from FCSR
static const uint16_t CSR_frm = 0x002;
// FPU Control and Status register (frm + fflags)
static const uint16_t CSR_fcsr = 0x003;
// machine mode status read/write register.
static const uint16_t CSR_mstatus = 0x300;
// ISA and extensions supported.
static const uint16_t CSR_misa = 0x301;
// Machine exception delegation
static const uint16_t CSR_medeleg = 0x302;
// Machine interrupt delegation
static const uint16_t CSR_mideleg = 0x303;
// Machine interrupt enable
static const uint16_t CSR_mie = 0x304;
// The base address of the M-mode trap vector.
static const uint16_t CSR_mtvec = 0x305;
// Scratch register for machine trap handlers.
static const uint16_t CSR_mscratch = 0x340;
// Exception program counters.
static const uint16_t CSR_uepc = 0x041;
static const uint16_t CSR_sepc = 0x141;
// Supervisor Address Translation and Protection
static const uint16_t CSR_satp = 0x180;
static const uint16_t CSR_hepc = 0x241;
static const uint16_t CSR_mepc = 0x341;
// Machine trap cause
static const uint16_t CSR_mcause = 0x342;
// Machine bad address or instruction.
static const uint16_t CSR_mtval = 0x343;
// Machine interrupt pending
static const uint16_t CSR_mip = 0x344;
// Software reset.
static const uint16_t CSR_mreset = 0x782;
// Trigger select
static const uint16_t CSR_tselect = 0x7a0;
// Trigger data1
static const uint16_t CSR_tdata1 = 0x7a1;
// Trigger data2
static const uint16_t CSR_tdata2 = 0x7a2;
// Trigger extra (RV64)
static const uint16_t CSR_textra = 0x7a3;
// Trigger info
static const uint16_t CSR_tinfo = 0x7a4;
// Trigger control
static const uint16_t CSR_tcontrol = 0x7a5;
// Machine context
static const uint16_t CSR_mcontext = 0x7a8;
// Supervisor context
static const uint16_t CSR_scontext = 0x7aa;
// Debug Control and status
static const uint16_t CSR_dcsr = 0x7b0;
// Debug PC
static const uint16_t CSR_dpc = 0x7b1;
// Debug Scratch Register 0
static const uint16_t CSR_dscratch0 = 0x7b2;
// Debug Scratch Register 1
static const uint16_t CSR_dscratch1 = 0x7b3;
// Non-stadnard usermode CSR:
//    Flush specified address in I-cache module without execution of fence.i
static const uint16_t CSR_flushi = 0x800;

// Standard read/write Machine CSRs:
// Machine Cycle counter
static const uint16_t CSR_mcycle = 0xB00;
// Machine Instructions-retired counter
static const uint16_t CSR_minsret = 0xB02;

// Non-standard machine mode CSR
// Stack overflow.
static const uint16_t CSR_mstackovr = 0xBC0;
// Stack underflow (non-standard CSR).
static const uint16_t CSR_mstackund = 0xBC1;
// MPU region address (non-standard CSR).
static const uint16_t CSR_mpu_addr = 0xBC2;
// MPU region mask (non-standard CSR).
static const uint16_t CSR_mpu_mask = 0xBC3;
// MPU region control (non-standard CSR).
static const uint16_t CSR_mpu_ctrl = 0xBC4;

// User Cycle counter for RDCYCLE pseudo-instruction
static const uint16_t CSR_cycle = 0xC00;
// User Timer for RDTIME pseudo-instruction
static const uint16_t CSR_time = 0xC01;
// User Instructions-retired counter for RDINSTRET pseudo-instruction
static const uint16_t CSR_insret = 0xC02;
// 0xC00 to 0xC1F reserved for counters
// Vendor ID.
static const uint16_t CSR_mvendorid = 0xf11;
// Architecture ID.
static const uint16_t CSR_marchid = 0xf12;
// Vendor ID.
static const uint16_t CSR_mimplementationid = 0xf13;
// Thread id (the same as core).
static const uint16_t CSR_mhartid = 0xf14;
// @}

// Dport request types:
static const int DPortReq_Write = 0;
static const int DPortReq_RegAccess = 1;
static const int DPortReq_MemAccess = 2;
static const int DPortReq_MemVirtual = 3;
static const int DPortReq_Progexec = 4;
static const int DPortReq_Total = 5;

// DCSR register halt causes:
static const uint32_t HALT_CAUSE_EBREAK = 1;                // software breakpoint
static const uint32_t HALT_CAUSE_TRIGGER = 2;               // hardware breakpoint
static const uint32_t HALT_CAUSE_HALTREQ = 3;               // halt request via debug interface
static const uint32_t HALT_CAUSE_STEP = 4;                  // step done
static const uint32_t HALT_CAUSE_RESETHALTREQ = 5;          // not implemented

static const uint32_t PROGBUF_ERR_NONE = 0;                 // no error
static const uint32_t PROGBUF_ERR_BUSY = 1;                 // abstract command in progress
static const uint32_t PROGBUF_ERR_NOT_SUPPORTED = 2;        // Request command not supported
static const uint32_t PROGBUF_ERR_EXCEPTION = 3;            // Exception occurs while executing progbuf
static const uint32_t PROGBUF_ERR_HALT_RESUME = 4;          // Command cannot be executed because of wrong CPU state
static const uint32_t PROGBUF_ERR_BUS = 5;                  // Bus error occurs
static const uint32_t PROGBUF_ERR_OTHER = 7;                // Other reason

// @name PRV bits possible values:
// @{
// User-mode
static const sc_uint<2> PRV_U = 0;
// super-visor mode
static const sc_uint<2> PRV_S = 1;
// hyper-visor mode
static const sc_uint<2> PRV_H = 2;
// machine mode
static const sc_uint<2> PRV_M = 3;
// @}

enum EExceptions {
    EXCEPTION_InstrMisalign = 0,                            // Instruction address misaligned
    EXCEPTION_InstrFault = 1,                               // Instruction access fault
    EXCEPTION_InstrIllegal = 2,                             // Illegal instruction
    EXCEPTION_Breakpoint = 3,                               // Breakpoint
    EXCEPTION_LoadMisalign = 4,                             // Load address misaligned
    EXCEPTION_LoadFault = 5,                                // Load access fault
    EXCEPTION_StoreMisalign = 6,                            // Store/AMO address misaligned
    EXCEPTION_StoreFault = 7,                               // Store/AMO access fault
    EXCEPTION_CallFromUmode = 8,                            // Environment call from U-mode
    EXCEPTION_CallFromSmode = 9,                            // Environment call from S-mode
    EXCEPTION_CallFromHmode = 10,                           // Environment call from H-mode
    EXCEPTION_CallFromMmode = 11,                           // Environment call from M-mode
    EXCEPTION_InstrPageFault = 12,                          // Instruction page fault
    EXCEPTION_LoadPageFault = 13,                           // Load page fault
    EXCEPTION_rsrv14 = 14,                                  // reserved
    EXCEPTION_StorePageFault = 15,                          // Store/AMO page fault
    EXCEPTION_StackOverflow = 16,                           // Stack overflow
    EXCEPTION_StackUnderflow = 17,                          // Stack underflow
    EXCEPTIONS_Total = 18
};

enum EInterrupts {
    INTERRUPT_XSoftware = 0,
    INTERRUPT_XTimer = 1,
    INTERRUPT_XExternal = 2,
    INTERRUPT_Total = 3
};

static const int SIGNAL_Exception = 0;
static const int SIGNAL_XSoftware = (EXCEPTIONS_Total + (4 * INTERRUPT_XSoftware));
static const int SIGNAL_XTimer = (EXCEPTIONS_Total + (4 * INTERRUPT_XTimer));
static const int SIGNAL_XExternal = (EXCEPTIONS_Total + (4 * INTERRUPT_XExternal));
static const int SIGNAL_HardReset = (SIGNAL_XExternal + 1);
static const int SIGNAL_Total = (SIGNAL_HardReset + 1);

static const int EXCEPTION_CallFromXMode = EXCEPTION_CallFromUmode;

// Instruction formats specified by ISA specification
enum EIsaType {
    ISA_R_type = 0,
    ISA_I_type = 1,
    ISA_S_type = 2,
    ISA_SB_type = 3,
    ISA_U_type = 4,
    ISA_UJ_type = 5,
    ISA_Total = 6
};

// Implemented instruction list and its indexes
enum EInstructionType {
    Instr_ADD = 0,
    Instr_ADDI = 1,
    Instr_ADDIW = 2,
    Instr_ADDW = 3,
    Instr_AND = 4,
    Instr_ANDI = 5,
    Instr_AUIPC = 6,
    Instr_BEQ = 7,
    Instr_BGE = 8,
    Instr_BGEU = 9,
    Instr_BLT = 10,
    Instr_BLTU = 11,
    Instr_BNE = 12,
    Instr_JAL = 13,
    Instr_JALR = 14,
    Instr_LB = 15,
    Instr_LH = 16,
    Instr_LW = 17,
    Instr_LD = 18,
    Instr_LBU = 19,
    Instr_LHU = 20,
    Instr_LWU = 21,
    Instr_LUI = 22,
    Instr_OR = 23,
    Instr_ORI = 24,
    Instr_SLLI = 25,
    Instr_SLT = 26,
    Instr_SLTI = 27,
    Instr_SLTU = 28,
    Instr_SLTIU = 29,
    Instr_SLL = 30,
    Instr_SLLW = 31,
    Instr_SLLIW = 32,
    Instr_SRA = 33,
    Instr_SRAW = 34,
    Instr_SRAI = 35,
    Instr_SRAIW = 36,
    Instr_SRL = 37,
    Instr_SRLI = 38,
    Instr_SRLIW = 39,
    Instr_SRLW = 40,
    Instr_SB = 41,
    Instr_SH = 42,
    Instr_SW = 43,
    Instr_SD = 44,
    Instr_SUB = 45,
    Instr_SUBW = 46,
    Instr_XOR = 47,
    Instr_XORI = 48,
    Instr_CSRRW = 49,
    Instr_CSRRS = 50,
    Instr_CSRRC = 51,
    Instr_CSRRWI = 52,
    Instr_CSRRCI = 53,
    Instr_CSRRSI = 54,
    Instr_URET = 55,
    Instr_SRET = 56,
    Instr_HRET = 57,
    Instr_MRET = 58,
    Instr_FENCE = 59,
    Instr_FENCE_I = 60,
    Instr_WFI = 61,
    Instr_DIV = 62,
    Instr_DIVU = 63,
    Instr_DIVW = 64,
    Instr_DIVUW = 65,
    Instr_MUL = 66,
    Instr_MULW = 67,
    Instr_MULH = 68,
    Instr_MULHSU = 69,
    Instr_MULHU = 70,
    Instr_REM = 71,
    Instr_REMU = 72,
    Instr_REMW = 73,
    Instr_REMUW = 74,
    Instr_AMOADD_W = 75,
    Instr_AMOXOR_W = 76,
    Instr_AMOOR_W = 77,
    Instr_AMOAND_W = 78,
    Instr_AMOMIN_W = 79,
    Instr_AMOMAX_W = 80,
    Instr_AMOMINU_W = 81,
    Instr_AMOMAXU_W = 82,
    Instr_AMOSWAP_W = 83,
    Instr_LR_W = 84,
    Instr_SC_W = 85,
    Instr_AMOADD_D = 86,
    Instr_AMOXOR_D = 87,
    Instr_AMOOR_D = 88,
    Instr_AMOAND_D = 89,
    Instr_AMOMIN_D = 90,
    Instr_AMOMAX_D = 91,
    Instr_AMOMINU_D = 92,
    Instr_AMOMAXU_D = 93,
    Instr_AMOSWAP_D = 94,
    Instr_LR_D = 95,
    Instr_SC_D = 96,
    Instr_ECALL = 97,
    Instr_EBREAK = 98,
    Instr_FADD_D = 99,
    Instr_FCVT_D_W = 100,
    Instr_FCVT_D_WU = 101,
    Instr_FCVT_D_L = 102,
    Instr_FCVT_D_LU = 103,
    Instr_FCVT_W_D = 104,
    Instr_FCVT_WU_D = 105,
    Instr_FCVT_L_D = 106,
    Instr_FCVT_LU_D = 107,
    Instr_FDIV_D = 108,
    Instr_FEQ_D = 109,
    Instr_FLD = 110,
    Instr_FLE_D = 111,
    Instr_FLT_D = 112,
    Instr_FMAX_D = 113,
    Instr_FMIN_D = 114,
    Instr_FMOV_D_X = 115,
    Instr_FMOV_X_D = 116,
    Instr_FMUL_D = 117,
    Instr_FSD = 118,
    Instr_FSUB_D = 119,
    Instr_Total = 120
};

static const int Instr_FPU_Total = ((Instr_FSUB_D - Instr_FADD_D) + 1);

// Depth of the fifo between Executor and MemoryAccess modules.
static const int CFG_MEMACCESS_QUEUE_DEPTH = 2;
// Register's tag used to detect reg hazard and it should be higher than available
// slots in the fifo Executor => Memaccess.
static const int CFG_REG_TAG_WIDTH = 3;

// Request type: [0]-read csr; [1]-write csr; [2]-change mode
static const int CsrReq_ReadBit = 0;
static const int CsrReq_WriteBit = 1;
static const int CsrReq_TrapReturnBit = 2;
static const int CsrReq_ExceptionBit = 3;                   // return instruction pointer
static const int CsrReq_InterruptBit = 4;                   // return instruction pointer
static const int CsrReq_BreakpointBit = 5;
static const int CsrReq_HaltBit = 6;
static const int CsrReq_ResumeBit = 7;
static const int CsrReq_WfiBit = 8;                         // wait for interrupt
static const int CsrReq_TotalBits = 9;

static const sc_uint<CsrReq_TotalBits> CsrReq_ReadCmd = (1 << CsrReq_ReadBit);
static const sc_uint<CsrReq_TotalBits> CsrReq_WriteCmd = (1 << CsrReq_WriteBit);
static const sc_uint<CsrReq_TotalBits> CsrReq_TrapReturnCmd = (1 << CsrReq_TrapReturnBit);
static const sc_uint<CsrReq_TotalBits> CsrReq_ExceptionCmd = (1 << CsrReq_ExceptionBit);
static const sc_uint<CsrReq_TotalBits> CsrReq_InterruptCmd = (1 << CsrReq_InterruptBit);
static const sc_uint<CsrReq_TotalBits> CsrReq_BreakpointCmd = (1 << CsrReq_BreakpointBit);
static const sc_uint<CsrReq_TotalBits> CsrReq_HaltCmd = (1 << CsrReq_HaltBit);
static const sc_uint<CsrReq_TotalBits> CsrReq_ResumeCmd = (1 << CsrReq_ResumeBit);
static const sc_uint<CsrReq_TotalBits> CsrReq_WfiCmd = (1 << CsrReq_WfiBit);

static const int MemopType_Store = 0;                       // 0=load; 1=store
static const int MemopType_Locked = 1;                      // AMO instructions
static const int MemopType_Reserve = 2;                     // LS load with reserve
static const int MemopType_Release = 3;                     // SC store with release
static const int MemopType_Total = 4;

}  // namespace debugger

