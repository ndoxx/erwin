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
function updateChannelFilter() {
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

function selectAllCHannels() {
	$("#channelSelector option").each(function() {
		$(this).prop('selected', true);
	});
	$(`.message`).each(function() {
		$(this).removeClass("chan-hidden");
	});
}

function updateVerbosityFilter() {
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

function updateChannelControls() {
	// Clear channel options
	$("#channelSelector option").remove();

	// For each channel in channels array, create a selectable option
	$('#channelSelector').attr('size',KLogger.channels.length);
	KLogger.channels.forEach(function (channel, index) {
		$("#channelSelector").append(`<option id="option-${channel}" selected="selected">${channel}</option>`);
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

function clearMessages() {
	$("#message-container").empty();
}

function createChannelSubscriptionsHiddenField(subscriptions) {
	$("#message-container").append(`<input type="hidden" id="hidSubscriptions" name="hidSubscriptions" value="${subscriptions}">`);
}

function loadChannelSubscriptions() {
	KLogger.channels = $("#message-container > #hidSubscriptions").val().split(',');
}

function downloadInnerHtml(filename, elId, mimeType) {
    var elHtml = document.getElementById(elId).innerHTML;
    var link = document.createElement('a');
    mimeType = mimeType || 'text/plain';

    link.setAttribute('download', filename);
    link.setAttribute('href', 'data:' + mimeType + ';charset=utf-8,' + encodeURIComponent(elHtml));
    link.click();
    link.remove();
}

function loadContents(contents) {
	clearMessages();
	html = $.parseHTML(contents);
	$("#message-container").append(html);
	loadChannelSubscriptions();
	updateChannelControls();
}

function readSingleFile(e) {
	var file = e.target.files[0];
	if (!file) {
		return;
	}
	var reader = new FileReader();
	reader.onload = function(e) {
		var contents = e.target.result;
		loadContents(contents);
	};
	reader.readAsText(file);
}

function handlePacket(item) {
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
				// Add hidden field in message container so that channel subscriptions can be serialized later on
				createChannelSubscriptionsHiddenField(KLogger.channels);
				updateChannelControls();
				break;
			case "new_connection":
				clearMessages();
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
		    handlePacket(curMatch[1]);
		}
	}

	// ---- FILE CONTROLS ----
	$("#controls-container").append(`<div class="divider"><span></span><span>File</span><span></span></div>`);

	// Export button
	$("#controls-container").append(`<input type=button value="Export" onclick="downloadInnerHtml('erwin.log','message-container','text/html')" style="width: 100%"></input>`)
	// Import button
	$("#controls-container").append(`<input id="fileDialog" type="file" style="display: none" />`);
	$("#controls-container").append(`<input id="btnShowFiledialog" type=button value="Import" style="width: 100%"></input>`);

	$("#btnShowFiledialog").on("click", function() {
		$("#fileDialog").trigger("click");
	});

	document.getElementById('fileDialog').addEventListener('change', readSingleFile, false);


	// ---- FILTER CONTROLS ----
	$("#controls-container").append(`<div class="divider"><span></span><span>Filter</span><span></span></div>`);

	// Create controls for channel filtering
	$("#controls-container").append(`<input type=button value="Select all" onclick="selectAllCHannels()" style="width: 100%"></input>`);
	$("#controls-container").append(`<select multiple size="${KLogger.channels.length}" id="channelSelector" onchange="updateChannelFilter()" style="width: 100%;"></select>`);
	updateChannelControls();

	// Severity filtering
	$("#controls-container").append(`<div id="rangeWithLabel" style="display: flex;">`);
	$("#rangeWithLabel").append(`<label for="verbositySlider" style="flex-grow: 1; margin-right: 5px;">Verbosity</label>`);
	$("#rangeWithLabel").append(`<input id="verbositySlider" type=range min="0" max="3" value="3" class="slider" oninput="updateVerbosityFilter()" style="width: 100%; flex-grow: 3;">`);
});