struct VertexAttributes
{
	float4 Pos : SV_Position;
	float2 Tex : TEXCOORD0;
};

float4 PSMain(VertexAttributes input) : SV_Target0
{
    return float4(input.Tex.x, input.Tex.y, 0, 0);
}
