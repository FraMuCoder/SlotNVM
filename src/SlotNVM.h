/*
 * SlotNVM
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SLOTNVM_SLOTNVM_H_
#define _SLOTNVM_SLOTNVM_H_

#include <stdint.h>
#include <string.h>
#include "NVMBase.h"

#ifdef __AVR_ARCH__
  #define _SLOTNVM_FLASHMEM_ PROGMEM
#endif

#ifndef _SLOTNVM_FLASHMEM_
  #define _SLOTNVM_FLASHMEM_
#endif

/*
 * Byte
 *  0       Slot No. (0 .. 250)
 *              0x00 or 0xFF cluster not used
 *              0x01 .. 0xFA a valid slot number
 *              0xFB .. 0xFE reserved for future use
 *  1       Bit 0-2 - unused, maybe later for extended length
 *          Bit 3   - skip CRC, 1 byte more user data, currently not supported
 *          Bit 4   - last cluster
 *          Bit 5   - start cluster
 *          Bit 6/7 - age increase every time the slot is rewritten
 *                    the "older" cluster(s) contains the newest data
 *  2       Next cluster number or own number for the last cluster
 *  3       Size of user data
 *
 *  4..n-3  User data
 *
 *  n-2     CRC-8 ToDo
 *  n-1     End byte must be 0xA5. Other values make this cluster invalid
 */

template <class BASE, address_t CLUSTER_SIZE>
class SlotNVM : public BASE {
public:
    static const uint16_t S_CLUSTER_CNT = BASE::S_SIZE / CLUSTER_SIZE;
    static const uint8_t S_USER_DATA_PER_CLUSTER = CLUSTER_SIZE - 6;
    static const uint8_t S_FIRST_SLOT = 1;
    static const uint8_t S_LAST_SLOT = 250;
    static const uint8_t S_END_BYTE = 0xA5;
    static const uint8_t S_AGE_MASK = 0xC0;
    static const uint8_t S_AGE_SHIFT = 6;
    static const uint8_t S_START_CLUSTER_FLAG = 0x20;
    static const uint8_t S_LAST_CLUSTER_FLAG = 0x10;
    static const uint8_t S_AGE_BITS_TO_OLDEST[];
    

    SlotNVM();

    bool begin();

    bool writeSlot(uint8_t slot, uint8_t *data, size_t len);

    bool readSlot(uint8_t slot, uint8_t *data, size_t &len);

    bool eraseSlot(uint8_t slot);

private:
    bool    m_initDone;
    uint8_t m_slotAvail[256 / 8];
    uint8_t m_usedCluster[S_CLUSTER_CNT / 8];
    uint8_t m_maxSlotLen;

    inline void setClusterBit(uint8_t usedCluster[S_CLUSTER_CNT / 8], uint8_t cluster) {
        usedCluster[cluster / 8] |= 1 << (cluster % 8);
    }

    inline void setClusterBit(uint8_t cluster) {
        setClusterBit(m_usedCluster, cluster);
    }

    inline void clearClusterBit(uint8_t usedCluster[S_CLUSTER_CNT / 8], uint8_t cluster) {
        usedCluster[cluster / 8] &= ~(1 << (cluster % 8));
    }

    inline void clearClusterBit(uint8_t cluster) {
        clearClusterBit(m_usedCluster, cluster);
    }

    inline bool isClusterBitSet(uint8_t usedCluster[S_CLUSTER_CNT / 8], uint8_t cluster) {
        return (usedCluster[cluster / 8] & (1 << (cluster % 8))) != 0;
    }

    inline bool isClusterBitSet(uint8_t cluster) {
        return isClusterBitSet(m_usedCluster, cluster);
    }

    inline void setSlotBit(uint8_t slotAvail[256 / 8], uint8_t slot) {
        slotAvail[slot / 8] |= 1 << (slot % 8);
    }

    inline void setSlotBit(uint8_t slot) {
        setSlotBit(m_slotAvail, slot);
    }

    inline void clearSlotBit(uint8_t slotAvail[256 / 8], uint8_t slot) {
        slotAvail[slot / 8] &= ~(1 << (slot % 8));
    }

