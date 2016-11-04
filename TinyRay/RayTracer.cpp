/*---------------------------------------------------------------------
*
* Copyright © 2015  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>



#if defined(WIN32) || defined(_WINDOWS)
#include <Windows.h>
#include <gl/GL.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#endif

#include "RayTracer.h"
#include "Ray.h"
#include "Scene.h"
#include "Camera.h"
#include "perlin.h"
//assume the monitor is sRGB
#define Gamma 2.2
#define MAX_DEPTH 5
RayTracer::RayTracer()
{
	m_buffHeight = m_buffWidth = 0.0;
	m_renderCount = 0;
	SetTraceLevel(5);
	m_traceflag = (TraceFlags)(TRACE_AMBIENT | TRACE_DIFFUSE_AND_SPEC |
		TRACE_SHADOW | TRACE_REFLECTION | TRACE_REFRACTION);

}

RayTracer::RayTracer(int Width, int Height)
{
	m_buffWidth = Width;
	m_buffHeight = Height;
	m_renderCount = 0;
	SetTraceLevel(5);

	m_framebuffer = new Framebuffer(Width, Height);

	//default set default trace flag, i.e. no lighting, non-recursive
	m_traceflag = (TraceFlags)(TRACE_AMBIENT | TRACE_DIFFUSE_AND_SPEC);
}

RayTracer::~RayTracer()
{
	delete m_framebuffer;
}

void RayTracer::DoRayTrace(Scene* pScene)
{
	Camera* cam = pScene->GetSceneCamera();

	Vector3 camRightVector = cam->GetRightVector();
	Vector3 camUpVector = cam->GetUpVector();
	Vector3 camViewVector = cam->GetViewVector();
	Vector3 centre = cam->GetViewCentre();
	Vector3 camPosition = cam->GetPosition();

	double sceneWidth = pScene->GetSceneWidth();
	double sceneHeight = pScene->GetSceneHeight();

	double pixelDX = sceneWidth / m_buffWidth;
	double pixelDY = sceneHeight / m_buffHeight;

	int total = m_buffHeight*m_buffWidth;
	int done_count = 0;

	Vector3 start;

	start[0] = centre[0] - ((sceneWidth * camRightVector[0])
		+ (sceneHeight * camUpVector[0])) / 2.0;
	start[1] = centre[1] - ((sceneWidth * camRightVector[1])
		+ (sceneHeight * camUpVector[1])) / 2.0;
	start[2] = centre[2] - ((sceneWidth * camRightVector[2])
		+ (sceneHeight * camUpVector[2])) / 2.0;

	Colour scenebg = pScene->GetBackgroundColour();

	if (m_renderCount == 0)
	{
		fprintf(stdout, "Trace start.\n");

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		Colour colour;
		//TinyRay on multiprocessors using OpenMP!!!
#pragma omp parallel for schedule (dynamic, 1) private(colour)
		for (int i = 0; i < m_buffHeight; i += 1) {
			for (int j = 0; j < m_buffWidth; j += 1) {
				colour = Colour(0, 0, 0);
				float SSAAF = 2;
				//	for (float tx = -1; tx < SSAAF; tx++) for (float ty = -1; ty < SSAAF; ty++) {
						//calculate the metric size of a pixel in the view plane (e.g. framebuffer)
				Vector3 pixel;

				pixel[0] = start[0] + (i + 0.5) * camUpVector[0] * pixelDY
					+ (j + 0.5) * camRightVector[0] * pixelDX;

				pixel[1] = start[1] + (i + 0.5) * camUpVector[1] * pixelDY
					+ (j + 0.5) * camRightVector[1] * pixelDX;
				pixel[2] = start[2] + (i + 0.5) * camUpVector[2] * pixelDY
					+ (j + 0.5) * camRightVector[2] * pixelDX;

				/*
				* setup first generation view ray
				* In perspective projection, each view ray originates from the eye (camera) position
				* and pierces through a pixel in the view plane
				*/
				Ray viewray;
				viewray.SetRay(camPosition, (pixel - camPosition).Normalise());

				double u = (double)j / (double)m_buffWidth;
				double v = (double)i / (double)m_buffHeight;

				scenebg = pScene->GetBackgroundColour();
				//TODO:supersampling!
				//trace the scene using the view ray
				//default colour is the background colour, unless something is hit along the way
				colour = this->TraceScene(pScene, viewray, scenebg, m_traceLevel);
				currentdepth = 0;
				//	}
					//	colour = 4.0 /colour / ;
						/*colour[0] = colour[0] / 4;
						colour[1] = colour[1] / 4;
						colour[2] = colour[2] / 4;*/
						/*
						* Draw the pixel as a coloured rectangle
						*/
				m_framebuffer->WriteRGBToFramebuffer(colour, j, i);
			}
		}

		fprintf(stdout, "Done!!!\n");
		m_renderCount++;
	}
}
const double ERR = 1e-12;

