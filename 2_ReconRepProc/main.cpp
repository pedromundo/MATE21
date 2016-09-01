#define CGAL_EIGEN3_ENABLED
//Old C includes
#include <sys/stat.h>

//Cpp includes
#include <iostream>
#include <utility>
#include <list>
#include <vector>
#include <fstream>
#include <sstream>

//OpenGL Stuff
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GL/glut.h>
#include <SOIL.h>

//CGAL STUff
#include <CGAL/trace.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/IO/output_surface_facets_to_polyhedron.h>
#include <CGAL/Poisson_reconstruction_function.h>
#include <CGAL/Point_with_normal_3.h>
#include <CGAL/compute_average_spacing.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/pca_estimate_normals.h>
#include <CGAL/mst_orient_normals.h>
#include <CGAL/property_map.h>
#include <CGAL/IO/read_xyz_points.h>
#include <CGAL/IO/print_wavefront.h>

//My includes
#include "myDataStructures.h"
#include "initShaders.h"

// Types
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::FT FT;
typedef CGAL::Point_with_normal_3<Kernel> Point_with_normal;
typedef Kernel::Sphere_3 Sphere;
typedef std::vector<Point_with_normal> PointList;
typedef CGAL::Polyhedron_3<Kernel> Polyhedron;
typedef CGAL::Poisson_reconstruction_function<Kernel> Poisson_reconstruction_function;
typedef CGAL::Surface_mesh_default_triangulation_3 STr;
typedef CGAL::Surface_mesh_complex_2_in_triangulation_3<STr> C2t3;
typedef CGAL::Implicit_surface_3<Kernel, Poisson_reconstruction_function> Surface_3;
typedef Kernel::Point_3 CGALPoint;
typedef Kernel::Vector_3 Vector;
// Point with normal vector stored in a std::pair.
typedef std::pair<CGALPoint, Vector> PointVectorPair;

void reshape(GLint x, GLint y);

// Particle data
GLuint vbo = 0, colorsVBO = 0;                 // OpenGL vertex buffer object

GLboolean g_bExitESC = false;
GLubyte *image;
GLuint axisShader, wWidth = 1024, wHeight = 768;
std::vector<Point>* points = new std::vector<Point>();
std::vector<Color>* colors = new std::vector<Color>();
GLint64 nvertices;
glm::mat4 Projection, View, Model, MVP;

int loadImage(const GLchar* imagePath){
	struct stat buffer;
	if (stat(imagePath, &buffer) == 0){
		GLint width, height;
		image = SOIL_load_image(imagePath, &width, &height, 0, SOIL_LOAD_RGB);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		return strcmp(SOIL_last_result(), "Image loaded");
	}
	else{
		return 0;
	}
}

void shaderPlumbing(){
	glProgramUniformMatrix4fv(axisShader, glGetUniformLocation(axisShader, "uMVP"), 1, false, glm::value_ptr(MVP));

	glPointSize(1);

	glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aPosition"));
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(GLfloat)*nvertices, points->data(), GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(axisShader, "aPosition"), 3, GL_FLOAT, GL_FALSE, 0, points->data());
	glDrawArrays(GL_POINTS, 0, (GLsizei)nvertices);

	if (!colors->empty()){
		glEnableVertexAttribArray(glGetAttribLocation(axisShader, "aColor"));
		glBindBuffer(GL_ARRAY_BUFFER, colorsVBO);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(GLfloat)*nvertices, colors->data(), GL_STATIC_DRAW);
		glVertexAttribPointer(glGetAttribLocation(axisShader, "aColor"), 4, GL_FLOAT, GL_FALSE, 0, colors->data());
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

float rot = 0.0;
void display(void){
	rot += 0.01f;

	Projection = glm::perspective(glm::radians(45.0f),
		(float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT),
		0.1f, 20.0f);

	View = glm::lookAt(glm::vec3(-2.0, -2.0, -2.0),
		glm::vec3(0.0, 0.0, 0.0),
		glm::vec3(0.0, 1.0, 0.0));

	Model = glm::mat4(1.0f);
	Model = glm::rotate(Model, 0.0f, glm::vec3(1.0, 0.0, 0.0));
	Model = glm::rotate(Model, rot, glm::vec3(0.0, 1.0, 0.0));
	Model = glm::rotate(Model, 0.0f, glm::vec3(0.0, 0.0, 1.0));

	MVP = Projection * View * Model;

	glClearColor(0.3f, 0.3f, 0.0f, 1.0f);
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

void keyboard(GLubyte key, GLint x, GLint y)
{
	char* fileName = new char[255];
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
	case 'o':
	case 'O':
		cout << "Enter file name: ";
		fflush(stdin);
		cin >> fileName;
		loadImage(fileName);
		break;
	case 'c':
	case 'C':
		//generateColor();
		break;
	case 'r':
	case 'R':
		break;
	default:
		break;
	}
}

void reshape(GLint x, GLint y)
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
	glutCreateWindow("Mesh Reconstruction");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	/*glutMouseFunc(click);
	glutMotionFunc(motion);*/
	glutReshapeFunc(reshape);
	glewInit();
	return 1;
}

