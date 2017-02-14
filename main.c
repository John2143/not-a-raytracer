#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "threadpool.h"
#include "matrix.c"
#include "util.c"

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#define NUM_THREADS 8

threadpool_t *threadpool;

//Window sizes
int w = 1000, h = 1000;

//the current drawing color
uint32_t color = 0xFF000000;

//deprecated, just invoke directly
void setColor(int r, int g, int b){
    color = col(r, g, b);
}

//deprecated, but works fine
void setPixel(uint32_t *image, int x, int y){
    image[y * h + x] = color;
}

//TAU is define in matrix.c as 2PI
#define PI 3.1415926

void blitVertLine(uint32_t *image, int x, int y1, int y2){
    //Prevent segfaulting for bad calls
    // (mabye it should panic?)
    if(x < 0 || x > w) return;

    //y1 should be the top one
    if(y2 < y1){
        int t = y1;
        y1 = y2;
        y2 = t;
    }

    //number of pixels to blit
    int ctr = y2 - y1;
    //Starting pixel
    uint32_t *ptr = &image[h * y1 + x];
    while(ctr--){
        //blit
        *ptr = color;
        //Move down one pixel
        ptr += h;
    }
}

void blitTriangle(uint32_t *image, matrix3 pos){
    for(size_t i = 0; i < 3; i++){
        //Dont draw triangles that are even partially offscreen (todo)
        if(pos.row[i].z < 0 ||
           pos.row[i].x < -1 || pos.row[i].x > 1 ||
           pos.row[i].y < -1 || pos.row[i].y > 1) return;

        //Translate the screen coordinates to pixel positions
        pos.row[i].x += 1;
        pos.row[i].x *= w/2;
        pos.row[i].y += 1;
        pos.row[i].y *= h/2;
        pos.row[i].y  = h - pos.row[i].y;
    }

    //'sort' the triangle so the rows go from left to right
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


    //TODO remove map
    int x;

    //Draw the left half, interpolating between vertexes
    for(x = pos.x.x; x <= pos.y.x; x++){
        blitVertLine(image, x,
            map(x, pos.x.x, pos.y.x, pos.x.y, pos.y.y),
            map(x, pos.x.x, pos.z.x, pos.x.y, pos.z.y)
        );
    }

    //Draw the right half the same way
    for(; x < pos.z.x; x++){
        blitVertLine(image, x,
            map(x, pos.y.x, pos.z.x, pos.y.y, pos.z.y),
            map(x, pos.x.x, pos.z.x, pos.x.y, pos.z.y)
        );
    }
}

#define numx 250
#define numy 250
#define scaling 1
#define maxx (numx / 2)
#define minx -maxx

//Hold the individual triangle strips
vec3 shape[numx * 2];
size_t shapesize; //number of shapes
//Holds a matrix of viewmapped triangles
matrix3 allTriangles[numx * numy * 2];
size_t triangleNum;

//reset the triangle list
#define beginMapping() triangleNum = 0;
//Reset the shape list
#define beginShape() shapesize = 0;
//Add a vertex to the triangle strip
#define vertex(x, y, z) shape[shapesize++] = (const vec3) {.d = {x, y, z}};
//move all the vertexes via the projection and view map, then add them all
//in sequence to the triangle list
#define endShape() \
{ \
    translateShapes(); \
    size_t i = 0; \
    for(; i < shapesize - 2; i++) { \
        matrix3 matr = {.row = {shape[i], shape[i + 1], shape[i + 2]}}; \
        allTriangles[triangleNum++] = matr; \
    } \
}

matrix4 transformMatrix, id4;

void translateShapes(){
    for(size_t i = 0; i < shapesize; i++) {
        //Translate all vertexes into their actual world coordinates
        vec4 hom = homogonize3(shape[i]);
        hom = multMatVec4(transformMatrix, hom);
        shape[i] = dehomogonize4(hom);

        //Translate the vertexes into screen coordinates. This is done
        //by dividng every vertex by their z coord in order to get screen
        //coordinate (-1 to 1)
        //Blit triange will take this range and produce pixel coordinates
        //Rendershapes will perform depth mapping using the z coordinate
        float z = shape[i].z;
        shape[i].x /= z;
        shape[i].y /= z;
    }
}

