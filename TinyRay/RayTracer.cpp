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
		clock_t tstart = clock();
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		Colour colour;
		int current = 0;
		Primitive* lastprim = nullptr;
		Primitive* currentprim;
		//TinyRay on multiprocessors using OpenMP!!!
#pragma omp parallel for schedule (dynamic, 1) private(colour)		
		for (int i = 0; i < m_buffHeight; i += 1) {
			for (int j = 0; j < m_buffWidth; j += 1) {
				current = i + j;
				colour = Colour(0, 0, 0);
				Vector3 pixel;

				pixel[0] = start[0] + (i + 0.5) * camUpVector[0] * pixelDY
					+ (j + 0.5) * camRightVector[0] * pixelDX;
				pixel[1] = start[1] + (i + 0.5) * camUpVector[1] * pixelDY
					+ (j + 0.5) * camRightVector[1] * pixelDX;
				pixel[2] = start[2] + (i + 0.5) * camUpVector[2] * pixelDY
					+ (j + 0.5) * camRightVector[2] * pixelDX;
				Ray primray;
				primray.SetRay(camPosition, (pixel - camPosition).Normalise());
				//find our primitve
				currentprim = (Primitive*)pScene->IntersectByRay(primray).data;
				//calculate the metric size of a pixel in the view plane (e.g. framebuffer)
				if (SuperSample == true /*&& currentprim != lastprim*/) {
					float amt = 2;
					float diffrenceamt = 0.3;
					lastprim = currentprim;
					for (int x = 0; x < amt; x++) {
						for (int y = 0; y < amt; y++) {
							float Xadd = -diffrenceamt*x;
							float Yadd = -diffrenceamt*y;
							if (x > (amt / 2) - 1) {
								//second pass move over 1
								Xadd = diffrenceamt*(x - (amt / 2) - 1);
							}
							if (y > (amt / 2) - 1) {
								//second pass move over 1
								Yadd = diffrenceamt*(y - (amt / 2) - 1);
							}
							pixel[0] = start[0] + (i + Xadd + 0.5) * camUpVector[0] * pixelDY
								+ (j + Yadd + 0.5) * camRightVector[0] * pixelDX;
							pixel[1] = start[1] + (i + Xadd + 0.5) * camUpVector[1] * pixelDY
								+ (j + Yadd + 0.5) * camRightVector[1] * pixelDX;
							pixel[2] = start[2] + (i + Xadd + 0.5) * camUpVector[2] * pixelDY
								+ (j + Yadd + 0.5) * camRightVector[2] * pixelDX;
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
							colour = colour + this->TraceScene(pScene, viewray, scenebg, m_traceLevel);
						}
					}
					float dvisor = amt*amt;
					colour[0] = colour[0] / dvisor;
					colour[1] = colour[1] / dvisor;
					colour[2] = colour[2] / dvisor;
				}
				else
				{
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
				}
				/*
				* Draw the pixel as a coloured rectangle
				*/
				m_framebuffer->WriteRGBToFramebuffer(colour, j, i);

			}
		}

		printf("Time taken: %.00fms\n", (double)(clock() - tstart) * 1000 / CLOCKS_PER_SEC);
		fprintf(stdout, "Done!!!\n");
		m_renderCount++;
	}
}
const double ERR = 1e-12;
float Distance(const Vector3& point1, const Vector3& point2)
{
	float distance = sqrt((point1[0] - point2[0]) * (point1[0] - point2[0]) +
		(point1[1] - point2[1]) * (point1[1] - point2[1]) +
		(point1[2] - point2[2]) * (point1[2] - point2[2]));
	return distance;
}
Colour RayTracer::TraceScene(Scene* pScene, Ray& ray, Colour incolour, int tracelevel, bool shadowray)
{
	RayHitResult result;
	tracelevel--;
	Colour outcolour = incolour; //the output colour based on the ray-primitive intersection
	Colour Reflection;// = Colour(1.0f, 1.0f, 1.0f);
	Colour Refraction;
	std::vector<Light*> *light_list = pScene->GetLightList();
	Vector3 cameraPosition = pScene->GetSceneCamera()->GetPosition();

	//Intersect the ray with the scene
	//TODO: Scene::IntersectByRay needs to be implemented first
	result = pScene->IntersectByRay(ray);
	Primitive* prim = (Primitive*)result.data;
	Material* mat = prim->GetMaterial();
	if (result.data) //the ray has hit something
	{
		//TODO:1. Non-recursive ray tracing:
		//	 When a ray intersect with an objects, determine the colour at the intersection point
		//	 using CalculateLighting
		outcolour = CalculateLighting(light_list, &cameraPosition, &result);
		//TODO: 2. The following conditions are for implementing recursive ray tracing
		if (m_traceflag & TRACE_REFLECTION)
		{
			//TODO: trace the reflection ray from the intersection point
			if (mat->GetReflectivity() > 0.0) {
				if (tracelevel > 1) {
					Ray reflectRay = Ray();
					Vector3 rray = ray.GetRay().Reflect(result.normal);
					Vector3 bias = rray.Normalise() * 0.05;
					reflectRay.SetRay(result.point + bias, rray);
					Vector3 reft;
					//a_Acc += refl * rcol * prim->GetMaterial()->GetColor();
					reft = TraceScene(pScene, reflectRay, outcolour, tracelevel);
					Reflection = reft *prim->GetMaterial()->GetDiffuseColour();
					prim->SetRelfection(Reflection);
					outcolour = CalculateLighting(light_list, &cameraPosition, &result);
				}
			}
		}
		//todo: refacted light colours our bound light
		if (m_traceflag & TRACE_REFRACTION)
		{
			//TODO: trace the refraction ray from the intersection point
			if (mat->GetRefractivity() > 0.0f) {
				if (tracelevel > 1) {
					Ray refractRay = Ray();
					Vector3 raray = ray.GetRay().Normalise().Refract(result.normal.Normalise(), (1.0 / mat->GetRefractivity()));
					Vector3 bias = raray * 0.01;
					refractRay.SetRay(result.point + bias, raray);
					Vector3 reft;
					if (tracelevel > 1) {
						//a_Acc += refl * rcol * prim->GetMaterial()->GetColor();
						reft = TraceScene(pScene, refractRay, outcolour, tracelevel);
					}
					//apply beers law so that the material changes the light intesity				
					Colour absorbance = prim->GetMaterial()->GetDiffuseColour() * 0.15f * pScene->IntersectByRay(refractRay).t;//distance into material
					Colour transparency = Colour(expf(absorbance[0]),
						expf(absorbance[1]),
						expf(absorbance[2]));

					Refraction = (reft)*prim->GetMaterial()->GetSpecularColour() + (prim->GetMaterial()->GetDiffuseColour() *0.1f);// *0.8f;
					outcolour = (reft)*prim->GetMaterial()->GetSpecularColour() + (prim->GetMaterial()->GetDiffuseColour() *0.1f);// *0.8f;
					//prim->SetRefraction(Refraction);
					//outcolour = CalculateLighting(light_list, &cameraPosition, &result);
					if (m_traceflag & TRACE_REFLECTION) {
						outcolour = outcolour + (Reflection *0.5f);
					}
				}
			}
		}
		//todo: clour bleed?
		if (m_traceflag & TRACE_SHADOW)
		{
			//TODO: trace the shadow ray from the intersection point	
			std::vector<Light*>::iterator lit_iter = light_list->begin();
			float shadowAccum = 0;
			int count = 4;
			int samplecount = count * count;
			double tlight = 0;//calculate where our light is along our ray.
			double bias = 0.0001;
			for (int i = 0; i < light_list->size(); i++) {
				Ray shadowray = Ray();
				RayHitResult shadowresult;
				Light* clight = *(lit_iter + i);
				//we will check the pixels around our centre line
				//then avg the result
				//x need to bit tangent and y is tangent
				if (Softshadows == true) {
					shadowAccum = 0;
					//adjust the light position so that we start offset from centre
					Vector3 adjustlightpos(clight->GetLightPosition()[0] - count / 2.0, clight->GetLightPosition()[1], clight->GetLightPosition()[2] - count / 2.0);
					for (int x = 0; x < count; x++) {
						for (int y = 0; y < count; y++) {
							//get the offset in the T axis
							double rand = (double)(std::rand() % (100 - 0 + 1)) / 100.0;
							Vector3 lightpos = Vector3(adjustlightpos[0] + x + rand, adjustlightpos[1], adjustlightpos[2] + y + rand);
							shadowray.SetRay(result.point + lightpos *bias, (lightpos - result.point).Normalise());
							shadowresult = pScene->IntersectByRay(shadowray);
							tlight = Distance(result.point, lightpos);
							if (shadowresult.data && shadowresult.t < tlight) {
								//we hit something
								if (((Primitive*)shadowresult.data)->GetMaterial()->CastShadow() == true) {
									shadowAccum += 1;
								}

							}
						}
					}
					outcolour = outcolour * (1.0 - (shadowAccum / samplecount));
				}
				else
				{
					Vector3 dir = (clight->GetLightPosition() - result.point).Normalise();
					shadowray.SetRay(result.point + dir *bias, dir);
					shadowresult = pScene->IntersectByRay(shadowray);
					//t is intersection distance along ray
					//so:
					tlight = Distance(result.point, clight->GetLightPosition());
					if (shadowresult.data && shadowresult.t < tlight) {
						//we hit something
						//check that the object we hit is not past our light
						if (((Primitive*)shadowresult.data)->GetMaterial()->CastShadow() == true) {
							outcolour = outcolour * 0.25f;
						}
					}
				}
			}
		}
	}
	return outcolour;
}

