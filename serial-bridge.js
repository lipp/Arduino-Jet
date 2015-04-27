var net = require('net');

var SerialPort = require("serialport").SerialPort;

var serial = new SerialPort("/dev/cu.usbmodem1431", {
	baudrate: 115200
});

serial.on('open', function(err) {
	console.log('serial open');

	if (err) {
		console.log('could not open serial', err);
		process.exit(1);
	}

	var sock = net.connect(11122);

	sock.pipe(serial);
	serial.pipe(sock);
	serial.pipe(process.stdout);


	// write one arbitrary handshake byte.
	// the arduino waits in "setup" until 1 char is available.
	// postpone this action. writing asap breaks the arduino :(
	setTimeout(function() {
		serial.write('x');
		serial.drain();
	}, 2000);
});
