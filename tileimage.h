#ifndef TILEIMAGE_H
#define TILEIMAGE_H

#include <string>
#include <vector>
#include <fstream>
#include <optional>
#include <filesystem>

constexpr uint32_t CHUNK_SIZE = 10 * 1024 * 1024; // Chunk of 10 MB
constexpr size_t   MAX_IMAGE_MEMORY = 10 * 1024 * 1024; // Max RAM memory for storing image

class TileImage
{
public:
    static std::optional<TileImage> load(const std::string p_path);
    std::optional<std::vector<uint8_t>> readAll();
    std::optional<std::vector<uint8_t>> readChunk(const uint32_t p_chunkSize=CHUNK_SIZE);
    size_t size() const;
    bool isLarge() const;


private:
    TileImage(const std::string p_path);

    std::ifstream m_file;
    std::string m_path;
    size_t m_size;
};

#endif // TILEIMAGE_H
