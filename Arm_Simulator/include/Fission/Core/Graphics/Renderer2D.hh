/**
*
* @file: Renderer2D.h
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

// todo: MESHES SUCK! improve interface

#pragma once
#include "Fission/Core/Graphics.hh"
#include "Fission/Core/Graphics/Renderer.hh"
#include "Fission/Core/Graphics/Font.hh"
#include "Fission/Base/Math/Matrix.hpp"
#include "Fission/Base/Rect.hpp"

namespace Fission
{
	enum class StrokeStyle {
		Center, Inside, Outside,
		Default = Center,
	};

	struct TextLayout {
		float width, height;
	};

	enum class BlendMode {
		Normal, Add, Source,
		Sub, Div, Mul,
		__count__
	};

	class Mesh 
	{
	public:
		FISSION_API Mesh( int vertex_count, int index_count, int color_count = 1 );
		FISSION_API Mesh( const Mesh & );
		FISSION_API ~Mesh();

		FISSION_API void push_color( color col );
		FISSION_API void push_vertex( v2f32 position, int color_index );
		FISSION_API void push_index( uint32_t index );

		FISSION_API void set_color( uint32_t index, color new_color );

		FISSION_API uint32_t vertex_count() const;
		FISSION_API uint32_t index_count() const;
		FISSION_API uint32_t color_count() const;

	public:
		struct MeshData * m_Data;
	};

	struct IFRenderer2D : public IFRenderer
	{
	public:

		virtual void FillRect( rf32 rect, color color ) = 0;

		virtual void DrawRect( rf32 rect, color color, float stroke_width, StrokeStyle stroke = StrokeStyle::Default ) = 0;

		virtual void FillTriangle( v2f32 p0, v2f32 p1, v2f32 p2, color color ) = 0;

		virtual void FillTriangleGrad( v2f32 p0, v2f32 p1, v2f32 p2, color c0, color c1, color c2 ) = 0;

		virtual void FillTriangleUV( v2f32 p0, v2f32 p1, v2f32 p2, v2f32 uv0, v2f32 uv1, v2f32 uv2, Resource::IFTexture2D * pTexture, color tint = colors::White ) = 0;

		virtual void FillRectGrad( rf32 rect, color color_topleft, color color_topright, color color_bottomleft, color color_bottomright ) = 0;

		virtual void FillRoundRect( rf32 rect, float rad, color color ) = 0;

		virtual void DrawRoundRect( rf32 rect, float rad, color color, float stroke_width, StrokeStyle stroke = StrokeStyle::Default ) = 0;

		virtual void DrawLine( v2f32 start, v2f32 end, color color, float stroke_width = 1.0f, StrokeStyle stroke = StrokeStyle::Default ) = 0;

		virtual void FillCircle( v2f32 point, float radius, color color ) = 0;

		virtual void DrawCircle( v2f32 point, float radius, color color, float stroke_width, StrokeStyle stroke = StrokeStyle::Default ) = 0;

		virtual void DrawCircle( v2f32 point, float radius, color inner_color, color outer_color, float stroke_width, StrokeStyle stroke = StrokeStyle::Default ) = 0;

		virtual void FillArrow( v2f32 start, v2f32 end, float width, color color ) = 0;

		virtual void DrawImage( Resource::IFTexture2D * pTexture, rf32 rect, rf32 uv, color tint = colors::White ) = 0;

		virtual void DrawImage( Resource::IFTexture2D * pTexture, rf32 rect, color tint = colors::White ) = 0;

		virtual void DrawMesh( const Mesh * m ) = 0;

		virtual void SelectFont( const Font * pFont ) = 0;

		virtual TextLayout DrawString( const char * str, v2f32 pos, color color ) = 0;

		virtual TextLayout DrawString( string_view sv, v2f32 pos, color color ) = 0;

		virtual TextLayout CreateTextLayout( const char * str ) = 0;

		virtual TextLayout CreateTextLayout( const char * str, uint32_t length ) = 0;

		virtual void SetBlendMode( BlendMode mode ) = 0;

		virtual void PushTransform( m23 const& transform ) = 0;

		virtual void PopTransform() = 0;

		virtual void Render() = 0;

	}; // class Fission::Renderer2D

	FISSION_API void CreateRenderer2D( IFRenderer2D ** ppRenderer2D );

} // namespace Fission
