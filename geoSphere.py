#! /usr/bin/python3

from math import sin, cos, sqrt, pi;

def add(a, b):
    return (a[0]+b[0], a[1] + b[1], a[2] + b[2])
#Vector3 add3(const Vector3 a, const Vector3 b, const Vector3 c);
#Vector3 add4(const Vector3 a, const Vector3 b, const Vector3 c, const Vector3 d);
#Vector3 sub(const Vector3 a, const Vector3 b);
#Vector3 mul(const Vector3 b, float a);
#Vector3 neg(const Vector3 v);

def mul(v, f):
    return (v[0]*f, v[1]*f, v[2]*f);

def normalized(v):
    return div_vec3( v, vLen(v));

def vLen(a):
    return sqrt( sum( [x**2 for x in a]));

def div_vec3( v, f):
    return (v[0]/f, v[1]/f, v[2]/f);
    
def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2]);

vertices = {};

def subdivide(v1, v2, v3, numIterations):


#    print ("edge lengths:", vLen( sub(v1, v2)), vLen( sub(v2, v3)), vLen(sub(v3, v1)));
#    exit(0);
#           v1
#          /  \  
#         /    \
#       v12----v31
#       / \    / \
#      /   \  /   \
#     v2----v23----v3
    global vertices;
    if numIterations <= 0:
        return;
    v12 = normalized(div_vec3( add(v1, v2), 2.0));
    v23 = normalized(div_vec3( add(v2, v3), 2.0));
    v31 = normalized(div_vec3( add(v3, v1), 2.0));
    
    if numIterations == 1:
        for v in (v1, v2, v3, v12, v23, v31):
            if not v in vertices: vertices[v] = v;
    else:
        subdivide(v1, v12, v31, numIterations-1);
        subdivide(v2, v12, v23, numIterations-1);
        subdivide(v3, v23, v31, numIterations-1);
        subdivide(v12, v23, v31, numIterations-1);
    
    
v1 = (0,0,1);
v2 = (sin(  90 / 180 * pi), cos(90 / 180 * pi), 0);
v3 = (sin( 180 / 180 * pi), cos(180 / 180 * pi), 0);
v4 = (sin( 270 / 180 * pi), cos(270 / 180 * pi), 0);
v5 = (sin( 360 / 180 * pi), cos(360 / 180 * pi), 0);

#v2 = (sin( 120 / 180 * pi), cos(120 / 180 * pi), 0);
#v3 = (sin( 240 / 180 * pi), cos(240 / 180 * pi), 0);
#v4 = (sin( 360 / 180 * pi), cos(360 / 180 * pi), 0);

numIterations = 5;

subdivide(v1, v2, v3, numIterations);
subdivide(v1, v3, v4, numIterations);
subdivide(v1, v4, v5, numIterations);
subdivide(v1, v5, v2, numIterations);

vertices = list(filter( lambda x: x[2] != 0.0, vertices));

for v in vertices:
#    print("vec3(", v[0], ",", v[1], ",", v[2], "),")
     print("{ .s= {", v[0], ",", v[1], ",", v[2],"}},");
print ( len(vertices), "unique vertices");
#print( vertices);


