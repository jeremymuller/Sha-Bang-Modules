#include "plugin.hpp"
#include "Vec3.cpp"

struct Block {
    bool isSet = false;
    bool isLocked = false;
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

    Vec getCenter() {
        return pos.plus(size/2.0);
    }

    void reset() {
        int r = floor(randRange(256));
        int g = floor(randRange(256));
        int b = floor(randRange(256));

        rgb = Vec3(r, g, b);
    }

    void distortColor() {
        rgb.x = static_cast<int>(rgb.x + randRange(-20, 20)) % 256;
        rgb.y = static_cast<int>(rgb.y + randRange(-20, 20)) % 256;
        rgb.z = static_cast<int>(rgb.z + randRange(-20, 20)) % 256;
    }

    void setColor(int r, int g, int b) {
        rgb = Vec3(r, g, b);
    }

    void setColor(NVGcolor color) {
        rgb.x = color.r;
        rgb.y = color.g;
        rgb.z = color.b;
    }

    Vec3 getColor() {
        return rgb;
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
        if (!isLocked) { // TODO: locked not working
            rgbVel = rgbVel.plus(rgbAcc);
            rgbVel = rgbVel.limit(maxspeed);
            rgb = rgb.plus(rgbVel);

            edges();

            rgbAcc = rgbAcc.mult(0.0); // reset acceleration
        }
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

struct MarchingCircle {
    Vec pos;
    Vec vel;
    Vec acc;
    float radius;
    int displayWidth = 690;
    int displayHeight = 380;
    int margin = 40;
    float velLimit = 0.8;;

    MarchingCircle() {}

    MarchingCircle(float x, float y) {
        pos = Vec(x, y);
        vel = Vec(randRange(-1.0, 1.0), randRange(-1.0, 1.0));
        acc  = Vec();
        radius = randRange(50.0, 100.0);
    }

    MarchingCircle(float x, float y, float r) {
        pos = Vec(x, y);
        vel = Vec(randRange(-1.0, 1.0), randRange(-1.0, 1.0));
        acc = Vec();
        radius = r;
    }

    void setSize(int width, int height) {
        displayWidth = width;
        displayHeight = height;
    }

    Vec getPos() {
        return pos;
    }

    float getRadius() {
        return radius;
    }

    void setRadius(float _radius) {
        radius = _radius;
    }

    void update() {
        acc.x = randRange(-0.05, 0.05);
        acc.y = randRange(-0.05, 0.05);

        vel = vel.plus(acc);
        vel = vel.normalize();
        vel = vel.mult(velLimit);
        pos = pos.plus(vel);

        checkEdges();
    }

    void checkEdges() {
        // bounce off
        float r = 0.0;
        if (pos.x - r < 0) {
            pos.x = r;
            vel.x *= -0.75;
        } else if (pos.x + r > displayWidth) {
            pos.x = displayWidth - r;
            vel.x *= -0.75;
        }

        if (pos.y - r < 0) {
            pos.y = r;
            vel.y *= -0.75;
        } else if (pos.y + r > displayHeight) {
            pos.y = displayHeight - r;
            vel.y *= -0.75;
        }

        // // wrap: don't think I like this as much
        // if (pos.x + radius < margin)
        //     pos.x = displayWidth + radius;
        // else if (pos.x - radius > displayWidth)
        //     pos.x = margin - radius;

        // if (pos.y + radius < 0) 
        //     pos.y = displayHeight + radius;
        // else if (pos.y - radius > displayHeight)
        //     pos.y = -radius;
    }
};

struct BlockMessage {
    Block block;
    int hertzIndex = 2;
    int colorMode = 0;
};