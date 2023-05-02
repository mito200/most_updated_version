/*
 *  Copyright 2023 Sergey Khabarov, sergeykhbr@gmail.com
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

#include <api_types.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "openocdwrap.h"

namespace debugger {

OcdCmdResume::OcdCmdResume(OpenOcdWrapper *parent, IJtag *ijtag)
    : ICommand(parent, "test") {

    briefDescr_.make_string("Run simulation for a specify number of steps"
                            "or a specific addres\n");
    detailedDescr_.make_string(
        "Description:\n"
        "    Run simulation for a specified number of steps.\n"
        "Usage:\n"
        "    c <N steps>\n"
        "Example:\n"
        "    c\n"
        "    c <addr>\n"
        "    c 1\n");
}


int OcdCmdResume::isValid(AttributeType *args) {
    AttributeType &name = (*args)[0u];
    if (!cmdName_.is_equal(name.to_string())
        && !name.is_equal("c")) {
        return CMD_INVALID;
    }
    if (args->size() == 1 || args->size() == 2) {
        return CMD_VALID;
    }
    return CMD_WRONG_ARGS;
}

void OcdCmdResume::exec(AttributeType *args, AttributeType *res) {
    OpenOcdWrapper *p = static_cast<OpenOcdWrapper *>(cmdParent_);
    res->attr_free();
    res->make_nil();

    p->setCommandInProgress(this);

    if (args->size() == 1) {
        p->resume();
    } else {
    }

    p->setCommandInProgress(0);
}

OpenOcdWrapper::OpenOcdWrapper(const char *name) 
    : TcpClient(0, name) {
    registerAttribute("Jtag", &jtag_);
    registerAttribute("CmdExecutor", &cmdexec_);
    registerAttribute("PollingMs", &pollingMs_);
    registerAttribute("OpenOcdPath", &openOcdPath_);
    registerAttribute("OpenOcdScript", &openOcdScript_);
    registerAttribute("GdbMode", &gdbMode_);
    openocd_ = 0;
    emsgstate_ = MsgIdle;
    msgcnt_ = 0;
    connectionDone_ = false;                // openocd should response '+' on connection
    gdbReq_ = GdbCommand_QStartNoAckMode();
    pcmdInProgress_ = 0;
    bbstate_ = IJtag::IDLE;

    RISCV_event_create(&eventJtagScanEnd_, "openocdwrap_jtagscan");
}

OpenOcdWrapper::~OpenOcdWrapper() {
    RISCV_event_close(&eventJtagScanEnd_);
    RISCV_event_close(&config_done_);
    if (openocd_) {
        delete openocd_;
    }
}

void OpenOcdWrapper::postinitService() {
    ijtag_ = static_cast<IJtag *>
            (RISCV_get_service_iface(jtag_.to_string(), IFACE_JTAG));

    icmdexec_ = static_cast<ICmdExecutor *>(
       RISCV_get_service_iface(cmdexec_.to_string(), IFACE_CMD_EXECUTOR));
    if (!icmdexec_) {
        RISCV_error("ICmdExecutor interface '%s' not found", 
                    cmdexec_.to_string());
    } else {
        pcmdResume_ = new OcdCmdResume(this, 0);
        icmdexec_->registerCommand(pcmdResume_);
    }

    // Run openocd as an external process using execv
    if (gdbMode_.to_bool()) {
        char tstr[256];
        RISCV_sprintf(tstr, sizeof(tstr), "%s.ext", getObjName());
        openocd_ = new ExternalProcessThread(this,
                                             tstr,
                                             openOcdPath_.to_string(),
                                             openOcdScript_.to_string());
        openocd_->run();
    }
    if (!run()) {
        RISCV_error("Can't create thread.", NULL);
        return;
    }
}

void OpenOcdWrapper::predeleteService() {
    if (icmdexec_) {
        icmdexec_->unregisterCommand(pcmdResume_);
        delete pcmdResume_;
    }
}

void OpenOcdWrapper::afterThreadStarted() {
    if (openocd_ && openocd_->isEnabled()) {
        openocd_->waitToStart();
        RISCV_sleep_ms(1000);

        // trying to connect to external openocd:4444 or 3333
        while (openocd_->isEnabled() && connectToServer() != 0) {
            RISCV_sleep_ms(1000);
        }
    } else {
        targetPort_.make_int64(9824);       // connect ot BitBang server

        while (connectToServer() != 0) {
            RISCV_sleep_ms(1000);
        }
    }
}


void OpenOcdWrapper::ExternalProcessThread::busyLoop() {
    char tstr[4096];
    RISCV_sprintf(tstr, sizeof(tstr), "%s/openocd -f %s",
                path_.to_string(),
                script_.to_string());

    RISCV_event_set(&eventLoopStarted_);
    retcode_ = RISCV_system(tstr);
    RISCV_info("External OpenOCD was closed with code %d", retcode_);
    stop();
}

int OpenOcdWrapper::processRxBuffer(const char *buf, int sz) {
    RISCV_debug("%s", buf);

    char pins;
    char tms;
    char tdo;
    uint8_t tdi;
    bool transmit = true;

    for (int i = 0; i < sz; i++) {
        tdi = 0;
        if (buf[i] == '1') {
            tdi = 1;
        } else if (buf[i] != '0') {
            bool st = true;
        }

        switch (bbstate_) {
        case IJtag::RESET:
            scanreq_.trst = 0;
            transmit = false;
            writeTxBuffer("r0R4", 4);  // srst=0 and trst=0, posedge, response
            bbstate_ = IJtag::IDLE;
            break;
        case IJtag::IDLE:
            tms = 0;
            tdo = 1;
            if (scanreq_.valid) {
                scanreq_.valid = false;
                if (scanreq_.trst) {
                    writeTxBuffer("uR", 2);  // srst=1 and trst=1, response
                    bbstate_ = IJtag::RESET;
                    transmit = false;
                } else {
                    tms = 1;
                    tdo = 1;
                    bbstate_ = IJtag::DRSCAN;
                }
            } else {
                transmit = false;
                RISCV_event_set(&eventJtagScanEnd_);
            }
            break;
        case IJtag::DRSCAN:
            if (ir_ != scanreq_.ir) {
                tms = 1;
                tdo = 1;
                bbstate_ = IJtag::IRSCAN;
            } else {
                tms = 0;
                tdo = 1;
                bbstate_ = IJtag::DRCAPTURE;
            }
            break;
        case IJtag::IRSCAN:
            tms = 0;
            tdo = 1;
            bbstate_ = IJtag::IRCAPTURE;
            break;
        case IJtag::IRCAPTURE:
            tms = 0;
            tdo = 1;
            ir_ = scanreq_.ir;
            ircnt_ = 0;
            bbstate_ = IJtag::IRSHIFT;
            break;
        case IJtag::IRSHIFT:
            tms = 0;
            tdo = (ir_ >> ircnt_) & 0x1;
            if (++ircnt_ == IRLEN) {
                tms = 1;
                bbstate_ = IJtag::IREXIT1;
            }
            break;
        case IJtag::IREXIT1:
            tms = 1;
            tdo = 1;
            bbstate_ = IJtag::IRUPDATE;
            break;
        case IJtag::IRUPDATE:
            tms = 1;
            tdo = 1;
            bbstate_ = IJtag::DRSCAN;
            break;

        case IJtag::DRCAPTURE:
            tms = 0;
            tdo = 1;
            dr_ = scanreq_.dr;
            drcnt_ = 0;
            bbstate_ = IJtag::DRSHIFT;
            break;
        case IJtag::DRSHIFT:
            tms = 0;
            tdo = dr_ & 0x1;
            dr_ >>= 1;
            if (ir_ == IJtag::IR_DMI) {
                dr_ |= static_cast<uint64_t>(tdi) << (ABITS + 33);
            } else {
                dr_ |= static_cast<uint64_t>(tdi) << 31;
            }

            if (++drcnt_ == scanreq_.drlen) {
                tms = 1;
                bbstate_ = IJtag::DREXIT1;
            }
            break;
        case IJtag::DREXIT1:
            tms = 1;
            tdo = 1;
            dr_ >>= 1;
            if (ir_ == IJtag::IR_DMI) {
                dr_ |= static_cast<uint64_t>(tdi) << (ABITS + 33);
            } else {
                dr_ |= static_cast<uint64_t>(tdi) << 31;
            }
            bbstate_ = IJtag::DRUPDATE;
            break;
        case IJtag::DRUPDATE:
            tms = 0;
            tdo = 1;
            bbstate_ = IJtag::IDLE;
            break;
        default:
            tms = 0;
            tdo = 1;
            bbstate_ = IJtag::IDLE;
        }
    }

    if (transmit) {
        char tclk = '0' + ((tms << 1) | tdo);
        writeTxBuffer(&tclk, 1);
        writeTxBuffer("R", 1);              // response
        tclk = '4' + ((tms << 1) | tdo);    // tck posedge
        writeTxBuffer(&tclk, 1);
    }
    return sz;

    if (pcmdInProgress_) {
        //pcmdInProgress_->processRxBuffer(buf, sz);
    }

    for (int i = 0; i < sz; i++) {
        if (!connectionDone_) {
            if (buf[i] == '+') {
                connectionDone_ = true;
            }
            continue;
        }

        msgbuf_[msgcnt_++] = buf[i];
        msgbuf_[msgcnt_] = '\0';

        switch (emsgstate_) {
        case MsgData:
            if (buf[i] == '#') {
                emsgstate_ = MsgCrcHigh;
            }
            break;
        case MsgCrcHigh:
            emsgstate_ = MsgCrcLow;
            break;
        case MsgCrcLow:
            gdbResp_ = GdbCommandGeneric(msgbuf_, msgcnt_);
            gdbReq_.handleResponse(&gdbResp_);
            emsgstate_ = MsgIdle;
            msgcnt_ = 0;
            break;
        default:
            if (buf[i] == '+') {
                msgcnt_ = 0;
                gdbReq_.setAck();
            } else if (buf[i] == '-') {
                msgcnt_ = 0;
                gdbReq_.setRequest();
            } else if (buf[i] == '$') {
                emsgstate_ = MsgData;
            } else {
                msgcnt_ = 0;
            }
        }
    }
    return 0;
}

uint64_t OpenOcdWrapper::scanReset() {
    scanreq_.ir = 0;
    scanreq_.dr = 0;
    scanreq_.drlen = 0;
    scanreq_.trst = true;
    scanreq_.valid = true;

    writeTxBuffer("R", 1);  // get TDI

    RISCV_event_clear(&eventJtagScanEnd_);
    RISCV_event_wait(&eventJtagScanEnd_);
    return 0;
}

uint64_t OpenOcdWrapper::scan(uint32_t ir, uint64_t dr, int drlen) {
    scanreq_.ir = ir;
    scanreq_.dr = dr;
    scanreq_.drlen = drlen;
    scanreq_.valid = true;

    writeTxBuffer("R", 1);  // get TDI

    RISCV_event_clear(&eventJtagScanEnd_);
    RISCV_event_wait(&eventJtagScanEnd_);
    return 0;
}

IJtag::DtmcsType OpenOcdWrapper::scanDtmcs() {
    IJtag::DtmcsType ret = {0};
    scan(IJtag::IR_DTMCS, 0, 32);
    ret.u32 = static_cast<uint32_t>(dr_);
    RISCV_debug("DTMCS = %08x: ver:%d, abits:%d, stat:%d",
            ret.u32, ret.bits.version, ret.bits.abits, ret.bits.dmistat);
    return ret;
}

uint32_t OpenOcdWrapper::scanDmi(uint32_t addr, uint32_t data, IJtag::EDmiOperation op) {
    IJtag::DmiType ret;
    uint64_t dr = addr;
    dr = (dr << 32) | data;
    dr = (dr << 2) | op;
    scan(IJtag::IR_DMI, dr, 34 + ABITS);

    // Do the same but with rena=0 and wena=0
    dr = static_cast<uint64_t>(addr) << 34;
    scan(IJtag::IR_DMI, dr, 34 + ABITS);
    ret.u64 = dr_;

    RISCV_debug("DMI [%02x] %08x, stat:%d",
            static_cast<uint32_t>(ret.bits.addr),
            static_cast<uint32_t>(ret.bits.data),
            static_cast<uint32_t>(ret.bits.status));
    return static_cast<uint32_t>(ret.bits.data);
}


int OpenOcdWrapper::sendData() {
    if (connectionDone_ && gdbReq_.isRequest()) {
        gdbReq_.clearRequest();
        writeTxBuffer(gdbReq_.to_string(),
                      gdbReq_.getStringSize());
    }
    return TcpClient::sendData();
}

void OpenOcdWrapper::resume() {
    if (openocd_ && openocd_->isEnabled()) {
        gdbReq_ = GdbCommand_Continue();
    } else {
        scanReset();
        IJtag::DtmcsType dtmcs;
        dtmcs = scanDtmcs();
        IJtag::dmi_dmstatus_type dmstatus;
        dmstatus.u32 = read_dmi(IJtag::DMI_DMSTATUS);
        bool st = true;
    }
}

void OpenOcdWrapper::halt() {
    if (openocd_->isEnabled()) {
        gdbReq_ = GdbCommand_Halt();
    }
}

void OpenOcdWrapper::step() {
    if (openocd_->isEnabled()) {
        gdbReq_ = GdbCommand_Step();
    }
}

}  // namespace debugger
