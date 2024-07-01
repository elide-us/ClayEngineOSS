#pragma once
/******************************************************************************/
/*                                                                            */
/* ClayEngine Interface and Extension Library (C) 2022 Epoch Meridian, LLC.   */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

namespace ClayEngine
{
	namespace Interfaces
	{
		struct ILayoutChanged
		{
			virtual void OnLayoutChanged(float x, float y, float width, float height) = 0;
		};

		struct IUpdate
		{
			virtual void Update(float elapsedTime) = 0;
		};

		struct IDraw
		{
			virtual void Draw() = 0;
		};
	}
}
