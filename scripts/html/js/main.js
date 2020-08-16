var KLogger = {};

KLogger.debug = false;
KLogger.wsUri = "ws://xdn.local:8337";
KLogger.websocket = false;

KLogger.dlog = function(text) {
	if(KLogger.debug) console.log(text);
}

function handle_message(item)
{
	try
	{
		KLogger.dlog(item);
		var e = JSON.parse(item);
		switch(e.action) {
			case "msg":
				$("#message-container").append('<div class="message">[' + e.timestamp + '][' + e.channel + '] ' + atob(e.message) + '</div>');
				break;
			default:
				KLogger.dlog("Unknown packet:");
				KLogger.dlog(e);
				break;
		}
	}
	catch(err)
	{
		$("#message-container").append('<div class="error">' + err + '</div>');
	}
}

$(function(){
	KLogger.websocket = new WebSocket(KLogger.wsUri);

	KLogger.websocket.onmessage = function(e) {
		
		// Multiple messages could be concatenated in e.data, we split them in an array
		var found = [],
		    rxp = /({.+?})/g,
		    curMatch;

		while(curMatch = rxp.exec(e.data)) {
		    found.push(curMatch[1]);
		}

		for(var i=0; i<found.length; i++)
		{
			handle_message(found[i]);
		}

		/*found.forEach(function (item, index) {
			handle_message(item);
		});*/
	}
});