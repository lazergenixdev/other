#pragma once

	enum Channels
	{
		r, g, b, a
	};

class Color
{
public:
	constexpr Color() : dword( 0x0u ) {}
	constexpr Color( unsigned int dw ) : dword( dw ) {}
	unsigned int operator,( const Channels& ch ) const
	{
		switch ( ch )
		{
		case r:
			return ( dword >> 24u );
		case g:
			return ( dword >> 16u );
		case b:
			return ( dword >> 8u );
		case a:
			return ( dword );
		}
	}
private:
	unsigned int dword;
};