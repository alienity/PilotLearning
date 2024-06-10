#ifndef UNITY_SPACE_TRANSFORMS_INCLUDED
#define UNITY_SPACE_TRANSFORMS_INCLUDED

// Return the PreTranslated ObjectToWorld Matrix (i.e matrix with _WorldSpaceCameraPos apply to it if we use camera relative rendering)
float4x4 GetObjectToWorldMatrix(RenderDataPerDraw renderDataPerDraw)
{
    return UNITY_MATRIX_M(renderDataPerDraw);
}

float4x4 GetWorldToObjectMatrix(RenderDataPerDraw renderDataPerDraw)
{
    return UNITY_MATRIX_I_M(renderDataPerDraw);
}

float4x4 GetPrevObjectToWorldMatrix(RenderDataPerDraw renderDataPerDraw)
{
    return UNITY_PREV_MATRIX_M(renderDataPerDraw);
}

float4x4 GetPrevWorldToObjectMatrix(RenderDataPerDraw renderDataPerDraw)
{
    return UNITY_PREV_MATRIX_I_M(renderDataPerDraw);
}

float4x4 GetWorldToViewMatrix(FrameUniforms frameUniform)
{
    return UNITY_MATRIX_V(frameUniform);
}

float4x4 GetViewToWorldMatrix(FrameUniforms frameUniform)
{
    return UNITY_MATRIX_I_V(frameUniform);
}

// Transform to homogenous clip space
float4x4 GetWorldToHClipMatrix(FrameUniforms frameUniform)
{
    return UNITY_MATRIX_VP(frameUniform);
}

// Transform to homogenous clip space
float4x4 GetViewToHClipMatrix(FrameUniforms frameUniform)
{
    return UNITY_MATRIX_P(frameUniform);
}

float3 TransformObjectToWorld(RenderDataPerDraw renderDataPerDraw, float3 positionOS)
{
    return mul(GetObjectToWorldMatrix(renderDataPerDraw), float4(positionOS, 1.0)).xyz;
}

float3 TransformWorldToObject(RenderDataPerDraw renderDataPerDraw, float3 positionWS)
{
    return mul(GetWorldToObjectMatrix(renderDataPerDraw), float4(positionWS, 1.0)).xyz;
}

float3 TransformWorldToView(FrameUniforms frameUniform, float3 positionWS)
{
    return mul(GetWorldToViewMatrix(frameUniform), float4(positionWS, 1.0)).xyz;
}

float3 TransformViewToWorld(FrameUniforms frameUniform, float3 positionVS)
{
    return mul(GetViewToWorldMatrix(frameUniform), float4(positionVS, 1.0)).xyz;
}

// Transforms position from object space to homogenous space
float4 TransformObjectToHClip(FrameUniforms frameUniform, RenderDataPerDraw renderDataPerDraw, float3 positionOS)
{
    // More efficient than computing M*VP matrix product
    return mul(GetWorldToHClipMatrix(frameUniform), mul(GetObjectToWorldMatrix(renderDataPerDraw), float4(positionOS, 1.0)));
}

// Transforms position from world space to homogenous space
float4 TransformWorldToHClip(FrameUniforms frameUniform, float3 positionWS)
{
    return mul(GetWorldToHClipMatrix(frameUniform), float4(positionWS, 1.0));
}

// Transforms position from view space to homogenous space
float4 TransformWViewToHClip(FrameUniforms frameUniform, float3 positionVS)
{
    return mul(GetViewToHClipMatrix(frameUniform), float4(positionVS, 1.0));
}

// Normalize to support uniform scaling
float3 TransformObjectToWorldDir(RenderDataPerDraw renderDataPerDraw, float3 dirOS, bool doNormalize = true)
{
    float3 dirWS = mul((float3x3)GetObjectToWorldMatrix(renderDataPerDraw), dirOS);
    if (doNormalize)
        return SafeNormalize(dirWS);
    return dirWS;
}

