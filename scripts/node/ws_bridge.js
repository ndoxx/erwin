// NodeJS server that translates raw TCP data into websocket traffic

const WebSocket = require('ws');
const net = require('net');
const wss = new WebSocket.Server({ port: 8337 });

const tcp_port = 31337;
const tcp_host = '127.0.0.1';

// On websocket connection, create a TCP server that redirects raw TCP to the websocket
wss.on('connection', function connection(ws) {
	const tcp_server = net.createServer(onClientConnection);
	tcp_server.listen(tcp_port,tcp_host,function(){
	   console.log(`TCP server started on port ${tcp_port} at ${tcp_host}`); 
	});

	// TCP Listener
	function onClientConnection(sock){
	    console.log(`${sock.remoteAddress}:${sock.remotePort} Connected`);
	    sock.on('data',function(data){
	        console.log(`${sock.remoteAddress}:${sock.remotePort}: ${data}`);
	        ws.send(`${data}`);
	    });
	    sock.on('close',function(){
	        console.log(`${sock.remoteAddress}:${sock.remotePort} Terminated the connection`);
	    });
	    sock.on('error',function(error){
	        console.error(`${sock.remoteAddress}:${sock.remotePort} Connection Error ${error}`);
	    });
	};
});


