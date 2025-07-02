function uploadFile() {
    var filePath = document.getElementById("spiffsFilepath").value;
    var upload_path = `/upload/${filePath}`;
    var fileInput = document.getElementById("spiffsFileInput").files;

    if (fileInput.length == 0) {
        alert("No file selected!");
    } else if (filePath.length == 0) {
        alert("File path on server is not set!");
    } else if (filePath.indexOf(' ') >= 0) {
        alert("File path on server cannot have spaces!");
    } else if (filePath[filePath.length-1] == '/') {
        alert("File name not specified after path!");
    } else if (fileInput[0].size > 100*1024) {
        alert("File size must be less than 100KB!");
    } else {
        document.getElementById("spiffsFileInput").disabled = true;
        document.getElementById("spiffsFilepath").disabled = true;
        document.getElementById("spiffsUploadButton").disabled = true;

        var file = fileInput[0];
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4) {
                if (xhr.status == 200) {
                    alert(xhr.responseText);
                } else if (xhr.status == 0) {
                    alert("Server closed the connection abruptly!");
                } else {
                    alert(xhr.status + "Error!\n" + xhr.responseText);
                }
                document.getElementById("spiffsFileInput").disabled = false;
                document.getElementById("spiffsFilepath").disabled = false;
                document.getElementById("spiffsUploadButton").disabled = false;
                menuGo("tools");          
            }
        };
        xhr.open("POST", upload_path, true);
        xhr.send(file);
    }
}

function uploadBinary(fileInput) {
    const abortButton = document.getElementById("otaAbortButton");
    const otaProgressContainer = document.getElementById("otaProgressContainer");
    const progressBar = document.getElementById("otaProgressBar");
    const progressLog = document.getElementById("otaLog");
    const binaryTypeMenu = document.getElementById("binaryType");
    const binaryType = binaryTypeMenu.options[binaryTypeMenu.selectedIndex].value;

    var file = fileInput[0];
    var xhr = new XMLHttpRequest();

    otaProgressContainer.style.display ='table-row-group';
    xhr.onreadystatechange = function() {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                alert(xhr.responseText);
            } else if (xhr.status == 0) {
                alert("Server closed the connection abruptly!");
            } else {
                alert(xhr.status + "Error!\n" + xhr.responseText);
            }
            otaProgressContainer.style.display ='none';
        }
    };
    xhr.upload.addEventListener("loadstart", (event) => {
        progressBar.classList.add("visible");
        progressBar.value = 0;
        progressBar.max = event.total;
        abortButton.disabled = false;
        progressLog.textContent = "Uploading (0%)…";
    });
    xhr.upload.addEventListener("progress", (event) => {
        progressBar.value = event.loaded;
        progressLog.textContent = `Uploading (${((event.loaded / event.total) * 100).toFixed(2)}%)…`;
    });
    xhr.upload.addEventListener("loadend", (event) => {
        progressBar.classList.remove("visible");
        abortButton.disabled = true;
        if (event.loaded !== 0) {
            progressLog.textContent = "Upload finished.";
        }
    });
    function errorAction(event) {
        progressBar.classList.remove("visible");
        progressLog.textContent = `Upload failed: ${event.type}`;
    }
    xhr.upload.addEventListener("error", errorAction);
    xhr.upload.addEventListener("abort", errorAction);
    xhr.upload.addEventListener("timeout", errorAction);

    abortButton.addEventListener("click", () => {xhr.abort();}, { once: true });

    xhr.upload.onprogress = function (event) {
        if (event.lengthComputable) {
            let progress = Math.round(event.loaded / event.total * 100);
            progressBar.style.width = progress + "%",
            progressLog.innerHTML = progress + "%"
        }
    }

    xhr.open("POST", `/ota?binaryType=${binaryType}`, true);
    xhr.send(file);
}