#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Sprite Class Library (C) 2022 Epoch Meridian, LLC.              */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include "ClayEngine.h"
#include "ClayMath.h"
#include "RenderSystem.h"
#include "ContentSystem.h"
#include "TimingSystem.h"
#include "Sprite.h"

#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "SimpleMath.h"

namespace ClayEngine
{
	namespace SimpleMath
	{
		using namespace DirectX;
		using namespace DirectX::SimpleMath;
		using namespace ClayEngine;
		using namespace ClayEngine::Graphics;
		using namespace ClayEngine::Platform;

		namespace Extensions
		{
			/// <summary>
			/// A class extension that provides an interface for adjust Destination, Position, and Size
			/// </summary>
			struct DestinationExtension
			{
			protected:
				Rectangle m_destination = { 0, 0, 0, 0 };
			public:
				Vector2 GetSize()
				{
					return Vector2{ float(m_destination.width), float(m_destination.height) };
				}
				void SetSize(long x, long y)
				{
					m_destination.width = x;
					m_destination.height = y;
				}
				void SetSize(float x, float y)
				{
					SetSize(long(x), long(y));
				}
				void SetSize(Vector2 size)
				{
					SetSize(size.x, size.y);
				}
				void AddSize(long x, long y)
				{
					m_destination.width += x;
					m_destination.height += y;
				}
				void AddSize(float x, float y)
				{
					AddSize(long(x), long(y));
				}
				void AddSize(Vector2 size)
				{
					AddSize(size.x, size.y);
				}

				Vector2 GetPosition()
				{
					return Vector2{ float(m_destination.x), float(m_destination.y) };
				}
				void SetPosition(long x, long y)
				{
					m_destination.x = x;
					m_destination.y = y;
				}
				void SetPosition(float x, float y)
				{
					SetPosition(long(x), long(y));
				}
				void SetPosition(Vector2 position)
				{
					SetPosition(position.x, position.y);
				}
				void AddPosition(long x, long y)
				{
					m_destination.x += x;
					m_destination.y += y;
				}
				void AddPosition(float x, float y)
				{
					AddPosition(long(x), long(y));
				}
				void AddPosition(Vector2 position)
				{
					AddPosition(position.x, position.y);
				}

				const Rectangle& GetDestination()
				{
					return m_destination;
				}
				void SetDestination(long x, long y, long width, long height)
				{
					SetPosition(x, y);
					SetSize(width, height);
				}
				void SetDestination(float x, float y, float width, float height)
				{
					SetDestination(long(x), long(y), long(width), long(height));
				}
				void SetDestination(Vector2 position, Vector2 size)
				{
					SetDestination(position.x, position.y, size.x, size.y);
				}
				void SetDestination(Rectangle destination)
				{
					SetDestination(destination.x, destination.y, destination.width, destination.height);
				}
				void AddDestination(long x, long y, long width, long height)
				{
					AddPosition(x, y);
					AddSize(width, height);
				}
				void AddDestination(float x, float y, float width, float height)
				{
					AddDestination(long(x), long(y), long(width), long(height));
				}
				void AddDestination(Vector2 position, Vector2 size)
				{
					AddDestination(position.x, position.y, size.x, size.y);
				}
				void AddDestination(Rectangle destination)
				{
					AddDestination(destination.x, destination.y, destination.width, destination.height);
				}
			};

			/// <summary>
			/// A class extension that provides an interface for adjusting Scale, Rotation, Color, Depth, Flip
			/// </summary>
			struct AdvancedSpriteExtension
			{
			protected:
				Vector2 m_origin = { 0.f, 0.f };
				Vector2 m_scale = { 1.f, 1.f };
				float m_rotation = { 0.f };

				Vector4 m_color = Vector4{ Colors::White };

				float m_depth = { 0.f };

				bool m_vertical = false;
				bool m_horizontal = false;
				SpriteEffects m_flip = SpriteEffects::SpriteEffects_None;

