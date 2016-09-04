# MATE21

## Project 1 - UltraSeven Effect
You have to include/link against:
- freeglut
- glew1.13
- glm
- SOIL
- CUDA
- The usual cuda headers/libs (tested with 7.5, should work with anything not too old)

## Project 2 - Object Reconstruction
Comprises several applications, each with their own dependencies:
- DepthSaver (Windows Only)
 - freeglut
 - SOIL
 - Kinect SDK (Kinect10.lib)
- Depth2Mesh
 - freeglut
 - SOIL
- PointProcessing
 - freeglut
 - GLEW
 - GLM
 - SOIL
 - CGAL
 - EIGEN3
 - GMP (included in CGAL)
 - BOOST
 
### General workflow
1. Capture a depth frame in DepthSaver by pressing S
2. Run Depth2Mesh and follow and use the console interface to convert your depth image to a .xyz file
3. Run PointProcessing and use the console window (not the openGL one) to process your point set and output an .obj file
4. Open the generated .obj file in Meshlab and project your high-quality to the mesh as vertex color, generate a parametrization of the mesh with color information and then replace the generated parametrization with a properly aligned and scaled version of your high-quality texture.
