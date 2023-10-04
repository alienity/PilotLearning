#pragma once
#ifndef GEOMETRY_ICOSPHERE_H
#define GEOMETRY_ICOSPHERE_H
#include "runtime/resource/basic_geometry/mesh_tools.h"

#include <vector>
#include <map>

namespace MoYu
{
    namespace Geometry
    {
		struct Icosphere : BasicMesh
		{
            // ctor/dtor
            Icosphere(float radius = 1.0f, int subdivision = 1, bool smooth = false);
            ~Icosphere() {}

            static BasicMesh ToBasicMesh(float radius = 0.5f, int subdivision = 2, bool smooth = true);
            
            // getters/setters
            float getRadius() const { return _radius; }
            void  setRadius(float radius);
            int   getSubdivision() const { return _subdivision; }
            void  setSubdivision(int subdivision);
            bool  getSmooth() const { return _smooth; }
            void  setSmooth(bool smooth);

            private:
            // static functions
            static void  computeFaceNormal(const float v1[3], const float v2[3], const float v3[3], float normal[3]);
            static void  computeVertexNormal(const float v[3], float normal[3]);
            static float computeScaleForLength(const float v[3], float length);
            static void  computeHalfVertex(const float v1[3], const float v2[3], float length, float newV[3]);
            static void  computeHalfTexCoord(const float t1[2], const float t2[2], float newT[2]);
            static bool  isSharedTexCoord(const float t[2]);
            static bool  isOnLineSegment(const float a[2], const float b[2], const float c[2]);

            // member functions
            void               updateRadius();
            std::vector<float> computeIcosahedronVertices();
            void               buildVerticesFlat();
            void               buildVerticesSmooth();
            void               subdivideVerticesFlat();
            void               subdivideVerticesSmooth();
            void               addVertex(float x, float y, float z);
            void               addVertices(const float v1[3], const float v2[3], const float v3[3]);
            void               addNormal(float nx, float ny, float nz);
            void               addNormals(const float n1[3], const float n2[3], const float n3[3]);
            void               addTexCoord(float s, float t);
            void               addTexCoords(const float t1[2], const float t2[2], const float t3[2]);
            void               addIndices(unsigned int i1, unsigned int i2, unsigned int i3);
            unsigned int       addSubVertexAttribs(const float v[3], const float n[3], const float t[2]);

            // memeber vars
            float                     _radius; // circumscribed radius
            int                       _subdivision;
            bool                      _smooth;
            std::vector<float>        _vertices;
            std::vector<float>        _normals;
            std::vector<float>        _texCoords;
            std::vector<unsigned int> _indices;
            std::map<std::pair<float, float>, unsigned int>
                _sharedIndices; // indices of shared vertices, key is tex coord (s,t)
		};

    } // namespace Geometry

}

#endif // !SPHEREMESH
