#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <matrix.c>

int w = 1000, h = 1000;
uint32_t color = 0xFF000000;

uint32_t col(int r, int g, int b){
    uint32_t c = 0xFF000000;
    c |= r << 16;
    c |= g << 8;
    c |= b << 0;
    return c;
}

void setColor(int r, int g, int b){
    color = col(r, g, b);
}

void setPixel(uint32_t *image, int h, int x, int y){
    image[y * h + x] = color;
}

#define PI 3.1415926

float map(float val, float stval, float enval, float newst, float newen){
    float range = enval - stval;
    if(range == 0) return newst;
    float pct = (val - stval) / range;
    float newrange = newen - newst;
    return newrange * pct + newst;
}

void drawLine(uint32_t *image, int h, int x, int y, int x2, int y2){
    float loops = 20;
    for(int i = 0; i < loops; i++){
        float xx = map(i, 0, loops, x, x2);
        float yy = map(i, 0, loops, y, y2);
        setPixel(image, h, xx, yy);
    }
}

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

void blitVertLine(uint32_t *image, int h, int x, int y1, int y2){
    if(x < 0 || x > w) return;
    int start = max(min(y1, y2), 0);
    int end   = min(max(y1, y2), h);
    for(int i = start; i < end; i++){
        setPixel(image, h, x, i);
    }
}

void blitTriangle(uint32_t *image, int h, matrix3 pos){
    if(pos.x.z < 0 || pos.y.z < 0 || pos.z.z < 0) return;
    if(pos.y.x < pos.x.x){
        vec3 temp = pos.x;
        pos.x = pos.y;
        pos.y = temp;
    }

    if(pos.z.x < pos.y.x){
        vec3 temp = pos.y;
        pos.y = pos.z;
        pos.z = temp;
    }

    if(pos.y.x < pos.x.x){
        vec3 temp = pos.x;
        pos.x = pos.y;
        pos.y = temp;
    }

    /*printMatrix3(pos);*/
    /*printf("\n\n");*/

    int x;
    for(x = pos.x.x; x <= pos.y.x; x++){
        blitVertLine(image, h, x,
                map(x, pos.x.x, pos.y.x, pos.x.y, pos.y.y),
                map(x, pos.x.x, pos.z.x, pos.x.y, pos.z.y)
        );
    }
    for(; x < pos.z.x; x++){
        blitVertLine(image, h, x,
                map(x, pos.y.x, pos.z.x, pos.y.y, pos.z.y),
                map(x, pos.x.x, pos.z.x, pos.x.y, pos.z.y)
        );
    }
}

vec3 shape[1000];
int shapesize;

#define beginShape() shapesize = 0;
#define vertex(x, y, z) shape[shapesize++] = (const vec3) {.d = {x, y, z}};
#define endShape() \
{ \
    int i = 0; \
    for(; i < shapesize - 2; i++) { \
        matrix3 matr = {.row = { shape[i], shape[i + 1], shape[i + 2] }}; \
        blitTriangle(pixels, h, matr); \
    } \
}

matrix4 transformMatrix, id4;
vec3 transpose;

void translateShapes(){
    for(int i = 0; i < shapesize; i++) {
        vec4 hom = homogonize3(addVec3(shape[i], transpose));
        hom = multMatVec4(transformMatrix, hom);
        shape[i] = dehomogonize4(hom);
        if(shape[i].z < 0) return;
        shape[i] = scaleVec3(shape[i], 1/shape[i].z);
        shape[i].x = map(shape[i].x, -1, 1, 0, w);
        shape[i].y = map(shape[i].y, -1, 1, h, 0);
    }
}

#define printShape() \
    for(int i = 0; i < shapesize; i++){ \
        printVec3(shape[i]); \
        printf("\n"); \
    }

#define numx 400
#define numy 100
#define maxvar 1
float heightMap[numx][numy] = {};
uint32_t colors[numy] = {};

#define maxx 40
#define minx -maxx

