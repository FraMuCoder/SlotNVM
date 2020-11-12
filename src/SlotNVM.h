/*
 * SlotNVM
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _SLOTNVM_SLOTNVM_H_
#define _SLOTNVM_SLOTNVM_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "NVMBase.h"

#ifdef __AVR_ARCH__
  #define _SLOTNVM_FLASHMEM_ PROGMEM

  #include <util/crc16.h>

  #ifdef E2END /* EEPROM available */
    #include "ArduinoEEPROM.h"
  #endif
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
 *  3       In first cluster the size of user data,
 *          In other cluster used bytes in this cluster (for CRC calc)
 *
 *  4..n-3  User data
 *
 *  n-2     CRC-8 if CRC_FUNC is not NULL else also user data
 *  n-1     End byte must be 0xA0 for SlotNVM without CRC
 *                           0xA1 for SlotNVM with CRC.
 *          Other values make this cluster invalid.
 *          The value might change with incompatible structure changes.
 */


/**
 * @tparam BASE             Base class handling NVM read and write, see NVMBase as example.
 *                          This class also specifies the size of the NVM via member S_SIZE.
 * @tparam CLUSTER_SIZE     Size of a cluster in byte.
 *                          This should not be lower than 7. Typical values are 16, 32, 64, 128, 256.
 * @tparam PROVISION        Bytes that must always be free to ensure that data can safely be rewritten.
 *                          If your slot data do not exceed this limit is can always be rewritten without deleting other data before.
 *                          This value is rounded to next multiple of user data of a cluster.
 * @tparam LAST_SLOT        Number of last usable slot.
 *                          Maximum value is 250.
 *                          0 mean number is equal to count of available cluster.
 *                          The result is stored in S_LAST_SLOT 
 * @tparam CRC_FUNC         Function to calculate a 8 bit CRC.
 *                          Prototype must be: uint8_t CRC8_function(uint8_t crc, uint8_t data).
 *                          NULL means not CRC is stored in NVM and therefore one extra data byte per cluster is available.
 * @tparam RND_TYPE         Return type of RND_FUNC.
 * @tparam RND_FUNC         Random function for wear leveling. Default is rand() from stdlib.h.
 *                          Please do not forget to call srand().
 */
template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION = 0, uint8_t LAST_SLOT = 0,
          uint8_t (*CRC_FUNC)(uint8_t crc, uint8_t data) = (uint8_t (*)(uint8_t, uint8_t))NULL,
          typename RND_TYPE = int, RND_TYPE (*RND_FUNC)() = &rand>
class SlotNVM : private BASE {
    static_assert(CLUSTER_SIZE <= 256, "CLUSTER_SIZE must be less or equal to 256.");
    static_assert(LAST_SLOT <= 250, "LAST_SLOT must be less or equal to 250.");

public:
    /// Count of clusters.
    static const uint16_t S_CLUSTER_CNT = BASE::S_SIZE / CLUSTER_SIZE;
    /// Max size of user data in one cluster.
    static const uint8_t S_USER_DATA_PER_CLUSTER = CLUSTER_SIZE - 6 + ((CRC_FUNC == NULL) ? 1 : 0);
    /// Count of reserved user bytes for overwriting slots.
    static const uint16_t S_PROVISION = ((PROVISION + S_USER_DATA_PER_CLUSTER - 1) / S_USER_DATA_PER_CLUSTER) * S_USER_DATA_PER_CLUSTER;
    /// First allowed slot number.
    static const uint8_t S_FIRST_SLOT = 1;
    /// Last allowed slot number.
    static const uint8_t S_LAST_SLOT = LAST_SLOT == 0 ? (S_CLUSTER_CNT > 250 ? 250 : S_CLUSTER_CNT) : (LAST_SLOT > 250 ? 250 : LAST_SLOT);
private:
    static const uint8_t S_END_BYTE = 0xA0 + ((CRC_FUNC == NULL) ? 0 : 1);
    static const uint8_t S_AGE_MASK = 0xC0;
    static const uint8_t S_AGE_SHIFT = 6;
    static const uint8_t S_START_CLUSTER_FLAG = 0x20;
    static const uint8_t S_LAST_CLUSTER_FLAG = 0x10;
    static const uint8_t S_AGE_BITS_TO_OLDEST[];

