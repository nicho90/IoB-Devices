/**
  Arduino GPS Client to retrieve the Location data from the GPS shield.
  The information are processed through the TinyGPS library and send via the Akeru
*/

#include <SoftwareSerial.h>
#include <Akeru.h>
#include <TinyGPS.h>
#include <DHT.h>

// libraries required for Bluetooth:
#include <SPI.h>
#include <EEPROM.h>
#include <lib_aci.h>
#include <aci_setup.h>
#include "uart_over_ble.h"

/**
  Put the nRF8001 setup in the RAM of the nRF8001.
*/
#include "services.h"
/**
  Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
  This would mean that the setup cannot be changed once put in.
  However this removes the need to do the setup of the nRF8001 on every reset.
*/

#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
    static services_pipe_type_mapping_t
        services_pipe_type_mapping[NUMBER_OF_PIPES] = SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
    #define NUMBER_OF_PIPES 0
    static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif

/* Store the setup for the nRF8001 in the flash of the AVR to save on RAM */
static const hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] PROGMEM = SETUP_MESSAGES_CONTENT;

/**
 aci_struct that will contain:
 - total initial credits
 - current credit
 - current state of the aci (setup/standby/active/sleep)
 - open remote pipe pending
 - close remote pipe pending
 - Current pipe available bitmap
 - Current pipe closed bitmap
 - Current connection interval, slave latency and link supervision timeout
 - Current State of the the GATT client (Service Discovery)
 - Status of the bond (R) Peer address
*/
static struct aci_state_t aci_state;

/*
  Temporary buffers for sending ACI commands
*/
static hal_aci_evt_t  aci_data;
//static hal_aci_data_t aci_cmd;

/*
  Timing change state variable
*/
static bool timing_change_done = false;

// ublox ubx messages to configure the gps receiver
uint8_t gpsoff[] = { 0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x16, 0x74 };
uint8_t gpson[] = { 0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x09, 0x00, 0x17, 0x76 };

/*
  Used to test the UART TX characteristic notification
*/
static uart_over_ble_t uart_over_ble;
static uint8_t         uart_buffer[20];
static uint8_t         uart_buffer_len = 0;
static uint8_t         dummychar = 0;

/*
  Initialize the radio_ack. This is the ack received for every transmitted packet.
*/
//static bool radio_ack_pending = false;

/* Define how assert should function in the BLE library */
void __ble_assert(const char *file, uint16_t line)
{
  Serial.print("ERROR ");
  Serial.print(file);
  Serial.print(": ");
  Serial.print(line);
  Serial.print("\n");
  while(1);
}
/*
  Description:

  In this template we are using the BTLE as a UART and can send and receive packets.
  The maximum size of a packet is 20 bytes.
  When a command it received a response(s) are transmitted back.
  Since the response is done using a Notification the peer must have opened it(subscribed to it) before any packet is transmitted.
  The pipe for the UART_TX becomes available once the peer opens it.
  See section 20.4.1 -> Opening a Transmit pipe
  In the master control panel, clicking Enable Services will open all the pipes on the nRF8001.

  The ACI Evt Data Credit provides the radio level ack of a transmitted packet.
*/

// Pin analog 0 for temp/humidity sensor
#define dht_dpin A0 
#define DHTTYPE DHT11
DHT dht(dht_dpin, DHTTYPE);

bool ftheftprotection = false;

TinyGPS gps;
// TD1208 chip on the Akeru uses RX=5, TX=4
SoftwareSerial ssAkeru(5,4);
SoftwareSerial ss(2,3);
SoftwareSerial ssBluetooth(6,7);

