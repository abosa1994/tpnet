#include "tileimage.h"
#include <iostream>
#include <filesystem>

std::optional<TileImage> TileImage::load(const std::string p_path)
{
    // Check if file exists and is a regular file
    if ( !std::filesystem::exists(p_path))
    {
        std::cerr << "Error: File " << p_path << " does not exist" << std::endl;
        return std::nullopt;
    }

    if ( !std::filesystem::is_regular_file(p_path) )
    {
        std::cerr << "Error: Path " << p_path << " is not a regular file" << std::endl;
        return std::nullopt;
    }

    //    std::cout << "File size: " << size << " bytes\n";

        //    } catch (const fs::filesystem_error& e) {
        //        std::cerr << "Filesystem error: " << e.what() << '\n';
        //        return 1;
        //    } catch (const std::exception& e) {
        //        std::cerr << "Error: " << e.what() << '\n';
        //        return 1;
        //    }

    // Get file size in bytes
    if ( std::filesystem::file_size(p_path) <= 0 )
    {
        std::cerr << "Error: File " << p_path << " has an invalid size" << std::endl;
        return std::nullopt;
    }

    return TileImage(p_path);
}


TileImage::TileImage(const std::string p_path) :
    m_path(p_path)
  , m_size(std::filesystem::file_size(p_path))
  , m_file(p_path, std::ios::binary)
{

}

std::optional<std::vector<uint8_t>> TileImage::readAll()
{
    if ( m_size <= MAX_IMAGE_MEMORY )
    {
        // Leggi tutto in memoria
        std::vector<uint8_t> data(m_size);
        m_file.read(reinterpret_cast<char*>(data.data()), m_size);

//        // Inserisci in cache
//        m_cache.put(tile_key, data);

        return data;
    }

    return std::nullopt;
}

std::optional<std::vector<uint8_t>> TileImage::readChunk(const uint32_t p_chunkSize)
{
    std::vector<uint8_t> l_chunk(p_chunkSize);
    m_file.read(reinterpret_cast<char*>(l_chunk.data()), CHUNK_SIZE);
    size_t l_bytesRead = m_file.gcount();  // bytes effettivamente letti
    if ( l_bytesRead > 0 )
        return l_chunk;

    return std::nullopt;
}

size_t TileImage::size() const
{
    return m_size;
}

bool TileImage::isLarge() const
{
    return m_size > MAX_IMAGE_MEMORY;
}


// Posso usare SMALL_IMAGE_THRESHOLD = CHUNK_SIZE

//#include <iostream>
//#include <filesystem> // C++17
//#include <string>

//namespace fs = std::filesystem;

//int main() {
//    std::string filePath;

//    std::cout << "Enter file path: ";
//    std::getline(std::cin, filePath);

//    try {
//        // Check if file exists and is a regular file
//        if (!fs::exists(filePath)) {
//            std::cerr << "Error: File does not exist.\n";
//            return 1;
//        }
//        if (!fs::is_regular_file(filePath)) {
//            std::cerr << "Error: Path is not a regular file.\n";
//            return 1;
//        }

//        // Get file size in bytes
//        auto size = fs::file_size(filePath);
//        std::cout << "File size: " << size << " bytes\n";

//    } catch (const fs::filesystem_error& e) {
//        std::cerr << "Filesystem error: " << e.what() << '\n';
//        return 1;
//    } catch (const std::exception& e) {
//        std::cerr << "Error: " << e.what() << '\n';
//        return 1;
//    }

//    return 0;
//}

//// Leggi dimensione
//size_t imgsize = std::filesystem::file_size(tile_path);

//// Invia sempre l'header prima
//Header header{};
//header.magic       = MAGIC_CHECK;
//header.type        = ResponseType::IMAGE;
//header.payload_len = imgsize;
//sendHeader(header.serialize());

//if ( imgsize <= SMALL_IMAGE_THRESHOLD )
//{
//    // Leggi tutto in memoria
//    std::vector<uint8_t> data(imgsize);
//    std::ifstream file(tile_path, std::ios::binary);
//    file.read(reinterpret_cast<char*>(data.data()), imgsize);

//    // Inserisci in cache
//    m_cache.put(tile_key, data);

//    // Invia
//    sendData(data.data(), imgsize);
//}
//else
//{
//    // Streaming a chunk — mai tutto in RAM
//    std::ifstream file(tile_path, std::ios::binary);
//    std::array<uint8_t, CHUNK_SIZE> chunk;

//    while ( file )
//    {
//        file.read(reinterpret_cast<char*>(chunk.data()), CHUNK_SIZE);
//        size_t bytes_read = file.gcount();  // bytes effettivamente letti
//        if ( bytes_read > 0 )
//            sendData(chunk.data(), bytes_read);
//    }
//}