    static_assert(S_CLUSTER_CNT <= 256, "Max. 256 cluster supported, please increase CLUSTER_SIZE.");
    static_assert((2*PROVISION) <= (S_USER_DATA_PER_CLUSTER*S_CLUSTER_CNT), "PROVISION must be less or equal to the half of available user data.");    

public:
    SlotNVM();

    /**
     * Initialize SlotNVM.
     * Call this once before every other call to SlotNVM. This will check current data of NVM and fix wrong data if found some.
     * @return  true if NVM data is readable and data structure is OK or fixed.             \n
     *          false if NVM data is not readable or data structure is corrupt and can not be fixed or begin() is called twice.
     */
    bool begin();

    /**
     * Check if begin is called before and returns true.
     * @return  true if SlotNVM is ready for use.
     */
    bool isValid() const {
        return m_initDone;
    }

    /**
     * Check if data is stored for a given slot.
     * @param slot  Slot number to check,
     * @return      true if there is data for this slot.
     */
    bool isSlotAvailable(uint8_t slot) const {
        return isSlotBitSet(slot);
    }

    /**
     * Write data.
     * @param slot  Slot number
     * @param data  Data to write
     * @param len   Lenght of data to write
     * @return      true on success else false
     */
    bool writeSlot(uint8_t slot, const uint8_t *data, nvm_size_t len);

    /**
     * Write data.
     * @param slot  Slot number
     * @param data  Data to write
     * @return      true on success else false
     */
    template <class T>
    bool writeSlot(uint8_t slot, T &data) {
      return writeSlot(slot, (const uint8_t *)&data, sizeof(T));
    }

    /**
     * Read data
     * If buffer is to small, nothing is read but len is set to needed size and false is returned.
     * So you can set data to NULL and len to 0 to read data length of a slot.
     * @param           slot    Slot number
     * @param           data    Buffer to read in
     * @param[in,out]   len     in:  Size of data buffer                            \n
     *                          out: On success count of bytes copied to data.
     *                               If data buffer was to small the size of slot
     *                               else value is not changed.
     * @return      true on success else false
     */
    bool readSlot(uint8_t slot, uint8_t *data, nvm_size_t &len) const;
    
    /**
     * Read data
     * If slot stores more or less byte than size of data false is returned and data untouched.
     * @param           slot    Slot number
     * @param           data    Data to read in
     * @return      true on success else false
     */
    template <class T>
    bool readSlot(uint8_t slot, T &data) const {
      nvm_size_t len = 0;
      readSlot(slot, NULL, len);
      if (len == sizeof(T)) {
        return readSlot(slot, (uint8_t *)&data, len);
      } else {
        return false;
      }
    }

    /**
     * Delete slot data.
     * @param slot  Slot number
     * @return      true on success else false
     */
    bool eraseSlot(uint8_t slot);

    /**
     * Get amount of total available user data.
     * Some of this might be reserved as provision for safe overwriting.
     * @return  Total size in bytes
     */
    nvm_size_t getSize() const {
        return S_CLUSTER_CNT * S_USER_DATA_PER_CLUSTER;
    }

    /**
     * Get amount of total usable user data.
     * @return  Usable data size in bytes
     */
    nvm_size_t getUsableSize() const {
        return S_CLUSTER_CNT * S_USER_DATA_PER_CLUSTER - S_PROVISION;
    }

    /**
     * Get amount of still writable user data.
     * @return  Free bytes
     */
    nvm_size_t getFree() const;

private:
    bool    m_initDone;
    uint8_t m_slotAvail[(S_LAST_SLOT + 7) / 8];
    uint8_t m_usedCluster[S_CLUSTER_CNT / 8];

