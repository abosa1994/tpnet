#include "tcpserver.h"

#include <netinet/tcp.h>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <chrono>

#include <cstring>
#include <iomanip>      // for std::setprecision

constexpr int g_maxQueuedConnections = 5;
constexpr int g_maxBufferSize = 256;

TcpServer::TcpServer(const unsigned int p_port) :
    m_port(p_port)
  , m_socket(-1)
  , m_socketAddr()
  , m_isRunning(false)
{

}

bool TcpServer::init()
{
    // Create socket (end point for communication)
    m_socket = socket(AF_INET, SOCK_STREAM, 0);     // valutare se usare AF_UNIX per comunicazioni strettamente locali -> prestazioni migliori
    if ( m_socket < 0 )
    {
        std::cerr << "ERROR: socket not opened on server side" << std::endl;
        return false;
    }

    int l_opt{1};
    if ( setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0 )
    {
        std::cerr << "ERROR: setsockopt (SO_REUSEADDR) failed on server side" << std::endl;
        return false;
    }
    if ( setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &l_opt, sizeof(l_opt)) < 0 )
    {
        std::cerr << "ERROR: setsockopt (SO_KEEPALIVE) failed on server side" << std::endl;
        return false;
    }

    int l_idle{10};
    int l_interval{5};
    int l_count{3};
    if ( setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPIDLE, &l_idle, sizeof(l_idle)) < 0 )
    {
        std::cerr << "ERROR: setsockopt (TCP_KEEPIDLE) failed on server side" << std::endl;
        return false;
    }
    if ( setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPINTVL, &l_interval, sizeof(l_interval)) < 0 )
    {
        std::cerr << "ERROR: setsockopt (TCP_KEEPIDLE) failed on server side" << std::endl;
        return false;
    }
    if ( setsockopt(m_socket, IPPROTO_TCP, TCP_KEEPCNT, &l_count, sizeof(l_count)) < 0 )
    {
        std::cerr << "ERROR: setsockopt (TCP_KEEPCNT) failed on server side" << std::endl;
        return false;
    }


    m_socketAddr.sin_family = AF_INET;
    m_socketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_socketAddr.sin_port = htons(m_port);

    // Bind socket to a specific address
    if ( bind(m_socket, toSockAddr(&m_socketAddr), sizeof(m_socketAddr)) < 0 )
    {
        std::cerr << "ERROR: socket not bound on server side" << std::endl;
        return false;
    }

    std::cout << "Server correctly initialized" << std::endl;
    return true;
}

