
cbuffer CBuf : register(b0)
{
    int4 select; // r, g, b - color | a - index of selected particle
    float4 particles[5];
};

Texture2D lines : register(t0);
Texture2D proton : register(t1);
Texture2D electron : register(t2);

SamplerState smplr : register(s0);

static const float particleSize = 10.0f;
static const float voltageSpread = 50.0f;

float GetVoltage( in float2 pos )
{
    float voltage = 0.0f;
    for ( int i = 0; i < 5; i++ )
    {
        voltage += ( particles[i].z / distance(pos, particles[i].xy) );
    }
    return voltage;
}

float4 main( float2 texcoord : TEXCOORD ) : SV_TARGET
{
    uint2 dim;
    lines.GetDimensions(dim.x, dim.y);
    float2 pos = texcoord * dim;

    float dist = length(pos - particles[select.w].xy);
    if ( dist >= particleSize + 4.0f && dist <= particleSize + 8.5f )
    {
        if ( dist >= particleSize + 5.0f && dist <= particleSize + 7.5f )
            return float4(select.rgb, 1.0f);
        else return float2(0.0f, 1.0f).rrrg;
    }

    for( int i = 0; i < 5; i++ )
    {
        if (distance(pos, particles[i].xy) < particleSize)
        {
            pos = pos - particles[i].xy;
            pos /= ( 2.0f * particleSize );
            pos -= 0.5f;
            return particles[i].z > 0.0f ? proton.Sample( smplr, pos ) : electron.Sample( smplr, pos );
        }
    }
    float voltage = GetVoltage( pos );
    return lines.Sample( smplr, texcoord ) + voltageSpread * ( voltage > 0 ? float4(voltage,0,0,0) : float4(0, 0, -voltage, 0) );
}