#pragma once
#include "Vector3.h"
class matrix3x3
{
public:
	matrix3x3();
	~matrix3x3();

	//Vector3 operator * (float scale) const;
	matrix3x3 operator *(Vector3 vec);
private:
	float mat[3][3];
};

