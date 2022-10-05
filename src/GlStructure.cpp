#include "GlStructure.h"
#include "Vertices.h"

VertexManager GlCore::BlockStructure::m_VertexManager;
std::vector<Texture> GlCore::BlockStructure::m_Textures;
bool GlCore::BlockStructure::bLoadedAssets = false;

namespace GlCore
{
    BlockStructure::BlockStructure(const GlCore::BlockType& bt)
        :m_UsedTexture(nullptr)
    {
        //Just need to be loaded once, every block shares this properies
        if(!bLoadedAssets)
        {
            InitVM();
            InitTextures();
            bLoadedAssets = true;
        }

        switch (bt) {
            case BlockType::GRASS:
                m_UsedTexture = &m_Textures[0];
                break;
            default:
                break;
        }
    }

    void BlockStructure::InitVM()
    {
        CubeData cd = Utils::Cube(Utils::ItemProps::POS_AND_TEX_COORDS);
        m_VertexManager.SendDataToOpenGLArray(cd.VertexData.data(), cd.VertexData.size(), cd.Lyt);
    }

    void BlockStructure::InitTextures()
    {
        m_Textures.emplace_back(""); //Future texture name
    }
}