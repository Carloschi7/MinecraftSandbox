#pragma once
#include <iostream>
#include <optional>
#include <thread>
#include "Chunk.h"

class World {
public:
    World();
    ~World();
    //Draws the number of chunks visible to the player
    void DrawRenderable();
    void UpdateScene();
    void HandleSelection();
    //Pushes setion data to eventually help with serialization
    void HandleSectionData();

    //Returns the corresponding chunk index if exists
    std::optional<uint32_t> IsChunk(const Chunk& chunk, const Gd::ChunkLocation& cl);
    Chunk& GetChunk(uint32_t index);

    Gd::WorldSeed& Seed();
    const Gd::WorldSeed& Seed() const;

    //Serialization utilities
    void SerializeSector(uint32_t index);
    void DeserializeSector(uint32_t index);

private:
    //Function which handles spawnable chunk pushing conditions
    bool IsPushable(const Chunk& chunk, const Gd::ChunkLocation& cl, const glm::vec3& vec);
    glm::vec2 SectionCentralPosFrom(uint32_t index);
    
private:
    VecType<Chunk> m_Chunks;
    //Non existing chunk which are near existing ones. They can spawn if the
    //player gets near enough
    std::vector<glm::vec3> m_SpawnableChunks;
    //Last player pos
    glm::vec3 m_LastPos;
    //Responsible for camera, skybox & other stuff
    GlCore::WorldStructure m_WorldStructure;
    //For terrain generation
    Gd::WorldSeed m_WorldSeed;
    //Handles section data
    std::vector<Gd::SectionData> m_SectionsData;
    //Determines whether m_Chunks is serializing/deserializing
    std::atomic_bool m_ChunkMemoryOperations = false;
    //Safe iteration size(useful when the chunks at the end of the vector are being serialized)
    std::atomic_uint32_t m_SafeChunkSize = 0;
    //Timer used to sort the spawnable chunks vector every now and then
    //(sorting every frame would be pointless)
    Utils::Timer m_SortingTimer;
};

