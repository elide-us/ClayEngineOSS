#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Storage header for file and JSON document I/O                              */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include <memory>

#include "json.hpp"

#include "Strings.h"

namespace ClayEngine
{
	constexpr auto c_bootstrap_json = "clayengine.json";
	constexpr auto c_bootstrap_headless_json = "clayheadless.json";

	using Document = nlohmann::json;

	/// <summary>
	/// This RAII class parses a given UTF-8 text file into a std::vector of std::string
	/// </summary>
	class TextFile
	{
		String m_filename;
		Strings m_lines = {};

	public:
		TextFile(String filename) noexcept(false);
		~TextFile();

		Strings const& GetLines() const;
	};
	using TextFilePtr = std::unique_ptr<TextFile>;
	using TextFileRaw = TextFile*;

	/// <summary>
	/// This RAII class parses a given UTF-8 JSON file into a nlohmann::json DOM object
	/// </summary>
	class JsonFile
	{
		String m_filename;
		Document m_document;

	public:
		JsonFile(String filename) noexcept(false);
		~JsonFile();

		// Helper functions to access document get/set for simple top layer values
		template<typename T> T GetValue(String key) { return m_document[key].get<T>(); }
		template<typename T> void SetValue(String key, T value) { m_document[key] = value; }
		template<typename T> T GetValue(String header, String key) { return m_document[header][key].get<T>(); }
		template<typename T> void SetValue(String header, String key, T value) { m_document[header][key] = value; }

		// Access the entire Document Object Model directly
		const Document& GetDocument() const;
	};
	using JsonFilePtr = std::unique_ptr<JsonFile>;
	using JsonFileRaw = JsonFile*;
}

#pragma region Orphaned Code Fragments
//TextFile(TextFile const&) = delete;
//TextFile& operator=(TextFile const&) = delete;
//TextFile(TextFile&&) = default;
//TextFile& operator=(TextFile&&) = default;
//JsonFile(JsonFile const&) = delete;
//JsonFile& operator=(JsonFile const&) = delete;
//JsonFile(JsonFile&&) = default;
//JsonFile& operator=(JsonFile&&) = default;
#pragma endregion
