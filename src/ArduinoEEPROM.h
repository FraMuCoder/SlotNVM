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


template <address_t SIZE = E2END + 1>
class ArduinoEEPROM {
public:
    static const address_t S_SIZE = SIZE;

    ArduinoEEPROM();

    static address_t getSize() { return SIZE; }

    static bool needErase() { return false; }

    bool erase(address_t start, address_t len);

    bool read(address_t addr, uint8_t &data) const;

    bool read(address_t addr, uint8_t *data, address_t len) const;

    bool write(address_t addr, uint8_t data);

    bool write(address_t addr, const uint8_t *data, address_t len);

private:

};


template <address_t SIZE>
ArduinoEEPROM<SIZE>::ArduinoEEPROM() {}

template <address_t SIZE>
bool ArduinoEEPROM<SIZE>::erase(address_t start, address_t len) {
    return false;
}

template <address_t SIZE>
bool ArduinoEEPROM<SIZE>::read(address_t addr, uint8_t &data) const {
    if (addr < SIZE) {
        data = EEPROM[addr];
        return true;
    } else {
        return false;
    }
}

template <address_t SIZE>
bool ArduinoEEPROM<SIZE>::read(address_t addr, uint8_t *data, address_t len) const {
    if ((addr < SIZE) && (data != NULL)) {
        for (address_t i = 0; i < len; ++i) {
            data[i] = EEPROM[addr + i];
        }
        return true;
    } else {
        return false;
    }
}

template <address_t SIZE>
bool ArduinoEEPROM<SIZE>::write(address_t addr, uint8_t data) {
    if (addr < SIZE) {
        EEPROM[addr] = data;
        return true;
    } else {
        return false;
    } 
}

template <address_t SIZE>
bool ArduinoEEPROM<SIZE>::write(address_t addr, const uint8_t *data, address_t len) {
    if ((addr < SIZE) && (data != NULL)) {
        for (address_t i = 0; (i < len) && ((addr+i) < SIZE); ++i) {
            EEPROM[addr+i] = data[i];
        }
        return true;
    } else {
        return false;
    } 
}


#endif // _SLOTNVM_ARDUINOEEPROM_H_