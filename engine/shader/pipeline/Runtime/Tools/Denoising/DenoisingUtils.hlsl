
#define MAX_REPROJECTION_DISTANCE 0.5
#define MAX_PIXEL_TOLERANCE 4
#define PROJECTION_EPSILON 0.000001

float ComputeMaxReprojectionWorldRadius(FrameUniforms frameUniform, float3 positionWS, float3 normalWS, float pixelSpreadAngleTangent, float maxDistance, float pixelTolerance)
{
    const float3 positionVS = GetWorldSpaceViewDir(frameUniform, positionWS);
    const float3 viewWS = normalize(positionVS);
    // const float3 viewWS = GetWorldSpaceNormalizeViewDir(frameUniform, positionWS);
    // float parallelPixelFootPrint = pixelSpreadAngleTangent * length(positionWS);
    float parallelPixelFootPrint = pixelSpreadAngleTangent * length(positionVS);
    float realPixelFootPrint = parallelPixelFootPrint / max(abs(dot(normalWS, viewWS)), PROJECTION_EPSILON);
    return max(maxDistance, realPixelFootPrint * pixelTolerance);
}

float ComputeMaxReprojectionWorldRadius(FrameUniforms frameUniform, float3 positionWS, float3 normalWS, float pixelSpreadAngleTangent)
{
    return ComputeMaxReprojectionWorldRadius(frameUniform, positionWS, normalWS, pixelSpreadAngleTangent, MAX_REPROJECTION_DISTANCE, MAX_PIXEL_TOLERANCE);
}
