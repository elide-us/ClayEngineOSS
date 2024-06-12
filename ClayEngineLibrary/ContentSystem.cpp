#include "pch.h"
#include "ContentSystem.h"

#include "DX11Resources.h"

using namespace ClayEngine;
using namespace ClayEngine::Graphics;
using namespace ClayEngine::Platform;

ContentSystem::ContentSystem()
{
    StartContentSystem();
}

ContentSystem::~ContentSystem()
{
    StopContentSystem();
}

void ClayEngine::Graphics::ContentSystem::StartContentSystem()
{
    auto device = Services::GetService<DX11Resources>(std::this_thread::get_id())->GetDevice();

    m_textures = Services::MakeService<TextureResources>(std::this_thread::get_id());
    m_textures->SetDevice(device);

    m_fonts = Services::MakeService<FontResources>(std::this_thread::get_id());
    m_fonts->SetDevice(device);

    m_json = std::make_unique<JsonFile>(c_content_filename);
    auto document = m_json->GetDocument();

    auto sprites = document["sprites"];
    std::for_each(sprites.begin(), sprites.end(), [&](auto& element) { m_textures->AddTexture(element.get<std::string>()); });

    auto fonts = document["fonts"];
    std::for_each(fonts.begin(), fonts.end(), [&](auto& element) { m_fonts->AddFont(element.get<std::string>()); });
}

void ClayEngine::Graphics::ContentSystem::StopContentSystem()
{
    if (m_fonts)
    {
        Services::RemoveService<FontResources>(std::this_thread::get_id());
        m_fonts.reset();
        m_fonts = nullptr;
    }

    if (m_textures)
    {
        Services::RemoveService<TextureResources>(std::this_thread::get_id());
        m_textures.reset();
        m_textures = nullptr;
    }

    if (m_json)
    {
        m_json.reset();
        m_json = nullptr;
    }
}

void ClayEngine::Graphics::ContentSystem::RestartContentSystem()
{
    StopContentSystem();

    StartContentSystem();
}

void ClayEngine::Graphics::ContentSystem::OnDeviceLost()
{

}

TextureResourcesRaw ClayEngine::Graphics::ContentSystem::GetTextureResources()
{
    if (m_textures)
        return m_textures.get();
    else
        return nullptr;
}

FontResourcesRaw ClayEngine::Graphics::ContentSystem::GetFontResources()
{
    if (m_fonts)
        return m_fonts.get();
    else
        return nullptr;
}

TextureRaw ClayEngine::Graphics::ContentSystem::GetTexture(String key)
{
    return m_textures->GetTexture(key);
}

SpriteFontRaw ClayEngine::Graphics::ContentSystem::GetFont(String key)
{
    return m_fonts->GetFont(key);
}