    inline static void setClusterBit(uint8_t usedCluster[S_CLUSTER_CNT / 8], uint8_t cluster) {
        usedCluster[cluster / 8] |= 1 << (cluster % 8);
    }

    inline void setClusterBit(uint8_t cluster) {
        setClusterBit(m_usedCluster, cluster);
    }

    inline static void clearClusterBit(uint8_t usedCluster[S_CLUSTER_CNT / 8], uint8_t cluster) {
        usedCluster[cluster / 8] &= ~(1 << (cluster % 8));
    }

    inline void clearClusterBit(uint8_t cluster) {
        clearClusterBit(m_usedCluster, cluster);
    }

    inline static bool isClusterBitSet(const uint8_t usedCluster[S_CLUSTER_CNT / 8], uint8_t cluster) {
        return (usedCluster[cluster / 8] & (1 << (cluster % 8))) != 0;
    }

    inline bool isClusterBitSet(uint8_t cluster) const {
        return isClusterBitSet(m_usedCluster, cluster);
    }

    inline static void setSlotBit(uint8_t slotAvail[(S_LAST_SLOT + 7) / 8], uint8_t slot) {
        if ((slot >= S_FIRST_SLOT) && (slot <= S_LAST_SLOT)) {
            slot -= S_FIRST_SLOT;
            slotAvail[slot / 8] |= 1 << (slot % 8);
        }
    }

    inline void setSlotBit(uint8_t slot) {
        setSlotBit(m_slotAvail, slot);
    }

    inline static void clearSlotBit(uint8_t slotAvail[(S_LAST_SLOT + 7) / 8], uint8_t slot) {
        if ((slot >= S_FIRST_SLOT) && (slot <= S_LAST_SLOT)) {
            slot -= S_FIRST_SLOT;
            slotAvail[slot / 8] &= ~(1 << (slot % 8));
        }
    }

    inline void clearSlotBit(uint8_t slot) {
        clearSlotBit(m_slotAvail, slot);
    }

    inline static bool isSlotBitSet(const uint8_t slotAvail[(S_LAST_SLOT + 7) / 8], uint8_t slot) {
        if ((slot >= S_FIRST_SLOT) && (slot <= S_LAST_SLOT)) {
            slot -= S_FIRST_SLOT;
            return (slotAvail[slot / 8] & (1 << (slot % 8))) != 0;
        } else {
            return false;
        }
    }

    inline bool isSlotBitSet(uint8_t slot) const {
        return isSlotBitSet(m_slotAvail, slot);
    }

    bool clearCluster(uint8_t cluster);

    bool clearClusters(uint8_t firstCluster);

    bool findStartCluser(uint8_t slot, uint8_t &startCluster) const;

    bool nextFreeCluster(uint8_t &nextCluster) const;

    inline static uint8_t crc_buf(uint8_t crc, const uint8_t *data, uint8_t len) {
        for (uint8_t i = 0; i < len; ++i) {
            crc = CRC_FUNC(crc, data[i]);
        }
        return crc;
    }
};

