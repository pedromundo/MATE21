//Old C includes
#include <Windows.h>
#include <cstdio>
#include <cfloat>
#include <sys/stat.h>
#include <NuiApi.h>
#include <SOIL.h>

//Cpp includes
#include <iostream>

//OpenGL Stuff
#include <GL/glew.h>
#include <GL/glut.h>

//My includes
#include "myDataStructures.h"
#include "initShaders.h"

GLvoid reshape(GLint x, GLint y);

GLboolean g_bExitESC = false;

//Shader Program Handle
GLuint basicShader;
//Window Dimensions
GLuint wWidth = 640, wHeight = 480;
//Data Dimensions
GLuint dWidth = 640, dHeight = 480, nvertices = dWidth * dHeight;
//Handlers for the VBO and FBOs
GLuint VertexArrayIDs[1], vertexbuffers[2];
//Control Variables
GLboolean saveFrame = false;
GLuint saveNumber = 0;

//Kinect and Windows.h stuff
BYTE *m_depthRGBX;
INuiSensor* m_pNuiSensor;
HANDLE m_hNextDepthFrameEvent = INVALID_HANDLE_VALUE;
HANDLE m_pDepthStreamHandle = INVALID_HANDLE_VALUE;

//Using std::vector because ffs no one wants to work with arrays in 2016
std::vector<Point>* points = new std::vector<Point>();

/// <summary>
/// Handle new depth data
/// </summary>
void ProcessDepth(void)
{
	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;

	// Attempt to get the depth frame	
	hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &imageFrame);

	if (FAILED(hr)){
		return;
	}

	INuiFrameTexture* pTexture;

	// Get the depth image pixel texture
	hr = m_pNuiSensor->NuiImageFrameGetDepthImagePixelFrameTexture(
		m_pDepthStreamHandle, &imageFrame, false, &pTexture);
	if (FAILED(hr))
	{
		goto ReleaseFrame;
	}

	NUI_LOCKED_RECT LockedRect;

	// Lock the frame data so the Kinect knows not to modify it while we're reading it
	pTexture->LockRect(0, &LockedRect, NULL, 0);

	// Make sure we've received valid data
	if (LockedRect.Pitch != 0)
	{
		// Limiar de profundidade
		// Get the min and max reliable depth for the current frame
		GLuint minDepth = 700;
		GLuint maxDepth = 920;

		BYTE * rgbrun = m_depthRGBX;
		const NUI_DEPTH_IMAGE_PIXEL * pBufferRun = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL *>(LockedRect.pBits);

		// end pixel is start + width*height - 1
		const NUI_DEPTH_IMAGE_PIXEL * pBufferEnd = pBufferRun + (dWidth * dHeight);

		while (pBufferRun < pBufferEnd)
		{
			// discard the portion of the depth that contains only the player index
			USHORT depth = pBufferRun->depth;

			// To convert to a byte, we're discarding the most-significant
			// rather than least-significant bits.
			// We're preserving detail, although the intensity will "wrap."
			// Values outside the reliable depth range are mapped to 0 (black).

			// Note: Using conditionals in this loop could degrade performance.
			// Consider using a lookup table instead when writing production code.
			BYTE intensity = static_cast<BYTE>(depth >= minDepth && depth <= maxDepth ? (depth == 0 || depth > 1023 ? 0 : 255 - (BYTE)(((float)depth / 1023.0f) * 255.0f)) : 0);

			// Write out blue byte
			*(rgbrun++) = intensity;

			// Write out green byte
			*(rgbrun++) = intensity;

			// Write out red byte
			*(rgbrun++) = intensity;

			// We're outputting BGR, the last byte in the 32 bits is unused so skip it
			// If we were outputting BGRA, we would write alpha here.
			*(rgbrun++) = 255;

			// Increment our index into the Kinect's depth buffer
			++pBufferRun;
		}

		//Write Bitmap if this flag is set, this is actually BGR and im writing RGB
		//but it shouldnt matter since its grayscale
		if (saveFrame) {
			char fileName[255]; // enough to hold all numbers up to 64-bits
			sprintf(fileName, "depthImage%d.bmp", saveNumber++);
			SOIL_save_image(fileName, SOIL_SAVE_TYPE_BMP, dWidth, dHeight, 4, m_depthRGBX);
			saveFrame = false;
		}
	}

	// We're done with the texture so unlock it
	pTexture->UnlockRect(0);

	pTexture->Release();

