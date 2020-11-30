// Vec3.cpp
// Extends Vec and gives it ability to calculate 3D vector

#include "plugin.hpp"

struct Vec3 : Vec {
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;

    Vec3() {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // TODO math when needed
    Vec3 plus(Vec3 b) const {
        return Vec3(x+b.x, y+b.y, z+b.z);
    }

    Vec3 minus(Vec3 b) const {
        return Vec3(x-b.x, y-b.y, z-b.z);
    }

    Vec3 mult(float s) const {
        return Vec3(x*s, y*s, z*s);
    }

    Vec3 div(float s) const {
        return Vec3(x/s, y/s, z/s);
    }

    Vec3 limit(float max) const {
        if (magSq() > max*max) {
            Vec3 n = normalize();
            return n.mult(max);
        }
        return Vec3(x, y, z);
    }

    float mag() const {
        return sqrt(x*x + y*y + z*z);
    }

    float magSq() const {
        return (x*x + y*y + z*z);
    }

    Vec3 normalize() const {
        float m = mag();
        if (m > 0) 
            return Vec3(x/m, y/m, z/m);
        else
            return Vec3(x, y, z);
    }

    Vec3 setMag(float len) const {
        Vec3 norm = normalize();
        norm = norm.mult(len);
        return norm;
    }

};