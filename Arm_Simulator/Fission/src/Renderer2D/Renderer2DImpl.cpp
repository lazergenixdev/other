﻿#include "Renderer2DImpl.h"
#include "Mesh.h"
#include <numbers>
#include <Fission/Base/Exception.hpp>

// might look into text snapping in more detail,
//    but for now it looks better than not and there
//    are no texture artifacts when snapping.
#define __SNAP_TEXT 1
#define _ROUND(_VAL) ((float)(int)((_VAL) + 0.5f))

namespace Fission {

	void CreateRenderer2D( IFRenderer2D ** ppRenderer2D )
	{
		*ppRenderer2D = new Renderer2DImpl;
	}

	std::vector<Renderer2DImpl::DrawData::sincos> Renderer2DImpl::DrawData::TrigCache =
	[](){
		using sincos = Renderer2DImpl::DrawData::sincos;
		std::vector<sincos> out;

		int geometry_persision = 11;

		// restrict the amount of persision to a reasonable range
		FISSION_ASSERT( geometry_persision >= 1 && geometry_persision < 100, "" );

		out.reserve( geometry_persision );
		static constexpr float quarter_rotation = ( std::numbers::pi_v<float> / 2.0f );
		const float n = (float)( geometry_persision + 1 );

		// Calculate sin/cos to cache for later render commands
		for( int i = 0; i < geometry_persision; i++ ) {
			float x = (float)( i + 1 ) * quarter_rotation / n;
			out.emplace_back( sinf( x ), cosf( x ) );
		}

		return out;
	}();

	void Renderer2DImpl::OnCreate( IFGraphics * gfx, size2 _Viewport_Size )
	{
		// Allocate aligned memory for faster access
		vertex_data = (vertex *)_aligned_malloc( vertex_max_count * sizeof vertex, 32 );
		index_data = (uint32_t *)_aligned_malloc( index_max_count * sizeof uint32_t, 32 );

		_viewport_size = _Viewport_Size;

		OnRecreate( gfx );
	}

	void Renderer2DImpl::OnRecreate( IFGraphics * gfx )
	{
		m_pGraphics = gfx;
		using namespace Resource;
		using namespace Resource::VertexLayoutTypes;

		auto vl = VertexLayout{};
		vl.Append( Float2, "Position" );
		vl.Append( Float2, "TexCoord" );
		vl.Append( Float4, "Color" );

		{ // Create Vertex Buffer
			IFVertexBuffer::CreateInfo info;
			info.vtxCount = vertex_max_count;
			info.pVertexLayout = &vl;
			info.type = IFVertexBuffer::Type::Dynamic;
			m_pVertexBuffer = gfx->CreateVertexBuffer( info );
		}
		{ // Create Index Buffer
			IFIndexBuffer::CreateInfo info;
			info.idxCount = index_max_count;
			info.size = IFIndexBuffer::Size::UInt32;
			info.type = IFIndexBuffer::Type::Dynamic;
			m_pIndexBuffer = gfx->CreateIndexBuffer( info );
		}
		{ // Create Index Buffer
			IFConstantBuffer::CreateInfo info;
			info.type = IFConstantBuffer::Type::Dynamic;
			info.max_size = 128;
			m_pTransformBuffer = gfx->CreateConstantBuffer( info );

			const auto res = v2f32{ (float)_viewport_size.w, (float)_viewport_size.h };

			const auto screen = m44(
				2.0f / res.x,	0.0f,		   -1.0f, 0.0f,
				0.0f,		   -2.0f / res.y,	1.0f, 0.0f,
				0.0f,			0.0f,			1.0f, 0.0f,
				0.0f,			0.0f,			0.0f, 1.0f
			).transpose();

			m_pTransformBuffer->SetData( &screen, sizeof(screen) );
		}
		{ // Create Shaders
			IFShader::CreateInfo info;
			info.pVertexLayout = &vl;
			info.sourceCode = R"(
cbuffer Transform : register(b0)
{
	matrix screen;
}

struct VS_OUT { 
	float2 tc : TexCoord; 
	float4 color : Color; 
	float4 pos : SV_Position; 
};
VS_OUT vs_main( float2 pos : Position, float2 tc : TexCoord, float4 color : Color ) { 
	VS_OUT vso; 
	vso.pos = mul( float4( pos, 1.0, 1.0 ), screen ); 
	vso.tc = tc; 
	vso.color = color;
	return vso; 
}
Texture2D tex;
SamplerState ss;
float4 ps_main( float2 tc : TexCoord, float4 color : Color ) : SV_Target { 
	if( tc.x < -0.5f ) return color;
	return tex.Sample( ss, tc ) * color;
}
			)";
			m_pShader = gfx->CreateShader( info );
		}
		{ // todo: more blenders
			IFBlender::CreateInfo info;

			info.blend = IFBlender::Blend::Normal;
			m_pBlenders[(int)BlendMode::Normal] = gfx->CreateBlender( info );

			info.blend = IFBlender::Blend::Add;
			m_pBlenders[(int)BlendMode::Add] = gfx->CreateBlender( info );

			m_pUseBlender = m_pBlenders[(int)BlendMode::Normal].get();
		}

