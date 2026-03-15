#include "tcpclient.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <netdb.h>
#include <array>

#include <cstring>
#include <iomanip>      // for std::setprecision

#include <thread>
#include <chrono>

#include <vector>

constexpr int g_maxBufferSize = 256;

TcpClient::TcpClient(const unsigned int p_port, const std::string& p_serverAddrStr) :
    m_port(p_port)
  , m_serverAddrStr(p_serverAddrStr)
  , m_socket(-1)
  , m_socketAddr()
{

}

bool TcpClient::init()
{
    addrinfo l_hints{};
    addrinfo* l_addresses{nullptr};

    l_hints.ai_family   = AF_UNSPEC;
    l_hints.ai_socktype = SOCK_STREAM;  // solo TCP
    l_hints.ai_protocol = IPPROTO_TCP;  // esplicito — solo TCP

    if ( getaddrinfo(m_serverAddrStr.c_str(), std::to_string(m_port).c_str(), &l_hints, &l_addresses) < 0 )
    {
        std::cerr << "ERROR: address info retrieval failed" << std::endl;
        return false;
    }

    int l_attempts{0};
    bool l_connected{false};
    while ( !l_connected && l_attempts < MAX_CONN_ATTEMPTS )
    {
        for ( addrinfo* l_ptr = l_addresses; l_ptr != nullptr && !l_connected; l_ptr = l_ptr->ai_next )
        {
            // Create socket (end point for communication)
            m_socket = socket(l_ptr->ai_family, l_ptr->ai_socktype, l_ptr->ai_protocol);
            if ( m_socket < 0 ) continue;

            if ( connect(m_socket, l_ptr->ai_addr, l_ptr->ai_addrlen) == 0 )
            {
                l_connected = true;
                break;
            }

            close(m_socket);
            m_socket = -1;
        }

        if ( !l_connected )
        {
            std::cerr << "ERROR: socket not opened on client side (attempt " << l_attempts++ << "/" << MAX_CONN_ATTEMPTS << ")" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
    }

    freeaddrinfo(l_addresses);

    if ( !l_connected )
    {
        std::cerr << "ERROR: couldn't connect to server (IP: " << m_serverAddrStr << "; port: " << m_port << ")" << std::endl;
        return false;
    }

    std::cout << "Client correctly initialized" << std::endl;
    return true;
}

bool TcpClient::getTile(const double p_lat, const double p_lon, const uint32_t p_zoom)
{
    std::optional<tcp_utils::TileRequest> l_tileRequest = tcp_utils::TileRequest::fromCoords(p_lat, p_lon, p_zoom);
    if ( !l_tileRequest )
    {
        std::cerr << "ERROR: invalid tile request parameters" << std::endl;
        return false;
    }

    // Fare check su sizeof di TileRequest?

    // Sending header first
    tcp_utils::Header l_header{};
    l_header.type = tcp_utils::RequestType::GET_TILE;
    l_header.payload_len = sizeof(tcp_utils::TileRequest);

    static_assert(sizeof(l_header) == 12, "Unexpected padding per request header");

    // Serialize header (trasform fields to network byte order)
    tcp_utils::Header l_serialHeader = l_header.serialize();

    const uint8_t* l_buffer = reinterpret_cast<const uint8_t*>(&l_serialHeader);

    if ( !sendData(l_buffer, sizeof(l_serialHeader)) )
    {
        std::cerr << "ERROR: no header data written from client to server" << std::endl;
        return false; /*continue*/;
    }

    tcp_utils::TileRequest l_serialRequest = l_tileRequest->serialize();
    l_buffer = reinterpret_cast<const uint8_t*>(&l_serialRequest);
    if ( !sendData(l_buffer, l_header.payload_len) )
    {
        std::cerr << "ERROR: no data written from client to server" << std::endl;
        return false; /*continue*/;
    }

//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//    }

    std::cout << std::fixed << std::setprecision(6) <<
                 "Sent request (lat: " << p_lat <<
                 "; lon: " << p_lon <<
                 "; zoom: " << p_zoom << ")" << std::endl;

    std::optional<tcp_utils::Header> l_recvHeader = recvHeader(m_socket);
    if ( !l_recvHeader ) return false;

    std::cout << "Received header with type: " << l_recvHeader->type << "; payload_size: " << l_recvHeader->payload_len << std::endl;
    std::optional<std::vector<uint8_t>> l_data = recvData(m_socket, l_recvHeader->payload_len);
    if ( !l_data.has_value() )
    {
        std::cerr << "ERROR: read invalid data" << std::endl;
        return false;
    }

    std::filesystem::path l_path = IMG_RECV_PATH / "img.png";
    std::ofstream l_imgFile(l_path, std::ios::binary);

    if ( !l_imgFile )
    {
        std::cerr << "ERROR: file " << l_path << " could not be opened" << std::endl;
        return false;
    }

    l_imgFile.write(reinterpret_cast<const char*>(l_data->data()), l_data->size());

    return true;
}

bool TcpClient::sendData(const uint8_t *p_data, const unsigned int p_size)
{
    size_t l_totalBytes{0};
    ssize_t l_bytesWritten{0};

    while ( l_totalBytes < p_size )
    {
        l_bytesWritten = send(m_socket, p_data + l_totalBytes, p_size-l_totalBytes, MSG_NOSIGNAL);
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

#warning METODO DUPLICATO FRA TCP_CLIENT E TCP_SERVER, VEDERE COME MIGLIORARE (MAGARI CLASSE COMUNE DA CUI EREDITANO?)
std::optional<tcp_utils::Header> TcpClient::recvHeader(int p_socket)
{
    static char l_buffer[sizeof(tcp_utils::Header)];

    int l_totalBytes{0};
    int l_bytesReceived{0};
    do
    {
        // Client bloccato il lettura
        int l_bytesReceived = recv(p_socket, l_buffer+l_totalBytes, sizeof(l_buffer)-l_totalBytes, 0);
        if ( l_bytesReceived < 0 )
        {
            std::cerr << "ERROR: recv failed" << std::endl;
            close(p_socket);
            return std::nullopt;
        }
        else if ( l_bytesReceived == 0 )
        {
            // remote side closed connection
            std::cerr << "ERROR: connection closed" << std::endl;
//            m_isRunning.store(false);
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

#warning CODICE DUPLICATO TRA CLIENT E SERVER
#warning CAMBIARE METODOLOGIA IN MODO DA SALVARE DATI TEMPORANEI SU BUFFER, SCRIVERLI SUBITO SU FILE E POI CONTINUARE A LEGGERE
std::optional<std::vector<uint8_t>> TcpClient::recvData(int p_socket, const uint32_t p_size)
{
    std::vector<uint8_t> l_buffer(p_size);

    size_t l_totalBytes{0};
    ssize_t l_bytesReceived{0};
    do
    {
        l_bytesReceived = recv(p_socket, l_buffer.data()+l_totalBytes, p_size-l_totalBytes, 0);
        if ( l_bytesReceived < 0 )
        {
            std::cerr << "ERROR: recv failed" << std::endl;
            close(p_socket);
            return std::nullopt;
        }
        else if ( l_bytesReceived == 0 )
        {
            // remote side closed connection
            std::cerr << "ERROR: connection closed" << std::endl;
#warning VA CHIUSO IL SOCKET COMUNQUE?
            return std::nullopt;
        }

        l_totalBytes += static_cast<size_t>(l_bytesReceived);

    } while ( l_totalBytes < p_size );

    return l_buffer;
}
