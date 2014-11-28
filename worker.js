"use strict"

importScripts('index.js'); 
console.log("worker ready");

onmessage = function (oEvent) {

    var msg = oEvent.data;

    if (msg.reason = "buildLightmap")
        buildLightmap(msg.data);
    else
        console.log("Worker received unknown message %o", msg);
};



function buildLightmap(contents)
{
        console.log("contents: %o", contents);
        //var buf = Module._malloc(myTypedArray.length*myTypedArray.BYTES_PER_ELEMENT);
        var fileData = _malloc( contents.byteLength);
        Module.HEAPU8.set(contents, fileData);
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
            
            postMessage({"reason": "lightmap", "id": wallId, "res": base64str});
            //console.log("base64 string is '%s'", base64str);
        }            
        _free(bspRoot);
        
}

