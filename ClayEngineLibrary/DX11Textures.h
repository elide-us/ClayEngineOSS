#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Texture Class Library (C) 2022 Epoch Meridian, LLC.             */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"
#include "DX11Resources.h"
#include "SpriteFont.h"

namespace ClayEngine
{
	namespace Graphics
	{
		using namespace DirectX;
		using namespace Microsoft::WRL;

		using ResourceComPtr = ComPtr<ID3D11Resource>;
		using ResourceRaw = ID3D11Resource*;
		using TextureComPtr = ComPtr<ID3D11ShaderResourceView>;
		using TextureRaw = ID3D11ShaderResourceView*;
		using SpriteFontPtr = std::unique_ptr<SpriteFont>;
		using SpriteFontRaw = SpriteFont*;

		inline bool ResourceIsD3D11Texture2D(TextureComPtr texture) noexcept
		{
			if (!texture) return false;

			ResourceComPtr resource = {};
			texture->GetResource(resource.GetAddressOf());

			D3D11_RESOURCE_DIMENSION dimension = {};
			resource->GetType(&dimension);

			return (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D);
		}

		class DX11DeviceInitExtension
		{
		protected:
			DevicePtr m_device = nullptr;
		public:
			DX11DeviceInitExtension()
			{

			}
			~DX11DeviceInitExtension()
			{
				m_device = nullptr;
			}

			void SetDevice(DevicePtr device) { m_device = device; }
			void OnDeviceLost() { m_device = nullptr; }
		};

		class TextureResources
			: public DX11DeviceInitExtension
		{
			using TexturesMap = std::map<String, TextureComPtr>;
			TexturesMap m_textures = {};

		public:
			TextureResources() = default;
			~TextureResources() = default;

			void AddTexture(String texture);
			TextureRaw GetTexture(String texture);
			void RemoveTexture(String texture);
			void ClearTextures();
		};
		using TextureResourcesPtr = std::unique_ptr<TextureResources>;
		using TextureResourcesRaw = TextureResources*;

		class FontResources
			: public DX11DeviceInitExtension
		{
			using FontsMap = std::map<String, SpriteFontPtr>;
			FontsMap m_fonts = {};

		public:
			FontResources() = default;
			~FontResources() = default;

			void AddFont(String font);
			SpriteFontRaw GetFont(String font);
			void RemoveFont(String font);
			void ClearFonts();
		};
		using FontResourcesPtr = std::unique_ptr<FontResources>;
		using FontResourcesRaw = FontResources*;
	}
}
