#include "plugin.hpp"
#include "Vec3.cpp"

struct Block {
    bool isSet = false;
    Vec pos;
    Vec3 rgb;
    Vec3 rgbVel = Vec3();
    Vec3 rgbAcc = Vec3(); 

    float size;
    float maxspeed;
    float maxforce;
    float radius;

    float sepInput = 0.0;
    float aliInput = 0.0;
    float cohInput = 0.0;

    Block() {}

    Block(float x, float y, float s) {
        isSet = true;
        pos = Vec(x, y);

        int r = floor(randRange(256));
        int g = floor(randRange(256));
        int b = floor(randRange(256));

        rgb = Vec3(r, g, b);
        size = s;

        maxspeed = 1;
        maxforce = 0.01;
        radius = 15;
    }

    void reset() {
        int r = floor(randRange(256));
        int g = floor(randRange(256));
        int b = floor(randRange(256));

        rgb = Vec3(r, g, b);
    }

    void setColor(int r, int g, int b) {
        rgb = Vec3(r, g, b);
    }

    void flock(Block blocks[], int size) {
        Vec3 sep = separate(blocks, size);
        Vec3 ali = align(blocks, size);
        Vec3 coh = cohesion(blocks, size);

        sep = sep.mult(1.1 + sepInput);
        ali = ali.mult(1.0 + aliInput);
        coh = coh.mult(1.8 + cohInput);

        applyForce(sep);
        applyForce(ali);
        applyForce(coh);
    }

    Vec3 seek(Vec3 target) {
        Vec3 desired = target.minus(rgb);
        desired = desired.setMag(maxspeed);
        Vec3 steer = desired.minus(rgbVel);
        steer = steer.limit(maxforce);
        return steer;
    }

    Vec3 separate(Block blocks[], int size) {
        Vec3 sum = Vec3();
        int count = 0;
        Vec3 steer = Vec3();

        for (int i = 0; i < size; i++) {
            if (blocks[i].isSet) {
                Vec3 diff = rgb.minus(blocks[i].rgb);
                diff = diff.normalize();
                sum = sum.plus(diff);
                count++;
            }
        }

        if (count > 0) {
            sum = sum.div(count);
            sum = sum.normalize();
            sum = sum.mult(maxspeed);

            steer = sum.minus(rgbVel);
            steer = steer.limit(maxforce);
        }
        return steer;
    }

    Vec3 align(Block blocks[], int size) {
        Vec3 sum = Vec3();
        int count = 0;
        
        for (int i = 0; i < size; i++) {
            if (blocks[i].isSet) {
                sum = sum.plus(blocks[i].rgbVel);
                count++;
            }
        }

        if (count > 0) {
            sum = sum.div((float)count);
            sum = sum.normalize();
            sum = sum.mult(maxspeed);
            Vec3 steer = sum.minus(rgbVel);
            steer = steer.limit(maxforce);
            return steer;
        } else {
            return Vec3();
        }
    }

    Vec3 cohesion(Block blocks[], int size) {
        Vec3 sum = Vec3();
        int count = 0;

        for (int i = 0; i < size; i++) {
            if (blocks[i].isSet) {
                sum = sum.plus(blocks[i].rgb);
                count++;
            }
        }

        if (count > 0) {
            sum = sum.div((float)count);
            return seek(sum);
        } else {
            return Vec3();
        }
    }

    void update() {
        rgbVel = rgbVel.plus(rgbAcc);
        rgbVel = rgbVel.limit(maxspeed);
        rgb = rgb.plus(rgbVel);

        edges();

        rgbAcc = rgbAcc.mult(0.0); // reset acceleration
    }

    void applyForce(Vec3 force) {
        rgbAcc = rgbAcc.plus(force);
    }

    void edges() {
        if (rgb.x > 255) {
            rgb.x = 255;
            rgbVel.x *= -1;
        } else if (rgb.x < 0) {
            rgb.x = 0;
            rgbVel.x *= -1;
        }

        if (rgb.y > 255) {
            rgb.y = 255;
            rgbVel.y *= -1;
        } else if (rgb.y < 0) {
            rgb.y = 0;
            rgbVel.y *= -1;
        }

        if (rgb.z > 255) {
            rgb.z = 255;
            rgbVel.z *= -1;
        } else if (rgb.z < 0) {
            rgb.z = 0;
            rgbVel.z *= -1;
        }
    }
};