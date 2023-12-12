const dgram = require('dgram');
const http = require('http');
const url = require('url');
const fs = require('fs');
const path = require('path');

const client = dgram.createSocket('udp4');

const serverHost = '127.0.0.1';

const message = 'reload_config';

const buffer = Buffer.from(message);

function sendReloadConfigRequest() {
    client.send(buffer, 0, buffer.length, settings.configReloadTriggerPort, serverHost, (err) => {
        if (err) {
            console.error(`Error sending message: ${err}`);
            client.close();
        }
        else {
            console.log(`Message sent to ${serverHost}:${settings.configReloadTriggerPort}`);
            client.close();
        }
    });

    client.on('message', (msg, rinfo) => {
        console.log(`Received message from server: ${msg} (from ${rinfo.address}:${rinfo.port})`);
    });
}

const server = http.createServer((req, res) => {
    const parsedUrl = url.parse(req.url, true);
    const pathname = parsedUrl.pathname === '/' ? '/index.html' : parsedUrl.pathname;
    const mimeTypes = {
        '.html': 'text/html',
        '.js': 'text/javascript',
        '.css': 'text/css',
    };
    if (pathname == "/applyConfig" && req.method === "POST") {
        const body = [];
        req.on('data', function (data) {
            body.push(data);
        });
        req.on('end', function () {
            const out = Buffer.concat(body);
            try {
                /**
                 * @type {{ ipRange: string[], subnetMask: string, subnetBase: string, myIp: string, leaseTime: string }}
                 */
                const parsed = JSON.parse(out.toString('utf-8'));
                const lines = [];
                const parsedKeys = Object.keys(parsed);
                for (let index = 0; index < parsedKeys.length; index++) {
                    const element = parsedKeys[index];
                    lines.push(`${element}="${parsed[element] instanceof Array ? ("[" + parsed[element].toString() + "]") : parsed[element].toString()}"`);
                }
                lines.push("");
                if (fs.existsSync(settings.pathToDHCPServerConfig))
                    fs.renameSync(settings.pathToDHCPServerConfig, path.dirname(settings.pathToDHCPServerConfig) + `/config.conf_${Date.now()}.bck`);
                fs.writeFileSync(settings.pathToDHCPServerConfig, lines.join("\n"));
                sendReloadConfigRequest();
            }
            catch (error) {
                res.writeHead(500, { 'Content-Type': 'text/plain' });
            }
            res.end();
        });
        return;
    }
    const filePath = path.join(__dirname, 'public', pathname);
    fs.access(filePath, fs.constants.F_OK, (err) => {
        if (err) {
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            res.end('Not Found');
            return;
        }
        fs.readFile(filePath, (err, data) => {
            if (err) {
                res.writeHead(500, { 'Content-Type': 'text/plain' });
                res.end('Internal Server Error');
                return;
            }

            const ext = path.extname(filePath);
            const contentType = mimeTypes[ext] || 'application/octet-stream';
            res.writeHead(200, { 'Content-Type': contentType });
            res.end(data);
        });
    });
});

const settings = {
    port: 7778,
    pathToDHCPServerConfig: "../config.conf",
    configReloadTriggerPort: 8080,
};
const settingsRaw = fs.readFileSync("config.conf", "utf8");
/**
 * @param {string} opt
 */
const parseOption = (opt) => {
    const optVal = opt.slice(opt.indexOf("=") + 1).slice(1).slice(0, -1);
    const booleanValue = (optVal === "true" || optVal === "false") ? optVal === "true" : undefined;
    const optKey = opt.split("=")[0];
    settings[optKey] = (booleanValue == undefined ? optVal : booleanValue);
};

const settingsSplit = settingsRaw.split("\n");
for (let i = 0; i < settingsSplit.length; i++) {
    const element = settingsSplit[i];
    if (element.includes("=\""))
        parseOption(element);
}

server.listen(settings.port, () => {
    console.log(`Server running on http://localhost:${server.address().port}/`);
});
