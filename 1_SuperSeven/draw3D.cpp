#include <cassert>
#include <float.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>

#include <GL/glew.h>

#include <GL/glut.h>

#include "initShaders.h"
#include "myDataStructures.h"
#include "readers.h"

using namespace std;

vector <Object*>	model3D;

GLuint axisShader;

GLuint axisVBO[3];
GLuint 	modelVBO[2];

Point 	bbMin		= { FLT_MAX, FLT_MAX, FLT_MAX},
        bbMax		= { FLT_MIN, FLT_MIN, FLT_MIN},
        bbCenter	= {0.0, 0.0, 0.0};

int winWdth = 1024, winHeight = 768;

bool drawRef = true, run = false;

float rotX = 0.0f, rotY = 0.0f;

/// ***********************************************************************
/// **
/// ***********************************************************************

void criaVBO() {

  ObjectVA axis_VA;

  axis_VA.vFace = (unsigned int *)malloc(4 * 1 * sizeof(unsigned int));
  if (!axis_VA.vFace)
    exit(-1);

  axis_VA.vPoint = (float *)malloc(4 * 3 * sizeof(float));
  if (!axis_VA.vPoint)
    exit(-1);

  axis_VA.vFace[0] = 0;
  axis_VA.vFace[1] = 1;
  axis_VA.vFace[2] = 2;
  axis_VA.vFace[3] = 3;

  axis_VA.vPoint[0] = -1.0;
  axis_VA.vPoint[1] = -1.0;
  axis_VA.vPoint[2] = 0.0;

  axis_VA.vPoint[3] = -1.0;
  axis_VA.vPoint[4] = 1.0;
  axis_VA.vPoint[5] = 0.0;

  axis_VA.vPoint[6] = 1.0;
  axis_VA.vPoint[7] = 1.0;
  axis_VA.vPoint[8] = 0.0;

  axis_VA.vPoint[9] = 1.0;
  axis_VA.vPoint[10] = -1.0;
  axis_VA.vPoint[11] = 0.0;

  axis_VA.vColor = (float *)malloc(4 * 4 * sizeof(float));
  if (!axis_VA.vColor)
    exit(-1);

  axis_VA.vColor[0] = 0.0;
  axis_VA.vColor[1] = 0.0;
  axis_VA.vColor[2] = 0.0;
  axis_VA.vColor[3] = 0.0;

  axis_VA.vColor[4] = 1.0;
  axis_VA.vColor[5] = 0.0;
  axis_VA.vColor[6] = 0.0;
  axis_VA.vColor[7] = 1.0;

  axis_VA.vColor[8] = 0.0;
  axis_VA.vColor[9] = 1.0;
  axis_VA.vColor[10] = 0.0;
  axis_VA.vColor[11] = 1.0;

  axis_VA.vColor[12] = 0.0;
  axis_VA.vColor[13] = 0.0;
  axis_VA.vColor[14] = 1.0;
  axis_VA.vColor[15] = 1.0;

  axis_VA.vTextCoord = NULL;
  axis_VA.vNormal = NULL;

  glGenBuffers(3, axisVBO);

  glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]);
  glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(float), axis_VA.vPoint,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]);
  glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), axis_VA.vColor,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * 1 * sizeof(unsigned int),
               axis_VA.vFace, GL_STATIC_DRAW);

  free(axis_VA.vPoint);
  free(axis_VA.vColor);
  free(axis_VA.vFace);
}