#if defined(__AVR_ARCH__) && defined(E2END)
  /**
   * SlotNVM with 16 bytes per cluster and no CRC.
   * 11 bytes per cluster or 68,8% can be used for user data.
   * Random function for wear leveling is random() so do not forget to call srandom() or randomSeed().
   * 
   * @tparam PROVISION        Bytes that must always be free to ensure that data can safely be rewritten.
   *                          If your slot data do not exceed this limit is can always be rewritten without deleting other data before.
   *                          This value is rounded to next multiple of user data of a cluster.
   * @tparam LAST_SLOT        Number of last usable slot.
   *                          Maximum value is 250.
   *                          0 mean number is equal to count of available cluster.
   */
  template <nvm_size_t PROVISION = 0, uint8_t LAST_SLOT = 0>
  class SlotNVM16noCRC : public SlotNVM<ArduinoEEPROM<>, 16, PROVISION, LAST_SLOT, (uint8_t (*)(uint8_t, uint8_t))NULL, long, &random> {};

  /**
   * SlotNVM with 32 bytes per cluster and no CRC.
   * 27 bytes per cluster or 84,4% can be used for user data.
   * Random function for wear leveling is random() so do not forget to call srandom() or randomSeed().
   * 
   * @tparam PROVISION        Bytes that must always be free to ensure that data can safely be rewritten.
   *                          If your slot data do not exceed this limit is can always be rewritten without deleting other data before.
   *                          This value is rounded to next multiple of user data of a cluster.
   * @tparam LAST_SLOT        Number of last usable slot.
   *                          Maximum value is 250.
   *                          0 mean number is equal to count of available cluster.
   */
  template <nvm_size_t PROVISION = 0, uint8_t LAST_SLOT = 0>
  class SlotNVM32noCRC : public SlotNVM<ArduinoEEPROM<>, 32, PROVISION, LAST_SLOT, (uint8_t (*)(uint8_t, uint8_t))NULL, long, &random> {};

  /**
   * SlotNVM with 64 bytes per cluster and no CRC.
   * 59 bytes per cluster or 92,2% can be used for user data.
   * Random function for wear leveling is random() so do not forget to call srandom() or randomSeed().
   * 
   * @tparam PROVISION        Bytes that must always be free to ensure that data can safely be rewritten.
   *                          If your slot data do not exceed this limit is can always be rewritten without deleting other data before.
   *                          This value is rounded to next multiple of user data of a cluster.
   * @tparam LAST_SLOT        Number of last usable slot.
   *                          Maximum value is 250.
   *                          0 mean number is equal to count of available cluster.
   */
  template <nvm_size_t PROVISION = 0, uint8_t LAST_SLOT = 0>
  class SlotNVM64noCRC : public SlotNVM<ArduinoEEPROM<>, 64, PROVISION, LAST_SLOT, (uint8_t (*)(uint8_t, uint8_t))NULL, long, &random> {};

  /**
   * SlotNVM with 16 bytes per cluster protected with CRC.
   * 10 bytes per cluster or 62,5% can be used for user data.
   * Random function for wear leveling is random() so do not forget to call srandom() or randomSeed().
   * 
   * @tparam PROVISION        Bytes that must always be free to ensure that data can safely be rewritten.
   *                          If your slot data do not exceed this limit is can always be rewritten without deleting other data before.
   *                          This value is rounded to next multiple of user data of a cluster.
   * @tparam LAST_SLOT        Number of last usable slot.
   *                          Maximum value is 250.
   *                          0 mean number is equal to count of available cluster.
   */
  template <nvm_size_t PROVISION = 0, uint8_t LAST_SLOT = 0>
  class SlotNVM16CRC : public SlotNVM<ArduinoEEPROM<>, 16, PROVISION, LAST_SLOT, &_crc8_ccitt_update, long, &random> {};

  /**
   * SlotNVM with 32 bytes per cluster protected with CRC.
   * 26 bytes per cluster or 81,3% can be used for user data.
   * Random function for wear leveling is random() so do not forget to call srandom() or randomSeed().
   * 
   * @tparam PROVISION        Bytes that must always be free to ensure that data can safely be rewritten.
   *                          If your slot data do not exceed this limit is can always be rewritten without deleting other data before.
   *                          This value is rounded to next multiple of user data of a cluster.
   * @tparam LAST_SLOT        Number of last usable slot.
   *                          Maximum value is 250.
   *                          0 mean number is equal to count of available cluster.
   */
  template <nvm_size_t PROVISION = 0, uint8_t LAST_SLOT = 0>
  class SlotNVM32CRC : public SlotNVM<ArduinoEEPROM<>, 32, PROVISION, LAST_SLOT, &_crc8_ccitt_update, long, &random> {};

  /**
   * SlotNVM with 64 bytes per cluster protected with CRC.
   * 58 bytes per cluster or 90,6% can be used for user data.
   * Random function for wear leveling is random() so do not forget to call srandom() or randomSeed().
   * 
   * @tparam PROVISION        Bytes that must always be free to ensure that data can safely be rewritten.
   *                          If your slot data do not exceed this limit is can always be rewritten without deleting other data before.
   *                          This value is rounded to next multiple of user data of a cluster.
   * @tparam LAST_SLOT        Number of last usable slot.
   *                          Maximum value is 250.
   *                          0 mean number is equal to count of available cluster.
   */
  template <nvm_size_t PROVISION = 0, uint8_t LAST_SLOT = 0>
  class SlotNVM64CRC : public SlotNVM<ArduinoEEPROM<>, 64, PROVISION, LAST_SLOT, &_crc8_ccitt_update, long, &random> {};
