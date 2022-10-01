struct VertexAttributes
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VertexAttributes VSMain(in uint VertexID : SV_VertexID)
{
	VertexAttributes va;
    va.texcoord = float2(uint2(VertexID, VertexID << 1) & 2);
    va.position = float4(lerp(float2(-1, 1), float2(1, -1), va.texcoord), 0, 1);
	return va;
}
