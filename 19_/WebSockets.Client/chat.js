//
// chat.js: websocket chatting program
//
window.onload = function () {

    // Create a reference for the required DOM elements.
    var nameView 	= document.getElementById("name-view");
    var textView 	= document.getElementById("text-view");
    var buttonSend 	= document.getElementById("send-button");
    var buttonStop 	= document.getElementById("stop-button");
    var label 		= document.getElementById("status-label");
    var chatArea 	= document.getElementById("chat-area");

    // Connect to the WebSocket server!
    var socket 		= new WebSocket("ws://localhost:9999");
    // var socket 		= new WebSocket("ws://192.168.0.12:9999");
		
    // WebSocket onopen event.
    socket.onopen = function (event) {
        label.innerHTML = "Connection open";
    }

    // WebSocket onmessage event.
    socket.onmessage = function (event) {
        if (typeof event.data === "string") {
            // Display message.
            chatArea.innerHTML = chatArea.innerHTML + "<p>" + event.data + "</p>";

            // Scroll to bottom.
            chatArea.scrollTop = chatArea.scrollHeight;
        }
    }
	
    // WebSocket onclose event.
    socket.onclose = function (event) {
        var code 	 = event.code;
        var reason	 = event.reason;
        var wasClean = event.wasClean;

        if (wasClean) {
            label.innerHTML = "Connection closed normally.";
        }
        else {
            label.innerHTML = "Connection closed with message: " + reason + " (Code: " + code + ")";
        }
    }

     // WebSocket onerror event.
    socket.onerror = function (event) {
        label.innerHTML = "Error: " + event;
    }

    // Disconnect and close the connection.
    buttonStop.onclick = function (event) {
        if (socket.readyState == WebSocket.OPEN) {
            socket.close();
        }
    }

    // Send the message and empty the text field.
    buttonSend.onclick = function (event) {
		sendText();
    }

    // Send the message and empty the text field.
    textView.onkeypress = function (event) {
        if (event.keyCode == 13) {
			sendText();
		}
    }
	
    // Send a text message using WebSocket.
    function sendText() {
        if (socket.readyState == WebSocket.OPEN) {
            var message = "[" + nameView.value + "] " + textView.value;
            socket.send(message);

            textView.value = "";
        }
    }
}