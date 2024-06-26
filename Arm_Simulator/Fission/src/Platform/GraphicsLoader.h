#pragma once
#include <Fission/Core/Graphics.hh>

namespace Fission
{
	struct GraphicsLoader : public IFObject
	{
		virtual void Initialize() = 0;

		virtual void CreateGraphics( GraphicsState * pState, IFGraphics ** ppGraphics ) = 0;

		// TODO: make a function to give the reason why an API is not supported.
		virtual bool CheckSupport( IFGraphics::API api ) = 0;
	};

	extern void CreateGraphicsLoader( GraphicsLoader ** ppGraphicsLoader );
}