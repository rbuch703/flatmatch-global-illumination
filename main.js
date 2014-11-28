"use strict"

var worker;

fileInput.addEventListener('change', handleFileSelect, false);

function onmessage(oEvent) {

    var msg = oEvent.data;

    if (msg.reason == "lightmap")
    {
        var img = document.createElement("IMG");
        img.src = "data:image/png;base64,"+msg.res;
        img.width= 32;
        img.height=32;
        imagesDiv.appendChild(img);
    } else if (msg.reason == "geometry") {
        var geo = JSON.parse(msg.geometry);
        console.log("received geometry %o", geo);
    } else if (msg.reason == "ready") {
        worker.ready = true;
        worker.startWorking();
    }
    else if (msg.reason == "done")
    {
        worker.terminate();
        worker = null;
        console.log("Job finished, worker terminated");
    } else
        console.log("Called back by the worker with unknown message '%o'!", msg);
};

function startWorking() 
{
    if (this.ready && this.data)
    {
        this.postMessage({"reason": "parseGeometry", "data": this.data});
        delete this.ready;
        delete this.data;
    }
}



function handleFileSelect(evt) {

    if (worker)
        worker.terminate();
        
    worker = new Worker("worker.js");
    worker.onmessage = onmessage;
    worker.startWorking = startWorking;

    var files = evt.target.files; // FileList object
    if (files.length != 1)
        return;
        
    var r = new FileReader();
    r.onload = function(e) { 
        worker.data = new Int8Array(e.target.result);
        worker.startWorking();
    }
    r.readAsArrayBuffer(files[0]);

}



