var KLogger = {};

KLogger.debug = false;
KLogger.wsUri = "ws://xdn.local:8337";
KLogger.websocket = false;
KLogger.ansi_up = false;

KLogger.messageStyles = ['type-raw', 'type-normal', 'type-item', 'type-event', 'type-notify', 'type-warning', 'type-error', 'type-fatal', 'type-bang', 'type-good', 'type-bad'];
KLogger.channels = ['core'];
KLogger.channel_colors = ['#ff6600'];

KLogger.dlog = function(text) {
	if(KLogger.debug) console.log(text);
}

function hslToRgb(h, s, l) {
	var r, g, b;
	if (s == 0) {
		r = g = b = l; // achromatic
	}
	else {
		function hue2rgb(p, q, t) {
			if (t < 0) t += 1;
			if (t > 1) t -= 1;
			if (t < 1/6) return p + (q - p) * 6 * t;
			if (t < 1/2) return q;
			if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
			return p;
		}

		var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		var p = 2 * l - q;

		r = hue2rgb(p, q, h + 1/3);
		g = hue2rgb(p, q, h);
		b = hue2rgb(p, q, h - 1/3);
	}

	return [ Math.floor(r * 255), Math.floor(g * 255), Math.floor(b * 255) ];
}

function componentToHex(c) {
	var hex = c.toString(16);
	return hex.length == 1 ? "0" + hex : hex;
}

function rgbToHex(rgb) {
	return "#" + componentToHex(rgb[0]) + componentToHex(rgb[1]) + componentToHex(rgb[2]);
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

function update_channel_controls() {
	// Clear channel options
	$("#channelSelector option").remove();

	// For each channel in channels array, create a selectable option
	$('#channelSelector').attr('size',KLogger.channels.length);
	KLogger.channels.forEach(function (channel, index) {
		$("#channelSelector").append(`<option id="option-${channel}" onclick="update_channel_filter()" selected="selected">${channel}</option>`);
	});

	// Also create a style for each channel
	KLogger.channel_colors = [];
	var hue_increment = 1.0 / (KLogger.channels.length + 1);
	var hue = hue_increment * 0.5;
	for(var ii=0; ii<KLogger.channels.length; ++ii)
	{
		KLogger.channel_colors.push(rgbToHex(hslToRgb(hue, 0.90, 0.5)));
		hue += hue_increment;
	}
}

function clear_messages() {
	$("#message-container > div.message").remove();
}

function handle_message(item) {
	try {
		KLogger.dlog(item);
		var e = JSON.parse(item);
		switch(e.action) {
			case "msg":
				var message = KLogger.ansi_up.ansi_to_html(atob(e.message));
				var styeClass = KLogger.messageStyles[e.type];
				var chanColor = KLogger.channel_colors[KLogger.channels.indexOf(e.channel)];
				var locationCaption = '';
				if(e.severity>=1 && e.file.length>0) {
					locationCaption = `<div class="location-caption">${e.file}:${e.line}</div>`;
				}
				$("#message-container").append(`<div class="message ${styeClass} filter-${e.channel} filter-sev-${e.severity}"><div class="messagehead"><div class="channel" style="background: ${chanColor}">${e.channel}</div><div class="timestamp">${e.timestamp}</div></div>${message}${locationCaption}</div>`);
				break;
			case "set_channels":
				KLogger.channels = e.channels;
				// Sort alphabetically
				KLogger.channels.sort(function (a, b) {
					if (a < b) return -1;
					else if (a > b) return 1;
					return 0;
				});
				update_channel_controls();
				break;
			case "new_connection":
				clear_messages();
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
	$("#controls-container").append(`<select multiple size="${KLogger.channels.length}" id="channelSelector" style="width: 100%;"></select>`);
	update_channel_controls();

	// Severity filtering
	$("#controls-container").append(`<label for="verbositySlider">Verbosity:</label>`);
	$("#controls-container").append(`<input id="verbositySlider" type=range min="0" max="3" value="3" class="slider" oninput="update_verbosity_filter()" style="width: 100%">`);
});