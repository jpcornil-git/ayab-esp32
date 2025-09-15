const logContent = document.getElementById('log');
let ws = null;
let serverName = window.location.hostname;

// Append status messages to the log panel
function logToConsole(message, type = 'status-message') {
    if (logContent) {
        const p = document.createElement('p');
        p.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
        p.classList.add(type);
        logContent.appendChild(p);
        logContent.scrollTop = logContent.scrollHeight;
    }
}

// Clear the log panel
function clearConsole() {
    if (logContent) {
        logContent.innerHTML = '<p style="color: #888;">Console cleared. Waiting for new messages ...</p>';
    }    
}

function arrayBufferToHexString(arrayBuffer) {
    const uint8Array = new Uint8Array(arrayBuffer);

    const hexString = Array.from(uint8Array).map(byte => {
        return byte.toString(16).padStart(2, '0');
    }).join(',');

    return hexString;
}

function byteToHexString(byte) {
    return byte.toString(16).padStart(2, '0')
}

function IntToHexString(bytes) {
    value = bytes[0] << 8 | bytes[1];
    return value.toString(16).padStart(4, '0')
}

// ---------------------------------------------------------------------------
// AYAB code
// ---------------------------------------------------------------------------

const AYAB_REQINFO  = 0x03;
const AYAB_INDSTATE = 0x84;
const AYAB_REPINFO  = 0xC3;

function sendAyabReqInfo(ws) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const messageBytes = new Uint8Array([AYAB_REQINFO]);
        // SLIP encode the message bytes
        const encodedBytes = slip.encode(messageBytes);
        // Send the SLIP-encoded Uint8Array over the WebSocket
        ws.send(encodedBytes);
        logToConsole(`SLIP Encoded (length ${encodedBytes.length}): ${arrayBufferToHexString(encodedBytes)}`, 'tx-message');
    } else {
        logToConsole('Error: WebSocket is not open, cannot send message.', 'error-message');
    }
}

// Handle message from AYAB firmware
function handleAyabMessage(message) {
    logToConsole(`Binary message (length ${message.length}): ${arrayBufferToHexString(message)}`, 'rx-message');
    switch (message[0]) {
        case AYAB_REPINFO:
                const decodedTagString = new TextDecoder().decode(message.slice(5,5+16));
                ayabFirmwareInfo.innerHTML = `
                <table><tbody>
                <tr><td>API version</td><td>${message[1]}</td></tr>
                <tr><td>Firmware version</td><td>v${message[2]}.${message[3]}.${message[4]}</td></tr>
                <tr><td>Tag</td><td>${decodedTagString}</td></tr>   
                </tbody></table>
                `;                     
            break;
        case AYAB_INDSTATE:
            logToConsole(`[Ayab] IndState (
                error = ${byteToHexString(message[1])},
                state = ${byteToHexString(message[2])},
                hall = (active = ${byteToHexString(message[10])}, left = ${IntToHexString(message.slice(3,5))},right = ${IntToHexString(message.slice(5,7))}),
                carriage = (type ${byteToHexString(message[7])}, position = ${byteToHexString(message[8])}, direction = ${byteToHexString(message[9])}),
                beltshift = ${byteToHexString(message[11])}
            )`, 'rx-message');
            break;
        default:
            logToConsole(`Unhandled message from ayab (${byteToHexString(message[0])})`, 'error-message');
    }
}

// ---------------------------------------------------------------------------
// Main code
// ---------------------------------------------------------------------------