    inline void clearSlotBit(uint8_t slot) {
        clearSlotBit(m_slotAvail, slot);
    }

    inline bool isSlotBitSet(uint8_t slotAvail[256 / 8], uint8_t slot) {
        return (slotAvail[slot / 8] & (1 << (slot % 8))) != 0;
    }

    inline bool isSlotBitSet(uint8_t slot) {
        return isSlotBitSet(m_slotAvail, slot);
    }

    bool clearCluster(uint8_t cluster);

    bool clearClusters(uint8_t firstCluster);

    bool findStartCluser(uint8_t slot, uint8_t &startCluster);

    bool nextFreeCluster(uint8_t &nextCluster);
};

template <class BASE, address_t CLUSTER_SIZE>
const uint8_t SlotNVM<BASE, CLUSTER_SIZE>::S_AGE_BITS_TO_OLDEST[] _SLOTNVM_FLASHMEM_ = {
        0xF0,   // _ _ _ _  => 0    Error (no age)
        0x00,   // 1 _ _ _  => 0    OK
        0x01,   // _ 1 _ _  => 1    OK
        0x01,   // 0 1 _ _  => 1    OK 0 is the old one
        0x02,   // _ _ 2 _  => 2    OK
        0xF2,   // 0 _ 2 _  => 2    Error there is a gap
        0x02,   // _ 1 2 _  => 2    OK 1 is the old one
        0xF2,   // 0 1 2 _  => 2    Error two old ones
        0x03,   // _ _ _ 3  => 3    OK
        0x00,   // 0 _ _ 3  => 0    OK 3 is the old one
        0xF3,   // _ 1 _ 3  => 3    Error there is a gap
        0xF1,   // 0 1 _ 3  => 1    Error two old ones
        0x03,   // _ _ 2 3  => 3    OK 2 is the old one
        0xF0,   // 0 _ 2 3  => 0    Error two old ones
        0xF3,   // _ 1 2 3  => 3    Error two old ones
        0xF3    // 0 1 2 3  => 3    Error three old ones
    };

template <class BASE, address_t CLUSTER_SIZE>
SlotNVM<BASE, CLUSTER_SIZE>::SlotNVM()
    : m_initDone(false)
    , m_slotAvail{0}
    , m_usedCluster{0}
    , m_maxSlotLen(0)
{
    // ToDo
}

