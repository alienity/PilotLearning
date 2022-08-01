struct VertexAttributes
{
	float4 Pos : SV_Position;
	float2 Tex : TEXCOORD0;
};

VertexAttributes VSMain(in uint VertexID : SV_VertexID)
{
	VertexAttributes va;
	va.Tex = float2(uint2(VertexID, VertexID << 1) & 2);
	va.Pos = float4(lerp(float2(-1, 1), float2(1, -1), va.Tex), 0, 1);
	return va;
}