void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Arduino setup"));
  Serial.println(F("Set line ending to newline to send data from the serial monitor"));
  
  if (NULL != services_pipe_type_mapping)
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = &services_pipe_type_mapping[0];
  }
  else
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
  }
  aci_state.aci_setup_info.number_of_pipes    = NUMBER_OF_PIPES;
  aci_state.aci_setup_info.setup_msgs         = (hal_aci_data_t*)setup_msgs;
  aci_state.aci_setup_info.num_setup_msgs     = NB_SETUP_MESSAGES;

  /*
    Tell the ACI library, the MCU to nRF8001 pin connections.
    The Active pin is optional and can be marked UNUSED
  */
  aci_state.aci_pins.board_name = REDBEARLAB_SHIELD_V1_1; // See board.h for details REDBEARLAB_SHIELD_V1_1 or BOARD_DEFAULT
  aci_state.aci_pins.reqn_pin   = 9; // SS for Nordic board, 9 for REDBEARLAB_SHIELD_V1_1
  aci_state.aci_pins.rdyn_pin   = 8; // 3 for Nordic board, 8 for REDBEARLAB_SHIELD_V1_1
  aci_state.aci_pins.mosi_pin   = MOSI;
  aci_state.aci_pins.miso_pin   = MISO;
  aci_state.aci_pins.sck_pin    = SCK;

  aci_state.aci_pins.spi_clock_divider      = SPI_CLOCK_DIV8; // SPI_CLOCK_DIV8  = 2MHz SPI speed
                                                              // SPI_CLOCK_DIV16 = 1MHz SPI speed
  
  aci_state.aci_pins.reset_pin              = UNUSED; // 4 for Nordic board, UNUSED for REDBEARLAB_SHIELD_V1_1
  aci_state.aci_pins.active_pin             = UNUSED;
  aci_state.aci_pins.optional_chip_sel_pin  = UNUSED;

  aci_state.aci_pins.interface_is_interrupt = false; // Interrupts still not available in Chipkit
  aci_state.aci_pins.interrupt_number       = 1;

  // We reset the nRF8001 here by toggling the RESET line connected to the nRF8001
  // If the RESET line is not available we call the ACI Radio Reset to soft reset the nRF8001
  // then we initialize the data structures required to setup the nRF8001
  // The second parameter is for turning debug printing on for the ACI Commands and Events so they be printed on the Serial
  lib_aci_init(&aci_state, false);

  // set the ublox 6 GPS receiver in power saving mode
  ss.begin(9600);
  delay(500);
  uint8_t powersave[] = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92};
  sendUBX(powersave, sizeof(powersave)/sizeof(uint8_t), ss);
  delay(500);
  ss.end();

  Serial.println(F("Setup done"));
}

void uart_over_ble_init(void)
{
  uart_over_ble.uart_rts_local = true;
}

bool uart_tx(uint8_t *buffer, uint8_t buffer_len)
{
  bool status = false;

  if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX) &&
      (aci_state.data_credit_available >= 1))
  {
    status = lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, buffer, buffer_len);
    if (status)
    {
      aci_state.data_credit_available--;
    }
  }

  return status;
}

bool uart_process_control_point_rx(uint8_t *byte, uint8_t length)
{
  bool status = false;
  aci_ll_conn_params_t *conn_params;

  if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_CONTROL_POINT_TX) )
  {
    Serial.println(*byte, HEX);
    switch(*byte)
    {
      /*
        Queues a ACI Disconnect to the nRF8001 when this packet is received.
        May cause some of the UART packets being sent to be dropped
      */
      case UART_OVER_BLE_DISCONNECT:
        /*
          Parameters:
          None
        */
        lib_aci_disconnect(&aci_state, ACI_REASON_TERMINATE);
        status = true;
        break;


      /*
        Queues an ACI Change Timing to the nRF8001
      */
      case UART_OVER_BLE_LINK_TIMING_REQ:
        /*
        Parameters:
          Connection interval min: 2 bytes
          Connection interval max: 2 bytes
          Slave latency:           2 bytes
          Timeout:                 2 bytes
          Same format as Peripheral Preferred Connection Parameters (See nRFgo studio -> nRF8001 Configuration -> GAP Settings
          Refer to the ACI Change Timing Request in the nRF8001 Product Specifications
        */
        conn_params = (aci_ll_conn_params_t *)(byte+1);
        lib_aci_change_timing( conn_params->min_conn_interval,
                                conn_params->max_conn_interval,
                                conn_params->slave_latency,
                                conn_params->timeout_mult);
        status = true;
        break;

      /*
        Clears the RTS of the UART over BLE
      */
      case UART_OVER_BLE_TRANSMIT_STOP:
        /*
          Parameters:
          None
        */
        uart_over_ble.uart_rts_local = false;
        status = true;
        break;


      /*
        Set the RTS of the UART over BLE
      */
      case UART_OVER_BLE_TRANSMIT_OK:
        /*
          Parameters:
          None
        */
        uart_over_ble.uart_rts_local = true;
        status = true;
        break;
    }
  }

  return status;
}

