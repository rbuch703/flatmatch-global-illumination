#ifndef COLOR3_H
#define COLOR3_H

#include <iostream>

class Color3 {
public:
    Color3( double _r, double _g, double _b): r(_r), g(_g), b(_b) {}
    
    Color3 operator+(const Color3 &other) const { return Color3( r+other.r, g+other.g, b+other.b); }
    Color3 operator-(const Color3 &other) const { return Color3( r-other.r, g-other.g, b-other.b); }

    Color3 operator*(const double a) const { return Color3( r*a, g*a, b*a); }
    Color3 operator*(const Color3 &other) const { return Color3(r*other.r, g*other.g, b*other.b);}
    Color3 operator/(const double a) const { return Color3( r/a, g/a, b/a); }

    bool operator==(const Color3 &a) const { return a.r == r && a.g == g && a.b == b;}
public:
    double r, g, b;
};

/*static std::ostream& operator<<(std::ostream &os, const Color3 &col)
{
    os << "(" << col.r << ", " << col.g << ", " << col.b << ")";
    return os;
}*/

#endif
