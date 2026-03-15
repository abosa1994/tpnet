#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <netinet/in.h>
#include <cstring>
#include <cmath>
#include <optional>

namespace tcp_utils
{
    constexpr uint32_t MAGIC_CHECK = 0xA72E9BC1;

    inline double htond(const double p_value)
    {
        uint64_t u;
        std::memcpy(&u, &p_value, sizeof(double));
        u = htobe64(u);
        double result;
        std::memcpy(&result, &u, sizeof(double));
        return result;

    }

    inline double ntohd(const double p_value)
    {
        uint64_t u;
        std::memcpy(&u, &p_value, sizeof(double));
        u = be64toh(u);
        double result;
        std::memcpy(&result, &u, sizeof(double));
        return result;
    }

    enum RequestType : uint32_t
    {
        GET_TILE,
        DELETE_TILE,
    };

    struct TileRequest
    {
        static std::optional<TileRequest> fromCoords(const double p_lat, const double p_lon, const uint32_t p_zoom)
        {
            // check su p_lat
            // check su p_lon

            return TileRequest(p_lat, p_lon, p_zoom);
        }

        static std::optional<TileRequest> fromBuffer(const uint8_t* p_buffer, size_t p_size)
        {
            if ( p_size != sizeof(TileRequest) )
                return std::nullopt;

            TileRequest l_request;  // default constructor privato
            std::memcpy(&l_request, p_buffer, sizeof(TileRequest));
            return l_request.deserialize();
        }

        inline TileRequest serialize() const
        {
            return TileRequest(htond(lat), htond(lon), htonl(zoom));
        }

        inline TileRequest deserialize() const
        {
            return TileRequest(ntohd(lat), ntohd(lon), ntohl(zoom));
        }

        inline double getLat() const { return lat; }
        inline double getLon() const { return lon; }
        inline uint32_t getZoom() const { return zoom; }

    private:
        TileRequest() = default;
        explicit TileRequest(const double p_lat, const double p_lon, const uint32_t p_zoom) :
            lat(p_lat)
          , lon(p_lon)
          , zoom(p_zoom)
        {

        }

        double lat;
        double lon;
        uint32_t zoom;
    };

    // Tutti campi a 32bit per eliminare alla radice il problema del padding (potenziale interpretazione diversa in base alla macchina client e server)
    // Con uint32_t il range è 0 - 4,294,967,295. Contando che la dimensione del pacchetto inviato è molto minore di 1GB, questi circa 4,3GB sono più che sufficienti
    struct Header
    {
        inline Header serialize() const
        {
            Header l_networkHeader;
            l_networkHeader.magic = htonl(magic);
            l_networkHeader.type = htonl(type);
            l_networkHeader.payload_len = htonl(payload_len);

            return l_networkHeader;
        }

        inline Header deserialize() const
        {
            Header l_hostHeader;

            l_hostHeader.magic = ntohl(magic);
            l_hostHeader.type = ntohl(type);
            l_hostHeader.payload_len = ntohl(payload_len);

            return l_hostHeader;
        }

        uint32_t magic{MAGIC_CHECK};
        uint32_t type;
        uint32_t payload_len;
    };

    // Conversione lat/lon → tile coordinates (standard Web Mercator)
    inline int lonToTileX(const double p_lon, const int p_zoom)
    {
        return static_cast<int>(
            std::floor((p_lon + 180.0) / 360.0 * (1 << p_zoom))
        );
    }

    inline int latToTileY(const double lat, const int zoom)
    {
        double l_latRad = lat * M_PI / 180.0;
        return static_cast<int>(
            std::floor((1.0 - std::log(std::tan(l_latRad) + 1.0 / std::cos(l_latRad)) / M_PI) / 2.0 * (1 << zoom))
        );
    }

}

#endif // COMMON_H
