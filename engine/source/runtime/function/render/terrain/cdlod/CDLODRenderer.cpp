#include "CDLODRenderer.h"

namespace MoYu
{

	
    class SimpleHeightmapSource : public IHeightmapSource
    {
    private:
        float* m_pData;
        int    m_pitch; // in bytes

        int m_width;
        int m_height;

    public:
        SimpleHeightmapSource(float* pData, int pitch, int width, int height)
        {
            m_pData  = pData;
            m_pitch  = pitch;
            m_width  = width;
            m_height = height;
        }

        ~SimpleHeightmapSource() {}

        virtual int GetSizeX() { return m_width; }
        virtual int GetSizeY() { return m_height; }

        virtual unsigned short GetHeightAt(int x, int y)
        {
            return (unsigned short)(m_pData[m_pitch / sizeof(*m_pData) * y + x] * 65535.0f + 0.5f);
        }

        virtual void GetAreaMinMaxZ(int x, int y, int sizeX, int sizeY, unsigned short& minZ, unsigned short& maxZ)
        {
            assert(x >= 0 && y >= 0 && (x + sizeX) <= m_width && (y + sizeY) <= m_height);
            minZ = 65535;
            maxZ = 0;

            for (int ry = 0; ry < sizeY; ry++)
            {
                // if( (ry + y) >= rasterSizeY )
                //    break;
                float* scanLine = &m_pData[x + (ry + y) * (m_pitch / sizeof(*m_pData))];
                // int sizeX = ::min( rasterSizeX - x, size );
                for (int rx = 0; rx < sizeX; rx++)
                {
                    unsigned short h = (unsigned short)(scanLine[rx] * 65535.0f + 0.5f);
                    minZ             = glm::min(minZ, h);
                    maxZ             = glm::max(maxZ, h);
                }
            }
        }
    };



}