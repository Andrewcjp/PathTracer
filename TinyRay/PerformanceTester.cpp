#include "PerformanceTester.h"



PerformanceTester::PerformanceTester(RayTracer* tracer, Scene* pscene)
{
	m_pScene = pscene;
	m_tracer = tracer;
}


PerformanceTester::~PerformanceTester()
{
}

void PerformanceTester::RunTests()
{
	printf("Starting Tests\n");
	SaveTest("Ambient Only", Test(RayTracer::TRACE_AMBIENT));
	SaveTest("Diffuse lighting", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC)));
	SaveTest("Diffuse lighting with shadows", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW)));
	SaveTest("Refraction", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_REFRACTION)));
	SaveTest("Relfection", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_REFLECTION)));
	SaveTest("Diffuse Lighting, Shadows, Reflect and Refract", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_REFRACTION | RayTracer::TRACE_REFLECTION | RayTracer::TRACE_SHADOW)));
	SaveTest("Diffuse Shadow With SuperSampling", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW), true, false));
	SaveTest("Diffuse Shadow With soft shadows", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW), false, true));
	SaveTest("Diffuse Shadow With Normal mapping", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW), false, false, true));
	SaveTest("Diffuse Lighting, Shadows, Reflect and Refract with soft Shadows, Super Sampling and normal mapping", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_REFRACTION | RayTracer::TRACE_REFLECTION | RayTracer::TRACE_SHADOW), true, true, true));
	WriteOutput();
	printf("All Tests complete!\n");
	m_tracer->m_traceflag = (RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC);
}

void PerformanceTester::RunAATests()
{
	printf("Starting AA Tests\n");
	m_tracer->SetAAFactor(2);
	SaveTest("4X AA", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW), true));
	m_tracer->SetAAFactor(3);
	SaveTest("9X AA", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW), true));
	m_tracer->SetAAFactor(4);
	SaveTest("16X AA", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW), true));
	m_tracer->SetAAFactor(6);
	SaveTest("36X AA", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW), true));
	m_tracer->SetAAFactor(8);
	SaveTest("64X AA", Test((RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC | RayTracer::TRACE_SHADOW), true));	
	WriteOutput();
	printf("All Tests complete!\n");
	m_tracer->m_traceflag = (RayTracer::TraceFlags)(RayTracer::TRACE_AMBIENT | RayTracer::TRACE_DIFFUSE_AND_SPEC);
	m_tracer->SetAAFactor(3);//reset the aa value to default
}

double PerformanceTester::Test(RayTracer::TraceFlags flags, bool AA, bool softshadows, bool nrmmap)
{
	clock_t tstart = clock();
	//reset the render count
	m_tracer->SetExtra(AA, softshadows, nrmmap);
	m_tracer->ResetRenderCount();
	m_tracer->m_traceflag = flags;
	m_tracer->DoRayTrace(m_pScene);
	//this renders the scene but does not output to the screen removing the OGL overhead.
	return ((double)(clock() - tstart) * 1000.0 / CLOCKS_PER_SEC);
}

void PerformanceTester::SaveTest(std::string testname, double time)
{
	std::ostringstream strs;
	strs << testname << " : " << time << std::endl;
	std::string str = strs.str();
	data += str;
}

void PerformanceTester::WriteOutput()
{
	std::ofstream stream;
	stream.open(path, std::ios::trunc);//replace any existing data
//	stream << testname << " : " << time << std::endl;
	stream << data << std::endl;
	stream.close();
}

