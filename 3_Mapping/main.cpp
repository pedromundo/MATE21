//OpenGL Stuff
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

//My includes
#include <SOIL.h>
#include "myDataStructures.h"
#include "initShaders.h"
#include "rply.h"
#include "rplyfile.h"

GLvoid reshape(GLint x, GLint y);

GLboolean g_bExitESC = false;

//Shader Program Handle
GLuint basicShader;
//Window Dimensions
GLuint wWidth = 640, wHeight = 480;
//# of vertices and tris
GLulong nvertices, ntriangles;
//Texture properties
int wTex, hTex, cTex;
//Handlers for the VBO and FBOs
GLuint VertexArrayIDs[1], vertexbuffers[4], textureArrays[1];
//MVP Matrices
glm::mat4 Projection, View, Model;

//Using std::vector because ffs no one wants to work with arrays in 2016
std::vector<Point>* points = new std::vector<Point>();
std::vector<Color>* colors = new std::vector<Color>();
std::vector<Normal>* normals = new std::vector<Normal>();
std::vector<Face>* faces = new std::vector<Face>();
TexCoord* texcoords;
GLubyte* texture;


GLvoid shaderPlumbing(){	
	printOpenGLError();
	//Point size 1 looks like shit
	glPointSize(2);

	//MVP matrix
	glm::mat4 MVP = Projection * Model * View;
	GLuint MVPId = glGetUniformLocation(basicShader, "MVP");
	glUniformMatrix4fv(MVPId, 1, GL_FALSE, glm::value_ptr(MVP));
	printOpenGLError();

	//position data
	glBindVertexArray(VertexArrayIDs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*nvertices, points->data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "aPosition"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	printOpenGLError();

	//color data
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[1]);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat)*nvertices, colors->data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "aColor"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "aColor"), 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);	
	printOpenGLError();

	//vertex uv coords
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2 * sizeof(GLfloat)*nvertices, texcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "vertTexCoord"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "vertTexCoord"), 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	printOpenGLError();

	//Element vertex IDs data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(GLuint)*ntriangles, faces->data(), GL_STATIC_DRAW);
	printOpenGLError();

	//Texture data		
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureArrays[0]);	
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wTex, hTex, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);
	glUniform1i(glGetUniformLocation(basicShader, "tex"), 0);
}

GLvoid display(GLvoid){
	glClearColor(0.3f, 0.3f, 0.0f, 1.0f);	
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);	
	shaderPlumbing();
	printOpenGLError();

	glDrawElements(GL_TRIANGLES, ntriangles * 3, GL_UNSIGNED_INT, faces->data());

	glutSwapBuffers();
	glutPostRedisplay();
}

GLvoid initShaders() {
	basicShader = InitShader("basicShader.vert", "basicShader.frag");
	glUseProgram(basicShader);
}

GLvoid keyboard(GLubyte key, GLint x, GLint y)
{
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
	default:
		break;
	}
}

GLvoid reshape(GLint x, GLint y)
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
	glutCreateWindow("OpenGL Viewer Scaffold");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glewInit();
	return 1;
}

inline GLfloat interpolate(const GLfloat a, const GLfloat b, const GLfloat coefficient)
{
	return a + coefficient * (b - a);
}

Point tempPoint = { 0.0f, 0.0f, 0.0f };
Normal tempNormal = { 0.0f, 0.0f, 0.0f };
Color tempColor = { 0.0f, 0.0f, 0.0f, 0.0f };

int vertex_cb(p_ply_argument argument) {
	long currItem;
	ply_get_argument_user_data(argument, NULL, &currItem);
	switch (currItem){
	case 0:
		tempPoint.x = ply_get_argument_value(argument);
		break;
	case 1:
		tempPoint.y = ply_get_argument_value(argument);
		break;
	case 2:
		tempPoint.z = ply_get_argument_value(argument);
		points->push_back(tempPoint);
		break;
	case 3:
		tempNormal.nx = ply_get_argument_value(argument);
		break;
	case 4:
		tempNormal.ny = ply_get_argument_value(argument);
		break;
	case 5:
		tempNormal.nz = ply_get_argument_value(argument);
		normals->push_back(tempNormal);
		break;
	case 6:
		tempColor.r = ply_get_argument_value(argument) / 255.0;
		break;
	case 7:
		tempColor.g = ply_get_argument_value(argument) / 255.0;
		break;
	case 8:
		tempColor.b = ply_get_argument_value(argument) / 255.0;
		break;
	case 9:
		tempColor.a = ply_get_argument_value(argument) / 255.0;
		colors->push_back(tempColor);
		break;
	default:
		break;
	}
	return 1;
}