Colour RayTracer::TraceScene(Scene* pScene, Ray& ray, Colour incolour, int tracelevel, bool shadowray)
{
	RayHitResult result;
	tracelevel--;
	Colour outcolour = incolour; //the output colour based on the ray-primitive intersection
	Colour Reflection;
	Colour Refraction;
	std::vector<Light*> *light_list = pScene->GetLightList();
	Vector3 cameraPosition = pScene->GetSceneCamera()->GetPosition();

	//Intersect the ray with the scene
	//TODO: Scene::IntersectByRay needs to be implemented first
	result = pScene->IntersectByRay(ray);
	Primitive* prim = (Primitive*)result.data;
	if (result.data) //the ray has hit something
	{
		Primitive* t = (Primitive*)result.data;

		//TODO:1. Non-recursive ray tracing:
		//	 When a ray intersect with an objects, determine the colour at the intersection point
		//	 using CalculateLighting

		//TODO: 2. The following conditions are for implementing recursive ray tracing
		if (m_traceflag & TRACE_REFLECTION)
		{
			//TODO: trace the reflection ray from the intersection point
			if (prim->GetMaterial()->GetSpecularColour().Norm() > 0.0f) {
				Ray reflectRay = Ray();

				//Vector3 rray = ray.GetRay() - result.normal* (2.0 * (ray.GetRay().DotProduct(result.normal)));
				Vector3 rray = ray.GetRay().Reflect(result.normal);
				Vector3 bias = rray.Normalise() * 0.05f;
				reflectRay.SetRay(result.point + bias, rray);
				Vector3 reft;
				if (tracelevel > 1) {
					//a_Acc += refl * rcol * prim->GetMaterial()->GetColor();
					reft = TraceScene(pScene, reflectRay, outcolour, tracelevel);
				}
				Reflection = reft *prim->GetMaterial()->GetDiffuseColour();
				outcolour = reft *prim->GetMaterial()->GetDiffuseColour();
			}
		}

		if (m_traceflag & TRACE_REFRACTION)
		{
			//TODO: trace the refraction ray from the intersection point
			if (prim->GetMaterial()->GetSpecularColour().Norm() > 0.0f) {
				Ray refractRay = Ray();
				Vector3 raray = ray.GetRay().Normalise().Refract(result.normal.Normalise(), (1.0 / 1.1));
				Vector3 bias = raray * 0.01;
				refractRay.SetRay(result.point + bias, raray);
				Vector3 reft;
				if (tracelevel > 1) {
					//a_Acc += refl * rcol * prim->GetMaterial()->GetColor();
					reft = TraceScene(pScene, refractRay, outcolour, tracelevel);
				}
				Colour absorbance = prim->GetMaterial()->GetDiffuseColour() * 0.15f * -1;
				Colour transparency = Colour(expf(absorbance[0]),
					expf(absorbance[1]),
					expf(absorbance[2]));

				Refraction = (reft*transparency) *prim->GetMaterial()->GetSpecularColour()*prim->GetMaterial()->GetDiffuseColour();// *0.8f;
			}
		}

		outcolour = CalculateLighting(light_list, &cameraPosition, &result) + Reflection + Refraction;
		if (m_traceflag & TRACE_SHADOW)
		{

			//TODO: trace the shadow ray from the intersection point	
			std::vector<Light*>::iterator lit_iter = light_list->begin();
			Light* clight = *lit_iter;
			Ray shadowray = Ray();
			Vector3 dir = (clight->GetLightPosition() - result.point).Normalise();
			Vector3 bias = dir * 0.0001;
			shadowray.SetRay(result.point + bias, dir);
			result = pScene->IntersectByRay(shadowray);
			if (result.data) {
				//we hit something
				if (((Primitive*)result.data)->GetMaterial()->CastShadow() == true) {
					outcolour = outcolour * 0.25;
				}
			}
		}
	}

	return outcolour;
}

