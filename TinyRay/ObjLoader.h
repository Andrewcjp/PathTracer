#pragma once
#include <vector>
#include <iostream>
#include "Vector3.h"
#include "Vector2.h"
#include "Triangle.h"
#include "Mesh.h"
class ObjLoader
{
public:
	ObjLoader();
	~ObjLoader();
	Mesh* BuildMesh(const char * path);
	
	bool ObjLoader::Load(const char * path,
		std::vector<Vector3> & out_vertices,
		std::vector<Vector3> & out_uvs,
		std::vector<Vector3> & out_normals);
private:
	std::vector<Triangle*> tris;

};

