#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine User Interface Class Library (C) 2022 Epoch Meridian, LLC.      */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include <map>
#include <array>
#include <memory>
#include <vector>
#include <typeinfo>
#include <typeindex>
//#include <functional>
#include <utility>

#include "SpriteBatch.h"
#include "Mouse.h"

#include "ClayEngine.h"
#include "InputSystem.h"
#include "ContentSystem.h"
#include "TimingSystem.h"
#include "WindowSystem.h"
#include "Sprite.h"

namespace ClayEngine
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;
	using namespace ClayEngine::Graphics;
	using namespace ClayEngine::Platform;
	using namespace ClayEngine::SimpleMath;
	using namespace ClayEngine::SimpleMath::Extensions;

	namespace Game
	{
		constexpr auto c_panel_frame_element_count = 8L;
		constexpr auto c_cursor_blink_rate = 125.f;

#pragma region Tree Element Classes
		/// <summary>
		/// A generic tree element system, each element holds one Sprite object
		/// </summary>
		//TreeElement* GetParent() { return m_parent; }
		//TreeElement* GetRoot()
		//{
		//	auto parent = GetParent();
		//	while (parent->GetParent() != nullptr)
		//	{
		//		parent->GetParent();
		//	}
		//	return parent;
		//}
		//	
		//TreeElements GetChildren()
		//{
		//	TreeElements v = {};

		//	if (c.size() > 0)
		//	{
		//		for (auto& outer : c)
		//		{
		//			auto ci = outer->GetChildren();
		//			if (ci.size() > 0)
		//			{
		//				for (auto inner : ci)
		//				{
		//					v.push_back(inner);
		//				}
		//			}
		//			v.push_back(outer.get());
		//		}
		//	}
		//	return v;
		//}

		//template<typename T, typename P>
		//class InterfaceElement
		//{
		//	using Index = size_t;
		//	using Type = std::type_index;
		//	using Object = void*;

		//	using ObjectPtr = std::unique_ptr<T>;
		//	using ParentPtr = std::unique_ptr<P>;

		//	struct ChildObject
		//	{
		//		Index m_index = 0;
		//		Type m_type = typeid(void);
		//		Object m_object = nullptr;
		//	};
		//	using ChildObjectPtr = std::unique_ptr<ChildObject>;

		//	Index m_parent_index = 0;
		//	Type m_parent_type = typeid(void);
		//	Object m_parent_object = nullptr;

		//	Index m_index = 0;
		//	Type m_type = typeid(void);
		//	ObjectPtr m_object = nullptr;

		//	using Children = std::vector<ChildObjectPtr>;
		//	Children m_children = {};

		//public:
		//	template<typename T, typename P>
		//	InterfaceElement(Index index, ObjectPtr object, ParentPtr parent)
		//		: m_index{ index }
		//		, m_object{ object }
		//		, m_parent{ parent }
		//	{}
		//	~InterfaceElement() = default;

		//	Index GetIndex() { return m_index; }
		//	T* GetObject() { return m_object; }

		//	//template<typename T, typename... Args>
		//	//std::unique_ptr<T> MakeChild(Args&&... args)
		//	template<typename T>
		//	void MakeChild()
		//	{
		//		//m_children.emplace_back(std::make_unique<MapObject>(std::forward<Args>(args)...));

		//		auto ie = std::make_unique<InterfaceElement<T>>(index + 1);
		//		m_children.emplace_back(ChildObject{ ie->GetIndex(), Type(typeid(T)), reinterpret_cast<Object>(ie.get()) });
		//	}
		//};
#pragma endregion

		/// <summary>
		/// An element that displays input text with cursor, highlight, and edit display capability
		/// </summary>
		class InputBar
			: public ClayEngine::SimpleMath::Extensions::DestinationExtension
			, public ClayEngine::Platform::InputSystemExtension
			, public ClayEngine::TimingSystemExtension
			, public ClayEngine::Interfaces::IUpdate
		{
			SpriteStringPtr m_text = nullptr;
			SpriteStringPtr m_cursor = nullptr;

			float m_elapsed = 0.F;
			bool m_cursor_visible = true;

			SpritePtr m_background = nullptr;
			//IDEA: Texture to indicate Overwrite mode (block) and also to indicate selection

			// For rendering substring
			Unicode m_substring = L"";
			int m_end_index = 0; // Where the real end of the string is at
			int m_cursor_index = 0; // Where the cursor is in the line
			int m_visible_beginning_index = 0; // Where the visible beginning of the string is at (the real beginning is always 0)
			int m_visible_cursor_index = 0;
			int m_visible_end_index = 0; // Where the visible end of the string is at (when editng back)

			float m_char_width = 12.F; // Consolas_24 = 18.F, Consolas_16 = 12.F
			int m_window_width = 0;
			int m_columns_count = 0;

			int m_last_cursor_index = 0;
			int m_last_end_index = 0;

		public:
			InputBar()
			{
				m_background = std::make_unique<Sprite>("Pixel", Rectangle{ 0, 0, 1, 1 }, Vector4{ 0.f, 0.f, 0.f, 0.65f }, m_destination);
				m_text = std::make_unique<SpriteString>("Consolas_16");
				m_text->SetString(L"");
				m_cursor = std::make_unique<SpriteString>("Consolas_16");
				m_cursor->SetString(L"_");

				ticker->AddUpdateCallback([&](float elapsedTime) { Update(elapsedTime); }); //TODO: Major refactoring to include Update callback attach for IUpdate

				OnLayoutChanged();
			}
			~InputBar()
			{
				if (m_cursor) m_cursor.reset();
				m_cursor = nullptr;

				if (m_text) m_text.reset();
				m_text = nullptr;

				if (m_background) m_background.reset();
				m_background = nullptr;
			}

			void OnLayoutChanged()
			{
				m_text->SetPosition(m_destination.x, m_destination.y);
				m_background->SetDestination(m_destination);

				m_columns_count = int(float(m_destination.width) / m_char_width) - 1;
			}

			void Update(float elapsedTime) override
			{
				m_elapsed += elapsedTime;

				m_end_index = m_is->GetBufferLength();
				m_cursor_index = m_is->GetCursorIndex();

				if (m_end_index < m_visible_end_index)
				{
					m_visible_end_index = m_end_index;
					if (m_end_index < m_columns_count)
					{
						m_visible_beginning_index = 0;
					}
					else
					{
						m_visible_beginning_index = m_end_index - m_columns_count;
					}
				}
				if (m_end_index > m_columns_count)
				{
					if (m_end_index > m_visible_end_index)
					{
						if (m_visible_cursor_index < m_visible_end_index)
						{
							m_visible_cursor_index = m_cursor_index - m_visible_beginning_index;
						}
						if (m_visible_cursor_index == m_visible_end_index)
						{
							++m_visible_beginning_index;
							++m_visible_end_index;
						}
					}
					if (m_cursor_index == m_end_index)
					{
						m_visible_beginning_index = m_cursor_index - m_columns_count;
						m_visible_cursor_index = m_cursor_index - m_visible_beginning_index;
						m_visible_end_index = m_end_index;
					}
					else
					{
						if (m_cursor_index < m_visible_beginning_index)
						{
							m_visible_beginning_index = m_cursor_index;
							m_visible_end_index = m_cursor_index + m_columns_count;
							m_visible_cursor_index = m_cursor_index - m_visible_beginning_index;
						}
						else if (m_cursor_index == m_visible_beginning_index)
						{
							if (m_visible_beginning_index > 0)
							{
								--m_visible_beginning_index;
								--m_visible_end_index;
							}
						}
						else
						{
							m_visible_cursor_index = m_cursor_index - m_visible_beginning_index;
						}
					}
				}
				else
				{
					m_visible_beginning_index = 0;
					m_visible_cursor_index = m_cursor_index;
					m_visible_end_index = m_end_index;
				}

				m_substring = m_is->GetBuffer().substr(m_visible_beginning_index, m_columns_count);
				m_text->SetString(m_substring);

				auto p = m_text->GetPosition();
				m_cursor->SetPosition(p.x + (m_visible_cursor_index * m_char_width), p.y);

				if (m_elapsed >= c_cursor_blink_rate)
				{
					m_elapsed = 0.f;
					m_cursor_visible = !m_cursor_visible;
					m_cursor->SetActive(m_cursor_visible);
				}
			}
		};
		using InputBarPtr = std::unique_ptr<InputBar>;

		/// <summary>
		/// An element that supports displaying window title text on a bar
		/// </summary>
		class TitleBar
			: public ClayEngine::SimpleMath::Extensions::DestinationExtension
		{
			SpritePtr m_sprite = nullptr;
			SpriteStringPtr m_title = nullptr;

		public:
			TitleBar(Unicode title)
			{
				m_sprite = std::make_unique<Sprite>("Pixel", Rectangle{ 0, 0, 1, 1 }, Vector4{ 0.f, 0.f, 0.f, 1.f }, m_destination);
				m_title = std::make_unique<SpriteString>("Mason_24");
				m_title->SetString(title);
			}
			~TitleBar()
			{
				if (m_title) m_title.reset();
				m_title = nullptr;

				if (m_sprite) m_sprite.reset();
				m_sprite = nullptr;
			}

			void SetTitle(Unicode title)
			{
				m_title->SetString(title);
			}

			void OnLayoutChanged()
			{
				m_sprite->SetDestination(m_destination);
				m_title->SetPosition(float((m_destination.x + (m_destination.width / 2)) - (m_title->GetWidth() / 2)), float(m_destination.y + 3));
			}
		};
		using TitleBarPtr = std::unique_ptr<TitleBar>;

		/// <summary>
		/// An element that displays text strings within the bounds of a resizable field
		/// </summary>
		class StringDisplayBox
			: public ClayEngine::SimpleMath::Extensions::DestinationExtension
			//, public ClayEngine::SimpleMath::Extensions::UpdateExtension
			, public ClayEngine::TimingSystemExtension
			, public ClayEngine::Interfaces::IUpdate
			, public ClayEngine::Platform::InputSystemExtension
		{
			using SpriteStrings = std::vector<SpriteStringPtr>;
			SpriteStrings m_strings = {};

			float m_font_width = 12.F;
			long m_font_height = 28L;

			int m_max_visible_rows = 0;

			int m_visible_columns = 0;
			int m_visible_rows = 0;

			SpritePtr m_background = nullptr;
			SpriteStringPtr  m_debug = nullptr;

		public:
			StringDisplayBox()
			{
				m_background = std::make_unique<Sprite>("Pixel", Rectangle{ 0, 0, 1, 1 }, Vector4{ 0.F, 0.F, 0.F, 0.35F });
				//m_debug = std::make_unique<SpriteString>("Consolas_10");

				m_max_visible_rows = int(float(WindowSystem::GetWindowHeight()) / float(m_font_height));
				m_strings.reserve(m_max_visible_rows);
				for (int i = 0; i < m_max_visible_rows; ++i)
				{
					m_strings.emplace_back(std::make_unique<SpriteString>("Consolas_16"));
				}

				ticker->AddUpdateCallback([&](float elapsedTime) { Update(elapsedTime); });
			}
			~StringDisplayBox()
			{
				if (m_debug) m_debug.reset();
				m_debug = nullptr;

				if (m_background) m_background.reset();
				m_background = nullptr;
			}

			void OnLayoutChanged()
			{
				m_is->GetDisplayBuffer()->Clear();
				m_background->SetDestination(m_destination);
				//m_debug->SetPosition(m_destination.x + 10L, m_destination.y - 24L);
			}

			void Update(float elapsedTime) override
			{
				m_visible_columns = int(float(m_destination.width) / m_font_width);
				m_visible_rows = int(float(m_destination.height) / float(m_font_height));

				//std::wstringstream wss;
				//wss << L"Col: " << m_visible_columns << L" Row: " << m_visible_rows << L" Max: " << m_max_visible_rows;
				//m_debug->SetString(wss.str());

				auto strings = m_is->GetDisplayBuffer()->GetBuffer();
				auto length = int(m_is->GetDisplayBufferSize());

				auto i = (length < m_visible_rows) ? 0 : length - m_visible_rows;
				auto d = (length < m_visible_rows) ? length : i + m_visible_rows;
				auto j = (length < m_visible_rows) ? m_visible_rows - d : 0;

				if (length < m_visible_rows)
				{
					while (i < d)
					{
						m_strings[j]->SetString(strings[i]);
						m_strings[j]->SetPosition(m_destination.x, m_destination.y + (m_font_height * j));
						++i; ++j;
					}
				}
				else
				{
					while (i < d)
					{
						m_strings[j]->SetString(strings[i]);
						m_strings[j]->SetPosition(m_destination.x, m_destination.y + (m_font_height * j));
						++i; ++j;
					}
				}
			}
		};
		using StringDisplayBoxPtr = std::unique_ptr<StringDisplayBox>;

		/// <summary>
		/// A basic UI panel that can have a custom frame, title bar, status bar, and hold other UI elements
		/// Filtering uses the following bit flags to determine how to affect a border click for resize:
		/// </summary>
		class BasicPanel
			: public ClayEngine::SimpleMath::Extensions::DestinationExtension
			, public ClayEngine::TimingSystemExtension
			, public ClayEngine::Interfaces::IUpdate
			, public ClayEngine::Platform::InputSystemExtension
		{

			class PanelFrameElement
				: public ClayEngine::SimpleMath::Extensions::DestinationExtension
				, public ClayEngine::TimingSystemExtension
			{
				SpritePtr m_sprite = nullptr;

				UINT m_mask = 0x00;
				// 0x0001 top
				// 0x0002 right
				// 0x0004 bottom
				// 0x0008 left
				// 0x0010 horizontal scale
				// 0x0020 vertical scale

			public:
				PanelFrameElement(String texture, Rectangle source, Vector4 color, UINT mask)
				{
					m_mask = mask;

					m_sprite = std::make_unique<Sprite>(texture, source, color);
				}
				//PanelFrameElement(PanelFrameElement&&) = default;
				//PanelFrameElement& operator=(PanelFrameElement&&) = default;
				//PanelFrameElement(PanelFrameElement const&) = delete;
				//PanelFrameElement& operator=(PanelFrameElement const&) = delete;
				~PanelFrameElement()
				{
					if (m_sprite) m_sprite.reset();
					m_sprite = nullptr;
				}

				//UINT GetMask() { return m_mask; }
				//void SetMask(UINT mask) { m_mask = mask; }

				void OnLayoutChanged()
				{
					m_sprite->SetDestination(m_destination);
				}
			};
			using PanelFrameElementPtr = std::unique_ptr<PanelFrameElement>;
			using PanelFrameElements = std::array<PanelFrameElementPtr, c_panel_frame_element_count>;

			PanelFrameElements m_elements = {}; // clockwise 0 = top left, 7 = left
			long m_frame_size = 6L; // (Border width)

			TitleBarPtr m_titlebar = nullptr;
			long m_titlebar_size = 32L;

			StringDisplayBoxPtr m_chatbox = nullptr;

			InputBarPtr m_inputbar = nullptr;
			long m_inputbar_size = 28L; // Consolas_24 = 36L, Consolas_16 = 28L

			// BasicPanel variables
			UINT m_hit_mask = 0x00;
			bool m_panel_hit = false;

			// BasicPanel interactions
			Mouse::State m_lmb_state;
			Mouse::ButtonStateTracker m_lmb_tracker;
			Vector2 m_lmb_drag = {};

		public:
			BasicPanel()
			{
				auto s = WindowSystem::GetWindowSize();
				auto x = s.left;
				auto y = s.top;
				auto width = s.right - s.left;
				auto height = s.bottom - s.top;

				m_destination = Rectangle{ x + 15, height - 375, width - 30, 360 };

				m_elements[0] = std::make_unique<PanelFrameElement>("GoldBox", Rectangle{ 0, 0, 54, 55 }, Vector4{ 1.f, 1.f, 1.f, 1.0f }, 0x09);
				m_elements[1] = std::make_unique<PanelFrameElement>("GoldBox", Rectangle{ 54, 0, 404, 55 }, Vector4{ 1.f, 1.f, 1.f, 1.0f }, 0x11);
				m_elements[2] = std::make_unique<PanelFrameElement>("GoldBox", Rectangle{ 458, 0, 54, 55 }, Vector4{ 1.f, 1.f, 1.f, 1.0f }, 0x03);
				m_elements[3] = std::make_unique<PanelFrameElement>("GoldBox", Rectangle{ 458, 55, 54, 403 }, Vector4{ 1.f, 1.f, 1.f, 1.0f }, 0x22);
				m_elements[4] = std::make_unique<PanelFrameElement>("GoldBox", Rectangle{ 458, 458, 54, 54 }, Vector4{ 1.f, 1.f, 1.f, 1.0f }, 0x06);
				m_elements[5] = std::make_unique<PanelFrameElement>("GoldBox", Rectangle{ 54, 458, 404, 54 }, Vector4{ 1.f, 1.f, 1.f, 1.0f }, 0x14);
				m_elements[6] = std::make_unique<PanelFrameElement>("GoldBox", Rectangle{ 0, 458, 54, 54 }, Vector4{ 1.f, 1.f, 1.f, 1.0f }, 0x0C);
				m_elements[7] = std::make_unique<PanelFrameElement>("GoldBox", Rectangle{ 0, 55, 54, 403 }, Vector4{ 1.f, 1.f, 1.f, 1.0f }, 0x28);

				m_titlebar = std::make_unique<TitleBar>(L"ClayEngine Sensorium");
				m_chatbox = std::make_unique<StringDisplayBox>();
				m_inputbar = std::make_unique<InputBar>();

				ticker->AddUpdateCallback([&](float elapsedTime) { Update(elapsedTime); });

				OnLayoutChanged();
			}
			~BasicPanel()
			{
				if (m_inputbar) m_inputbar.reset();
				m_inputbar = nullptr;

				if (m_chatbox) m_chatbox.reset();
				m_chatbox = nullptr;

				if (m_titlebar) m_titlebar.reset();
				m_titlebar = nullptr;

				std::for_each(m_elements.begin(), m_elements.end(), [&](auto& element) { element.reset(); });
			}

			void OnLayoutChanged()
			{
				auto innerbox = Rectangle{ m_destination.x + m_frame_size, m_destination.y + m_frame_size + m_titlebar_size,
					m_destination.width - (m_frame_size * 2), m_destination.height - (m_frame_size * 2) - m_titlebar_size - m_inputbar_size };

				m_elements[0]->SetDestination(m_destination.x, m_destination.y, m_frame_size, m_frame_size); // Top Left
				m_elements[1]->SetDestination(m_destination.x + m_frame_size, m_destination.y, m_destination.width - (m_frame_size * 2), m_frame_size); // Top
				m_elements[2]->SetDestination(m_destination.x + m_destination.width - m_frame_size, m_destination.y, m_frame_size, m_frame_size); // Top Right
				m_elements[3]->SetDestination(m_destination.x + m_destination.width - m_frame_size, m_destination.y + m_frame_size, m_frame_size, m_destination.height - (m_frame_size * 2)); // Right
				m_elements[4]->SetDestination(m_destination.x + m_destination.width - m_frame_size, m_destination.y + m_destination.height - m_frame_size, m_frame_size, m_frame_size); // Bottom Right
				m_elements[5]->SetDestination(m_destination.x + m_frame_size, m_destination.y + m_destination.height - m_frame_size, m_destination.width - (m_frame_size * 2), m_frame_size); // Bottom
				m_elements[6]->SetDestination(m_destination.x, m_destination.y + m_destination.height - m_frame_size, m_frame_size, m_frame_size); // Bottom Left
				m_elements[7]->SetDestination(m_destination.x, m_destination.y + m_frame_size, m_frame_size, m_destination.height - (m_frame_size * 2)); // Left

				m_titlebar->SetDestination(innerbox.x, m_destination.y + m_frame_size, innerbox.width, m_titlebar_size);
				m_chatbox->SetDestination(innerbox);
				m_inputbar->SetDestination(innerbox.x, innerbox.y + innerbox.height, innerbox.width, m_inputbar_size);

				std::for_each(m_elements.begin(), m_elements.end(), [&](auto& element) { element->OnLayoutChanged(); });

				m_titlebar->OnLayoutChanged();
				m_chatbox->OnLayoutChanged();
				m_inputbar->OnLayoutChanged();
			}

			void Update(float elapsedTime) override
			{
				m_lmb_state = m_is->GetMouseState();

				// On LMB Down
				if (m_lmb_state.leftButton && !m_lmb_tracker.leftButton)
				{
					if (m_destination.Contains(Vector2{ float(m_lmb_state.x), float(m_lmb_state.y) }))
					{
						m_hit_mask = 0x00;
						m_panel_hit = true;

						if (m_lmb_state.y >= m_destination.y && m_lmb_state.y <= m_destination.y + m_frame_size)
							m_hit_mask |= 0x01; // Top
						if (m_lmb_state.x >= (m_destination.x + m_destination.width - m_frame_size) && m_lmb_state.x <= m_destination.x + m_destination.width)
							m_hit_mask |= 0x02; // Right
						if (m_lmb_state.y >= (m_destination.y + m_destination.height - m_frame_size) && m_lmb_state.y <= m_destination.y + m_destination.height)
							m_hit_mask |= 0x04; // Bottom
						if (m_lmb_state.x >= m_destination.x && m_lmb_state.x <= m_destination.x + m_frame_size)
							m_hit_mask |= 0x08; // Left
					}
				}

				// LMB held down & dragging
				if (m_lmb_state.leftButton && m_lmb_tracker.leftButton)
				{
					if (m_panel_hit)
					{
						Vector2 lmb_drag = { float(m_lmb_state.x - m_lmb_tracker.GetLastState().x), float(m_lmb_state.y - m_lmb_tracker.GetLastState().y) };

						if (0x01 & m_hit_mask) // Top is clicked
						{
							m_destination.y += long(lmb_drag.y);
							m_destination.height -= long(lmb_drag.y);
						}
						if (0x02 & m_hit_mask) // Right is clicked
						{
							m_destination.width += long(lmb_drag.x);
						}
						if (0x04 & m_hit_mask) // Bottom is clicked
						{
							m_destination.height += long(lmb_drag.y);
						}
						if (0x08 & m_hit_mask) // Left is clicked
						{
							m_destination.x += long(lmb_drag.x);
							m_destination.width -= long(lmb_drag.x);
						}
						if (!m_hit_mask) // Contains is clicked
						{
							m_destination.x += long(lmb_drag.x);
							m_destination.y += long(lmb_drag.y);
						}

						OnLayoutChanged();
					}
				}

				// On LMB Up
				if (!m_lmb_state.leftButton && m_lmb_tracker.leftButton)
				{
					m_panel_hit = false;
					m_hit_mask = 0x00;
				}

				m_lmb_tracker.Update(m_lmb_state);
			}
		};
		using BasicPanelPtr = std::unique_ptr<BasicPanel>;
		using BasicPanelRaw = BasicPanel*;

		/// <summary>
		/// A display in the corner showing the current Frames Per Second
		/// </summary>
		class FramesPerSecond
			: public ClayEngine::Interfaces::IUpdate
			, public ClayEngine::TimingSystemExtension
		{
			SpriteStringPtr m_string = nullptr;

		public:
			FramesPerSecond()
			{
				m_string = std::make_unique<SpriteString>("Consolas_16");
				m_string->SetString(L"FPS: ");
				m_string->SetPosition(Vector2{ float(WindowSystem::GetWindowWidth() - 115), 4.F });
				m_string->SetRGBA(Vector4{ 1.F, 0.F, 0.F, 1.F });

				ticker->AddUpdateCallback([&](float elapsedTime) { Update(elapsedTime); });
			}
			~FramesPerSecond()
			{
				if (m_string) m_string.reset();
				m_string = nullptr;
			}

			void Update(float elapsedTime) override
			{
				auto fps = ticker->GetFPS();

				std::wstringstream wss;
				wss << L"FPS: " << fps;

				m_string->SetString(wss.str());
			}
		};
		using FramesPerSecondPtr = std::unique_ptr<FramesPerSecond>;


		//template<typename ElementType>
		//class SensoriumElement
		//{
		//	using SensoriumElementPtr = std::unique_ptr<SensoriumElement>;
		//	SensoriumElement* m_parent = nullptr;

		//	using SensoriumElements = std::vector<SensoriumElementPtr>;
		//	SensoriumElements m_children = {};

		//	using ElementTypePtr = std::unique_ptr<ElementType>;
		//	ElementTypePtr m_element = nullptr;

		//public:
		//	using Type = ElementType;
		//	//using Element = m_element;

		//	template<typename ElementType, typename... Args>
		//	SensoriumElement(SensoriumElement* parent = nullptr, Args&&... args)
		//	{
		//		m_parent = parent;
		//		//m_element = element;
		//		m_element = std::make_unique<ElementType>(std::forward<Args>(args)...);
		//	}
		//	~SensoriumElement()
		//	{

		//	}

		//	const SensoriumElement* MakeChild()
		//	{
		//		m_children.emplace_back(std::make_unique<SensoriumElement>(this));
		//		return(m_children.cend() - 1)->get();
		//	}
		//	const SensoriumElements& GetChildren()
		//	{
		//		return m_children;
		//	}
		//};
		//using SensoriumElementPtr = std::unique_ptr<SensoriumElement>;
		//using SensoriumChildren = std::vector<SensoriumElement*>;


		/// <summary>
		/// Temporary class for prototyping various UI elements
		/// </summary>
		class Sensorium
		{
			BasicPanelPtr m_basicpanel = nullptr;
			FramesPerSecondPtr m_fps = nullptr;

			//SensoriumElementPtr m_tree = nullptr;

		public:
			Sensorium()
			{
				//m_tree = std::make_unique<SensoriumElement>();

				//auto child = m_tree->MakeChild();

				m_fps = std::make_unique<FramesPerSecond>();
				m_basicpanel = std::make_unique<BasicPanel>();

			}
			~Sensorium()
			{
				if (m_basicpanel) m_basicpanel.reset();
				m_basicpanel = nullptr;

				if (m_fps) m_fps.reset();
				m_fps = nullptr;
			}
		};
		using SensoriumPtr = std::unique_ptr<Sensorium>;


















	}
}


//class IUpdateExtension : IUpdate
//{
//public:
//	IUpdateExtension()
//	{
//		Services::GetService<TimingSystem>()->AddUpdateCallback([&](float elapsedTime) { Update(elapsedTime); });
//	}
//};
