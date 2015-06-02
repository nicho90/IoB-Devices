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

  // Wait 5 seconds for the modem to warm up
  delay(5000);

  // Init modem
  Akeru.begin();
  
}

typedef struct {
  char sign;
  int counter;
} Payload;


void loop() { 
  
 Payload p;
 
 p.counter = 0;
 p.sign = 'a';
  
   if(!Akeru.isReady()) {
      Serial.println("Modem not ready");
      delay(1000);
    } else {
      Serial.println("Modem ready");
      delay(1000);
      
      if (p.counter==0) {
        // Send data:
        Serial.println(sizeof(p.sign));
        //Serial.println(sizeof(p.lat));
        //Serial.println(sizeof(p.lon));
        Serial.println(sizeof(p));
        Akeru.send(&p, sizeof(p));
        Serial.println("Message sent");
        delay(1000);
        
        p.counter++;
      
    } else {
      Serial.println("Counter>0");
    }
  }
}
