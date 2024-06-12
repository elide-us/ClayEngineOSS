#include "pch.h"
#include "Sprite.h"

#include "TimingSystem.h"
#include "SpriteBatch.h"

using namespace ClayEngine;
using namespace ClayEngine::Graphics;
using namespace ClayEngine::SimpleMath;

#pragma region Sprite
Sprite::Sprite(String texture, Rectangle source, Vector4 color, Rectangle destination)
{
	m_texture = m_cs->GetTexture(texture); //TODO: Exception handling for this call

	m_color = color;
	m_destination = destination;
	m_source = source;

	m_active = true;
}

Sprite::~Sprite()
{
	m_texture = nullptr;
}

void Sprite::Draw()
{
	if (!m_active) return;

	if (m_drawscalar)
	{
		// Draw using X/Y Position and Scale values
		m_spritebatch->Draw(
			m_texture,
			GetPosition(),
			&m_source,
			m_color,
			m_rotation,
			m_origin,
			m_scale,
			m_flip,
			m_depth
		);
	}
	else
	{
		// Draw using destination Rectangle
		m_spritebatch->Draw(
			m_texture,      // ID3D11ShaderResourceView*
			m_destination,  // const RECT&
			&m_source,      // const RECT*
			m_color,        // FXMVECTOR (Colors::White)
			m_rotation,     // float (0)
			m_origin,       // const XMFLOAT2& (Float2Zero)
			m_flip,         // SpriteEffects (SpriteEffects_None)
			m_depth         // float (0)
		);
	}

}
#pragma endregion

#pragma region SpriteStrip
SpriteStrip::SpriteStrip(String texture, Rectangle source, size_t frameCount, bool hasStatic)
{
	m_texture = m_cs->GetTexture(texture);
	m_frame_count = frameCount;
	m_has_static = hasStatic;

	for (auto i = 0ULL; i <= frameCount; ++i)
	{
		RECT r;
		r.left = source.x + (source.width * long(i));
		r.top = source.y;
		r.right = r.left + source.width;
		r.bottom = r.top + source.height;

		if (i < m_frame_count || (i == frameCount && hasStatic))
		{
			m_frames.push_back(r);
		}
	}

	m_active = true;
}

SpriteStrip::~SpriteStrip()
{
	m_texture = nullptr;
}

void SpriteStrip::Update(float elapsedTime)
{
	if (!m_active || m_animation_paused) return;

	m_frame_elapsed_time += elapsedTime;

	if (m_frame_elapsed_time >= m_time_per_frame)
	{
		if (m_draw_static)
		{
			m_current_frame = m_has_static ? m_frame_count : 0;
		}
		else
		{
			m_current_frame = (m_current_frame + 1) % m_frame_count;
		}

		m_frame_elapsed_time = 0.f;
	}
}

void SpriteStrip::Draw()
{
	if (!m_active) return;

	if (m_drawscalar)
	{
		// Draw using X/Y Position and Scale values
		m_spritebatch->Draw(
			m_texture,
			GetPosition(),
			&m_frames[m_current_frame],
			m_color,
			m_rotation,
			m_origin,
			m_scale,
			m_flip,
			m_depth
		);
	}
	else
	{
		// Draw using destination Rectangle
		m_spritebatch->Draw(
			m_texture,						// ID3D11ShaderResourceView*
			m_destination,					// const RECT&
			&m_frames[m_current_frame],		// const RECT*
			m_color,						// FXMVECTOR (Colors::White)
			m_rotation,						// float (0)
			m_origin,						// const XMFLOAT2& (Float2Zero)
			m_flip,							// SpriteEffects (SpriteEffects_None)
			m_depth							// float (0)
		);
	}
}
#pragma endregion

#pragma region SpriteString
SpriteString::SpriteString(String font)
{
	m_spritefont = m_cs->GetFont(font);
	m_shadowfont = m_cs->GetFont(font);

	m_string = L"";

	m_active = true;
}

SpriteString::~SpriteString()
{
	m_shadowfont = nullptr;
	m_spritefont = nullptr;
}

void SpriteString::Draw()
{
	if (!m_active) return;

	if (m_drawshadow)
	{
		auto pos = GetPosition();
		pos.x += m_shadow_offset;
		pos.y += m_shadow_offset;

		m_shadowfont->DrawString(
			m_spritebatch,
			m_string.c_str(),
			pos,
			Vector4{ DirectX::Colors::Black },
			m_rotation,
			m_origin,
			m_scale,
			m_flip,
			m_depth
		);
	}

	m_spritefont->DrawString(
		m_spritebatch,
		m_string.c_str(),
		GetPosition(),
		m_color,
		m_rotation,
		m_origin,
		m_scale,
		m_flip,
		m_depth
	);
}
#pragma endregion
