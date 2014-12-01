"use strict"

importScripts('index.js'); 
console.log("worker ready");
postMessage({"reason": "ready"});

onmessage = function (oEvent) {

    var msg = oEvent.data;

    if (msg.reason = "buildLightmap")
        buildLightmap(msg.data);
    else
        console.log("Worker received unknown message %o", msg);
};



function buildLightmap(contents)
{
        //console.log("contents: %o", contents);
        //var buf = Module._malloc(myTypedArray.length*myTypedArray.BYTES_PER_ELEMENT);
        var fileData = _malloc( contents.byteLength);
        Module.HEAPU8.set(contents, fileData);
        
        base64ptr = _base64_encode(fileData, contents.byteLength);
        var base64str = Pointer_stringify(base64ptr);
        _free(base64ptr);
        postMessage({"reason": "layout", "layout": base64str});
        
        //console.log("raw file data loaded");
        var geo = _parseLayoutStaticMem(fileData, contents.byteLength, 1/30.0);
        _free(fileData);
        
        var geoPtr = _getJsonString(geo);
        var geoStr = Pointer_stringify(geoPtr);
        _free(geoPtr);
        postMessage({"reason": "geometry", "geometry": geoStr});
        
        //console.log("geometry object created at %s", geo);
        var numWalls = _geometryGetNumWalls(geo);
        console.log("geometry consists of %s walls", numWalls);
        var bspRoot = _buildBspTree(_geometryGetWallPtr(geo, 0), numWalls);
        console.log("bspRoot at %s", bspRoot);
        
        for (var wallId = 0; wallId < numWalls; wallId++)
        {
            //console.log("processing wall %s", wallId);
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
        postMessage({"reason": "done"});
        
}

