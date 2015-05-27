/**
Arduino GPS Client to retrieve the Location data from the GPS shield.
The information are processed through the TinyGPS library and logged to the console
**/

#include <SoftwareSerial.h>
#include <TinyGPS.h>

TinyGPS gps;
SoftwareSerial ss(0,1);

void setup()
{
  ss.begin(9600);
  Serial.begin(9600);
  Serial.print("Finished setup");
}

void loop()
{ 
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;
 
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
    Serial.print("HDOP = ");
    Serial.println(gps.hdop()/100.0);
    Serial.print("SAT = ");
    Serial.println(gps.satellites());
    Serial.print("Speed = ");
    Serial.println(gps.f_speed_kmph());
    newData = false;
  }
}