#define printShape() \
    for(int i = 0; i < shapesize; i++){ \
        printVec3(shape[i]); \
        printf("\n"); \
    }

/*int cmptriangles(const matrix3 *a, const matrix3 *b){*/
int cmptriangles(const void *a, const void *b){
    //Compare z coordinates
    float fa = ((const matrix3 *) a)->x.z;
    float fb = ((const matrix3 *) b)->x.z;

    return (fa < fb) - (fa > fb);
}

int hasSorted = 0;
uint32_t colors[numy * numx] = {0};
void renderShapes(uint32_t *pixels, vec3 charpos){
    //We do not need to depth map if the triangles are already back to front
    if(charpos.z < 1) goto SKIP_SORT;
    qsort(allTriangles, triangleNum, sizeof(matrix3), cmptriangles);

SKIP_SORT:

    for(size_t i = 0; i < triangleNum; i++){
        //Draw every triangle
        //TODO make color correspond to row again now that depth mapping works
        color = colors[i / 2];
        blitTriangle(pixels, allTriangles[i]);
    }
}

float heightMap[numx][numy];
void nextNoise();

void draw(uint32_t *pixels, float offset, float yoffset, vec3 charPos, int frames){
    //Move world by some amount
    nextNoise();

    //Math magic ahead
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
    /*transformMatrix = scaleMatrix4(transformMatrix, scaling);*/
    transformMatrix = multMatMat4(transformMatrix, rotateMatrixX);
    transformMatrix = multMatMat4(transformMatrix, rotateMatrixY);
    transformMatrix = multMatMat4(transformMatrix, homoTrans(scaleVec3(charPos, -1)));
    //Magic over: A vector as been created that can take any vec4 and move it
    //to where it should be for the camera (facing the z+ axis at 0,0,0)

    for(int i = 0; i < w * h; i++){
        //Black with alpha
        pixels[i] = 0xFF000000;
    }

    //Clear the triangle array
    beginMapping();
    for(int z = numy - 1; z >= 1; z--){
        //Start a new triangle strip
        beginShape();
        for(int x = 0; x < numx; x++){
            //Calculate world value x
            float xx = x * (maxx - minx) / (float) numx + minx;
            //vertex(x, y, z) will be automatically transformed after endshape
            //into a projection map (retaining Z coordinate)
            //The strip looks like this because each dot is connected.
            //inside the loop produces one top dot and one bottom dot, then
            //the next iteration will connect them.
            // * * * * *
            // |/|/|/|/|
            // * * * * *
            //The endShape function connects the top and bottom rows
            //automatically producing actual triangles instead of just a zaggy
            //line
            // *-*-*-*-*
            // |/|/|/|/|
            // *-*-*-*-*
            vertex(xx, heightMap[x][z], (z + 3));
            vertex(xx, heightMap[x][z - 1], (z + 2));
        }
        endShape();
    }
    //Charpos is passed to disable z depth testing in some scenarios
    renderShapes(pixels, charPos);
}

#define maxvar 50

//Generate the heightmap at an offset using perlin noise
//This can be optimized extremely well by reducing the number of calls to noise
void newNoise(float offset){
    for(int j = 0; j < numy; j++){
        for(int i = 0; i < numx; i++){
            heightMap[i][j] = map(perlin2d(i, j + offset, .01, 7), 0, 1, -maxvar, maxvar);
        }
    }
}

void nextNoise(){
    static float offset = 0;
    //check if arrays can be safely memcpy'd to themselves TODO

    //shift array toward the origin
    for(int i = 0; i < numx; i++){
        for(int j = 0; j < numy - 1; j++){
            heightMap[i][j] = heightMap[i][j + 1];
        }
    }

    //Generate new terrain at the far side
    for(int i = 0; i < numx; i++){
        heightMap[i][numy - 1] = map(perlin2d(i, numy - 1 + offset, .01, 7), 0, 1, -maxvar, maxvar);
    }
    offset += 1;
}