void draw(uint32_t *pixels, int h, float offset, float yoffset, vec3 charPos, int frames){
    transformMatrix = id4;
    transpose = scaleVec3(charPos, -1);
    matrix4 rotateMatrix = {.d = {
        {1, 0, 0, 0},
        {0, cos(yoffset), -sin(yoffset), 0},
        {0, sin(yoffset), cos(yoffset), 0},
        {0, 0, 0, 1}
    }};
    transformMatrix = multMatMat4(transformMatrix, rotateMatrix);
    matrix4 rotateMatrix2 = {.d = {
        {cos(-offset), 0, sin(-offset), 0},
        {0, 1, 0, 0},
        {-sin(-offset), 0, cos(-offset), 0},
        {0, 0, 0, 1}
    }};
    transformMatrix = multMatMat4(transformMatrix, rotateMatrix2);

    for(int i = 0; i < w * h; i++){
        pixels[i] = 0xFF000000;
    }
    for(int z = numy; z >= 1; z--){
        beginShape();
        color = colors[z];
        for(int x = 0; x < numx; x++){
            float xx = map(x, 0, numx, minx, maxx);
            vertex(xx, heightMap[x][z], z + 3);
            vertex(xx, heightMap[x][z - 1], z - 1 + 3);
        }
        translateShapes();
        endShape();
    }
}

uint32_t hsv2rgb(float hue, float sat, float val)
{
    double      p, q, t, ff;
    long        i;
    struct rgb{
        float r, g, b;
    } out;

    hue /= 60.0;
    i = (long) hue;
    p = val * (1.0 - sat);
    q = val * (1.0 - (sat * hue));
    t = val * (1.0 - (sat * (1.0 - hue)));

    switch(i) {
    case 0:
        out.r = val;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = val;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = val;
        out.b = t;
        break;

    case 3:
        out.r = p;
        out.g = q;
        out.b = val;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = val;
        break;
    case 5:
    default:
        out.r = val;
        out.g = p;
        out.b = q;
        break;
    }
    return col(out.r * 255, out.g * 255, out.b * 255);
}

int main(int argc, char **argv){
    (void) argc; (void) argv;
    srand(time(NULL));

    id4 = identity4();

    for(int j = 0; j < numy; j++){
        for(int i = 0; i < numx; i++){
            heightMap[i][j] = map(randomFloat(), 0, 1, -maxvar, maxvar);
            /*heightMap[i][j] = map(j, 0, numy, -maxvar, maxvar);*/
        }
        int sat = map(j, 0, numy, 100, 255);
        colors[j] = hsv2rgb(map(j, 0, numy, 0, 360), .5, 1);
    }

    const vec3 test = {.d = {1, 2, 3}};
    printf("Testt\n");
    printVec3(test);
    printf("\nIMatr\n");
    printMatrix4(identity4());

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

    /*printf("\nTrans\n");*/
    /*printMatrix3(multMatMat3(trans, identityMatrix));*/

    printf("\n\n");
    printVec2(multMatVec2(
        (const matrix2) {.d = {{1, 2}, {3, 4}}},
        (const vec2) {.d = {5, 6}}
    ));
    printf("\n");
    printVec2((const vec2) {.d = {17, 39}});


    /*for(size_t i = 0; i < numspheres; i++){*/
        /*spheres[i] = scaleVec3(randomVec3(), randomFloat()*15 + 5);*/
    /*}*/

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
                    charPos = addVec3(charPos, scaleVec3(diff, .1));
                } break;
                case SDL_SCANCODE_S:{
                    vec3 diff = aimToVec(offset, yoffset);
                    charPos = addVec3(charPos, scaleVec3(diff, -.1));
                } break;
                case SDL_SCANCODE_D:{
                    charPos.x += cos(offset) * .1;
                    charPos.z -= sin(offset) * .1;
                } break;
                case SDL_SCANCODE_A:{
                    charPos.x -= cos(offset) * .1;
                    charPos.z += sin(offset) * .1;
                } break;
                case SDL_SCANCODE_LSHIFT:{
                    charPos.y -= .1;
                } break;
                case SDL_SCANCODE_SPACE:{
                    charPos.y += .1;
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


            /*vec3 diff = aimToVec(offset, yoffset);*/
            printf("View: ");
            /*[>[>printVec3(diff);<]<]*/
            printf("yaw %f, pit %f", offset, yoffset);
            /*printf(" Pos: ", offset, yoffset);*/
            /*printVec3(charPos);*/
            printf("\r");
        }

        uint32_t *pixels;
        int pitch;
        SDL_LockTexture(texture, NULL, (void **) &pixels, &pitch);

        draw(pixels, h, offset, yoffset, charPos, i);

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
