# ChirpLoRaWAN

Collection of Arduino Sketches and node.js scripts to support using a Chirp! capacitive soil moisture sensor - which works great! - to The Things Network of LoRaWAN devices. 

Chirp is open source hardware and software and quite affordable at $15 each and it works great! The ATTINY44A can be reprogrammed with a minimal firmware that does not try to read light levels, make the buzzer chirp, etc. In fact, I strip off the coin battery holder and buzzer for use with IOT soil moisture readings. 

We use an Arduino Pro Mini to interface via i2c with the Chirp! to take readings, then via SPI to a RFM95W LoRa module to transmit the reading to a nearby LoRaWAN Gateway and on to the things network.   A node.js script watches the mqtt queue for TTN devices and collects the data in a mongodb. 

Chirp!
https://wemakethings.net/chirp/
