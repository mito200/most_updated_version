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
 * @brief      CSR bus inerconnect 2 masters 1 slave.
 */

#include "ic_csr_m2_s1.h"

namespace debugger {

ic_csr_m2_s1::ic_csr_m2_s1(sc_module_name name_, bool async_reset) : sc_module(name_),
    i_clk("i_clk"),
    i_nrst("i_nrst"),
    // master[0]
    i_m0_req_valid("i_m0_req_valid"),
    o_m0_req_ready("o_m0_req_ready"),
    i_m0_req_type("i_m0_req_type"),
    i_m0_req_addr("i_m0_req_addr"),
    i_m0_req_data("i_m0_req_data"),
    o_m0_resp_valid("o_m0_resp_valid"),
    i_m0_resp_ready("i_m0_resp_ready"),
    o_m0_resp_data("o_m0_resp_data"),
    o_m0_resp_exception("o_m0_resp_exception"),
    // master[1]
    i_m1_req_valid("i_m1_req_valid"),
    o_m1_req_ready("o_m1_req_ready"),
    i_m1_req_type("i_m1_req_type"),
    i_m1_req_addr("i_m1_req_addr"),
    i_m1_req_data("i_m1_req_data"),
    o_m1_resp_valid("o_m1_resp_valid"),
    i_m1_resp_ready("i_m1_resp_ready"),
    o_m1_resp_data("o_m1_resp_data"),
    o_m1_resp_exception("o_m1_resp_exception"),
    // slave[0]
    o_s0_req_valid("o_s0_req_valid"),
    i_s0_req_ready("i_s0_req_ready"),
    o_s0_req_type("o_s0_req_type"),
    o_s0_req_addr("o_s0_req_addr"),
    o_s0_req_data("o_s0_req_data"),
    i_s0_resp_valid("i_s0_resp_valid"),
    o_s0_resp_ready("o_s0_resp_ready"),
    i_s0_resp_data("i_s0_resp_data"),
    i_s0_resp_exception("i_s0_resp_exception") {
    async_reset_ = async_reset;

    SC_METHOD(comb);
    sensitive << i_m0_req_valid;
    sensitive << i_m0_req_type;
    sensitive << i_m0_req_addr;
    sensitive << i_m0_req_data;
    sensitive << i_m0_resp_ready;
    sensitive << i_m1_req_valid;
    sensitive << i_m1_req_type;
    sensitive << i_m1_req_addr;
    sensitive << i_m1_req_data;
    sensitive << i_m1_resp_ready;
    sensitive << i_s0_req_ready;
    sensitive << i_s0_resp_valid;
    sensitive << i_s0_resp_data;
    sensitive << i_s0_resp_exception;
    sensitive << r.midx;
    sensitive << r.acquired;

    SC_METHOD(registers);
    sensitive << i_nrst;
    sensitive << i_clk.pos();
};

void ic_csr_m2_s1::generateVCD(sc_trace_file *i_vcd, sc_trace_file *o_vcd) {
    if (o_vcd) {
        sc_trace(o_vcd, i_m0_req_valid, i_m0_req_valid.name());
        sc_trace(o_vcd, o_m0_req_ready, o_m0_req_ready.name());
        sc_trace(o_vcd, i_m0_req_type, i_m0_req_type.name());
        sc_trace(o_vcd, i_m0_req_addr, i_m0_req_addr.name());
        sc_trace(o_vcd, i_m0_req_data, i_m0_req_data.name());
        sc_trace(o_vcd, o_m0_resp_valid, o_m0_resp_valid.name());
        sc_trace(o_vcd, i_m0_resp_ready, i_m0_resp_ready.name());
        sc_trace(o_vcd, o_m0_resp_data, o_m0_resp_data.name());
        sc_trace(o_vcd, i_m1_req_valid, i_m1_req_valid.name());
        sc_trace(o_vcd, o_m1_req_ready, o_m1_req_ready.name());
        sc_trace(o_vcd, i_m1_req_type, i_m1_req_type.name());
        sc_trace(o_vcd, i_m1_req_addr, i_m1_req_addr.name());
        sc_trace(o_vcd, i_m1_req_data, i_m1_req_data.name());
        sc_trace(o_vcd, o_m1_resp_valid, o_m1_resp_valid.name());
        sc_trace(o_vcd, i_m1_resp_ready, i_m1_resp_ready.name());
        sc_trace(o_vcd, o_m1_resp_data, o_m1_resp_data.name());
        sc_trace(o_vcd, o_s0_req_valid, o_s0_req_valid.name());
        sc_trace(o_vcd, i_s0_req_ready, i_s0_req_ready.name());
        sc_trace(o_vcd, o_s0_req_type, o_s0_req_type.name());
        sc_trace(o_vcd, o_s0_req_addr, o_s0_req_addr.name());
        sc_trace(o_vcd, o_s0_req_data, o_s0_req_data.name());
        sc_trace(o_vcd, i_s0_resp_valid, i_s0_resp_valid.name());
        sc_trace(o_vcd, o_s0_resp_ready, o_s0_resp_ready.name());
        sc_trace(o_vcd, i_s0_resp_data, i_s0_resp_data.name());
        sc_trace(o_vcd, i_s0_resp_exception, i_s0_resp_exception.name());

        std::string pn(name());
        sc_trace(o_vcd, r.midx, pn + ".r_midx");
    }
}

void ic_csr_m2_s1::comb() {
    v = r;

    if (!r.acquired && (i_m0_req_valid || i_m1_req_valid)) {
        v.acquired = 1;
        if (i_m0_req_valid) {
            v.midx = 0;
        } else {
            v.midx = 1;
        }
    }
    if ((r.midx.read() == 0 && i_s0_resp_valid && i_m0_resp_ready)
     || (r.midx.read() == 1 && i_s0_resp_valid && i_m1_resp_ready)) {
        v.acquired = 0;
    }
    
    if (r.midx.read() == 0 || (!r.acquired.read() && i_m0_req_valid.read())) {
        o_s0_req_valid = i_m0_req_valid;
        o_m0_req_ready = i_s0_req_ready;
        o_s0_req_type = i_m0_req_type;
        o_s0_req_addr = i_m0_req_addr;
        o_s0_req_data = i_m0_req_data;
        o_m0_resp_valid = i_s0_resp_valid;
        o_s0_resp_ready = i_m0_resp_ready;
        o_m0_resp_data = i_s0_resp_data;
        o_m0_resp_exception = i_s0_resp_exception;
        o_m1_req_ready = 0;
        o_m1_resp_valid = 0;
        o_m1_resp_data = 0;
        o_m1_resp_exception = 0;
    } else {
        o_s0_req_valid = i_m1_req_valid;
        o_m1_req_ready = i_s0_req_ready;
        o_s0_req_type = i_m1_req_type;
        o_s0_req_addr = i_m1_req_addr;
        o_s0_req_data = i_m1_req_data;
        o_m1_resp_valid = i_s0_resp_valid;
        o_s0_resp_ready = i_m1_resp_ready;
        o_m1_resp_data = i_s0_resp_data;
        o_m1_resp_exception = i_s0_resp_exception;
        o_m0_req_ready = 0;
        o_m0_resp_valid = 0;
        o_m0_resp_data = 0;
        o_m0_resp_exception = 0;
    }


    if (!async_reset_ && !i_nrst.read()) {
        R_RESET(v);
    }
}

void ic_csr_m2_s1::registers() {
    if (async_reset_ && i_nrst.read() == 0) {
        R_RESET(r);
    } else {
        r = v;
    }
}

}  // namespace debugger

