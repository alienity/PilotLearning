#ifndef MOTION_VEC_VERTEX_COMMON_INCLUDED
#define MOTION_VEC_VERTEX_COMMON_INCLUDED

// Available semantic start from TEXCOORD4
struct AttributesPass
{
    float3 previousPositionOS : TEXCOORD4; // Contain previous transform position (in case of skinning for example)
#if defined (_ADD_PRECOMPUTED_VELOCITY)
    float3 precomputedVelocity    : TEXCOORD5; // Add Precomputed Velocity (Alembic computes velocities on runtime side).
#endif
};

struct VaryingsPassToPS
{
    // Note: Z component is not use currently
    // This is the clip space position. Warning, do not confuse with the value of positionCS in PackedVarying which is SV_POSITION and store in positionSS
    float4 positionCS;
    float4 previousPositionCS;
};

// Available interpolator start from TEXCOORD8
struct PackedVaryingsPassToPS
{
    // Note: Z component is not use
    float3 interpolators0 : TEXCOORD8;
    float3 interpolators1 : TEXCOORD9;
};

PackedVaryingsPassToPS PackVaryingsPassToPS(VaryingsPassToPS input)
{
    PackedVaryingsPassToPS output;
    output.interpolators0 = float3(input.positionCS.xyw);
    output.interpolators1 = float3(input.previousPositionCS.xyw);

    return output;
}

VaryingsPassToPS UnpackVaryingsPassToPS(PackedVaryingsPassToPS input)
{
    VaryingsPassToPS output;
    output.positionCS = float4(input.interpolators0.xy, 0.0, input.interpolators0.z);
    output.previousPositionCS = float4(input.interpolators1.xy, 0.0, input.interpolators1.z);

    return output;
}

#define VaryingsPassType VaryingsPassToPS

// We will use custom attributes for this pass
#define VARYINGS_NEED_PASS
#include "../../RenderPipeline/ShaderPass/VertMesh.hlsl"

void MotionVectorPositionZBias(FrameUniforms frameUniform,VaryingsToPS input)
{
#if UNITY_REVERSED_Z
    input.vmesh.positionCS.z -= frameUniform.cameraUniform.unity_MotionVectorsParams.z * input.vmesh.positionCS.w;
#else
    input.vmesh.positionCS.z += unity_MotionVectorsParams.z * input.vmesh.positionCS.w;
#endif
}

