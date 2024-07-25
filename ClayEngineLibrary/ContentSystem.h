#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngineOSS (C) 2024 Elideus                                             */
/* Content system API class                                                   */
/* https://github.com/elide-us                                                */
/*                                                                            */
/******************************************************************************/

#include "Services.h"
#include "Storage.h"
#include "DX11Textures.h"

namespace ClayEngine
{
    constexpr auto c_content_filename = "content.json";

    /// <summary>
    ///  API entry point for the font and 2D texture resources
    /// </summary>
    class ContentSystem
    {
        AffinityData m_affinity;

        JsonFilePtr m_json = nullptr;

        TextureResourcesPtr m_textures = nullptr;
        FontResourcesPtr m_fonts = nullptr;

    public:
        ContentSystem(AffinityData affinityId);
        ~ContentSystem();

        void StartContentSystem();
        void StopContentSystem();
        void RestartContentSystem();

        void OnDeviceLost();

        TextureResourcesRaw GetTextureResources();
        FontResourcesRaw GetFontResources();
        TextureRaw GetTexture(String key);
        SpriteFontRaw GetFont(String key);
    };
    using ContentSystemPtr = std::unique_ptr<ContentSystem>;
    using ContentSystemRaw = ContentSystem*;

    //TODO: Another instance of possible issues due to Affinity
    class ContentSystemExtension
    {
    protected:
        ContentSystemRaw m_cs = nullptr;
    public:
        ContentSystemExtension()
        {
            m_cs = Services::GetService<ContentSystem>(std::this_thread::get_id());
        }
        ~ContentSystemExtension()
        {
            m_cs = nullptr;
        }
    };
}