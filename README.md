# Arduino-Jet
Jet Peer lib for Arduino (http://jetbus.io).

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

4. Change the ip/server name to match the machine where the Daemon runs.

```c++
#define JET_DAEMON YOUR_IP
```

5. Compile and upload the sketch (uses cc3000 Wifi)

6. Open [Radar](http://jetbus.io/radar.html)

Enter `ws://localhost:11123` as the Daemon url.
Press "connect", press "fetch", watch your analog data.

# Memory

TODO doc defines / limits

# API

TODO