// Handle message from ESP32 firmware (JSON)
function handleJSONMessage(data) {
    logToConsole(`Message (text): ${data}`, 'rx-message');
    try {
        const message = JSON.parse(data);
        switch(message.id) {
            case ws_api.repSystemInfo:
                const esp32FirmwareInfo = document.getElementById('esp32FirmwareInfo');
                esp32FirmwareInfo.innerHTML = `
                <table><tbody>
                <tr><td>ESP-IDF version</td><td>${message.data["esp-idf"].version}</td></tr>
                <tr><td>Firmware version</td><td>${message.data.esp32_firmware.version}</td></tr>
                <tr><td>Date</td><td>${message.data.esp32_firmware.compile_date}</td></tr>
                <tr><td>Time</td><td>${message.data.esp32_firmware.compile_time}</td></tr>
                </tbody></table>
                `;     
                break;
            case ws_api.repGetNetworkParam:
                for (const key in message.data) {
                    switch (key) {
                        case ws_wifi_params.ssid:
                            document.getElementById(`wifi_${ws_wifi_params.ssid}`).value = message.data[key];
                            break;
                        case ws_wifi_params.password:
                            document.getElementById(`wifi_${ws_wifi_params.password}`).value = message.data[key];
                            break;
                        case ws_wifi_params.hostname:
                            document.getElementById(`wifi_${ws_wifi_params.hostname}`).value = message.data[key];
                            break;                        
                    }
                }
                break;
            case ws_api.repSetNetworkParam:
                if (message.result === 0) {
                    logToConsole('WiFi parameters updated.', 'status-message');
                    alert('Wifi settings updated.');
                } else {
                    logToConsole(`WiFi parameters failed (error=${message.result}).`, 'status-message');
                    alert(`Update failed (error=${message.result}).`);
                }                
                break;
            case ws_api.repListFiles:
                container = document.getElementById('littlefs-table');
                fileTable = message.data.list_files;
                if (fileTable.length !== 0) {                  
                    // Add table header
                    let tableHTML = '<table>';
                    tableHTML += '<thead><tr><th><button id="littlefs-deleteButton">Delete</button></th><th>Name</th><th>Size</th></tr></thead>';
                    // Add table body
                    tableHTML += '<tbody>';
                    fileTable.forEach(item => {
                        tableHTML += '<tr>';
                        tableHTML += `<td><input type="checkbox" class="row-checkbox" id="checkbox-${item.name}" data-id="${item.name}"></td>`;
                        if ('url' in item) {
                            tableHTML += `<td><a href="http://${serverName}${item['url']}">${item['name']}</a></td>`;
                        } else {
                            tableHTML += `<td>${item['name']}</td>`;
                        }
                        tableHTML += `<td>${item['size']}</td>`;
                        tableHTML += '</tr>';
                    });
                    tableHTML += '</tbody>';
                    tableHTML += '</table>';
                    tableHTML += '<div id="littlefs-status"></div>';
                    container.innerHTML = tableHTML;
                }

                littlefsDeleteButton = document.getElementById('littlefs-deleteButton');
                littlefsStatus = document.getElementById('littlefs-status');
                littlefsDeleteButton.addEventListener('click', (event) => {
                    const selectedIds = getSelectedRowIds('row-checkbox').map(name => `/${name}`);
                    if (selectedIds.length > 0) {
                        littlefsStatus.textContent = `Selected files: ${selectedIds.join(', ')}`;
                        sendWebSocketMessage({ id: ws_api.reqDeleteFiles, list_files: selectedIds});
                    } else {
                        littlefsStatus.textContent = 'No files selected.';
                    }
                });                
                break;
            case ws_api.repDeleteFiles:
                littlefsStatus = document.getElementById('littlefs-status');
                if (message.result === 0) {
                    littlefsStatus.textContent = 'File(s) deleted.';
                } else {
                    littlefsStatus.textContent = 'Error occured.';
                }
                // Update file table
                sendWebSocketMessage({ id: ws_api.reqListFiles });
                menuGo("tools");
                break;                
            default:
                logToConsole(`Unexpected message id received: ${message.id}`, 'error-message');
        }

    } catch (e) {
        console.error('Failed to parse JSON from WebSocket:', e);
        logToConsole(`Erreur parsing JSON: ${e.message}`, 'error-message');
    }        
}

// Send message via WebSocket
function sendWebSocketMessage(message) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        const jsonMessage = JSON.stringify(message);
        ws.send(jsonMessage);
        logToConsole(`Message (text): ${jsonMessage}`, 'tx-message');
    } else {
        logToConsole('Error: WebSocket is not open, cannot send message.', 'error-message');
    }
}

