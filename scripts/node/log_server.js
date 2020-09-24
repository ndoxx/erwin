// NodeJS server that translates raw TCP data into websocket traffic

const WebSocket = require('ws');
const net = require('net');
const wss = new WebSocket.Server({port : 8337});

const tcp_port = 31337;
const tcp_host = '127.0.0.1';

var wsock = false;
wss.on('connection', function connection(ws) { wsock = ws; });

const tcp_server = net.createServer(onClientConnection);
tcp_server.listen(tcp_port, tcp_host,
                  function() { console.log(`TCP server started on port ${tcp_port} at ${tcp_host}`); });

// TCP Listener
function onClientConnection(sock)
{
    console.log(`${sock.remoteAddress}:${sock.remotePort} Connected`);
    sock.on('data', function(data) {
        console.log(`${sock.remoteAddress}:${sock.remotePort}: ${data}`);
        wsock.send(`${data}`);
    });
    sock.on('close', function() { console.log(`${sock.remoteAddress}:${sock.remotePort} Terminated the connection`); });
    sock.on('error',
            function(error) { console.error(`${sock.remoteAddress}:${sock.remotePort} Connection Error ${error}`); });
};

// HTTP and static file servers
var http = require('http');
var url = require('url');
var fs = require('fs');
var nstatic = require('node-static');
var fileServer = new nstatic.Server('./html');

var server = http.createServer(function(request, response) {
    // Redirect root path to log.html
    if(request.url == '/')
    {
        fileServer.serveFile('/log.html', 200, {}, request, response);
        return;
    }
    request
        .addListener('end',
                     function() {
                         fileServer.serve(request, response, function(err, result) {
                             if(err)
                             { // There was an error serving the file
                                 console.error("Error serving " + request.url + " - " + err.message);

                                 // Respond to the client
                                 response.writeHead(err.status, err.headers);
                                 response.end();
                             }
                         });
                     })
        .resume();
});
server.listen(8095);