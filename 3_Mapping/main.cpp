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

GLboolean g_bExitESC = false, g_bRotateModel = false;

//Window Dimensions
GLuint wWidth = 1024, wHeight = 768;
//Shader uniforms
GLuint tessLevel = 20, lightDiffusePower = 100, lightSpecularPower = 10, lightDistance = 10;
GLfloat displacementStrength = 0.1f;
//# of vertices and tris
GLulong nvertices, ntriangles;
//Texture properties
GLint wTex, hTex, cTex, wNor, hNor, cNor, wHei, hHei, cHei;
//Handlers for the VBOs, FBOs, texArrays shader programs
GLuint VertexArrayIDs[1], vertexbuffers[2], textureArrays[3], basicShader;
GLfloat fov = 60.0f;
std::size_t vertexSize = (3 * sizeof(GLfloat) + 3 * sizeof(GLfloat) + 2 * sizeof(GLfloat) + 2 * sizeof(glm::vec3));
//MVP Matrices
glm::mat4 Projection, View, Model;
glm::vec3 eyePos = glm::vec3(1, 1.3, 1.0);

//Using std::vector because no one wants to work with arrays in 2016
std::vector<Vertex> *vertices = new std::vector<Vertex>();
std::vector<Face> *faces = new std::vector<Face>();

GLubyte* texture;
GLubyte* normalmap;
GLubyte* heightmap;

void computeTangents(){
	for (GLint i = 0; i < faces->size(); ++i){
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
		GLfloat r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
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
	//Eye Position
	GLuint eyePosId = glGetUniformLocation(basicShader, "eyePos");
	glUniform3f(eyePosId, eyePos.x, eyePos.y, eyePos.z);
	printOpenGLError();
	//Tesselation Level
	GLuint tessLevelId = glGetUniformLocation(basicShader, "tessLevel");
	glUniform1ui(tessLevelId, tessLevel);
	printOpenGLError();
	//Diffuse lighting intensity
	GLuint lightDiffusePowerId = glGetUniformLocation(basicShader, "lightDiffusePower");
	glUniform1ui(lightDiffusePowerId, lightDiffusePower);
	printOpenGLError();
	//Specular lighting intensity
	GLuint lightSpecularPowerId = glGetUniformLocation(basicShader, "lightSpecularPower");
	glUniform1ui(lightSpecularPowerId, lightSpecularPower);
	printOpenGLError();
	//Light distance for intensity calculations
	GLuint lightDistanceId = glGetUniformLocation(basicShader, "lightDistance");
	glUniform1ui(lightDistanceId, lightDistance);
	printOpenGLError();
	//Displacement strength
	GLuint displacementStrengthId = glGetUniformLocation(basicShader, "displacementStrength");
	glUniform1f(displacementStrengthId, displacementStrength);
	printOpenGLError();
	//Light position	
	GLuint lightID = glGetUniformLocation(basicShader, "lightPos");
	glUniform3f(lightID, eyePos.x, eyePos.y, eyePos.z);
	printOpenGLError();

	glBindVertexArray(VertexArrayIDs[0]);
	//Vertex attributes
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "appPosition_modelspace"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "appPosition_modelspace"), 3, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)0);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "appNormal_modelspace"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "appNormal_modelspace"), 3, GL_FLOAT, GL_FALSE, vertexSize, (const GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "appTexCoord"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "appTexCoord"), 2, GL_FLOAT, GL_FALSE, vertexSize, (const GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "appTangent_modelspace"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "appTangent_modelspace"), 3, GL_FLOAT, GL_FALSE, vertexSize, (const GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "appBinormal_modelspace"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "appBinormal_modelspace"), 3, GL_FLOAT, GL_FALSE, vertexSize, (const GLvoid*)(11 * sizeof(GLfloat)));
	printOpenGLError();

	//Element vertex IDs
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[1]);

	printOpenGLError();
}

