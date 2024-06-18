#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Static Services Class Library (C) 2022 Epoch Meridian, LLC.     */
/*                                                                            */
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

#include "Strings.h"

namespace ClayEngine
{
	using Function = ::std::function<void()>;
	using Functions = ::std::vector<Function>;

	/// <summary>
	/// Stores and provides raw pointers to services created by this Service, but owned by other objects
	/// </summary>
	class Services
	{
		using Type = ::std::type_index;
		using Object = void*;
		using ServicesMap = ::std::map<Type, Object>;
		using Affinity = ::std::thread::id;
		using ThreadMap = ::std::map<Affinity, ServicesMap>;

		//inline static ServicesMap m_services = {};
		inline static ThreadMap m_services = {};

	public:
		/// <summary>
		/// This factory method will create a unique_ptr object and return it to the caller.
		/// This class will retain a copy of the pointer, and others can request a raw 
		/// poitner to the object, but the objects themselves are expected to be RAII and not
		/// directly owned here, this class just provides pointers. The caller is expected to
		/// validate if the pointer is valid before using it.
		/// </summary>
		template<typename T, typename... Args>
		static ::std::unique_ptr<T> MakeService(Affinity threadId, Args&&... args)
		{
			::std::wstringstream wss;

			auto i = m_services.find(threadId);
			if (i == m_services.end())
			{
				// Make a new ServiceMap for this thread affinity
				ServicesMap m = {};
				m_services.emplace(threadId, m);
				auto s = &m_services.begin()->second;

				auto p = ::std::make_unique<T>(::std::forward<Args>(args)...);
				auto o = reinterpret_cast<Object>(p.get());
				auto t = Type(typeid(T));

				s->emplace(t, o);
				return ::std::move(p);
			}
			else
			{
				// We have an existing ServiceMap for this Owner, make as normal
				auto s = &i->second;

				auto it = s->find(Type(typeid(T)));
				if (it == s->end())
				{
					auto p = std::make_unique<T>(std::forward<Args>(args)...);
					auto o = reinterpret_cast<Object>(p.get());
					auto t = Type(typeid(T));

					wss << __func__ << L"() SUCCESS: " << t.name() << L" 0x" << std::setw(12) << std::hex << o;
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

				if (!silent)
				{
					wss << __func__ << L"() ERROR: Service not found.";
					WriteLine(wss.str());
				}

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
}
