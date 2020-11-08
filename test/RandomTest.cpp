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

// dumy
static uint8_t dummyCRC(uint8_t crc, uint8_t data) {
    return crc ^ data;
}

typedef SlotNVM<NVMRAMMock<1024>, 32>               SlotNVMtoTest;
typedef SlotNVM<NVMRAMMock<1024>, 32, 0, &dummyCRC> SlotNVMcrcToTest;

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

CPPUNIT_TEST( doRndNoCrcTest );
CPPUNIT_TEST( doRndWithCrcTest );

CPPUNIT_TEST_SUITE_END();

private:
    unsigned                            m_cntWrite;
    unsigned                            m_cntErase;
    std::vector<std::vector<uint8_t>>   m_slotData;
    std::random_device                  m_rndDev;
    std::mt19937                        m_rndGen;
    std::discrete_distribution<>        m_rndCmdDist;
    std::uniform_int_distribution<>     m_rndSlotDist;
    std::uniform_int_distribution<>     m_rndByteDist;

public:

    RandomTest()
        : m_rndGen(m_rndDev())
        , m_rndCmdDist({500, 250, 5}) // write, erase, writeErr
        , m_rndSlotDist(1, 250)
        , m_rndByteDist(0, 255)
    {}

    void doRndNoCrcTest() {
        SlotNVMtoTest toTest;
        runTest(toTest, 5000);
    }

    void doRndWithCrcTest() {
        SlotNVMcrcToTest toTest;
        runTest(toTest, 5000);
    }

    template <class T>
    void runTest(T &toTest, unsigned cnt) {
        reset();

        toTest.begin();

        //std::cout << "Start random test..." << std::endl;

        for (unsigned c = 0; c < cnt; c++) {
            switch (m_rndCmdDist(m_rndGen)) {
            case 0: // write
                ++m_cntWrite;
                testWrite(toTest);
                break;
            case 1: // erase
                ++m_cntErase;
                testErase(toTest);
                break;
            case 2: // write error
                toTest.setWriteErrorAfterXbytes(m_rndByteDist(m_rndGen));
                break;
            }
        }

        //std::cout << "Test end (write:" << m_cntWrite << ", erase:" << m_cntErase << ")" << std::endl;
    }

    template <class T>
    void testWrite(T &toTest) {
        uint8_t slot = m_rndSlotDist(m_rndGen);
        size_t len = m_rndByteDist(m_rndGen) + 1;
        std::vector<uint8_t> data(len);
        std::vector<uint8_t> before = toTest.m_memory;


        for (size_t i = 0; i < len; ++i) {
            data[i] = m_rndByteDist(m_rndGen);
        }

        bool res = toTest.writeSlot(slot, &data[0], len);

        if (res) {
            m_slotData[slot-1] = data;
        }

        if (!fullTest(toTest)) {
            std::cout << "Write slot " << std::dec << (int)slot << " error!" << std::endl;
            dumpData(before, "NVM before:");
            dumpData(toTest.m_memory, "NVM after: ");
            CPPUNIT_ASSERT( false );
        }

    }

    template <class T>
    void testErase(T &toTest) {
        uint8_t slot = m_rndSlotDist(m_rndGen);
        std::vector<uint8_t> before = toTest.m_memory;
        bool res = toTest.eraseSlot(slot);

        if (res) {
            m_slotData[slot-1].clear();
        }
        if (!fullTest(toTest)) {
            std::cout << "Erase slot " << std::dec << (int)slot << " error!" << std::endl;
            dumpData(before, "NVM before:");
            dumpData(toTest.m_memory, "NVM after: ");
            CPPUNIT_ASSERT( false );
        }
    }

    template <class T>
    bool fullTest(T &toTest) {
        bool res = nvmTest(toTest);
        if (!res) return res;

        T newTestObj;
        newTestObj.m_memory = toTest.m_memory;
        res = newTestObj.begin();
        if (!res) return res;

        return nvmTest(newTestObj);
    }

    template <class T>
    bool nvmTest(T &toTest) {
        for (int slot = 1; slot <= 250; ++slot) {
            std::vector<uint8_t> data(256);
            size_t len = data.size();

            bool res = toTest.readSlot(slot, &data[0], len);

            if (res) {
                data.resize(len);
                if (m_slotData[slot-1] != data) {
                    dumpData(m_slotData[slot-1], "expected: ");
                    dumpData(data, "read: ");
                    return false;
                }
            } else {
                if (m_slotData[slot-1].size() != 0) {
                    dumpData(m_slotData[slot-1], "expected: ");
                    std::cout << "read: -";
                    return false;
                }
            }
        }

        return true;
    }

    void reset() {
        m_cntWrite = m_cntErase = 0;
        m_slotData.clear();
        m_slotData.resize(250);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( RandomTest );
