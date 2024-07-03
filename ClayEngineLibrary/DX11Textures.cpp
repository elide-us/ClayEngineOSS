#include "pch.h"
#include "DX11Textures.h"

using namespace DirectX;

#include "WICTextureLoader.h"

ClayEngine::TextureResources::TextureResources(Affinity affinityId)
	: m_affinity(affinityId)
{

}

void ClayEngine::TextureResources::AddTexture(String texture)
{
	if (m_device.Get() == 0) WriteLine("AddTexture DEBUG: Cannot access Textures before Device is assigned.");
	auto it = m_textures.find(texture);
	if (it != m_textures.end()) WriteLine("AddTexture DEBUG: Duplicate key found when trying to add texture.");

	auto s = ToUnicode(texture.c_str());
	std::wstringstream path;
	path << L"content\\sprites\\" << s << L".dds";

	TextureComPtr t = {};
	ThrowIfFailed(CreateWICTextureFromFile(m_device.Get(), path.str().c_str(), nullptr, t.ReleaseAndGetAddressOf()));
	assert(ResourceIsD3D11Texture2D(t));

	m_textures.emplace(texture, std::move(t));

	std::wstringstream ss;
	ss << L"AddTexture SUCCESS: " << s << L" from " << path.str();
	WriteLine(ss.str());
}

ClayEngine::TextureRaw ClayEngine::TextureResources::GetTexture(String texture)
{
	auto it = m_textures.find(texture);
	if (it != m_textures.end())
	{
		return it->second.Get();
	}
	WriteLine("GetTexture DEBUG: Texture key not found in Textures map.");
	return nullptr;
}

void ClayEngine::TextureResources::RemoveTexture(String texture)
{
	auto it = m_textures.find(texture);
	if (it != m_textures.end())
	{
		auto s = ToUnicode(texture.c_str());
		std::wstringstream path;
		path << L"content\\sprites\\" << s << L".dds";

		std::wstringstream ss;
		ss << L"RemoveTexture SUCCESS: " << s << L" from " << path.str();
		WriteLine(ss.str());

		m_textures.erase(it);
		return;
	}
	WriteLine("GetTexture DEBUG: Texture key not found in Textures map.");
}

void ClayEngine::TextureResources::ClearTextures()
{
	m_textures.clear();
	WriteLine("ClearTextures INFO: Texture map cleared.");
}

ClayEngine::FontResources::FontResources(Affinity affinityId)
	: m_affinity(affinityId)
{

}

void ClayEngine::FontResources::AddFont(String font)
{
	if (m_device.Get() == 0) WriteLine("AddFont DEBUG: Cannot access Fonts before Device is assigned.");
	auto it = m_fonts.find(font);
	if (it != m_fonts.end()) WriteLine("AddFont DEBUG: Duplicate key found when trying to add font.");

	auto s = ToUnicode(font.c_str());
	std::wstringstream path;
	path << L"content\\fonts\\" << s << L".spritefont";

	m_fonts.emplace(font, std::make_unique<SpriteFont>(m_device.Get(), path.str().c_str()));

	std::wstringstream ss;
	ss << L"AddFont SUCCESS: " << s << L" from " << path.str();
	WriteLine(ss.str());
}

ClayEngine::SpriteFontRaw ClayEngine::FontResources::GetFont(String font)
{
	auto it = m_fonts.find(font);
	if (it != m_fonts.end())
	{
		return it->second.get();
	}
	WriteLine("GetFont DEBUG: Font key not found in Fonts map.");
	return nullptr;
}

void ClayEngine::FontResources::RemoveFont(String font)
{
	auto it = m_fonts.find(font);
	if (it != m_fonts.end())
	{
		auto s = ToUnicode(font.c_str());
		std::wstringstream path;
		path << L"content\\sprites\\" << s << L".dds";

		std::wstringstream ss;
		ss << L"RemoveFont SUCCESS: " << s << L" from " << path.str();
		WriteLine(ss.str());

		m_fonts.erase(it);
		return;
	}
	WriteLine("GetFont DEBUG: Font key not found in Fonts map.");
}

void ClayEngine::FontResources::ClearFonts()
{
	m_fonts.clear();
	WriteLine("ClearFonts INFO: SpriteFont map cleared.");
}
