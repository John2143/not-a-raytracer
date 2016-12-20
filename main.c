#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <matrix.c>

void setPixel(uint32_t *image, int h, int x, int y, int r, int g, int b){
    uint32_t pixel = 0xFF000000;
    pixel |= r << 16;
    pixel |= g << 8;
    pixel |= b << 0;
    image[y * h + x] = pixel;
}

#define PI 3.1415926

float map(float val, float stval, float enval, float newst, float newen){
    float range = enval - stval;
    float pct = (val - stval) / range;
    float newrange = newen - newst;
    return newrange * pct + newst;
}

float distsq(const vec3 a, const vec3 b){
    float x, y, z;
    x = a.x - b.x;
    y = a.y - b.y;
    z = a.z - b.z;
    return x*x + y*y + z*z;
}

matrix3 transformMatrix;
matrix3 identityMatrix = {.row ={
    {.d = {1, 0, 0}},
    {.d = {0, 1, 0}},
    {.d = {0, 0, 1}},
}};

vec3 spheres[10] = {
    {0, 0, 10}
};

#define numspheres (sizeof(spheres) / sizeof(*spheres))
vec3 realSpheres[numspheres];

const float rayDist = 30;
const float radius = 2;

float raytraceStep(float yaw, float pitch){
    float dist = 0;
    const float delta = .1;
    vec3 pos = {0, 0, 0};
    vec3 deltaVec = aimToVec(yaw, pitch);

    for(; dist < rayDist; dist += delta){
        for(size_t i = 0; i < numspheres; i++){
            if(distsq(realSpheres[i], pos) < radius*radius){
                return sqrt(dist);
            }
        }
        pos = addVec3(pos, deltaVec);
    }
    return rayDist;
}

float raytraceToSpheres(float yaw, float pitch){
    float min = rayDist;
    for(size_t i = 0; i < numspheres; i++){
        vec3 circle = realSpheres[i];
        float distxz = circle.x - tan(-yaw) * circle.z;
        float distyz = circle.y - tan(pitch) * circle.z;
        /*float disty = 0;*/
        float distsq = distxz * distxz + distyz * distyz;
        float r2 = radius * radius;
        if(distsq < r2){
            float delz1 = sqrt(r2 - distxz * distxz);
            float delz2 = sqrt(r2 - distyz * distyz);
            float newmin = length3(circle);
            if(newmin < min) min = newmin;
        }
    }
    return min;
}

/*#define sgn(x) ((x) < 0 ? -1 : 1)*/

/*float raytraceToSpheresDepthCirc(float yaw, float pitch){*/
    /*float mindist = rayDist;*/
    /*for(size_t i = 0; i < numspheres; i++){*/
        /*yaw = yaw + PI/2;*/
        /*vec3 circ = realSpheres[i];*/
        /*float D = circ.z * (cos(yaw) - circ.x) - circ.x * (sin(yaw) - circ.z);*/
        /*float P = radius * radius - D * D;*/
        /*if(P < 0) continue;*/
        /*vec2 point;*/
        /*float SP = sqrt(P);*/
        /*point.x = D * sin(yaw) - cos(yaw) * SP;*/
        /*point.y = D * cos(yaw) - sin(yaw) * SP;*/
        /*mindist = length2(point);*/
    /*}*/
    /*return mindist;*/
/*}*/

/*float raytraceToSpheres(float yaw, float pitch){*/
    /*return 4;*/
/*}*/

void drawLine(uint32_t *image, int h, int x, int y, int x2, int y2, int r, int g, int b){
    float loops = 20;
    for(int i = 0; i < loops; i++){
        float xx = map(i, 0, loops, x, x2);
        float yy = map(i, 0, loops, y, y2);
        setPixel(image, h, xx, yy, r, g, b);
    }
}

