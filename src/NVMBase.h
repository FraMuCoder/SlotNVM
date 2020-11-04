/*
 * SlotNVM
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SLOTNVM_NVMBASE_H_
#define _SLOTNVM_NVMBASE_H_

#include <stdint.h>

typedef uint16_t    address_t;

class NVMBase {
public:
    virtual address_t getSize() = 0;

    virtual bool needErase() = 0;

    virtual bool erase(address_t start, address_t len) = 0;

    virtual bool read(address_t addr, uint8_t &data) = 0;

    virtual bool read(address_t addr, uint8_t *data, address_t len) = 0;

    virtual bool write(address_t addr, uint8_t data) = 0;

    virtual bool write(address_t addr, const uint8_t *data, address_t len) = 0;
};

#endif // _SLOTNVM_NVMBASE_H_
