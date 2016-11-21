/*---------------------------------------------------------------------
*
* Copyright © 2015  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#pragma once

#include "Ray.h"
#include "Material.h"
class Material;


//An abstract class representing a basic primitive in TinyRay
class Primitive
{
private:
	Material				*m_pMaterial;		//pointer to the material associated to the primitive
	Colour					m_reflectioncol;	//the current relfection colour.
	Colour					m_refractedcolour;	// the colour if this is refractive
protected:
	float meshScale = 0.1;
public:
	//enum for primitive types
	enum PRIMTYPE
	{
		PRIMTYPE_Plane = 0,	//plane
		PRIMTYPE_Sphere, //sphere
		PRIMTYPE_Triangle, //generic triangle
		PRIMTYPE_Box, //box
		PRIMTYPE_Bounding
	};
	void OverridePrimType(PRIMTYPE type) {
		m_primtype = type;
	}
	PRIMTYPE				m_primtype; //primitive type

	Primitive() { m_pMaterial = nullptr; }
	virtual					~Primitive() { ; }


	virtual RayHitResult	IntersectByRay(Ray& ray) = 0;  //An interface for computing intersection between a ray and this primitve

	inline void				SetMaterial(Material* pMat)
	{
		m_pMaterial = pMat;
	}

	inline Material*		GetMaterial()
	{
		return m_pMaterial;
	}
	virtual void SetRelfection(Colour col) {
		m_reflectioncol = col;
	}
	virtual void SetRefraction(Colour col) {
		m_refractedcolour = col;
	}
	//point for textures
	virtual Colour GetDiffuseColour(Vector3 point) {
		return m_pMaterial->GetDiffuseColour();
	}
	Colour GetRelfectionColour() {
		return m_reflectioncol;
	}
	Colour GetRefractedColour() {
		if (m_refractedcolour.Norm() > 0.1) {
			return m_refractedcolour;
		}
		return Colour(1, 1, 1);
	}
	virtual Colour GetNormalColour(Vector3 point) {
		return Colour(0, 0, 0);
	}
	virtual Vector3 GetUAxis() {
		return Vector3(0, 0, 0);
	}
	virtual Vector3 GetVAxis() {
		return Vector3(0, 0, 0);
	}
};
