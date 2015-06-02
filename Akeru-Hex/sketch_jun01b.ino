#include <SoftwareSerial.h>
#include <Akeru.h>

/* 
 * Akeru Libary at: http://snootlab.com/lang-en/shields-snootlab/547-akeru-beta-32-fr.html
 * or at: https://github.com/hackerspacesg/SIGFOX
 */
 
// Initialize the modem:
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Serial.println("Starting...");

  // Wait 1 second for the modem to warm up
  delay(2000);

  // Init modem
  Akeru.begin();
  
}

typedef struct {
  boolean b;
  float lat;
  float lon;
} Payload;


void loop() { 
  
 Payload p;
 
p.b = true;
p.lat = 51.9692082;
p.lon = 7.595995;
  // 51.9692082
  // 7.595995
  
   if(!Akeru.isReady()) {
      Serial.println("Modem not ready");
      delay(1000);
    } else {
      Serial.println("Modem ready");
      delay(1000);
      
        // Send data:
Serial.println(sizeof(p.lat));
  Serial.println(sizeof(p.lon));
      Serial.println(sizeof(p.b));
        Serial.println(sizeof(p));
        Akeru.send(&p, sizeof(p));
        Serial.println("Message sent");
        delay(1000);
   
    }
}
