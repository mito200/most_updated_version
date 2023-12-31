/*
 *  Copyright 2021 Sergey Khabarov, sergeykhbr@gmail.com
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

#include <inttypes.h>
#include <string.h>
#include <axi_maps.h>
#include "fw_api.h"

//#define DEBUG_DATA_BLOCK

#define CMD0 0
#define CMD1 1    // Card initialization
#define CMD8 8
#define CMD58 58
#define CMD12 12  // Stop transmission (for Multiple Block Read)
#define CMD17 17  // Read Single Block 
#define CMD18 18  // Multiple Read Operation
#define ACMD41 41
#define CMD55 55  // Next command is an application specific command ACMD
#define DATA_START_BLOCK 0xFE     // for Single Block Read, Single Block Write and Multiple Block Read

#define R1_IDLE_BIT        0x01
#define R1_ERASE_RESET_BIT 0x02
#define R1_ILLEGAL_BIT     0x04
#define R1_CRC_ERR_BIT     0x08

#define RX_WAIT_SYNC 0
#define RX_IS_SYNCED 1

typedef struct SpiDriverDataType {
    qspi_map *map;
    ESdCardType etype;
    int rxcnt;
    uint8_t rxbuf[512]; // 512 data block
    uint16_t crc16_rx;
    uint16_t crc16_calculated;
} SpiDriverDataType;

static int32_t spi_is_idle(SpiDriverDataType *p) {
    // Bits[6:4] state machine state. 0 = idle    
    if ((p->map->rsrv4 & 0x70) == 0) {
        return 1;
    }
    return 0;
}

static void spi_init_crc15(SpiDriverDataType *p) {
    p->map->crc16 = 0;
}

static int32_t spi_send_dummy(SpiDriverDataType *p, uint32_t cnt) {
    p->map->rsrv4 = (cnt << 16);  // transmit bytes 1 no CRC
    while (!spi_is_idle(p)) {}
    return 0;
}

static int32_t spi_send_cmd(SpiDriverDataType *p, uint8_t cmd, uint32_t payload) {
    p->map->txdata = 0x40 | cmd;  // [7] 0=start bit; [6] 1=host request
    p->map->txdata = payload & 0xFF;
    p->map->txdata = (payload >> 8) & 0xFF;
    p->map->txdata = (payload >> 16) & 0xFF;
    p->map->txdata = (payload >> 24) & 0xFF;

    p->map->rsrv4 = (5 << 16) | (1 << 7);  // send cmd + 4 bytes + generate CRC7 byte
    while (!spi_is_idle(p)) {}
    return 0;
}

int read_rx_fifo(SpiDriverDataType *p) {
    int ret = 0;
    uint32_t rdata;

    rdata = p->map->rxdata;
    while ((rdata & QSPI_RXDATA_EMPTY) == 0) {
        if (p->rxcnt < sizeof(p->rxbuf)) {
            p->rxbuf[p->rxcnt++] = (uint8_t)(rdata & 0xFF);
        }
        rdata = p->map->rxdata;
        ret++;
    }
    p->rxcnt = 0;  // prepare for the next read
    return ret;
}

static int32_t spi_recv_bytes(SpiDriverDataType *p, uint32_t cnt, uint32_t synced) {
    uint32_t rxcnt;
    // [8] rx_ena=1
    // [9] rx_synced=0
    // [10] rx_data_block=0
    // [31:16] number of bytes=cnt
    p->map->rsrv4 = (1 << 8) | (synced << 9) | (cnt << 16);
    while (!spi_is_idle(p)) {}
    rxcnt = read_rx_fifo(p);
    if ((uint32_t)rxcnt != cnt) {
        printf_uart("warning: recv_bytes %d != %d\r\n", cnt, rxcnt);
    }
    return rxcnt;
}

static uint32_t spi_recv_sync_data_block(SpiDriverDataType *p) {
    uint32_t rxcnt;
    // [8] rx_ena=1
    // [9] rx_synced=0
    // [10] rx_data_block=1
    // [31:16] number of bytes=1
    p->map->rsrv4 = (1 << 8) | (1 << 10) | (1 << 16);
    while (!spi_is_idle(p)) {}
    rxcnt = read_rx_fifo(p);
    if (rxcnt != 1) {
        printf_uart("warning: recv_bytes %d != 1\r\n", rxcnt);
    }
    return p->rxbuf[0];
}

uint16_t spi_recv_data_block_crc16(SpiDriverDataType *p) {
    uint16_t ret = 0;
    // [8] rx_ena=1
    // [9] rx_synced=1
    // [10] rx_data_block=0
    // [31:16] number of bytes=2
    p->map->rsrv4 = (1 << 8) | (1 << 9) | (2 << 16);
    while (!spi_is_idle(p)) {}
    ret = (uint16_t)(p->map->rxdata & 0xFF);
    ret = (ret << 8) | (uint16_t)(p->map->rxdata & 0xFF);
    return ret;
}

uint8_t get_r1_response(SpiDriverDataType *p) {
    uint8_t ret;
    spi_recv_bytes(p, 1, RX_WAIT_SYNC);
    ret = p->rxbuf[0];
    return ret;
}

// R3 is 40-bits split on [R1 (8-bits),R3 (32-bits)]
uint32_t get_ui32(SpiDriverDataType *p) {
    uint32_t ret = 0;
    int rxcnt;
    rxcnt = spi_recv_bytes(p, 4, RX_IS_SYNCED);
    if (rxcnt != 4) {
        printf_uart("warning: get_ui32 rxcnt != %d\r\n", rxcnt);
    }
    for (int i = 0; i < rxcnt; i++) {
        ret = (ret << 8) | p->rxbuf[i];
    }
    return ret;
}


ESdCardType spi_sd_card_init(SpiDriverDataType *p) {
    int rdcnt;
    uint8_t R1;
    uint32_t R3;
    uint32_t R7;
    uint32_t OCR;
    uint32_t HCS;   // High Capacity Support
    int watchdog;
    clint_map *clint = (clint_map *)ADDR_BUS0_XSLV_CLINT;
    p->etype = SD_Unknown;

    // Reset SD-card
    R1 = 0;
    do {
        printf_uart("CMD0: %08x ", 0x0);
        spi_send_cmd(p, CMD0, 0x0);

        R1 = get_r1_response(p);
        printf_uart("R1: %02x\r\n", R1);
    } while (R1 != 0x01);

    // Delay 2000 usec as implemented in uboot
    watchdog = clint->mtime;
    watchdog += 2 * SYS_HZ / 1000;
    while (clint->mtime < watchdog) {}
    

    // Interface Condition Command:
    // payload[7:0] check pattern. Recommended 10101010b
    // payload[11:8] voltage supplied (VHS)
    //               0000 Not defined
    //               0001 2.7-3.6V
    //               0010 Reserved for Low Voltage Range
    //               0100 Reserved
    //               1000 Reserved
    //               Others Not defined
    printf_uart("%s", "CMD8: 000001AA ");
    spi_send_cmd(p, CMD8, 0x1AA);

    R1 = get_r1_response(p);
    R7 = get_ui32(p);
    printf_uart("R1: %02x %08x\r\n", R1, R7);

    if (R1 == 0x01) {
       // SD-card ver 2 or higher (Standard or High Capacity)
       p->etype = SD_Ver2x_StandardCapacity;
    } else if (R1 & R1_ILLEGAL_BIT) {
       // SD-card ver 1.x or (Standard Capacity)
       p->etype = SD_Ver1x;
    } else {
       p->etype = SD_Unknown;
       return p->etype;
    }

    // CMD58 (Get OCR)
    // Not mandatory to send CMD58. Though it is recommended to be done in order
    // to get the supported voltage range of the card
    // Argument: none; Response R3 (5 bytes R1 + OCR)
    printf_uart("%s", "CMD58: 00000000 ");
    spi_send_cmd(p, CMD58, 0);

    // R3 is 40-bits split on [R1 (8-bits),R3 (32-bits)]
    R1 = get_r1_response(p);
    OCR = get_ui32(p);
    printf_uart("OCR: %02x %08x\r\n", R1, OCR);
    if (R1 != 0x01) {
        p->etype = SD_Unknown;
        return p->etype;
    }


    // ACMD41. Initialize the card.
    // [31] - reserved
    // [20] - HCS High Capacity Support (OCR)
    // [29:24] - reserved
    // [23:0] -Vdd Voltage Windows
    // Argument: none; Response R1
    watchdog = 0;
    do {
        HCS = OCR & 0xFFFFF;  // [23:0] VDD Voltage window
        if (p->etype == SD_Ver2x_StandardCapacity) {
            printf_uart("%s", "CMD55: 00000000 ");
            spi_send_cmd(p, CMD55, 0);
            R1 = get_r1_response(p);
            printf_uart("R1: %02x\r\n", R1);
            HCS |= (1 << 30);
        }

        printf_uart("ACMD41: %08x ", HCS);
        spi_send_cmd(p, ACMD41, HCS);  // Support High Capacity cards
        R1 = get_r1_response(p);
        R3 = get_ui32(p);
        printf_uart("R3: %02x %08x\r\n", R1, R3);
        if (R1 & R1_ILLEGAL_BIT) {
           break;
        }
        if (R1 == 0x01) {
            // initialization in progress, no error just wait:
            watchdog = 0;
        }

        // The 'in idle state' bit in the R1 response of ACMD41 is used by the card 
        // to inform the host if initialization of ACMD41 is completed.
        //     Setting this bit to 1 indicates that the card is still initializing.
        //     Setting this bit to 0 indicates completion of initialization.
        // The host repeatedly issues ACMD41 until this bit is set to 0
    } while (((R1 & R1_IDLE_BIT) != 0) && watchdog++ < 2);


    if (R1 & R1_ILLEGAL_BIT) {
        // ACMD41 is unsupported, use CMD1
        watchdog = 0;
        do {
            printf_uart("CMD1: %08x ", HCS);
            spi_send_cmd(p, CMD1, HCS);  // Support High Capacity cards
            R1 = get_r1_response(p);
            printf_uart("R1: %02x\r\n", R1);
            if (R1 == 0x01) {
                // initialization in progress, no error just wait:
                watchdog = 0;
            }
        } while (((R1 & 0x1) != 0) && watchdog++ < 2);
    }
 
    // SD-card in Ready State: idle = 0
    if (p->etype == SD_Unknown || p->etype == SD_Ver1x) {
        return p->etype;
    }


    if (p->etype == SD_Ver2x_StandardCapacity) {
        // CMD58 (get CCS). Card capacity information (Standard or Extended)
        // Not mandatory to send CMD58. Though it is recommended to be done in order
        // to get the supported voltage range of the card
        // Argument: none; Response R3 (5 bytes R1 + OCR)
        printf_uart("%s", "CMD58: 00000000 ");
        spi_send_cmd(p, CMD58, 0);

        R1 = get_r1_response(p);
        R3 = get_ui32(p);
        HCS = (R3 >> 30) & 0x1;
        printf_uart("R3: %02x %04x HCS=%d\r\n", R1, R3, HCS);

        // R1=0: Ready State without errors:
        if (R1 != 0x00) {
            p->etype = SD_Unknown;
        } else if (HCS) {
            p->etype = SD_Ver2x_HighCapacity;
        } else {
            p->etype = SD_Ver2x_StandardCapacity;
        }
    }

    return p->etype;
}

int spi_sd_read_block(SpiDriverDataType *p) {
   // Check Data token:
    uint8_t data_prefix = 0;

    data_prefix = spi_recv_sync_data_block(p);
#ifdef DEBUG_DATA_BLOCK
    printf_uart("StartToken: %02x ", data_prefix);
#endif
    if (data_prefix != DATA_START_BLOCK) {
        printf_uart("wrong prefix: %02x != %02x\r\n",
                    data_prefix, DATA_START_BLOCK);
        return 0;
    }
#ifdef DEBUG_DATA_BLOCK
    printf_uart("%s", "\r\n");
#endif

    spi_init_crc15(p);
    spi_recv_bytes(p, 512, RX_IS_SYNCED);

    // Expected CRC16:
    p->crc16_calculated = (uint16_t)p->map->crc16;
    // Received CRC16:
    p->crc16_rx = spi_recv_data_block_crc16(p);
    return 512;
}

int spi_sd_card_memcpy(uint64_t src, uint64_t dst, int sz) {
    uint32_t sd_addr;
    int block_size = 512;
    int rdcnt;
    int bytes_copied = 0;
    uint8_t R1;
    prci_map *prci = (prci_map *)ADDR_BUS1_APB_PRCI;
    SpiDriverDataType *p = (SpiDriverDataType *)fw_get_ram_data("spi");

    if (p->etype == SD_Ver2x_HighCapacity) {
        sd_addr = (uint32_t)(src >> 9);  // Data block is always 512 bytes
    } else {
        sd_addr = (uint32_t)src;
        // Block size could be changed from 512 if partial block is enabled
    }

#ifdef DEBUG_DATA_BLOCK
    printf_uart("CMD18: %08x ", sd_addr);
#endif
    spi_send_cmd(p, CMD18, sd_addr);
    R1 = get_r1_response(p);
#ifdef DEBUG_DATA_BLOCK
    printf_uart("R1: %02x\r\n", R1);
#endif

    // SD-card should be in Transfer State without errors:
    if (R1 != 0x00) {
        printf_uart("CMD18 failed src=0x%llx R1: %02x\r\n", (uint64_t)src, R1);
        return 0;
    }

    while (bytes_copied < sz) {
        if (spi_sd_read_block(p)) {
#ifdef DEBUG_DATA_BLOCK
            printf_uart("%s ", "DATA: ");
            for (int i = 0; i < 8; i++) {
                printf_uart("%02x ", p->rxbuf[i]);
            }
            printf_uart(".. %0x ?= %04x\r\n", p->crc16_rx, p->crc16_calculated);
#endif
            if (p->crc16_rx != p->crc16_calculated) {
                printf_uart("src=0x%llx CRC failed: %04x != %04x\r\n", 
                           (uint64_t)src, p->crc16_rx, p->crc16_calculated);
            }

            if ((prci->pll_status & PRCI_PLL_STATUS_DDR_CALIB_DONE) != 0) {
                memcpy((void *)dst, p->rxbuf, 512);
            }
            dst += 512;
            bytes_copied += 512;
        } else {
            printf_uart("Data block prefix not found src=0x%llx\r\n", (uint64_t)src);
            break;
        }
    }

    // Stop Data block
#ifdef DEBUG_DATA_BLOCK
    printf_uart("CMD12: %08x ", sd_addr);
#endif
    spi_send_cmd(p, CMD12, sd_addr);
    R1 = get_r1_response(p);
#ifdef DEBUG_DATA_BLOCK
    printf_uart("R1: %02x\r\n", R1);
#endif

    return bytes_copied;
}

ESdCardType spi_init(void) {
    int watchdog;

    SpiDriverDataType *p = (SpiDriverDataType *)fw_malloc(sizeof(SpiDriverDataType));
    memset(p, 0, sizeof(SpiDriverDataType));
    p->map = (qspi_map *)ADDR_BUS1_APB_QSPI2;
    p->map->sckdiv = 2;    // half period
    p->map->rsrv1[0] = 4096;    // watchdog to receive start bit

    fw_register_ram_data("spi", p);
   
    return spi_sd_card_init(p);
}

