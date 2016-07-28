//Old C includes
#include <cstdio>
#include <cfloat>
#include <ctime>

//Cpp includes
#include <iostream>

//OpenGL Stuff
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <GL/glew.h>
#include <GL/glut.h>

// CUDA standard includes
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <cufft.h>
#include <helper_functions.h>
#include <rendercheck_gl.h>
#include <helper_cuda.h>
#include <helper_cuda_gl.h>

//My includes
#include "defines.h"
#include "dev_array.h"
#include "myDataStructures.h"
#include "readers.h"
#include "hello-world.cuh"

#define MAX_EPSILON_ERROR 1.0f

const char *sSDKname = "fluidsGL";
// CUDA example code that implements the frequency space version of
// Jos Stam's paper 'Stable Fluids' in 2D. This application uses the
// CUDA FFT library (CUFFT) to perform velocity diffusion and to
// force non-divergence in the velocity field at each time step. It uses
// CUDA-OpenGL interoperability to update the particle field directly
// instead of doing a copy to system memory before drawing. Texture is
// used for automatic bilinear interpolation at the velocity advection step.

void cleanup(void);
void reshape(int x, int y);

// CUFFT plan handle
cufftHandle planr2c;
cufftHandle planc2r;
static cData *vxfield = NULL;
static cData *vyfield = NULL;

cData *hvfield = NULL;
cData *dvfield = NULL;
static int wWidth = MAX(512, DIM);
static int wHeight = MAX(512, DIM);

static int clicked = 0;

// Particle data
GLuint vbo = 0, colorsVBO = 0;                 // OpenGL vertex buffer object
struct cudaGraphicsResource *cuda_vbo_resource; // handles OpenGL-CUDA exchange
static cData *particles = NULL; // particle positions in host memory
static int lastx = 0, lasty = 0;

// Texture pitch
size_t tPitch = 0; // Now this is compatible with gcc in 64-bit

char *ref_file = NULL;
bool g_bQAAddTestForce = true;
int  g_iFrameToCompare = 100;
int  g_TotalErrors = 0;

bool g_bExitESC = false;

// CheckFBO/BackBuffer class objects
CheckRender       *g_CheckRender = NULL;

void addForces(cData *v, int dx, int dy, int spx, int spy, float fx, float fy, int r);
void advectVelocity(cData *v, float *vx, float *vy, int dx, int pdx, int dy, float dt);
void diffuseProject(cData *vx, cData *vy, int dx, int dy, float dt, float visc);
void updateVelocity(cData *v, float *vx, float *vy, int dx, int pdx, int dy);
void advectParticles(GLuint vbo, cData *v, int dx, int dy, float dt);

GLuint InitShader(const GLchar *vShaderFile, const GLchar *fShaderFile);

GLuint axisShader;

GLfloat *colors;

GLvoid generateColors() {
	colors = (GLfloat *)malloc(4 * DS * sizeof(GLfloat));

	for (GLuint var = 0; var < 4 * DS; ++var) {
		colors[var] = (GLfloat)rand() / RAND_MAX;
	}
}

void simulateFluids(void)
{
	// simulate fluid
	advectVelocity(dvfield, (float *)vxfield, (float *)vyfield, DIM, RPADW, DIM, DT);
	diffuseProject(vxfield, vyfield, CPADW, DIM, DT, VIS);
	updateVelocity(dvfield, (float *)vxfield, (float *)vyfield, DIM, RPADW, DIM);
	advectParticles(vbo, dvfield, DIM, DIM, DT);
}

glm::mat4 Model, View, Projection;
glm::mat4 MVP = Projection * View * Model;

GLvoid shaderPlumbing(){
	glProgramUniformMatrix4fv(axisShader,
		glGetUniformLocation(axisShader, "uMVP"), 1, false,
		glm::value_ptr(MVP));

	glPointSize(1);
	
	glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aPosition"));
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(glGetAttribLocation(axisShader, "aPosition"), 2, GL_FLOAT, 0, 0, NULL);	
	glDrawArrays(GL_POINTS, 0, DS);

	glBindBuffer(GL_ARRAY_BUFFER, colorsVBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat)*DS, colors, GL_STREAM_DRAW);

	glVertexAttribPointer(glGetAttribLocation(axisShader, "aColor"), 2,
		GL_FLOAT, GL_FALSE, 0, colors);
	glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aColor"));

	glUnmapBuffer(GL_ARRAY_BUFFER);
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
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableClientState(GL_VERTEX_ARRAY);	
	glDisable(GL_TEXTURE_2D);

	// Finish timing before swap buffers to avoid refresh sync	
	glutSwapBuffers();

	glutPostRedisplay();
}