void aci_loop()
{
  static bool setup_required = false;

  // We enter the if statement only when there is a ACI event available to be processed
  if (lib_aci_event_get(&aci_state, &aci_data))
  {
    aci_evt_t * aci_evt;
    aci_evt = &aci_data.evt;
    switch(aci_evt->evt_opcode)
    {
      /**
        As soon as you reset the nRF8001 you will get an ACI Device Started Event
      */
      case ACI_EVT_DEVICE_STARTED:
      {
        aci_state.data_credit_total = aci_evt->params.device_started.credit_available;
        switch(aci_evt->params.device_started.device_mode)
        {
          case ACI_DEVICE_SETUP:
            /**
              When the device is in the setup mode
            */
            Serial.println(F("Evt Device Started: Setup"));
            setup_required = true;
            break;

          case ACI_DEVICE_STANDBY:
            Serial.println(F("Evt Device Started: Standby"));
            // Looking for a phone by sending radio advertisements
            // When a phone connects to us we will get an ACI_EVT_CONNECTED event from the nRF8001
            if (aci_evt->params.device_started.hw_error)
            {
              delay(20); //Handle the HW error event correctly.
            }
            else
            {
              // calling lib_aci_connect with advertising time 0 (endless) and advertising interval of 50ms
              lib_aci_connect(0, 0x0050);
              Serial.println(F("Advertising started: Tap Connect on the nRF UART app"));
            }

            break;
        }
      }
      break; // ACI Device Started Event

      case ACI_EVT_CMD_RSP:
        // If an ACI command response event comes with an error -> stop
        if (ACI_STATUS_SUCCESS != aci_evt->params.cmd_rsp.cmd_status)
        {
          // ACI ReadDynamicData and ACI WriteDynamicData will have status codes of
          // TRANSACTION_CONTINUE and TRANSACTION_COMPLETE
          // all other ACI commands will have status code of ACI_STATUS_SCUCCESS for a successful command
          Serial.print(F("ACI Command"));
          Serial.println(aci_evt->params.cmd_rsp.cmd_opcode, HEX);
          Serial.print(F("Evt Cmd respone: Status"));
          Serial.println(aci_evt->params.cmd_rsp.cmd_status, HEX);
        }
        if (ACI_CMD_GET_DEVICE_VERSION == aci_evt->params.cmd_rsp.cmd_opcode)
        {
          // Store the version and configuration information of the nRF8001 in the Hardware Revision String Characteristic
          lib_aci_set_local_data(&aci_state, PIPE_DEVICE_INFORMATION_HARDWARE_REVISION_STRING_SET,
            (uint8_t *)&(aci_evt->params.cmd_rsp.params.get_device_version), sizeof(aci_evt_cmd_rsp_params_get_device_version_t));
        }
        break;

      case ACI_EVT_CONNECTED:
        Serial.println(F("Evt Connected"));
        uart_over_ble_init();
        timing_change_done              = false;
        aci_state.data_credit_available = aci_state.data_credit_total;

        /*
          Get the device version of the nRF8001 and store it in the Hardware Revision String
        */
        lib_aci_device_version();
        break;

      case ACI_EVT_PIPE_STATUS:
        Serial.println(F("Evt Pipe Status"));
        if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX) && (false == timing_change_done))
        {
          lib_aci_change_timing_GAP_PPCP(); // change the timing on the link as specified in the nRFgo studio -> nRF8001 conf. -> GAP.
                                            // Used to increase or decrease bandwidth
          timing_change_done = true;

          char hello[]="Bluetooth works";
          uart_tx((uint8_t *)&hello[0], strlen(hello));
          Serial.print(F("Sending:"));
          Serial.println("Bluetooth works");
        }
        break;

      case ACI_EVT_TIMING:
        Serial.println(F("Evt link connection interval changed"));
        lib_aci_set_local_data(&aci_state,
                                PIPE_UART_OVER_BTLE_UART_LINK_TIMING_CURRENT_SET,
                                (uint8_t *)&(aci_evt->params.timing.conn_rf_interval), /* Byte aligned */
                                PIPE_UART_OVER_BTLE_UART_LINK_TIMING_CURRENT_SET_MAX_SIZE);
        break;

      case ACI_EVT_DISCONNECTED:
        Serial.println(F("Evt Disconnected/Advertising timed out"));
        lib_aci_connect(0/* in seconds  : 0 means forever */, 0x0050 /* advertising interval 50ms*/);
        Serial.println(F("Advertising started. Tap Connect on the nRF UART app"));
        break;

      case ACI_EVT_DATA_RECEIVED:
        Serial.print(F("Pipe Number: "));
        Serial.println(aci_evt->params.data_received.rx_data.pipe_number, DEC);

        if (PIPE_UART_OVER_BTLE_UART_RX_RX == aci_evt->params.data_received.rx_data.pipe_number)
          {

            Serial.print(F(" Data (Hex): "));

            // Switch statement to parse app messages
            switch(aci_evt->params.data_received.rx_data.aci_data[aci_evt->len - 3]) {
              case 49:
                Serial.println(aci_evt->params.data_received.rx_data.aci_data[aci_evt->len - 3]);
                Serial.println("Theft-Protection is on");
                // theft protection on
                ftheftprotection = true;
                // turn GPS on
                sendUBX(gpsoff, sizeof(gpsoff)/sizeof(uint8_t), ss);
                break;
              case 48:
                Serial.println(aci_evt->params.data_received.rx_data.aci_data[aci_evt->len - 3]);
                Serial.println("Theft-Protection is off");
                // theft protection off
                ftheftprotection = false;
                // turn GPS on
                sendUBX(gpson, sizeof(gpson)/sizeof(uint8_t), ss);
                break;
              default:
                Serial.println(aci_evt->params.data_received.rx_data.aci_data[aci_evt->len - 3]);
                Serial.println("Last char unknown command");
            }
            
            // Print message:          
            
            for(int i=0; i<aci_evt->len - 2; i++)
            {
              Serial.print((char)aci_evt->params.data_received.rx_data.aci_data[i]);
              uart_buffer[i] = aci_evt->params.data_received.rx_data.aci_data[i];
              Serial.print(F(" "));
            }
            uart_buffer_len = aci_evt->len - 2;
            Serial.println(F(""));
            if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX))
            {
              /* Do this to test the loopback otherwise comment it out */
              /*
              if (!uart_tx(&uart_buffer[0], aci_evt->len - 2))
              {
                Serial.println(F("UART loopback failed"));
              }
              else
              {
                Serial.println(F("UART loopback OK"));
              }
              */
            }
        }

        if (PIPE_UART_OVER_BTLE_UART_CONTROL_POINT_RX == aci_evt->params.data_received.rx_data.pipe_number)
        {
          uart_process_control_point_rx(&aci_evt->params.data_received.rx_data.aci_data[0], aci_evt->len - 2); //Subtract for Opcode and Pipe number
        }

        break;

      case ACI_EVT_DATA_CREDIT:
        aci_state.data_credit_available = aci_state.data_credit_available + aci_evt->params.data_credit.credit;
        break;

      case ACI_EVT_PIPE_ERROR:
        // See the appendix in the nRF8001 Product Specication for details on the error codes
        Serial.print(F("ACI Evt Pipe Error: Pipe #"));
        Serial.print(aci_evt->params.pipe_error.pipe_number, DEC);
        Serial.print(F(" - Pipe Error Code: 0x"));
        Serial.println(aci_evt->params.pipe_error.error_code, HEX);

        // Increment the credit available as the data packet was not sent.
        // The pipe error also represents the Attribute protocol Error Response sent from the peer and that should not be counted
        // for the credit.
        if (ACI_STATUS_ERROR_PEER_ATT_ERROR != aci_evt->params.pipe_error.error_code)
        {
          aci_state.data_credit_available++;
        }
        break;

      case ACI_EVT_HW_ERROR:
        Serial.print(F("HW error: "));
        Serial.println(aci_evt->params.hw_error.line_num, DEC);

        for(uint8_t counter = 0; counter <= (aci_evt->len - 3); counter++)
        {
          Serial.write(aci_evt->params.hw_error.file_name[counter]); //uint8_t file_name[20];
        }
        Serial.println();
        lib_aci_connect(0/* in seconds, 0 means forever */, 0x0050 /* advertising interval 50ms*/);
        Serial.println(F("Advertising started. Tap Connect on the nRF UART app"));
        break;

    }
  }
  else
  {
    //Serial.println(F("No ACI Events available"));
    // No event in the ACI Event queue and if there is no event in the ACI command queue the arduino can go to sleep
    // Arduino can go to sleep now
    // Wakeup from sleep from the RDYN line
  }

  /**
    setup_required is set to true when the device starts up and enters setup mode.
    It indicates that do_aci_setup() should be called. The flag should be cleared if
    do_aci_setup() returns ACI_STATUS_TRANSACTION_COMPLETE.
   */
  if(setup_required)
  {
    if (SETUP_SUCCESS == do_aci_setup(&aci_state))
    {
      setup_required = false;
    }
  }
}