Colour RayTracer::CalculateLighting(std::vector<Light*>* lights, Vector3* campos, RayHitResult* hitresult)
{
	Colour outcolour;
	Colour Linearcol;
	std::vector<Light*>::iterator lit_iter = lights->begin();

	Primitive* prim = (Primitive*)hitresult->data;
	Material* mat = prim->GetMaterial();

	outcolour = mat->GetAmbientColour();
	Colour CurrentDiffuse;
	
	//Generate the grid pattern on the plane
	/*
	else {*/
		if (m_traceflag & TRACE_DIFFUSE_AND_SPEC) {
			CurrentDiffuse = prim->GetDiffuseColour(hitresult->point);
			if (mat->HasDiffuseTexture() == true) {
				//calculate Uvs
				/*m_UAxis = vector3(m_Plane.N.y, m_Plane.N.z, -m_Plane.N.x);
				m_VAxis = m_UAxis.Cross(m_Plane.N);*/
				CurrentDiffuse = prim->GetDiffuseColour(hitresult->point);
			}
		}
//	}

	////Go through all lights in the scene
	////Note the default scene only has one light source
	if (m_traceflag & TRACE_DIFFUSE_AND_SPEC)
	{
		//ip = l dot n *c *il
		//TODO: Calculate and apply the lighting of the intersected object using the illumination model covered in the lecture
		//i.e. diffuse using Lambertian model, for specular, you can use either Phong or Blinn-Phong model
		Light* clight = *lit_iter;
		Vector3 diff = clight->GetLightPosition() - hitresult->point;
		float distance = sqrtf(diff.DotProduct(diff));

		Vector3 lightdir = (clight->GetLightPosition() - hitresult->point).Normalise();
		float diffuseamt = max(lightdir.DotProduct(hitresult->normal.Normalise()), 0.0);
		diffuseamt = min(max(diffuseamt, 0), 1);
		//bhlim-phong
		Vector3 H = (lightdir + *campos).Normalise();//half vector
		float ndotH = max(hitresult->normal.Normalise().DotProduct(H), 0.0);
		float specular = pow(ndotH, mat->GetSpecPower());
		specular = min(max(specular, 0), 1);
		//not used?
		float cosTheta = hitresult->normal.DotProduct(lightdir);//normal and light
		float cosAlpha = campos->DotProduct(hitresult->normal);
		//std::clamp(0, 1, 1); not yet in C++17!
		Linearcol = mat->GetAmbientColour()
			+ CurrentDiffuse * clight->GetLightColour()  * diffuseamt /** (cosTheta / (distance*distance))*/
			+ mat->GetSpecularColour() * clight->GetLightColour()  *  specular /** (pow(cosAlpha, 5) / (distance*distance))*/;
		outcolour = Linearcol;
		//apply gamma correction
		outcolour[0] = pow(Linearcol[0], 1.0 / Gamma);
		outcolour[1] = pow(Linearcol[1], 1.0 / Gamma);
		outcolour[2] = pow(Linearcol[2], 1.0 / Gamma);
	}
	else {
		outcolour = CurrentDiffuse;
	}

	return outcolour;
}

