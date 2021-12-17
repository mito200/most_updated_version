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

#pragma once

#include <inttypes.h>

#define PLIC_GLOBAL_IRQ_MAX 1024

// sizeof 0x80
typedef struct plic_context_ie_type {
    volatile uint32_t irq_enable[PLIC_GLOBAL_IRQ_MAX/32];  // [RW]
} plic_context_ie_type;

// sizeof 0x1000
typedef struct plic_context_prio_type {
    volatile uint32_t priority;               // [RW] masking (disabling) all interrupt with <= priority
    volatile uint32_t claim_complete;         // [RW] read clears pending bit
    uint8_t rsrv[0x1000 - 2*sizeof(uint32_t)];
} plic_context_prio_type;

typedef struct plic_map {
    volatile uint32_t src_prioirty[PLIC_GLOBAL_IRQ_MAX];                                     // 0x000004: 0 unused, [RW] source priority
    volatile uint32_t pending[PLIC_GLOBAL_IRQ_MAX/32];                                       // 0x001000: [RO] pending array
    uint32_t rsrv1[PLIC_GLOBAL_IRQ_MAX - PLIC_GLOBAL_IRQ_MAX/32];                            // 0x001080
    plic_context_ie_type ctx_ie[(0x200000 - 0x2000)/sizeof(plic_context_ie_type)];           // 0x002000, 0x002080,...
    plic_context_prio_type ctx_prio[(0x04000000 - 0x200000)/sizeof(plic_context_prio_type)]; // 0x200000, 0x201000,..
} plic_map;