PackedVaryingsType MotionVectorVS(FrameUniforms frameUniform, RenderDataPerDraw renderData,
    VaryingsType varyingsType, AttributesMesh inputMesh, AttributesPass inputPass)
{
    MotionVectorPositionZBias(frameUniform, varyingsType);

    // Use unjiterred matrix for motion vector
    varyingsType.vpass.positionCS = mul(UNITY_MATRIX_UNJITTERED_VP(frameUniform), float4(varyingsType.vmesh.positionRWS, 1.0));

    // Note: unity_MotionVectorsParams.y is 0 is forceNoMotion is enabled
    bool forceNoMotion = frameUniform.cameraUniform.unity_MotionVectorsParams.y == 0.0;

    if (forceNoMotion)
    {
        varyingsType.vpass.previousPositionCS = float4(0.0, 0.0, 0.0, 1.0);
    }
    else
    {
        bool previousPositionCSComputed = false;
        float3 effectivePositionOS = (float3)0.0f;
        float3 previousPositionRWS = (float3)0.0f;

        bool hasDeformation = frameUniform.cameraUniform.unity_MotionVectorsParams.x > 0.0; // Skin or morph target
        effectivePositionOS = (hasDeformation ? inputPass.previousPositionOS : inputMesh.positionOS);

        // See _TransparentCameraOnlyMotionVectors in HDCamera.cs
#ifdef _WRITE_TRANSPARENT_MOTION_VECTOR
        if (_TransparentCameraOnlyMotionVectors > 0)
        {
            previousPositionRWS = varyingsType.vmesh.positionRWS.xyz;
#ifndef TESSELLATION_ON
            varyingsType.vpass.previousPositionCS = mul(UNITY_MATRIX_PREV_VP, float4(previousPositionRWS, 1.0));
#endif
            previousPositionCSComputed = true;
        }
#endif

        if (!previousPositionCSComputed)
        {
            // Need to apply any vertex animation to the previous worldspace position, if we want it to show up in the motion vector buffer
#if defined(HAVE_MESH_MODIFICATION)
            AttributesMesh previousMesh = inputMesh;
            previousMesh.positionOS = effectivePositionOS;

#ifdef USE_CUSTOMINTERP_SUBSTRUCT
            // Create a dummy value here to avoid modifying the current custom interpolator values when calculting the previous mesh
            // Since this value is never being used it should be removed by the shader compiler
            VaryingsMeshType dummyVaryingsMesh = (VaryingsMeshType)0;
#endif
            previousMesh = ApplyMeshModification(previousMesh, _LastTimeParameters.xyz
#ifdef USE_CUSTOMINTERP_SUBSTRUCT
                , dummyVaryingsMesh
#endif
#ifdef HAVE_VFX_MODIFICATION
                , inputElement
#endif
            );

#if defined(UNITY_DOTS_INSTANCING_ENABLED) && defined(DOTS_DEFORMED)
            // Deformed vertices in DOTS are not cumulative with built-in Unity skinning/blend shapes
            // Needs to be called after vertex modification has been applied otherwise it will be
            // overwritten by Compute Deform node
            ApplyPreviousFrameDeformedVertexPosition(inputMesh.vertexID, previousMesh.positionOS);
#endif

#if defined(HAVE_VFX_MODIFICATION)
            // Only handle the VFX case here since it is only used with ShaderGraph (and ShaderGraph always has mesh modification enabled).
            previousMesh = VFXTransformMeshToPreviousElement(previousMesh, inputElement);
#endif

#if defined(_ADD_CUSTOM_VELOCITY) // For shader graph custom velocity
            // Note that to fetch custom velocity here we must use the inputMesh and not the previousMesh
            // in the case the custom velocity depends on the positionOS
            // otherwise it will apply two times the modifications.
            // However the vertex animation will still be perform correctly as we used previousMesh position
            // where we could ahve trouble is if time is used to drive custom velocity, this will not work
            previousMesh.positionOS -= GetCustomVelocity(inputMesh
#ifdef HAVE_VFX_MODIFICATION
                , inputElement
#endif
            );
#endif

#if defined(_ADD_PRECOMPUTED_VELOCITY)
            previousMesh.positionOS -= inputPass.precomputedVelocity;
#endif

#if defined(HAVE_VFX_MODIFICATION) && VFX_WORLD_SPACE
            //previousMesh.positionOS is already in absolute world space
            previousPositionRWS = GetCameraRelativePositionWS(previousMesh.positionOS);
#else

            previousPositionRWS = TransformPreviousObjectToWorld(previousMesh.positionOS);

#endif // defined(HAVE_VFX_MODIFICATION) && VFX_WORLD_SPACE

#else // HAVE_MESH_MODIFICATION

#if defined(_ADD_CUSTOM_VELOCITY) // For shader graph custom velocity
            effectivePositionOS -= GetCustomVelocity(inputMesh
#ifdef HAVE_VFX_MODIFICATION
                , inputElement
#endif
            );
#endif

#if defined(_ADD_PRECOMPUTED_VELOCITY)
            effectivePositionOS -= inputPass.precomputedVelocity;
#endif

            previousPositionRWS = TransformPreviousObjectToWorld(renderData, effectivePositionOS);
#endif // HAVE_MESH_MODIFICATION

#ifdef ATTRIBUTES_NEED_NORMAL
            float3 normalWS = TransformPreviousObjectToWorldNormal(renderData, inputMesh.normalOS);
#else
            float3 normalWS = float3(0.0, 0.0, 0.0);
#endif

        }
        
        // Final computation from previousPositionRWS (if not already done)
        if (!previousPositionCSComputed)
        {
            varyingsType.vpass.previousPositionCS = mul(UNITY_MATRIX_PREV_VP(frameUniform), float4(previousPositionRWS, 1.0));
        }
    }

    return PackVaryingsType(varyingsType);
}

#endif
