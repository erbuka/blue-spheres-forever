#include "BsfPch.h"

#include "Subscription.h"

namespace bsf
{
	void Subscription::Unsubscribe()
	{
		if (m_Valid)
		{
			m_Valid = false;
			m_Unsubscribe();
		}
	}
}