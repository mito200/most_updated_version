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

#include "tcpclient.h"

namespace debugger {

TcpClient::TcpClient(const char *name) : IService(name) {
    registerInterface(static_cast<IThread *>(this));
    registerAttribute("Enable", &isEnable_);
    RISCV_mutex_init(&mutexTx_);
    hsock_ = 0;
    targetIP_.make_string("127.0.0.1");
    targetPort_.make_int64(3333);
}

TcpClient::~TcpClient() {
    RISCV_mutex_destroy(&mutexTx_);
}

void TcpClient::postinitService() {
    if (hsock_ == 0) {
        connectToServer();
    }

    if (isEnable_.to_bool()) {
        if (!run()) {
            RISCV_error("Can't create thread.", NULL);
            return;
        }
    }
}

int TcpClient::updateData(const char *buf, int buflen) {
    int tsz = RISCV_sprintf(&asyncbuf_[0].ibyte, sizeof(asyncbuf_),
                    "['%s',", "Console");

    memcpy(&asyncbuf_[tsz], buf, buflen);
    tsz += buflen;
    asyncbuf_[tsz++].ubyte = ']';
    asyncbuf_[tsz++].ubyte = '\0';

    sendData(&asyncbuf_[0].ubyte, tsz);
    return buflen;
}

void TcpClient::busyLoop() {
    int rxbytes;
    int txbytes;

    txcnt_ = 0;
    while (isEnabled()) {
        rxbytes = recv(hsock_, rxbuf_, sizeof(rxbuf_), 0);
        if (rxbytes <= 0) {
            // Timeout:
            continue;
        } 

        rxbuf_[rxbytes] = '\0';
        RISCV_debug("i=>[%d]: %s", rxbytes, rxbuf_);
        processData(rxbuf_, rxbytes, txbuf_, &txbytes);

        if (txbytes != 0) {
            sendData(txbuf_, txbytes);
        }
    }

    closeSocket();
}

int TcpClient::sendData(const char *buf, int sz) {
    int total = sz;
    const char *ptx = buf;
    int txbytes;

    while (total > 0) {
        txbytes = send(hsock_, ptx, total, 0);
        if (txbytes <= 0) {
            RISCV_error("Send error: txcnt=%d", txcnt_);
            loopEnable_.state = false;
            return -1;
        }
        total -= txbytes;
        ptx += txbytes;
    }
    return 0;
}

int TcpClient::connectToServer() {
    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) < 0) {
        return -1;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    /**
     * Check availability of IPv4 address assigned via attribute 'hostIP'.
     * If it woudn't be found use the last avaialble IP address.
     */
    int retval;
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    retval = getaddrinfo(hostName, "0", &hints, &result);
    if (retval != 0) {
        return -1;
    }

    hsock_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (hsock_ < 0) {
        RISCV_error("%s", "Error: socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)");
        return -1;
    }

    memset(&sockaddr_ipv4_, 0, sizeof(struct sockaddr_in));
    sockaddr_ipv4_.sin_family = AF_INET;
    sockaddr_ipv4_.sin_addr.s_addr = inet_addr(targetIP_.to_string());
    sockaddr_ipv4_.sin_port = htons(static_cast<uint16_t>(targetPort_.to_uint32()));

    int enable = 1;
    if (setsockopt(hsock_, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char *>(&enable), sizeof(int)) < 0) {
        RISCV_error("%s", "setsockopt(SO_REUSEADDR) failed");
        return -1;
    }

    int res = connect(hsock_,
                   reinterpret_cast<struct sockaddr *>(&sockaddr_ipv4_),
                   sizeof(sockaddr_ipv4_));
    if (res != 0) {
        RISCV_error("Error: connect(hsock_, \"%s\", ...)", targetIP_.to_string());
        return -1;
    }

    RISCV_info("IPv4 address %s:%d . . . connected",
                inet_ntoa(sockaddr_ipv4_.sin_addr),
                ntohs(sockaddr_ipv4_.sin_port));

    return 0;
}


void TcpClient::closeSocket() {
    if (hsock_ < 0) {
        return;
    }

#if defined(_WIN32) || defined(__CYGWIN__)
    closesocket(hsock_);
#else
    shutdown(hsock_, SHUT_RDWR);
    close(hsock_);
#endif
    hsock_ = -1;
}

}  // namespace debugger