Vector3 TimesMat(Vector3 target, Vector3 bitangent, Vector3 tangent, Vector3 normal) {
	target[0] = tangent.Normalise().DotProduct(target);
	target[1] = bitangent.Normalise().DotProduct(target);
	target[2] = normal.Normalise().DotProduct(target);
	return target;
}

//todo: sphereical Texture warpping
//todo: box texture wrapping.
//todo: ? handle uv mapping
Colour RayTracer::CalculateLighting(std::vector<Light*>* lights, Vector3* campos, RayHitResult* hitresult)
{
	Colour outcolour;
	Colour Linearcolsum;
	std::vector<Light*>::iterator lit_iter = lights->begin();

	Primitive* prim = (Primitive*)hitresult->data;
	Material* mat = prim->GetMaterial();
	Colour CurrentDiffuse = mat->GetAmbientColour();
	if (m_traceflag & TRACE_DIFFUSE_AND_SPEC) {
		CurrentDiffuse = prim->GetDiffuseColour(hitresult->point);
		if ((m_traceflag & TRACE_REFLECTION) && mat->GetReflectivity() > 0.0) {
			CurrentDiffuse = prim->GetRelfectionColour();
		}
	}
	bool blinn = false;
	////Go through all lights in the scene
	////Note the default scene only has one light source
	if (m_traceflag & TRACE_DIFFUSE_AND_SPEC)
	{
		for (int i = 0; i < lights->size(); i++) {
			//ip = l dot n *c *il
			//TODO: Calculate and apply the lighting of the intersected object using the illumination model covered in the lecture
			//i.e. diffuse using Lambertian model, for specular, you can use either Phong or Blinn-Phong model
			Light* clight = *(lit_iter + i);
			Vector3 normal;
			if (normalmapping && (mat->HasNormalTexture() == true)) {
				//preturb the normal to the simulted normal
				normal = prim->GetNormalColour(hitresult->point).Normalise();
			}
			else
			{
				normal = hitresult->normal.Normalise();
			}
			Vector3 eyepos = campos->Normalise();
			Vector3 lightpos = clight->GetLightPosition();
			Vector3 pos = hitresult->point;
			//convert positons to TBN
			if (normalmapping && (mat->HasNormalTexture() == true)) {
				Vector3 tangent = normal.CrossProduct(prim->GetUAxis().Normalise());
				Vector3 bitangent = tangent.CrossProduct(normal).Normalise();

				lightpos = TimesMat(lightpos, bitangent, tangent, normal);
				eyepos = TimesMat(eyepos, bitangent, tangent, normal);
				//		pos = TimesMat(pos, bitangent, tangent, normal);
				//		CurrentDiffuse = prim->GetDiffuseColour(pos);
			}
			Vector3 lightdir = (lightpos - pos).Normalise();
			double diffuseamt = max(lightdir.DotProduct(normal), 0.0);
			double specular = 0;

			//bhlim-phong
			if (blinn) {
				Vector3 H = (lightdir + eyepos).Normalise();//half vector
				double ndotH = max(normal.DotProduct(H), 0.0);
				specular = pow(ndotH, mat->GetSpecPower());
			}
			else
			{
				Vector3 invlightdir = lightdir*-1.0;
				Vector3 R = invlightdir.Reflect(normal).Normalise();
				//R = R * -1;
				specular = pow(max(eyepos.DotProduct(R), 0.0), mat->GetSpecPower());
			}
			Colour out = mat->GetAmbientColour()
				+ (CurrentDiffuse * clight->GetLightColour()*prim->GetRefractedColour()  * diffuseamt)
				+ (mat->GetSpecularColour() * clight->GetLightColour()*prim->GetRefractedColour()   *  specular);
			Linearcolsum = Linearcolsum + out;
		}
		//sum all the colours from lights then gamma correct
		//apply gamma correction
		outcolour[0] = pow(Linearcolsum[0], 1.0 / Gamma);
		outcolour[1] = pow(Linearcolsum[1], 1.0 / Gamma);
		outcolour[2] = pow(Linearcolsum[2], 1.0 / Gamma);
	}
	else
	{
		outcolour = CurrentDiffuse;
	}

	return outcolour;
}

