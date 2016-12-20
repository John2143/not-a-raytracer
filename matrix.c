#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define MATR(DIM, indicies) \
 \
typedef union vec ## DIM{ \
    struct { \
        float indicies; \
    }; \
    float d[DIM]; \
} vec ## DIM; \
 \
typedef union matrix ## DIM{ \
    vec ## DIM row[DIM]; \
    float d[DIM][DIM]; \
} matrix ## DIM; \
 \
void printVec ## DIM(const vec ## DIM vec){ \
    printf("[ "); \
    for(size_t i = 0; i < DIM; i++){ \
        printf("%8f ", vec.d[i]); \
    }\
    printf("]"); \
} \
 \
void printMatrix ## DIM(const matrix ## DIM matr){ \
    for(size_t i = 0; i < DIM; i++){ \
        printVec ## DIM(matr.row[i]); \
        printf("\n"); \
    } \
} \
 \
vec ## DIM multMatVec ## DIM(const matrix ## DIM trans, const vec ## DIM vec){ \
    vec ## DIM new; \
    for(size_t i = 0; i < DIM; i++){ \
        new.d[i] = 0; \
        for(size_t j = 0; j < DIM; j++){ \
            new.d[i] += trans.d[i][j] * vec.d[j]; \
        } \
    } \
    return new; \
} \
 \
matrix ## DIM multMatMat ## DIM(const matrix ## DIM a, const matrix ## DIM b){ \
    matrix ## DIM new; \
    for(size_t i = 0; i < DIM; i++){ \
        for(size_t j = 0; j < DIM; j++){ \
            new.row[i].d[j] = 0; \
            for(size_t k = 0; k < DIM; k++){ \
                new.d[i][j] += a.d[i][k] * b.d[k][j]; \
            } \
        } \
    } \
    return new; \
} \
 \
vec ## DIM scaleVec ## DIM(const vec ## DIM vec, float scl){ \
    vec ## DIM new; \
    for(size_t i = 0; i < DIM; i++){ \
        new.d[i] = vec.d[i] * scl; \
    } \
    return new; \
} \
 \
matrix ## DIM scaleMatrix ## DIM(const matrix ## DIM trans, float scl){ \
    matrix ## DIM new; \
    for(size_t i = 0; i < DIM; i++){ \
        new.row[i] = scaleVec ## DIM(trans.row[i], scl); \
    } \
    return new; \
} \
 \
vec ## DIM addVec ## DIM(const vec ## DIM a, const vec ## DIM b){ \
    vec ## DIM new; \
    for(size_t i = 0; i < DIM; i++){ \
        new.d[i] = a.d[i] + b.d[i]; \
    } \
    return new; \
} \
 \
float lengthsq ## DIM(const vec ## DIM vec){ \
    float len = 0; \
    for(int i = 0; i < DIM; i++){ \
        len += vec.d[i] * vec.d[i]; \
    } \
    return len; \
} \
 \
float length ## DIM(const vec ## DIM vec){ \
    return sqrt(lengthsq ## DIM(vec)); \
} \
 \
vec ## DIM normalize ## DIM(const vec ## DIM vec){ \
    vec ## DIM norm; \
    float len = length ## DIM(vec); \
    for(int i = 0; i < DIM; i++){ \
        norm.d[i] = vec.d[i] / len; \
    } \
    return norm; \
} \

#define comma ,

MATR(2, x comma y)
MATR(3, x comma y comma z)
MATR(4, x comma y comma z comma w)

#undef comma

vec3 aimToVec(float yaw, float pitch){
    vec3 result;
    result.x = cos(pitch) * sin(yaw);
    result.y = cos(pitch) * cos(yaw);
    result.z = sin(pitch);
    return result;
}

float randomFloat(){
    return rand() / (float) RAND_MAX;
}

#define TAU 6.2831853

vec3 randomVec3(){
    float yaw = randomFloat() * TAU;
    float pitch = randomFloat() * TAU;
    return aimToVec(yaw, pitch);
}