bool TcpServer::run()
{
    m_isRunning.store(true);

    // Listen for connections
    sockaddr_in l_clientSocketAddr;
    socklen_t l_clientSocketLen{sizeof(l_clientSocketAddr)};
    int l_clientSocket;
    if ( listen(m_socket, g_maxQueuedConnections) < 0 )
    {
        std::cerr << "ERROR: listen failed on server side" << std::endl;
        return false;
    }

    // Accept connection
    l_clientSocket = accept(m_socket, toSockAddr(&l_clientSocketAddr), &l_clientSocketLen);
    if ( l_clientSocket < 0 )
    {
        std::cerr << "ERROR: accept failed on server side" << std::endl;
        return false;
    }

    // Read message
    while ( m_isRunning.load() )
    {
        std::optional<tcp_utils::Header> l_header = recvHeader(l_clientSocket);
        if ( !l_header.has_value() ) continue;

        std::cout << "Received header with type: " << l_header->type << "; payload_size: " << l_header->payload_len << std::endl;
        std::optional<std::vector<uint8_t>> l_data = recvData(l_clientSocket, l_header->payload_len);
        if ( !l_data.has_value() )
        {
            std::cerr << "ERROR: read invalid data" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        switch ( l_header->type )
        {
            case tcp_utils::RequestType::GET_TILE:
            {
                if ( l_header->payload_len != sizeof(tcp_utils::TileRequest) )
                {
                    std::cerr << "ERROR: invalid request for RequestType " << l_header->type << std::endl;
                    break;
                }

                std::optional<tcp_utils::TileRequest> l_request = tcp_utils::TileRequest::fromBuffer(l_data->data(), sizeof(tcp_utils::TileRequest) );

                if ( !l_request )
                {
                    std::cerr << "ERROR: invalid request for RequestType " << l_header->type << std::endl;
                    break;
                }

                bool res = handleTileRequest(l_clientSocket, l_request.value());

                break;
            }
            default:
            {
                std::cerr << "ERROR: RequestType " << l_header->type << " not handled" << std::endl;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    close(l_clientSocket);

    return true;
}

std::optional<tcp_utils::Header> TcpServer::recvHeader(int p_clientSocket)
{
    static char l_buffer[sizeof(tcp_utils::Header)];

    int l_totalBytes{0};
    int l_bytesReceived{0};
    do
    {
        int l_bytesReceived = recv(p_clientSocket, l_buffer+l_totalBytes, sizeof(l_buffer)-l_totalBytes, MSG_DONTWAIT);
        if ( l_bytesReceived < 0 )
        {
          // recv failed, handle error
            if (errno == EAGAIN)
            {
                // try again
                return std::nullopt;
            }
            else
            {
                std::cerr << "ERROR: recv failed" << std::endl;
                close(p_clientSocket);
                return std::nullopt;
            }
        }
        else if ( l_bytesReceived == 0 )
        {
            // remote side closed connection
            std::cerr << "ERROR: connection closed" << std::endl;
            m_isRunning.store(false);
            return std::nullopt;
        }

        l_totalBytes += l_bytesReceived;

    } while ( l_totalBytes < sizeof(tcp_utils::Header) );

    tcp_utils::Header l_header = reinterpret_cast<tcp_utils::Header*>(&l_buffer)->deserialize();
    if ( l_header.magic != tcp_utils::MAGIC_CHECK )
    {
        std::cerr << "ERROR: Read invalid packet (magic check failed)" << std::endl;
        return std::nullopt;
    }

    return l_header;
}

std::optional<std::vector<uint8_t>> TcpServer::recvData(int p_clientSocket, const uint32_t p_size)
{
    std::vector<uint8_t> l_buffer(p_size);

    size_t l_totalBytes{0};
    ssize_t l_bytesReceived{0};
    do
    {
        l_bytesReceived = recv(p_clientSocket, l_buffer.data()+l_totalBytes, p_size-l_totalBytes, MSG_DONTWAIT);
        if ( l_bytesReceived < 0 )
        {
          // recv failed, handle error
            if ( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                // try again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            else
            {
                std::cerr << "ERROR: recv failed" << std::endl;
                close(p_clientSocket);
                return std::nullopt;
            }
        }
        else if ( l_bytesReceived == 0 )
        {
            // remote side closed connection
            std::cerr << "ERROR: connection closed" << std::endl;
            return std::nullopt;
        }

        l_totalBytes += static_cast<size_t>(l_bytesReceived);

    } while ( l_totalBytes < p_size );

    return l_buffer;
}

bool TcpServer::handleTileRequest(const int p_clientSocket, const tcp_utils::TileRequest &p_request)
{
# warning FARE CHECK DI VALIDITY SU LAT E LON

    // Elaborate request
    std::cout << std::fixed << std::setprecision(6) <<
                 "Received request (lat: " << p_request.getLat() <<
                 "; lon: " << p_request.getLon() <<
                 "; zoom: " << p_request.getZoom() << ")" << std::endl;

    // Search image
    int l_tileX = tcp_utils::lonToTileX(p_request.getLon(), p_request.getZoom());
    int l_tileY = tcp_utils::latToTileY(p_request.getLat(), p_request.getZoom());

    std::filesystem::path l_imagePath = IMG_STORAGE_PATH / std::to_string(p_request.getZoom()) /
                                                           std::to_string(l_tileX) / std::to_string(l_tileY).append(".png");

    std::optional<TileImage> l_tileImage = TileImage::load(l_imagePath);
    if ( !l_tileImage )
    {
        std::cerr << "ERROR: Image " << l_imagePath.filename() << " not found" << std::endl;
        return false;
    }

    // Write message - reply
    return sendImage(p_clientSocket, l_tileImage.value());
}

#warning VEDERE SE SI PUO' RENDERE TILE_IMAGE CONST IN QUALCHE MODO?
bool TcpServer::sendImage(const int p_destSocket, TileImage& p_tileImage)
{
    size_t l_imgSize = p_tileImage.size();

    // Build packet header
    tcp_utils::Header l_header{};
    l_header.type = tcp_utils::RequestType::GET_TILE;
    l_header.payload_len = l_imgSize;

    static_assert(sizeof(l_header) == 12, "Unexpected padding per request header");

    // Serialize header (trasform fields to network byte order)
    tcp_utils::Header l_serialHeader = l_header.serialize();

    const uint8_t* l_buffer = reinterpret_cast<const uint8_t*>(&l_serialHeader);

    if ( !sendData(p_destSocket, l_buffer, sizeof(l_serialHeader)) )
    {
        std::cerr << "ERROR: no header data written from server to client" << std::endl;
        return false;
    }

    // Different sending methodology based on image size
    if ( l_imgSize <= MAX_IMAGE_MEMORY )
    {
        const std::optional<std::vector<uint8_t>> l_data = p_tileImage.readAll();
        if ( !l_data ) return false;

        if ( !sendData(p_destSocket, l_data->data(), l_header.payload_len) )
        {
            std::cerr << "ERROR: Failed data writing from server to client" << std::endl;
            return false;
        }
    }
    else
    {
        while ( std::optional<std::vector<uint8_t>> l_data = p_tileImage.readChunk() )
        {
            if ( !sendData(p_destSocket, l_data->data(), l_data->size()) )
            {
                std::cerr << "ERROR: Failed data writing from server to client" << std::endl;
                return false;
            }
        }
    }

    return true;
}


bool TcpServer::sendData(int p_destSocket, const uint8_t *p_data, const unsigned int p_size)
{
    size_t l_totalBytes{0};
    ssize_t l_bytesWritten{0};

    while ( l_totalBytes < p_size )
    {
        l_bytesWritten = send(p_destSocket, p_data + l_totalBytes, p_size-l_totalBytes, MSG_NOSIGNAL);
        if ( l_bytesWritten <= 0 )
        {
            std::cerr << "ERROR: failed to send data"
                      << " (sent " << l_totalBytes << "/" << p_size << " bytes)"
                      << " errno: " << strerror(errno) << std::endl;;
            return false;
        }
        l_totalBytes += l_bytesWritten;
    }

    return true;
}


