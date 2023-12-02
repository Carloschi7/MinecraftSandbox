#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include "VertexManager.h"
#include "Memory.h"

namespace GlCore
{
    //Pair consisting of an array of integers which represents the initial data
    //vertex for each cube face, and an integer which denotes how many faces
    //are being rendered
    using DrawableData = std::pair<std::array<u8, 3>, u8>;
    static constexpr f32 one_third = 1.0f / 3.0f;

    //TODO is this a good idea? moving everything to a struct
    struct MeshElement
    {
        f32* data;
        uint32_t count;
        Layout lyt;
    };

    struct MeshStorage 
    {
        MeshElement pos_and_tex_coord_default;
        MeshElement pos_and_tex_coord_depth;
        MeshElement pos_and_tex_coords_decal2d;
        MeshElement pos_and_tex_coords_screen;
        MeshElement water_layer;
        MeshElement crossaim;
        MeshElement inventory;
        MeshElement inventory_entry;
    };

    //Functions that return the raw buffer layout.
    //the buffers will be defined in AllocateMeshStorage
    Layout PosAndTexCoordDefaultLayout();
    Layout PosAndTexCoordDecal2DLayout();
    Layout PosAndTexCoordsScreenLayout();
    Layout WaterLayerLayout();
    Layout PosAndTexCoordsDepthLayout();
    Layout CrossaimLayout();
    Layout InventoryLayout();
    Layout InventoryEntryLayout();

    MeshStorage AllocateMeshStorage(Memory::Arena* arena);
    void FreeMeshStorage(MeshStorage& storage, Memory::Arena* arena);
    u8 GetNormVertexBegin(const glm::vec3& vec);
}