// Normalize to support uniform scaling
float3 TransformWorldToObjectDir(RenderDataPerDraw renderDataPerDraw, float3 dirWS, bool doNormalize = true)
{
    float3 dirOS = mul((float3x3)GetWorldToObjectMatrix(renderDataPerDraw), dirWS);
    if (doNormalize)
        return normalize(dirOS);
    return dirOS;
}

// Transforms vector from world space to view space
float3 TransformWorldToViewDir(FrameUniforms frameUniform, float3 dirWS, bool doNormalize = false)
{
    float3 dirVS = mul((float3x3)GetWorldToViewMatrix(frameUniform), dirWS).xyz;
    if (doNormalize)
        return normalize(dirVS);
    return dirVS;
}

// Transforms vector from view space to world space
float3 TransformViewToWorldDir(FrameUniforms frameUniform, float3 dirVS, bool doNormalize = false)
{
    float3 dirWS = mul((float3x3)GetViewToWorldMatrix(frameUniform), dirVS).xyz;
    if (doNormalize)
        return normalize(dirWS);
    return dirWS;
}

// Transforms normal from world space to view space
float3 TransformWorldToViewNormal(FrameUniforms frameUniform, float3 normalWS, bool doNormalize = false)
{
    // assuming view matrix is uniformly scaled, we can use direction transform
    return TransformWorldToViewDir(frameUniform, normalWS, doNormalize);
}

// Transforms normal from view space to world space
float3 TransformViewToWorldNormal(FrameUniforms frameUniform, float3 normalVS, bool doNormalize = false)
{
    // assuming view matrix is uniformly scaled, we can use direction transform
    return TransformViewToWorldDir(frameUniform, normalVS, doNormalize);
}

// Transforms vector from world space to homogenous space
float3 TransformWorldToHClipDir(FrameUniforms frameUniform, float3 directionWS, bool doNormalize = false)
{
    float3 dirHCS = mul((float3x3)GetWorldToHClipMatrix(frameUniform), directionWS).xyz;
    if (doNormalize)
        return normalize(dirHCS);
    return dirHCS;
}

// Transforms normal from object to world space
float3 TransformObjectToWorldNormal(RenderDataPerDraw renderDataPerDraw, float3 normalOS, bool doNormalize = true)
{
    // Normal need to be multiply by inverse transpose
    float3 normalWS = mul(normalOS, (float3x3)GetWorldToObjectMatrix(renderDataPerDraw));
    if (doNormalize)
        return SafeNormalize(normalWS);
    return normalWS;
}

// Transforms normal from world to object space
float3 TransformWorldToObjectNormal(RenderDataPerDraw renderDataPerDraw, float3 normalWS, bool doNormalize = true)
{
    // Normal need to be multiply by inverse transpose
    float3 normalOS = mul(normalWS, (float3x3)GetObjectToWorldMatrix(renderDataPerDraw));
    if (doNormalize)
        return SafeNormalize(normalOS);
    return normalOS;
}

float3x3 CreateTangentToWorld(float3 normal, float3 tangent, float flipSign)
{
    // For odd-negative scale transforms we need to flip the sign
    float sgn = flipSign;
    float3 bitangent = cross(normal, tangent) * sgn;
    return float3x3(tangent, bitangent, normal);
}

// this function is intended to work on Normals (handles non-uniform scale)
// tangentToWorld is the matrix representing the transformation of a normal from tangent to world space
float3 TransformTangentToWorld(float3 normalTS, float3x3 tangentToWorld, bool doNormalize = false)
{
    // Note matrix is in row major convention with left multiplication as it is build on the fly
    float3 result = mul(normalTS, tangentToWorld);
    if (doNormalize)
        return SafeNormalize(result);
    return result;
}

