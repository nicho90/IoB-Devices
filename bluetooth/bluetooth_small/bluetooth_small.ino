//"RBL_nRF8001.h/spi.h/boards.h" is needed in every new project
#include <SPI.h>
#include <EEPROM.h>
#include <boards.h>
#include <RBL_nRF8001.h>

//String message;
void setup()
{
  //  
  // For BLE Shield and Blend:
  //   Default pins set to 9 and 8 for REQN and RDYN
  //   Set your REQN and RDYN here before ble_begin() if you need
  //
  // ble_set_pins(3, 2);
  
  // Set your BLE advertising name here, max. length 10
  //ble_set_name("My BLE");
  
  // Init. and start BLE library.
  ble_begin();
  
  // Enable serial debug
  Serial.begin(9600);
}

unsigned char buf[16] = {0};
unsigned char len = 0;

void loop()
{ 
  if ( ble_connected())
  {
    if (ble_available())
    {
      while ( ble_available())
      {
        switch(ble_read()) {
          case 49:
            Serial.write(ble_read());
            Serial.println();
            Serial.write("Turn Theft-Protection on");
            Serial.println();
            break;
          case 48:
            Serial.write(ble_read());
            Serial.println();
            Serial.write("Turn Theft-Protection off"); 
            Serial.println(); 
            break;
          default:
            Serial.write(ble_read());
            Serial.println();
            Serial.write("Unknown command");   
            Serial.println();
        }        
      }
    }
  }

  ble_do_events();
  delay(1000);  
}

