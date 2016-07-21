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

#include <cuda.h>
#include <cudaGL.h>
#include <cuda_gl_interop.h>

#include "myDataStructures.h"
#include "readers.h"
#include "hello-world.h"

using namespace std;

vector<Object *> model3D;

GLuint axisShader;

GLuint axisVBO[3];
GLuint modelVBO[3];

GLfloat *colors;

Point bbMin = {FLT_MAX, FLT_MAX, FLT_MAX}, bbMax = {FLT_MIN, FLT_MIN, FLT_MIN},
      bbCenter = {0.0, 0.0, 0.0};

GLint winWdth = 1024, winHeight = 768;

GLboolean drawRef = true, run = true, spin = false, randomcolors = false;

GLfloat rotX = 0.0f, rotY = 0.0f;

GLuint InitShader(const GLchar *vShaderFile, const GLchar *fShaderFile);

/// ***********************************************************************
/// **
/// ***********************************************************************

GLvoid criaVBO() {

  ObjectVA axis_VA;

  axis_VA.vFace = (GLuint *)malloc(4 * 1 * sizeof(GLuint));
  if (!axis_VA.vFace)
    exit(-1);

  axis_VA.vPoint = (GLfloat *)malloc(4 * 3 * sizeof(GLfloat));
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

  axis_VA.vColor = (GLfloat *)malloc(4 * 4 * sizeof(GLfloat));
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
  glBufferData(GL_ARRAY_BUFFER, 4 * 3 * sizeof(GLfloat), axis_VA.vPoint,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]);
  glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), axis_VA.vColor,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * 1 * sizeof(GLuint), axis_VA.vFace,
               GL_STATIC_DRAW);

  free(axis_VA.vPoint);
  free(axis_VA.vColor);
  free(axis_VA.vFace);
}

GLvoid generateColors() {
  colors = (GLfloat *)malloc(4 * model3D[0]->vPoint.size() * sizeof(GLfloat));

  for (GLuint var = 0; var < 4 * model3D[0]->vPoint.size(); ++var) {
    colors[var] = (GLfloat)random() / RAND_MAX;
  }
}

GLvoid buildModel() {

  ObjectVA model3D_VA;
  GLuint f, iv, v;
  Point massCenter = {0.0, 0.0, 0.0};

  model3D_VA.vPoint =
      (GLfloat *)malloc(model3D[0]->vPoint.size() * 3 * sizeof(GLfloat));
  if (!model3D_VA.vPoint)
    exit(0);

  model3D_VA.vFace =
      (GLuint *)malloc(model3D[0]->vFace.size() * 3 * sizeof(GLuint));
  if (!model3D_VA.vFace)
    exit(0);

  for (v = 0; v < model3D[0]->vPoint.size(); v++) {
    model3D_VA.vPoint[v * 3 + 0] = model3D[0]->vPoint[v]->pto.x;
    model3D_VA.vPoint[v * 3 + 1] = model3D[0]->vPoint[v]->pto.y;
    model3D_VA.vPoint[v * 3 + 2] = model3D[0]->vPoint[v]->pto.z;
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

  for (f = 0; f < model3D[0]->vFace.size(); f++) {
    for (iv = 0; iv < 3; iv++) {
      model3D_VA.vFace[f * 3 + iv] = model3D[0]->vFace[f]->indV[iv];
    }
  }

  generateColors();

  if (!colors)
    exit(-1);

  glGenBuffers(3, modelVBO);

  glBindBuffer(GL_ARRAY_BUFFER, modelVBO[0]);

  glBufferData(GL_ARRAY_BUFFER, model3D[0]->vPoint.size() * 3 * sizeof(GLfloat),
               model3D_VA.vPoint, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, modelVBO[1]);

  glBufferData(GL_ARRAY_BUFFER, model3D[0]->vPoint.size() * 4 * sizeof(GLfloat),
               colors, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelVBO[2]);

  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               model3D[0]->vFace.size() * 3 * sizeof(GLuint), model3D_VA.vFace,
               GL_STATIC_DRAW);

  free(model3D_VA.vPoint);
  free(colors);
  free(model3D_VA.vFace);

  cout << "Model Data: " << endl;
  cout << "		# of objects 	: " << model3D.size() << endl;
  cout << "		# of facets 	: " << model3D[0]->vFace.size() << endl;
  cout << "		# of vertices 	: " << model3D[0]->vPoint.size()
       << endl;
  cout << "Bounding Box" << endl;
  cout << "Min.   Point = ( " << bbMin.x << " , " << bbMin.y << " , " << bbMin.z
       << " ) " << endl;
  cout << "Max.   Point = ( " << bbMax.x << " , " << bbMax.y << " , " << bbMax.z
       << " ) " << endl;
  cout << "Center Point = ( " << bbCenter.x << " , " << bbCenter.y << " , "
       << bbCenter.z << " ) " << endl;
}

