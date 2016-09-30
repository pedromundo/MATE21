//OpenGL Stuff
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 

//My includes
#include <SOIL.h>
#include <map>
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
int wTex, hTex, cTex, wNor, hNor, cNor;
//Handlers for the VBO and FBOs
GLuint VertexArrayIDs[1], vertexbuffers[2], textureArrays[2];
//MVP Matrices
glm::mat4 Projection, View, Model;

//Using std::vector because no one wants to work with arrays in 2016
std::vector<Vertex> *vertices = new std::vector<Vertex>();
std::vector<Face> *faces = new std::vector<Face>();

GLubyte* texture;
GLubyte* normalmap;

void computeTangentBasis(){
	for (int i = 0; i < faces->size(); ++i){
		// Shortcuts for vertices
		Vertex a = vertices->at(faces->at(i).f1);
		Vertex b = vertices->at(faces->at(i).f2);
		Vertex c = vertices->at(faces->at(i).f3);

		glm::vec3 & v0 = glm::vec3(a.x, a.y, a.z);
		glm::vec3 & v1 = glm::vec3(b.x, b.y, b.z);
		glm::vec3 & v2 = glm::vec3(c.x, c.y, c.z);

		// Shortcuts for UVs
		glm::vec2 & uv0 = glm::vec2(a.uv.u, a.uv.v);
		glm::vec2 & uv1 = glm::vec2(b.uv.u, b.uv.v);
		glm::vec2 & uv2 = glm::vec2(c.uv.u, c.uv.v);

		// Edges of the triangle : postion delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		// UV delta
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;
		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y)*r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x)*r;
		// Set the same tangent for all three vertices of the triangle.	
		vertices->at(faces->at(i).f1).tan = tangent;
		vertices->at(faces->at(i).f2).tan = tangent;
		vertices->at(faces->at(i).f3).tan = tangent;
		// Same thing for binormals
		vertices->at(faces->at(i).f1).bin = bitangent;
		vertices->at(faces->at(i).f2).bin = bitangent;
		vertices->at(faces->at(i).f3).bin = bitangent;
	}
}

GLvoid shaderPlumbing(){
	printOpenGLError();
	//Point size 1 looks like shit
	glPointSize(1);

	//MVP matrix	
	GLuint MVPId = glGetUniformLocation(basicShader, "MVP");
	glUniformMatrix4fv(MVPId, 1, GL_FALSE, glm::value_ptr(Projection * View * Model));
	//MV matrix 
	GLuint MVId = glGetUniformLocation(basicShader, "MV");
	glUniformMatrix3fv(MVId, 1, GL_FALSE, glm::value_ptr(glm::mat3(View * Model)));
	printOpenGLError();
	//V Matrix
	GLuint MId = glGetUniformLocation(basicShader, "M");
	glUniformMatrix3fv(MId, 1, GL_FALSE, glm::value_ptr(glm::mat3(Model)));
	printOpenGLError();
	//Light position
	glm::vec3 lightPos = glm::vec3(0, 0.2, 1);
	GLuint lightID = glGetUniformLocation(basicShader, "lightPos");
	glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);

	std::size_t vertexSize = (3 * sizeof(GLfloat) + 3 * sizeof(GLfloat) + 2 * sizeof(GLfloat) + 2 * sizeof(glm::vec3));

	glBindVertexArray(VertexArrayIDs[0]);
	//position data
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, vertexSize*nvertices, vertices->data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "Position_modelspace"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "Position_modelspace"), 3, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)0);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "vertNormal_modelspace"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "vertNormal_modelspace"), 3, GL_FLOAT, GL_FALSE, vertexSize, (const GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "vertTexCoord"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "vertTexCoord"), 2, GL_FLOAT, GL_FALSE, vertexSize, (const GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "vertTangent_modelspace"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "vertTangent_modelspace"), 3, GL_FLOAT, GL_FALSE, vertexSize, (const GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "vertBinormal_modelspace"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "vertBinormal_modelspace"), 3, GL_FLOAT, GL_FALSE, vertexSize, (const GLvoid*)(11 * sizeof(GLfloat)));
	printOpenGLError();

	//Element vertex IDs data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(GLuint)*ntriangles, faces->data(), GL_STATIC_DRAW);
	printOpenGLError();
}

