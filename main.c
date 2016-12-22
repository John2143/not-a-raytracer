#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <matrix.c>

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

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
    if(val >= enval) return newen;
    if(val <= stval) return newst;
    float range = enval - stval;
    if(range == 0) return newst;
    float pct = (val - stval) / range;
    float newrange = newen - newst;
    return newrange * pct + newst;
}

void blitVertLine(uint32_t *image, int h, int x, int y1, int y2){
    if(x < 0 || x > w) return;
    int start = max(min(y1, y2), 0);
    int end   = min(max(y1, y2), h);
    for(int i = start; i < end; i++){
        setPixel(image, h, x, i);
    }
}

void blitTriangle(uint32_t *image, int h, matrix3 pos){
    for(size_t i = 0; i < 3; i++){
        if(pos.row[i].z < 0 ||
           pos.row[i].x < -1 || pos.row[i].x > 1 ||
           pos.row[i].y < -1 || pos.row[i].y > 1) return;
        pos.row[i].x += 1;
        pos.row[i].x *= w/2;
        pos.row[i].y += 1;
        pos.row[i].y *= h/2;
        pos.row[i].y  = h - pos.row[i].y;
    }
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

#define numx 200
#define numy 400
#define scaling 1
#define maxx (scaling * numx / 2)
#define minx -maxx

vec3 shape[numx * 2];
int shapesize;

#define beginShape() shapesize = 0;
#define vertex(x, y, z) shape[shapesize++] = (const vec3) {.d = {x, y, z}};
#define endShape() \
{ \
    translateShapes(); \
    int i = 0; \
    for(; i < shapesize - 2; i++) { \
        matrix3 matr = {.row = { shape[i], shape[i + 1], shape[i + 2] }}; \
        blitTriangle(pixels, h, matr); \
    } \
}

matrix4 transformMatrix, id4;

void translateShapes(){
    for(int i = 0; i < shapesize; i++) {
        vec4 hom = homogonize3(shape[i]);
        hom = multMatVec4(transformMatrix, hom);
        shape[i] = dehomogonize4(hom);
        shape[i] = scaleVec3(shape[i], 1/shape[i].z);
    }
}

#define printShape() \
    for(int i = 0; i < shapesize; i++){ \
        printVec3(shape[i]); \
        printf("\n"); \
    }

float heightMap[numx][numy] = {0};
uint32_t colors[numy] = {0};
void newNoise(float offset);

void draw(uint32_t *pixels, int h, float offset, float yoffset, vec3 charPos, int frames){
    newNoise((numx * numy / 1000 ) * frames / 30.);
    matrix4 rotateMatrixX = {.d = {
        {1, 0, 0, 0},
        {0, cos(yoffset), -sin(yoffset), 0},
        {0, sin(yoffset), cos(yoffset), 0},
        {0, 0, 0, 1}
    }};
    matrix4 rotateMatrixY = {.d = {
        {cos(-offset), 0, sin(-offset), 0},
        {0, 1, 0, 0},
        {-sin(-offset), 0, cos(-offset), 0},
        {0, 0, 0, 1}
    }};

    transformMatrix = id4;
    transformMatrix = multMatMat4(transformMatrix, rotateMatrixX);
    transformMatrix = multMatMat4(transformMatrix, rotateMatrixY);
    transformMatrix = multMatMat4(transformMatrix, homoTrans(scaleVec3(charPos, -1)));

    for(int i = 0; i < w * h; i++){
        pixels[i] = 0xFF000000;
    }
    for(int z = numy; z >= 1; z--){
        beginShape();
        color = colors[z];
        for(int x = 0; x < numx; x++){
            float xx = map(x, 0, numx, minx, maxx);
            vertex(xx * scaling, heightMap[x][z] * scaling, (z + 3) * scaling);
            vertex(xx * scaling, heightMap[x][z - 1] * scaling, (z - 1 + 3) * scaling);
        }
        endShape();
    }
}

uint32_t hsv2rgb(float h, float s, float v )
{
    int i;
    float f, p, q, t;
    float r, g, b;
    if(s == 0) {
        v *= 255;
        return col(v, v, v);
    }
    h /= 60;
    i = floor( h );
    f = h - i;
    p = v * ( 1 - s );
    q = v * ( 1 - s * f );
    t = v * ( 1 - s * ( 1 - f ) );
    switch( i ) {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        default:
            r = v;
            g = p;
            b = q;
    }
    return col(r * 255, g * 255, b * 255);
}

static int SEED = 0;

static int hash[] = {208,34,231,213,32,248,233,56,161,78,24,140,71,48,140,254,245,255,247,247,40,
                     185,248,251,245,28,124,204,204,76,36,1,107,28,234,163,202,224,245,128,167,204,
                     9,92,217,54,239,174,173,102,193,189,190,121,100,108,167,44,43,77,180,204,8,81,
                     70,223,11,38,24,254,210,210,177,32,81,195,243,125,8,169,112,32,97,53,195,13,
                     203,9,47,104,125,117,114,124,165,203,181,235,193,206,70,180,174,0,167,181,41,
                     164,30,116,127,198,245,146,87,224,149,206,57,4,192,210,65,210,129,240,178,105,
                     228,108,245,148,140,40,35,195,38,58,65,207,215,253,65,85,208,76,62,3,237,55,89,
                     232,50,217,64,244,157,199,121,252,90,17,212,203,149,152,140,187,234,177,73,174,
                     193,100,192,143,97,53,145,135,19,103,13,90,135,151,199,91,239,247,33,39,145,
                     101,120,99,3,186,86,99,41,237,203,111,79,220,135,158,42,30,154,120,67,87,167,
                     135,176,183,191,253,115,184,21,233,58,129,233,142,39,128,211,118,137,139,255,
                     114,20,218,113,154,27,127,246,250,1,8,198,250,209,92,222,173,21,88,102,219};

int noise2(int x, int y)
{
    int tmp = hash[(y + SEED) % 256];
    return hash[(tmp + x) % 256];
}

float lin_inter(float x, float y, float s)
{
    return x + s * (y-x);
}

float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3-2*s));
}

