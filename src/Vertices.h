#pragma once
#include <iostream>

namespace Utils
{
    struct VertexData
    {
        std::vector<float> vertices;
        Layout lyt;
    };

    static constexpr float one_third = 1.0f / 3.0f;

    static const std::vector<float> pos_and_tex_coord_single = {
        //Back
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        //Front
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        //Left
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        //Right
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         //Bottom
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         //Top
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f

    };

    static const std::vector<float> pos_and_tex_coord_sided
    {
        //Back
        -0.5f, -0.5f, -0.5f,  0.0f,     2.0f * one_third,
         0.5f, -0.5f, -0.5f,  0.0f,     one_third,
         0.5f,  0.5f, -0.5f,  0.25f,    one_third,
         0.5f,  0.5f, -0.5f,  0.25f,    one_third,
        -0.5f,  0.5f, -0.5f,  0.25f,    2.0f * one_third,
        -0.5f, -0.5f, -0.5f,  0.0f,     2.0f * one_third,
        //Front
        -0.5f, -0.5f,  0.5f,  0.0f,     2.0f * one_third,
         0.5f, -0.5f,  0.5f,  0.0f,     one_third,
         0.5f,  0.5f,  0.5f,  0.25f,    one_third,
         0.5f,  0.5f,  0.5f,  0.25f,    one_third,
        -0.5f,  0.5f,  0.5f,  0.25f,    2.0f * one_third,
        -0.5f, -0.5f,  0.5f,  0.0f,     2.0f * one_third,
        //Left
        -0.5f,  0.5f,  0.5f,  0.5f,     2.0f * one_third,
        -0.5f,  0.5f, -0.5f,  0.5f,     one_third,
        -0.5f, -0.5f, -0.5f,  0.75f,    one_third,
        -0.5f, -0.5f, -0.5f,  0.75f,    one_third,
        -0.5f, -0.5f,  0.5f,  0.75f,    2.0f * one_third,
        -0.5f,  0.5f,  0.5f,  0.5f,     2.0f * one_third,
        //Right
         0.5f,  0.5f,  0.5f,  0.5f,     2.0f * one_third,
         0.5f,  0.5f, -0.5f,  0.5f,     one_third,
         0.5f, -0.5f, -0.5f,  0.75f,    one_third,
         0.5f, -0.5f, -0.5f,  0.75f,    one_third,
         0.5f, -0.5f,  0.5f,  0.75f,    2.0f * one_third,
         0.5f,  0.5f,  0.5f,  0.5f,     2.0f * one_third,
         //Bottom
        -0.5f, -0.5f, -0.5f,  0.75f,    one_third,
         0.5f, -0.5f, -0.5f,  1.0f,     one_third,
         0.5f, -0.5f,  0.5f,  1.0f,     2.0f * one_third,
         0.5f, -0.5f,  0.5f,  1.0f,     2.0f * one_third,
        -0.5f, -0.5f,  0.5f,  0.75f,    2.0f * one_third,
        -0.5f, -0.5f, -0.5f,  0.75f,    one_third,
        //Top
        -0.5f,  0.5f, -0.5f,  0.25f,    one_third,
         0.5f,  0.5f, -0.5f,  0.50f,    one_third,
         0.5f,  0.5f,  0.5f,  0.50f,    2.0f * one_third,
         0.5f,  0.5f,  0.5f,  0.50f,    2.0f * one_third,
        -0.5f,  0.5f,  0.5f,  0.25f,    2.0f * one_third,
        -0.5f,  0.5f, -0.5f,  0.25f,    one_third,
    };

    static const std::vector<float> vertices_crossaim = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,

         0.5f,  0.5f,
        -0.5f,  0.5f,
        -0.5f, -0.5f,
    };

    enum class VertexProps{POS_AND_TEX_COORDS_SINGLE = 0, POS_AND_TEX_COORD_SIDED};

    inline VertexData Cube(const VertexProps& ip)
    {
        Layout lyt;

        switch(ip)
        {
        case VertexProps::POS_AND_TEX_COORDS_SINGLE:
            lyt.PushAttribute({3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0});
            lyt.PushAttribute({2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 3 * sizeof(float)});
            return { pos_and_tex_coord_single, lyt };

        case VertexProps::POS_AND_TEX_COORD_SIDED:
            lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0 });
            lyt.PushAttribute({ 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 3 * sizeof(float) });
            return { pos_and_tex_coord_sided, lyt };

        default:
            return {};
        }
    }

    inline VertexData CrossAim()
    {
        Layout lyt;
        lyt.PushAttribute({ 2,GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0 });
        return { vertices_crossaim, lyt };
    }

    //Starting index of respective cube face
    inline uint32_t GetNormVertexBegin(const glm::vec3& vec)
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