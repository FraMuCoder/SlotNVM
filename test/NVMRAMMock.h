/*
 * SlotNVM
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SLOTNVM_NVMRAMMOCK_H_
#define _SLOTNVM_NVMRAMMOCK_H_

#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <exception>
#include "NVMBase.h"

class PowerLostException : public std::exception {

};

template <address_t SIZE, bool NEED_ERASE = false, uint8_t DEFAULT_VALUE = 0xFF, address_t PAGE_SIZE = 128>
class NVMRAMMock {
public:
    static const address_t S_SIZE = SIZE;

    NVMRAMMock();

    static address_t getSize() { return SIZE; }

    static bool needErase() { return NEED_ERASE; }

    bool erase(address_t start, address_t len);

    bool read(address_t addr, uint8_t &data) const;

    bool read(address_t addr, uint8_t *data, address_t len) const;

    bool write(address_t addr, uint8_t data);

    bool write(address_t addr, const uint8_t *data, address_t len);

    void setWriteErrorAfterXbytes(address_t bytes) {
        m_writeErrorAfterXbytes = bytes;
    }

    void dump() const;

private:
    std::vector<uint8_t>    m_memory;
    std::vector<size_t>     m_writeCount;
    address_t               m_writeErrorAfterXbytes;
};


template <address_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, address_t PAGE_SIZE>
NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::NVMRAMMock()
    : m_memory(SIZE, DEFAULT_VALUE)
    , m_writeCount(SIZE)
    , m_writeErrorAfterXbytes(0)
{}

template <address_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, address_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::erase(address_t start, address_t len) {
    if (NEED_ERASE && (len > 0) && (start < SIZE)) {
        // round up to next page size
        --len;
        len /= PAGE_SIZE;
        len *= PAGE_SIZE;
        len += PAGE_SIZE;

        address_t end = start + len;
        for (address_t i = start; i < end; ++i) {
            m_memory[i] = DEFAULT_VALUE;
            ++(m_writeCount[i]);
        }
        return false;
    } else {
        return false;
    }
}

template <address_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, address_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::read(address_t addr, uint8_t &data) const {
    if (addr < SIZE) {
        data = m_memory[addr];
        return true;
    } else {
        return false;
    }
}

template <address_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, address_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::read(address_t addr, uint8_t *data, address_t len) const {
    if ((addr < SIZE) && (data != NULL)) {
        std::memcpy(data, &m_memory[addr], len);
        return true;
    } else {
        return false;
    }
}

template <address_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, address_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::write(address_t addr, uint8_t data) {
    if (addr < SIZE) {
        if (m_writeErrorAfterXbytes > 0) {
            --m_writeErrorAfterXbytes;
            if (m_writeErrorAfterXbytes == 0) {
                throw PowerLostException();
                return false;
            }
        }
        if (NEED_ERASE) {
            if (DEFAULT_VALUE == 0xFF) {
                m_memory[addr] &= data;
            } else {
                m_memory[addr] |= data;
            }
        } else {
            m_memory[addr] = data;
            ++(m_writeCount[addr]);
        }
        return true;
    } else {
        return false;
    } 
}

template <address_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, address_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::write(address_t addr, const uint8_t *data, address_t len) {
    if ((addr < SIZE) && (data != NULL)) {
        for (address_t i = 0; i < len; ++i) {
            bool ret = write(addr+i, data[i]);
            if (!ret) return false;
        }
        return true;
    } else {
        return false;
    } 
}

template <address_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, address_t PAGE_SIZE>
void NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::dump() const {
    const address_t blockSize = 16;
    for (address_t i = 0; i < (SIZE / blockSize); ++i) {
        std::cout << std::hex << std::setfill('0') << std::right;
        std::cout << std::setw(4) << i << ":";
        for (address_t o = 0; o < blockSize; ++o) {
            if (o == (blockSize/2)) {
                std::cout << " -";
            }
            std::cout << " " << std::setw(2) << (int)m_memory[i*blockSize+o];
        }
        std::cout << std::endl;
    }
}

#endif // _SLOTNVM_NVMRAMMOCK_H_
