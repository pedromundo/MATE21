#ifndef KERNEL_CUH_
#define KERNEL_CUH_

#include "defines.h"

// Vector data type used to velocity and force fields
typedef float2 cData;

void setupTexture(GLint x, GLint y);
void bindTexture(void);
void unbindTexture(void);
void updateTexture(cData *data, size_t w, size_t h, size_t pitch);
void deleteTexture(void);

// This method adds constant force vectors to the velocity field
// stored in 'v' according to v(x,t+1) = v(x,t) + dt * f.
__global__ void
addForces_k(cData *v, GLint dx, GLint dy, GLint spx, GLint spy, GLfloat fx, GLfloat fy, GLint r, size_t pitch);

// This method performs the velocity advection step, where we
// trace velocity vectors back in time to update each grid cell.
// That is, v(x,t+1) = v(p(x,-dt),t). Here we perform bilinear
// interpolation in the velocity space.
__global__ void
advectVelocity_k(cData *v, GLfloat *vx, GLfloat *vy,
GLint dx, GLint pdx, GLint dy, GLfloat dt, GLint lb);

// This method performs velocity diffusion and forces mass conservation
// in the frequency domain. The inputs 'vx' and 'vy' are complex-valued
// arrays holding the Fourier coefficients of the velocity field ina
// X and Y. Diffusion in this space takes a simple form described as:
// v(k,t) = v(k,t) / (1 + visc * dt * k^2), where visc is the viscosity,
// and k is the wavenumber. The projection step forces the Fourier
// velocity vectors to be orthogonal to the wave wave vectors for each
// wavenumber: v(k,t) = v(k,t) - ((k dot v(k,t) * k) / k^2.
__global__ void
diffuseProject_k(cData *vx, cData *vy, GLint dx, GLint dy, GLfloat dt,
GLfloat visc, GLint lb);

// This method updates the velocity field 'v' using the two complex
// arrays from the previous step: 'vx' and 'vy'. Here we scale the
// real components by 1/(dx*dy) to account for an unnormalized FFT.
__global__ void
updateVelocity_k(cData *v, GLfloat *vx, GLfloat *vy,
GLint dx, GLint pdx, GLint dy, GLint lb, size_t pitch);

// This method updates the particles by moving particle positions
// according to the velocity field and time step. That is, for each
// particle: p(t+1) = p(t) + dt * v(p(t)).
__global__ void
advectParticles_k(cData *part, cData *v, GLint dx, GLint dy,
GLfloat dt, GLint lb, size_t pitch);

#endif