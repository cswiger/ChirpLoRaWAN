/*******************************************************************************
 * 
 * ver ttn6 - for deep in ground sensor
 * 
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the (early prototype version of) The Things Network.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in g1,
 *  0.1% in g2).
 *
 * Change DEVADDR to a unique address!
 * See http://thethingsnetwork.org/wiki/AddressSpace
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 *******************************************************************************/

/*  
 *  Arduino pro mini with LMiC - uses up 96% of memory!
 */ 

// do this in lmic/src/lmic/config.h file
//#define CFG_us915 1

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>


// registered with:
// # ./ttnctl devices register personalized xxxxxxxx yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
//  INFO Registered personalized device           AppSKey=yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy DevAddr=xxxxxxxx Flags=0 NwkSKey=zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
//
// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const PROGMEM u1_t NWKSKEY[16] = { 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz, 0xzz };

// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially.
static const u1_t PROGMEM APPSKEY[16] = { 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy, 0xyy };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
static const u4_t DEVADDR = 0xxxxxxxxx ; // <-- Change this address for every node 

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

char mydata[4];
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 1800;  // 30 minute

// Pin mapping arduino pro mini in pill box
const lmic_pinmap lmic_pins = {    // original lmic lib
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {3, 6, LMIC_UNUSED_PIN}    // like at http://forum.thethingsnetwork.org/t/node-with-esp8266-and-rfm95w/1002/45
                                    // pin 6 is needed for transmit complete signal back to lmic
};

// for Chirp i2c
void writeI2CRegister8bit(int addr, int value) {
  Wire.beginTransmission(addr);
  Wire.write(value);
  Wire.endTransmission();
}

unsigned int readI2CRegister16bit(int addr, int reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission();
  delay(20);
  Wire.requestFrom(addr, 2);
  unsigned int t = Wire.read() << 8;
  t = t | Wire.read();
  return t;
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if(LMIC.dataLen) {
                // data received in rx slot after tx
                Serial.print(F("Data Received: "));
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        uint16_t val = readI2CRegister16bit(0x20, 0);  // readings range from 242 dry to 495 wet
        memcpy(mydata,&val,sizeof(val));
        LMIC_setTxData2(1, (uint8_t*)mydata, 2, 0);  // 
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    writeI2CRegister8bit(0x20, 6); //reset


    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly 
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif


    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // Set data rate and transmit power (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF10,14);

    // disable ALL channels except the one used by the single channel gateway, chnl 57, 913.7
    // This works now that I discovered you put #define CFG_us915 in lmic/config/h  and not here
    for (u1_t chl=0; chl<64; chl++) {
      if (chl != 57) {
        LMIC_disableChannel(chl);
      }
    }

 
    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}


  

