#include <aJSON.h>

#include <SPI.h>



#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>

#include <ArduinoJetPeer.h>


// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

#define WLAN_SSID       "gspot"        // cannot be longer than 32 characters!
#define WLAN_PASS       "kickmetogspot"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

uint32_t ip;

Adafruit_CC3000_Client client;
JetPeer peer;
JetState* testState;

#define JET_DAEMON  "192.168.1.149"
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
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

 
   ip = 0;
  // Try looking up the website's IP address
 
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
  
  peer.set_client(client);
  testState = new JetState(peer, "ARDU/test", 123);
  
}

void loop(void)
{
  peer.loop();
  delay(1000);
}



/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

