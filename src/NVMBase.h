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

/// Use this as base for your NVM access class.
/// You do not need to derive form this class, just use the same members.
class NVMBase {
public:
    static const nvm_size_t S_SIZE = 0;

    /// currently not needed
    virtual nvm_address_t getSize() = 0;

    /// currently not needed
    virtual bool needErase() = 0;

    /// currently not needed
    virtual bool erase(nvm_address_t start, nvm_size_t len) = 0;

    /// read one byte
    virtual bool read(nvm_address_t addr, uint8_t &data) = 0;

    /// read a byte block
    virtual bool read(nvm_address_t addr, uint8_t *data, nvm_size_t len) = 0;

    /// write one byte
    virtual bool write(nvm_address_t addr, uint8_t data) = 0;

    /// write a byte block
    virtual bool write(nvm_address_t addr, const uint8_t *data, nvm_size_t len) = 0;
};

#endif // _SLOTNVM_NVMBASE_H_
