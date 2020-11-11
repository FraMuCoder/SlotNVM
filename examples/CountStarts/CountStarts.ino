// Include the header
#include <SlotNVM.h>

// Create an instance
SlotNVM16CRC<> slotNVM;

int starts = 1;

void setup() {
  Serial.begin(115200);
  
  // Call begin() once
  slotNVM.begin();
  // Init random generator for better wear leveling
  randomSeed(analogRead(0));

  // Now you can use readSlot() and writeSlot()
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