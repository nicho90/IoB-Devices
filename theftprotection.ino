/**
Arduino GPS Client to retrieve the Location data from the GPS shield.
The information are processed through the TinyGPS library and send via the Akeru
**/

#include <SoftwareSerial.h>
#include <Akeru.h>
#include <TinyGPS.h>

TinyGPS gps;
// Akeru uses RX=5, TX=4
SoftwareSerial ssAkeru(5,4);
SoftwareSerial ss(6,7);

void setup()
{  
  Serial.begin(9600);
  Serial.print("Finished setup");
}

typedef struct {
  bool theftprotection;
  float lat;
  float lng;
} Payload;

void loop()
{ 
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;
	Payload p;
 
  // start gps listening
  ss.begin(9600);
  
  // parse GPS data for one second and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      //Serial.print(c); //uncomment to see the full NMEA datasets
      if (gps.encode(c)) // Did a new valid sentence come in?
        Serial.println("New Data!");
        newData = true;
    }
  }
  // end gps in order to make port listening for akeru available
  ss.end();
    
    if (newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    Serial.println("");      
    Serial.print("LAT = ");  
    Serial.println(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print("LON = ");
    Serial.println(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    newData = false;

    p.theftprotection = true;
    p.lat = flat;
    p.lng = flon;

    // Init modem
    Akeru.begin();
    // Wait 1 second for the modem to warm up
    delay(1000);
    
    if(!Akeru.isReady()){
      Serial.println("Modem not ready");
      delay(1000);
    } else if(
      p.lat != TinyGPS::GPS_INVALID_F_ANGLE &&
      p.lng != TinyGPS::GPS_INVALID_F_ANGLE) {
      Serial.println("Modem ready and coordinates valid");
      delay(1000);
      
      Akeru.send(&p, sizeof(p));
      Serial.println("Message sent");
      delay(1000); 
    } else {
      Serial.println("Coordinates not valid");
    }
    // end modem in order to make port listening for gps available
    ssAkeru.end();
  }
}
