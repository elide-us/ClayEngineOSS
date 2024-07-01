#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* ClientCore header provides context objects for client windows              */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include <utility>
#include <string>

#include "Strings.h"
#include "Services.h"

#include "WindowSystem.h"
//#include "TimingSystem.h"
//#include "InputSystem.h"

namespace ClayEngine
{
    /// <summary>
    /// ClayEngineClient is the primary client context object for a client affinity group
    /// </summary>
    class ClayEngineClient
        : public ThreadExtension
    {
        HINSTANCE m_instance_handle = NULL;
		UINT m_show_flags = SW_SHOWDEFAULT;
        LPWSTR m_cmd_line = NULL;

        Affinity m_affinity;

    public:
        ClayEngineClient(HINSTANCE hInstance, UINT nCmdShow, LPWSTR nCmdLine, Unicode className, Unicode windowName);
        ~ClayEngineClient();

        void SetAffinity(Affinity affinity)
		{
			m_affinity = affinity;
		}
    };
    using ClayEngineClientPtr = std::unique_ptr<ClayEngineClient>;
    using ClayEngineClientRaw = ClayEngineClient*;
}
