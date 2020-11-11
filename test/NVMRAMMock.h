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

template <nvm_size_t SIZE, bool NEED_ERASE = false, uint8_t DEFAULT_VALUE = 0xFF, nvm_size_t PAGE_SIZE = 128>
class NVMRAMMock {
public:
    static const nvm_size_t S_SIZE = SIZE;

    NVMRAMMock();

    static nvm_size_t getSize() { return SIZE; }

    static bool needErase() { return NEED_ERASE; }

    bool erase(nvm_address_t start, nvm_size_t len);

    bool read(nvm_address_t addr, uint8_t &data) const;

    bool read(nvm_address_t addr, uint8_t *data, nvm_size_t len) const;

    bool write(nvm_address_t addr, uint8_t data);

    bool write(nvm_address_t addr, const uint8_t *data, nvm_size_t len);

    void setWriteErrorAfterXbytes(uint16_t bytes) {
        m_writeErrorAfterXbytes = bytes;
    }

    void dump() const;

    void dumpWriteCounts() const;

private:
    std::vector<uint8_t>    m_memory;
    std::vector<size_t>     m_writeCount;
    uint16_t                m_writeErrorAfterXbytes;
};


template <nvm_size_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, nvm_size_t PAGE_SIZE>
NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::NVMRAMMock()
    : m_memory(SIZE, DEFAULT_VALUE)
    , m_writeCount(SIZE)
    , m_writeErrorAfterXbytes(0)
{}

template <nvm_size_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, nvm_size_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::erase(nvm_address_t start, nvm_size_t len) {
    if (NEED_ERASE && (len > 0) && (start < SIZE)) {
        // round up to next page size
        --len;
        len /= PAGE_SIZE;
        len *= PAGE_SIZE;
        len += PAGE_SIZE;

        nvm_size_t end = start + len;
        for (nvm_size_t i = start; i < end; ++i) {
            m_memory[i] = DEFAULT_VALUE;
            ++(m_writeCount[i]);
        }
        return false;
    } else {
        return false;
    }
}

template <nvm_size_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, nvm_size_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::read(nvm_address_t addr, uint8_t &data) const {
    if (addr < SIZE) {
        data = m_memory[addr];
        return true;
    } else {
        return false;
    }
}

template <nvm_size_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, nvm_size_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::read(nvm_address_t addr, uint8_t *data, nvm_size_t len) const {
    if ((addr < SIZE) && (data != NULL)) {
        std::memcpy(data, &m_memory[addr], len);
        return true;
    } else {
        return false;
    }
}

template <nvm_size_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, nvm_size_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::write(nvm_address_t addr, uint8_t data) {
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

template <nvm_size_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, nvm_size_t PAGE_SIZE>
bool NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::write(nvm_address_t addr, const uint8_t *data, nvm_size_t len) {
    if ((addr < SIZE) && (data != NULL)) {
        for (nvm_size_t i = 0; i < len; ++i) {
            bool ret = write(addr+i, data[i]);
            if (!ret) return false;
        }
        return true;
    } else {
        return false;
    } 
}

template <nvm_size_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, nvm_size_t PAGE_SIZE>
void NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::dump() const {
    const nvm_size_t blockSize = 16;
    for (nvm_size_t i = 0; i < (SIZE / blockSize); ++i) {
        std::cout << std::hex << std::setfill('0') << std::right;
        std::cout << std::setw(4) << i << ":";
        for (nvm_size_t o = 0; o < blockSize; ++o) {
            if (o == (blockSize/2)) {
                std::cout << " -";
            }
            std::cout << " " << std::setw(2) << (int)m_memory[i*blockSize+o];
        }
        std::cout << std::endl;
    }
}

template <nvm_size_t SIZE, bool NEED_ERASE, uint8_t DEFAULT_VALUE, nvm_size_t PAGE_SIZE>
void NVMRAMMock<SIZE, NEED_ERASE, DEFAULT_VALUE, PAGE_SIZE>::dumpWriteCounts() const {
    const nvm_size_t blockSize = 16;
    for (nvm_size_t i = 0; i < (SIZE / blockSize); ++i) {
        std::cout << std::hex << std::setfill('0') << std::right;
        std::cout << std::setw(4) << i << ":";
        for (nvm_size_t o = 0; o < blockSize; ++o) {
            if (o == (blockSize/2)) {
                std::cout << " -";
            }
            std::cout << " " << std::setw(4) << (int)m_writeCount[i*blockSize+o];
        }
        std::cout << std::endl;
    }
}

#endif // _SLOTNVM_NVMRAMMOCK_H_
