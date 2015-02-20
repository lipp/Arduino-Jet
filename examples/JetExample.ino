#include <SPI.h>

#include <aJSON.h>
#include <ArduinoJetPeer.h>

#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include "utility/debug.h"


// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

#define WLAN_SSID       "YOUR_SSID"        // cannot be longer than 32 characters!
#define WLAN_PASS       "YOUR_PASSWORD"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

uint32_t ip;

Adafruit_CC3000_Client client;
JetPeer peer;
JetState* ledState;
JetState* analog1State;
JetState* analog2State;

#define JET_DAEMON  "192.168.1.149"

bool set_led(aJsonObject* led_val, void* context)
{
  if (led_val->valuebool) {
    Serial.println("LED ON");
    //digitalWrite(13, HIGH);
  } else {
    Serial.println("LED OFF");
    digitalWrite(13, LOW);
  }
  return true;
}
/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/
void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n"));
  if (!cc3000.begin())
  {
    Serial.println(F("Unable to initialise the CC3000!"));
    while(1);
  }

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);

  /* NOTE: Secure connections are not available in 'Tiny' mode!
     By default connectToAP will retry indefinitely, however you can pass an
     optional maximum number of retries (greater than zero) as the fourth parameter.

     ALSO NOTE: By default connectToAP will retry forever until it can connect to
     the access point.  This means if the access point doesn't exist the call
     will _never_ return!  You can however put in an optional maximum retry count
     by passing a 4th parameter to the connectToAP function below.  This should
     be a number of retries to make before giving up, for example 5 would retry
     5 times and then fail if a connection couldn't be made.
  */
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }

  while (!cc3000.checkDHCP())
  {
    delay(100);
  }

  ip = 0;

  while (ip == 0) {
    if (! cc3000.getHostByName(JET_DAEMON, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }

  client = cc3000.connectTCP(ip, 11122);

  if (client.connected()) {
    Serial.println("connected");
  } else {
    Serial.println("not connected");
  }

  peer.init(client);

  ledState = peer.state("ARDU/led", aJson.createItem((bool)true));
  ledState->set_handler(set_led);

  analog1State = peer.state("ARDU/analog1", aJson.createItem(analogRead(0)));
  analog2State = peer.state("ARDU/analog2", aJson.createItem(analogRead(1)));
   // initialize digital pin 13 as an output.
  //pinMode(13, OUTPUT);
}

long previousMillis = 0;
long interval = 50;


void loop(void)
{
  peer.loop();
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    analog1State->value(aJson.createItem(analogRead(0)));
    analog2State->value(aJson.createItem(analogRead(1)));
    //Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  }

  delay(10);
}
