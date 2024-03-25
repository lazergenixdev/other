struct VS_OUT
{
    float2 texcoord : TEXCOORD;
    float4 pos : SV_Position;
};

VS_OUT main( float2 pos : Position,  float2 tc : TexCoord )
{
    VS_OUT vso;
    vso.pos = float4( pos, 0.5f, 1.0f );
    vso.texcoord = tc;
	return vso;
}