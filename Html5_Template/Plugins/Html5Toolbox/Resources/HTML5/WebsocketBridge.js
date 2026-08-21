var WebsocketBridge = {
    socket: null,
    onMessageCallback: null,
    onOpenCallback: null,
    onErrorCallback: null,

    connect: function(url, roomId) {
        console.log("Connecting to " + url);
        try {
            this.socket = new WebSocket(url);
        } catch (e) {
            console.error("WebSocket creation failed: " + e);
            if (this.onErrorCallback) {
                this.onErrorCallback("WebSocket creation failed");
            }
            return;
        }

        this.socket.onopen = () => {
            console.log("WebSocket connected!");
            const joinMsg = JSON.stringify({
                type: "JOIN_ROOM",
                roomId: roomId || "default",
                clientId: this.generateId()
            });
            this.socket.send(joinMsg);
            if (this.onOpenCallback) {
                this.onOpenCallback();
            }
        };

        this.socket.onmessage = (event) => {
            if (this.onMessageCallback) {
                this.onMessageCallback(event.data);
            }
        };

        this.socket.onerror = (error) => {
            console.error("WebSocket error: " + error);
            if (this.onErrorCallback) {
                this.onErrorCallback("WebSocket error: " + error);
            }
        };

        this.socket.onclose = () => {
            console.log("WebSocket closed");
        };
    },

    send: function(message) {
        if (this.socket && this.socket.readyState === WebSocket.OPEN) {
            this.socket.send(message);
        } else {
            console.warn("WebSocket not ready, message not sent");
        }
    },

    disconnect: function() {
        if (this.socket) {
            this.socket.close();
        }
    },

    generateId: function() {
        return Math.random().toString(36).substring(2, 15);
    }
};

if (typeof Module !== 'undefined') {
    Module.WebsocketBridge = WebsocketBridge;
}