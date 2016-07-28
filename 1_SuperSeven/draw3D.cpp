//Old C includes
#include <cstdio>
#include <cfloat>

//Cpp includes
#include <iostream>

//OpenGL Stuff
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <GL/glut.h>
#include <SOIL.h>

// CUDA standard includes
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <cufft.h>
#include <rendercheck_gl.h>
#include <helper_cuda.h>
#include <helper_cuda_gl.h>

//My includes
#include "defines.h"
#include "myDataStructures.h"
#include "hello-world.cuh"

#define MAX_EPSILON_ERROR 1.0f

// CUDA example code that implements the frequency space version of
// Jos Stam's paper 'Stable Fluids' in 2D. This application uses the
// CUDA FFT library (CUFFT) to perform velocity diffusion and to
// force non-divergence in the velocity field at each time step. It uses
// CUDA-OpenGL interoperability to update the particle field directly
// instead of doing a copy to system memory before drawing. Texture is
// used for automatic bilinear interpolation at the velocity advection step.

void cleanup(void);
void reshape(GLint x, GLint y);

// CUFFT plan handle
cufftHandle planr2c;
cufftHandle planc2r;
static cData *vxfield = NULL;
static cData *vyfield = NULL;

cData *hvfield = NULL;
cData *dvfield = NULL;
static GLint wWidth = MAX(512, DIM);
static GLint wHeight = MAX(512, DIM);

static GLint clicked = 0;

// Particle data
GLuint vbo = 0, colorsVBO = 0;                 // OpenGL vertex buffer object
struct cudaGraphicsResource *cuda_vbo_resource; // handles OpenGL-CUDA exchange
static cData *particles = NULL; // particle positions in host memory
static GLint lastx = 0, lasty = 0;

// Texture pitch
size_t tPitch = 0; // Now this is compatible with gcc in 64-bit

GLchar *ref_file = NULL;
GLboolean g_bQAAddTestForce = true;
GLint  g_iFrameToCompare = 100;
GLint  g_TotalErrors = 0;

GLboolean g_bExitESC = false;

// CheckFBO/BackBuffer class objects
CheckRender       *g_CheckRender = NULL;

void addForces(cData *v, GLint dx, GLint dy, GLint spx, GLint spy, GLfloat fx, GLfloat fy, GLint r);
void advectVelocity(cData *v, GLfloat *vx, GLfloat *vy, GLint dx, GLint pdx, GLint dy, GLfloat dt);
void diffuseProject(cData *vx, cData *vy, GLint dx, GLint dy, GLfloat dt, GLfloat visc);
void updateVelocity(cData *v, GLfloat *vx, GLfloat *vy, GLint dx, GLint pdx, GLint dy);
void advectParticles(GLuint vbo, cData *v, GLint dx, GLint dy, GLfloat dt);

GLuint InitShader(const GLchar *vShaderFile, const GLchar *fShaderFile);

GLuint axisShader;

GLfloat *colors, *chromaKeyingBase = new GLfloat[4]{1.0f, 0.0f, 0.0f, 1.0f}, *chromaKeyingDest = new GLfloat[4]{1.0f, 0.0f, 0.0f, 1.0f};
GLubyte *image;

void bindImage(){
	GLint width, height;
	image = SOIL_load_image("ultraseven.jpg", &width, &height, 0, SOIL_LOAD_RGB);
	cout << SOIL_last_result() << endl;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
}

void generateColor() {
	chromaKeyingDest = (GLfloat *)malloc(4 * DS * sizeof(GLfloat));

	for (GLuint var = 0; var < 4; ++var) {
		chromaKeyingDest[var] = (GLfloat)rand() / RAND_MAX;
	}
}

void simulateFluids(void)
{
	// simulate fluid
	advectVelocity(dvfield, (GLfloat *)vxfield, (GLfloat *)vyfield, DIM, RPADW, DIM, DT);
	diffuseProject(vxfield, vyfield, CPADW, DIM, DT, VIS);
	updateVelocity(dvfield, (GLfloat *)vxfield, (GLfloat *)vyfield, DIM, RPADW, DIM);
	advectParticles(vbo, dvfield, DIM, DIM, DT);
}

glm::mat4 Model, View, Projection;
glm::mat4 MVP = Projection * View * Model;

