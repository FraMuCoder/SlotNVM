/*
 * SlotNVM
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SLOTNVM_NVMBASE_H_
#define _SLOTNVM_NVMBASE_H_

#include <stdint.h>

typedef uint16_t    nvm_address_t;
typedef uint16_t    nvm_size_t;     // width must be >= nvm_address_t, set this to uint32_t if you need 64KiByte EEPROM

class NVMBase {
public:
    virtual nvm_address_t getSize() = 0;

    virtual bool needErase() = 0;

    virtual bool erase(nvm_address_t start, nvm_size_t len) = 0;

    virtual bool read(nvm_address_t addr, uint8_t &data) = 0;

    virtual bool read(nvm_address_t addr, uint8_t *data, nvm_size_t len) = 0;

    virtual bool write(nvm_address_t addr, uint8_t data) = 0;

    virtual bool write(nvm_address_t addr, const uint8_t *data, nvm_size_t len) = 0;
};

#endif // _SLOTNVM_NVMBASE_H_