Face tempFace;
TexCoord tempTexCoord;

int face_cb(p_ply_argument argument) {
	long length, value_index, currItem;
	ply_get_argument_property(argument, NULL, &length, &value_index);
	ply_get_argument_user_data(argument, NULL, &currItem);
	if (length == 3){
		switch (value_index) {
		case 0:
			tempFace.idVertices[0] = ply_get_argument_value(argument);
			break;
		case 1:
			tempFace.idVertices[1] = ply_get_argument_value(argument);
			break;
		case 2:
			tempFace.idVertices[2] = ply_get_argument_value(argument);
			faces->push_back(tempFace);
			break;
		default:
			break;
		}
	}
	else if (length == 6){
		switch (value_index) {
		case 0:
			tempTexCoord.u = ply_get_argument_value(argument);
			break;
		case 1:
			tempTexCoord.v = ply_get_argument_value(argument);
			texcoords[tempFace.idVertices[0]] = tempTexCoord;
			break;
		case 2:
			tempTexCoord.u = ply_get_argument_value(argument);
			break;
		case 3:
			tempTexCoord.v = ply_get_argument_value(argument);
			texcoords[tempFace.idVertices[1]] = tempTexCoord;
			break;
		case 4:
			tempTexCoord.u = ply_get_argument_value(argument);
			break;
		case 5:
			tempTexCoord.v = ply_get_argument_value(argument);
			texcoords[tempFace.idVertices[2]] = tempTexCoord;
			break;
		default:
			break;
		}
	}
	return 1;
}

GLint main(GLint argc, GLchar **argv)
{
	//Setting up our MVP Matrices
	Model = glm::mat4(1.0f);
	View = glm::lookAt(
		glm::vec3(2, 2, 2), // Camera is at (3,3,3), in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
		);
	Projection = glm::perspective(glm::radians(45.0f), (GLfloat)wWidth / (GLfloat)wHeight, 0.1f, 100.0f);

	texture = SOIL_load_image("bag_tex.png", &wTex, &hTex, &cTex, SOIL_LOAD_RGB);

	//Read model from .ply file	
	p_ply ply = ply_open("bag.ply", NULL, 0, NULL);
	if (!ply) return EXIT_FAILURE;
	if (!ply_read_header(ply)) return EXIT_FAILURE;
	nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
	texcoords = (TexCoord*)malloc(sizeof(TexCoord)*nvertices);
	ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 1);
	ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 2);
	ply_set_read_cb(ply, "vertex", "nx", vertex_cb, NULL, 3);
	ply_set_read_cb(ply, "vertex", "ny", vertex_cb, NULL, 4);
	ply_set_read_cb(ply, "vertex", "nz", vertex_cb, NULL, 5);
	ply_set_read_cb(ply, "vertex", "red", vertex_cb, NULL, 6);
	ply_set_read_cb(ply, "vertex", "green", vertex_cb, NULL, 7);
	ply_set_read_cb(ply, "vertex", "blue", vertex_cb, NULL, 8);
	ply_set_read_cb(ply, "vertex", "alpha", vertex_cb, NULL, 9);
	ntriangles = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, NULL, 0);
	ply_set_read_cb(ply, "face", "texcoord", face_cb, NULL, 1);
	if (!ply_read(ply)) return EXIT_FAILURE;
	ply_close(ply);

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif	

	if (false == initGL(&argc, argv))
	{
		return EXIT_FAILURE;
	}

	glGenVertexArrays(1, VertexArrayIDs);
	glGenBuffers(4, vertexbuffers);
	glGenTextures(1, textureArrays);

	initShaders();

	glutMainLoop();

	return EXIT_SUCCESS;
}