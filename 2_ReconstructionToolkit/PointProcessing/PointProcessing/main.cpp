#define CGAL_EIGEN3_ENABLED

#include "stdafx.h"
using namespace std;

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

int main(int argc, char **argv)
{

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

	FT sm_angle; // Min triangle angle in degrees.
	FT sm_radius; // Max triangle size w.r.t. point set average spacing.
	FT sm_distance; // Surface Approximation error w.r.t. point set average spacing.

	// Poisson options
	std::string input;
	cout << "Enter minimum desired triangle angle (in degrees): (20.0)";
	cin.clear();
	cin.ignore(INT_MAX, '\n');
	std::getline(std::cin, input);
	if (!input.empty()) {
		std::istringstream stream(input);
		stream >> sm_angle;
	}
	else{
		sm_angle = 20;
	}

	cout << "\nEnter maximum desired triangle size: (30.0)";
	std::getline(std::cin, input);
	if (!input.empty()) {
		std::istringstream stream(input);
		stream >> sm_radius;
	}
	else{
		sm_radius = 30;
	}

	cout << "\nEnter maximum desired error factor: (1.0)";
	std::getline(std::cin, input);
	if (!input.empty()) {
		std::istringstream stream(input);
		stream >> sm_distance;
	}
	else{
		sm_distance = 1.0;
	}

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

	cout << "Generating mesh... (might take a long time)" << endl;
	// Generates surface mesh with manifold option
	STr tr; // 3D Delaunay triangulation for surface mesh generation
	C2t3 c2t3(tr); // 2D complex in 3D Delaunay triangulation
	CGAL::make_surface_mesh(c2t3,             // reconstructed mesh
		surface,                              // implicit surface
		criteria,                             // meshing criteria
		CGAL::Manifold_with_boundary_tag());  // require manifold mesh

	if (tr.number_of_vertices() == 0)
		return EXIT_FAILURE;

	cout << "Enter the desired filename of the output mesh (poisson parameters will be prepended): ";
	char outName[255];
	cin >> outName;
	cout << "Writing to file..." << endl;
	//Writes reconstructed surface mesh to file
	stringstream ss;
	ss << sm_angle << "-" << sm_radius << "-" << sm_distance << "-" << outName;
	std::ofstream out(ss.str());
	Polyhedron output_mesh;
	CGAL::output_surface_facets_to_polyhedron(c2t3, output_mesh);
	CGAL::print_polyhedron_wavefront(out, output_mesh);
	out.close();
	//TODO load and display .obj file
	cout << "All done!" << endl;
	return 0;
}