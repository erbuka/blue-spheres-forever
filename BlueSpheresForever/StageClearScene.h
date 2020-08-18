#pragma once

#include <list>

#include "EventEmitter.h"
#include "Scene.h"
#include "Common.h";
#include "StageCodeHelper.h";

namespace bsf
{
	struct StageClearInfo
	{
		uint64_t Score;
		uint32_t CollectedRings;
		bool Perfect;
		uint32_t StageNumber;
	};

	class StageClearScene : public Scene
	{
	public:

		StageClearScene(const GameInfo& gameInfo, uint32_t collectedRings, bool perfect);

		void OnAttach() override;
		void OnRender(const Time& time) override;
		void OnDetach() override;
	private:

		std::list<Unsubscribe> m_Subscriptions;

		GameInfo m_GameInfo;
		
		uint32_t m_CurrentStage = 0, m_NextStage = 0;
		StageCodeHelper m_CurrentStageCode;

		InterpolatedValue<uint32_t> m_RingBonus, m_PerfectBonus;
		InterpolatedValue<uint64_t> m_Score;

		void RenderUI();
	};
}