GLvoid initShaders() {
	axisShader = InitShader("axisShader.vert", "axisShader.frag");
}

// very simple von neumann middle-square prng.  can't use rand() in -qatest
// mode because its implementation varies across platforms which makes testing
// for consistency in the important parts of this program difficult.
float myrand(void)
{
	static int seed = 72191;
	char sq[22];

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
		return rand() / (float)RAND_MAX;
	}
}

void initParticles(cData *p, int dx, int dy)
{
	int i, j;

	for (i = 0; i < dy; i++)
	{
		for (j = 0; j < dx; j++)
		{
			p[i*dx + j].x = (j + 0.5f + (myrand() - 0.5f)) / dx;
			p[i*dx + j].y = (i + 0.5f + (myrand() - 0.5f)) / dy;
		}
	}
}

//void initParticles(cData *p, int dx, int dy)
//{
//	int i, j;
//
//	for (i = 0; i < dy; i++)
//	{
//		for (j = 0; j < dx; j++)
//		{
//			p[i*dx + j].x = (j + 0.5f + (myrand() - 0.5f)) / dx;
//			p[i*dx + j].y = (i + 0.5f + (myrand() - 0.5f)) / dy;
//		}
//	}
//}

void keyboard(unsigned char key, int x, int y)
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

void click(int button, int updown, int x, int y)
{
	lastx = x;
	lasty = y;

	clicked = !clicked;
}

void motion(int x, int y)
{
	// Convert motion coordinates to domain
	float fx = (lastx / (float)wWidth);
	float fy = (lasty / (float)wHeight);
	int nx = (int)(fx * DIM);
	int ny = (int)(fy * DIM);

	if (clicked && nx < DIM - FR && nx > FR - 1 && ny < DIM - FR && ny > FR - 1)
	{
		int ddx = x - lastx;
		int ddy = y - lasty;
		fx = ddx / (float)wWidth;
		fy = ddy / (float)wHeight;
		int spy = ny - FR;
		int spx = nx - FR;
		addForces(dvfield, DIM, DIM, spx, spy, FORCE * DT * fx, FORCE * DT * fy, FR);
		lastx = x;
		lasty = y;
	}

	glutPostRedisplay();
}

void reshape(int x, int y)
{
	wWidth = x;
	wHeight = y;
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
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

int initGL(int *argc, char **argv)
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


int main(int argc, char **argv)
{
	int devID;
	cudaDeviceProp deviceProps;

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif

	printf("%s Starting...\n\n", sSDKname);

	printf("NOTE: The CUDA Samples are not meant for performance measurements. Results may vary when GPU Boost is enabled.\n\n");

	// First initialize OpenGL context, so we can properly set the GL for CUDA.
	// This is necessary in order to achieve optimal performance with OpenGL/CUDA interop.
	if (false == initGL(&argc, argv))
	{
		exit(EXIT_SUCCESS);
	}

	// use command-line specified CUDA device, otherwise use device with highest Gflops/s
	devID = findCudaGLDevice(argc, (const char **)argv);

	// get number of SMs on this GPU
	checkCudaErrors(cudaGetDeviceProperties(&deviceProps, devID));
	printf("CUDA device [%s] has %d Multi-Processors\n",
		deviceProps.name, deviceProps.multiProcessorCount);

	// automated build testing harness
	if (checkCmdLineFlag(argc, (const char **)argv, "file"))
	{
		getCmdLineArgumentString(argc, (const char **)argv, "file", &ref_file);
	}

	// Allocate and initialize host data	
	initShaders();
	generateColors();

	hvfield = (cData *)malloc(sizeof(cData) * DS);
	memset(hvfield, 0, sizeof(cData) * DS);

	// Allocate and initialize device data
	cudaMallocPitch((void **)&dvfield, &tPitch, sizeof(cData)*DIM, DIM);

	cudaMemcpy(dvfield, hvfield, sizeof(cData) * DS,
		cudaMemcpyHostToDevice);
	// Temporary complex velocity field data
	cudaMalloc((void **)&vxfield, sizeof(cData) * PDS);
	cudaMalloc((void **)&vyfield, sizeof(cData) * PDS);

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
