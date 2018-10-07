#! /usr/bin/python3

from sys import argv;
import base64;
import json;
import os;
import re;
import subprocess;

if len(argv) != 8:
    print("usage:", argv[0], "<source image> <offer_id> <scale> <latitude> <longitude> <yaw> <level>")
    exit(1)

source_image = argv[1]
offer_id = int(argv[2])
scale = float(argv[3])
latitude = float(argv[4])
longitude = float(argv[5])
yaw = float(argv[6])
level = int(argv[7])


def ensureDirectoryExists(name):
    if os.path.exists(name):
        if not os.path.isdir(name):
            print("Need write access to directory '" + name + "', but a file with the same name already exists");
            exit(0);
    else:
        os.mkdir(name)

ensureDirectoryExists("tiles");

for f in os.listdir("tiles"):
    if re.match( "^tile_\d+.(png|raw)$", f):
        os.remove("tiles/" + f);

call = ["./globalIllumination", source_image, str(scale)];
subprocess.call(call)

template = open("offer_template.json").read();
collisionMap = open("collisionMap.json").read();
geometry = open("geometry.json").read();

template = template.replace("$COLLISION_MAP", collisionMap);
template = template.replace("$LONGITUDE", str(longitude))
template = template.replace("$LATITUDE", str(latitude))
template = template.replace("$LEVEL", str(level))
template = template.replace("$SCALE", str(scale))
template = template.replace("$YAW", str(yaw))
template = template.replace("$LAYOUT", geometry)
template = template.replace("$ROW_ID", str(offer_id))


ensureDirectoryExists("rest");
ensureDirectoryExists("rest/get");
ensureDirectoryExists("rest/get/offer");

f_out = open("rest/get/offer/"+str(offer_id), "w")
f_out.write(template);
f_out.close();


ensureDirectoryExists("rest/get/layout");
f_out = open("rest/get/layout/"+str(offer_id), "wb")
f_out.write( open(source_image, "rb").read())
f_out.close()


ensureDirectoryExists("rest/get/textures");


textures = {}
for f in os.listdir("tiles"):
    m = re.match( "^tile_(\d+).png$", f)
    if m:
        textureId = int(m.group(1));
        textures[textureId] =  str(base64.b64encode(open("tiles/"+f, "rb").read()), "ASCII");

#    for texId, tex in cursor.fetchall():
#        textures[texId] = str( base64.b64encode(tex), "ASCII");

f_out = open("rest/get/textures/" + str(offer_id), "w").write(json.dumps(textures))

