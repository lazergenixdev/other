/**
* 
* @file: Engine.h
* @author: lazergenixdev@gmail.com
* 
* 
* This file is provided under the MIT License:
* 
* Copyright (c) 2021 Lazergenix Software
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/
#pragma once
#include <Fission/Core/Graphics/Renderer.hh>
#include <Fission/Core/Scene.hh>
#include <Fission/Base/Version.hpp>

namespace Fission
{
	struct IFEngine : public IFObject
	{

		//! @brief Function that contains the main game loop, the
		//!	       application is expected to terminate after this function returns.
		virtual void Run( Platform::ExitCode * ) = 0;


		//! @brief Starts a shutdown of the engine, resulting in the application closing.
		virtual void Shutdown( Platform::ExitCode ) = 0;


		/**
		 * @brief Loads all the reasources need to run our application,
		 *          this includes: IFGraphics* and IFWindow*, and also giving 
		 *          the application a reference to the Engine instance.
		 * 
		 * @note This function can only be called once after the engine is created,
		 *			subsequent calls will trigger an exception and will have no effect.
		 */
		virtual void LoadApplication( class FApplication * app ) = 0;


		/**
		* @brief Register a render to be used with the engine.
		*        (can be retrieved using @GetRenderer)
		* 
		* @note Renderers are managed by the engine using the IFRenderer interface.
		*/
		virtual void RegisterRenderer( const char * _Name, IFRenderer * _Renderer ) = 0;


		/**
		 * @brief  Get a renderer from it's name.
		 *
		 * @param  _Name: Name of the renderer you wish to retrieve;
		 *                "$internal2D" | type: IFRenderer2D | Engine's internal 2D renderer.
		 */
		virtual IFRenderer * GetRenderer( const char * _Name ) = 0;

		template <class RendererType>
		inline RendererType * GetRenderer( const char * _Name )
		{
			return dynamic_cast<RendererType*>( GetRenderer( _Name ) );
		}



		virtual IFDebugLayer * GetDebug() = 0;


		// create a new scene and set switch to that scene.
		virtual void EnterScene( const SceneKey& key ) = 0;


		// go to the previous scene in history.
		virtual void ExitScene() = 0;


		virtual void ClearSceneHistory() = 0;


		//! @brief  Get Fission Engine Version.
		virtual version GetVersion() = 0;


		//! @brief Gets version in format: "Fission vX.Y.Z"
		virtual const char * GetVersionString() = 0;


	}; // struct Fission::IFEngine

	FISSION_API void CreateEngine( void * instance, IFEngine ** ppEngine );

} // namespace Fission