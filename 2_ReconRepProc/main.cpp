//Old C includes
#include <cstdio>
#include <cfloat>
#include <sys/stat.h>

//Cpp includes
#include <iostream>

//OpenGL Stuff
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GL/glut.h>
#include <SOIL.h>

//My includes
#include "rply.h"
#include "myDataStructures.h"
#include "initShaders.h"

void reshape(GLint x, GLint y);

// Particle data
GLuint vbo = 0, colorsVBO = 0;                 // OpenGL vertex buffer object

GLboolean g_bExitESC = false;

GLuint axisShader;
GLubyte *image;
GLuint wWidth = 1024, wHeight = 768;
std::vector<Point>* points = new std::vector<Point>();
std::vector<Color>* colors = new std::vector<Color>();
GLint64 nvertices;
glm::mat4 Projection, View, Model, MVP;

int loadImage(const GLchar* imagePath){
	struct stat buffer;
	if (stat(imagePath, &buffer) == 0){
		GLint width, height;
		image = SOIL_load_image(imagePath, &width, &height, 0, SOIL_LOAD_RGB);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		return strcmp(SOIL_last_result(), "Image loaded");
	}
	else{
		return 0;
	}
}

//void generateColor() {
//	chromaKeyingDest = (GLfloat *)malloc(4 * DS * sizeof(GLfloat));
//
//	for (GLuint var = 0; var < 4; ++var) {
//		chromaKeyingDest[var] = (GLfloat)rand() / RAND_MAX;
//	}
//}

void shaderPlumbing(){
	glProgramUniformMatrix4fv(axisShader, glGetUniformLocation(axisShader, "uMVP"), 1, false, glm::value_ptr(MVP));

	glPointSize(1);

	glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aPosition"));
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*nvertices, points->data(), GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(axisShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, points->data());
	glDrawArrays(GL_POINTS, 0, (GLsizei)nvertices);

	if (!colors->empty()){
		glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aColor"));
		glBindBuffer(GL_ARRAY_BUFFER, colorsVBO);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat)*nvertices, colors->data(), GL_STATIC_DRAW);
		glVertexAttribPointer(glGetAttribLocation(axisShader, "aColor"), 4, GL_FLOAT, GL_FALSE, 0, colors->data());
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

float rot = 0.0;
void display(void){
	rot += 0.01f;

	Projection = glm::perspective(glm::radians(45.0f),
		(float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT),
		0.1f, 20.0f);

	View = glm::lookAt(glm::vec3(-2.0, -2.0, -2.0),
		glm::vec3(0.0, 0.0, 0.0),
		glm::vec3(0.0, 1.0, 0.0));

	Model = glm::mat4(1.0f);
	Model = glm::rotate(Model, 0.0f, glm::vec3(1.0, 0.0, 0.0));
	Model = glm::rotate(Model, rot, glm::vec3(0.0, 1.0, 0.0));
	Model = glm::rotate(Model, 0.0f, glm::vec3(0.0, 0.0, 1.0));

	MVP = Projection * View * Model;

	glClearColor(0.3f, 0.3f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	shaderPlumbing();
	glDisable(GL_TEXTURE_2D);

	// Finish timing before swap buffers to avoid refresh sync	
	glutSwapBuffers();
	glutPostRedisplay();
}

void initShaders() {
	axisShader = InitShader("axisShader.vert", "axisShader.frag");
	glUseProgram(axisShader);
}

void keyboard(GLubyte key, GLint x, GLint y)
{
	char* fileName = new char[255];
	switch (key)
	{
	case 27:
		g_bExitESC = true;
#if defined (__APPLE__) || defined(MACOSX)
		exit(EXIT_SUCCESS);
#else
		glutDestroyWindow(glutGetWindow());
		return;
#endif
		break;
	case 'o':
	case 'O':
		cout << "Enter file name: ";
		fflush(stdin);
		cin >> fileName;
		loadImage(fileName);
		break;
	case 'c':
	case 'C':
		//generateColor();
		break;
	case 'r':
	case 'R':
		break;
	default:
		break;
	}
}

void reshape(GLint x, GLint y)
{
	wWidth = x;
	wHeight = y;
	glViewport(0, 0, x, y);
	glutPostRedisplay();
}

GLint initGL(GLint *argc, GLchar **argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(wWidth, wHeight);
	glutCreateWindow("Mesh Reconstruction");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	/*glutMouseFunc(click);
	glutMotionFunc(motion);*/
	glutReshapeFunc(reshape);
	glewInit();
	return 1;
}

Point tempPoint = { 0.0, 0.0, 0.0 };
Color tempColor = { 1.0, 1.0, 1.0, 1.0 };
static int vertex_cb(p_ply_argument argument) {
	long idata;
	ply_get_argument_user_data(argument, NULL, &idata);
	switch (idata){
	case 0:
		tempPoint.x = (GLfloat)ply_get_argument_value(argument);
		break;
	case 1:
		tempPoint.y = (GLfloat)ply_get_argument_value(argument);
		break;
	case 2:
		tempPoint.z = (GLfloat)ply_get_argument_value(argument);
		points->push_back(tempPoint);
		tempPoint = { 0.0, 0.0, 0.0 };
		break;
	case 3:
		tempColor.r = (GLfloat)ply_get_argument_value(argument) / 255.0f;
		break;
	case 4:
		tempColor.g = (GLfloat)ply_get_argument_value(argument) / 255.0f;
		break;
	case 5:
		tempColor.b = (GLfloat)ply_get_argument_value(argument) / 255.0f;
		break;
	case 6:
		tempColor.a = (GLfloat)ply_get_argument_value(argument) / 255.0f;
		colors->push_back(tempColor);
		tempColor = { 1.0, 1.0, 1.0, 1.0 };
		break;
	default:
		break;
	}
	return 1;
}

GLint main(GLint argc, GLchar **argv)
{
	srand(_threadid);

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif	

	if (false == initGL(&argc, argv))
	{
		exit(EXIT_FAILURE);
	}

	initShaders();

	p_ply ply = ply_open("bota_pretty_biggur.ply", NULL, 0, NULL);

	if (!ply) return 1;
	if (!ply_read_header(ply)) return 1;
	nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
	ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 1);
	ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 2);
	ply_set_read_cb(ply, "vertex", "red", vertex_cb, NULL, 3);
	ply_set_read_cb(ply, "vertex", "green", vertex_cb, NULL, 4);
	ply_set_read_cb(ply, "vertex", "blue", vertex_cb, NULL, 5);
	ply_set_read_cb(ply, "vertex", "alpha", vertex_cb, NULL, 6);
	printf("Vertices: %ld\n", nvertices);
	if (!ply_read(ply)) return 1;
	ply_close(ply);

	/*for (std::vector<Color>::iterator it = colors->begin(); it != colors->end(); ++it){
		printf("{%g, %g, %g}\n", it->r, it->g, it->b);
		}*/

	glutMainLoop();

	exit(EXIT_SUCCESS);
	return 0;
}