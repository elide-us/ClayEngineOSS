#include "pch.h"
#include "ContentSystem.h"

#include "DX11Resources.h"

ClayEngine::ContentSystem::ContentSystem(AffinityData affinityData)
    : m_affinity_data(affinityData)
{
    StartContentSystem();
}

ClayEngine::ContentSystem::~ContentSystem()
{
    StopContentSystem();
}

void ClayEngine::ContentSystem::StartContentSystem()
{
    auto device = Services::GetService<DX11Resources>(m_affinity_data.this_thread)->GetDevice();

    m_textures = Services::MakeService<TextureResources>(m_affinity_data, true);
    m_textures->SetDevice(device);

    m_fonts = Services::MakeService<FontResources>(m_affinity_data, true);
    m_fonts->SetDevice(device);

    m_json = std::make_unique<JsonFile>(c_content_filename);
    auto document = m_json->GetDocument();

    auto sprites = document["sprites"];
    std::for_each(sprites.begin(), sprites.end(), [&](auto& element) { m_textures->AddTexture(element.get<std::string>()); });

    auto fonts = document["fonts"];
    std::for_each(fonts.begin(), fonts.end(), [&](auto& element) { m_fonts->AddFont(element.get<std::string>()); });
}

void ClayEngine::ContentSystem::StopContentSystem()
{
    if (m_fonts)
    {
        Services::RemoveService<FontResources>(m_affinity_data.this_thread);
        m_fonts.reset();
        m_fonts = nullptr;
    }

    if (m_textures)
    {
        Services::RemoveService<TextureResources>(m_affinity_data.this_thread);
        m_textures.reset();
        m_textures = nullptr;
    }

    if (m_json)
    {
        m_json.reset();
        m_json = nullptr;
    }
}

void ClayEngine::ContentSystem::RestartContentSystem()
{
    StopContentSystem();

    StartContentSystem();
}

void ClayEngine::ContentSystem::OnDeviceLost()
{

}

ClayEngine::TextureResourcesRaw ClayEngine::ContentSystem::GetTextureResources()
{
    if (m_textures)
        return m_textures.get();
    else
        return nullptr;
}

ClayEngine::FontResourcesRaw ClayEngine::ContentSystem::GetFontResources()
{
    if (m_fonts)
        return m_fonts.get();
    else
        return nullptr;
}

ClayEngine::TextureRaw ClayEngine::ContentSystem::GetTexture(String key)
{
    return m_textures->GetTexture(key);
}

ClayEngine::SpriteFontRaw ClayEngine::ContentSystem::GetFont(String key)
{
    return m_fonts->GetFont(key);
}
