#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Services header provides static threaded service registry                  */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include <functional>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <map>
#include <sstream>
#include <ios>
#include <exception>
#include <memory>
#include <utility>
#include <thread>
#include <future>

#include "Strings.h"

namespace ClayEngine
{
	struct AffinityData
	{
		std::thread::id this_thread;
		std::thread::id root_thread;
	};

	using Type = std::type_index;
	using Object = void*;

	using Future = std::future<void>;
	using Promise = std::promise<void>;

	using Thread = std::thread;
	using Affinity = std::thread::id;

	using ServicesMap = std::map<Type, Object>;
	using AffinityMap = std::map<Affinity, ServicesMap>;

	/// <summary>
	/// Stores and provides raw pointers to services created by this Service, but owned by other objects
	/// </summary>
	class Services
	{
		inline static AffinityMap m_services = {};

	public:
		/// <summary>
		/// This factory method will create a unique_ptr object and return it to the caller.
		/// This class will retain a copy of the pointer, and others can request a raw 
		/// pointer to the object, but the objects themselves are expected to be RAII and not
		/// directly owned here, this class just provides pointers. The caller is expected to
		/// remove the entry when the service is deconstructed.
		/// </summary>
		template<typename T, typename... Args>
		static std::unique_ptr<T> MakeService(AffinityData affinityData, Args&&... args)
		{
			std::wstringstream wss;

			auto i = m_services.find(affinityData.this_thread);
			if (i == m_services.end())
			{
				ServicesMap sm = {};

				auto p = std::make_unique<T>(affinityData, std::forward<Args>(args)...);
				auto o = reinterpret_cast<Object>(p.get());
				auto t = Type(typeid(T));

				wss << __func__ << L"() SUCCESS: " << t.name() << L" " << o;
				WriteLine(wss.str());

				sm.emplace(t, o);
				m_services.emplace(affinityData.this_thread, sm);
				return std::move(p);
			}
			else
			{
				auto s = &i->second;

				auto it = s->find(Type(typeid(T)));
				if (it == s->end())
				{
					auto p = std::make_unique<T>(affinityData, std::forward<Args>(args)...);
					auto o = reinterpret_cast<Object>(p.get());
					auto t = Type(typeid(T));

					wss << __func__ << L"() SUCCESS: " << t.name() << L" " << o;
					WriteLine(wss.str());

					s->emplace(t, o);
					return std::move(p);
				}
				else
				{
					// Unique Service constraint violation
					wss << __func__ << L"() ERROR: Service not created due to unique key constraint violation.";
					WriteLine(wss.str());

					return nullptr;
				}
			}
		}

		template<typename T, typename... Args>
		static std::unique_ptr<T> MakeDxService(AffinityData affinityData, Args&&... args)
		{
			std::wstringstream wss;

			auto i = m_services.find(affinityData.this_thread);
			if (i != m_services.end())
			{
				auto s = &i->second;

				auto it = s->find(Type(typeid(T)));
				if (it == s->end())
				{
					auto p = std::make_unique<T>(std::forward<Args>(args)...);
					auto o = reinterpret_cast<Object>(p.get());
					auto t = Type(typeid(T));

					wss << __func__ << L"() SUCCESS: " << t.name() << L" " << o;
					WriteLine(wss.str());

					s->emplace(t, o);
					return std::move(p);
				}
				else
				{
					// Unique Service constraint violation
					wss << __func__ << L"() ERROR: Service not created due to unique key constraint violation.";
					WriteLine(wss.str());

					return nullptr;
				}
			}
			else
			{
				wss << __func__ << L"() ERROR: Service not created due to missing service map.";
				WriteLine(wss.str());
				return nullptr;
			}
		}

		/// <summary>
		/// Returns a raw pointer to a unique_ptr object owned by some other class. You are 
		/// expected to check if the poitner is valid before you use it.
		/// </summary>
		template<typename T>
		static T* GetService(Affinity threadId, bool silent = true)
		{
			std::wstringstream wss;

			auto i = m_services.find(threadId);
			if (i != m_services.end())
			{
				auto _services = &i->second;

				auto it = _services->find(Type(typeid(T)));
				if (it != _services->end())
				{
					auto p = reinterpret_cast<T*>(it->second);
					auto t = Type(typeid(T));

					//wss << __func__ << L"() INFO: " << t.name() << L" 0x" << std::hex << it->second;
					//WriteLine(wss.str());

					return p;
				}

#ifdef _DEBUG
				wss << __func__ << L"() ERROR: Service not found.";
				WriteLine(wss.str());
#endif

				return nullptr;
			}

			return nullptr;
		}

		/// <summary>
		/// Used to remove a service from the collection that has been deleted. At this time
		/// there's no checking on a pointer from this perspective, so we have to remove the 
		/// service pointer here if we are, for example, resetting some service component
		/// </summary>
		template<typename T>
		static void RemoveService(Affinity threadId)
		{
			std::wstringstream wss;

			auto i = m_services.find(threadId);
			if (i != m_services.end())
			{
				auto _services = &i->second;

				auto it = _services->find(Type(typeid(T)));
				if (it != _services->end())
				{
					auto t = Type(typeid(T));

					wss << __func__ << L"() SUCCESS: " << t.name();
					WriteLine(wss.str());

					_services->erase(it);
					//TODO: Check if AffinityMap size == 0 and erase that if necessary.
					return;
				}
				else
				{
					wss << __func__ << L"() ERROR: Service not found for removal.";
					WriteLine(wss.str());
				}
			}
			else
			{
				wss << __func__ << L"() ERROR: ServiceMap not found for thread.";
				WriteLine(wss.str());
			}
		}
	};

	///<summary>
	/// This extension provides the members for a threaded class, but you 
	/// still need to provide the functor and create the thread object
	///</summary>
	class ThreadExtension
	{
	protected:
		Promise m_promise = {};
		Thread m_thread;
	public:
		~ThreadExtension()
		{
			if (m_thread.joinable())
			{
				m_promise.set_value();
				m_thread.join();
			}
		}
	};

}
