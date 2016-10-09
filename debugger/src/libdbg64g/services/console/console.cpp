/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      shell console class implementation.
 */

#include "console.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include "api_types.h"
#include "coreservices/iserial.h"
#include "coreservices/isignal.h"

namespace debugger {

/** Class registration in the Core */
REGISTER_CLASS(ConsoleService)

#define ENTRYSYMBOLS "riscv# "

static const int STDIN = 0;

ConsoleService::ConsoleService(const char *name) 
    : IService(name), IHap(HAP_ConfigDone), portExecutor(this, "") {
    registerInterface(static_cast<IThread *>(this));
    registerInterface(static_cast<IConsole *>(this));
    registerInterface(static_cast<IHap *>(this));
    registerInterface(static_cast<IRawListener *>(this));
    registerAttribute("Enable", &isEnable_);
    registerAttribute("LogFile", &logFile_);
    registerAttribute("StepQueue", &stepQueue_);
    registerAttribute("AutoComplete", &autoComplete_);
    registerAttribute("CommandExecutor", &commandExecutor_);
    registerAttribute("Signals", &signals_);
    registerAttribute("InputPort", &inPort_);

    RISCV_mutex_init(&mutexConsoleOutput_);
    RISCV_event_create(&config_done_, "config_done");
    RISCV_register_hap(static_cast<IHap *>(this));

    isEnable_.make_boolean(true);
    logFile_.make_string("");
	stepQueue_.make_string("");
    autoComplete_.make_string("");
    commandExecutor_.make_string("");
	signals_.make_string("");
    inPort_.make_string("");

    logfile_ = NULL;
    iclk_ = NULL;
    cmdSizePrev_ = 0;

#ifdef DBG_ZEPHYR
    tst_cnt_ = 0;
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#else
    struct termios new_settings;
    tcgetattr(0, &original_settings_);
    new_settings = original_settings_;
     
    /// Disable canonical mode, and set buffer size to 1 byte
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 0;
     
    tcsetattr(STDIN, TCSANOW, &new_settings);
    term_fd_ = fileno(stdin);
#endif
}

ConsoleService::~ConsoleService() {
    if (logfile_) {
        fclose(logfile_);
    }
#if defined(_WIN32) || defined(__CYGWIN__)
#else
    tcsetattr(STDIN, TCSANOW, &original_settings_);
#endif
    RISCV_event_close(&config_done_);
    RISCV_mutex_destroy(&mutexConsoleOutput_);
}

void ConsoleService::postinitService() {
    ISerial *iport = static_cast<ISerial *>
            (RISCV_get_service_iface(inPort_.to_string(), IFACE_SERIAL));
    if (iport) {
        iport->registerRawListener(static_cast<IRawListener *>(this));
    }

    if (isEnable_.to_bool()) {
        if (!run()) {
            RISCV_error("Can't create thread.", NULL);
            return;
        }
    }
    if (logFile_.size()) {
        enableLogFile(logFile_.to_string());
    }

    iclk_ = static_cast<IClock *>
	    (RISCV_get_service_iface(stepQueue_.to_string(), IFACE_CLOCK));

    iautocmd_ = static_cast<IAutoComplete *>(
            RISCV_get_service_iface(autoComplete_.to_string(), 
                                    IFACE_AUTO_COMPLETE));
    if (!iautocmd_) {
        RISCV_error("Can't get IAutoComplete interface %s",
                    autoComplete_.to_string());
    }

    iexec_ = static_cast<ICmdExecutor *>(
            RISCV_get_service_iface(commandExecutor_.to_string(), 
                                    IFACE_CMD_EXECUTOR));
    if (!iexec_) {
        RISCV_error("Can't get ICmdExecutor interface %s",
                    commandExecutor_.to_string());
    } else {
        iexec_->registerRawListener(
            static_cast<IRawListener *>(&portExecutor));
    }

    ISignal *itmp = static_cast<ISignal *>
        (RISCV_get_service_iface(signals_.to_string(), IFACE_SIGNAL));
    if (itmp) {
        itmp->registerSignalListener(static_cast<ISignalListener *>(this));
    }

#ifdef DBG_ZEPHYR
    if (iclk_) {
	    iclk_->registerStepCallback(static_cast<IClockListener *>(this), 550000);
	    iclk_->registerStepCallback(static_cast<IClockListener *>(this), 12000000);
	    iclk_->registerStepCallback(static_cast<IClockListener *>(this), 20000000);//6000000);
	    iclk_->registerStepCallback(static_cast<IClockListener *>(this), 35000000);
	}
#endif

    // Redirect output stream to a this console
    RISCV_set_default_output(static_cast<IConsole *>(this));
}

void ConsoleService::predeleteService() {
    stop();
}

void ConsoleService::stepCallback(uint64_t t) {
#ifdef DBG_ZEPHYR
    if (iclk_ == NULL) {
        return;
    }
    IService *uart = static_cast<IService *>(RISCV_get_service("uart0"));
    if (uart) {
        ISerial *iserial = static_cast<ISerial *>(
                    uart->getInterface(IFACE_SERIAL));
        switch (tst_cnt_) {
        case 0:
            //iserial->writeData("ping", 4);
            iserial->writeData("dhry", 4);
            break;
        case 1:
            iserial->writeData("ticks", 5);
            break;
        case 2:
            iserial->writeData("help", 4);
            break;
        case 3:
            iserial->writeData("pnp", 4);
            break;
        default:;
        }
        tst_cnt_++;
    }
#endif
}



void ConsoleService::hapTriggered(IFace *isrc, EHapType type, 
                                  const char *descr) {
    RISCV_event_set(&config_done_);
}

void ConsoleService::updateSignal(int start, int width, uint64_t value) {
    char sx[128];
    RISCV_sprintf(sx, sizeof(sx), "<led[%d:%d]> %02" RV_PRI64 "xh\n", 
                    start + width - 1, start, value);
    writeBuffer(sx);
}

void ConsoleService::updateData(const char *buf, int buflen) {
    for (int i = 0; i < buflen; i++) {
        if (buf[i] == '\r' || buf[i] == '\n') {
            if (serial_input_.size()) {
                serial_input_ = "<serialconsole> " + serial_input_ + "\n";
                writeBuffer(serial_input_.c_str());
            }
            serial_input_ .clear();
        } else {
            serial_input_ += buf[i];
        }
    }
}

void ConsoleService::busyLoop() {
    RISCV_event_wait(&config_done_);

    bool cmd_ready;
    AttributeType cmd, cursor, cmdres;
    processScriptFile();
    while (isEnabled()) {
        if (!isData()) {
            RISCV_sleep_ms(50);
            continue;
        }

        cmd_ready = iautocmd_->processKey(getData(), &cmd, &cursor);
        if (cmd_ready) {
            RISCV_mutex_lock(&mutexConsoleOutput_);
            std::cout << "\r\n";
            if (logfile_) {
                fwrite(ENTRYSYMBOLS, sizeof(ENTRYSYMBOLS), 1, logfile_);
                fwrite(cmd.to_string(), cmd.size(), 1, logfile_);
                fwrite("\n", 1, 1, logfile_);
                fflush(logfile_);
            }
            RISCV_mutex_unlock(&mutexConsoleOutput_);
            if (!iexec_->exec(cmd.to_string(), &cmdres, false)) {
                // No response data:
                RISCV_mutex_lock(&mutexConsoleOutput_);
                std::cout << '\r' << ENTRYSYMBOLS ;
                RISCV_mutex_unlock(&mutexConsoleOutput_);
            }
        } else {
            RISCV_mutex_lock(&mutexConsoleOutput_);
            std::cout << '\r' << ENTRYSYMBOLS << cmd.to_string();
            if (cmdSizePrev_ > cmd.size()) {
                clearLine(static_cast<int>(cmdSizePrev_ - cmd.size()));
            }
            RISCV_mutex_unlock(&mutexConsoleOutput_);
        }
        std::cout.flush();
        cmdSizePrev_ = cmd.size();
    }
    loopEnable_ = false;
    threadInit_.Handle = 0;
}

void ConsoleService::processScriptFile() {
    enum EScriptState {
        SCRIPT_normal,
        SCRIPT_comment,
        SCRIPT_command
    } scr_state;
    scr_state = SCRIPT_normal;

    const AttributeType *glb = RISCV_get_global_settings();
    if ((*glb)["ScriptFile"].size() == 0) {
        return;
    }
    const char *script_name = (*glb)["ScriptFile"].to_string();
    FILE *script = fopen(script_name, "r");
    if (!script) {
        RISCV_error("Script file '%s' not found", script_name);
        return;
    } 
    fseek(script, 0, SEEK_END);
    long script_sz = ftell(script);
    if (script_sz == 0) {
        return;
    }
    char *script_buf = new char [script_sz + 1];
    fseek(script, 0, SEEK_SET);
    fread(script_buf, 1, script_sz, script);
    script_buf[script_sz] = '\0';
    fclose(script);

    bool crlf = false;
    for (long i = 0; i < script_sz; i++) {

        switch (scr_state) {
        case SCRIPT_normal:
            if (crlf && script_buf[i] == '\n') {
                crlf = false;
            } else if (script_buf[i] == '/'
                    && script_buf[i + 1] == '/') {
                scr_state = SCRIPT_comment;
                i++;
            } else {
                //addToCommandLine(script_buf[i]);
            }
            break;
        case SCRIPT_command:
            //addToCommandLine(script_buf[i]);
        case SCRIPT_comment:
            if (script_buf[i] == '\r' || script_buf[i] == '\n') {
                scr_state = SCRIPT_normal;
            }
        default:;
        }

        crlf = script_buf[i] == '\r';
    }
    delete [] script_buf;
    RISCV_info("Script '%s' was finished", script_name);
}

void ConsoleService::writeBuffer(const char *buf) {
    size_t sz = strlen(buf);
    if (!sz) {
        return;
    }
    RISCV_mutex_lock(&mutexConsoleOutput_);
    clearLine(70);
    std::cout << buf;
    if (buf[sz-1] != '\r' && buf[sz-1] != '\n') {
        std::cout << "\r\n";
    }
    std::cout << ENTRYSYMBOLS << cmdLine_.c_str();
    std::cout.flush();

    if (logfile_) {
        fwrite(buf, strlen(buf), 1, logfile_);
        fflush(logfile_);
    }
    RISCV_mutex_unlock(&mutexConsoleOutput_);
}

void ConsoleService::clearLine(int num) {
    for (int i = 0; i < num; i++) {
        std::cout << ' ';
    }
    for (int i = 0; i < num; i++) {
        std::cout << '\b';
    }
}

void ConsoleService::enableLogFile(const char *filename) {
    if (logfile_) {
        fclose(logfile_);
        logfile_ = NULL;
    }
    logfile_ = fopen(filename, "w");
    if (!logfile_) {
        RISCV_error("Can not open file '%s'", filename);
    }
}

bool ConsoleService::isData() {
#if defined(_WIN32) || defined(__CYGWIN__)
    return _kbhit() ? true: false;
#else
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting != 0;
#endif
}

uint8_t ConsoleService::getData() {
#if defined(_WIN32) || defined(__CYGWIN__)
    return static_cast<uint8_t>(_getch());
#else
    unsigned char ch;
    //int err = 
    read(term_fd_, &ch, sizeof(ch));
    return ch;
    //return getchar();
#endif
}

/*void ConsoleService::addToCommandLine(int val) {
    bool set_history_end = true;
    uint8_t symb = static_cast<uint8_t>(val);
    if (!convertToWinKey(symb)) {
        return;
    }

    switch (symb_seq_) {
    case 0:
        break;
    case (ARROW_PREFIX << 8) | KB_UP:
        set_history_end = false;
        if (history_idx_ == history_.size()) {
            unfinshedLine_ = cmdLine_;
        }
        if (history_idx_ > 0) {
            history_idx_--;
        }
        RISCV_mutex_lock(&mutexConsoleOutput_);
        cmdLine_ = std::string(history_[history_idx_].to_string());
        clearLine();
        std::cout << ENTRYSYMBOLS << cmdLine_;
        std::cout.flush();
        RISCV_mutex_unlock(&mutexConsoleOutput_);
        break;
    case (ARROW_PREFIX << 8) | KB_DOWN:
        set_history_end = false;
        if (history_idx_ == (history_.size() - 1)) {
            history_idx_++;
            cmdLine_ = unfinshedLine_;
        } else if (history_idx_ < (history_.size() - 1)) {
            history_idx_++;
            cmdLine_ = std::string(history_[history_idx_].to_string());
        }
        RISCV_mutex_lock(&mutexConsoleOutput_);
        clearLine();
        std::cout << ENTRYSYMBOLS << cmdLine_;
        std::cout.flush();
        RISCV_mutex_unlock(&mutexConsoleOutput_);
        break;
    case '\b':// 1. Backspace button:
        if (cmdLine_.size()) {
            RISCV_mutex_lock(&mutexConsoleOutput_);
            cmdLine_.erase(cmdLine_.size() - 1);
            std::cout << "\b \b";
            std::cout.flush();
            RISCV_mutex_unlock(&mutexConsoleOutput_);
        }
        break;
    case '\n':
    case '\r':// 2. Enter button:
        processCommandLine();
        break;
    default:
        RISCV_mutex_lock(&mutexConsoleOutput_);
        cmdLine_ += symb;
        std::cout << symb;
        std::cout.flush();
        RISCV_mutex_unlock(&mutexConsoleOutput_);
    }

    if (set_history_end) {
        history_idx_ = history_.size();
    }
}
*/
void ConsoleService::processCommandLine() {
    //symb_seq_ = 0;
    //addToHistory(cmdLine_.c_str());

    RISCV_mutex_lock(&mutexConsoleOutput_);
    char tmpStr[256];
    char *pStr = tmpStr;
    if (cmdLine_.size() >= 256) {
        pStr = new char [cmdLine_.size() + 1];
    }
    strcpy(pStr, cmdLine_.c_str());
    cmdLine_.clear();

    std::cout << "\r\n" ENTRYSYMBOLS;
    if (logfile_) {
        int len = sprintf(tmpbuf_, ENTRYSYMBOLS "%s\n", pStr);
        fwrite(tmpbuf_, len, 1, logfile_);
        fflush(logfile_);
    }
    RISCV_mutex_unlock(&mutexConsoleOutput_);

    if (pStr[0] == '\0') {
        return;
    }

    AttributeType t1;
    t1.from_config(pStr);
    if (t1.is_list()) {
        if (strcmp(t1[0u].to_string(), "wait") == 0) {
            RISCV_sleep_ms(static_cast<int>(t1[1].to_int64()));
        } else if (strcmp(t1[0u].to_string(), "uart0") == 0) {
            std::string strCmd(t1[1].to_string());
            size_t idx = strCmd.find("\\r", 0);
            if (idx != std::string::npos) {
                strCmd.replace(idx, 2, "\r");
            }
            IService *uart = 
                static_cast<IService *>(RISCV_get_service("uart0"));
            if (uart) {
                ISerial *iserial = static_cast<ISerial *>(
                            uart->getInterface(IFACE_SERIAL));
                iserial->writeData(strCmd.c_str(),
                                   static_cast<int>(strCmd.size()));
            }
        }
    } else {
        /*for (unsigned i = 0; i < consoleListeners_.size(); i++) {
            IConsoleListener *ilstn = 
            static_cast<IConsoleListener *>(consoleListeners_[i].to_iface());
            ilstn->udpateCommand(pStr);
        }*/
    }
    if (pStr != tmpStr) {
        delete [] pStr;
    }
}


}  // namespace debugger
