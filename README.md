# MATE21

## Project 1 - UltraSeven Effect
You have to include/link against:
- freeglut
- glew1.13
- glm
- SOIL
- CUDA
- The usual cuda headers/libs (tested with 7.5, should work with anything not too old)

## Project 2 - Reconstruction Toolkit
A set of tools, each with their own dependencies to reconstruct 3D models from Kinect depth maps. The Bag folder has possible input files, and a folder with a meshlab project and the achieved results, the dependencies are as follows:
- DepthSaver (Windows Only)
 - GLEW
 - freeglut
 - SOIL
 - Kinect SDK (Kinect10.lib)
- Depth2Mesh (Further development at https://github.com/pedromundo/Depth2Mesh)
 - freeglut
 - SOIL
- PointProcessing 
 - CGAL
 - EIGEN3
 - GMP (included in CGAL)
 - BOOST
 
### General workflow
1. Capture a depth frame in DepthSaver by pressing S
2. Clean your depth image up, remove outliers by painting them over with a black brush, a 1-2px gaussian blur filter also does wonders.
3. Run Depth2Mesh and follow and use the console interface to convert your depth image to a .xyz file
4. Run PointProcessing and use the console window (not the openGL one) to process your point set and output an .obj file
5. Open the generated .obj file in Meshlab and project your high-quality texture to the mesh as vertex color, generate a parametrization of the mesh with color information and then replace the generated parametrization with a properly aligned and scaled version of your high-quality texture.

## Project 3 - Mapping Techniques
This project presents a modern opengl approach to produce 3 shading effects: texture mapping, normal mapping and displacement mapping. The dependencies are:
- freeglut
- glew1.13
- glm
- SOIL
Several hotkeys are available:
- a/z - to zoom in and out
- w - change to writeframe mode
- p - change to filled polygon mode
- r - start/stop rotation
- +/- - to increase and decrese the tesselation levels
- ESC - to exit
