#include "pch.h"
#include "Storage.h"

ClayEngine::TextFile::TextFile(ClayEngine::String filename) noexcept(false)
    : m_filename{ filename }
{
    // Open file stream for reading
    std::ifstream ifs;
    ifs.open(m_filename, std::ios::in);

    // Read the lines into a vector and close stream
    for (ClayEngine::String line; std::getline(ifs, line);)
    {
        m_lines.push_back(line);
    }

    ifs.close();
}

ClayEngine::TextFile::~TextFile()
{
    // Open file stream for writing (and truncate target file)
    std::ofstream ofs;
    ofs.open(m_filename, std::ios::out | std::ios_base::trunc);

    // Loop lines into the out file
    std::for_each(m_lines.begin(), m_lines.end(), [&](auto& element) { ofs << element << std::endl; });
    ofs.close();
}

ClayEngine::Strings const& ClayEngine::TextFile::GetLines() const
{
    return m_lines;
}

ClayEngine::JsonFile::JsonFile(ClayEngine::String filename) noexcept(false)
    :m_filename{ filename }
{
    std::ifstream ifs{ m_filename };
    ifs >> m_document;
    ifs.close();
}

ClayEngine::JsonFile::~JsonFile()
{
    std::ofstream ofs{ m_filename };
    ofs << std::setw(2) << m_document << std::endl;
}

ClayEngine::Document const& ClayEngine::JsonFile::GetDocument() const
{
    return m_document;
}