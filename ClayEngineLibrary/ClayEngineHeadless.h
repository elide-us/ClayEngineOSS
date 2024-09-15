#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/*               */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "Strings.h"
#include "Services.h"

namespace ClayEngine
{
	class ClayEngineHeadless
		: public ThreadExtension
	{
		AffinityData m_affinity_data;

	public:
		ClayEngineHeadless(Affinity pRoot);
		~ClayEngineHeadless();

		void SetContextAffinity(Affinity affinity);
		const AffinityData& GetAffinityData() const;
	};
	using ClayEngineHeadlessPtr = std::unique_ptr<ClayEngineHeadless>;

	using HeadlessMap = std::map<String, ClayEngineHeadlessPtr>;
}
