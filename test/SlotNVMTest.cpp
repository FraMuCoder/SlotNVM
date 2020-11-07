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

#include <cppunit/extensions/HelperMacros.h>


class SlotNVMTest : public CppUnit::TestFixture  {

CPPUNIT_TEST_SUITE( SlotNVMTest );

CPPUNIT_TEST( test_begin );
CPPUNIT_TEST( test_begin_01 );
CPPUNIT_TEST( test_begin_02 );
CPPUNIT_TEST( test_begin_03 );
CPPUNIT_TEST( test_begin_04 );
CPPUNIT_TEST( test_begin_06 );
CPPUNIT_TEST( test_begin_07 );
CPPUNIT_TEST( test_begin_08 );
CPPUNIT_TEST( test_begin_09 );
CPPUNIT_TEST( test_begin_10 );

CPPUNIT_TEST( test_readSlot_01 );
CPPUNIT_TEST( test_readSlot_02 );
CPPUNIT_TEST( test_readSlot_03 );

CPPUNIT_TEST( test_writeSlot_00 );
CPPUNIT_TEST( test_writeSlot_01 );
CPPUNIT_TEST( test_writeSlot_02 );

CPPUNIT_TEST( test_eraseSlot_00 );
CPPUNIT_TEST( test_eraseSlot_01 );

CPPUNIT_TEST( test_getFree_00 );

CPPUNIT_TEST( test_nextFreeCluster );

CPPUNIT_TEST( test_provision );

CPPUNIT_TEST_SUITE_END();

private:
    SlotNVM<NVMRAMMock<64>, 8>      *tinyNVM;
    SlotNVM<NVMRAMMock<256>, 16>    *smallNVM;
    SlotNVM<NVMRAMMock<1024>, 32>   *mediumNVM;
public:
    void setUp() {
        tinyNVM = new SlotNVM<NVMRAMMock<64>, 8>;
        smallNVM = new SlotNVM<NVMRAMMock<256>, 16>;
        mediumNVM = new SlotNVM<NVMRAMMock<1024>, 32>;  
    }

    void tearDown()  {
        delete tinyNVM;
        delete smallNVM;
        delete mediumNVM;
    }

    void setTinyCluster(uint8_t cluster, uint8_t slot, uint8_t age = 0, uint16_t length = 2, 
                        bool isFirst = true, int16_t nextCluster = -1,
                        uint8_t dataA = 0xAB, uint8_t dataB = 0xCD) {
        uint16_t address = cluster * 8;
        tinyNVM->m_memory[address + 0] = slot;
        tinyNVM->m_memory[address + 1] = (age << 6) 
                                       | (isFirst ? 0x20 : 0x00)
                                       | ((nextCluster < 0) ? 0x10 : 0x00);
        tinyNVM->m_memory[address + 2] = (nextCluster < 0) ? cluster : nextCluster;
        tinyNVM->m_memory[address + 3] = length - 1;
        tinyNVM->m_memory[address + 4] = dataA;
        tinyNVM->m_memory[address + 5] = dataB;
        tinyNVM->m_memory[address + 6] = 0xFF;
        tinyNVM->m_memory[address + 7] = 0xA5;
    }

    void test_begin() {
        // empty NVM
        bool ret = mediumNVM->begin();
        CPPUNIT_ASSERT( ret );

        // bad format but repairable
        smallNVM->m_memory[0]  = 0x01;
        smallNVM->m_memory[1]  = 0x20;
        smallNVM->m_memory[15] = 0xA5;
        ret = smallNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( smallNVM->m_memory[0] == 0 );

        // no start cluster
        setTinyCluster(0, 1, 2, 1, false);
        setTinyCluster(1, 1, 1, 2, false);
        ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[1*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == 0 );
    }