#define numKeys 300
char keysDown[numKeys] = {};

int main(int argc, char **argv){
    (void) argc; (void) argv;
    srand(time(NULL));
    newNoise(0);

    //Initialize an identity matrix that will be used often
    id4 = identity4();

    //Define the color for each triangle strip
    for(int j = 0; j < numy * numx; j++){
        float hue = map(j, 0, numx * numy, 0, 360);
        colors[j] = hsv2rgb(hue, .5, 1);
    }


#if TESTMATRIX
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

    if(1) return;
#endif

    threadpool = threadpool_create(NUM_THREADS, NUM_THREADS << 1, 0);

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

    //TODO calculate viewmatrix using below variables
    /*float aspectRatio = w/h;*/
    /*float vfov = PI/4;*/
    /*float hfov = aspectRatio * vfov;*/

    //Camera details: yaw is measured with 0 = facing +z, PI/2 facing +x.
    //This means most times where I use yaw I will actually use -yaw because
    //sin works with clockwise being negative, not positive as many operations
    //expect
    //
    //offset = yaw, yoffset = pitch
    //roll is always 0
    float offset = 0;
    float yoffset = -PI/7;
    //Pos in 3d space
    vec3 charPos = {.d = {0, maxvar, -maxvar}};

    printf("\n\n");
    /*SDL_CaptureMouse(1);*/
    SDL_ShowCursor(0);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    for(int i = 0;;i++){
        char keyDown = 1;
        for(SDL_Event event; SDL_PollEvent(&event);){
            switch(event.type){
            case SDL_QUIT:
                goto CLEANUP;
            case SDL_KEYUP: keyDown = 0;
            case SDL_KEYDOWN:{
                int code = event.key.keysym.scancode;
                if(code >= numKeys) break;
                keysDown[code] = keyDown;
            } break;
            case SDL_MOUSEMOTION:
                offset += event.motion.xrel * .005;
                yoffset -= event.motion.yrel * .005;
            break;
            }

            //Clamp pitch vector to prevent confusing movement
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

        if(keysDown[SDL_SCANCODE_W]){
            vec3 diff = aimToVec(offset, yoffset);
            charPos = addVec3(charPos, scaleVec3(diff, 1));
        }
        if(keysDown[SDL_SCANCODE_S]){
            vec3 diff = aimToVec(offset, yoffset);
            charPos = addVec3(charPos, scaleVec3(diff, -1));
        }
        if(keysDown[SDL_SCANCODE_D]){
            charPos.x += cos(offset);
            charPos.z -= sin(offset);
        }
        if(keysDown[SDL_SCANCODE_A]){
            charPos.x -= cos(offset);
            charPos.z += sin(offset);
        }
        if(keysDown[SDL_SCANCODE_Z] || keysDown[SDL_SCANCODE_LSHIFT]){
            charPos.y -= 1;
        };
        if(keysDown[SDL_SCANCODE_SPACE]){
            charPos.y += 1;
        };
        if(keysDown[SDL_SCANCODE_Q]){
            goto CLEANUP;
        }

        //obtain a raw pointer to the video memory
        uint32_t *pixels;
        int pitch;
        SDL_LockTexture(texture, NULL, (void **) &pixels, &pitch);

        //TODO h has been made global
        // parameter i is the framecounter
        draw(pixels, offset, yoffset, charPos, i);

        SDL_UnlockTexture(texture);

        //Blit the texture to the renderer (send to graphics card) and render
        SDL_Rect r;
        r.x = r.y = 0; r.w = w, r.h = h;
        SDL_Rect r2 = r;
        SDL_RenderCopy(renderer, texture, &r, &r2);
        SDL_RenderPresent(renderer);
    }
CLEANUP:

    threadpool_destroy(threadpool, threadpool_graceful);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
