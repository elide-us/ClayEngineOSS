#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include <locale>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <vector>

namespace ClayEngine
{
	using Locale = std::locale;

	using Unicode = std::wstring;
	using UnicodeStrings = std::vector<Unicode>;

	using String = std::string;
	using Strings = std::vector<String>;

	/// <summary>
	/// Convert an ANSI string to a Unicode string
	/// </summary>
	inline Unicode ToUnicode(String string)
	{
		std::wstringstream wss;
		wss << string.c_str();
		return wss.str().c_str();
	}

	/// <summary>
	/// Write a Unicode string to the console
	/// </summary>
	inline void WriteLine(Unicode message)
	{
		std::wstringstream wss;
		wss << L"[" << std::setfill(L'0') << std::setw(8) << std::this_thread::get_id() << L"] " << message << std::endl;

		std::wcout << wss.str();
	}

	/// <summary>
	/// Convert a Unicode string to an ANSI string
	/// </summary>
	inline String ToString(Unicode string)
	{
		std::stringstream ss;
		ss << string.c_str();
		return ss.str().c_str();
	}

	/// <summary>
	/// Write an ANSI string to the console
	/// </summary>
	inline void WriteLine(String message)
	{
		auto u = ToUnicode(message);
		WriteLine(u);
	}

	inline void ThrowIfFailed(HRESULT hr, String reason)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors
			WriteLine(reason);
			throw std::exception(reason.c_str());
		}
	}

	inline void ThrowIfFailed(HRESULT hr)
	{
		ThrowIfFailed(hr, "ClayEngine::ThrowIfFailed()");
	}

	/// <summary>
	/// Returns a vector of Strings from a string using space as default delimiter 
	/// </summary>
	inline Strings SplitString(String string)
	{
		Strings v = {};

		if (string.length() > 0)
		{
			std::istringstream iss(string);
			String s;

			while (std::getline(iss, s, ' '))
			{
				v.push_back(s);
			}
		}
		return v;
	}
	
	/// <summary>
	/// Returns a vector of ANSI strings based on an input string and a char to delimit the tokens
	/// </summary>
	inline Strings LineSplit(const String& line, const char delimiter)
	{
		std::istringstream iss(line); // Turn the line into an stringstream so we can getline on it

		Strings v = {};
		String s;

		while (std::getline(iss, s, delimiter))
		{
			v.push_back(s);
		}

		return v;
	}

}
