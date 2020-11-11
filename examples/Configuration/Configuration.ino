#include <SlotNVM.h>

// struct for my configuration
struct configuration {
  char name[10];
  int  age;
};

const char DEF_NAME[] PROGMEM = "Arduino";
const int  DEF_AGE = 42;

configuration myConfig;

// Create an instance
// This SlotNVM reserves some space to always
// allow to rewrite slot data with the size
// up to the same size like configuration.
SlotNVM32CRC<sizeof(configuration)> slotNVM;

const uint8_t CFG_SLOT = slotNVM.S_LAST_SLOT;

void setup() {
  Serial.begin(115200);
  
  // Call begin() once
  slotNVM.begin();
  // Init random generator for better wear leveling
  randomSeed(analogRead(0));

  if (!slotNVM.readSlot(CFG_SLOT, myConfig)) {
    // No configuration stored, use default
    Serial.println(F("Default configuration used"));
    strcpy_P(myConfig.name, DEF_NAME);
    myConfig.age = DEF_AGE;

    // Note: Reservation works only for rewriting
    // a slot not for the first write.
    // If you want to ensure to change this
    // configuration you should write it
    // before there is no space left.
    slotNVM.writeSlot(CFG_SLOT, myConfig);
  } else {
    Serial.println(F("Configuration was loaded"));
  }

  // Use other slots for your business
  for (uint8_t slot = 1; slot < CFG_SLOT; ++slot) {
    if (slotNVM.isSlotAvailable(slot)) {
      // Do something with this slot
      Serial.print(F("Slot "));
      Serial.print(slot);
      Serial.println(F(" has some data."));
    }
  }
}

void loop() {
}