bool stringComplete = false;  // whether the string is complete
uint8_t stringIndex = 0;      //Initialize the index to store incoming chars

// End of Bluetooth setup

// payload message struct
typedef struct {
  bool theftprotection;
  float lat;
  float lng;
  int temperature;
} Payload;

void loop()
{
  ssBluetooth.begin(9600);
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;
  Payload p;
  
  // Read Bluetooth
  // Process any ACI commands or events
  aci_loop();
  
  // print the string when a newline arrives:
  if (stringComplete) 
  {
    Serial.print(F("Sending: "));
    Serial.println((char *)&uart_buffer[0]);
  
    uart_buffer_len = stringIndex + 1;
  
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, uart_buffer, uart_buffer_len))
    {
      Serial.println(F("Serial input dropped"));
    }
  
    // clear the uart_buffer:
    for (stringIndex = 0; stringIndex < 20; stringIndex++)
    {
      uart_buffer[stringIndex] = ' ';
    }
  
    // reset the flag and the index in order to receive more data
    stringIndex    = 0;
    stringComplete = false;
  }

  // For ChipKit you have to call the function that reads from Serial
  #if defined (__PIC32MX__)
    if (Serial.available())
    {
      serialEvent();
    }
  #endif

  ssBluetooth.end();
  ss.begin(9600);

  // Parse GPS data for one second and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      //Serial.print(c); // uncomment to see the full NMEA datasets
      if (gps.encode(c)){ // Did a new valid sentence come in?
        newData = true;
      } 
    }
  }

  // end gps in order to make port listening for akeru available
  // gps receiver will continue to get a fix or track gps satellites in the background
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
    
    // Read DHT sensor; temperature disabled right now
    float humi = dht.readHumidity();
    int temp = (int)dht.readTemperature();
    
    // check if returns are valid, if they are NaN (not a number) then something went wrong!
    if (isnan(temp) || isnan(humi)) 
    {
        Serial.println("Failed to read from DHT");
    } 
    else 
    {
        Serial.print("Humidity: "); 
        Serial.print(humi);
        Serial.print(" %\t");
        Serial.print("Temperature: "); 
        Serial.print(temp);
        Serial.println(" *C");
    }
    
    p.theftprotection = ftheftprotection;
    //TODO: check if flat is too short
    p.lat = flat;
    //TODO: check if flon is too short
    p.lng = flon;
    p.temperature = (int)temp;

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
        Serial.println(sizeof(p));
        Akeru.send(&p, sizeof(p)); // send payload to Sigfox network
        Serial.println("Message sent");
      }
    }
    // end modem in order to make port listening for gps available
    ssAkeru.end();
  } else {
    
    Serial.print("No GPS data\n");
    
    // Read DHT sensor
    float humi = dht.readHumidity();
    int temp = (int)dht.readTemperature();
     
    // check if returns are valid, if they are NaN (not a number) then something went wrong!
    if (isnan(temp) || isnan(humi))
    {
      Serial.println("Failed to read from DHT");
    } 
    else 
    {
      Serial.print("Humidity: "); 
      Serial.print(humi);
      Serial.print(" %\t");
      Serial.print("Temperature: "); 
      Serial.print(temp);
      Serial.println(" *C");
      Serial.print("Theft protection: ");
      Serial.println(ftheftprotection);
    }
  }

  delay(2500); // wait 2.5 seconds to the next cycle
}

/*
 COMMENT ONLY FOR ARDUINO
 SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 Serial Event is NOT compatible with Leonardo, Micro, Esplora
 */
void serialEvent() {

  while(Serial.available() > 0){
    // get the new byte:
    dummychar = (uint8_t)Serial.read();
    if(!stringComplete)
    {
      if (dummychar == '\n')
      {
        // if the incoming character is a newline, set a flag
        // so the main loop can do something about it
        stringIndex--;
        stringComplete = true;
      }
      else
      {
        if(stringIndex > 19)
        {
          Serial.println("Serial input truncated");
          stringIndex--;
          stringComplete = true;
        }
        else
        {
          // add it to the uart_buffer
          uart_buffer[stringIndex] = dummychar;
          stringIndex++;
        }
      }
    }
  }
}

// https://ukhas.org.uk/guides:ublox6
// Send a byte array of UBX protocol to the GPS
void sendUBX(uint8_t *MSG, uint8_t len, SoftwareSerial gpsserial) {
  for(int i=0; i<len; i++) {
    gpsserial.write(MSG[i]);
    Serial.print(MSG[i], HEX);
  }
  gpsserial.println();
}