#endif

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
const uint8_t SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::S_AGE_BITS_TO_OLDEST[] _SLOTNVM_FLASHMEM_ = {
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

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::SlotNVM()
    : m_initDone(false)
    , m_slotAvail{0}
    , m_usedCluster{0}
{
    // ToDo
}

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
bool SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::begin() {
    if (m_initDone) return false;

    // first check used cluster and available slots
    for (uint16_t cluster = 0; cluster < S_CLUSTER_CNT; ++cluster) {
        nvm_address_t cAddr = cluster * CLUSTER_SIZE;
        uint8_t slot;
        uint8_t d;
        uint8_t crc = 0;

        bool res = this->read(cAddr, slot);                            // read slot no.
        if (!res) return false;
        if ((slot < S_FIRST_SLOT) || (slot > S_LAST_SLOT)) continue;    // skip unused
        if (CRC_FUNC != NULL) {
            crc = CRC_FUNC(crc, slot);
        }

        res = this->read(cAddr + CLUSTER_SIZE - 1, d);                 // read end byte
        if (!res) return false;
        if (S_END_BYTE != d) continue;                                  // skip incomplete written

        if (CRC_FUNC != NULL) {
            res = this->read(cAddr + 1, d);                            // read flags
            if (!res) return false;
            crc = CRC_FUNC(crc, d);
            bool isFirst = (d & S_START_CLUSTER_FLAG) != 0;

            res = this->read(cAddr + 2, d);                            // read next cluster
            if (!res) return false;
            crc = CRC_FUNC(crc, d);

            res = this->read(cAddr + 3, d);                            // read length
            if (!res) return false;
            crc = CRC_FUNC(crc, d);

            uint16_t len = d;
            if (isFirst) {
                ++len;
                if (len > S_USER_DATA_PER_CLUSTER) {
                    len = S_USER_DATA_PER_CLUSTER;
                }
            } else {
                if (len > S_USER_DATA_PER_CLUSTER) {
                    continue;                                           // skip invalid len data
                }
            }

            for (uint8_t i = 4; i < (4 + len); ++i) {                   // read user data
                res = this->read(cAddr + i, d);
                if (!res) return false;
                crc = CRC_FUNC(crc, d);
            }
            res = this->read(cAddr + CLUSTER_SIZE - 2, d);             // read CRC
            if (!res) return false;
            if (d != crc) continue;                                     // skip invalid CRC
        }

        // we have found a valid cluster
        setClusterBit(cluster);
        setSlotBit(slot);
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
            nvm_address_t cAddr = cluster * CLUSTER_SIZE;
            uint8_t d;

            bool res = this->read(cAddr, d);                           // read slot no.
            if (!res) return false;
            if (d != slot) continue;                                    // skip other slots

            // we don't need to check end byte again, they are already marked as unused in last check

            // remember all cluster so we can delete old and defective
            setClusterBit(clusterUsedBySlot, cluster);

            res = this->read(cAddr + 1, d);                            // read flags
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
            nvm_address_t cAddr = startCluster * CLUSTER_SIZE;
            uint8_t flags, startLen, curLen;
            uint8_t res = this->read(cAddr + 1, flags);                // read flags
            if (!res) return false;
            res = this->read(cAddr + 3, startLen);                     // read length
            if (!res) return false;
            uint16_t doNotExceetLen = startLen + 1 + S_USER_DATA_PER_CLUSTER; // ToDo dynamic min length
            uint16_t curMaxDataLen = S_USER_DATA_PER_CLUSTER; // should not exceed realLen + S_USER_DATA_PER_CLUSTER

            bool err = false;
            uint8_t curCluster = startCluster;
            while (!err && ((flags & S_LAST_CLUSTER_FLAG) == 0)) {
                res = this->read(cAddr + 2, curCluster);               // read next cluster number
                setClusterBit(validCluster, curCluster);
                if (!res) return false;
                if (isClusterBitSet(clusterUsedBySlot, curCluster)) {   // next cluster belong to this slot
                    cAddr = curCluster * CLUSTER_SIZE;                 // next address
                    res = this->read(cAddr + 1, flags);                // read flags
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

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
bool SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::writeSlot(uint8_t slot, const uint8_t *data, nvm_size_t len) {
    if (!m_initDone) return false;
    if (data == NULL) return false;
    if (len < 1) return false;
    if (len > 256) return false;
    if ((slot < S_FIRST_SLOT) || (slot > S_LAST_SLOT)) return false;
    uint8_t oldStartCluster;
    nvm_address_t cAddr;
    uint8_t d[4];
    uint8_t newAge = 0;
    bool res;
    bool overwrite = findStartCluser(slot, oldStartCluster);
    nvm_size_t free = getFree();

    if (overwrite) {
        cAddr = oldStartCluster * CLUSTER_SIZE;
        res = this->read(cAddr + 1, d[0]);                      // read old age
        if (!res) return false;
        newAge = (d[0] & S_AGE_MASK) >> S_AGE_SHIFT;
        newAge = ((newAge + 1) << S_AGE_SHIFT) & S_AGE_MASK;

        res = this->read(cAddr + 3, d[0]);                      // read old length
        if (!res) return false;
        nvm_size_t extraFree = ((d[0] + S_USER_DATA_PER_CLUSTER - 1) / S_USER_DATA_PER_CLUSTER) * S_USER_DATA_PER_CLUSTER;
        if (extraFree > S_PROVISION) {
            free += S_PROVISION;
        } else {
            free += extraFree;
        }
    }

    if (free < len) return false;

    const uint8_t cntCluster = (len - 1) / S_USER_DATA_PER_CLUSTER + 1;
    uint8_t newCluster[cntCluster];
    uint8_t nextCluster = 255;
    if (RND_FUNC != NULL) {
        nextCluster = RND_FUNC() % S_CLUSTER_CNT;
    }
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

        // calc length of user data in this cluser
        uint16_t offset = i * S_USER_DATA_PER_CLUSTER;
        nvm_size_t toCopy = len - offset;
        if (toCopy > S_USER_DATA_PER_CLUSTER) {
            toCopy = S_USER_DATA_PER_CLUSTER;
        }

        // write the header
        d[0] = slot;
        d[1] = newAge
             | ((i == 0) ? S_START_CLUSTER_FLAG : 0x00) 
             | ((i == (cntCluster-1)) ? S_LAST_CLUSTER_FLAG : 0x00);
        d[2] = (i == (cntCluster-1)) ? slot : newCluster[i+1];
        d[3] = (i == 0) ? len - 1 : toCopy;
        res = this->write(cAddr, d, 4);
        if (!res) return false;
        uint8_t crc;
        if (CRC_FUNC != NULL) {
            crc = crc_buf(0, d, 4);
        }

        // write data
        res = this->write(cAddr + 4, data + offset, toCopy);
        if (!res) return false;

        if (CRC_FUNC != NULL) {
            crc = crc_buf(crc, data + offset, toCopy);

            // write CRC
            res = this->write(cAddr + CLUSTER_SIZE - 2, crc);
            if (!res) return false;
        }

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

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
bool SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::readSlot(uint8_t slot, uint8_t *data, nvm_size_t &len) const {
    if (!m_initDone) return false;

    uint8_t curCluster;
    bool res = findStartCluser(slot, curCluster);
    if (!res) return false;

    nvm_address_t cAddr = curCluster * CLUSTER_SIZE;
    uint8_t d;

    res = this->read(cAddr + 3, d);                 // read length
    if (!res) return false;
    nvm_size_t lenToCopy = d + 1;
    if (lenToCopy > len) {
        len = lenToCopy;
        return false;
    }
    len = lenToCopy;
    if (data == NULL) return false;

    uint8_t flags;
    do {
        res = this->read(cAddr + 1, flags);         // read flags
        if (!res) return false;
        nvm_size_t curCopy = (lenToCopy > S_USER_DATA_PER_CLUSTER) ? S_USER_DATA_PER_CLUSTER : lenToCopy;
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

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
bool SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::eraseSlot(uint8_t slot) {
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

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
bool SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::clearCluster(uint8_t cluster) {
    nvm_address_t cAddr = cluster * CLUSTER_SIZE;
    if (this->write(cAddr, 0x00)) {
        clearClusterBit(cluster);
        return true;
    } else {
        return false;
    }
}

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
bool SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::clearClusters(uint8_t firstCluster) {
    nvm_address_t cAddr = firstCluster * CLUSTER_SIZE;
    bool res = this->write(cAddr, 0x00);
    if (!res) return false;
    clearClusterBit(firstCluster);

    uint8_t maxDeep = uint8_t(256 / S_USER_DATA_PER_CLUSTER);
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

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
nvm_size_t SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::getFree() const {
    nvm_size_t free = getSize();
    for (uint16_t cluster = 0; cluster < S_CLUSTER_CNT; ++cluster) {
        if (isClusterBitSet(cluster)) {
            free -= S_USER_DATA_PER_CLUSTER;
        }
    }
    if (free < S_PROVISION) {
        return 0;
    } else {
        return free - S_PROVISION;
    }
}

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
bool SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::findStartCluser(uint8_t slot, uint8_t &startCluster) const {
    for (uint16_t cluster = 0; cluster < S_CLUSTER_CNT; ++cluster) {
        if (!isClusterBitSet(cluster)) continue;                    // skip unused
        nvm_address_t cAddr = cluster * CLUSTER_SIZE;
        uint8_t d;

        bool res = this->read(cAddr, d);                           // read slot no.
        if (!res) return false;
        if (d != slot) continue;                                    // skip other slots

        // we don't need to check end byte again, they are already marked as unused in begin()

        res = this->read(cAddr + 1, d);                            // read flags
        if (!res) return false;
        if ((d & S_START_CLUSTER_FLAG) != 0) {                      // start cluster found
            startCluster = cluster;
            return true;
        }
    }

    return false;
}

template <class BASE, nvm_size_t CLUSTER_SIZE, nvm_size_t PROVISION, uint8_t LAST_SLOT,
          uint8_t (*CRC_FUNC)(uint8_t, uint8_t), typename RND_TYPE, RND_TYPE (*RND_FUNC)()>
bool SlotNVM<BASE, CLUSTER_SIZE, PROVISION, LAST_SLOT, CRC_FUNC, RND_TYPE, RND_FUNC>::nextFreeCluster(uint8_t &nextCluster) const {
    if (S_CLUSTER_CNT < 256) {
        if (nextCluster > S_CLUSTER_CNT) nextCluster = S_CLUSTER_CNT;
    }
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
