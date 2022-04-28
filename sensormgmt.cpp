#include <arduino.h>
#include <RTClib.h>

#include <dht.h>

#include "wateringsystem.h"

dht myDHT;

#define dht_apin A0 // Analog Pin DHT sensor 

const float vfactor = 0.16042;

// Reading the Solar charge input voltage
// 0-30 volts, based on a voltage divider that maps 
// 0-30 to 0-5 
float voltageRead() {
  float volts;
  int val = analogRead(A0);
  volts = ((float) val / 1023) * 5.0;
  if(volts < 0.25)
  {
    return 0.0;  
  }
  
  volts = volts/vfactor + 8.2;
  return volts;
}



const float vfactor2 = 0.22765;

// Reading the Solar charge input voltage
// 0-30 volts, based on a voltage divider that maps 
// 0-30 to 0-5 
float solarVoltageRead() {
  float volts;
  int val = analogRead(voltagePin);
  volts = ((float) val / 1023) * 5.0;
    
  volts = volts/vfactor2;
  return volts;
}


float readDHTSensor()
{
  int dht_result = 0; 
  if((dht_result = myDHT.read11(dht_apin)) != 0)
  {
    Serial.print(F("DHT Error: "));
    Serial.print(dht_result);
    Serial.println();

    return 0;
  }

  return  myDHT.temperature;
}
