#pragma once

#ifdef BSF_ENABLE_DIAGNOSTIC

#include <string>
#include <map>
#include <list>
#include <chrono>
#include <imgui.h>
#include <unordered_map>



#define BSF_DIAGNOSTIC_BEGIN() DiagnosticTool::Get().Begin()
#define BSF_DIAGNOSTIC_FUNC() DiagnosticGuard bsf_dguard##__LINE__ (__FUNCTION__)
#define BSF_DIAGNOSTIC_SCOPE(name) DiagnosticGuard bsf_dguard##__LINE__ (name)
#define BSF_DIAGNOSTIC_END() DiagnosticTool::Get().End()

namespace bsf
{


	struct DiagnosticToolStats
	{
		static constexpr size_t Samples = 10;
		const char* StackItemName = nullptr;
		uint32_t Calls = 0;
		float ExecutionTime = 0.0f;
		float MeanExecutionTime = 0.0f;
		float MaxExecutionTime = 0.0f;
		std::list<float> LastExecutionTimes = std::list<float>(Samples);
	};
	
	struct DiagnosticGuard
	{
	public:
		DiagnosticGuard(const char* name);
		~DiagnosticGuard();
	private:
		using Clock = std::chrono::system_clock;
		Clock::time_point m_t0;
		const char* m_Name;
	};

	class DiagnosticTool
	{
	public:
		static DiagnosticTool& Get();
		
		const std::unordered_map<const char*, DiagnosticToolStats>& GetStats() const { return m_Stats; }

		void Begin();
		void End();


	private:
		friend class DiagnosticGuard;
		DiagnosticTool() = default;
		std::unordered_map<const char*, DiagnosticToolStats> m_Stats;
	};





}

#else
#define BSF_DIAGNOSTIC_BEGIN()
#define BSF_DIAGNOSTIC_FUNC()
#define BSF_DIAGNOSTIC_SCOPE(name)
#define BSF_DIAGNOSTIC_END()
#endif