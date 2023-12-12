const dgram = require('dgram');

const client = dgram.createSocket('udp4');

const serverHost = '127.0.0.1';
const serverPort = 8080;

const message = 'reload_config';

const buffer = Buffer.from(message);

function sendReloadConfigRequest() {
    client.send(buffer, 0, buffer.length, serverPort, serverHost, (err) => {
        if (err) {
            console.error(`Error sending message: ${err}`);
            client.close();
        }
        else {
            console.log(`Message sent to ${serverHost}:${serverPort}`);
            client.close();
        }
    });

    client.on('message', (msg, rinfo) => {
        console.log(`Received message from server: ${msg} (from ${rinfo.address}:${rinfo.port})`);
    });
}
sendReloadConfigRequest();
