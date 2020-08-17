var KLogger = {};

KLogger.debug = false;
KLogger.wsUri = "ws://xdn.local:8337";
KLogger.websocket = false;
KLogger.ansi_up = false;

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
				var message = KLogger.ansi_up.ansi_to_html(atob(e.message));
				$("#message-container").append('<div class="message filter-' + e.channel + '"><div class="messagehead"><div class="channel channel-' + e.channel + '">' + e.channel + '</div><div class="timestamp">' + e.timestamp + '</div></div>' + message + '</div>');
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

// Add or remove the "hidden" CSS class to hide / show messages of a given channel
function update_filter(channel)
{
	var cb = document.getElementById('check-' + channel);
	if(cb.checked == true){
		$(".filter-" + channel).each(function() {
			$(this).removeClass("hidden");
		});
	}
	else{
		$(".filter-" + channel).each(function() {
			$(this).addClass("hidden");
		});
	}
}

$(function(){
	KLogger.ansi_up = new AnsiUp;
	KLogger.websocket = new WebSocket(KLogger.wsUri);

	KLogger.websocket.onmessage = function(e) {
		
		// Multiple messages could be concatenated in e.data, we split them in an array
		var rxp = /({.+?})/g, curMatch;

		while(curMatch = rxp.exec(e.data)) {
		    handle_message(curMatch[1]);
		}
	}

	// Create checkbox controls for channel filtering
	var channels = ['core','application','editor','event','asset','memory','thread','entity','scene','script','render','shader','texture','util','config','ios'];

	channels.forEach(function (channel, index)
	{
		var id = 'check-' + channel;
		$("#controls-container").append('<input type="checkbox" id="' + id + '" onclick="update_filter(\''+ channel + '\')" checked>' + channel + '</input>');
	});
});