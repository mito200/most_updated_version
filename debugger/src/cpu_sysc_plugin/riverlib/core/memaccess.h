/*
 *  Copyright 2019 Sergey Khabarov, sergeykhbr@gmail.com
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef __DEBUGGER_RIVERLIB_MEMSTAGE_H__
#define __DEBUGGER_RIVERLIB_MEMSTAGE_H__

#include <systemc.h>
#include "../river_cfg.h"
#include "queue.h"

namespace debugger {

SC_MODULE(MemAccess) {
    sc_in<bool> i_clk;
    sc_in<bool> i_nrst;
    sc_in<sc_uint<CFG_CPU_ADDR_BITS>> i_e_pc;       // Execution stage instruction pointer
    sc_in<sc_uint<32>> i_e_instr;                   // Execution stage instruction value
    sc_in<bool> i_e_flushd;
    sc_out<bool> o_flushd;

    sc_in<sc_uint<6>> i_reg_waddr;                  // Register address to be written (0=no writing)
    sc_in<sc_uint<2>> i_reg_wtag;                   // Register tag for writeback operation
    sc_in<bool> i_memop_valid;                      // Memory request is valid
    sc_in<sc_uint<RISCV_ARCH>> i_memop_wdata;       // Register value to be written
    sc_in<bool> i_memop_sign_ext;                   // Load data with sign extending (if less than 8 Bytes)
    sc_in<bool> i_memop_type;                       // 1=store;0=Load data from memory and write to i_res_addr
    sc_in<sc_uint<2>> i_memop_size;                 // Encoded memory transaction size in bytes: 0=1B; 1=2B; 2=4B; 3=8B
    sc_in<sc_uint<CFG_CPU_ADDR_BITS>> i_memop_addr;    // Memory access address
    sc_out<bool> o_memop_ready;                     // Ready to accept memop request

    sc_out<bool> o_wb_wena;                         // Write enable signal
    sc_out<sc_uint<6>> o_wb_waddr;                  // Output register address (0 = x0 = no write)
    sc_out<sc_uint<RISCV_ARCH>> o_wb_wdata;         // Register value
    sc_out<sc_uint<2>> o_wb_wtag;
    sc_in<bool> i_wb_ready;

    // Memory interface:
    sc_in<bool> i_mem_req_ready;                    // Data cache is ready to accept read/write request
    sc_out<bool> o_mem_valid;                       // Memory request is valid
    sc_out<bool> o_mem_write;                       // Memory write request
    sc_out<sc_uint<CFG_CPU_ADDR_BITS>> o_mem_addr;     // Data path requested address
    sc_out<sc_uint<64>> o_mem_wdata;                // Data path requested data (write transaction)
    sc_out<sc_uint<8>> o_mem_wstrb;                 // 8-bytes aligned strobs
    sc_out<sc_uint<2>> o_mem_size;                  // 1,2,4 or 8-bytes operation for uncached access
    sc_in<bool> i_mem_data_valid;                   // Data path memory response is valid
    sc_in<sc_uint<CFG_CPU_ADDR_BITS>> i_mem_data_addr; // Data path memory response address
    sc_in<sc_uint<64>> i_mem_data;                  // Data path memory response value
    sc_out<bool> o_mem_resp_ready;                  // Pipeline is ready to accept memory operation response

    void comb();
    void registers();

    SC_HAS_PROCESS(MemAccess);

    MemAccess(sc_module_name name_, bool async_reset);
    virtual ~MemAccess();

    void generateVCD(sc_trace_file *i_vcd, sc_trace_file *o_vcd);

private:
    static const unsigned State_Idle = 0;
    static const unsigned State_WaitReqAccept = 1;
    static const unsigned State_WaitResponse = 2;
    static const unsigned State_Hold = 3;

    struct RegistersType {
        sc_signal<sc_uint<2>> state;
        sc_signal<bool> memop_w;
        sc_signal<sc_uint<CFG_CPU_ADDR_BITS>> memop_addr;
        sc_signal<sc_uint<64>> memop_wdata;
        sc_signal<sc_uint<8>> memop_wstrb;
        sc_signal<bool> memop_sign_ext;
        sc_signal<sc_uint<2>> memop_size;

        sc_signal<sc_uint<CFG_CPU_ADDR_BITS>> memop_res_pc;
        sc_signal<sc_uint<32>> memop_res_instr;
        sc_signal<sc_uint<6>> memop_res_addr;
        sc_signal<sc_uint<2>> memop_res_wtag;
        sc_signal<sc_uint<RISCV_ARCH>> memop_res_data;
        sc_signal<bool> memop_res_wena;

        sc_signal<sc_uint<RISCV_ARCH>> hold_rdata;
    } v, r;

    void R_RESET(RegistersType &iv) {
        iv.state = State_Idle;
        iv.memop_w = 0;
        iv.memop_addr = 0;
        iv.memop_wdata = 0;
        iv.memop_wstrb = 0;
        iv.memop_sign_ext = 0;
        iv.memop_size = 0;
        iv.memop_res_pc = 0;
        iv.memop_res_instr = 0;
        iv.memop_res_addr = 0;
        iv.memop_res_wtag = 0;
        iv.memop_res_data = 0;
        iv.memop_res_wena = 0;
        iv.hold_rdata = 0;
    }

    static const int QUEUE_WIDTH = 1   // i_e_flushd
                                 + 2   // wtag
                                 + 64  // wdata width
                                 + 8   // wstrb 
                                 + RISCV_ARCH
                                 + 6
                                 + 32
                                 + CFG_CPU_ADDR_BITS
                                 + 2
                                 + 1
                                 + 1
                                 + CFG_CPU_ADDR_BITS;

    Queue<2, QUEUE_WIDTH> *queue0;

    sc_signal<bool> queue_we;
    sc_signal<bool> queue_re;
    sc_signal<sc_biguint<QUEUE_WIDTH>> queue_data_i;
    sc_signal<sc_biguint<QUEUE_WIDTH>> queue_data_o;
    sc_signal<bool> queue_nempty;
    sc_signal<bool> queue_full;

    bool async_reset_;
};

}  // namespace debugger

#endif  // __DEBUGGER_RIVERLIB_EXECUTE_H__
