const applyButton = document.getElementById("applyButton");
const leasesTable = document.getElementsByClassName("leases-table")[0];
const refreshLeaseList = document.getElementById("refreshLeaseList");
applyButton.addEventListener("click", () => {
    const loadingCanvas = applyButton.parentElement.getElementsByTagName("canvas")[0];
    const loadingContext = loadingCanvas.getContext('2d');

    loadingCanvas.style.display = 'inline-block';
    animateLoading(loadingContext);
    const collected = collectOptions();
    appendToConsole("collectOptions: " + JSON.stringify(collected, null, 4));
    fetch("./applyConfig", { method: "POST", body: JSON.stringify(collected) }).then(x => {
        if (x.status == 200) {
            appendToConsole('Changes applied successfully!', 'green');
        }
        else {
            appendToConsole('Got error', 'red');
        }
        loadingCanvas.style.display = 'none';
        loadingContext.clearRect(0, 0, loadingCanvas.width, loadingCanvas.height);
    });
});
function isValidIpv4Addr(ip) {
    return /^(?=\d+\.\d+\.\d+\.\d+$)(?:(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]|[0-9])\.?){4}$/.test(ip);
}
function appendOption(name, type, values = [], placeholder = "", id, onchange = { "change": () => undefined }) {
    if (typeof onchange === "function")
        onchange = {
            "change": onchange,
        };
    const containerForInputs = document.getElementById("containerForInputs");
    const label = document.createElement("label");
    label.for = "opt-" + name;
    label.textContent = name + ":";
    const outNodes = [label];
    switch (type) {
        case "input": {
            const input = document.createElement("input");
            input.name = "opt-" + name;
            input.id = "opt-" + name;
            input.placeholder = placeholder;
            // input.addEventListener("focusout", onchange.focusout);
            // input.addEventListener('input', onchange.change);
            delete Object.assign(onchange, { ["input"]: onchange["change"] })["change"];
            for (const eventName in onchange) {
                input.addEventListener(eventName, onchange[eventName]);
            }
            outNodes.push(input);
            break;
        }
        case "select": {
            const select = document.createElement("select");
            select.name = "opt-" + name;
            select.id = "opt-" + name;
            for (let index = 0; index < values.length; index++) {
                const element = values[index];
                const option = document.createElement("option");
                option.value = element.value;
                option.textContent = element.textContent;
                select.appendChild(option);
            }
            select.addEventListener('change', onchange.change);
            outNodes.push(select);
            break;
        }
        case "array": {
            const arrayBase = document.createElement("div");
            arrayBase.id = "opt-" + name;
            const arrayNewElementButton = document.createElement("button");
            arrayNewElementButton.style.marginBottom = "10px";
            arrayNewElementButton.textContent = "+ New";
            arrayNewElementButton.className = "newElementButton";
            arrayNewElementButton.addEventListener("click", (ev) => {
                /**
                 * @type {typeof arrayNewElementButton}
                 */
                const target = ev.target;
                const newElement = document.createElement("input");
                newElement.style.width = "100px";
                const removeButton = document.createElement("button");
                removeButton.textContent = "- Remove";
                removeButton.addEventListener("click", (ev2) => ev2.target.parentElement.remove());
                const br = document.createElement("br");
                const final_ = document.createElement("span");
                // target.parentElement.appendChild(newElement);
                // target.parentElement.appendChild(removeButton);
                // target.parentElement.appendChild(br);
                final_.append(newElement, removeButton, br);
                target.parentElement.appendChild(final_);
                final_.after(target);
            });
            arrayBase.appendChild(arrayNewElementButton);
            outNodes.push(arrayBase);
            break;
        }
        default:
            break;
    }
    const final = document.createElement("span");
    final.append(...outNodes);
    final.id = "opt-id_" + id;
    toCollect.push(id);
    containerForInputs.append(final);
}
function appendToConsole(message, color) {
    const consoleWindow = document.getElementById('console');
    const span = document.createElement('span');
    span.style.color = color || '#fff';
    span.innerHTML = message;
    consoleWindow.appendChild(span);
    const br = document.createElement("br");
    consoleWindow.appendChild(br);

    consoleWindow.scrollTop = consoleWindow.scrollHeight;
}
function animateLoading(context) {
    const radius = 8;
    let startAngle = 0;
    let endAngle = Math.PI;

    function drawFrame() {
        context.clearRect(0, 0, context.canvas.width, context.canvas.height);
        context.beginPath();
        context.arc(context.canvas.width / 2, context.canvas.height / 2, radius, startAngle, endAngle);
        context.strokeStyle = '#fff';
        context.lineWidth = 2;
        context.stroke();
        context.closePath();

        startAngle += 0.1;
        endAngle += 0.1;

        requestAnimationFrame(drawFrame);
    }

    drawFrame();
}
function validateIpAddress(event) {
    const inputElement = event.target;
    const ipAddress = inputElement.value;

    if (isValidIpv4Addr(ipAddress) || ipAddress == "") {
        inputElement.classList.remove('invalid-input');
    }
    else {
        inputElement.classList.add('invalid-input');
    }
}
const toCollect = [];
function collectOptions() {
    const final = {};
    for (let index = 0; index < toCollect.length; index++) {
        const element = toCollect[index];
        const foundElement = document.getElementById("opt-id_" + element);
        const actualValueElement = foundElement.children[1];
        if (actualValueElement instanceof HTMLInputElement) {
            final[element] = actualValueElement.value;
        }
        else if (actualValueElement instanceof HTMLSelectElement) {
            final[element] = actualValueElement.selectedIndex;
        }
        else if (actualValueElement instanceof HTMLDivElement) {
            final[element] = [];
            const valueCount = actualValueElement.children.length - 1;
            // the last one is the button
            for (let index2 = 0; index2 < valueCount; index2++) {
                const element2 = actualValueElement.children[index2];
                final[element].push(element2.children[0].value);
            }
        }
    }
    return final;
}
function applyOptions(options) {
    for (const key in options) {
        if (Object.prototype.hasOwnProperty.call(options, key)) {
            const value = options[key];
            const element = document.getElementById("opt-id_" + key);
            const actualValueElement = element.children[1];

            if (actualValueElement instanceof HTMLInputElement) {
                actualValueElement.value = value;
            }
            else if (actualValueElement instanceof HTMLSelectElement) {
                actualValueElement.selectedIndex = value;
            }
            else if (actualValueElement instanceof HTMLDivElement) {
                for (let index = 0; index < value.length; index++) {
                    actualValueElement.getElementsByClassName("newElementButton")[0].click();
                    const element2 = actualValueElement.children[index];
                    element2.children[0].value = value[index];
                }
            }
        }
    }
}
function cidr2dotDec(val) {
    return [255, 255, 255, 255]
        .map(() => [...Array(8).keys()]
            .reduce((rst) => (rst * 2 + (val-- > 0)), 0))
        .join('.');
}
appendOption("IP Range", "array", undefined, undefined, "ipRange");
appendOption("Subnet mask", "input", undefined, "Enter a valid dotted-decimal notation or CIDR notation.", "subnetMask", {
    "change": (event) => {
        const inputElement = event.target;
        const ipAddress = inputElement.value;

        if (isValidIpv4Addr(ipAddress) || ipAddress == "" || (ipAddress.length == 3 && (/^\d+$/.test(inputElement.value.slice(1))))) {
            inputElement.classList.remove('invalid-input');
        }
        else {
            inputElement.classList.add('invalid-input');
        }
    },
    "focusout": (event) => {
        const inputElement = event.target;
        const ipAddress = inputElement.value;
        if (ipAddress.startsWith("/")) {
            inputElement.value = cidr2dotDec(parseInt(ipAddress.split("/")[1]));
        }
    },
});
appendOption("Subnet base", "input", undefined, "Enter a valid dotted-decimal notation.", "subnetBase", validateIpAddress);
appendOption("My IP", "input", undefined, "Enter a valid dotted-decimal notation.", "myIp", validateIpAddress);
appendOption("Lease time", "input", undefined, "Enter time for lease expiration, in seconds.", "leaseTime", (event) => {
    const inputElement = event.target;
    if ((/^\d+$/.test(inputElement.value) && parseInt(inputElement.value) <= 31536000) || inputElement.value == "") {
        inputElement.classList.remove('invalid-input');
    }
    else {
        inputElement.classList.add('invalid-input');
    }
});
const leasesTableBody = leasesTable.getElementsByTagName("tbody")[0];
function appendTableData(tableBody, valuesArray) {
    for (let index = 0; index < valuesArray.length; index++) {
        const obj = valuesArray[index];
        const tableRow = document.createElement("tr");
        const valuesKeys = Object.keys(obj);
        for (let index2 = 0; index2 < valuesKeys.length; index2++) {
            const keyEl = valuesKeys[index2];
            const tableData = document.createElement("td");
            tableData.textContent = obj[keyEl];
            tableData.setAttribute("val", keyEl);
            tableRow.appendChild(tableData);
        }
        tableBody.appendChild(tableRow);
    }
}
refreshLeaseList.addEventListener("click", () => {
    const controller = new AbortController();
    const loadingCanvas = refreshLeaseList.parentElement.getElementsByTagName("canvas")[0];
    const loadingContext = loadingCanvas.getContext('2d');

    loadingCanvas.style.display = 'inline-block';
    animateLoading(loadingContext);

    const timeoutId = setTimeout(() => {
        controller.abort();
        loadingCanvas.style.display = 'none';
        loadingContext.clearRect(0, 0, loadingCanvas.width, loadingCanvas.height);
        appendToConsole("Timed out, is the server running?", "red");
    }, 5000);
    fetch("./getLeases", { signal: controller.signal }).then(async x => {
        clearTimeout(timeoutId);
        const raw = await x.arrayBuffer();
        const rawUint8 = new Uint8Array(raw);
        if (String.fromCharCode(rawUint8[0]) != "x")
            return;
        const final_ = [{
            ipAddress: "",
            macAddress: "",
            startTimestamp: "",
            duration: "",
            hostName: "",
            startIndx: -1,
        }];
        final_.length = rawUint8.filter(x2 => x2 == "\n".charCodeAt(0)).length;
        for (let index = 0; index < final_.length; index++) {
            if (final_.length - 1 != index)
                final_[index + 1] = Object.assign({}, final_[index]);
        }
        let commaOffset = 0;
        let current = 0;
        const t = new TextDecoder();
        for (let index = 0; index < rawUint8.length; index++) {
            const final = final_[current];
            if (final == undefined)
                break;
            const element = rawUint8[index];
            const asChar = String.fromCharCode(element);
            if (commaOffset == 0 && asChar == "x") {
                final.startIndx = index;
                index += 1;
                continue;
            }
            // duration
            if (commaOffset == 0 && final.startIndx != -1) {
                final.duration = rawUint8.filter((v, i) => i >= index && i < rawUint8.indexOf(",".charCodeAt(0), index + 1)).map(x2 => String.fromCharCode(x2)).join("");
                commaOffset++;
                index += 2;
                continue;
            }
            // start time
            if (commaOffset == 1 && final.duration != "") {
                final.startTimestamp = rawUint8.filter((v, i) => i >= index && i < rawUint8.indexOf(",".charCodeAt(0), index + 1)).map(x2 => String.fromCharCode(x2)).join("");
                commaOffset++;
                index += 11;
                continue;
            }
            // ip addr
            if (commaOffset == 2 && final.startTimestamp != "") {
                final.ipAddress = `${rawUint8[index]}.${rawUint8[index + 1]}.${rawUint8[index + 2]}.${rawUint8[index + 3]}`;
                commaOffset++;
                index += 4;
                continue;
            }
            // mac addr
            if (commaOffset == 3 && final.ipAddress != "") {
                final.macAddress = `${rawUint8[index]}:${rawUint8[index + 1]}:${rawUint8[index + 2]}:${rawUint8[index + 3]}:${rawUint8[index + 4]}:${rawUint8[index + 5]}`.split(":").map(x2 => parseInt(x2).toString(16)).join(":");
                commaOffset++;
                index += 6;
                continue;
            }
            // host name
            if (commaOffset == 4 && final.macAddress != "") {
                // final.hostName = rawUint8.filter((v, i) => i >= index && i < rawUint8.indexOf("\n".charCodeAt(0))).map(x2 => String.fromCharCode(x2)).join("");
                final.hostName = t.decode(rawUint8.filter((v, i) => i >= index && i < rawUint8.indexOf("\n".charCodeAt(0), final.startIndx)));
                commaOffset++;
                // index +=;
                continue;
            }
            if (asChar == "\n") {
                current++;
                commaOffset = 0;
            }
        }
        console.log(final_);
        final_.forEach(x2 => x2.startTimestamp = Number.parseInt(x2.startTimestamp));
        final_.forEach(x2 => delete x2.startIndx);
        Array.from(leasesTableBody.children).filter(x2 => x2.remove());
        appendTableData(leasesTableBody, final_);

        loadingCanvas.style.display = 'none';
        loadingContext.clearRect(0, 0, loadingCanvas.width, loadingCanvas.height);
    });
});
/**
 * @param {string} opt
 */
const parseOption = (opt, out) => {
    const optVal = opt.slice(opt.indexOf("=") + 1).slice(1).slice(0, -1);
    let arrayValue = false;
    if (optVal.startsWith("[") && optVal.endsWith("]"))
        arrayValue = true;
    const optKey = opt.split("=")[0];
    out[optKey] = arrayValue ? JSON.parse(optVal) : optVal;
};
fetch("/getConfig").then(x => x.text()).then(x => {
    const settingsSplit = x.split("\n");
    const final = {};
    for (let i = 0; i < settingsSplit.length; i++) {
        const element = settingsSplit[i];
        if (element.includes("=\""))
            parseOption(element, final);
    }
    applyOptions(final);
});