int main(int argc, char **argv){
    (void) argc; (void) argv;
    srand(time(NULL));
    int w = 1000, h = 1000;

    const vec3 test = {.d = {1, 2, 3}};
    printf("Testt\n");
    printVec3(test);
    printf("\nIMatr\n");
    printMatrix3(identityMatrix);

    const matrix3 trans = {.d = {
        {2, 5, 0},
        {4, 3, 0},
        {0, 0, 1},
    }};

    vec3 result, intendedResult;

    result = multMatVec3(trans, test);
    intendedResult = (const vec3) {.d = {12, 10, 3}};
    printf("\nTrans\nIS     ");

    printVec3(result);
    printf("\nSHOULD ");
    printVec3(intendedResult);

    printf("\nTrans\n");
    printMatrix3(multMatMat3(trans, identityMatrix));

    printf("\n\n");
    printVec2(multMatVec2(
        (const matrix2) {.d = {{1, 2}, {3, 4}}},
        (const vec2) {.d = {5, 6}}
    ));
    printf("\n");
    printVec2((const vec2) {.d = {17, 39}});


    for(size_t i = 0; i < numspheres; i++){
        spheres[i] = scaleVec3(randomVec3(), randomFloat()*15 + 5);
    }

    if(SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0){
        printf("SDL failed to start: %s", SDL_GetError());
        return 0;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Test window",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
        0
    );

    if(!window){
        printf("Failed to create error: %s\n", SDL_GetError());
        return 0;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    float aspectRatio = w/h;
    /*float vfov = PI/4;*/
    /*float hfov = aspectRatio * vfov;*/
    float offset = 0;
    float yoffset = 0;
    vec3 charPos = {};

    printf("\n\n");
    SDL_CaptureMouse(1);
    SDL_ShowCursor(0);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    for(int i = 0;;i++){
        for(SDL_Event event; SDL_PollEvent(&event);){
            switch(event.type){
            case SDL_QUIT:
                goto CLEANUP;
            case SDL_KEYDOWN:
                switch(event.key.keysym.scancode){
                case SDL_SCANCODE_LEFT:
                    offset -= .1;
                break;
                case SDL_SCANCODE_RIGHT:
                    offset += .1;
                break;
                case SDL_SCANCODE_UP:
                    yoffset += .1;
                break;
                case SDL_SCANCODE_DOWN:
                    yoffset -= .1;
                break;
                case SDL_SCANCODE_W:{
                    vec3 diff = aimToVec(offset, yoffset);
                    /*charPos = addVec3(charPos, scaleVec3(diff, .1));*/
                    charPos.z -= .1;
                } break;
                case SDL_SCANCODE_Q:
                    goto CLEANUP;
                default:;
                }
            break;
            case SDL_MOUSEMOTION:
                offset += event.motion.xrel * .005;
                yoffset -= event.motion.yrel * .005;
            break;
            }
            if(yoffset < -PI/2) yoffset = -PI/2;
            if(yoffset >  PI/2) yoffset =  PI/2;
            /*printf("View: [%6f %6f]          \r", offset, yoffset);*/
        }

        uint32_t *pixels;
        int pitch;
        SDL_LockTexture(texture, NULL, (void **) &pixels, &pitch);

#define precision 10

        transformMatrix = identityMatrix;
        matrix3 rotateMatrix = {.d = {
            {1, 0, 0},
            {0, cos(yoffset), -sin(yoffset)},
            {0, sin(yoffset), cos(yoffset)},
        }};
        transformMatrix = multMatMat3(transformMatrix, rotateMatrix);
        matrix3 rotateMatrix2 = {.d = {
            {cos(offset), 0, sin(offset)},
            {0, 1, 0},
            {-sin(offset), 0, cos(offset)},
        }};
        transformMatrix = multMatMat3(transformMatrix, rotateMatrix2);

        for(size_t i = 0; i < numspheres; i++){
            realSpheres[i] = spheres[i];
        }

        for(size_t i = 0; i < numspheres; i++){
            realSpheres[i] = addVec3(charPos, realSpheres[i]);
        }

        for(size_t i = 0; i < numspheres; i++){
            realSpheres[i] = multMatVec3(transformMatrix, realSpheres[i]);
        }

        for(int y = 0; y < h; y += precision){
            for(int x = 0; x < w; x += precision){
                float yaw   = map(x, 0, w, -1, 1);
                float pitch = map(y, 0, h, 1, -1);

                yaw = atan(yaw);
                pitch = atan(pitch);

                float dist = raytraceToSpheres(yaw, pitch);
                /*float dist = raytraceStep(yaw, pitch);*/
                unsigned char r = map(dist, 0, 30, 255, 0);
                unsigned char g = map(dist, 0, 30, 255, 0);
                unsigned char b = map(dist, 0, 30, 255, 0);
                for(int i = 0; i < precision; i++){
                    for(int j = 0; j < precision; j++){
                        setPixel(pixels, h, x + i, y + j, r, g, b);
                    }
                }
            }
        }

        /*//Draw unit vectors*/

        /*[>i<]*/
        /*drawLine(pixels, h, 30, 30,*/
                /*map(cos(offset), -1, 1, 10, 50), 30,*/
                /*255, 255, 255);*/
        /*[>j<]*/
        /*drawLine(pixels, h, 30, 30,*/
                /*30, map(cos(yoffset), -1, 1, 50, 10),*/
                /*255, 255, 255);*/

        SDL_UnlockTexture(texture);

        SDL_Rect r;
        r.x = r.y = 0; r.w = w, r.h = h;
        SDL_Rect r2 = r;
        SDL_RenderCopy(renderer, texture, &r, &r2);
        SDL_RenderPresent(renderer);
    }
CLEANUP:

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
