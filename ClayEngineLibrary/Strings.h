#pragma once

#include <thread>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

namespace ClayEngine
{
	using Unicode = ::std::wstring;
	using UnicodeStrings = ::std::vector<::std::wstring>;
	using String = ::std::string;
	using Strings = ::std::vector<::std::string>;
	using StringStream = ::std::stringstream;
	using StringStreamW = ::std::wstringstream;
	using IStringStream = ::std::istringstream;

	/// <summary>
	/// Convert an ANSI string to a Unicode string
	/// </summary>
	inline Unicode ToUnicode(String string)
	{
		StringStreamW wss;
		wss << string.c_str();
		return wss.str().c_str();
	}

	/// <summary>
	/// Write a Unicode string to the console
	/// </summary>
	inline void WriteLine(Unicode message)
	{
		StringStreamW wss;
		wss << L"[" << ::std::setfill(L'0') << ::std::setw(8) << ::std::this_thread::get_id() << L"] " << message << ::std::endl;

		::std::wcout << wss.str();
	}

	/// <summary>
	/// Convert a Unicode string to an ANSI string
	/// </summary>
	inline String ToString(Unicode string)
	{
		StringStream ss;
		ss << string.c_str();
		return ss.str().c_str();
	}

	/// <summary>
	/// Write an ANSI string to the console
	/// </summary>
	/// <param name="message"></param>
	inline void WriteLine(String message)
	{
		auto u = ToUnicode(message);
		WriteLine(u);
	}

	/// <summary>
	/// Returns a vector of Strings from a string using space as default delimiter 
	/// </summary>
	/// <param name="string"></param>
	/// <returns></returns>
	inline Strings SplitString(String string)
	{
		Strings v = {};

		if (string.length() > 0)
		{
			IStringStream iss(string);
			String s;

			while (::std::getline(iss, s, ' '))
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
		IStringStream iss(line); // Turn the line into an stringstream so we can getline on it

		Strings v = {};
		String s;

		while (::std::getline(iss, s, delimiter))
		{
			v.push_back(s);
		}

		return v;
	}

}