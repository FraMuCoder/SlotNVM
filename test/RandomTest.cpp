/*
 * SlotNVM
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

// include all headers needed by classes under test before define private and protected as public
#include <iostream>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <cstdint>
#include <cstring>
#include <iomanip>

// make all public just for testing
#define private public
#define protected public

#include "SlotNVM.h"
#include "NVMRAMMock.h"

// and reset defines
#undef private
#undef protected

#include <random>
#include <cppunit/extensions/HelperMacros.h>


typedef SlotNVM<NVMRAMMock<1024>, 32> SlotNVMtoTest;

void dumpData(std::vector<uint8_t> &data , const std::string &str = "") {
    std::cout << str << std::dec << "[" << data.size() << "]" << std::hex << std::setfill('0') << std::right;

    for (size_t i = 0; i < data.size(); ++i) {
        if ((i % 16) == 0) {
            std::cout << std::endl;
            std::cout << std::setw(4) << i << ":";
        } else if ((i % 8) == 0) {
            std::cout << " -";
        }
        std::cout << " " << std::setw(2) << (int)data[i];
    }

    std::cout << std::endl;
}

class RandomTest : public CppUnit::TestFixture {

CPPUNIT_TEST_SUITE( RandomTest );

CPPUNIT_TEST( doRndTest );

CPPUNIT_TEST_SUITE_END();

private:
    SlotNVMtoTest      *m_slotNVM;
    unsigned            m_cntWrite;
    unsigned            m_cntErase;
    std::vector<std::vector<uint8_t>>   m_oldData;
    std::vector<std::vector<uint8_t>>   m_newData;
    std::random_device  m_rndDev;
    std::mt19937        m_rndGen;
    std::discrete_distribution<>    m_rndCmdDist;
    std::uniform_int_distribution<> m_rndSlotDist;
    std::uniform_int_distribution<> m_rndByteDist;

//    void runTest(unsigned cnt);

//    void testWrite();
//    void testErase();

//    bool fullTest();
//    bool nvmTest(SlotNVMtoTest &toTest);

//    void reset();

public:

    RandomTest()
        : m_slotNVM(NULL)
        , m_rndGen(m_rndDev())
        , m_rndCmdDist({5000, 2500}) // write, erase
        , m_rndSlotDist(1, 250)
        , m_rndByteDist(0, 255)
    {}

    void doRndTest() {
        runTest(5000);
    }

    void runTest(unsigned cnt) {
        reset();

        m_slotNVM->begin();

        std::cout << "Start a test..." << std::endl;

        for (unsigned c = 0; c < cnt; c++) {
            switch (m_rndCmdDist(m_rndGen)) {
            case 0: // write
                ++m_cntWrite;
                testWrite();
                break;
            case 1: // erase
                ++m_cntErase;
                testErase();
                break;
            }
        }

        std::cout << "Test end (write:" << m_cntWrite << ", erase:" << m_cntErase << ")" << std::endl;
    }

    void testWrite() {
        uint8_t slot = m_rndSlotDist(m_rndGen);
        size_t len = m_rndByteDist(m_rndGen) + 1;
        std::vector<uint8_t> data(len);
        std::vector<uint8_t> before = m_slotNVM->m_memory;


        for (size_t i = 0; i < len; ++i) {
            data[i] = m_rndByteDist(m_rndGen);
        }

        m_oldData[slot-1] = std::move<>(m_newData[slot-1]);
        m_newData[slot-1] = data;

        bool res = m_slotNVM->writeSlot(slot, &data[0], len);

        if (!res) {
            m_newData[slot-1] = std::move<>(m_oldData[slot-1]);
        } else {
            m_newData[slot-1] = data;
        }
        if (!fullTest()) {
            std::cout << "Write slot " << slot << " error!" << std::endl;
            dumpData(before, "NVM before:");
            dumpData(m_slotNVM->m_memory, "NVM after: ");
        }

    }

    void testErase() {
        uint8_t slot = m_rndSlotDist(m_rndGen);
        std::vector<uint8_t> before = m_slotNVM->m_memory;
        bool res = m_slotNVM->eraseSlot(slot);

        if (res) {
            m_oldData[slot-1].clear();
            m_newData[slot-1].clear();
        }
        if (!fullTest()) {
            std::cout << "Erase slot " << slot << " error!" << std::endl;
            dumpData(before, "NVM before:");
            dumpData(m_slotNVM->m_memory, "NVM after: ");
        }
    }

    bool fullTest() {
        bool res = nvmTest(*m_slotNVM);
        if (!res) return res;

        SlotNVMtoTest newTestObj;
        newTestObj.m_memory = m_slotNVM->m_memory;
        res = newTestObj.begin();
        if (!res) return res;

        return nvmTest(newTestObj);
    }

    bool nvmTest(SlotNVMtoTest &toTest) {
        for (int slot = 1; slot <= 250; ++slot) {
            std::vector<uint8_t> data(256);
            size_t len = data.size();

            bool res = toTest.readSlot(slot, &data[0], len);

            if (res) {
                data.resize(len);
                if (m_newData[slot-1] != data) {
                    dumpData(m_newData[slot-1], "expected: ");
                    dumpData(data, "read: ");
                    return false;
                }
            } else {
                if (m_newData[slot-1].size() != 0) {
                    dumpData(m_newData[slot-1], "expected: ");
                    std::cout << "read: -";
                    return false;
                }
            }
        }

        return true;
    }

    void reset() {
        if (m_slotNVM) delete m_slotNVM;
        m_slotNVM = new SlotNVMtoTest;
        m_cntWrite = m_cntErase = 0;
        m_oldData.clear();
        m_oldData.resize(250);
        m_newData.clear();
        m_newData.resize(250);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( RandomTest );