ReleaseFrame:
	// Release the frame
	m_pNuiSensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);
}

GLvoid shaderPlumbing(){
	//Point size 1 looks like shit
	glPointSize(2);

	//position data
	glBindVertexArray(VertexArrayIDs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*nvertices, points->data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "aPosition"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	//color data
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffers[1]);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(BYTE)*nvertices, m_depthRGBX, GL_STATIC_DRAW);
	glEnableVertexAttribArray(glGetAttribLocation(basicShader, "aColor"));
	glVertexAttribPointer(glGetAttribLocation(basicShader, "aColor"), 4, GL_UNSIGNED_BYTE, GL_FALSE, 0, (GLvoid*)0);
}

GLvoid display(GLvoid){
	glClearColor(0.3f, 0.3f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
	shaderPlumbing();

	glDrawArrays(GL_POINTS, 0, (GLsizei)nvertices);
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
	case 's':
	case 'S':
		saveFrame = true;
		break;
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
	glutCreateWindow("Kinect Depth Image Viewer - Press S to capture the current frame");
	glutDisplayFunc(display);
	glutIdleFunc(ProcessDepth);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glewInit();
	return 1;
}

inline GLfloat interpolate(const GLfloat a, const GLfloat b, const GLfloat coefficient)
{
	return a + coefficient * (b - a);
}

GLint main(GLint argc, GLchar **argv)
{
	m_depthRGBX = new BYTE[dWidth * dHeight * 4];

	for (GLuint i = 0; i < dHeight; i++)
	{
		for (GLuint j = 0; j < dWidth; j++)
		{
			points->push_back({ interpolate(-1.0f, 1.0f, j / (GLfloat)(dWidth - 1)), interpolate(1.0f, -1.0f, i / (GLfloat)(dHeight - 1)), 1.0f });
		}
	}

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif	

	if (false == initGL(&argc, argv))
	{
		return EXIT_FAILURE;
	}

	glGenVertexArrays(1, VertexArrayIDs);
	glGenBuffers(2, vertexbuffers);

	initShaders();

	INuiSensor * pNuiSensor;
	HRESULT hr;

	int iSensorCount = 0;
	hr = NuiGetSensorCount(&iSensorCount);
	if (FAILED(hr))
	{
		return hr;
	}

	cout << "Hang on... trying to find a ready Kinect sensor..." << endl;

	// Look at each Kinect sensor
	for (GLuint i = 0; i < iSensorCount; ++i)
	{
		// Create the sensor so we can check status, if we can't create it, move on to the next
		hr = NuiCreateSensorByIndex(i, &pNuiSensor);
		if (FAILED(hr))
		{
			continue;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		if (S_OK == hr)
		{
			cout << "Kinect OK, let's begin!" << endl;
			m_pNuiSensor = pNuiSensor;
			break;
		}

		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();
	}

	if (NULL != m_pNuiSensor)
	{
		// Initialize the Kinect and specify that we'll be using depth
		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH);

		if (SUCCEEDED(hr))
		{
			// Create an event (really just a flag) that will be signaled when depth data is available
			m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			// Open a depth image stream to receive depth frames
			hr = m_pNuiSensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_DEPTH,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hNextDepthFrameEvent,
				&m_pDepthStreamHandle);
		}
	}

	if (NULL == m_pNuiSensor || FAILED(hr))
	{
		cout << "No ready Kinect found!";
		return E_FAIL;
	}

	glutMainLoop();

	return EXIT_SUCCESS;
}