template <class BASE, address_t CLUSTER_SIZE>
bool SlotNVM<BASE, CLUSTER_SIZE>::begin() {
    if (m_initDone) return false;

    // first check used cluster and available slots
    for (uint16_t cluster = 0; cluster < S_CLUSTER_CNT; ++cluster) {
        address_t c_addr = cluster * CLUSTER_SIZE;
        uint8_t slot;
        uint8_t d;

        bool res = this->read(c_addr, slot);                            // read slot no.
        if (!res) return false;
        if ((slot < S_FIRST_SLOT) || (slot > S_LAST_SLOT)) continue;    // skip unused

        res = this->read(c_addr + CLUSTER_SIZE - 1, d);                 // read end byte
        if (!res) return false;
        if (S_END_BYTE != d) continue;                                  // skip incomplete written

        // ToDo CRC check

        // we have found a valid cluster
        setClusterBit(cluster);
        setSlotBit(slot);

        res = this->read(c_addr + 3, d);                                // read user data size
        if (!res) return false;
        if (d > m_maxSlotLen) m_maxSlotLen = d; // ToDo store this only after slot validity check
    }

    // check slot validity
    for (uint8_t slot = S_FIRST_SLOT; slot <= S_LAST_SLOT; ++slot) {
        if (!isSlotBitSet(slot)) continue;                              // skip unused
        uint8_t clusterUsedBySlot[S_CLUSTER_CNT / 8] = {0};
        uint8_t validCluster[S_CLUSTER_CNT / 8];
        uint8_t firstCluster[4] = {0};
        uint8_t firstClusterMask = 0;
        uint8_t firstClusterCnt = 0;

        // find all cluster used by current slot and all start cluster
        for (uint16_t cluster = 0; cluster < S_CLUSTER_CNT; ++cluster) {
            if (!isClusterBitSet(cluster)) continue;                    // skip unused
            address_t c_addr = cluster * CLUSTER_SIZE;
            uint8_t d;

            bool res = this->read(c_addr, d);                           // read slot no.
            if (!res) return false;
            if (d != slot) continue;                                    // skip other slots

            // we don't need to check end byte again, they are already marked as unused in last check

            // remember all cluster so we can delete old and defective
            setClusterBit(clusterUsedBySlot, cluster);

            res = this->read(c_addr + 1, d);                            // read flags
            if (!res) return false;
            uint8_t age = (d & S_AGE_MASK) >> S_AGE_SHIFT;              // age 0..3
            if ((d & S_START_CLUSTER_FLAG) != 0) {                      // start cluster found
                ++firstClusterCnt;
                firstCluster[age] = cluster;
                firstClusterMask |= 1 << age;
            }
        }

        bool foundValid = false;

        // check validity of all founded start cluster beginning with the oldest
        while (!foundValid && (firstClusterMask > 0)) {
            uint8_t oldes;
#ifdef __AVR_ARCH__
            oldes = pgm_read_byte(S_AGE_BITS_TO_OLDEST + firstClusterMask) & 0x03;
#else
            oldes = S_AGE_BITS_TO_OLDEST[firstClusterMask] & 0x03;
#endif
            memset(validCluster, 0, sizeof(validCluster));

            uint8_t startCluster = firstCluster[oldes];
            setClusterBit(validCluster, startCluster);
            address_t c_addr = startCluster * CLUSTER_SIZE;
            uint8_t flags, startLen, curLen;
            uint8_t res = this->read(c_addr + 1, flags);                // read flags
            if (!res) return false;
            res = this->read(c_addr + 3, startLen);                     // read length
            if (!res) return false;
            uint16_t doNotExceetLen = startLen + 1 + S_USER_DATA_PER_CLUSTER; // ToDo dynamic min length
            uint16_t curMaxDataLen = S_USER_DATA_PER_CLUSTER; // should not exceed realLen + S_USER_DATA_PER_CLUSTER

            bool err = false;
            uint8_t curCluster = startCluster;
            while (!err && ((flags & S_LAST_CLUSTER_FLAG) == 0)) {
                res = this->read(c_addr + 2, curCluster);               // read next cluster number
                setClusterBit(validCluster, curCluster);
                if (!res) return false;
                if (isClusterBitSet(clusterUsedBySlot, curCluster)) {   // next cluster belong to this slot
                    c_addr = curCluster * CLUSTER_SIZE;                 // next address
                    res = this->read(c_addr + 1, flags);                // read flags
                    if (!res) return false;

                    if (((flags & S_AGE_MASK) >> S_AGE_SHIFT) != oldes) {
                        err = true;                                     // wrong age
                        break;
                    }

                    if ((flags & S_START_CLUSTER_FLAG) != 0) {          // this is also a start cluster
                        err = true;
                    } else {
                        curMaxDataLen += S_USER_DATA_PER_CLUSTER;
                        if (curMaxDataLen >= doNotExceetLen) {          // more cluster than needed, this also stops..
                            err = true;                                 // ...cluster rings
                        }
                    }
                } else {                                                // next cluster do not belong to this slot
                    err = true;
                }
            }

            if (curMaxDataLen < (startLen + 1)) {                       // some data is missing
                err = true;
            }

            if (!err) {                                                 // we have a winner
                foundValid = true;
            } else {
                firstClusterMask &= ~(1 << oldes);
            }
        } // while (firstClusterMask > 0)

        // remove all not valid cluster
        for (uint16_t cluster = 0; cluster < S_CLUSTER_CNT; ++cluster) {
            if (!isClusterBitSet(clusterUsedBySlot, cluster)) continue;             // skip unused
            if (foundValid && isClusterBitSet(validCluster, cluster)) continue;     // skip valid
            clearCluster(cluster);
        }
        if (!foundValid) {
            clearSlotBit(slot);
        }
    }

    m_initDone = true;

    return m_initDone;
}

