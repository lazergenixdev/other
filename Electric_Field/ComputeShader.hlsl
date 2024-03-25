#define NUMLINES 16
#define NUMPARTICLES 5 // ref
#define TAU 6.2831853071f

#define USE_ONLY_POS_OR_NEG
//#define USE_EXTRA_THICC_LINES

static const float maxDist = 5.0f;

cbuffer CBuf : register(b0)
{
    int4 padding;
    float4 particles[NUMPARTICLES]; // xy - position | z - charge | w - unused
}

RWTexture2D<float4> rwFieldLines : register(u0);

float2 GetFieldVector( float2 pos )
{
    float2 fv = float2( 0.0f, 0.0f );
    for( int i = 0; i < NUMPARTICLES; i++ )
    {
        float2 dir = pos - particles[i].xy;
        fv += normalize( dir ) * ( particles[i].z / dot(dir, dir) );
    }
    return fv;
}

[numthreads(NUMLINES, NUMPARTICLES, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    float theta = TAU * DTid.x / NUMLINES;
    float2 draw = particles[DTid.y].xy + 5.0f * float2( cos(theta), sin(theta) );
    int i;
#ifdef USE_ONLY_POS_OR_NEG
    int npositive = 0;
    for( i = 0; i < NUMPARTICLES; i++ )
    {
        if( particles[i].z > 0 ) npositive++;
    }

    bool show_pos = npositive > ( NUMPARTICLES / 2 );
    if( particles[DTid.y].z > 0 != show_pos ) return;
#endif
    for( i = 0; i < 10000; i++ )
    {
        float2 field = GetFieldVector( draw );
        draw += ( (particles[DTid.y].z < 0.0f) ? -normalize( field ) : normalize( field ) );
        float s = length( field ) * 5000.0f;
        rwFieldLines[ draw.xy ] = float4( s, s, s, 1.0f );
#ifdef USE_EXTRA_THICC_LINES
        rwFieldLines[ int2(draw.x + 1, draw.y) ] = float4( s, s, s, 1.0f );
        rwFieldLines[ int2(draw.x - 1, draw.y) ] = float4( s, s, s, 1.0f );
        rwFieldLines[ int2(draw.x, draw.y + 1) ] = float4( s, s, s, 1.0f );
        rwFieldLines[ int2(draw.x, draw.y - 1) ] = float4( s, s, s, 1.0f );
#endif
    }
}