void shaderPlumbing(){
	glProgramUniformMatrix4fv(axisShader, glGetUniformLocation(axisShader, "uMVP"), 1, false, glm::value_ptr(MVP));
	glProgramUniform4fv(axisShader, glGetUniformLocation(axisShader, "aChromaKeyingBase"), 1, chromaKeyingBase);
	glProgramUniform4fv(axisShader, glGetUniformLocation(axisShader, "aChromaKeyingDest"), 1, chromaKeyingDest);

	glPointSize(1);

	glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aPosition"));
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(glGetAttribLocation(axisShader, "aPosition"), 2, GL_FLOAT, 0, 0, NULL);
	glDrawArrays(GL_POINTS, 0, DS);

	glBindBuffer(GL_ARRAY_BUFFER, colorsVBO);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLubyte)*DS, image, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(glGetAttribLocation(axisShader, "aColor"), 3,
		GL_UNSIGNED_BYTE, GL_FALSE, 0, image);
	glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aColor"));

	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void display(void)
{
	simulateFluids();

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

// very simple von neumann middle-square prng.  can't use rand() in -qatest
// mode because its implementation varies across platforms which makes testing
// for consistency in the important parts of this program difficult.
GLfloat myrand(void)
{
	static GLint seed = 72191;
	GLchar sq[22];

	if (ref_file)
	{
		seed *= seed;
		sprintf(sq, "%010d", seed);
		// pull the middle 5 digits out of sq
		sq[8] = 0;
		seed = atoi(&sq[3]);

		return seed / 99999.f;
	}
	else
	{
		return rand() / (GLfloat)RAND_MAX;
	}
}

void initParticles(cData *p, GLint dx, GLint dy)
{
	GLint i, j;

	for (i = 0; i < dy; i++)
	{
		for (j = 0; j < dx; j++)
		{
			p[i*dx + j].x = (j + 0.5f + (myrand() - 0.5f)) / dx;
			p[i*dx + j].y = (i + 0.5f + (myrand() - 0.5f)) / dy;
		}
	}
}

void keyboard(GLubyte key, GLint x, GLint y)
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
	case 'c':
	case 'C':
		generateColor();
		break;
	case 'r':
	case 'R':
		memset(hvfield, 0, sizeof(cData) * DS);
		cudaMemcpy(dvfield, hvfield, sizeof(cData) * DS,
			cudaMemcpyHostToDevice);

		initParticles(particles, DIM, DIM);

		cudaGraphicsUnregisterResource(cuda_vbo_resource);

		getLastCudaError("cudaGraphicsUnregisterBuffer failed");

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cData) * DS,
			particles, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		cudaGraphicsGLRegisterBuffer(&cuda_vbo_resource, vbo, cudaGraphicsMapFlagsNone);

		getLastCudaError("cudaGraphicsGLRegisterBuffer failed");
		break;

	default:
		break;
	}
}

void click(GLint button, GLint updown, GLint x, GLint y)
{
	lastx = x;
	lasty = y;

	clicked = !clicked;
}

void motion(GLint x, GLint y)
{
	// Convert motion coordinates to domain
	GLfloat fx = (lastx / (GLfloat)wWidth);
	GLfloat fy = (lasty / (GLfloat)wHeight);
	GLint nx = (GLint)(fx * DIM);
	GLint ny = (GLint)(fy * DIM);

	if (clicked && nx < DIM - FR && nx > FR - 1 && ny < DIM - FR && ny > FR - 1)
	{
		GLint ddx = x - lastx;
		GLint ddy = y - lasty;
		fx = ddx / (GLfloat)wWidth;
		fy = ddy / (GLfloat)wHeight;
		GLint spy = ny - FR;
		GLint spx = nx - FR;
		addForces(dvfield, DIM, DIM, spx, spy, FORCE * DT * fx, FORCE * DT * fy, FR);
		lastx = x;
		lasty = y;
	}

	glutPostRedisplay();
}

void reshape(GLint x, GLint y)
{
	wWidth = x;
	wHeight = y;
	glViewport(0, 0, x, y);
	glutPostRedisplay();
}