float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int+1, y_int);
    int u = noise2(x_int, y_int+1);
    int v = noise2(x_int+1, y_int+1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}

float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x*freq;
    float ya = y*freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for(i=0; i<depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin/div;
}

void newNoise(float offset){
#define maxvar 30
    for(int j = 0; j < numy; j++){
        for(int i = 0; i < numx; i++){
            heightMap[i][j] = map(perlin2d(i, j + offset, .01, 7), 0, 1, -maxvar, maxvar);
        }
    }
}

int main(int argc, char **argv){
    (void) argc; (void) argv;
    srand(time(NULL));

    id4 = identity4();

    for(int j = 0; j < numy; j++){
        float hue = map(j, 0, numy, 0, 360);
        colors[j] = hsv2rgb(hue, .5, 1);
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
    float yoffset = -PI/7;
    vec3 charPos = {0, maxvar, -maxvar};

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
                    charPos = addVec3(charPos, scaleVec3(diff, 1));
                } break;
                case SDL_SCANCODE_S:{
                    vec3 diff = aimToVec(offset, yoffset);
                    charPos = addVec3(charPos, scaleVec3(diff, -1));
                } break;
                case SDL_SCANCODE_D:{
                    charPos.x += cos(offset);
                    charPos.z -= sin(offset);
                } break;
                case SDL_SCANCODE_A:{
                    charPos.x -= cos(offset);
                    charPos.z += sin(offset);
                } break;
                case SDL_SCANCODE_LSHIFT:{
                    charPos.y -= 1;
                } break;
                case SDL_SCANCODE_SPACE:{
                    charPos.y += 1;
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
            /*printf("View: ");*/
            /*[>[>printVec3(diff);<]<]*/
            /*printf("yaw %f, pit %f", offset, yoffset);*/
            /*printf(" Pos: ", offset, yoffset);*/
            /*printVec3(charPos);*/
            /*printf("\r");*/
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