void buildModel() {

ObjectVA model3D_VA;
unsigned int f, iv, v;
Point massCenter = {0.0, 0.0, 0.0};

    int totFaces=0, totVertices=0;

    model3D_VA.vPoint 	= (float *) malloc(model3D[0]->vPoint.size()*3*sizeof(float));
    if (!model3D_VA.vPoint)
        exit(0);

    model3D_VA.vFace 	= (unsigned int *) malloc(model3D[0]->vFace.size()*3*sizeof(unsigned int));
    if (!model3D_VA.vFace)
        exit(0);

    for (v = 0 ; v < model3D[0]->vPoint.size() ; v++ ) {
        model3D_VA.vPoint[v*3+0] = model3D[0]->vPoint[v]->pto.x;
        model3D_VA.vPoint[v*3+1] = model3D[0]->vPoint[v]->pto.y;
        model3D_VA.vPoint[v*3+2] = model3D[0]->vPoint[v]->pto.z;
        if (model3D[0]->vPoint[v]->pto.x > bbMax.x)
            bbMax.x = model3D[0]->vPoint[v]->pto.x;
        if (model3D[0]->vPoint[v]->pto.x < bbMin.x)
            bbMin.x = model3D[0]->vPoint[v]->pto.x;
        if (model3D[0]->vPoint[v]->pto.y > bbMax.y)
            bbMax.y = model3D[0]->vPoint[v]->pto.y;
        if (model3D[0]->vPoint[v]->pto.y < bbMin.y)
            bbMin.y = model3D[0]->vPoint[v]->pto.y;
        if (model3D[0]->vPoint[v]->pto.z > bbMax.z)
            bbMax.z = model3D[0]->vPoint[v]->pto.z;
        if (model3D[0]->vPoint[v]->pto.z < bbMin.z)
            bbMin.z = model3D[0]->vPoint[v]->pto.z;
        massCenter.x += model3D[0]->vPoint[v]->pto.x;
        massCenter.y += model3D[0]->vPoint[v]->pto.y;
        massCenter.z += model3D[0]->vPoint[v]->pto.z;
        }

    massCenter.x /= model3D[0]->vPoint.size();
    massCenter.y /= model3D[0]->vPoint.size();
    massCenter.z /= model3D[0]->vPoint.size();

    for (f = 0 ; f < model3D[0]->vFace.size() ; f++ ) {
        for (iv = 0 ; iv < 3 ; iv++ ) {
            model3D_VA.vFace[f*3+iv] = model3D[0]->vFace[f]->indV[iv];
            }
        }

    glGenBuffers(	2, modelVBO);

    glBindBuffer(	GL_ARRAY_BUFFER, modelVBO[0]);

    glBufferData(	GL_ARRAY_BUFFER, model3D[0]->vPoint.size()*3*sizeof(float),
                    model3D_VA.vPoint, GL_STATIC_DRAW);

    glBindBuffer(	GL_ELEMENT_ARRAY_BUFFER, modelVBO[1]);

    glBufferData(	GL_ELEMENT_ARRAY_BUFFER, model3D[0]->vFace.size()*3*sizeof(unsigned int),
                    model3D_VA.vFace, GL_STATIC_DRAW);

    free(model3D_VA.vPoint);
    free(model3D_VA.vFace);

    cout << "Model Data: " << endl;
    cout << "		# of objects 	: " << model3D.size() << endl;
    cout << "		# of facets 	: " << model3D[0]->vFace.size() << endl;
    cout << "		# of vertices 	: " << model3D[0]->vPoint.size() << endl;
    cout << "Bounding Box" << endl;
    cout << "Min.   Point = ( " << bbMin.x << " , " << bbMin.y << " , " << bbMin.z << " ) " << endl;
    cout << "Max.   Point = ( " << bbMax.x << " , " << bbMax.y << " , " << bbMax.z << " ) " << endl;
    cout << "Center Point = ( " << bbCenter.x << " , " << bbCenter.y << " , " << bbCenter.z << " ) " << endl;
}

/// ***********************************************************************
/// **
/// ***********************************************************************

void drawAxis() {
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]);
    glVertexAttribPointer(glGetAttribLocation(axisShader,"aColor"), 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(axisShader,"aColor"));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);
    glPointSize(8.0);
    glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void reshape(int w, int h) {

  winWdth = w;
  winHeight = h;
  glViewport(0, 0, winWdth, winHeight);
  glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void idle() {
    glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void keyboard(unsigned char key, int x, int y) {

  switch (key) {
  case 27:
    exit(0);
    break;
  case 'A':
  case 'a':
    drawRef = !drawRef;
    break;
  case 'R':
  case 'r':
    run = !run;
    if (run)
      glutIdleFunc(idle);
    else
      glutIdleFunc(NULL);
    break;
  }
  glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void display(void) {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (drawRef) {
    glUseProgram(axisShader);
    drawAxis();
  }

  glutSwapBuffers();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initGL(void) {

  glClearColor(0.0, 0.0, 0.0, 0.0);

  if (glewInit()) {
    cout << "Unable to initialize GLEW ... exiting" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << endl;

  cout << "Opengl Version: " << glGetString(GL_VERSION) << endl;
  cout << "Opengl Vendor : " << glGetString(GL_VENDOR) << endl;
  cout << "Opengl Render : " << glGetString(GL_RENDERER) << endl;
  cout << "Opengl Shading Language Version : "
       << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

void initShaders(void) {

  // Load shaders and use the resulting shader program
  axisShader = InitShader("axisShader.vert", "axisShader.frag");
}

/* ************************************************************************* */
/* ************************************************************************* */
/* *****                                                               ***** */
/* ************************************************************************* */
/* ************************************************************************* */

int main(int argc, char *argv[]) {

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutCreateWindow("draw3D");
  glutReshapeWindow(winWdth, winHeight);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(NULL);

  readModelOBJ("malha.obj",model3D);

  initGL();

  criaVBO();

  initShaders();

  glutMainLoop();

  return (0);
}
