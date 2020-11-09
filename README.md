# SlotNVM

I was looking for a way to store different data with different size to EEPROM where the size is only known at runtime.
Also data should not get lost if a write access was interrupted.
Most libraries I found can handle only compile time fixed data size or writes are not transactional or are not suitable for Arduino.
This library should implement this as an Arduino library.

Two possible solutions come to my mind.
The first is a block based organization. There is a lot of extra data needed and if you don't need a full block
some bytes could remain unused. In contrast it is robust. If one block is defect all other could still be OK.
An other solution is a heap implementation. There is less overhead and no unused bytes. But such an implementation is a little bit more difficult
and if one structure is defect all data behind this is lost or at least not allocateable.

This repository implement a block based EEPROM structure. Later also Flash access could added, therefore I will write about NVM from now.
The NVM is split into blocks of equal size, called cluster. You do not access this cluster directly but store your data in virtual slots.
Such a slot is indicate by a number and can be placed in any of the cluster or also in more than one.

## Features

Currently implemented:

* Support for Arduino buildin EEPROM
* Extendable to other EEPROM using own access class
* Transactional write
* Possibility to reserve some free cluster to ensure that data can always safely be rewritten.
* Low RAM usage
* Up to 64KiByte EEPROM (256 cluser with 256 bytes each)
* Up to 250 slots
* Possibility to reduce maximum slots to reduce RAM usage
* Use you own 8 bit CRC function (no xor in/out)
* Possibility to disable CRC for more available user data.

Currently not implemented:

* Flash memory
* Wear leveling
* CRC xor in/out (and will never implement?)
* Non transactional write as optional fallback

## Usage

Especially for Arduino with integrated EEPROM you can use one the following classes.

* `SlotNVM16noCRC<>`
* `SlotNVM32noCRC<>`
* `SlotNVM64noCRC<>`
* `SlotNVM16CRC<>`
* `SlotNVM32CRC<>`
* `SlotNVM64CRC<>`

Excample code:

    // include the header
    #include <SlotNVM.h>
    
    // create an instance
    SlotNVM16CRC<> slotNVM;
    
    int starts = 1;
    
    void setup() {
      Serial.begin(115200);
      
      // call begin() once
      slotNVM.begin();

      // now you can use readSlot() and writeSlot()
      if (slotNVM.readSlot(1, starts)) {
        Serial.print(F("This is start no. "));
        Serial.println(starts);
      } else {
        Serial.println(F("This is the first start"));
      }

      if (starts < 3) {
        ++starts;
        slotNVM.writeSlot(1, starts);
      }
    }
    
    void loop() {
    }


| SlotNVM class    | CRC | Bytes / cluster | User data / cluster  | Usable data / % |
| ---------------- | --- | ---------------:| --------------------:| ---------------:|
| SlotNVM16noCRC<> | no  |              16 |                   11 |           68,8% |
| SlotNVM32noCRC<> | no  |              32 |                   27 |           84,4% |
| SlotNVM64noCRC<> | no  |              64 |                   59 |           92,2% |
| SlotNVM16CRC<>   | yes |              16 |                   10 |           62,5% |
| SlotNVM32CRC<>   | yes |              32 |                   26 |           81,3% |
| SlotNVM64CRC<>   | yes |              64 |                   58 |           90,6% |

Arduino Nano Every 256 bytes EEPROM

| SlotNVM class    | Cluster | Slots | Usable size / bytes | RAM usage / byte |
| ---------------- | -------:| -----:| -------------------:| ----------------:|
| SlotNVM16noCRC<> |      16 |    16 |                 176 |                5 |
| SlotNVM32noCRC<> |       8 |     8 |                 216 |                3 |
| SlotNVM64noCRC<> |       4 |     4 |                 236 |                3 |
| SlotNVM16CRC<>   |      16 |    16 |                 160 |                5 |
| SlotNVM32CRC<>   |       8 |     8 |                 208 |                3 |
| SlotNVM64CRC<>   |       4 |     4 |                 232 |                3 |

Arduino Uno / Genuino, Nano, Leonardo, Micro with 1024 bytes EEPROM

| SlotNVM class    | Cluster | Slots | Usable size / bytes | RAM usage / byte |
| ---------------- | -------:| -----:| -------------------:| ----------------:|
| SlotNVM16noCRC<> |      64 |    64 |                 704 |               17 |
| SlotNVM32noCRC<> |      32 |    32 |                 864 |                9 |
| SlotNVM64noCRC<> |      16 |    16 |                 944 |                5 |
| SlotNVM16CRC<>   |      64 |    64 |                 640 |               17 |
| SlotNVM32CRC<>   |      32 |    32 |                 832 |                9 |
| SlotNVM64CRC<>   |      16 |    16 |                 928 |                5 |

Arduino Mega with 4096 bytes EEPROM

| SlotNVM class    | Cluster | Slots | Usable size / bytes | RAM usage / byte |
| ---------------- | -------:| -----:| -------------------:| ----------------:|
| SlotNVM16noCRC<> |     256 |   250 |                2816 |               65 |
| SlotNVM32noCRC<> |     128 |   128 |                3456 |               33 |
| SlotNVM64noCRC<> |      64 |    64 |                3776 |               17 |
| SlotNVM16CRC<>   |     256 |   250 |                2560 |               65 |
| SlotNVM32CRC<>   |     128 |   128 |                3328 |               33 |
| SlotNVM64CRC<>   |      64 |    64 |                3712 |               17 |

## Install

Just download the code as zip file. In GitHub click on the `[Code]`-button and select `Download ZIP`.

In Arduino IDE select `Sketch` -> `Include library` -> `Add ZIP Library ...` to add the downloaded ZIP file.

## License

SlotNVM is distributed under the [MIT License](LICENSE).
