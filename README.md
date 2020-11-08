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

## Fetures

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

## Install

Just download the code as zip file. In GitHub click on the `[Code]`-button and select `Download ZIP`.

In Arduino IDE select `Sketch` -> `Include library` -> `Add ZIP Library ...` to add the downloaded ZIP file.

## License

SlotNVM is distributed under the [MIT License](LICENSE).
