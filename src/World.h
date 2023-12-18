#pragma once
#include <iostream>
#include <optional>
#include <thread>
#include <future>
#include <random>
#include "Chunk.h"

class Inventory;

//Some updates that happen in some world functions
struct WorldEvent
{
    u8 crafting_table_open_command : 1;
    u8 held_sprite_render : 1;
    u8 unused1 : 1;
    u8 unused2 : 1;
    u8 unused3 : 1;
    u8 unused4 : 1;
    u8 unused5 : 1;
    u8 unused6 : 1;
};

class World {
public:
    World();
    ~World();
    //Renders visible world
    void Render(const Inventory& inventory, const glm::vec3& camera_position, const glm::vec3& camera_direction);
    [[nodiscard]] WorldEvent UpdateScene(Inventory& inventory, f32 elapsed_time);
    [[nodiscard]] WorldEvent HandleSelection(Inventory& inventory, const glm::vec3& camera_position, const glm::vec3& camera_direction);
    void CheckPlayerCollision(const glm::vec3& position);
    //Pushes setion data to eventually help with serialization
    void HandleSectionData();

    //Returns the corresponding chunk index if exists
    std::optional<u32> IsChunk(const Chunk& chunk, const Defs::ChunkLocation& cl);
    Chunk& GetChunk(u32 index);
    inline Utils::UnorderedMap<u64, ChunkGeneration>& CollectGenerations() { return m_Generations; }

    Defs::WorldSeed& Seed();
    const Defs::WorldSeed& Seed() const;

    //Serialization utilities
    void SerializeSector(u32 index);
    void DeserializeSector(u32 index);

private:
    //Function which handles spawnable chunk pushing conditions
    bool IsPushable(const Chunk& chunk, const Defs::ChunkLocation& cl, const glm::vec3& vec);
    glm::vec2 SectionCentralPosFrom(u32 index);
    
public:
    //Vector which stores a the general area of all watermaps found up to that moment
    Utils::Vector<Defs::WaterArea> pushed_areas;
    Utils::Vector<glm::vec3> relative_leaves_positions;
    std::mt19937 random_engine;

private:
    //Global OpenGL environment state
    GlCore::State& m_State;
    //The Vector is a vector type that uses our allocated memory
    //mapped_space, in this case we do not directly allocate a heap section
    //of contiguous chunks like in Vector<Chunk> because we want to be
    //able to be flexible in multithreading. We just store a vistual address
    //in our virtual memory space
    Utils::Vector<Ptr<Chunk>> m_Chunks;
    //Keeps track of the generated terrain for each chunk, helps to optimize
    //the number of blocks generated per chunk
    Utils::UnorderedMap<u64, ChunkGeneration> m_Generations;
    //Non existing chunk which are near existing ones. They can spawn if the
    //player gets near enough
    Utils::Vector<glm::vec3> m_SpawnableChunks;
    //Last player pos, used to update the shadow texture
    glm::vec3 m_LastPos;
    //For terrain generation
    Defs::WorldSeed m_WorldSeed;
    //Handles section data
    Utils::Vector<Defs::SectionData> m_SectionsData;  
    //Timer used to sort the spawnable chunks vector every now and then
    //(sorting every frame would be pointless)
    Utils::Timer m_SortingTimer;

    //Buffer which holds an address to a small chunk buffer that contains the chunk
    //which the player can collide with
    //Also by declaring this we minimize the amount of allocation per frame
    //in the renderer thread
    Ptr<Chunk*> m_CollisionChunkBuffer;
    Ptr<VAddr*> m_RemovableChunkBuffer;

    //Serialization threads
    std::future<void> m_SerializingFut;
};