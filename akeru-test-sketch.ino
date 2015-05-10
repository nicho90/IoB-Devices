#include <SoftwareSerial.h>
#include <Akeru.h>
 
// Initialize the modem:
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Serial.println("Starting...");

  // Wait 1 second for the modem to warm up
  delay(1000);

  // Init modem
  Akeru.begin();
  
}

typedef struct {
  char user;
  int currentDate;
  int currentTime;
  int counter;
} Payload;


void loop() { 
  
 Payload p;
 
 p.counter = 0;
 p.user = 'n';
 p.currentDate = 20150510;
 p.currentTime = 1941;
  
   if(!Akeru.isReady()) {
      Serial.println("Modem not ready");
      delay(1000);
    } else {
      Serial.println("Modem ready");
      delay(1000);
      
      if (p.counter==0) {
        // Send data:
        Akeru.send(&p, sizeof(p));
        Serial.println("Message sent");
        delay(1000);
        
        p.counter++;
      
    } else {
      Serial.println("Counter>0");
    }
  }
}
