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
 *
 * Implemented algorithm provides 4 clocks per instruction
 */

#pragma once

#include <systemc.h>
#include "../../river_cfg.h"

namespace debugger {

SC_MODULE(AluLogic) {
    sc_in<bool> i_clk;
    sc_in<bool> i_nrst;
    sc_in<sc_uint<3>> i_mode;               // operation type: [0]AND;[1]=OR;[2]XOR
    sc_in<sc_uint<RISCV_ARCH>> i_a1;        // Operand 1
    sc_in<sc_uint<RISCV_ARCH>> i_a2;        // Operand 2
    sc_out<sc_uint<RISCV_ARCH>> o_res;      // Result

    void comb();
    void registers();

    SC_HAS_PROCESS(AluLogic);

    AluLogic(sc_module_name name_, bool async_reset);

    void generateVCD(sc_trace_file *i_vcd, sc_trace_file *o_vcd);

private:

    struct RegistersType {
        sc_signal<sc_uint<RISCV_ARCH>> res;
    } v, r;

    void R_RESET(RegistersType &iv) {
        iv.res = 0;
    }

    bool async_reset_;
};


}  // namespace debugger
