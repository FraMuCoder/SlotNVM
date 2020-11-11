/*
 * SlotNVM
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */
 
#ifndef _SLOTNVM_ARDUINOEEPROM_H_
#define _SLOTNVM_ARDUINOEEPROM_H_

#include "NVMBase.h"
#include <EEPROM.h>


template <nvm_size_t SIZE = E2END + 1>
class ArduinoEEPROM {
public:
    static const nvm_address_t S_SIZE = SIZE;

    ArduinoEEPROM();

    static nvm_address_t getSize() { return SIZE; }

    static bool needErase() { return false; }

    bool erase(nvm_address_t start, nvm_size_t len);

    bool read(nvm_address_t addr, uint8_t &data) const;

    bool read(nvm_address_t addr, uint8_t *data, nvm_size_t len) const;

    bool write(nvm_address_t addr, uint8_t data);

    bool write(nvm_address_t addr, const uint8_t *data, nvm_size_t len);

private:

};


template <nvm_size_t SIZE>
ArduinoEEPROM<SIZE>::ArduinoEEPROM() {}

template <nvm_size_t SIZE>
bool ArduinoEEPROM<SIZE>::erase(nvm_address_t start, nvm_size_t len) {
    return false;
}

template <nvm_size_t SIZE>
bool ArduinoEEPROM<SIZE>::read(nvm_address_t addr, uint8_t &data) const {
    if (addr < SIZE) {
        data = EEPROM[addr];
        return true;
    } else {
        return false;
    }
}

template <nvm_size_t SIZE>
bool ArduinoEEPROM<SIZE>::read(nvm_address_t addr, uint8_t *data, nvm_size_t len) const {
    if ((addr < SIZE) && (data != NULL)) {
        for (nvm_address_t i = 0; i < len; ++i) {
            data[i] = EEPROM[addr + i];
        }
        return true;
    } else {
        return false;
    }
}

template <nvm_size_t SIZE>
bool ArduinoEEPROM<SIZE>::write(nvm_address_t addr, uint8_t data) {
    if (addr < SIZE) {
        EEPROM[addr] = data;
        return true;
    } else {
        return false;
    } 
}

template <nvm_size_t SIZE>
bool ArduinoEEPROM<SIZE>::write(nvm_address_t addr, const uint8_t *data, nvm_size_t len) {
    if ((addr < SIZE) && (data != NULL)) {
        for (nvm_address_t i = 0; (i < len) && ((addr+i) < SIZE); ++i) {
            EEPROM[addr+i] = data[i];
        }
        return true;
    } else {
        return false;
    } 
}


#endif // _SLOTNVM_ARDUINOEEPROM_H_