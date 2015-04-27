
#include <ArduinoJetPeer.h>
#include <SerialClient.h>

// wrap Serial as "(Ethernet)Client"
SerialClient serialClient(Serial);

/* jet global vars: peer and states/methods */
JetPeer peer;
JetState *ledState;
JetState *analog1State;
JetState *analog2State;

#define LED_PIN 13

/* a jet state callback function example */
bool set_led(aJsonObject *led_val, void *context) {
  if (led_val->valuebool) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  return true;
}

/* a jet fetch callback function example */
void print_analog_1(const char *path, const char *event, aJsonObject *val,
                    void *context) {
  // do something useful here
}
/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/
void setup(void) {
  // init led
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);

  /* tell the peer which is the (network/serial) client */
  peer.init(serialClient);

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

void loop(void) {
  peer.loop();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    analog1State->value(aJson.createItem(analogRead(0)));
    analog2State->value(aJson.createItem(analogRead(1)));
  }
}