		m_DrawBuffer.reserve( 20 );
		m_DrawBuffer.emplace_back( this, 0u, 0u );
	}

	void Renderer2DImpl::Destroy()
	{
		_aligned_free( vertex_data );
		_aligned_free( index_data );
		delete this;
	}

	void Renderer2DImpl::OnResize( IFGraphics * , size2 size )
	{
		const auto screen = m44(
			2.0f / (float)size.w, 0.0f,                -1.0f, 0.0f,
			0.0f,                -2.0f / (float)size.h, 1.0f, 0.0f,
			0.0f,                 0.0f,                 1.0f, 0.0f,
			0.0f,                 0.0f,                 0.0f, 1.0f
		).transpose();

		m_pTransformBuffer->SetData( &screen, sizeof( screen ) );
	}

	void Renderer2DImpl::Render()
	{
		if( m_DrawBuffer.size() == 1u && m_DrawBuffer[0].vtxCount == 0u ) return;

		auto & end = m_DrawBuffer.back();

		m_pShader->Bind();
		m_pVertexBuffer->SetData( vertex_data, end.vtxCount + end.vtxStart );
		m_pVertexBuffer->Bind();
		m_pIndexBuffer->SetData( index_data, end.idxCount + end.idxStart );
		m_pIndexBuffer->Bind();
		m_pTransformBuffer->Bind(Resource::IFConstantBuffer::Target::Vertex,0);
		m_pUseBlender->Bind();

		for( auto && cmd : m_DrawBuffer )
		{
			if( cmd.Texture ) cmd.Texture->Bind(0);
			m_pGraphics->DrawIndexed( cmd.idxCount, cmd.idxStart, cmd.vtxStart );
		}

		m_DrawBuffer.clear();
		m_DrawBuffer.emplace_back( this, 0u, 0u );

	}

	void Renderer2DImpl::FillTriangle( v2f32 p0, v2f32 p1, v2f32 p2, color color )
	{
		m_DrawBuffer.back().AddTriangle( p0, p1, p2, color, color, color );
	}

	void Renderer2DImpl::FillTriangleGrad( v2f32 p0, v2f32 p1, v2f32 p2, color c0, color c1, color c2 )
	{
		m_DrawBuffer.back().AddTriangle( p0, p1, p2, c0, c1, c2 );
	}

	void Renderer2DImpl::FillTriangleUV( v2f32 p0, v2f32 p1, v2f32 p2, v2f32 uv0, v2f32 uv1, v2f32 uv2, Resource::IFTexture2D * pTexture, color tint )
	{
		SetTexture( pTexture );
		m_DrawBuffer.back().AddTriangleUV( p0, p1, p2, uv0, uv1, uv2, tint );
	}

	void Renderer2DImpl::FillRect( rf32 rect, color color )
	{
		m_DrawBuffer.back().AddRectFilled( rect, color );
	}

	void Renderer2DImpl::DrawRect( rf32 rect, color color, float stroke_width, StrokeStyle stroke )
	{
		m_DrawBuffer.back().AddRect( rect, color, stroke_width, stroke );
	}

	void Renderer2DImpl::FillRectGrad( rf32 rect, color color_topleft, color color_topright, color color_bottomleft, color color_bottomright )
	{
		m_DrawBuffer.back().AddRectFilled( rect, color_topleft, color_topright, color_bottomleft, color_bottomright );
	}

	void Renderer2DImpl::FillRoundRect( rf32 rect, float rad, color color )
	{
		m_DrawBuffer.back().AddRoundRectFilled( rect, rad, color );
	}

	void Renderer2DImpl::DrawRoundRect( rf32 rect, float rad, color color, float stroke_width, StrokeStyle stroke )
	{
		m_DrawBuffer.back().AddRoundRect( rect, rad, color, stroke_width, stroke );
	}

	void Renderer2DImpl::DrawLine( v2f32 start, v2f32 end, color color, float stroke_width, StrokeStyle stroke )
	{
		(void)stroke;
		m_DrawBuffer.back().AddLine( start, end, stroke_width, color, color );
	}

	void Renderer2DImpl::FillCircle( v2f32 point, float radius, color color )
	{
		m_DrawBuffer.back().AddCircleFilled( point, radius, color );
	}

	void Renderer2DImpl::DrawCircle( v2f32 point, float radius, color color, float stroke_width, StrokeStyle stroke )
	{
		m_DrawBuffer.back().AddCircle( point, radius, color, color, stroke_width, stroke );
	}

	void Renderer2DImpl::DrawCircle( v2f32 point, float radius, color inner_color, color outer_color, float stroke_width, StrokeStyle stroke )
	{
		m_DrawBuffer.back().AddCircle( point, radius, inner_color, outer_color, stroke_width, stroke );
	}

	void Renderer2DImpl::FillArrow( v2f32 start, v2f32 end, float width, color c )
	{
		if( start == end )
		{
			m_DrawBuffer.back().AddCircleFilled( start, width * 0.2f, c );
			return;
		}

		v2f32 diff = start - end;
		v2f32 par = diff.norm();
		float lensq = diff.lensq();

		if( lensq < width * width )
		{
			const v2f32 center = start;
			const v2f32 perp = par.perp() * ( sqrtf( lensq ) * 0.5f );
			const v2f32 l = center - perp, r = center + perp;

			m_DrawBuffer.back().AddTriangle( end, l, r, c, c, c );
		}
		else
		{
			const v2f32 perp = par.perp() * ( width * 0.5f );
			const v2f32 center = end + par * width;
			const v2f32 l = center - perp, r = center + perp;

			m_DrawBuffer.back().AddTriangle( end, l, r, c, c, c );
			m_DrawBuffer.back().AddLine( start, center, 0.4f * width, c, c );
		}

	}

	void Renderer2DImpl::DrawImage( Resource::IFTexture2D * pTexture, rf32 rect, rf32 uv, color tint )
	{
		SetTexture( pTexture );
		m_DrawBuffer.back().AddRectFilledUV( rect, uv, tint );
	}

	void Renderer2DImpl::DrawImage( Resource::IFTexture2D * pTexture, rf32 rect, color tint )
	{
		SetTexture( pTexture );
		m_DrawBuffer.back().AddRectFilledUV( rect, { 0.0f, 1.0f, 0.0f, 1.0f }, tint );
	}

	void Renderer2DImpl::DrawMesh( const Mesh * m )
	{
		m_DrawBuffer.back().AddMesh( m );
	}

	void Renderer2DImpl::SelectFont( const Font * pFont )
	{
		m_pSelectedFont = pFont;
	}

	TextLayout Renderer2DImpl::DrawString( const char * str, v2f32 pos, color c )
	{
		FISSION_ASSERT( m_pSelectedFont, "you're not supposed to do that." );

		SetTexture( m_pSelectedFont->GetTexture2D() );

		float start = 0.0f;
		const Font::Glyph * glyph;

		while( str[0] != '\0' )
		{
			if( *str == '\r' || *str == '\n' ) { str++; pos.y += m_pSelectedFont->GetSize(); start = 0.0f; continue; }
			glyph = m_pSelectedFont->GetGylph( (wchar_t)*str );

#if __SNAP_TEXT
			const auto left = _ROUND( pos.x + glyph->offset.x + start );
			const auto right = _ROUND( left + glyph->size.x );
			const auto top = _ROUND( pos.y + glyph->offset.y );
			const auto bottom = _ROUND( top + glyph->size.y );
#else
			const auto left = pos.x + glyph->offset.x + start;
			const auto right = left + glyph->size.x;
			const auto top = pos.y + glyph->offset.y;
			const auto bottom = top + glyph->size.y;
#endif

			if( *str != L' ' )
			m_DrawBuffer.back().AddRectFilledUV( { left, right, top, bottom }, glyph->rc, c );

			start += glyph->advance;

			str++;
		}

		return TextLayout{ start, (float)m_pSelectedFont->GetSize() };
	}

	TextLayout Renderer2DImpl::DrawString( string_view sv, v2f32 pos, color color )
	{
		FISSION_ASSERT( m_pSelectedFont, "you're not supposed to do that." );

		SetTexture( m_pSelectedFont->GetTexture2D() );

		float start = 0.0f;
		const Font::Glyph* glyph;

		size_t p = 0;

		while( p < sv.size() )
		{
			const auto& ch = sv.c_str()[p];
			if( ch == L'\r' || ch == L'\n' ) { ++p, pos.y += m_pSelectedFont->GetSize(); start = 0.0f; continue; }
			glyph = m_pSelectedFont->GetGylph( ch );

#if __SNAP_TEXT
			const auto left = _ROUND( pos.x + glyph->offset.x + start );
			const auto right = _ROUND( left + glyph->size.x );
			const auto top = _ROUND( pos.y + glyph->offset.y );
			const auto bottom = _ROUND( top + glyph->size.y );
#else
			const auto left = pos.x + glyph->offset.x + start;
			const auto right = left + glyph->size.x;
			const auto top = pos.y + glyph->offset.y;
			const auto bottom = top + glyph->size.y;
#endif
			if( ch != L' ' )
				m_DrawBuffer.back().AddRectFilledUV( { left, right, top, bottom }, glyph->rc, color );

			start += glyph->advance;

			++p;
		}

		return TextLayout{ start, (float)m_pSelectedFont->GetSize() };
	}

	TextLayout Renderer2DImpl::CreateTextLayout( const char * str )
	{
		FISSION_ASSERT( m_pSelectedFont, "you're not supposed to do that." );

		TextLayout out{ 0.0f,(float)m_pSelectedFont->GetSize() };

		while( str[0] != '\0' )
		{
			const Font::Glyph * g = m_pSelectedFont->GetGylph( (wchar_t)*str );
			out.width += g->advance;
			str++;
		}

		return out;
	}

	TextLayout Renderer2DImpl::CreateTextLayout( const char * str, uint32_t length )
	{
		FISSION_ASSERT( m_pSelectedFont, "you're not supposed to do that." );

		TextLayout out{ 0.0f,(float)m_pSelectedFont->GetSize() };

		uint32_t pos = 0;
		while( pos < length )
		{
			const Font::Glyph * g = m_pSelectedFont->GetGylph( (wchar_t)str[pos] );
			out.width += g->advance;
			pos++;
		}

		return out;
	}

	void Renderer2DImpl::SetBlendMode( BlendMode mode )
	{
		m_pUseBlender = m_pBlenders[(int)mode].get();
	}

	void Renderer2DImpl::PushTransform( m23 const& transform )
	{
		m_TransformStack.emplace_back( transform );
		_set_accumulated_transform();
	}

	void Renderer2DImpl::PopTransform()
	{
		FISSION_ASSERT( !m_TransformStack.empty(), "No Transforms were left to be poped" );
		m_TransformStack.pop_back();
		_set_accumulated_transform();
	}

	void Renderer2DImpl::_set_accumulated_transform()
	{
		if( m_TransformStack.empty() ) {
			m_accTransform = m23::Identity();
			return;
		}

		m_accTransform = m_TransformStack.front();

		for( int i = 1; i < m_TransformStack.size(); i++ )
			m_accTransform = m_TransformStack[i] * m_accTransform;
	}

	void Renderer2DImpl::SetTexture( Resource::IFTexture2D * tex )
	{
		FISSION_ASSERT( tex, "Cannot bind a null texture. (Use non-UV functions to not use a texture)" );

		auto & end = m_DrawBuffer.back();
		// There is no texture set in this draw call, use it directly.
		if( end.Texture == nullptr ) end.Texture = tex;
		// The texture we are using in this draw call is different, so we make another draw call.
		else if( end.Texture != tex )
		{
			m_DrawBuffer.emplace_back( this, end.vtxStart + end.vtxCount, end.idxStart + end.idxCount );
			m_DrawBuffer.back().Texture = tex;
		}
	}

	Renderer2DImpl::DrawData::DrawData( Renderer2DImpl * parent, uint32_t vc, uint32_t ic )
		: vtxStart( vc ), idxStart( ic ),
		pVtxData( parent->vertex_data + vtxStart ), pIdxData( parent->index_data + idxStart ),
		mat( &parent->m_accTransform )
	{}

	void Renderer2DImpl::DrawData::AddRect( rf32 rect, color c, float stroke_width, StrokeStyle stroke )
	{
		for( int i = 0; i < 8; i++ ) {
		// I bet you've never seen code like this:
			i & 0x1 ?
			(
				pIdxData[idxCount++] = vtxCount + i,
				pIdxData[idxCount++] = vtxCount + ( i + 1u ) % 8u,
				pIdxData[idxCount++] = vtxCount + ( i + 2u ) % 8u
			):(
				pIdxData[idxCount++] = vtxCount + i,
				pIdxData[idxCount++] = vtxCount + ( i + 2u ) % 8u,
				pIdxData[idxCount++] = vtxCount + ( i + 1u ) % 8u
			)
			;
		}

		float in_l = rect.x.low, out_l = in_l;
		float in_r = rect.x.high, out_r = in_r;
		float in_t = rect.y.low, out_t = in_t;
		float in_b = rect.y.high, out_b = in_b;

		switch( stroke )
		{
		case StrokeStyle::Center:
		{
			float half_stroke = stroke_width / 2.0f;

			in_l += half_stroke, in_t += half_stroke;
			in_r -= half_stroke, in_b -= half_stroke;

			out_l -= half_stroke, out_t -= half_stroke;
			out_r += half_stroke, out_b += half_stroke;
			break;
		}
		case StrokeStyle::Inside:
		{
			stroke_width = std::min( stroke_width, ( out_b - out_t ) / 2.0f );
			stroke_width = std::min( stroke_width, ( out_r - out_l ) / 2.0f );

			in_l += stroke_width, in_t += stroke_width;
			in_r -= stroke_width, in_b -= stroke_width;
			break;
		}
		case StrokeStyle::Outside:
		{
			out_l -= stroke_width, out_t -= stroke_width;
			out_r += stroke_width, out_b += stroke_width;
			break;
		}
		default:throw std::logic_error( "this don't make no fucking sense" );
		}

		pVtxData[vtxCount++] = vertex( *mat * v2f32( out_l, out_b ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( in_l , in_b  ), c );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( out_l, out_t ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( in_l , in_t  ), c );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( out_r, out_t ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( in_r , in_t  ), c );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( out_r, out_b ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( in_r , in_b  ), c );
	}

	void Renderer2DImpl::DrawData::AddRoundRectFilled( rf32 rect, float rad, color c )
	{
		// This is some good fucking code
		const float & left   = rect.x.low;
		const float & right  = rect.x.high;
		const float & top    = rect.y.low;
		const float & bottom = rect.y.high;

		// don't give me that garbage rect and expect something on the screen smh
		if( left == right || top == bottom ) return;

		// WHY DID YOU EVEN CALL fill ROUND rect, YOU DISAPPOINT ME
		if( rad <= 0.0f ) { AddRectFilled( rect, c ); return; }

		// I could probably handle this better
		rad = std::min( rad, ( bottom - top ) / 2.0f );
		rad = std::min( rad, ( right - left ) / 2.0f );

		const float inner_left = left + rad;
		const float inner_right = right - rad;
		const float inner_top = top + rad;
		const float inner_bottom = bottom - rad;

		int v_count = 9u + (int)TrigCache.size() * 4;
		for( auto i = 1; i < v_count - 1; i++ ) {
			pIdxData[idxCount++] = vtxCount;
			pIdxData[idxCount++] = vtxCount + i;
			pIdxData[idxCount++] = vtxCount + i + 1u;
		}
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 1;
		pIdxData[idxCount++] = vtxCount + v_count - 1;

#define _push_vertex(X,Y,C) pVtxData[vtxCount++] = vertex( *mat * v2f32( X, Y ), C )

		// Center
		_push_vertex( ( right + left ) / 2.0f, ( bottom + top ) / 2.0f, c );

		// Bottom Left
		_push_vertex( inner_left, bottom, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_left - TrigCache[i].sin * rad, inner_bottom + TrigCache[i].cos * rad, c );
		_push_vertex( left, inner_bottom, c );

		// Top Left
		_push_vertex( left, inner_top, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_left - TrigCache[i].cos * rad, inner_top - TrigCache[i].sin * rad, c );
		_push_vertex( inner_left, top, c );

		// Top Right
		_push_vertex( inner_right, top, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_right + TrigCache[i].sin * rad, inner_top - TrigCache[i].cos * rad, c );
		_push_vertex( right, inner_top, c );

		// Bottom Right
		_push_vertex( right, inner_bottom, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_right + TrigCache[i].cos * rad, inner_bottom + TrigCache[i].sin * rad, c );
		_push_vertex( inner_right, bottom, c );

#undef _push_vertex
	}
	
	void Renderer2DImpl::DrawData::AddRoundRect( rf32 rect, float rad, color c, float stroke_width, StrokeStyle stroke )
	{
		if( rad <= 0.0f ) { AddRect( rect, c, stroke_width, stroke ); return; }

		rad = std::max( stroke_width, rad );

		switch( stroke )
		{
		case Fission::StrokeStyle::Center:
			rect.expand( -stroke_width / 2.0f );
			break;
		case Fission::StrokeStyle::Inside:
			break;
		case Fission::StrokeStyle::Outside:
			break;
		default:throw std::logic_error((const char*)u8"how? 🤔");
		}

		// same old
		const float & left = rect.x.low;
		const float & right = rect.x.high;
		const float & top = rect.y.low;
		const float & bottom = rect.y.high;
		if( left == right || top == bottom ) return;

		// I could probably handle this better
		//rad = std::min( rad, ( bottom - top ) / 2.0f );
		//rad = std::min( rad, ( right - left ) / 2.0f );

		const float inner_left = left + rad;
		const float inner_right = right - rad;
		const float inner_top = top + rad;
		const float inner_bottom = bottom - rad;

		int v_count0 = 8u + (int)TrigCache.size() * 4;
		for( auto i = 0; i < v_count0 - 1; i++ ) {
			pIdxData[idxCount++] = vtxCount + i;
			pIdxData[idxCount++] = vtxCount + v_count0 + i;
			pIdxData[idxCount++] = vtxCount + i + 1;

			pIdxData[idxCount++] = vtxCount + v_count0 + i;
			pIdxData[idxCount++] = vtxCount + v_count0 + i + 1;
			pIdxData[idxCount++] = vtxCount + i + 1;
		}
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + v_count0;
		pIdxData[idxCount++] = vtxCount + v_count0 - 1;

		pIdxData[idxCount++] = vtxCount + v_count0;
		pIdxData[idxCount++] = vtxCount + v_count0 - 1;
		pIdxData[idxCount++] = vtxCount + v_count0 * 2 - 1;

#define _push_vertex(X,Y,C) pVtxData[vtxCount++] = vertex( *mat * v2f32( X, Y ), C )

		// // // // // // // // INNER // // // // // // // //
		// Bottom Left
		_push_vertex( inner_left, bottom, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_left - TrigCache[i].sin * rad, inner_bottom + TrigCache[i].cos * rad, c );
		_push_vertex( left, inner_bottom, c );

		// Top Left
		_push_vertex( left, inner_top, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_left - TrigCache[i].cos * rad, inner_top - TrigCache[i].sin * rad, c );
		_push_vertex( inner_left, top, c );

		// Top Right
		_push_vertex( inner_right, top, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_right + TrigCache[i].sin * rad, inner_top - TrigCache[i].cos * rad, c );
		_push_vertex( right, inner_top, c );

		// Bottom Right
		_push_vertex( right, inner_bottom, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_right + TrigCache[i].cos * rad, inner_bottom + TrigCache[i].sin * rad, c );
		_push_vertex( inner_right, bottom, c );

		// // // // // // // // OUTER // // // // // // // //
		rad -= stroke_width;
		// Bottom Left
		_push_vertex( inner_left, bottom - stroke_width, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_left - TrigCache[i].sin * rad, inner_bottom + TrigCache[i].cos * rad, c );
		_push_vertex( left + stroke_width, inner_bottom, c );

		// Top Left
		_push_vertex( left + stroke_width, inner_top, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_left - TrigCache[i].cos * rad, inner_top - TrigCache[i].sin * rad, c );
		_push_vertex( inner_left, top + stroke_width, c );

		// Top Right
		_push_vertex( inner_right, top + stroke_width, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_right + TrigCache[i].sin * rad, inner_top - TrigCache[i].cos * rad, c );
		_push_vertex( right - stroke_width, inner_top, c );

		// Bottom Right
		_push_vertex( right - stroke_width, inner_bottom, c );
		for( int i = 0; i < TrigCache.size(); i++ )
			_push_vertex( inner_right + TrigCache[i].cos * rad, inner_bottom + TrigCache[i].sin * rad, c );
		_push_vertex( inner_right, bottom - stroke_width, c );

#undef _push_vertex
	}

	void Renderer2DImpl::DrawData::AddMesh( const Mesh * m )
	{
		const auto * colors = m->m_Data->color_buffer.data();

		for( auto && i : m->m_Data->index_buffer )
			pIdxData[idxCount++] = i + vtxCount;

		for( auto && v : m->m_Data->vertex_buffer )
			pVtxData[vtxCount++] = vertex( *mat * v.pos, colors[v.color_index] );
	}

	void Renderer2DImpl::DrawData::AddCircleFilled( v2f32 center, float rad, color c )
	{
		const int count = ( (int)TrigCache.size() + 1u ) * 4u - 2;
		for( int i = 0; i < count; i++ )
		{
			pIdxData[idxCount++] = vtxCount;
			pIdxData[idxCount++] = vtxCount + i + 1u;
			pIdxData[idxCount++] = vtxCount + i + 2u;
		}

		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + rad, center.y ), c );
		for( auto && trig : TrigCache )
		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + trig.cos * rad, center.y + trig.sin * rad ), c );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x, center.y + rad ), c );
		for( auto && trig : TrigCache )
		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - trig.sin * rad, center.y + trig.cos * rad ), c );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - rad, center.y ), c );
		for( auto && trig : TrigCache )
		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - trig.cos * rad, center.y - trig.sin * rad ), c );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x, center.y - rad ), c );
		for( auto && trig : TrigCache )
		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + trig.sin * rad, center.y - trig.cos * rad ), c );
	}

	void Renderer2DImpl::DrawData::AddCircle( v2f32 center, float rad, color inc, color outc, float stroke_width, StrokeStyle stroke )
	{
		const int count = ( (int)TrigCache.size() + 1u ) * 4u;
		float hw = stroke_width * 0.5f;
		float inner_rad, outer_rad;
		switch( stroke )
		{
		case Fission::StrokeStyle::Center: { float hsw = stroke_width * 0.5f; inner_rad= rad - hsw; outer_rad= rad + hsw; break; }
		case Fission::StrokeStyle::Inside:	inner_rad= rad - stroke_width, outer_rad= rad; break;
		case Fission::StrokeStyle::Outside: inner_rad= rad, outer_rad= rad + stroke_width; break;
		default: throw std::logic_error( "this don't make no fucking sense" );
		}

		// This algorithm could probably be a heck of a lot more optimized,
		// but honesty this is good enough and I don't feel like wasting any more time on this.

		int count2 = count * 2;
		for( int i = 0; i < count; i++ )
		{
			pIdxData[idxCount++] = vtxCount + (i * 2u	  )%count2;
			pIdxData[idxCount++] = vtxCount + (i * 2u + 1u)%count2;
			pIdxData[idxCount++] = vtxCount + (i * 2u + 2u)%count2;

			pIdxData[idxCount++] = vtxCount + (i * 2u + 1u)%count2;
			pIdxData[idxCount++] = vtxCount + (i * 2u + 3u)%count2;
			pIdxData[idxCount++] = vtxCount + (i * 2u + 2u)%count2;
		}

		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + inner_rad, center.y ), inc );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + outer_rad, center.y ), outc );
		for( auto && trig : TrigCache )
			pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + trig.cos * inner_rad, center.y + trig.sin * inner_rad ), inc ),
			pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + trig.cos * outer_rad, center.y + trig.sin * outer_rad ), outc );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x, center.y + inner_rad ), inc );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x, center.y + outer_rad ), outc );
		for( auto && trig : TrigCache )
			pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - trig.sin * inner_rad, center.y + trig.cos * inner_rad ), inc ),
			pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - trig.sin * outer_rad, center.y + trig.cos * outer_rad ), outc );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - inner_rad, center.y ), inc );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - outer_rad, center.y ), outc );
		for( auto && trig : TrigCache )
			pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - trig.cos * inner_rad, center.y - trig.sin * inner_rad ), inc ),
			pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x - trig.cos * outer_rad, center.y - trig.sin * outer_rad ), outc );

		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x, center.y - inner_rad ), inc );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x, center.y - outer_rad ), outc );
		for( auto && trig : TrigCache )
			pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + trig.sin * inner_rad, center.y - trig.cos * inner_rad ), inc ),
			pVtxData[vtxCount++] = vertex( *mat * v2f32( center.x + trig.sin * outer_rad, center.y - trig.cos * outer_rad ), outc );
	}

	void Renderer2DImpl::DrawData::AddTriangle( v2f32 p0, v2f32 p1, v2f32 p2, color c0, color c1, color c2 )
	{
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 1u;
		pIdxData[idxCount++] = vtxCount + 2u;

		pVtxData[vtxCount++] = vertex( *mat * p0, c0 );
		pVtxData[vtxCount++] = vertex( *mat * p1, c1 );
		pVtxData[vtxCount++] = vertex( *mat * p2, c2 );
	}

	void Renderer2DImpl::DrawData::AddTriangleUV( v2f32 p0, v2f32 p1, v2f32 p2, v2f32 uv0, v2f32 uv1, v2f32 uv2, color c )
	{
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 1u;
		pIdxData[idxCount++] = vtxCount + 2u;

		pVtxData[vtxCount++] = vertex( *mat * p0, uv0, c );
		pVtxData[vtxCount++] = vertex( *mat * p1, uv1, c );
		pVtxData[vtxCount++] = vertex( *mat * p2, uv2, c );
	}

	void Renderer2DImpl::DrawData::AddLine( v2f32 start, v2f32 end, float stroke, color startColor, color endColor )
	{
		const auto edge_vector = ( end - start ).perp().norm() * stroke / 2.0f;

		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 1u;
		pIdxData[idxCount++] = vtxCount + 2u;
		pIdxData[idxCount++] = vtxCount + 2u;
		pIdxData[idxCount++] = vtxCount + 1u;
		pIdxData[idxCount++] = vtxCount + 3u;

		pVtxData[vtxCount++] = vertex( *mat * (start + edge_vector), startColor );
		pVtxData[vtxCount++] = vertex( *mat * (start - edge_vector), startColor );
		pVtxData[vtxCount++] = vertex( *mat * (end   + edge_vector), endColor );
		pVtxData[vtxCount++] = vertex( *mat * (end   - edge_vector), endColor );
	}

	void Renderer2DImpl::DrawData::AddRectFilledUV( rf32 rect, rf32 uv, color c )
	{
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 1u;
		pIdxData[idxCount++] = vtxCount + 2u;
		pIdxData[idxCount++] = vtxCount + 3u;
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 2u;

		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.low , rect.y.high ), v2f32( uv.x.low , uv.y.high ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.low , rect.y.low  ), v2f32( uv.x.low , uv.y.low  ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.high, rect.y.low  ), v2f32( uv.x.high, uv.y.low  ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.high, rect.y.high ), v2f32( uv.x.high, uv.y.high ), c );
	}

	void Renderer2DImpl::DrawData::AddRectFilled( v2f32 tl, v2f32 tr, v2f32 bl, v2f32 br, color c )
	{
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 1u;
		pIdxData[idxCount++] = vtxCount + 2u;
		pIdxData[idxCount++] = vtxCount + 3u;
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 2u;

		pVtxData[vtxCount++] = vertex( *mat * bl, c );
		pVtxData[vtxCount++] = vertex( *mat * tl, c );
		pVtxData[vtxCount++] = vertex( *mat * tr, c );
		pVtxData[vtxCount++] = vertex( *mat * br, c );
	}

	void Renderer2DImpl::DrawData::AddRectFilled( rf32 rect, color c )
	{
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 1u;
		pIdxData[idxCount++] = vtxCount + 2u;
		pIdxData[idxCount++] = vtxCount + 3u;
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 2u;

		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.low , rect.y.high ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.low , rect.y.low  ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.high, rect.y.low  ), c );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.high, rect.y.high ), c );
	}

	void Renderer2DImpl::DrawData::AddRectFilled( rf32 rect, color tl, color tr, color bl, color br )
	{
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 1u;
		pIdxData[idxCount++] = vtxCount + 2u;
		pIdxData[idxCount++] = vtxCount + 3u;
		pIdxData[idxCount++] = vtxCount;
		pIdxData[idxCount++] = vtxCount + 2u;

		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.low , rect.y.high ), bl );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.low , rect.y.low  ), tl );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.high, rect.y.low  ), tr );
		pVtxData[vtxCount++] = vertex( *mat * v2f32( rect.x.high, rect.y.high ), br );
	}

}