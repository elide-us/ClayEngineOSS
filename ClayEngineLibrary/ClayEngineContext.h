#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/*            */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "Strings.h"
#include "Services.h"

namespace ClayEngine
{
    struct ClayEngineContextThread
    {
        THREAD Thread;
        PROMISE Promise = {};
    };

    /// <summary>
    /// ClayEngineClient is the primary client context object for a client affinity group
    /// </summary>
    class ClayEngineClient
    {
        HINSTANCE m_instance_handle = {};
		UINT m_show_flags = SW_SHOWDEFAULT;

        AffinityData m_affinity_data;
        Document m_document;

        ClayEngineContextThread m_thread = {};

    public:
        ClayEngineClient(Document document, HINSTANCE hInstance, Affinity pRoot, Unicode className, Unicode windowName);
        ~ClayEngineClient();

        void SetContextAffinity(Affinity affinity);
        const AffinityData& GetAffinityData() const;

    };
    using ClayEngineClientPtr = std::unique_ptr<ClayEngineClient>;
    using ClientMap = std::map<String, ClayEngineClientPtr>;

    class ClayEngineServer
    {
        HINSTANCE m_instance_handle = {};
		UINT m_show_flags = SW_SHOWDEFAULT;

        AffinityData m_affinity_data;
		Document m_document;

        ClayEngineContextThread m_thread;
    public:
		ClayEngineServer(Document document, HINSTANCE hInstance, Affinity pRoot, Unicode className, Unicode windowName);
		~ClayEngineServer();

		void SetContextAffinity(Affinity affinity);
		const AffinityData& GetAffinityData() const;
    };
	using ClayEngineServerPtr = std::unique_ptr<ClayEngineServer>;
	using ServerMap = std::map<String, ClayEngineServerPtr>;

	class ClayEngineHeadless
	{
		AffinityData m_affinity_data;
        Document m_document;

		ClayEngineContextThread m_thread;

	public:
		ClayEngineHeadless(Document document, Affinity affinity, Unicode className);
		~ClayEngineHeadless();

		void SetContextAffinity(Affinity affinity);
		const AffinityData& GetAffinityData() const;
	};
	using ClayEngineHeadlessPtr = std::unique_ptr<ClayEngineHeadless>;
	using HeadlessMap = std::map<String, ClayEngineHeadlessPtr>;
}
