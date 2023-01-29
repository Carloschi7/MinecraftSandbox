#pragma once
#include <iostream>

namespace GlCore
{
    struct VertexData
    {
        std::vector<float> vertices;
        Layout lyt;
    };


    //Pair consisting of an array of integers which represents the initial data
    //vertex for each cube face, and an integer which denotes how many faces
    //are being rendered
    using DrawableData = std::pair<std::array<uint8_t, 3>, uint8_t>;

    static constexpr float one_third = 1.0f / 3.0f;

    static const std::vector<float> pos_and_tex_coord_sided
    {
        //Back
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.0f,     2.0f * one_third,
         0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.0f,     one_third,
         0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.25f,    one_third,
         0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.25f,    one_third,
        -0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.25f,    2.0f * one_third,
        -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.0f,     2.0f * one_third,
        //Front                 
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,   0.0f,     2.0f * one_third,
         0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,   0.0f,     one_third,
         0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,   0.25f,    one_third,
         0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,   0.25f,    one_third,
        -0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,   0.25f,    2.0f * one_third,
        -0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,   0.0f,     2.0f * one_third,
        //Left                  
        -0.5f,  0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,   0.5f,     2.0f * one_third,
        -0.5f,  0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,   0.5f,     one_third,
        -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,   0.75f,    one_third,
        -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,   0.75f,    one_third,
        -0.5f, -0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,   0.75f,    2.0f * one_third,
        -0.5f,  0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,   0.5f,     2.0f * one_third,
        //Right                 
         0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,   0.5f,     2.0f * one_third,
         0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,   0.5f,     one_third,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,   0.75f,    one_third,
         0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,   0.75f,    one_third,
         0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 0.0f,   0.75f,    2.0f * one_third,
         0.5f,  0.5f,  0.5f,    1.0f, 0.0f, 0.0f,   0.5f,     2.0f * one_third,
         //Bottom               
        -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,   0.75f,    one_third,
         0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,   1.0f,     one_third,
         0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,   1.0f,     2.0f * one_third,
         0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,   1.0f,     2.0f * one_third,
        -0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,   0.75f,    2.0f * one_third,
        -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,   0.75f,    one_third,
        //Top                   
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   0.25f,    one_third,
         0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   0.50f,    one_third,
         0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,   0.50f,    2.0f * one_third,
         0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,   0.50f,    2.0f * one_third,
        -0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,   0.25f,    2.0f * one_third,
        -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   0.25f,    one_third,
    };

    static const std::vector<float> vertices_crossaim = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,

         0.5f,  0.5f,
        -0.5f,  0.5f,
        -0.5f, -0.5f,
    };

    inline VertexData Cube()
    {
        Layout lyt;
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0 });
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 3 * sizeof(float) });
        lyt.PushAttribute({ 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 6 * sizeof(float) });
        return { pos_and_tex_coord_sided, lyt };
    }

    inline VertexData CubeForDepth()
    {
        Layout lyt;
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0 });
        return { pos_and_tex_coord_sided, lyt };
    }

    inline VertexData CrossAim()
    {
        Layout lyt;
        lyt.PushAttribute({ 2,GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0 });
        return { vertices_crossaim, lyt };
    }

    inline Layout MatrixAttributeLayout()
    {
        Layout lyt;
        lyt.PushAttribute({ 16, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), 0});
        return lyt;
    }

    //Starting index of respective cube face
    inline uint8_t GetNormVertexBegin(const glm::vec3& vec)
    {
        if (vec == glm::vec3(0.0f, 0.0f, -1.0f))
            return 0;
        if (vec == glm::vec3(0.0f, 0.0f, 1.0f))
            return 6;
        if (vec == glm::vec3(-1.0f, 0.0f, 0.0f))
            return 12;
        if (vec == glm::vec3(1.0f, 0.0f, 0.0f))
            return 18;
        if (vec == glm::vec3(0.0f, -1.0f, 0.0f))
            return 24;
        if (vec == glm::vec3(0.0f, 1.0f, 0.0f))
            return 30;
    }
}