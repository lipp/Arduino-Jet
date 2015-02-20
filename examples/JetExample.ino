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

/* jet global vars: peer and states/methods */
JetPeer peer;
JetState* ledState;
JetState* analog1State;
JetState* analog2State;

#define JET_DAEMON  "YOUR_IP"

/* a jet state callback function example */
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

/* a jet fetch callback function example */
void print_analog_1(const char* path, const char* event, aJsonObject* val, void* context) {
  Serial.print(path);Serial.print(F(" "));Serial.print(event);Serial.print(F(" "));Serial.println(val->valueint);
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
  Serial.println(F("Hello, Jet Folks!\n"));
  if (!cc3000.begin())
  {
    Serial.println(F("Unable to initialise the CC3000!"));
    while(1);
  }

  Serial.println(F("creating network connection to daemon...\n"));

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);

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

  /* tell the peer which is the network client */
  peer.init(client);

  /* create a state with a "set" callback function (set_led).
    the initial value is "true".
    */
  ledState = peer.state("ARDU/led", aJson.createItem((bool)true), set_led);

  /* create two states for the analog inputs. no "set" callback function is
    provided, so this is read-only.
    in the loop function (every 100ms) new values a read out and are posted.
   */
  analog1State = peer.state("ARDU/analog1", aJson.createItem(analogRead(0)));
  analog2State = peer.state("ARDU/analog2", aJson.createItem(analogRead(1)));

  /* create a fetcher which is subsribed to the "own" state ARDU/analog1.
     every time the value changes, the print_analog_1 function is called.
     this is not very useful but demonstrates the principle.
   */
  peer.fetch("ARDU/analog1", print_analog_1);

}

long previousMillis = 0;
long interval = 100;

void loop(void)
{
  peer.loop();
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    analog1State->value(aJson.createItem(analogRead(0)));
    analog2State->value(aJson.createItem(analogRead(1)));
    //Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  }

  delay(10);
}
