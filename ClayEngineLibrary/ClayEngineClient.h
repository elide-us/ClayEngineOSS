#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* ClientCore header provides context objects for client windows              */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "Strings.h"
#include "Services.h"
//#include "TimingSystem.h"

namespace ClayEngine
{
    /// <summary>
    /// ClayEngineClient is the primary client context object for a client affinity group
    /// </summary>
    class ClayEngineClient
        : public ThreadExtension
    {
        HINSTANCE m_instance_handle = {};
		UINT m_show_flags = SW_SHOWDEFAULT;

        AffinityData m_affinity_data;

    public:
        ClayEngineClient(HINSTANCE hInstance, Affinity pRoot, Unicode className, Unicode windowName);
        ~ClayEngineClient();

        void SetContextAffinity(Affinity affinity);
        const AffinityData& GetAffinityData() const;

    };
    using ClayEngineClientPtr = std::unique_ptr<ClayEngineClient>;
    using ClayEngineClientRaw = ClayEngineClient*;

    using ClientMap = std::map<String, ClayEngineClientPtr>;
}
