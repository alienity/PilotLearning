//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PathTracer.h"
#include "DirectXCollision.h"
#include "BRDF.h"
#include "../Graphics/Sampling.h"
/*
// Vertex operators
inline Vertex operator+(const Vertex& a, const Vertex& b)
{
    Vertex c;
    c.Position = a.Position + b.Position;
    c.Normal = a.Normal + b.Normal;
    c.TexCoord = a.TexCoord + b.TexCoord;
    c.LightMapUV = a.LightMapUV + b.LightMapUV;
    c.Tangent = a.Tangent + b.Tangent;
    c.Bitangent = a.Bitangent + b.Bitangent;
    return c;
}

inline Vertex operator-(const Vertex& a, const Vertex& b)
{
    Vertex c;
    c.Position = a.Position - b.Position;
    c.Normal = a.Normal - b.Normal;
    c.TexCoord = a.TexCoord - b.TexCoord;
    c.LightMapUV = a.LightMapUV - b.LightMapUV;
    c.Tangent = a.Tangent - b.Tangent;
    c.Bitangent = a.Bitangent - b.Bitangent;
    return c;
}

inline Vertex operator*(const Vertex& a, float b)
{
    Vertex c;
    c.Position = a.Position * b;
    c.Normal = a.Normal * b;
    c.TexCoord = a.TexCoord * b;
    c.LightMapUV = a.LightMapUV * b;
    c.Tangent = a.Tangent * b;
    c.Bitangent = a.Bitangent * b;
    return c;
}

// Interpolates 3 vertex values using barycentric coordinates
template<typename T> T BarycentricLerp(const T& v0, const T& v1, const T& v2, float u, float v)
{
    return v0 + (v1 - v0) * u + (v2 - v0) * v;
}

// Interpolates triangle vertex values using the barycentric coordinates from a BVH hit result
template<typename T> T TriangleLerp(const Ray& ray, const BVHData& bvhData, const std::vector<T>& vertexData)
{
    const glm::uint64 triangleIdx = ray.primID;
    const glm::uvec3& triangle = bvhData.Triangles[triangleIdx];
    const T& v0 = vertexData[triangle.x];
    const T& v1 = vertexData[triangle.y];
    const T& v2 = vertexData[triangle.z];

    return BarycentricLerp(v0, v1, v2, ray.u, ray.v);
}

// Returns the direct sun radiance for a direction on the skydome
static glm::float3 SampleSun(glm::float3 sampleDir)
{
    glm::float3 res = glm::float3(0.0f);
    if(SampleFramework11::EnableSun)
    {
        float cosSunAngularRadius = std::cos(MoYu::f::DEG_TO_RAD * SampleFramework11::SunSize);
        glm::float3 sunDir = glm::normalize(SampleFramework11::SunDirection);
        float cosGamma = glm::dot(sunDir, sampleDir);
        if(cosGamma >= cosSunAngularRadius)
            res = SampleFramework11::SunLuminance();
    }

    return res;
}

// Checks if a hit triangle is back-facing
static bool IsTriangleBackFacing(const Ray& ray, const BVHData& bvhData)
{
    // Compute the triangle normal
    const glm::uint64 triangleIdx = ray.primID;
    const glm::uvec3& triangle = bvhData.Triangles[triangleIdx];
    const glm::float3& v0 = bvhData.Vertices[triangle.x].Position;
    const glm::float3& v1 = bvhData.Vertices[triangle.y].Position;
    const glm::float3& v2 = bvhData.Vertices[triangle.z].Position;

    glm::float3 triNml = glm::normalize(glm::cross(v2 - v0, v1 - v0));
    return glm::dot(triNml, glm::vec3(ray.dir)) <= 0.0f;
}

// Returns true the the ray is occluded by a triangle
static bool Occluded(RTCScene scene, const glm::float3& position, const glm::float3& direction, float nearDist, float farDist)
{
    Ray ray(position, direction, nearDist, farDist);
    rtcOccluded1(scene, RTCRay_(ray));
    return ray.Hit();
}

// Calculates diffuse and specular from a spherical area light
static glm::float3 SampleSphericalAreaLight(const glm::float3& position, const glm::float3& normal, RTCScene scene,
                                       const glm::float3& diffuseAlbedo, const glm::float3& cameraPos,
                                       bool includeSpecular, glm::float3 specAlbedo, float roughness,
                                       float u1, float u2, float lightRadius,
                                       const glm::float3& lightPos, const glm::float3& lightColor,
                                       glm::float3& irradiance, glm::float3& sampleDir)
{
    const float radius2 = lightRadius * lightRadius;
    const float invPDF = 2.0f * MoYu::f::PI * radius2;

    glm::float3 result = glm::float3(0.0f);
    glm::float3 areaLightIrradiance = glm::float3(0.0f);

    float r = lightRadius;
    float x = u1;
    float y = u2;

    glm::float3 samplePos;
    samplePos.x = lightPos.x + 2.0f * r * std::cos(2.0f * MoYu::f::PI * y) * std::sqrt(x * (1.0f - x));
    samplePos.y = lightPos.y + 2.0f * r * std::sin(2.0f * MoYu::f::PI * y) * std::sqrt(x * (1.0f - x));
    samplePos.z = lightPos.z + r * (1.0f - 2.0f * x);

    sampleDir = samplePos - position;
    float sampleDirLen = glm::length(sampleDir);
    bool visible = false;
    if(sampleDirLen > 0.0f)
    {
        sampleDir /= sampleDirLen;

        visible = (SampleFramework11::EnableAreaLightShadows == false) || (Occluded(scene, position, sampleDir, 0.1f, sampleDirLen) == false);
    }

    float areaNDotL = std::abs(glm::dot(sampleDir, glm::normalize(samplePos - lightPos)));
    float sDotN = MoYu::Saturate(glm::dot(sampleDir, normal));

    float invRSqr = 1.0f / (sampleDirLen * sampleDirLen);

    float attenuation = areaNDotL * invRSqr;
    if(attenuation && visible)
    {
        glm::float3 sampleIrradiance = MoYu::Saturate(glm::dot(normal, sampleDir)) * lightColor * attenuation;
        glm::float3 sample = CalcLighting(normal, sampleIrradiance, sampleDir, diffuseAlbedo, position,
                                     cameraPos, roughness, includeSpecular, specAlbedo);
        result = sample * invPDF;
        areaLightIrradiance = sampleIrradiance * invPDF;
    }

    irradiance += areaLightIrradiance;

    return result;
}

// Calculates diffuse and specular contribution from the area light, given a 2D random sample point
// representing a location on the surface of the light
glm::float3 SampleAreaLight(const glm::float3& position, const glm::float3& normal, RTCScene scene,
                       const glm::float3& diffuseAlbedo, const glm::float3& cameraPos,
                       bool includeSpecular, glm::float3 specAlbedo, float roughness,
                       float u1, float u2, glm::float3& irradiance, glm::float3& sampleDir)
{
    glm::float3 lightPos = glm::float3(SampleFramework11::AreaLightX, SampleFramework11::AreaLightY, SampleFramework11::AreaLightZ);
    return SampleSphericalAreaLight(position, normal, scene, diffuseAlbedo, cameraPos, includeSpecular,
                                    specAlbedo, roughness, u1, u2, SampleFramework11::AreaLightSize,
                                    lightPos, SampleFramework11::AreaLightColor, irradiance, sampleDir);
}

// Checks to see if a ray intersects with the area light
static float AreaLightIntersection(const glm::float3& rayStart, const glm::float3& rayDir, float tStart, float tEnd)
{
    
    DirectX::BoundingSphere areaLightSphere;
    areaLightSphere.Center = DirectX::XMFLOAT3(SampleFramework11::AreaLightX, SampleFramework11::AreaLightY, SampleFramework11::AreaLightZ);
    areaLightSphere.Radius = SampleFramework11::AreaLightSize;

    DirectX::XMFLOAT3 tmpRayStart = DirectX::XMFLOAT3(rayStart.x, rayStart.y, rayStart.z);
    DirectX::XMFLOAT3 tmpRayDir = DirectX::XMFLOAT3(rayDir.x, rayDir.y, rayDir.z);
    
    float intersectDist = FLT_MAX;
    bool intersects = areaLightSphere.Intersects(XMLoadFloat3(&tmpRayStart), XMLoadFloat3(&tmpRayDir), intersectDist);
    intersects = intersects && (intersectDist >= tStart && intersectDist <= tEnd);
    return intersects ? intersectDist : FLT_MAX;
}

// Computes the difuse and specular contribution from the sun, given a 2D random sample point
// representing a location on the surface of the light
glm::float3 SampleSunLight(const glm::float3& position, const glm::float3& normal, RTCScene scene,
                             const glm::float3& diffuseAlbedo, const glm::float3& cameraPos,
                             bool includeSpecular, glm::float3 specAlbedo, float roughness,
                             float u1, float u2, glm::float3& irradiance)
{
    // Treat the sun as a spherical area light that's very far away from the surface
    const float sunDistance = 1000.0f;
    const float radius = std::tan(MoYu::DegToRad(SampleFramework11::SunSize)) * sunDistance;
    glm::float3 sunLuminance = SampleFramework11::SunLuminance();
    glm::float3 sunPos = position + SampleFramework11::SunDirection * sunDistance;
    glm::float3 sampleDir;
    return SampleSphericalAreaLight(position, normal, scene, diffuseAlbedo, cameraPos, includeSpecular,
                                    specAlbedo, roughness, u1, u2, radius, sunPos, sunLuminance, irradiance, sampleDir);
}

// Generates a full list of sample points for all integration types
void GenerateIntegrationSamples(IntegrationSamples& samples, glm::uint64 sqrtNumSamples, glm::uint64 tileSizeX, glm::uint64 tileSizeY,
                                SampleModes sampleMode, glm::uint64 numIntegrationTypes, MoYu::Random& rng)
{
    const glm::uint64 numSamplesPerPixel = sqrtNumSamples * sqrtNumSamples;
    const glm::uint64 numTilePixels = tileSizeX * tileSizeY;
    const glm::uint64 numSamplesPerTile = numSamplesPerPixel * numTilePixels;
    samples.Init(numTilePixels, numIntegrationTypes, numSamplesPerPixel);

    for(glm::uint64 pixelIdx = 0; pixelIdx < numTilePixels; ++pixelIdx)
    {
        for(glm::uint64 typeIdx = 0; typeIdx < numIntegrationTypes; ++typeIdx)
        {
            glm::float2* typeSamples = samples.GetSamplesForType(pixelIdx, typeIdx);
            if(sampleMode == SampleModes::Stratified)
            {
                GenerateStratifiedSamples2D(typeSamples, sqrtNumSamples, sqrtNumSamples, rng);
                Shuffle(typeSamples, numSamplesPerPixel, rng);
            }
            else if(sampleMode == SampleModes::UniformGrid)
            {
                GenerateGridSamples2D(typeSamples, sqrtNumSamples, sqrtNumSamples);
                Shuffle(typeSamples, numSamplesPerPixel, rng);
            }
            else if(sampleMode == SampleModes::Hammersley)
            {
                GenerateHammersleySamples2D(typeSamples, numSamplesPerPixel, typeIdx);
                Shuffle(typeSamples, numSamplesPerPixel, rng);
            }
            else if(sampleMode == SampleModes::CMJ)
            {
                GenerateCMJSamples2D(typeSamples, sqrtNumSamples, sqrtNumSamples, rng.RandomUint());
                Shuffle(typeSamples, numSamplesPerPixel, rng);
            }
            else
            {
                GenerateRandomSamples2D(typeSamples, numSamplesPerPixel, rng);
            }
        }
    }
}

// Returns the incoming radiance along the ray specified by params.RayDir, computed using unidirectional
// path tracing
glm::float3 PathTrace(const PathTracerParams& params, MoYu::Random& randomGenerator, float& illuminance, bool& hitSky)
{
    // Initialize to the view parameters, must be reset every loop iteration
    Ray ray(params.RayStart, params.RayDir, 0.0f, params.RayLen);
    illuminance = 0.0f;
    glm::float3 radiance;
    glm::float3 irradiance;
    glm::float3 throughput = glm::float3(1.0f);
    glm::float3 irrThroughput = glm::float3(1.0f);

    // Keep tracing paths until we reach the specified max
    const glm::int64 maxPathLength = params.MaxPathLength;
    for(glm::int64 pathLength = 1; pathLength <= maxPathLength || maxPathLength == -1; ++pathLength)
    {
        const bool indirectSpecOnly = params.ViewIndirectSpecular && pathLength == 1;
        const bool indirectDiffuseOnly = params.ViewIndirectDiffuse && pathLength == 1;
        const bool enableSpecular = (params.EnableBounceSpecular || (pathLength == 1)) && params.EnableSpecular;
        const bool enableDiffuse = params.EnableDiffuse;
        const bool skipDirect = SampleFramework11::ShowGroundTruth && (!SampleFramework11::EnableDirectLighting || indirectDiffuseOnly) && (pathLength == 1);

        // See if we should randomly terminate this path using Russian Roullete
        const glm::int32 rouletteDepth = params.RussianRouletteDepth;
        if(pathLength >= rouletteDepth && rouletteDepth != -1)
        {
            float continueProbability = std::min<float>(params.RussianRouletteProbability, MoYu::ComputeLuminance(throughput));
            if(randomGenerator.RandomFloat() > continueProbability)
                break;
            throughput /= continueProbability;
            irrThroughput /= continueProbability;
        }

        // Set this to true to keep the loop going
        bool continueTracing = false;

        // Check for intersection with the scene
        rtcIntersect1(params.SceneBVH->Scene, RTCRayHit_(ray));

        float sceneDistance = FLT_MAX;
        if (ray.geomID == RTC_INVALID_GEOMETRY_ID)
            sceneDistance = ray.tfar;
        
        glm::float3 rayOrigin = glm::float3(ray.org);
        glm::float3 rayDir = glm::float3(ray.dir);

        // Check for intersection with the area light for primary rays
        float lightDistance = FLT_MAX;
        if(params.EnableDirectAreaLight && SampleFramework11::EnableAreaLight && pathLength == 1)
            lightDistance = AreaLightIntersection(rayOrigin, rayDir, ray.tnear, ray.tfar);

        if(lightDistance < sceneDistance)
        {
            // We hit the area light: just return the uniform radiance of the light source
            radiance = SampleFramework11::AreaLightColor * throughput;
            irradiance = glm::float3(0.0f);
        }
        else if(sceneDistance < FLT_MAX)
        {
            // We hit a triangle in the scene
            if(pathLength == maxPathLength)
            {
                // There's no point in continuing anymore, since none of our scene surfaces are emissive.
                break;
            }

            const BVHData& bvh = *params.SceneBVH;

            // Treat back-facing triangles as pure black
            if(IsTriangleBackFacing(ray, bvh))
                break;

            // Interpolate the vertex data
            Vertex hitSurface = TriangleLerp(ray, bvh, bvh.Vertices);

            hitSurface.Normal = glm::normalize(hitSurface.Normal);
            hitSurface.Tangent = glm::normalize(hitSurface.Tangent);
            hitSurface.Bitangent = glm::normalize(hitSurface.Bitangent);

            // Look up the material data
            const glm::uint64 materialIdx = bvh.MaterialIndices[ray.primID];

            glm::float3 albedo = glm::float3(1.0f);
            if(SampleFramework11::EnableAlbedoMaps && !indirectDiffuseOnly)
                albedo = SampleTexture2D(hitSurface.TexCoord, bvh.MaterialDiffuseMaps[materialIdx]);

            glm::float3x3 tangentToWorld;
            tangentToWorld[0] = (hitSurface.Tangent);
            tangentToWorld[1] = (hitSurface.Bitangent);
            tangentToWorld[2] = (hitSurface.Normal);

            // Normal mapping
            glm::float3 normal = hitSurface.Normal;
            const auto& normalMap = bvh.MaterialNormalMaps[materialIdx];
            if(SampleFramework11::EnableNormalMaps && normalMap.Texels.size() > 0)
            {
                normal = glm::float3(SampleTexture2D(hitSurface.TexCoord, normalMap));
                normal = normal * 2.0f - 1.0f;
                normal.z = std::sqrt(1.0f - MoYu::Saturate(normal.x * normal.x + normal.y * normal.y));
                normal = MoYu::Lerp(glm::float3(0.0f, 0.0f, 1.0f), normal, SampleFramework11::NormalMapIntensity);
                normal = glm::normalize(tangentToWorld * normal);
            }

            tangentToWorld[2] = normal;

            float sqrtRoughness = glm::float3(SampleTexture2D(hitSurface.TexCoord, bvh.MaterialRoughnessMaps[materialIdx])).x;
            float metallic =  glm::float3(SampleTexture2D(hitSurface.TexCoord, bvh.MaterialMetallicMaps[materialIdx])).x;
            metallic = MoYu::Saturate(metallic + SampleFramework11::MetallicOffset);

            glm::float3 diffuseAlbedo = MoYu::Lerp(albedo, glm::float3(0.0f), metallic) * SampleFramework11::DiffuseAlbedoScale;
            glm::float3 specAlbedo = MoYu::Lerp(glm::float3(0.03f), albedo, metallic);
            sqrtRoughness *= SampleFramework11::RoughnessScale;
            if(SampleFramework11::RoughnessOverride >= 0.01f)
                sqrtRoughness = SampleFramework11::RoughnessOverride;

            sqrtRoughness = MoYu::Saturate(sqrtRoughness);
            float roughness = sqrtRoughness * sqrtRoughness;

            diffuseAlbedo *= enableDiffuse ? 1.0f : 0.0f;

            if(indirectSpecOnly == false)
            {
                // Compute direct lighting from the sun
                glm::float3 directLighting;
                glm::float3 directIrradiance;
                if((SampleFramework11::EnableDirectLighting || pathLength > 1) && SampleFramework11::EnableSun)
                {
                    glm::float2 sunSample = params.SampleSet->Sun();
                    if(pathLength > 1)
                        sunSample = randomGenerator.RandomFloat2();
                    glm::float3 sunDirectLighting = SampleSunLight(hitSurface.Position, normal, bvh.Scene, diffuseAlbedo,
                                                     rayOrigin, enableSpecular, specAlbedo, roughness,
                                                     sunSample.x, sunSample.y, directIrradiance);
                    if(!skipDirect || SampleFramework11::BakeDirectSunLight)
                        directLighting += sunDirectLighting;
                }

                // Compute direct lighting from the area light
                if(SampleFramework11::EnableAreaLight)
                {
                    glm::float2 areaLightSample = params.SampleSet->AreaLight();
                    if(pathLength > 1)
                        areaLightSample = randomGenerator.RandomFloat2();
                    glm::float3 areaLightSampleDir;
                    glm::float3 areaLightDirectLighting = SampleAreaLight(hitSurface.Position, normal, bvh.Scene, diffuseAlbedo,
                                                     rayOrigin, enableSpecular, specAlbedo, roughness,
                                                     areaLightSample.x, areaLightSample.y, directIrradiance, areaLightSampleDir);
                    if(!skipDirect || SampleFramework11::BakeDirectAreaLight)
                        directLighting += areaLightDirectLighting;
                }

                radiance += directLighting * throughput;
                irradiance += directIrradiance * irrThroughput;
            }

            // Pick a new path, using MIS to sample both our diffuse and specular BRDF's
            if(SampleFramework11::EnableIndirectLighting || params.ViewIndirectSpecular)
            {
                const bool enableDiffuseSampling = metallic < 1.0f && SampleFramework11::EnableIndirectDiffuse && enableDiffuse && indirectSpecOnly == false;
                const bool enableSpecularSampling = enableSpecular && SampleFramework11::EnableIndirectSpecular && !indirectDiffuseOnly;
                if(enableDiffuseSampling || enableSpecularSampling)
                {
                    // Randomly select if we should sample our diffuse BRDF, or our specular BRDF
                    glm::float2 brdfSample = params.SampleSet->BRDF();
                    if(pathLength > 1)
                        brdfSample = randomGenerator.RandomFloat2();

                    float selector = brdfSample.x;
                    if(enableSpecularSampling == false)
                        selector = 0.0f;
                    else if(enableDiffuseSampling == false)
                        selector = 1.0f;

                    glm::float3 sampleDir;
                    glm::float3 v = glm::normalize(rayOrigin - hitSurface.Position);

                    if(selector < 0.5f)
                    {
                        // We're sampling the diffuse BRDF, so sample a cosine-weighted hemisphere
                        if(enableSpecularSampling)
                            brdfSample.x *= 2.0f;
                        sampleDir = SampleCosineHemisphere(brdfSample.x, brdfSample.y);
                        sampleDir = glm::normalize(tangentToWorld * sampleDir);
                    }
                    else
                    {
                        // We're sampling the GGX specular BRDF
                        if(enableDiffuseSampling)
                            brdfSample.x = (brdfSample.x - 0.5f) * 2.0f;
                        sampleDir = SampleDirectionGGX(v, normal, roughness, tangentToWorld, brdfSample.x, brdfSample.y);
                    }

                    glm::float3 h = glm::normalize(v + sampleDir);
                    float nDotL = MoYu::Saturate(glm::dot(sampleDir, normal));

                    float diffusePDF = enableDiffuseSampling ? nDotL * MoYu::f::ONE_OVER_PI : 0.0f;
                    float specularPDF = enableSpecularSampling ? GGX_PDF(normal, h, v, roughness) : 0.0f;
                    float pdf = diffusePDF + specularPDF;
                    if(enableDiffuseSampling && enableSpecularSampling)
                        pdf *= 0.5f;

                    if(nDotL > 0.0f && pdf > 0.0f && glm::dot(sampleDir, hitSurface.Normal) > 0.0f)
                    {
                        // Compute both BRDF's
                        glm::float3 brdf = glm::float3(0.0f);
                        if(enableDiffuseSampling)
                            brdf += ((SampleFramework11::ShowGroundTruth && params.ViewIndirectDiffuse && pathLength == 1) ? glm::float3(1, 1, 1) : diffuseAlbedo) * MoYu::f::ONE_OVER_PI;

                        if(enableSpecularSampling)
                        {
                            float spec = GGX_Specular(roughness, normal, h, v, sampleDir);
                            brdf += Fresnel(specAlbedo, h, sampleDir) * spec;
                        }

                        throughput *= brdf * nDotL / pdf;
                        irrThroughput *= nDotL / pdf;

                        // Generate the ray for the new path
                        ray = Ray(hitSurface.Position, sampleDir, 0.001f, FLT_MAX);

                        continueTracing = true;
                    }
                }
            }
        }
        else {
            // We hit the sky, so we'll sample the sky radiance and then bail out
            hitSky = true;

            if (SampleFramework11::SkyMode == SkyModes::Procedural)
            {
                glm::float3 skyRadiance = Skybox::SampleSky(*params.SkyCache, rayDir);
                if (pathLength == 1 && params.EnableDirectSun)
                    skyRadiance += SampleSun(rayDir);
                radiance += skyRadiance * throughput;
                irradiance += skyRadiance * irrThroughput;
            }
            else if (SampleFramework11::SkyMode == SkyModes::Simple)
            {
                glm::float3 skyRadiance = SampleFramework11::SkyColor;
                if (pathLength == 1 && params.EnableDirectSun)
                    skyRadiance += SampleSun(rayDir);
                radiance += skyRadiance * throughput;
                irradiance += skyRadiance * irrThroughput;
            }
            else if (SampleFramework11::SkyMode >= SampleFramework11::CubeMapStart)
            {
                glm::float3 cubeMapRadiance = SampleCubemap(rayDir, params.EnvMaps[SampleFramework11::SkyMode - SampleFramework11::CubeMapStart]);
                radiance += cubeMapRadiance * throughput;
                irradiance += cubeMapRadiance * irrThroughput;
            }
        }

        if(continueTracing == false)
            break;
    }

    illuminance = MoYu::ComputeLuminance(irradiance);
    return radiance;
}
*/