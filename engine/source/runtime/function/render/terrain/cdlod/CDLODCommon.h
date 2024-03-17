#ifndef CDLOD_COMMON_H
#define CDLOD_COMMON_H

#include "runtime/core/math/moyu_math2.h"

namespace MoYu
{

	//////////////////////////////////////////////////////////////////////////
    // Interface for providing source height data to CDLODQuadTree
    //////////////////////////////////////////////////////////////////////////
    class IHeightmapSource
    {
    public:
        virtual int GetSizeX() = 0;
        virtual int GetSizeY() = 0;

        // returned value is converted to height using following formula:
        // 'WorldHeight = WorldMinZ + GetHeightAt(,) * WorldSizeZ / 65535.0f'
        virtual unsigned short GetHeightAt(int x, int y) = 0;

        virtual void GetAreaMinMaxZ(int x, int y, int sizeX, int sizeY, unsigned short& minZ, unsigned short& maxZ) = 0;
    };

    struct MapDimensions
    {
        float MinX;
        float MinY;
        float MinZ;
        float SizeX;
        float SizeY;
        float SizeZ;

        float MaxX() const { return MinX + SizeX; }
        float MaxY() const { return MinY + SizeY; }
        float MaxZ() const { return MinZ + SizeZ; }
    };

}

#endif // CDLOD_COMMON_H
