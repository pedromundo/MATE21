#ifndef __INIT_SHADERS__
#define __INIT_SHADERS__ 1

#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

// Define a helpful macro for handling offsets into buffer objects
#define BUFFER_OFFSET(offset) ((GLvoid *)(offset))

typedef struct Shader {
	const GLchar *filename;
	GLenum type;
	GLchar *source;
} tShader;

static GLchar *readShaderSource(const GLchar *shaderFile);

GLuint InitShader(const GLchar *vShaderFile, const GLchar *fShaderFile, const GLchar *tcShaderFile, const GLchar *teShaderFile, const GLchar *gShaderFile);

#endif //__INIT_SHADERS__
