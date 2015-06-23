/**
Arduino GPS Client to retrieve the Location data from the GPS shield.
The information are processed through the TinyGPS library and send via the Akeru
**/

#include <SoftwareSerial.h>
#include <Akeru.h>
#include <TinyGPS.h>
#include <dht.h>

// Pin analog 0 for temp/humidity sensor
#define dht_dpin A0
dht DHT;

TinyGPS gps;
// Akeru uses RX=5, TX=4
SoftwareSerial ssAkeru(5,4);
SoftwareSerial ss(6,7);

void setup()
{
  Serial.begin(9600);
  Serial.print("Finished setup\n");
}

typedef struct {
  bool theftprotection;
  float lat;
  float lng;
  int temperature;
} Payload;

void loop()
{
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;
	Payload p;

 ss.begin(9600);

  // PARSE GPS data for one second and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      //Serial.print(c); //uncomment to see the full NMEA datasets
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }
  // end gps in order to make port listening for akeru available
  ss.end();

  if (newData)
  {
    // IF NEW GPS-DATA
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    Serial.println("");
    Serial.print("LAT = ");
    Serial.println(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print("LON = ");
    Serial.println(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    newData = false;

    DHT.read11(dht_dpin);
    Serial.print("Humidity = ");
    Serial.print(DHT.humidity);
    Serial.print("%  ");
    Serial.print("temperature = ");
    Serial.print(DHT.temperature);
    Serial.println("C  ");

    p.theftprotection = true;
    p.lat = flat;
    p.lng = flon;
    p.temperature = (int)DHT.temperature;


    // INIT MODEM
    Akeru.begin();
    // Wait 1 second for the modem to warm up
    delay(1000);

    if(!Akeru.isReady()) {
      Serial.println("Modem not ready");
    } else {
      Serial.println("Modem ready");

      // CHECK IF LNG/LAT != 0
      if(p.lat == 0 || p.lng == 0) {
        Serial.println("Unknown position");
      } else {
        Serial.println("Message-Size: ");
        Serial.println(sizeof(p));
        Akeru.send(&p, sizeof(p));
        Serial.println("Message sent!");
        delay(1000);
      }

    }
    // end modem in order to make port listening for gps available
    ssAkeru.end();
  } else {

    Serial.print("Theft-Protection: 1\n");
    Serial.print("No GPS data\n");
    Serial.print("LAT = 0.00000\n");
    Serial.print("LON = 0.00000\n");
    DHT.read11(dht_dpin);
    Serial.print("Humidity = ");
    Serial.print(DHT.humidity);
    Serial.print("%  ");
    Serial.print("temperature = ");
    Serial.print(DHT.temperature);
    Serial.println("C\n");

    delay(1000);
  }
}