GLint main(GLint argc, GLchar **argv)
{

#if defined(__linux__)
	setenv("DISPLAY", ":0", 0);
#endif	

	if (false == initGL(&argc, argv))
	{
		exit(EXIT_FAILURE);
	}

	char fname[255];
	cout << "Enter the name of the .xyz file holding the point set to be reconstructed: ";
	cin >> fname;
	std::list<PointVectorPair> points;
	std::ifstream stream(fname);
	if (!stream ||
		!CGAL::read_xyz_points(stream,
		std::back_inserter(points),
		CGAL::First_of_pair_property_map<PointVectorPair>()))
	{
		std::cerr << "Error: cannot read file " << fname << std::endl;
		return EXIT_FAILURE;
	}

	cout << "Estimating normals..." << endl;
	// Estimates normals direction.	
	const int nb_neighbors = 18; // K-nearest neighbors = 3 rings
	CGAL::pca_estimate_normals<CGAL::Sequential_tag>(points.begin(), points.end(),
		CGAL::First_of_pair_property_map<PointVectorPair>(),
		CGAL::Second_of_pair_property_map<PointVectorPair>(),
		nb_neighbors);

	cout << "Orienting normals... (might take a while)" << endl;
	// Orients normals -- for some reason only works in release mode.	
	std::list<PointVectorPair>::iterator unoriented_points_begin =
		CGAL::mst_orient_normals(points.begin(), points.end(),
		CGAL::First_of_pair_property_map<PointVectorPair>(),
		CGAL::Second_of_pair_property_map<PointVectorPair>(),
		nb_neighbors);
	// Optional: delete points with an unoriented normal
	// if you plan to call a reconstruction algorithm that expects oriented normals.
	points.erase(unoriented_points_begin, points.end());

	// Poisson options
	FT sm_angle = 20.0; // Min triangle angle in degrees.
	FT sm_radius = 30; // Max triangle size w.r.t. point set average spacing.
	FT sm_distance = 0.375; // Surface Approximation error w.r.t. point set average spacing.

	PointList poissonPoints = PointList();

	std::list<PointVectorPair>::iterator itePoints;

	for (itePoints = points.begin();
		itePoints != points.end();
		itePoints++)
	{
		PointVectorPair currentPoint = (*itePoints);
		poissonPoints.push_back(Point_with_normal(currentPoint.first.x(), currentPoint.first.y(), currentPoint.first.z(), currentPoint.second));
	}

	cout << "Creating the implicit function..." << endl;
	// Creates implicit function from the read points using the default solver.
	Poisson_reconstruction_function function(poissonPoints.begin(), poissonPoints.end(),
		CGAL::make_normal_of_point_with_normal_pmap(PointList::value_type()));

	cout << "Computing the implicit function for each vertex..." << endl;
	// Computes the Poisson indicator function f()
	// at each vertex of the triangulation.
	if (!function.compute_implicit_function(true)){
		return EXIT_FAILURE;
	}

	cout << "Computing average spacing..." << endl;
	// Computes average spacing
	FT average_spacing = CGAL::compute_average_spacing<CGAL::Sequential_tag>(poissonPoints.begin(), poissonPoints.end(),
		6 /* knn = 1 ring */);

	cout << "Computing bounding sphere..." << endl;
	// Gets one point inside the implicit surface
	// and computes implicit function bounding sphere radius.
	CGALPoint inner_point = function.get_inner_point();	
	Sphere bsphere = function.bounding_sphere();
	FT radius = std::sqrt(bsphere.squared_radius());

	cout << "Defining the implicit surface..." << endl;
	// Defines the implicit surface: requires defining a
	// conservative bounding sphere centered at inner point.
	FT sm_sphere_radius = 5.0 * radius;
	FT sm_dichotomy_error = sm_distance*average_spacing / 1000.0; // Dichotomy error must be << sm_distance
	Surface_3 surface(function,
		Sphere(inner_point, sm_sphere_radius*sm_sphere_radius),
		sm_dichotomy_error / sm_sphere_radius);

	// Defines surface mesh generation criteria
	CGAL::Surface_mesh_default_criteria_3<STr> criteria(sm_angle,  // Min triangle angle (degrees)
		sm_radius*average_spacing,  // Max triangle size
		sm_distance*average_spacing); // Approximation error

	cout << "Generating mesh..." << endl;
	cout << "Enter the desired filename of the output mesh (poisson parameters will be prepended): ";
	char outName[255];
	cin >> outName;
	cout << "Writing to file..." << endl;
	// Generates surface mesh with manifold option
	STr tr; // 3D Delaunay triangulation for surface mesh generation
	C2t3 c2t3(tr); // 2D complex in 3D Delaunay triangulation
	CGAL::make_surface_mesh(c2t3,             // reconstructed mesh
		surface,                              // implicit surface
		criteria,                             // meshing criteria
		CGAL::Manifold_tag());  // require manifold mesh

	if (tr.number_of_vertices() == 0)
		return EXIT_FAILURE;

	//Writes reconstructed surface mesh to file
	stringstream ss;
	ss << sm_angle << "-" << sm_radius << "-" << sm_distance << "-" << outName;
	std::ofstream out(ss.str());
	Polyhedron output_mesh;
	CGAL::output_surface_facets_to_polyhedron(c2t3, output_mesh);
	CGAL::print_polyhedron_wavefront(out, output_mesh);
	out.close();
	cout << "All done! Displaying..." << endl;

	initShaders();

	glutMainLoop();
	return 0;
}