// this function is intended to work on Normals (handles non-uniform scale)
// This function does the exact inverse of TransformTangentToWorld() and is
// also decribed within comments in mikktspace.h and it follows implicitly
// from the scalar triple product (google it).
// tangentToWorld is the matrix representing the transformation of a normal from tangent to world space
float3 TransformWorldToTangent(float3 normalWS, float3x3 tangentToWorld, bool doNormalize = true)
{
    // Note matrix is in row major convention with left multiplication as it is build on the fly
    float3 row0 = tangentToWorld[0];
    float3 row1 = tangentToWorld[1];
    float3 row2 = tangentToWorld[2];

    // these are the columns of the inverse matrix but scaled by the determinant
    float3 col0 = cross(row1, row2);
    float3 col1 = cross(row2, row0);
    float3 col2 = cross(row0, row1);

    float determinant = dot(row0, col0);

    // inverse transposed but scaled by determinant
    // Will remove transpose part by using matrix as the first arg in the mul() below
    // this makes it the exact inverse of what TransformTangentToWorld() does.
    float3x3 matTBN_I_T = float3x3(col0, col1, col2);
    float3 result = mul(matTBN_I_T, normalWS);
    if (doNormalize)
    {
        float sgn = determinant < 0.0 ? (-1.0) : 1.0;
        return SafeNormalize(sgn * result);
    }
    else
        return result / determinant;
}

// this function is intended to work on Vectors/Directions
// tangentToWorld is the matrix representing the transformation of a normal from tangent to world space
float3 TransformWorldToTangentDir(float3 dirWS, float3x3 tangentToWorld, bool doNormalize = false)
{
    // Note matrix is in row major convention with left multiplication as it is build on the fly
    float3 result = mul(tangentToWorld, dirWS);
    if (doNormalize)
        return SafeNormalize(result);
    return result;
}

// this function is intended to work on Vectors/Directions
// This function does the exact inverse of TransformWorldToTangentDir()
// tangentToWorld is the matrix representing the transformation of a normal from tangent to world space
float3 TransformTangentToWorldDir(float3 dirWS, float3x3 tangentToWorld, bool doNormalize = false)
{
    // Note matrix is in row major convention with left multiplication as it is build on the fly
    float3 row0 = tangentToWorld[0];
    float3 row1 = tangentToWorld[1];
    float3 row2 = tangentToWorld[2];

    // these are the columns of the inverse matrix but scaled by the determinant
    float3 col0 = cross(row1, row2);
    float3 col1 = cross(row2, row0);
    float3 col2 = cross(row0, row1);

    float determinant = dot(row0, col0);

    // inverse transposed but scaled by determinant
    // Will remove transpose part by using matrix as the second arg in the mul() below
    // this makes it the exact inverse of what TransformWorldToTangentDir() does.
    float3x3 matTBN_I_T = float3x3(col0, col1, col2);
    float3 result = mul(dirWS, matTBN_I_T);
    if (doNormalize)
    {
        float sgn = determinant < 0.0 ? (-1.0) : 1.0;
        return SafeNormalize(sgn * result);
    }
    else
        return result / determinant;
}

// tangentToWorld is the matrix representing the transformation of a normal from tangent to world space
float3 TransformTangentToObject(RenderDataPerDraw renderDataPerDraw, float3 dirTS, float3x3 tangentToWorld)
{
    // Note matrix is in row major convention with left multiplication as it is build on the fly
    float3 normalWS = TransformTangentToWorld(dirTS, tangentToWorld);
    return TransformWorldToObjectNormal(renderDataPerDraw, normalWS);
}

// tangentToWorld is the matrix representing the transformation of a normal from tangent to world space
float3 TransformObjectToTangent(RenderDataPerDraw renderDataPerDraw, float3 dirOS, float3x3 tangentToWorld)
{
    // Note matrix is in row major convention with left multiplication as it is build on the fly

    // don't normalize, as normalWS will be normalized after TransformWorldToTangent
    float3 normalWS = TransformObjectToWorldNormal(renderDataPerDraw, dirOS, false);

    // transform from world to tangent
    return TransformWorldToTangent(normalWS, tangentToWorld);
}

#endif
