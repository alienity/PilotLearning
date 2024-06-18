ByteAddressBuffer g_CounterBuffer : register(t0);
RWByteAddressBuffer g_IndirectArgsBuffer : register(u0);

[numthreads(1, 1, 1)]
void CSMain(uint GI : SV_GroupIndex) {
    uint ListCount = g_CounterBuffer.Load(0);

    // The inner sort always sorts all groups (rounded up to multiples of 2048)
    g_IndirectArgsBuffer.Store3(0, uint3((ListCount + 7) / 8, 1, 1));
}