void cleanup(void)
{
	cudaGraphicsUnregisterResource(cuda_vbo_resource);

	unbindTexture();
	deleteTexture();

	// Free all host and device resources
	free(hvfield);
	free(particles);
	cudaFree(dvfield);
	cudaFree(vxfield);
	cudaFree(vyfield);
	cufftDestroy(planr2c);
	cufftDestroy(planc2r);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &vbo);

	if (g_bExitESC)
	{
		// cudaDeviceReset causes the driver to clean up all state. While
		// not mandatory in normal operation, it is good practice.  It is also
		// needed to ensure correct operation when the application is being
		// profiled. Calling cudaDeviceReset causes all profile data to be
		// flushed before the application exits
		checkCudaErrors(cudaDeviceReset());
	}
}

GLint initGL(GLint *argc, GLchar **argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(wWidth, wHeight);
	glutCreateWindow("Compute Stable Fluids");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(click);
	glutMotionFunc(motion);
	glutReshapeFunc(reshape);

	glewInit();

	return true;
}


GLint main(GLint argc, GLchar **argv)
{
	GLint devID;
	cudaDeviceProp deviceProps;

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif	

	// First initialize OpenGL context, so we can properly set the GL for CUDA.
	// This is necessary in order to achieve optimal performance with OpenGL/CUDA interop.
	if (false == initGL(&argc, argv))
	{
		exit(EXIT_SUCCESS);
	}

	// use command-line specified CUDA device, otherwise use device with highest Gflops/s
	devID = findCudaGLDevice(argc, (const GLchar **)argv);

	// get number of SMs on this GPU
	checkCudaErrors(cudaGetDeviceProperties(&deviceProps, devID));
	printf("CUDA device [%s] has %d Multi-Processors\n",
		deviceProps.name, deviceProps.multiProcessorCount);

	// automated build testing harness
	if (checkCmdLineFlag(argc, (const GLchar **)argv, "file"))
	{
		getCmdLineArgumentString(argc, (const GLchar **)argv, "file", &ref_file);
	}

	// Allocate and initialize host data	
	bindImage();
	initShaders();

	hvfield = (cData *)malloc(sizeof(cData) * DS);
	memset(hvfield, 0, sizeof(cData) * DS);

	// Allocate and initialize device data
	cudaMallocPitch((GLvoid **)&dvfield, &tPitch, sizeof(cData)*DIM, DIM);

	cudaMemcpy(dvfield, hvfield, sizeof(cData) * DS,
		cudaMemcpyHostToDevice);
	// Temporary complex velocity field data
	cudaMalloc((GLvoid **)&vxfield, sizeof(cData) * PDS);
	cudaMalloc((GLvoid **)&vyfield, sizeof(cData) * PDS);

	setupTexture(DIM, DIM);
	bindTexture();

	// Create particle array
	particles = (cData *)malloc(sizeof(cData) * DS);
	memset(particles, 0, sizeof(cData) * DS);

	initParticles(particles, DIM, DIM);

	// Create CUFFT transform plan configuration
	checkCudaErrors(cufftPlan2d(&planr2c, DIM, DIM, CUFFT_R2C));
	checkCudaErrors(cufftPlan2d(&planc2r, DIM, DIM, CUFFT_C2R));
	// TODO: update kernels to use the new unpadded memory layout for perf
	// rather than the old FFTW-compatible layout
	cufftSetCompatibilityMode(planr2c, CUFFT_COMPATIBILITY_FFTW_PADDING);
	cufftSetCompatibilityMode(planc2r, CUFFT_COMPATIBILITY_FFTW_PADDING);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cData) * DS,
		particles, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_vbo_resource, vbo, cudaGraphicsMapFlagsNone));

	getLastCudaError("cudaGraphicsGLRegisterBuffer failed");


#if defined (__APPLE__) || defined(MACOSX)
	atexit(cleanup);
#else
	glutCloseFunc(cleanup);
#endif
	glutMainLoop();

	// cudaDeviceReset causes the driver to clean up all state. While
	// not mandatory in normal operation, it is good practice.  It is also
	// needed to ensure correct operation when the application is being
	// profiled. Calling cudaDeviceReset causes all profile data to be
	// flushed before the application exits
	cudaDeviceReset();
	exit(EXIT_SUCCESS);
	return 0;
}