GLvoid display(GLvoid){
	glClearColor(0.3f, 0.3f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	shaderPlumbing();

	glDrawElements(GL_TRIANGLES, 3 * ntriangles, GL_UNSIGNED_INT, (void*)0);

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

GLvoid process(GLvoid){
	View = glm::rotate(View, 0.01f, glm::vec3(0.0, 1.0, 0.0));
}

GLint initGL(GLint *argc, GLchar **argv)
{
	glutInit(argc, argv);
	glutIdleFunc(process);
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

Vertex tempPoint = { 0.0f, 0.0f, 0.0f };

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
		break;
	case 3:
		tempPoint.normal.nx = ply_get_argument_value(argument);
		break;
	case 4:
		tempPoint.normal.ny = ply_get_argument_value(argument);
		break;
	case 5:
		tempPoint.normal.nz = ply_get_argument_value(argument);
		vertices->push_back(tempPoint);
		break;
	default:
		break;
	}
	return 1;
}

Face tempFace;
int face_cb(p_ply_argument argument) {
	long length, value_index, currItem;
	ply_get_argument_property(argument, NULL, &length, &value_index);
	ply_get_argument_user_data(argument, NULL, &currItem);
	if (length == 3){
		switch (value_index) {
		case 0:
			tempFace.f1 = ply_get_argument_value(argument);
			break;
		case 1:
			tempFace.f2 = ply_get_argument_value(argument);
			break;
		case 2:
			tempFace.f3 = ply_get_argument_value(argument);
			faces->push_back(tempFace);
			break;
		default:
			break;
		}
	}
	else if (length == 6){
		switch (value_index) {
		case 0:
			vertices->at(tempFace.f1).uv.u = ply_get_argument_value(argument);
			break;
		case 1:
			vertices->at(tempFace.f1).uv.v = ply_get_argument_value(argument);
			break;
		case 2:
			vertices->at(tempFace.f2).uv.u = ply_get_argument_value(argument);
			break;
		case 3:
			vertices->at(tempFace.f2).uv.v = ply_get_argument_value(argument);
			break;
		case 4:
			vertices->at(tempFace.f3).uv.u = ply_get_argument_value(argument);
			break;
		case 5:
			vertices->at(tempFace.f3).uv.v = ply_get_argument_value(argument);
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
		glm::vec3(1.2, 1.2, 1.2),
		glm::vec3(0, 0, 0),
		glm::vec3(0, 1, 0)
		);
	Projection = glm::perspective(glm::radians(60.0f), (GLfloat)wWidth / (GLfloat)wHeight, 0.1f, 100.0f);

	//Read model from .ply file	
	p_ply ply = ply_open("bag.ply", NULL, 0, NULL);
	if (!ply) return EXIT_FAILURE;
	if (!ply_read_header(ply)) return EXIT_FAILURE;
	nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, NULL, 0);
	ply_set_read_cb(ply, "vertex", "y", vertex_cb, NULL, 1);
	ply_set_read_cb(ply, "vertex", "z", vertex_cb, NULL, 2);
	ply_set_read_cb(ply, "vertex", "nx", vertex_cb, NULL, 3);
	ply_set_read_cb(ply, "vertex", "ny", vertex_cb, NULL, 4);
	ply_set_read_cb(ply, "vertex", "nz", vertex_cb, NULL, 5);
	ntriangles = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, NULL, 0);
	ply_set_read_cb(ply, "face", "texcoord", face_cb, NULL, 1);
	if (!ply_read(ply)) return EXIT_FAILURE;
	ply_close(ply);

	//Read textures from files
	texture = SOIL_load_image("bag_tex.png", &wTex, &hTex, &cTex, SOIL_LOAD_RGB);
	normalmap = SOIL_load_image("bag_normal.png", &wNor, &hNor, &cNor, SOIL_LOAD_RGB);

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif	

	if (false == initGL(&argc, argv))
	{
		return EXIT_FAILURE;
	}

	glGenVertexArrays(1, VertexArrayIDs);
	glGenBuffers(2, vertexbuffers);
	glGenTextures(2, textureArrays);

	computeTangentBasis();
	initShaders();

	//Texture data		
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureArrays[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wTex, hTex, 0, GL_RGB, GL_UNSIGNED_BYTE, texture);
	glUniform1i(glGetUniformLocation(basicShader, "tex"), 0);

	//Normal data
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureArrays[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wTex, hTex, 0, GL_RGB, GL_UNSIGNED_BYTE, normalmap);
	glUniform1i(glGetUniformLocation(basicShader, "nor"), 1);
	printOpenGLError();

	glutMainLoop();

	return EXIT_SUCCESS;
}