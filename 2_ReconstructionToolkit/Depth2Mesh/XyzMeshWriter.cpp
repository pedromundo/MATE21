#include "XyzMeshWriter.h"
#include <iostream>
#include <fstream>

using namespace std;


XyzMeshWriter::XyzMeshWriter()
{
}


XyzMeshWriter::~XyzMeshWriter()
{
}

int XyzMeshWriter::writeDepthToMeshfile(const char* fileName, const unsigned char* depthData, const unsigned char* colorData, bool cullBlack, int width, int height, unsigned int minMM, unsigned int maxMM){
	float minX = -1.0f, maxX = 1.0f, minY = 1.0f, maxY = -1.0f, minZ = 0.0f, maxZ = 1.0f;
	ofstream  os;
	os.open(fileName, ios::out);
	int numPixels = width * height;
	int currPixel = 1;
	os << "#Generated by Depth2Mesh\n";
	while (currPixel <= numPixels){
		unsigned char z = *(depthData++);
		if (!(cullBlack && z == 0)){
			//Kinect calibration data, don't even remember what this is tbh
			double cx = 313.68782938, cy = 259.01834898, fx_inv = 1 / 526.37013657, fy_inv = fx_inv;
			double u = currPixel > width ? (currPixel - (currPixel / width) * width) : currPixel;
			double v = currPixel > width ? (currPixel / width) : 0.0f;

			double z_in_mm = interpolate(minMM, maxMM, z / 255.0);

			double vx = z_in_mm * (u - cx) * fx_inv;
			double vy = -z_in_mm * (v - cy) * fy_inv;
			double vz = z_in_mm;

			os << vx;
			os << "\t";
			os << vy;
			os << "\t";
			os << vz;
			os << "\n";
			/*os << interpolate(minX, maxX, (currPixel > width ? (currPixel - (currPixel / width) * width) / (float)width : currPixel / (float)width));
			os << "\t";
			os << interpolate(minY, maxY, (currPixel > width ? (currPixel / width) / (float)height : 0.0f));
			os << "\t";
			os << interpolate(minZ, maxZ, z / 255.0f);
			os << "\n";*/
		}
		++currPixel;
	}
	os.close();
	return os.good();
}