			public:
				const Vector2& GetOrigin()
				{
					return m_origin;
				}
				void SetOrigin(float x, float y)
				{
					m_origin.x = x;
					m_origin.y = y;
				}
				void SetOrigin(Vector2 origin)
				{
					SetOrigin(origin.x, origin.y);
				}

				const Vector2& GetScale()
				{
					return m_scale;
				}
				void SetScale(float x, float y)
				{
					m_scale.x = x;
					m_scale.y = y;
				}
				void SetScale(Vector2 scale)
				{
					SetScale(scale.x, scale.y);
				}
				void SetScale(float scale)
				{
					SetScale(scale, scale);
				}

				float GetRotation()
				{
					return m_rotation;
				}
				void SetRotation(float radians)
				{
					m_rotation = std::fmod(radians, c_2pi);
				}
				void AddRotation(float radians)
				{
					m_rotation += std::fmod(radians, c_2pi);
				}

				const Vector4& GetRGBA()
				{
					return m_color;
				}
				void SetRGBA(float red, float green, float blue, float alpha)
				{
					m_color.x = (red > 1.f) ? 1.f : (red < 0.f) ? 0.f : red;
					m_color.y = (green > 1.f) ? 1.f : (green < 0.f) ? 0.f : green;
					m_color.z = (blue > 1.f) ? 1.f : (blue < 0.f) ? 0.f : blue;
					m_color.w = (alpha > 1.f) ? 1.f : (alpha < 0.f) ? 0.f : alpha;
				}
				void SetRGBA(Vector4 color)
				{
					SetRGBA(color.x, color.y, color.z, color.w);
				}
				void AddRGBA(float red, float green, float blue, float alpha)
				{
					auto res_x = m_color.x += red;
					m_color.x = (res_x > 1.f) ? 1.f : (res_x < 0.f) ? 0.f : res_x;
					auto res_y = m_color.y += green;
					m_color.y = (res_y > 1.f) ? 1.f : (res_y < 0.f) ? 0.f : res_y;
					auto res_z = m_color.z += blue;
					m_color.z = (res_z > 1.f) ? 1.f : (res_z < 0.f) ? 0.f : res_z;
					auto res_w = m_color.w += alpha;
					m_color.w = (res_w > 1.f) ? 1.f : (res_w < 0.f) ? 0.f : res_w;
				}

				float GetAlpha()
				{
					return m_color.z;
				}
				void SetAlpha(float alpha)
				{
					m_color.w = (alpha > 1.f) ? 1.f : (alpha < 0.f) ? 0.f : alpha;
				}
				void AddAlpha(float alpha)
				{
					auto result = m_color.w + alpha;
					m_color.w = (result > 1.f) ? 1.f : (result < 0.f) ? 0.f : result;
				}

				float GetDepth()
				{
					return m_depth;
				}
				void SetDepth(float depth)
				{
					m_depth = (depth > 1.f) ? 1.f : (depth < 0.f) ? 0.f : depth;
				}
				void AddDepth(float depth)
				{
					auto result = m_depth + depth;
					m_depth = (result > 1.f) ? 1.f : (result < 0.f) ? 0.f : result;
				}

				void SetHorizontalMirror(bool horizontal)
				{
					m_horizontal = horizontal;
					if (m_horizontal)
					{
						if (m_vertical)
							m_flip = SpriteEffects::SpriteEffects_FlipBoth;
						else
							m_flip = SpriteEffects::SpriteEffects_FlipHorizontally;
					}
					else
					{
						if (m_vertical)
							m_flip = SpriteEffects::SpriteEffects_FlipVertically;
						else
							m_flip = SpriteEffects::SpriteEffects_None;
					}
				}
				void SetVerticalMirror(bool vertical)
				{
					m_vertical = vertical;
					if (m_vertical)
					{
						if (m_horizontal)
							m_flip = SpriteEffects::SpriteEffects_FlipBoth;
						else
							m_flip = SpriteEffects::SpriteEffects_FlipVertically;
					}
					else
					{
						if (m_horizontal)
							m_flip = SpriteEffects::SpriteEffects_FlipHorizontally;
						else
							m_flip = SpriteEffects::SpriteEffects_None;
					}
				}
			};
		}

