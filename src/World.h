#pragma once
#include <iostream>
#include <optional>
#include <thread>
#include <future>
#include "Chunk.h"

class Inventory;

class World {
public:
    World();
    ~World();
    //Renders visible world
    void Render();
    void UpdateScene(Inventory& inventory, float elapsed_time);
    void HandleSelection(Inventory& inventory);
    //Pushes setion data to eventually help with serialization
    void HandleSectionData();
    void PushWaterLayer(std::shared_ptr<std::vector<glm::vec3>> vec);

    //Returns the corresponding chunk index if exists
    std::optional<uint32_t> IsChunk(const Chunk& chunk, const Defs::ChunkLocation& cl);
    Chunk& GetChunk(uint32_t index);

    Defs::WorldSeed& Seed();
    const Defs::WorldSeed& Seed() const;

    //Serialization utilities
    void SerializeSector(uint32_t index);
    void DeserializeSector(uint32_t index);

private:
    //Function which handles spawnable chunk pushing conditions
    bool IsPushable(const Chunk& chunk, const Defs::ChunkLocation& cl, const glm::vec3& vec);
    glm::vec2 SectionCentralPosFrom(uint32_t index);
    
private:
    //Global OpenGL environment state
    GlCore::State& m_State;
    //We prefer a vector of shared pointers instead of a vector
    //of stack allocated objects because when we are reallocating
    //the vector and the render thread is doing stuff with a chunk,
    //the chunk object is not invalidated
#ifdef MC_MULTITHREADING
    Utils::TSVector<Chunk> m_Chunks;
#else
    std::vector<std::shared_ptr<Chunk>> m_Chunks;
#endif
    //Non existing chunk which are near existing ones. They can spawn if the
    //player gets near enough
    std::vector<glm::vec3> m_SpawnableChunks;
    //Water layers pushed for current iteration
    std::vector<std::shared_ptr<std::vector<glm::vec3>>> m_DrawableWaterLayers;

    //Last player pos, used to update the shadow texture
    glm::vec3 m_LastPos;
    //For terrain generation
    Defs::WorldSeed m_WorldSeed;
    //Handles section data
    std::vector<Defs::SectionData> m_SectionsData;  
    //Timer used to sort the spawnable chunks vector every now and then
    //(sorting every frame would be pointless)
    Utils::Timer m_SortingTimer;

    //Serialization threads
    std::future<void> m_SerializingFut;
};
