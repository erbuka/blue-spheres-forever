#include "BsfPch.h"

#ifdef BSF_ENABLE_DIAGNOSTIC

#include "Profiler.h"
#include "Log.h"

namespace bsf
{
	DiagnosticTool& DiagnosticTool::Get()
	{
		static DiagnosticTool instance;
		return instance;
	}


	void DiagnosticTool::Begin()
	{
		for (auto& item : m_Stats)
		{
			item.second.ExecutionTime = 0.0f;
			item.second.Calls = 0;
		}
	}

	void DiagnosticTool::End()
	{
		for (auto& item : m_Stats)
		{
			item.second.LastExecutionTimes.pop_back();
			item.second.LastExecutionTimes.push_front(0.0f);
		}

		for (auto& item : m_Stats)
		{
			auto& execTimes = item.second.LastExecutionTimes;
			execTimes.front() = item.second.ExecutionTime;
			item.second.MaxExecutionTime = std::max(item.second.MaxExecutionTime, item.second.ExecutionTime);
			item.second.MeanExecutionTime = std::accumulate(execTimes.begin(), execTimes.end(), 0.0f) / (float)(DiagnosticToolStats::Samples);
		}
	}

	DiagnosticGuard::DiagnosticGuard(const char* name) : m_Name(name)
	{
		m_t0 = Clock::now();
	}

	DiagnosticGuard::~DiagnosticGuard()
	{
		auto& stats = DiagnosticTool::Get().m_Stats[m_Name];
		auto t1 = Clock::now();
		stats.Calls++;
		stats.ExecutionTime += 
			std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(t1 - m_t0).count();
	}
}

#endif