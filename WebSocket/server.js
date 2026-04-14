const WebSocket = require('ws');

const wss = new WebSocket.Server({ port: 3006 });

console.log("WS Server running: ws://localhost:3006");

wss.on('connection', (ws) => {
    console.log("Unreal connected");

    // Send Level Name
    ws.send("Login Success");

    ws.on('message', (msg) => {
        console.log("From Unreal:", msg.toString());
    });
});