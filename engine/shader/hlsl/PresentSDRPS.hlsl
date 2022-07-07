struct VertexAttributes
{
	float4 Pos : SV_Position;
	float2 Tex : TEXCOORD0;
};

float3 PSMain(VertexAttributes input) : SV_Target0
{
    return float3(input.Tex.x, input.Tex.y, 0);
}
