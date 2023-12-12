const applyButton = document.getElementById("applyButton");
applyButton.addEventListener("click", () => {
    const loadingCanvas = document.getElementById('loading');
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
function appendOption(name, type, values = [], placeholder = "", id, onchange) {
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
            input.addEventListener('input', onchange);
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
            select.addEventListener('change', onchange);
            outNodes.push(select);
            break;
        }
        case "array": {
            const arrayBase = document.createElement("div");
            arrayBase.id = "opt-" + name;
            const arrayNewElementButton = document.createElement("button");
            arrayNewElementButton.textContent = "+ New";
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
appendOption("IP Range", "array", undefined, undefined, "ipRange");
appendOption("Subnet mask", "input", undefined, "Enter a valid dotted-decimal notation.", "subnetMask", validateIpAddress);
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