template <class BASE, address_t CLUSTER_SIZE>
bool SlotNVM<BASE, CLUSTER_SIZE>::writeSlot(uint8_t slot, uint8_t *data, size_t len) {
    if (!m_initDone) return false;
    if (data == NULL) return false;
    if (len < 1) return false;
    if (len > 256) return false;
    uint8_t oldStartCluster;
    address_t cAddr;
    uint8_t d[4];
    uint8_t newAge = 0;
    bool res;
    bool overwrite = findStartCluser(slot, oldStartCluster);

    if (overwrite) {
        cAddr = oldStartCluster * CLUSTER_SIZE;
        res = this->read(cAddr + 1, d[0]);                      // read old age
        if (!res) return false;
        newAge = (d[0] & S_AGE_MASK) >> S_AGE_SHIFT;
        newAge = ((newAge + 1) << S_AGE_SHIFT) & S_AGE_MASK;
    }

    const uint8_t cntCluster = (len - 1) / S_USER_DATA_PER_CLUSTER + 1;
    uint8_t newCluster[cntCluster];
    uint8_t nextCluster = 255;
    for (uint8_t i = 0; i < cntCluster; ++i) {
        bool ret = nextFreeCluster(nextCluster);
        if (!ret) return false;
        newCluster[i] = nextCluster;
    }

    // write the data beginning with the last cluster
    for (int16_t i = cntCluster-1; i >= 0; --i) {
        nextCluster = newCluster[i];
        cAddr = nextCluster * CLUSTER_SIZE;

        res = this->read(cAddr + CLUSTER_SIZE - 1, d[0]);       // at first read last byte
        if (!res) return false;

        if (d[0] == S_END_BYTE) {
            // last byte should become valid at last, so make it invalid
            res = this->write(cAddr + CLUSTER_SIZE - 1, 0x00);
            if (!res) return false;
        }

        // write the header
        d[0] = slot;
        d[1] = newAge
             | ((i == 0) ? S_START_CLUSTER_FLAG : 0x00) 
             | ((i == (cntCluster-1)) ? S_LAST_CLUSTER_FLAG : 0x00);
        d[2] = (i == (cntCluster-1)) ? slot : newCluster[i+1];
        d[3] = len - 1;
        res = this->write(cAddr, d, 4);
        if (!res) return false;

        // write data
        uint16_t offset = i * S_USER_DATA_PER_CLUSTER;
        size_t toCopy = len - offset;
        if (toCopy > S_USER_DATA_PER_CLUSTER) {
            toCopy = S_USER_DATA_PER_CLUSTER;
        }
        res = this->write(cAddr + 4, data + offset, toCopy);
        if (!res) return false;

        // now make cluster valid
        res = this->write(cAddr + CLUSTER_SIZE - 1, S_END_BYTE);
        if (!res) return false;

        setClusterBit(nextCluster);
    }

    if (overwrite) {
        clearClusters(oldStartCluster); // ignore the result it's to late to say writeSlot gone wrong
    } else {
        setSlotBit(slot);
    }

    return true;
}

template <class BASE, address_t CLUSTER_SIZE>
bool SlotNVM<BASE, CLUSTER_SIZE>::readSlot(uint8_t slot, uint8_t *data, size_t &len) {
    if (!m_initDone) return false;
    if (data == NULL) return false;

    uint8_t curCluster;
    bool res = findStartCluser(slot, curCluster);
    if (!res) return false;

    address_t cAddr = curCluster * CLUSTER_SIZE;
    uint8_t d;

    res = this->read(cAddr + 3, d);                 // read length
    if (!res) return false;
    size_t lenToCopy = d + 1;
    if (lenToCopy > len) return false;
    len = lenToCopy;

    uint8_t flags;
    do {
        res = this->read(cAddr + 1, flags);         // read flags
        if (!res) return false;
        size_t curCopy = (lenToCopy > S_USER_DATA_PER_CLUSTER) ? S_USER_DATA_PER_CLUSTER : lenToCopy;
        res = this->read(cAddr + 4, data, curCopy); // read data into buffer
        if (!res) return false;
        data += curCopy;
        lenToCopy -= curCopy;

        res = this->read(cAddr + 2, curCluster);    // read next cluster
        if (!res) return false;
        cAddr = curCluster * CLUSTER_SIZE;
    } while (((flags & S_LAST_CLUSTER_FLAG) == 0) && (lenToCopy > 0));

    return true;
}

