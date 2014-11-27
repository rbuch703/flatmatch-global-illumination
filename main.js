"use strict"

function handleFileSelect(evt) {
    var files = evt.target.files; // FileList object
    if (files.length != 1)
        return;
        
    // files is a FileList of File objects. List some properties.
    var file = files[0];
    
    var r = new FileReader();
    r.onload = function(e) { 
        var contents = e.target.result;
        //console.log("read %o", contents);
        var data = new Int8Array(contents);

        //var buf = Module._malloc(myTypedArray.length*myTypedArray.BYTES_PER_ELEMENT);
        var fileData = _malloc( contents.byteLength);
        Module.HEAPU8.set(data, fileData);
        console.log("raw file data loaded");
        var geo = _parseLayoutStaticMem(fileData, contents.byteLength, 1/30.0);
        _free(fileData);
        console.log("geometry object created at %s", geo);
        var numWalls = _geometryGetNumWalls(geo);
        console.log("found %s walls", numWalls);
        var bspRoot = _buildBspTree(_geometryGetWallPtr(geo, 0), numWalls);
        console.log("bspRoot at %s", bspRoot);
        
        for (var wallId = 0; wallId < numWalls; wallId++)
        {
            console.log("processing wall %s", wallId);
            var wall = _geometryGetWallPtr(geo, wallId);
            _performAmbientOcclusionNativeOnWall(geo, bspRoot, wall);
            var base64ptr = _saveAsBase64Png( 
                _geometryGetWallPtr(geo, wallId), 
                _geometryGetTexelPtr(geo), 1);
                
            var base64str = Pointer_stringify(base64ptr);
            _free(base64ptr);
            //console.log("base64 string is '%s'", base64str);
            var img = document.createElement("IMG");
            img.src = "data:image/png;base64,"+base64str;
            img.width= 32;
            img.height=32;
            imagesDiv.appendChild(img);
        }            
        _free(bspRoot);
        
    }
    r.readAsArrayBuffer(file);
}

fileInput.addEventListener('change', handleFileSelect, false);