// Handle WebSocket connection
function connectWebSocket() {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.close();
    }
    const serverNameInput = document.getElementById('serverNameInput');

    //const websocketUrl = 'ws://192.168.1.22/ws';
    const websocketUrl = `ws://${serverNameInput.value}/ws`;

    const wsStatus = document.getElementById('websocketStatus');

    if (wsStatus) wsStatus.textContent = 'Connecting...';
    logToConsole(`Trying to connect to ${websocketUrl}...`, 'status-message');

    slipDecoder = new slip.Decoder(handleAyabMessage);

    try {
        ws = new WebSocket(websocketUrl);

        ws.onopen = () => {
            if (wsStatus) wsStatus.textContent = 'Connected';
            logToConsole('WebSocket connected.', 'status-message');
            document.getElementById('esp32FirmwareInfo').innerHTML = '';
            document.getElementById('ayabFirmwareInfo').innerHTML = '';          
            sendWebSocketMessage({ id: ws_api.reqSystemInfo });
        };

        ws.onmessage = (event) => {
            if (typeof event.data === 'string') {
                handleJSONMessage(event.data);
            } else if (event.data instanceof Blob) {
                event.data.arrayBuffer().then(buffer => {
                    const uint8Array = new Uint8Array(buffer);
                    // Feed the raw bytes into the SLIP decoder
                    message = slipDecoder.decode(uint8Array);
                }).catch(error => {
                    logToConsole(`Error reading Blob: ${error}`, 'error-message');
                });                
            } else if (event.data instanceof ArrayBuffer) {
                const uint8Array = new Uint8Array(event.data);
                // Feed the raw bytes into the SLIP decoder
                message = slipDecoder.decode(uint8Array);
            } else {
                logToConsole('Received unknow websocket data type.', 'error-message');
            }
        };

        ws.onclose = (event) => {
            if (wsStatus) wsStatus.textContent = 'Disconnected';
            logToConsole(`WebSocket disconnected (code: ${event.code}, reason: ${event.reason || 'N/A'})`, 'status-message');
            if (this === ws) {
                ws = null;
            }
        };

        ws.onerror = (error) => {
            if (wsStatus) wsStatus.textContent = 'Connection error';
            logToConsole('WebSocket error: Unable to connect', 'error-message');
            if (this === ws) {
                ws.close();
                ws = null;
            }
        };
    } catch (error) {
        if (wsStatus) wsStatus.textContent = 'Connection failed';
        logToConsole(`Failed to create WebSocket: ${error.message}`, 'status-message');
        ws = null;
    }
}

