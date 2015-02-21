# Arduino-Jet
Jet Peer lib for Arduino (http://jetbus.io).

Arduino-Jet enables you to easily integrate distributed web-capable realtime
communication with your Arduino. See [Jet Homepage](http://jetbus.io) for more
details and a Live Control Center called ["Radar"](http://jetbus.io/radar.html).

As the Arduino's resources are rather limited, this lib does not implement every
feature of other [Jet](http://jetbus.io) implementations.

This library contains a modified version of [aJson](https://github.com/interactive-matter/aJson),
which does not use malloc and free. DONT'T use aJson side-by-side!

# Quickstart example

```c++
// add library
#include <ArduinoJetPeer.h>

// create a global peer
JetPeer peer;

// define a function which is invoked, whenever
// someone tries to change your state.
bool setLedBrightness(aJsonObject* val, void* context) {
  setLedBrightness(val->valueint);
}

void setup() {
  ...
  // connect to jet daemon
  socket.connect(daemon_ip, 11122);

  // pass client socket to peer
  peer.init(socket);

  // create a state with initial value and set callback function
  peer.state("led/brightness", aJson.createItem(100), setLedBrightness);
}
```

# Setup

1. Clone this repo to your Arduino/libraries folder (on OSX ~/Documents/Arduino/libraries).

2. Install the node.js Daemon on your host machine or your "cloud" server:

  ```sh
  $ npm install -g node-jet
  ```

3. Start the Jet Daemon:

  ```sh
  $ jetd.js
  ```

  The default ports of the Jet Daemon are 11123 (websocket) and 11122 (raw).
  Load the `JetExample` sketch.

4. Load contained example "JetExample". This example uses cc3000 wifi shield.

   Change the ip/server name to match the machine where the Daemon runs.

  ```c++
  #define WLAN_SSID  "YOUR_SSID"
  #define WLAN_PASS  "YOUR_PASSWORD"
  #define JET_DAEMON "YOUR_IP"
  ```

5. Compile and upload the sketch

6. Open [Radar](http://jetbus.io/radar.html)

  Enter `ws://localhost:11123` as the Daemon url.
  Press "connect", press "fetch", watch your analog data.

# API

## `JetPeer` [class]

### `void JetPeer::init(Client& socket)`

Initializes a JetPeer instance with a Client reference. The socket must be already connected
with the Jet Daemon (default port 11122). The concrete class of the Client can be
an `EthernetClient` or `Adafruit_CC3000_Client` or any other class which derives from `Client`.

JetPeer::init MUST be called before any other function.

```c++
JetPeer peer;
EthernetClient sock;
...
sock.connect(jet_daemon_ip, 11122); // default jet raw port
...
peer.init(sock);
```

### `JetState* JetPeer::state(const char* path, aJsonObject* value, [set_handler_t set_callback], [void* set_context])`

Adds a state to the Daemon with the given `path` and the given `value`.
The set_callback is optional. When specified, the set_callback will be invoked,
whenever some (other) peer calls `set` with the corresponding `path`.
The set_context is also optional and - when specified - is passed to the set_callback
as second param.

The max number of states is limited by JET_MAX_STATES (default=3);

```c++
// create a read only state
JetState* test = peer.state("arduino/test", aJson.createItem(123));
```

```c++
bool setFoo(aJsonObject* val, void* context) {
  aJsonObject* hello = aJson.getObjectItem(val, "hello");
  if (hello) {
    ...
    return true;
  } else {
    return false;
  }
}
...
// create a settable state with object structure
aJsonObject* val = aJson.createObject();
aJson.addItemToObject(val, "hello", aJson.createItem("world"));
JetState* foo = peer.state("arduino/foo", val, setFoo);
```

### `JetFetcher* JetPeer::fetch(const char* path, fetch_handler_t fetch_callback, [void* fetch_context])`

Sets up a new simple path based fetch. The fetch_callback is called, whenever a relevent event
(add/remove/change) takes place. With fetch, peers can wait for other (remote) states/methods,
synchronize to "master values" or -more general - react to other stuff.

The max number of fetchers is limited by JET_MAX_FETCHERS (default=3);

```c++

void print_event(const char* path, const char* event, aJsonObject* val, void* context) {
  Serial.println(path);
  Serial.println(event);
  ...
};

peer.fetch("master/brightness", print_event);
```

### `JetFetcher* JetPeer::fetch(aJsonObject* fetch_expression, fetch_handler_t fetch_callback, [void* fetch_context])`

Sets up a new fetch. The fetch_callback is called, whenever a relevent event
(add/remove/change) takes place. With fetch, peers can wait for other (remote) states/methods,
synchronize to "master values" or -more general - react to other stuff.

The max number of fetchers is limited by JET_MAX_FETCHERS (default=3);

```c++

void print_event(const char* path, const char* event, aJsonObject* val, void* context) {
  Serial.println(path);
  Serial.println(event);
  ...
};

// create expr JSON:
// {
//   path:{ startsWith: "ardu/analog" },
//   value: { lessThan: 100 }
// }
//
aJsonObject* expr = aJson.createObject();
aJsonObject* pathRule = aJson.createObject();
aJson.addItemToObject(pathRule, "startsWith", aJson.createItem("ardu/analog"));
aJsonObject* valueRule = aJson.createObject();
aJson.addItemToObject(pathRule, "lessThan", aJson.createItem(100));
aJson.addItemToObject(expr, "path", pathRule);
aJson.addItemToObject(expr, "value", valueRule);
peer.fetch(expr, print_event);
```

### `void JetPeer::set(const char* path, aJsonObject* value)`

Sets another state to a new value. The other state's set_callback function will
be invoked.

NOTE: The response to the set request is not available.

### `void JetPeer::call(const char* path, aJsonObject* args)`

Calls a method specified by `path` with `args` (arguments). The `args` must be either
of type JSON Object or JSON Array.

NOTE: The response to the call request is not available.

### `JetMethod* JetPeer::method(const char*, call_handler_t call_handler)`

NOT IMPLEMENTED YET

# Memory

TODO doc defines / limits