template <class BASE, address_t CLUSTER_SIZE>
bool SlotNVM<BASE, CLUSTER_SIZE>::eraseSlot(uint8_t slot) {
    if (!m_initDone) return false;
    uint8_t firstCluster;
    bool res = findStartCluser(slot, firstCluster);
    if (!res) return false;
    res = clearClusters(firstCluster);
    if (res) {
        clearSlotBit(slot);
    }
    return res;
}

template <class BASE, address_t CLUSTER_SIZE>
bool SlotNVM<BASE, CLUSTER_SIZE>::clearCluster(uint8_t cluster) {
    address_t c_addr = cluster * CLUSTER_SIZE;
    if (this->write(c_addr, 0x00)) {
        clearClusterBit(cluster);
        return true;
    } else {
        return false;
    }
}

template <class BASE, address_t CLUSTER_SIZE>
bool SlotNVM<BASE, CLUSTER_SIZE>::clearClusters(uint8_t firstCluster) {
    address_t cAddr = firstCluster * CLUSTER_SIZE;
    bool res = this->write(cAddr, 0x00);
    if (!res) return false;
    clearClusterBit(firstCluster);

    uint8_t maxDeep = 256 / S_USER_DATA_PER_CLUSTER;
    uint8_t flags;
    do {
        res = this->read(cAddr + 1, flags);             // read flags
        if (!res) break; // it's enough that the first cluster became invalid
        flags &= S_LAST_CLUSTER_FLAG;
        if (flags == 0x00) {
            res = this->read(cAddr + 2, firstCluster);  // read next cluster
            if (!res) break;
            cAddr = firstCluster * CLUSTER_SIZE;
            res = this->write(cAddr, 0x00);
            if (!res) break;
            clearClusterBit(firstCluster);
        }
        --maxDeep;
    } while ((flags == 0x00) && (maxDeep > 0));

    return true;
}

template <class BASE, address_t CLUSTER_SIZE>
bool SlotNVM<BASE, CLUSTER_SIZE>::findStartCluser(uint8_t slot, uint8_t &startCluster) {
    for (uint16_t cluster = 0; cluster < S_CLUSTER_CNT; ++cluster) {
        if (!isClusterBitSet(cluster)) continue;                    // skip unused
        address_t c_addr = cluster * CLUSTER_SIZE;
        uint8_t d;

        bool res = this->read(c_addr, d);                           // read slot no.
        if (!res) return false;
        if (d != slot) continue;                                    // skip other slots

        // we don't need to check end byte again, they are already marked as unused in begin()

        res = this->read(c_addr + 1, d);                            // read flags
        if (!res) return false;
        if ((d & S_START_CLUSTER_FLAG) != 0) {                      // start cluster found
            startCluster = cluster;
            return true;
        }
    }

    return false;
}

template <class BASE, address_t CLUSTER_SIZE>
bool SlotNVM<BASE, CLUSTER_SIZE>::nextFreeCluster(uint8_t &nextCluster) {
    if (nextCluster > S_CLUSTER_CNT) nextCluster = S_CLUSTER_CNT;
    uint16_t startCluster = nextCluster;
    ++nextCluster;
    while (nextCluster != startCluster) {
        if (nextCluster >= S_CLUSTER_CNT) nextCluster = 0;
        if (isClusterBitSet(nextCluster)) {
            ++nextCluster;
        } else {
            return true;
        }
    }

    return false;
}

#endif // _SLOTNVM_SLOTNVM_H_