    void test_begin_01() {
        // simple data
        setTinyCluster(0, 1);
        setTinyCluster(1, 2);
        setTinyCluster(2, 3);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 1 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[1*8 + 0] == 2 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[2*8 + 0] == 3 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0x07 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == (0x07 << 1) );
    }

    void test_begin_02() {
        // more data in one slot
        setTinyCluster(0, 1, 0, 6, true, 3);
        setTinyCluster(1, 1, 0, 6, false);
        setTinyCluster(3, 1, 0, 6, false, 1);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 1 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[1*8 + 0] == 1 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[3*8 + 0] == 1 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0x0B );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == (0x01 << 1) );
    }

    void test_begin_03() {
        // some old data
        setTinyCluster(0, 1, 0);    // old
        setTinyCluster(2, 1, 1);    // new
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[2*8 + 0] == 1 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0x04 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == (0x01 << 1) );
    }

    void test_begin_04() {
        // incomplete new data
        setTinyCluster(0, 1, 0);            // old but valid
        setTinyCluster(2, 1, 1);            // new ...
        tinyNVM->m_memory[2*8 + 7] = 0xFF;  // ... but incomplete written
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 1 );
        //CPPUNIT_ASSERT( tinyNVM->m_memory[2*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0x01 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == (0x01 << 1) );
    }

    void test_begin_05() {
        // incomplete new data
        setTinyCluster(0, 1, 0);            // old but valid
        setTinyCluster(2, 1, 1, 4, true, 1);// new incomplete, one cluster is missing
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 1 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[2*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0x01 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == (0x01 << 1) );
    }

    void test_begin_06() {
        // wrong age in second cluster
        setTinyCluster(0, 1, 2, 4, true, 1);
        setTinyCluster(1, 1, 1, 4);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[1*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == 0 );
    }

    void test_begin_07() {
        // cluster loop
        setTinyCluster(2, 1, 0, 6, true, 3);
        setTinyCluster(3, 1, 0, 6, false, 4);
        setTinyCluster(4, 1, 0, 6, false, 3);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[2*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[3*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[4*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == 0 );
    }

    void test_begin_08() {
        // too less cluster for data length
        setTinyCluster(0, 1, 2, 3);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == 0 );
    }

    void test_begin_09() {
        // too many cluster for data length
        setTinyCluster(0, 1, 2, 2, true, 1);
        setTinyCluster(1, 1, 2, 2, false);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == 0 );
    }

    void test_begin_10() {
        // like test_begin_05 but reverse cluster order
        setTinyCluster(0, 1, 2, 6, true, 1);    // more and newer data but incomplete
        setTinyCluster(1, 1, 1, 2);             // less and older data but valid
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 2 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == 2 );
    }

    void test_readSlot_01() {
        // simple data
        setTinyCluster(0, 1);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        uint8_t data[10] = {0};
        size_t size = sizeof(data);
        ret = tinyNVM->readSlot(1, data, size);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( size == 2 );
        CPPUNIT_ASSERT( data[0] = 0xAB );
        CPPUNIT_ASSERT( data[1] = 0xCD );
    }

    void test_readSlot_02() {
        // more data
        setTinyCluster(0, 1, 0, 5, true, 1, 0xA1, 0xA2);
        setTinyCluster(1, 1, 0, 5, false, 2, 0xA3, 0xA4);
        setTinyCluster(2, 1, 0, 5, false, -1, 0xA5);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        uint8_t data[10] = {0};
        size_t size = sizeof(data);
        ret = tinyNVM->readSlot(1, data, size);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( size == 5 );
        CPPUNIT_ASSERT( data[0] = 0xA1 );
        CPPUNIT_ASSERT( data[1] = 0xA2 );
        CPPUNIT_ASSERT( data[2] = 0xA3 );
        CPPUNIT_ASSERT( data[3] = 0xA4 );
        CPPUNIT_ASSERT( data[4] = 0xA5 );
    }

    void test_readSlot_03() {
        // read errors

        uint8_t data[10] = {0};
        size_t size = sizeof(data);
        bool ret = tinyNVM->readSlot(1, data, size);// called before begin()
        CPPUNIT_ASSERT( !ret );

        setTinyCluster(0, 1);
        ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        ret = tinyNVM->readSlot(1, NULL, size);     // NULL pointer
        CPPUNIT_ASSERT( !ret );

        size = 1;
        ret = tinyNVM->readSlot(1, data, size);     // buffer to small
        CPPUNIT_ASSERT( !ret );

        size = sizeof(data);
        ret = tinyNVM->readSlot(2, data, size);     // unused slot
        CPPUNIT_ASSERT( !ret );
    }

    void test_writeSlot_00() {
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        // simple write
        uint8_t data[] = { 0xB1, 0xB2 };
        ret = tinyNVM->writeSlot(1, data, sizeof(data));
        CPPUNIT_ASSERT( ret );

        uint8_t dataR[4] = {0};
        size_t size = sizeof(dataR);
        ret = tinyNVM->readSlot(1, dataR, size);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( size == 2 );
        CPPUNIT_ASSERT( dataR[0] == data[0] );
        CPPUNIT_ASSERT( dataR[1] == data[1] );

        // is it still readable after restart, test it with an other SlotNVM instance
        SlotNVM<NVMRAMMock<64>, 8> tinyNVM2;
        tinyNVM2.m_memory = tinyNVM->m_memory;

        ret = tinyNVM2.begin();
        CPPUNIT_ASSERT( ret );
        dataR[0] = dataR[1] = 0;
        size = sizeof(dataR);
        ret = tinyNVM2.readSlot(1, dataR, size);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( size == 2 );
        CPPUNIT_ASSERT( dataR[0] == data[0] );
        CPPUNIT_ASSERT( dataR[1] == data[1] );
    }

    void test_writeSlot_01() {
        setTinyCluster(0, 1, 0, 4, true, 2, 0xA1, 0xA2);
        setTinyCluster(2, 1, 0, 4, false, -1, 0xA3);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        // simple overwrite
        uint8_t data[] = { 0xB1, 0xB2 };
        ret = tinyNVM->writeSlot(1, data, sizeof(data));
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[2*8 + 0] == 0 );
        CPPUNIT_ASSERT( (tinyNVM->m_usedCluster[0] & 0x05) == 0 );

        uint8_t dataR[4] = {0};
        size_t size = sizeof(dataR);
        ret = tinyNVM->readSlot(1, dataR, size);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( size == 2 );
        CPPUNIT_ASSERT( dataR[0] == data[0] );
        CPPUNIT_ASSERT( dataR[1] == data[1] );
    }

    void test_writeSlot_02() {
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        // write more data
        uint8_t data[] = { 0xC1, 0xC2, 0xC3, 0xC4, 0xC5 };
        ret = tinyNVM->writeSlot(1, data, sizeof(data));
        CPPUNIT_ASSERT( ret );

        uint8_t dataR[5] = {0};
        size_t size = sizeof(dataR);
        ret = tinyNVM->readSlot(1, dataR, size);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( size == 5 );
        CPPUNIT_ASSERT( dataR[0] == data[0] );
        CPPUNIT_ASSERT( dataR[1] == data[1] );
        CPPUNIT_ASSERT( dataR[2] == data[2] );
        CPPUNIT_ASSERT( dataR[3] == data[3] );
        CPPUNIT_ASSERT( dataR[4] == data[4] );

        // is it still readable after restart, test it with an other SlotNVM instance
        SlotNVM<NVMRAMMock<64>, 8> tinyNVM2;
        tinyNVM2.m_memory = tinyNVM->m_memory;

        ret = tinyNVM2.begin();
        CPPUNIT_ASSERT( ret );
        dataR[0] = dataR[1] = dataR[2] = dataR[3] = dataR[4] = 0;
        size = sizeof(dataR);
        ret = tinyNVM2.readSlot(1, dataR, size);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( size == 5 );
        CPPUNIT_ASSERT( dataR[0] == data[0] );
        CPPUNIT_ASSERT( dataR[1] == data[1] );
        CPPUNIT_ASSERT( dataR[2] == data[2] );
        CPPUNIT_ASSERT( dataR[3] == data[3] );
        CPPUNIT_ASSERT( dataR[4] == data[4] );
    }

    void test_eraseSlot_00() {
        setTinyCluster(0, 1);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        // simple erase
        uint8_t data[] = { 0xB1, 0xB2 };
        ret = tinyNVM->eraseSlot(1);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == 0 );
    }

    void test_eraseSlot_01() {
        setTinyCluster(0, 1, 0, 4, true, 2);
        setTinyCluster(2, 1, 0, 4, false);
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        // simple erase
        uint8_t data[] = { 0xB1, 0xB2 };
        ret = tinyNVM->eraseSlot(1);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( tinyNVM->m_memory[0*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_memory[2*8 + 0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_usedCluster[0] == 0 );
        CPPUNIT_ASSERT( tinyNVM->m_slotAvail[0] == 0 );
    }

    void test_getFree_00() {
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        size_t total = tinyNVM->getSize();
        CPPUNIT_ASSERT( total == (64/8)*(8-6) );

        size_t free = tinyNVM->getFree();
        CPPUNIT_ASSERT( free == total );

        uint8_t data[] = { 0xC1, 0xC2 };
        ret = tinyNVM->writeSlot(1, data, sizeof(data));
        CPPUNIT_ASSERT( ret );

        free = tinyNVM->getFree();
        CPPUNIT_ASSERT( free == total-(8-6) );
    }

    void test_nextFreeCluster() {
        uint8_t nextCluster = 0;
        bool ret = tinyNVM->nextFreeCluster(nextCluster);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( nextCluster == 1 );

        ret = tinyNVM->nextFreeCluster(nextCluster);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( nextCluster == 2 );

        nextCluster = 6;
        ret = tinyNVM->nextFreeCluster(nextCluster);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( nextCluster == 7 );

        ret = tinyNVM->nextFreeCluster(nextCluster);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( nextCluster == 0 );

        ret = tinyNVM->nextFreeCluster(nextCluster);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( nextCluster == 1 );

        nextCluster = 100;
        ret = tinyNVM->nextFreeCluster(nextCluster);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( nextCluster == 0 );

        setTinyCluster(0, 1);
        setTinyCluster(1, 2);
        setTinyCluster(7, 3);
        setTinyCluster(5, 4);

        ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );

        nextCluster = 4;
        ret = tinyNVM->nextFreeCluster(nextCluster);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( nextCluster == 6 );

        ret = tinyNVM->nextFreeCluster(nextCluster);
        CPPUNIT_ASSERT( ret );
        CPPUNIT_ASSERT( nextCluster == 2 );
    }

    void test_provision() {
        // no provision
        bool ret = tinyNVM->begin();
        CPPUNIT_ASSERT( ret );
        size_t free = tinyNVM->getFree();
        CPPUNIT_ASSERT( free == 16 );

        uint8_t data[] = { 0xC1, 0xC2, 0xC3, 0xC4 };
        ret = tinyNVM->writeSlot(1, data, sizeof(data));    // now 2/8 cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyNVM->getFree();
        CPPUNIT_ASSERT( free == 12 );
        ret = tinyNVM->writeSlot(2, data, sizeof(data));    // now 4/8 cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyNVM->getFree();
        CPPUNIT_ASSERT( free == 8 );
        ret = tinyNVM->writeSlot(3, data, sizeof(data));    // now 6/8 cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyNVM->getFree();
        CPPUNIT_ASSERT( free == 4 );
        ret = tinyNVM->writeSlot(4, data, 2);               // now 7/8 cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyNVM->getFree();
        CPPUNIT_ASSERT( free == 2 );
        ret = tinyNVM->writeSlot(5, data, sizeof(data));    // not possible
        CPPUNIT_ASSERT( !ret );
        ret = tinyNVM->writeSlot(6, data, 2);               // now 8/8 cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyNVM->getFree();
        CPPUNIT_ASSERT( free == 0 );
        ret = tinyNVM->writeSlot(7, data, 2);               // not possible
        CPPUNIT_ASSERT( !ret );

        // with provision
        // Provision is 3 bytes but is rouded to next S_USER_DATA_PER_CLUSTER, so it is set to 4
        SlotNVM<NVMRAMMock<64>, 8, 3> tinyWithProvision;
        ret = tinyWithProvision.begin();
        CPPUNIT_ASSERT( ret );
        free = tinyWithProvision.getFree();
        CPPUNIT_ASSERT( free == 12 );

        ret = tinyWithProvision.writeSlot(1, data, sizeof(data));    // now 2/(6+2) cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyWithProvision.getFree();
        CPPUNIT_ASSERT( free == 8 );
        ret = tinyWithProvision.writeSlot(2, data, sizeof(data));    // now 4/(6+2) cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyWithProvision.getFree();
        CPPUNIT_ASSERT( free == 4 );
        ret = tinyWithProvision.writeSlot(3, data, sizeof(data));    // now 6/(6+2) cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyWithProvision.getFree();
        CPPUNIT_ASSERT( free == 0 );
        ret = tinyWithProvision.writeSlot(4, data, 2);               // not possible
        CPPUNIT_ASSERT( !ret );
        ret = tinyWithProvision.writeSlot(3, data, 1);               // rewrite psoible now 5/(6+2) cluster used
        free = tinyWithProvision.getFree();
        CPPUNIT_ASSERT( free == 2 );
        CPPUNIT_ASSERT( ret );
        ret = tinyWithProvision.writeSlot(5, data, sizeof(data));    // not possible
        CPPUNIT_ASSERT( !ret );
        ret = tinyWithProvision.writeSlot(6, data, 2);               // now 6/(6+2) cluser used
        CPPUNIT_ASSERT( ret );
        free = tinyWithProvision.getFree();
        CPPUNIT_ASSERT( free == 0 );
        ret = tinyWithProvision.writeSlot(7, data, 2);               // not possible
        CPPUNIT_ASSERT( !ret );
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SlotNVMTest );
