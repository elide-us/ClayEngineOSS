#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Content System API Class (C) 2022 Epoch Meridian, LLC.          */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"
#include "DX11Textures.h"

namespace ClayEngine
{
	namespace Graphics
	{
        using namespace ClayEngine::Platform;

        constexpr auto c_content_filename = "content.json";

        /// <summary>
        ///  API entry point for the font and 2D texture resources
        /// </summary>
        class ContentSystem
        {
            JsonFilePtr m_json = nullptr;

            TextureResourcesPtr m_textures = nullptr;
            FontResourcesPtr m_fonts = nullptr;

        public:
            ContentSystem();
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
}