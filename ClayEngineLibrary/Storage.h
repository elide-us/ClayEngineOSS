#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Storage Class Library (C) 2022-2024 Epoch Meridian, LLC.        */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "Strings.h"
#include "json.hpp"

namespace ClayEngine
{
	namespace Platform
	{
		/// <summary>
		/// This RAII class parses a given UTF-8 text file into a std::vector of std::string
		/// </summary>
		class TextFile
		{
			String m_filename;
			Strings m_lines = {};

		public:
			TextFile(String filename) noexcept(false);
			TextFile(TextFile const&) = delete;
			TextFile& operator=(TextFile const&) = delete;
			TextFile(TextFile&&) = default;
			TextFile& operator=(TextFile&&) = default;
			~TextFile();

			Strings const& GetLines() const;
		};
		using TextFilePtr = std::unique_ptr<TextFile>;
		using TextFileRaw = TextFile*;

		using Document = nlohmann::json;

		/// <summary>
		/// This RAII class parses a given UTF-8 JSON file into a nlohmann::json DOM object
		/// </summary>
		class JsonFile
		{
			String m_filename;
			Document m_document;

		public:
			JsonFile(String filename) noexcept(false);
			JsonFile(JsonFile const&) = delete;
			JsonFile& operator=(JsonFile const&) = delete;
			JsonFile(JsonFile&&) = default;
			JsonFile& operator=(JsonFile&&) = default;
			~JsonFile();

			Document const& GetDocument() const;
		};
		using JsonFilePtr = std::unique_ptr<JsonFile>;
		using JsonFileRaw = JsonFile*;
	}
}