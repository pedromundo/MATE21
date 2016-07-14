#ifndef MY_DATA_STRUCTURES
#define MY_DATA_STRUCTURES 1

#include <vector>

using namespace std;

typedef struct {
  float x;
  float y;
  float z;
} Point;

typedef struct {
  float r;
  float g;
  float b;
  float a;
} Color;

typedef struct {
  Point pto;
  Point normal;
  int ID;
  int valencia;
  Color color;
} Vertice;

typedef struct {
  int ID;
  int indV[3];
  Point normal;
} Face;

typedef struct {
  int ID;
  vector<Vertice *> vPoint;
  vector<Face *> vFace;
} Object;

typedef struct {
  float *vPoint;
  float *vNormal;
  float *vColor;
  float *vTextCoord;
  unsigned int *vFace;
} ObjectVA;

struct MyVertex
  {
    float x, y, z;        //Vertex
    float nx, ny, nz;     //Normal
    float s0, t0;         //Texcoord0
  };

#endif
