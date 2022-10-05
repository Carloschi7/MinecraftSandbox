//Header which handles all the OpenGL-related stuff
#pragma once
#include <iostream>
#include "MainIncl.h"

namespace GlCore {

    enum class BlockType {
        GRASS = 0
    };

    struct CubeData {
        std::vector<float> VertexData;
        Layout Lyt;
    };

    class BlockStructure {
    public:
        BlockStructure(const BlockType &bt);

    private:
        void InitVM();
        void InitTextures();

        //Cube geometry
        static VertexManager m_VertexManager;
        //Various cube textures
        static std::vector<Texture> m_Textures;
        //For first initialization
        static bool bLoadedAssets;
        Texture* m_UsedTexture;
    };
}