document.addEventListener('DOMContentLoaded', () => {
    const sidebarContent = document.querySelector('.sidebar-content');
    const mainContent = document.querySelector('.main-content');
    const menuContent = document.querySelector('.menu-bar ul');

    const contents = {
        home: {
            main: `
                <h2>Welcome to ayab-esp32 webapp !</h2>
                <p>This example illustrates usage of websocket to communicate with the ayab server.</p>
                <div id="imageArea"/</div>
            `,
            sidebar: `
                <h3>Connection</h3>
                <p>Status: <span id="websocketStatus"></span></p>
                <label for="serverNameInput" class="form-label">Server</label><input type="text" class="form-input" id ="serverNameInput" name="serverNameInput" placeholder="WebSocket server name">
                <button id="connectWsButton">Connect</button>
                <hr style="margin: 15px 0;">
                <p>Device information: <span id="esp32FirmwareInfo"></span></p>
                <hr style="margin: 15px 0;">
                <button id="ayabReqInfoButton">ReqInfo</button>
                <p>Ayab information: <span id="ayabFirmwareInfo"></span></p>
                <hr style="margin: 15px 0;">
                <input type="file" id="imageUpload" style="display:none;" accept="image/png">
                <button id="openImageFileButton">Open Image File</button>
                <span id="displayImageFile">No file chosen</span>
            `
        },
        wifi: {
            main: `<h2>Wifi setup</h2>
                <p>If you want to connect to your WiFi network, enter corresponding <b>SSID</b> and <b>password</b>.</p> 
                <p><b>Hostname</b> can be used to access the device using .local domain name rather than an IP address, e.g. <a href=http://ayab.local>http://ayab.local</a>.</p>
                <p style="color: red;">When the device cannot connect to a WiFi network, it falls back to Access Point mode which you can access using the SSID = 'AP Ayab WiFi'</p>
            `,            
            sidebar: `
                <form id="wifiForm">
                <label for="ssid" class="form-label">SSID</label><input type="text" class="form-input" id ="wifi_${ws_wifi_params.ssid}" name="ssid" placeholder="WiFI network name (SSID)">
                <label for="password" class="form-label">Password</label><input type="text" class="form-input" id ="wifi_${ws_wifi_params.password}" name="password" placeholder="WiFi password">
                <label for="hostname" class="form-label">Hostname</label><input type="text" class="form-input" id ="wifi_${ws_wifi_params.hostname}" name="hostname" placeholder="Desired hostname">           
                <button type ="submit">Update</button>
                </form>
            `
        },
        tools: {
            main: `
                <table><tbody>
                    <tr>
                        <td colspan="3"><h2>littlefs Directory</h2></td>
                    </tr><tr>
                        <td colspan="2">Select a file</td>
                        <td>
                            <input type="file" id="littlefsFileInput" style="display:none;" style="width:100%;">
                            <button type="button" id="littlefsBrowseButton">Browse</button>        
                        </td>
                    </tr><tr>
                        <td>Set path on server</td>
                        <td><input type="text" id="littlefsFilepath" style="width:100%;"></td>
                        <td><button type="button" id="littlefsUploadButton">Upload</button></td>
                    </tr><tr>
                    </tr>
                </tbody></table>
                <div id="littlefs-table"></div>`,
            sidebar: `
                <h2>Reset</h2>
                <button id="resetESP32Button">ESP32</button>
                <button id="resetRA4M1Button">RA4M1</button>
                <h2>Upload image file</h2>
                <table><tbody>
                    <col width="100px" />
                    <tr>
                        <form action="/ota">
                            <td>OTA Update</td>
                            <td><select id="binaryType">
                                <option value="esp32_app" selected>ESP32 App</option>
                                <option value="esp32_littlefs">ESP32 Filesystem</option>
                                <option value="ra4m1_app">RA4M1 App</option>
                            </select></td>
                    </tr><tr>
                        <td>Select File</td>
                        <td><button type="button" id="otaButton"/>Upload</button>
                            <input type="file" id="otaFileInput"  style="display:none;" accept=".bin">
                        </td>
                        </form>
                    </tr>
                </tbody><tbody id="otaProgressContainer" style="display:none;">
                    <tr>
                        <td><p><output id="otaLog"></output></td>
                        <td><button type="button" id="otaAbortButton">Abort</button></td>
                    </tr><tr>
                        <td colspan="2"><progress id="otaProgressBar" /></td>                        
                    </tr>
                </tbody></table>`
        },
        help: {
            main: `
                <h2>Getting help</h2>
                <h3 id="discord">Discord</h3>
                <p>We do most user support on Discord. Please direct your questions to the <a href="https://discord.gg/jafq6UJaDc">#help</a> channel.</p>
                <h2>File a bug</h2>
                <p>You can file a bug through <a href="https://github.com/AllYarnsAreBeautiful/ayab-desktop/issues">Github</a>. </p>
                <p>First check and see if anyone has already reported your bug, if they have feel free to add more information about how you reproduced the error. If not click the green "New Issue" button and give us as much detail as you can. </p>
                <p>Please include the version of the software that you're working with. If you have the 1.0 release, navigate to <code>AYAB</code> -&gt; <code>About AYAB</code> and include the string there.</p>
                <p>We may reach out for more information if we need it.</p>
                <p>If you don't have a github account then let us know what's going on on Discord.</p></p>`,
            sidebar: `
                <h2>Useful links</h2><ul>
                <li><a href="https://manual.ayab-knitting.com"/>Manuals</a></li>
                <li><a href="https://www.ayab-knitting.com/ayab-software"/>Desktop Software</a></li>
                <li><a href="https://www.ayab-knitting.com/ayab-hardware"/>Interface & Shields</a></li>
                <li>GitHub repositories
                    <ul>
                        <li><a href="https://github.com/AllYarnsAreBeautiful/ayab-patterns"/>Patterns</a></li>
                        <li><a href="https://github.com/AllYarnsAreBeautiful/ayab-desktop"/>Software</a></li>
                        <li><a href="https://github.com/AllYarnsAreBeautiful/ayab-firmware"/>Firmware</a></li>
                        <li><a href="https://github.com/AllYarnsAreBeautiful/ayab-hardware"/>Hardware</a></li>
                    </ul>
                </li></ul>`
        }
    };

    // Main function to update content and attach event listeners
    function updateContent(contentKey) {
        const selectedContent = contents[contentKey];
        if (selectedContent) {
            mainContent.innerHTML = selectedContent.main;
            sidebarContent.innerHTML = selectedContent.sidebar;

            // ---------------------------------------------------------------
            // "Home" Menu
            // ---------------------------------------------------------------
            if (contentKey === 'home') {
                const connectWsButton = document.getElementById('connectWsButton');
                const serverNameInput = document.getElementById('serverNameInput');
                const ayabReqInfoButton = document.getElementById('ayabReqInfoButton');
                const imageUpload = document.getElementById('imageUpload');
                const openImageFileButton = document.getElementById('openImageFileButton');
                const displayImageFile = document.getElementById('displayImageFile');                

                if (serverName.length == 0) {
                    serverName = 'ayab.local';
                }                
                serverNameInput.value = serverName; 

                if (!ws || ws.readyState !== WebSocket.OPEN) {
                    connectWebSocket(); // will issue a reqSystemInfo on open
                } else {
                    sendWebSocketMessage({ id: ws_api.reqSystemInfo });
                }

                serverNameInput.addEventListener('keyup', (event) => {
                    if (event.key === 'Enter') {
                        connectWsButton.click();
                        logToConsole(`Updated ${serverName}.`, 'status-message');
                    }
                });

                connectWsButton.addEventListener('click', () => {
                    serverName = serverNameInput.value;
                    connectWebSocket();
                    logToConsole("Reconnexion WebSocket demandée par l'utilisateur.", 'status-message');
                });

                ayabReqInfoButton.addEventListener('click' , () => {
                    sendAyabReqInfo(ws);
                });

                openImageFileButton.addEventListener('click', (event) => {
                    imageUpload.click();
                });

                imageUpload.addEventListener('change', (event) => {
                    const file = event.target.files[0];
                    displayImageFile.textContent = file;
                    if (file) {
                        if (!file.type.startsWith('image/')) {
                            alert('Select a valid file format (png).');
                            return;
                        }
                        const reader = new FileReader();
                        reader.onload = (event) => {
                            const imageUrl = event.target.result;
                            const imageArea = document.getElementById('imageArea');
                            imageArea.innerHTML = `
                                <h2>Image</h2>
                                <p style="margin-top: 20px; text-align: left;">Image "${file.name}" (size: ${Math.round(file.size / 1024)} KB) successfully loaded.</p>
                                <img src="${imageUrl}" alt="Image" style="max-width: 100%; height: auto; display: block; margin: 0 auto;">
                            `;
                        };
                        reader.onerror = () => { alert("Error while reading file."); };
                        reader.readAsDataURL(file);
                    } else {
                        logToConsole('No file supplied.', 'error-message');
                    }                    
                });
            // ---------------------------------------------------------------                
            // "Wifi" Menu
            // ---------------------------------------------------------------
            } else if (contentKey === 'wifi') {
                // FIXME: Use websocket/Json iso POST ?
                sendWebSocketMessage({ id: ws_api.reqGetNetworkParam });                
                wifiForm.addEventListener('submit', (event) => {
                    event.preventDefault();

                    const wifiForm = document.getElementById('wifiForm');
                    const formData = new FormData(wifiForm);

                    message = {id: ws_api.reqSetNetworkParam};
                    message.data = Object.fromEntries(formData.entries());
                    sendWebSocketMessage(message);
                });
            // ---------------------------------------------------------------                
            // "Tools" Menu
            // ---------------------------------------------------------------                
            } else if(contentKey == 'tools') {
                sendWebSocketMessage({ id: ws_api.reqListFiles });
                const littlefsFileInput = document.getElementById('littlefsFileInput');
                littlefsFileInput.addEventListener('change', (event) => {
                    files=event.target.files;
                    document.getElementById("littlefsFilepath").value = files[0].name;
                });
                                
                const littlefsBrowseButton = document.getElementById('littlefsBrowseButton');
                littlefsBrowseButton.addEventListener('click', (event) => {
                    littlefsFileInput.click();
                });
                
                const littlefsUploadButton = document.getElementById('littlefsUploadButton');
                littlefsUploadButton.addEventListener('click', (event) => {
                    uploadFile();
                });

                const resetESP32Button = document.getElementById('resetESP32Button');
                resetESP32Button.addEventListener('click', (event) => {
                    sendWebSocketMessage({ id: ws_api.reqESP32Reset });
                    // Close websocket
                    ws.close();
                    alert("ESP32 reset, you may have to reconnect");
                    // Return home
                    menuGo("home");                    
                });

                const resetRA4M1Button = document.getElementById('resetRA4M1Button');
                resetRA4M1Button.addEventListener('click', (event) => {
                    sendWebSocketMessage({ id: ws_api.reqRA4M1Reset });
                });

                const otaButton = document.getElementById('otaButton');
                const otaFileInput = document.getElementById('otaFileInput');
                const otaAbortButton = document.getElementById('otaAbortButton');
                otaFileInput.addEventListener('change', (event) => {
                    uploadBinary(event.target.files);
                })
                otaButton.addEventListener('click', (event) => {
                    otaFileInput.click();
                });
                otaAbortButton.addEventListener('click', (event) => {
                    otaFileInput.click();
                });                
            }
            // ---------------------------------------------------------------           
            // "Console" Clear button (always present)
            // ---------------------------------------------------------------                
            if (clearConsoleButton) {
                clearConsoleButton.addEventListener('click', clearConsole);
            }
        }
    }

    // Attach event listener to the main menu and handle page update
    if (menuContent) {
        menuContent.addEventListener('click', (event) => {
            const targetLink = event.target.closest('.menu-bar ul li a');

            if (targetLink) {
                event.preventDefault();

                const allMenuLinks = document.querySelectorAll('.menu-bar ul li a');
                allMenuLinks.forEach(item => item.classList.remove('active'));

                targetLink.classList.add('active');

                const contentKey = targetLink.dataset.content;
                updateContent(contentKey);
            }
        });
    }

    menuGo("home");
});