		/// <summary>
		/// Base sprite class provides support for sprite batch drawing and shape manipulation
		/// </summary>
		class BaseSprite
			: public Extensions::DestinationExtension
			, public Extensions::AdvancedSpriteExtension
			, public ClayEngine::Graphics::SpriteBatchExtension
			, public ClayEngine::Graphics::ContentSystemExtension
		{
		protected:
			bool m_active = false;
			bool m_drawscalar = false;

		public:
			BaseSprite() = default;
			~BaseSprite() = default;

			bool GetActive() { return m_active; }
			void SetActive(bool active) { m_active = active; }
		};

		/// <summary>
		/// A simple Sprite implementation with no animation
		/// </summary>
		class Sprite : public BaseSprite
			, public Extensions::DrawExtension
		{
			TextureRaw m_texture = nullptr;

			RECT m_source = { 0, 0, 0, 0 };

		public:
			Sprite(String texture, Rectangle source, Vector4 color = Vector4(Colors::White), Rectangle destination = Rectangle{ 0, 0, 0, 0 });
			~Sprite();

			bool Contains(Vector2 location) { return m_destination.Contains(location); }

			void Draw() override;
		};
		using SpritePtr = std::unique_ptr<Sprite>;
		using SpriteRaw = Sprite*;

		/// <summary>
		/// An implementation of BaseSprite that provides a standard animated sprite strip
		/// with an optional static frame
		/// </summary>
		class SpriteStrip : public BaseSprite
			, public Extensions::UpdateDrawExtension
		{
			bool m_animation_paused = false;
			bool m_draw_static = false;

			TextureRaw m_texture = nullptr;

			using Frames = std::vector<RECT>;
			Frames m_frames = {};

			double m_frame_elapsed_time = 0.;
			size_t m_current_frame = 0;
			double m_time_per_frame = 0.1; // In seconds
			size_t m_frame_count = 0;

			bool m_has_static = false;

		public:
			/// <summary>
			/// Assumes the sprite animation frames are horizontally arranged with no gap and one optional "static" frame at the end
			/// </summary>
			SpriteStrip(String texture, Rectangle source, size_t frameCount, bool hasStatic);
			~SpriteStrip();

			void SetPaused(bool paused) { m_animation_paused = paused; }
			void SetStatic(bool stat) { m_draw_static = stat; }
			void SetFramerate(float fps) { m_time_per_frame = 1.f / fps; }

			void Update(float elapsedTime) override;
			void Draw() override;
		};
		using SpriteStripPtr = std::unique_ptr<SpriteStrip>;
		using SpriteStripRaw = SpriteStrip*;

		/// <summary>
		/// A realized class of BaseSprite that implements drawing text on the screen
		/// </summary>
		class SpriteString : public BaseSprite
			, public Extensions::DrawExtension
		{
			SpriteFontRaw m_spritefont = nullptr;
			SpriteFontRaw m_shadowfont = nullptr;

			bool m_drawshadow = true;
			int m_shadow_offset = 1;

			Unicode m_string = {};

		public:
			SpriteString(String font);
			~SpriteString();

			void SetString(Unicode string) { m_string = string; }
			float GetWidth() { return Vector2(m_spritefont->MeasureString(m_string.c_str())).x; }
			void SetDropShadow(bool enable) { m_drawshadow = enable; }
			void SetShadowOffset(int offset) { m_shadow_offset = offset; }

			void Draw() override;
		};
		using SpriteStringPtr = std::unique_ptr<SpriteString>;
		using SpriteStringRaw = SpriteString*;
	}
}
