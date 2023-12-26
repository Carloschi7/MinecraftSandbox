#include "Vertices.h"
#include "Memory.h"

namespace GlCore
{
    Layout PosAndTexCoordDefaultLayout()
    {
        Layout lyt;
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 0 });
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 3 * sizeof(f32) });
        lyt.PushAttribute({ 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 6 * sizeof(f32) });
        return lyt;
    }

    Layout PosAndTexCoordDecal2DLayout()
    {
        Layout lyt;
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 0 });
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 3 * sizeof(f32) });
        lyt.PushAttribute({ 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 6 * sizeof(f32) });
        return lyt;
    }

    //Data of the vertex manager on which the bound texture to the screen framebuffer
    //will be drawn
    Layout PosAndTexCoordsScreenLayout() {
        Layout lyt;
        lyt.PushAttribute({ 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0 });
        lyt.PushAttribute({ 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 2 * sizeof(f32) });
        return lyt;
    }

    Layout WaterLayerLayout()
    {
        Layout lyt;
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 0 });
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 3 * sizeof(f32) });
        lyt.PushAttribute({ 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), 6 * sizeof(f32) });
        return lyt;
    }

    Layout PosAndTexCoordsDepthLayout()
    {
        Layout lyt;
        lyt.PushAttribute({ 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), 0 });
        return lyt;
    }

    Layout CrossaimLayout()
    {
        Layout lyt;
        lyt.PushAttribute({ 2,GL_FLOAT, GL_FALSE, 2 * sizeof(f32), 0 });
        return lyt;
    }

    Layout InventoryLayout()
    {
        Layout lyt;
        lyt.PushAttribute({ 2,GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0 });
        lyt.PushAttribute({ 2,GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 2 * sizeof(f32) });
        return lyt;
    }

    Layout InventoryEntryLayout()
    {
        Layout lyt;
        lyt.PushAttribute({ 2,GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0 });
        lyt.PushAttribute({ 2,GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 2 * sizeof(f32) });
        return lyt;
    }

    MeshStorage AllocateMeshStorage(Memory::Arena* arena) 
    {
        //back: +0, front: +0, left: +2, right: +2, bottom: +3, top: +1
        const f32 pos_and_tex_coord_default[]
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

        const f32 pos_and_tex_coord_depth[]
        {
            //Back
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            //Front             
            -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            //Left              
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            //Right             
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             //Bottom           
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,
            //Top               
            -0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
        };

        const f32 pos_and_tex_coords_decal2d[]
        {
            //Back
            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.0f,     2.0f * one_third,
             0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.0f,     one_third,
             0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.25f,    one_third,
             0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.25f,    one_third,
            -0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.25f,    2.0f * one_third,
            -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,   0.0f,     2.0f * one_third,
        };

        const f32 pos_and_tex_coords_screen[]
        {
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f,

            1.0f, 1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f
        };

        const f32 water_layer[]
        {
            -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   0.25f,    one_third,
             0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   0.50f,    one_third,
             0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,   0.50f,    2.0f * one_third,
             0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,   0.50f,    2.0f * one_third,
            -0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,   0.25f,    2.0f * one_third,
            -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 0.0f,   0.25f,    one_third,
        };

        const f32 crossaim[]
        {
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,

             0.5f,  0.5f,
            -0.5f,  0.5f,
            -0.5f, -0.5f,
        };

        const f32 inventory[]
        {
            -0.5f, -0.5f,   0.0f, 0.0f,
             0.5f, -0.5f,   1.0f, 0.0f,
             0.5f,  0.5f,   1.0f, 1.0f,

             0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f,   0.0f, 1.0f,
            -0.5f, -0.5f,   0.0f, 0.0f,
        };

        const f32 inventory_entry[]
        {
            // The 0.01f padding adjust the item image perfectly to the center,
            // that probably does not happen by default because of unfortunate
            // float rounding
            -0.5f, -0.5f,   0.25f, 2.0f * one_third - 0.01f,
             0.5f, -0.5f,   0.50f, 2.0f * one_third - 0.01f,
             0.5f,  0.5f,   0.50f, one_third,
             0.5f,  0.5f,   0.50f, one_third,
            -0.5f,  0.5f,   0.25f, one_third,
            -0.5f, -0.5f,   0.25f, 2.0f * one_third - 0.01f,
        };

        MeshStorage result{};
        using namespace Memory;
        result.pos_and_tex_coord_default.data = static_cast<f32*>(AllocateUnchecked(arena, sizeof(pos_and_tex_coord_default)));
        result.pos_and_tex_coord_depth.data = static_cast<f32*>(AllocateUnchecked(arena, sizeof(pos_and_tex_coord_depth)));
        result.pos_and_tex_coords_decal2d.data = static_cast<f32*>(AllocateUnchecked(arena, sizeof(pos_and_tex_coords_decal2d)));
        result.pos_and_tex_coords_screen.data = static_cast<f32*>(AllocateUnchecked(arena, sizeof(pos_and_tex_coords_screen)));
        result.water_layer.data = static_cast<f32*>(AllocateUnchecked(arena, sizeof(water_layer)));
        result.crossaim.data = static_cast<f32*>(AllocateUnchecked(arena, sizeof(crossaim)));
        result.inventory.data = static_cast<f32*>(AllocateUnchecked(arena, sizeof(inventory)));
        result.inventory_entry.data = static_cast<f32*>(AllocateUnchecked(arena, sizeof(inventory_entry)));

        std::memcpy(result.pos_and_tex_coord_default.data, pos_and_tex_coord_default, sizeof(pos_and_tex_coord_default));
        std::memcpy(result.pos_and_tex_coord_depth.data, pos_and_tex_coord_depth, sizeof(pos_and_tex_coord_depth));
        std::memcpy(result.pos_and_tex_coords_decal2d.data, pos_and_tex_coords_decal2d, sizeof(pos_and_tex_coords_decal2d));
        std::memcpy(result.pos_and_tex_coords_screen.data, pos_and_tex_coords_screen, sizeof(pos_and_tex_coords_screen));
        std::memcpy(result.water_layer.data, water_layer, sizeof(water_layer));
        std::memcpy(result.crossaim.data, crossaim, sizeof(crossaim));
        std::memcpy(result.inventory.data, inventory, sizeof(inventory));
        std::memcpy(result.inventory_entry.data, inventory_entry, sizeof(inventory_entry));

        result.pos_and_tex_coord_default.count = sizeof(pos_and_tex_coord_default) / sizeof(f32);
        result.pos_and_tex_coord_depth.count = sizeof(pos_and_tex_coord_depth) / sizeof(f32);
        result.pos_and_tex_coords_decal2d.count = sizeof(pos_and_tex_coords_decal2d) / sizeof(f32);
        result.pos_and_tex_coords_screen.count = sizeof(pos_and_tex_coords_screen) / sizeof(f32);
        result.water_layer.count = sizeof(water_layer) / sizeof(f32);
        result.crossaim.count = sizeof(crossaim) / sizeof(f32);
        result.inventory.count = sizeof(inventory) / sizeof(f32);
        result.inventory_entry.count = sizeof(inventory_entry) / sizeof(f32);

        result.pos_and_tex_coord_default.lyt = PosAndTexCoordDefaultLayout();
        result.pos_and_tex_coord_depth.lyt = PosAndTexCoordsDepthLayout();
        result.pos_and_tex_coords_decal2d.lyt = PosAndTexCoordDecal2DLayout();
        result.pos_and_tex_coords_screen.lyt = PosAndTexCoordsScreenLayout();
        result.water_layer.lyt = WaterLayerLayout();
        result.crossaim.lyt = CrossaimLayout();
        result.inventory.lyt = InventoryLayout();
        result.inventory_entry.lyt = InventoryEntryLayout();

        return result;
    }

    void FreeMeshStorage(MeshStorage& storage, Memory::Arena* arena)
    {
        using namespace Memory;
        FreeUnchecked(arena, storage.pos_and_tex_coord_default.data);
        FreeUnchecked(arena, storage.pos_and_tex_coord_depth.data);
        FreeUnchecked(arena, storage.pos_and_tex_coords_decal2d.data);
        FreeUnchecked(arena, storage.pos_and_tex_coords_screen.data);
        FreeUnchecked(arena, storage.water_layer.data);
        FreeUnchecked(arena, storage.crossaim.data);
        FreeUnchecked(arena, storage.inventory.data);
        FreeUnchecked(arena, storage.inventory_entry.data);
    }



    //Starting index of respective cube face
    u8 GetNormVertexBegin(const glm::vec3& vec)
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
