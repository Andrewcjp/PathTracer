#pragma once

#include "Primitive.h"
#include "Triangle.h"


class TriMesh :	public Primitive
{
	private: 
		Triangle*					m_triangles;
		int							m_numtriangles;

	public:
		TriMesh();
		TriMesh(const char* filename);
		~TriMesh();

		void LoadTriMeshFromOBJFile(const char* filename);

		RayHitResult IntersectByRay(Ray& ray);
		Colour GetDiffuseColour(Vector3 point);
};