/// ***********************************************************************
/// **
/// ***********************************************************************

GLvoid drawAxis() {
  glBindBuffer(GL_ARRAY_BUFFER, axisVBO[0]);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, axisVBO[1]);
  glVertexAttribPointer(glGetAttribLocation(axisShader, "aColor"), 4, GL_FLOAT,
                        GL_FALSE, 0, 0);
  glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aColor"));

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, axisVBO[2]);
  glPointSize(8.0);
  glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, 0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

glm::mat4 Model, View, Projection;
glm::mat4 MVP = Projection * View * Model;

GLvoid drawModel() {
  glProgramUniformMatrix4fv(axisShader,
                            glGetUniformLocation(axisShader, "uMVP"), 1, false,
                            glm::value_ptr(MVP));

  glBindBuffer(GL_ARRAY_BUFFER, modelVBO[0]);
  glVertexAttribPointer(glGetAttribLocation(axisShader, "aPosition"), 3,
                        GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aPosition"));

  glBindBuffer(GL_ARRAY_BUFFER, modelVBO[1]);
  glVertexAttribPointer(glGetAttribLocation(axisShader, "aColor"), 4, GL_FLOAT,
                        GL_FALSE, 0, 0);
  glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aColor"));

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelVBO[2]);
  glPointSize(8.0);
  glDrawElements(GL_TRIANGLES, 144 * 3, GL_UNSIGNED_INT, 0);
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

GLvoid reshape(GLint w, GLint h) {
  winWdth = w;
  winHeight = h;
  glViewport(0, 0, winWdth, winHeight);
  glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

GLuint timer = 0;
GLint angle = 0;
GLboolean clockwise = true;
GLvoid idle() {
  ++timer;
  if (timer >= 1) {

    if (randomcolors) {
      generateColors();
      glBindBuffer(GL_ARRAY_BUFFER, modelVBO[1]);
      glBufferData(GL_ARRAY_BUFFER,
                   model3D[0]->vPoint.size() * 4 * sizeof(GLfloat), colors,
                   GL_STATIC_DRAW);
    }

    if (spin) {
      if (clockwise) {
        ++angle;
        if (angle > 30) {
          clockwise = false;
        }
      } else {
        --angle;
        if (angle < -30) {
          clockwise = true;
        }
      }
      glProgramUniform1i(axisShader, glGetUniformLocation(axisShader, "uAngle"),
                         angle);
    }

    timer = 0;
  }

  glutPostRedisplay();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

GLvoid keyboard(GLubyte key, GLint x, GLint y) {

  switch (key) {
  case 27:
    exit(0);
    break;
  case 'A':
  case 'a':
    drawRef = !drawRef;
    break;
  case 'C':
  case 'c':
    randomcolors = !randomcolors;
    break;
  case 'S':
  case 's':
    spin = !spin;
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

GLvoid display(GLvoid) {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (drawRef) {
    glUseProgram(axisShader);
    // drawAxis();
    drawModel();
  }

  glutSwapBuffers();
}

/* ************************************************************************* */
/*                                                                           */
/* ************************************************************************* */

GLvoid initGL(GLvoid) {

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

GLvoid initShaders(GLvoid) {

  // Load shaders and use the resulting shader program
  axisShader = InitShader("axisShader.vert", "axisShader.frag");
}

/* ************************************************************************* */
/* ************************************************************************* */
/* *****                                                               ***** */
/* ************************************************************************* */
/* ************************************************************************* */

GLint main(GLint argc, GLchar *argv[]) {

  srand(time(NULL));

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - winWdth) / 2,
                         (glutGet(GLUT_SCREEN_HEIGHT) - winHeight) / 2);
  glutCreateWindow("Ultra Seven Effect");
  glutReshapeWindow(winWdth, winHeight);
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);

  if (run)
    glutIdleFunc(idle);
  else
    glutIdleFunc(NULL);

  readModelOBJ("malha.obj", model3D);  

  initGL();

  // criaVBO();

  buildModel();  

  doCuda();

  initShaders();

  glutMainLoop();

  return (0);
}