GLvoid display(GLvoid){
	printOpenGLError();
	glClearColor(0.3f, 0.3f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shaderPlumbing();
	glDrawElements(GL_PATCHES, 3 * ntriangles, GL_UNSIGNED_INT, (void*)0);
	printOpenGLError();

	glutSwapBuffers();
	glutPostRedisplay();

	//Unbinding stuff
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(glGetAttribLocation(basicShader, "appPosition_modelspace"));
	glDisableVertexAttribArray(glGetAttribLocation(basicShader, "appNormal_modelspace"));
	glDisableVertexAttribArray(glGetAttribLocation(basicShader, "appTexCoord"));
	glDisableVertexAttribArray(glGetAttribLocation(basicShader, "appTangent_modelspace"));
	glDisableVertexAttribArray(glGetAttribLocation(basicShader, "appBinormal_modelspace"));
}

GLvoid initShaders() {
	basicShader = InitShader("basicShader.vert", "basicShader.frag", "basicShader.tesc", "basicShader.tese", "basicShader.geom");
	glUseProgram(basicShader);
	printOpenGLError();
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
	case 'r':
	case 'R':
		g_bRotateModel = !g_bRotateModel;
		break;
	case 'a':
	case 'A':
		fov -= 2.0f;
		Projection = glm::perspective(glm::radians(fov), (GLfloat)wWidth / (GLfloat)wHeight, 0.1f, 100.0f);
		break;
	case 'z':
		fov += 2.0f;
		Projection = glm::perspective(glm::radians(fov), (GLfloat)wWidth / (GLfloat)wHeight, 0.1f, 100.0f);
		break;
	case 'w':
	case 'W':
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		break;
	case 'p':
	case 'P':
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		break;
	case '-':
	case '_':
		--tessLevel;
		break;
	case '=':
	case '+':
		++tessLevel;
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
	if (g_bRotateModel){
		View = glm::rotate(View, 0.05f, glm::vec3(0.0, 0.5, 0.0));
	}
}

GLint initGL(GLint *argc, GLchar **argv)
{
	glutInit(argc, argv);
	glutIdleFunc(process);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(wWidth, wHeight);
	glutCreateWindow("3 - Mapping Techniques");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glewInit();
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	return 1;
}

inline GLfloat interpolate(const GLfloat a, const GLfloat b, const GLfloat coefficient)
{
	return a + coefficient * (b - a);
}

Vertex tempPoint = { 0.0f, 0.0f, 0.0f };

GLint vertex_cb(p_ply_argument argument) {
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
GLint face_cb(p_ply_argument argument) {
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

GLvoid initTextures(){
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wNor, hNor, 0, GL_RGB, GL_UNSIGNED_BYTE, normalmap);
	glUniform1i(glGetUniformLocation(basicShader, "nor"), 1);
	printOpenGLError();

	//Height data
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, textureArrays[2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, wHei, hHei, 0, GL_RGB, GL_UNSIGNED_BYTE, heightmap);
	glUniform1i(glGetUniformLocation(basicShader, "hei"), 2);
	printOpenGLError();

}

GLvoid initVBO(){
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, vertexSize*nvertices, vertices->data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexbuffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(GLuint)*ntriangles, faces->data(), GL_STATIC_DRAW);

	delete vertices, faces;
}

GLint main(GLint argc, GLchar **argv)
{
	//Setting up our MVP Matrices
	Model = glm::mat4(1.0f);
	View = glm::lookAt(
		eyePos,
		glm::vec3(0, 0.5, 0),
		glm::vec3(0, 1, 0)
		);
	Projection = glm::perspective(glm::radians(fov), (GLfloat)wWidth / (GLfloat)wHeight, 0.1f, 100.0f);

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
	normalmap = SOIL_load_image("normal_cloth_texture.png", &wNor, &hNor, &cNor, SOIL_LOAD_RGB);
	heightmap = SOIL_load_image("depth_cloth_texture_lighter.png", &wHei, &hHei, &cHei, SOIL_LOAD_RGB);

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif	

	if (false == initGL(&argc, argv))
	{
		return EXIT_FAILURE;
	}

	glGenVertexArrays(1, VertexArrayIDs);
	glGenBuffers(2, vertexbuffers);
	glGenTextures(3, textureArrays);

	computeTangents();
	initShaders();
	initTextures();

	initVBO();

	delete texture, normalmap, heightmap;

	glutMainLoop();

	return EXIT_SUCCESS;
}