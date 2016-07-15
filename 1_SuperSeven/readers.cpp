#include <assert.h>
#include <iostream>

#include <GL/glew.h>

#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "myDataStructures.h"

#include "glm.h"

using namespace std;

/* ************************************************************************* */
/* ************************************************************************* */

void readModelOBJ(char *filename, vector<Object *> &model3D) {

  GLMmodel *pmodel = NULL;

#define T(x) (pmodel->triangles[(x)])

  cout << "Start loading OBJ model " << filename << " ...." << endl;

  pmodel = glmReadOBJ(filename);
  if (!pmodel) {
    cout << "Error reading OBJ file !!" << endl;
    exit(0);
  }
  //	glmUnitize(pmodel);
  glmFacetNormals(pmodel);
  glmVertexNormals(pmodel, 90.0);

  GLMgroup *group;
  GLMtriangle *triangle;

  assert(pmodel);
  assert(pmodel->vertices);

  Object *obj = new Object;
  obj->ID = 0;

  int minIndFace = pmodel->numvertices + 10000;

  for (unsigned int i = 1; i <= pmodel->numvertices; i++) {
    Vertice *v = new Vertice;

    v->pto.x = pmodel->vertices[3 * i + 0];
    v->pto.y = pmodel->vertices[3 * i + 1];
    v->pto.z = pmodel->vertices[3 * i + 2];
    v->normal.x = pmodel->normals[3 * i + 0];
    v->normal.y = pmodel->normals[3 * i + 1];
    v->normal.z = pmodel->normals[3 * i + 2];
    v->ID = i;
    v->valencia = 0;

    obj->vPoint.push_back(v);
  }
  /*
      int iFace = 0;

          for (unsigned int i = 1; i <= pmodel->numtriangles; i++) {
                  Face* f = new Face;

                  f->ID		= iFace++;
                  f->normal.x =
     pmodel->normals[pmodel->triangles[i].nindices[0]];
                  f->normal.y =
     pmodel->normals[pmodel->triangles[i].nindices[1]];
                  f->normal.z =
     pmodel->normals[pmodel->triangles[i].nindices[2]];

                  for(unsigned int k = 0; k < 3; k++)  {
                          f->indV[k] = pmodel->triangles[i].vindices[k];
                          if (minIndFace > f->indV[k])
                                  minIndFace = f->indV[k];
                          }

                  obj->vFace.push_back(f);
          }
  */
  group = pmodel->groups;

  int iFace = 0;

  while (group) {

    for (unsigned int i = 0; i < group->numtriangles; i++) {

      triangle = &T(group->triangles[i]);
      Face *f = new Face;
      f->ID = iFace++;
      f->normal.x = pmodel->normals[triangle->nindices[0]];
      f->normal.y = pmodel->normals[triangle->nindices[1]];
      f->normal.z = pmodel->normals[triangle->nindices[2]];

      for (unsigned int k = 0; k < 3; k++) {
        f->indV[k] = (triangle->vindices[k]);

        if (minIndFace > f->indV[k])
          minIndFace = f->indV[k];
      }
      obj->vFace.push_back(f);
    }

    group = group->next;
  }

  if (minIndFace > 0)
    for (unsigned int f = 0; f < obj->vFace.size(); f++)
      for (unsigned int iv = 0; iv < 3; iv++)
        obj->vFace[f]->indV[iv] -= minIndFace;

  model3D.push_back(obj);

  cout << "Finish loading OBJ model " << filename << "." << endl;
}
