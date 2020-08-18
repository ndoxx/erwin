var KLogger = {};

KLogger.debug = false;
KLogger.wsUri = "ws://xdn.local:8337";
KLogger.websocket = false;
KLogger.ansi_up = false;

KLogger.dlog = function(text) {
	if(KLogger.debug) console.log(text);
}

var messageStyles = ['type-raw', 'type-normal', 'type-item', 'type-event', 'type-notify', 'type-warning', 'type-error', 'type-fatal', 'type-bang', 'type-good', 'type-bad'];
var channels = ['core','application','editor','event','asset','memory','thread','entity','scene','script','render','shader','texture','util','config','ios'];

function handle_message(item) {
	try {
		KLogger.dlog(item);
		var e = JSON.parse(item);
		switch(e.action) {
			case "msg":
				var message = KLogger.ansi_up.ansi_to_html(atob(e.message));
				var styeClass = messageStyles[e.type];
				var locationCaption = '';
				if(e.severity>=1 && e.file.length>0) {
					locationCaption = `<div class="location-caption">${e.file}:${e.line}</div>`;
				}
				$("#message-container").append(`<div class="message ${styeClass} filter-${e.channel} filter-sev-${e.severity}"><div class="messagehead"><div class="channel channel-${e.channel}">${e.channel}</div><div class="timestamp">${e.timestamp}</div></div>${message}${locationCaption}</div>`);
				break;
			default:
				KLogger.dlog("Unknown packet:");
				KLogger.dlog(e);
				break;
		}
	}
	catch(err) {
		$("#message-container").append(`<div class="error">${err}</div>`);
	}
}

// Add or remove the "hidden" CSS class to hide / show messages of a given channel
function update_channel_filter() {
	$("#channelSelector option").each(function() {
		var channel = $(this).text();
		if($(this).prop('selected')) {
			$(`.filter-${channel}`).each(function() {
				$(this).removeClass("chan-hidden");
			});
		}
		else {
			$(`.filter-${channel}`).each(function() {
				$(this).addClass("chan-hidden");
			});
		}
	});
}

function select_all_channels() {
	$("#channelSelector option").each(function() {
		$(this).prop('selected', true);
	});
	$(`.message`).each(function() {
		$(this).removeClass("chan-hidden");
	});
}

function update_verbosity_filter() {
	var verbosity = $("#verbositySlider").val();
	for(var severity=0; severity<=3; ++severity) {
		if(verbosity >= 3-severity) {
			$(`.filter-sev-${severity}`).each(function() {
				$(this).removeClass("sev-hidden");
			});
		}
		else {
			$(`.filter-sev-${severity}`).each(function() {
				$(this).addClass("sev-hidden");
			});
		}
	}
}

$(function() {
	KLogger.ansi_up = new AnsiUp;
	KLogger.websocket = new WebSocket(KLogger.wsUri);

	KLogger.websocket.onmessage = function(e) {
		
		// Multiple messages could be concatenated in e.data, we split them in an array
		var rxp = /({.+?})/g, curMatch;

		while(curMatch = rxp.exec(e.data)) {
		    handle_message(curMatch[1]);
		}
	}

	// Create controls for channel filtering
	$("#controls-container").append(`<input type=button value="Select all" onclick="select_all_channels()" style="width: 100%"></input>`);
	$("#controls-container").append(`<select multiple size="${channels.length}" id="channelSelector" style="width: 100%"></select>`);

	channels.forEach(function (channel, index) {
		$("#channelSelector").append(`<option id="option-${channel}" onclick="update_channel_filter()" selected="selected">${channel}</option>`);
	});

	// Severity filtering
	$("#controls-container").append(`<label for="verbositySlider">Verbosity:</label>`);
	$("#controls-container").append(`<input id="verbositySlider" type=range min="0" max="3" value="3" class="slider" oninput="update_verbosity_filter()" style="width: 100%">`);
});