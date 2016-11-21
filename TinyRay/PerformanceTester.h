#pragma once
#include "RayTracer.h"
#include <ios>
#include <stdio.h>
#include "Scene.h"
#include <time.h>
#include <string>
#include <fstream>
#include <sstream>
class PerformanceTester
{
public:

	PerformanceTester(RayTracer * tracer, Scene * pscene);
	~PerformanceTester();
	void RunTests();
	void RunAATests();
private:
	char* path = "../Test.txt";
	double Test(RayTracer::TraceFlags flags, bool AA = false, bool softshadows = false, bool nrmmap = false);
	RayTracer* m_tracer;
	Scene* m_pScene;
	void SaveTest(std::string testname,double time);
	std::string data;
	void WriteOutput();

};

