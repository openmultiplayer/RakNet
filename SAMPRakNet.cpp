#include "SAMPRakNet.hpp"
#include <climits>
#include <cstring>
#include <string>
#include <vector>

#include <RakPeer.h>
#include <SocketLayer.h>
#include <PacketEnumerations.h>

uint8_t SAMPRakNet::buffer_[MAXIMUM_MTU_SIZE];
#ifdef BUILD_FOR_CLIENT
char SAMPRakNet::authkeyBuffer_[AUTHKEY_RESPONSE_LEN];
bool SAMPRakNet::connectAsNpc_ = false;
#endif
#ifndef BUILD_FOR_CLIENT
uint32_t SAMPRakNet::token_;
#endif
uint16_t SAMPRakNet::portNumber = 7777;
#ifndef BUILD_FOR_CLIENT
Query* SAMPRakNet::query_ = nullptr;
#endif
unsigned int SAMPRakNet::timeout_ = 10000;
#ifndef BUILD_FOR_CLIENT
unsigned int SAMPRakNet::minConnectionTime_ = 0;
unsigned int SAMPRakNet::messagesLimit_ = 500;
unsigned int SAMPRakNet::messageHoleLimit_ = 3000;
unsigned int SAMPRakNet::acksLimit_ = 3000;
unsigned int SAMPRakNet::networkLimitsBanTime_ = 60000;
bool SAMPRakNet::logCookies_ = false;
ICore* SAMPRakNet::core_ = nullptr;
FlatHashSet<uint32_t> SAMPRakNet::incomingConnections_;
RakNet::RakNetTime SAMPRakNet::gracePeriod_ = 0;
#endif

uint16_t
SAMPRakNet::
    GetPort()
{
    return portNumber;
}

void SAMPRakNet::
    SetPort(uint16_t value)
{
    portNumber = value;
}

uint8_t*
SAMPRakNet::
    Decrypt(uint8_t const* src, int len)
{
    static const uint8_t
        key[256]
        = {
              0xB4,
              0x62,
              0x07,
              0xE5,
              0x9D,
              0xAF,
              0x63,
              0xDD,
              0xE3,
              0xD0,
              0xCC,
              0xFE,
              0xDC,
              0xDB,
              0x6B,
              0x2E,
              0x6A,
              0x40,
              0xAB,
              0x47,
              0xC9,
              0xD1,
              0x53,
              0xD5,
              0x20,
              0x91,
              0xA5,
              0x0E,
              0x4A,
              0xDF,
              0x18,
              0x89,
              0xFD,
              0x6F,
              0x25,
              0x12,
              0xB7,
              0x13,
              0x77,
              0x00,
              0x65,
              0x36,
              0x6D,
              0x49,
              0xEC,
              0x57,
              0x2A,
              0xA9,
              0x11,
              0x5F,
              0xFA,
              0x78,
              0x95,
              0xA4,
              0xBD,
              0x1E,
              0xD9,
              0x79,
              0x44,
              0xCD,
              0xDE,
              0x81,
              0xEB,
              0x09,
              0x3E,
              0xF6,
              0xEE,
              0xDA,
              0x7F,
              0xA3,
              0x1A,
              0xA7,
              0x2D,
              0xA6,
              0xAD,
              0xC1,
              0x46,
              0x93,
              0xD2,
              0x1B,
              0x9C,
              0xAA,
              0xD7,
              0x4E,
              0x4B,
              0x4D,
              0x4C,
              0xF3,
              0xB8,
              0x34,
              0xC0,
              0xCA,
              0x88,
              0xF4,
              0x94,
              0xCB,
              0x04,
              0x39,
              0x30,
              0x82,
              0xD6,
              0x73,
              0xB0,
              0xBF,
              0x22,
              0x01,
              0x41,
              0x6E,
              0x48,
              0x2C,
              0xA8,
              0x75,
              0xB1,
              0x0A,
              0xAE,
              0x9F,
              0x27,
              0x80,
              0x10,
              0xCE,
              0xF0,
              0x29,
              0x28,
              0x85,
              0x0D,
              0x05,
              0xF7,
              0x35,
              0xBB,
              0xBC,
              0x15,
              0x06,
              0xF5,
              0x60,
              0x71,
              0x03,
              0x1F,
              0xEA,
              0x5A,
              0x33,
              0x92,
              0x8D,
              0xE7,
              0x90,
              0x5B,
              0xE9,
              0xCF,
              0x9E,
              0xD3,
              0x5D,
              0xED,
              0x31,
              0x1C,
              0x0B,
              0x52,
              0x16,
              0x51,
              0x0F,
              0x86,
              0xC5,
              0x68,
              0x9B,
              0x21,
              0x0C,
              0x8B,
              0x42,
              0x87,
              0xFF,
              0x4F,
              0xBE,
              0xC8,
              0xE8,
              0xC7,
              0xD4,
              0x7A,
              0xE0,
              0x55,
              0x2F,
              0x8A,
              0x8E,
              0xBA,
              0x98,
              0x37,
              0xE4,
              0xB2,
              0x38,
              0xA1,
              0xB6,
              0x32,
              0x83,
              0x3A,
              0x7B,
              0x84,
              0x3C,
              0x61,
              0xFB,
              0x8C,
              0x14,
              0x3D,
              0x43,
              0x3B,
              0x1D,
              0xC3,
              0xA2,
              0x96,
              0xB3,
              0xF8,
              0xC4,
              0xF2,
              0x26,
              0x2B,
              0xD8,
              0x7C,
              0xFC,
              0x23,
              0x24,
              0x66,
              0xEF,
              0x69,
              0x64,
              0x50,
              0x54,
              0x59,
              0xF1,
              0xA0,
              0x74,
              0xAC,
              0xC6,
              0x7D,
              0xB5,
              0xE6,
              0xE2,
              0xC2,
              0x7E,
              0x67,
              0x17,
              0x5E,
              0xE1,
              0xB9,
              0x3F,
              0x6C,
              0x70,
              0x08,
              0x99,
              0x45,
              0x56,
              0x76,
              0xF9,
              0x9A,
              0x97,
              0x19,
              0x72,
              0x5C,
              0x02,
              0x8F,
              0x58,
          };
    uint8_t
        cur,
        checksum = 0,
        port = GetPort() ^ 0xCC;
    for (int i = 1; i != len; ++i) {
        // Alternate the mask every byte.
        cur = (uint8_t)src[i];
        if (!(i & 1))
            cur ^= port;
        cur = key[cur];
        checksum ^= cur & 0xAA;
        buffer_[i - 1] = cur;
    }
    if (src[0] != checksum) {
        return nullptr;
    }
    return buffer_;
}

uint8_t*
SAMPRakNet::
    Encrypt(uint8_t const* src, int len)
{
    static const uint8_t
        key[256]
        = {
              0x27,
              0x69,
              0xFD,
              0x87,
              0x60,
              0x7D,
              0x83,
              0x02,
              0xF2,
              0x3F,
              0x71,
              0x99,
              0xA3,
              0x7C,
              0x1B,
              0x9D,
              0x76,
              0x30,
              0x23,
              0x25,
              0xC5,
              0x82,
              0x9B,
              0xEB,
              0x1E,
              0xFA,
              0x46,
              0x4F,
              0x98,
              0xC9,
              0x37,
              0x88,
              0x18,
              0xA2,
              0x68,
              0xD6,
              0xD7,
              0x22,
              0xD1,
              0x74,
              0x7A,
              0x79,
              0x2E,
              0xD2,
              0x6D,
              0x48,
              0x0F,
              0xB1,
              0x62,
              0x97,
              0xBC,
              0x8B,
              0x59,
              0x7F,
              0x29,
              0xB6,
              0xB9,
              0x61,
              0xBE,
              0xC8,
              0xC1,
              0xC6,
              0x40,
              0xEF,
              0x11,
              0x6A,
              0xA5,
              0xC7,
              0x3A,
              0xF4,
              0x4C,
              0x13,
              0x6C,
              0x2B,
              0x1C,
              0x54,
              0x56,
              0x55,
              0x53,
              0xA8,
              0xDC,
              0x9C,
              0x9A,
              0x16,
              0xDD,
              0xB0,
              0xF5,
              0x2D,
              0xFF,
              0xDE,
              0x8A,
              0x90,
              0xFC,
              0x95,
              0xEC,
              0x31,
              0x85,
              0xC2,
              0x01,
              0x06,
              0xDB,
              0x28,
              0xD8,
              0xEA,
              0xA0,
              0xDA,
              0x10,
              0x0E,
              0xF0,
              0x2A,
              0x6B,
              0x21,
              0xF1,
              0x86,
              0xFB,
              0x65,
              0xE1,
              0x6F,
              0xF6,
              0x26,
              0x33,
              0x39,
              0xAE,
              0xBF,
              0xD4,
              0xE4,
              0xE9,
              0x44,
              0x75,
              0x3D,
              0x63,
              0xBD,
              0xC0,
              0x7B,
              0x9E,
              0xA6,
              0x5C,
              0x1F,
              0xB2,
              0xA4,
              0xC4,
              0x8D,
              0xB3,
              0xFE,
              0x8F,
              0x19,
              0x8C,
              0x4D,
              0x5E,
              0x34,
              0xCC,
              0xF9,
              0xB5,
              0xF3,
              0xF8,
              0xA1,
              0x50,
              0x04,
              0x93,
              0x73,
              0xE0,
              0xBA,
              0xCB,
              0x45,
              0x35,
              0x1A,
              0x49,
              0x47,
              0x6E,
              0x2F,
              0x51,
              0x12,
              0xE2,
              0x4A,
              0x72,
              0x05,
              0x66,
              0x70,
              0xB8,
              0xCD,
              0x00,
              0xE5,
              0xBB,
              0x24,
              0x58,
              0xEE,
              0xB4,
              0x80,
              0x81,
              0x36,
              0xA9,
              0x67,
              0x5A,
              0x4B,
              0xE8,
              0xCA,
              0xCF,
              0x9F,
              0xE3,
              0xAC,
              0xAA,
              0x14,
              0x5B,
              0x5F,
              0x0A,
              0x3B,
              0x77,
              0x92,
              0x09,
              0x15,
              0x4E,
              0x94,
              0xAD,
              0x17,
              0x64,
              0x52,
              0xD3,
              0x38,
              0x43,
              0x0D,
              0x0C,
              0x07,
              0x3C,
              0x1D,
              0xAF,
              0xED,
              0xE7,
              0x08,
              0xB7,
              0x03,
              0xE6,
              0x8E,
              0xAB,
              0x91,
              0x89,
              0x3E,
              0x2C,
              0x96,
              0x42,
              0xD9,
              0x78,
              0xDF,
              0xD0,
              0x57,
              0x5D,
              0x84,
              0x41,
              0x7E,
              0xCE,
              0xF7,
              0x32,
              0xC3,
              0xD5,
              0x20,
              0x0B,
              0xA7,
          };
    uint8_t
        cur,
        checksum = 0,
        port = GetPort() ^ 0xCC;
    for (int i = 0; i != len; ++i) {
        // Alternate the mask every byte.
        cur = (uint8_t)src[i];
        checksum ^= cur & 0xAA;
        cur = key[cur];
        if (i & 1)
            cur ^= port;
        buffer_[i + 1] = cur;
    }
    buffer_[0] = checksum;
    return buffer_;
}

#ifdef BUILD_FOR_CLIENT
inline uint8_t transformAuthSha1(const uint8_t value, const uint8_t xorValue)
{
    static const uint8_t authHashTransformTable[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
        0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80,
        0x08, 0x06, 0x00, 0x00, 0x00, 0xE4, 0xB5, 0xB7, 0x0A, 0x00, 0x00, 0x00,
        0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0B, 0x13, 0x00, 0x00, 0x0B,
        0x13, 0x01, 0x00, 0x9A, 0x9C, 0x18, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41,
        0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8E, 0x7C, 0xFB, 0x51, 0x93, 0x00, 0x00,
        0x00, 0x20, 0x63, 0x48, 0x52, 0x4D, 0x00, 0x00, 0x7A, 0x25, 0x00, 0x00,
        0x80, 0x83, 0x00, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x80, 0xE9, 0x00, 0x00,
        0x75, 0x30, 0x00, 0x00
    };

    uint8_t result = value;

    for (const uint8_t &entry : authHashTransformTable) {
      result = result ^ entry ^ xorValue;
    }

    return result;
}

inline const char * uint8ToHex(const uint8_t num) {
    static uint8_t buffer[2];

    buffer[0] = (num >> 4) & 0xF;
    buffer[1] = num & 0xF;

    for (uint8_t &entry : buffer) {
        uint8_t result = entry + '0';

        if (result > '9')
            result = entry + '7';

        entry = result;
    }

    return (char*)buffer;
}

char*
SAMPRakNet::
    PrepareAuthkeyResponse(const char* initialKey)
{
    static const uint8_t code_from_CAnimManager_AddAnimation[] =
    {
        0xFF, 0x25, 0x34, 0x39, // gta_sa.exe + 0x4D3AA0
        0x4D, 0x00, 0x90, 0x90, // gta_sa.exe + 0x4D3AA4
        0x90, 0x90, 0x56, 0x57, // gta_sa.exe + 0x4D3AAC
        0x50, 0x8B, 0x44, 0x24, // gta_sa.exe + 0x4D3AA8
        0x14, 0x8D, 0x0C, 0x80  // gta_sa.exe + 0x4D3AB0
    };

    RakNet::CSHA1 sha1;
    sha1.Update( (unsigned char*) initialKey, strlen(initialKey) );
    sha1.Final();

    auto sha1_finalized = (uint32_t*)sha1.GetHash();

    uint32_t sha1_digits[5];
    for (int i = 0; i < 5; i++) {
        uint32_t digit = sha1_finalized[i];
        // Flipping bytes order
        digit = ((digit & 0xFF) << 24) | ((digit & 0xFF00) << 8) |
                ((digit >> 8) & 0xFF00) | ((digit >> 24) & 0xFF);
        sha1_digits[i] = digit;
    }

    auto sha1_digits_u8 = (uint8_t*)&sha1_digits;

    static const uint8_t xorValues[] = {
        0x2F, 0x45, 0x6F, 0xDB
    };

    for (int i = 0; i < 20; i++) {
        uint8_t xorVal = xorValues[i / 5];
        sha1_digits_u8[i] = transformAuthSha1(sha1_digits_u8[i], xorVal);
        sha1_digits_u8[i] ^= code_from_CAnimManager_AddAnimation[i];
    }

    for (int i = 0; i < 40;) {
        uint8_t hash = sha1_digits_u8[i / 2];
        const char* hashHex = uint8ToHex(hash);
        authkeyBuffer_[i] = hashHex[0];
        i++;
        authkeyBuffer_[i] = hashHex[1];
        i++;
    }

    return authkeyBuffer_;
}
#endif

#ifndef BUILD_FOR_CLIENT
void SAMPRakNet::
    HandleQuery(SOCKET instance, int outsize, const sockaddr_in& client, char const* buf, int insize)
{
    if (query_ == nullptr) {
        return;
    }

    Span<const char> output = query_->handleQuery(
        Span<const char>(buf, insize),
        instance,
        client,
        outsize);
    if (output.data()) {
        sendto(instance, output.data(), output.size(), 0, reinterpret_cast<const sockaddr*>(&client), outsize);
    }
}

struct AuthEntry {
    StringView send;
    StringView recv;
} static const AuthTable[UCHAR_MAX + 1] = {
    { "6C407EC29DE59E2", "D9412F235647BAA582089C6F66817F8B8811C057" },
    { "277C2AD934406F33", "132770E4744F6E78F2CBB4D3F3638EC05D7EA79D" },
    { "3A968DE22423B39", "D1080D41AD614649282887E4001C93AAEDBCA570" },
    { "479E294F68A7AB7", "D2A4C61BB6D9FAB47FF4732B70497FAD91BB7FC2" },
    { "583C3D05A2A3CCA", "CD8856BDA216335F3D275587E57D75E62FC8ED3A" },
    { "70A2762B77CD22CC", "B028F73A7B37AB5EF9B990ECA397C78841B7A086" },
    { "4EBF34953B6E1779", "45AB65DF7D64D902A3B130CEA3F9529AC4F260B6" },
    { "29B6F33709B1150", "180ED6A653ECBB56A5DC9DD5E4FD862FEE792990" },
    { "58E30DCFA7343C", "4E50D5C757887821522338185A32DC1A2B263D5E" },
    { "6F2B3B214EFD2674", "6C3F004F320AE3B70FA8BAB7EEB8D66A176497F6" },
    { "347D638A5EB52CA0", "50E366B9D4EF5EFDBFD7BC128FE9F339B19CB873" },
    { "6CF77623265513D", "EBBD29EFED75C5C5D2816F86B737FC4F5021A231" },
    { "664361377D3A1B42", "B9B72B3FA9C877AF292087F765112F859B72C64D" },
    { "49342E5C4471804", "EC664F610315BD0B049A6A42B7003BBAFF99CAF1" },
    { "AFCCAF2B1C3C57", "6E87E67009B19014A2714DB2E174474B3A5C303F" },
    { "7E9F50617E9B60F6", "90D62A04AC0B96827FB0EA67514D3B9A1E3BF91F" },
    { "1AA7B7342075E25", "BF43CD93D89C5285E27B84FE8EFB4E1AC501A3B1" },
    { "471A23BD25EA94", "A17C2F9CE17F09BA78D846F8C8CFC7F7079C23B2" },
    { "47A9A3573E92187", "AC4E454B5D51815B18867CA04560EB5AD8838D7E" },
    { "162A776E4E115434", "E188A468F6A8EE07441C4A1AC63FB29F8D53951B" },
    { "4148515F1974A6C", "E20C4BEB9A4166188C49D8AC7715E759902AF87B" },
    { "23791E662438167E", "DAC1332B09E674A4A619D5A1A1855B17A4D97F8D" },
    { "631298D109F6059", "AEF468EE84F95E064042E7B6D0DCE0608677401A" },
    { "5563C0B31DC4BF0", "99C56BD48BCD93D555D708A1D78E3F4F259B4A79" },
    { "46F3100E2B265EE3", "D8A6A2017ED4EC0717AAE74CF6409D6814E13052" },
    { "5B3060B750A695B", "77E63FDEA5EDD8B5ED8563D0F38E0E83CDECE996" },
    { "58823B662D053635", "A56B5A1E4F8938B7B02E043CD0E02E1AD92E080B" },
    { "81F76391352664", "0FDBD2E992763F49120A5B424BDE97D578303FEF" },
    { "36B56FCF5B015B90", "18CFC66AB764D902910E7A1C6300442CE501CFBF" },
    { "7D5B7C5250495603", "C6F997E2A56407A82F384DB2BB23041B1252C680" },
    { "16C823B4191F29C6", "A309F3B06CB2316FEB4A67E1D3953D63F51D00EA" },
    { "14C2153854076F07", "AD777E54C165C0A09871C37DE42EEFE09B81DFF3" },
    { "9DA7435192B51BE", "0D273519B1154DB6FA1DCF4CE2752D0A69D96218" },
    { "7B57621711AA6491", "E163DBDE328DC9A9F43060C7DF162D157B10400A" },
    { "2F7069A619321AF4", "DE4DB227F4891DB3B5C0C9ACCC7991AAD3780CFA" },
    { "2BBE2F8B1DD12E94", "8776CC093D240126E58F125BE7858E259AF8B5FF" },
    { "77F0C0F320873F6", "D94EAFF471FE5CC6976909F4DCD77AC2AA765D88" },
    { "78C17227522625B", "39E962FC90673235224A76D48C8CA530CC2B9A6C" },
    { "772747AE35C352E8", "F44374A4DBE562211BA7179F19CC8E1861220218" },
    { "27CD12F132C43B03", "1E385E9446FB0605BA96D9DB4A603304DFEAF90B" },
    { "46C421701E3E6600", "AA66E00E75CB893196A84DBC29009BF614C66084" },
    { "1B76BE366E571FD", "7365966472CDB73AC8D4E42C02838C6FFA5C991F" },
    { "78DB3B7CB98140A", "38C0E472DB303430DED2205FAE4596AF08603580" },
    { "DEF546C3D36678E", "5BCF5C415256E41CF8BD9F5BE2A42F7AD7996750" },
    { "1A6329A852B24CE6", "C2AE658442E941A1ADEFA2C96DCA9E1D3B97A23E" },
    { "5B921DED436A1B4E", "62A24EA43DC2E7FDFE89AAB5F7B587EAD0771DC8" },
    { "45B816011BB42A05", "FA52DB6492A87A0078ABD837468A4AD7B4B7E6ED" },
    { "5D5E603D1FBA74AC", "D8E1BF3412E8172CB3710A03F0CAB73E009565DF" },
    { "4519354810895EEF", "085DC16D7C4A3975337FCA16C30E785CADDFDEFE" },
    { "5374362547645B68", "B126F73BDA26767C36DF0261E10C5556B96C9379" },
    { "452BB701B5E79C0", "CA0BC7D627C3B33F1834189D9183F2F19A2D233F" },
    { "5F9A9E445282018", "18A1FC58F1A5DFBEFA1281DF46090A7A50E03C48" },
    { "177421352773A7", "D85E24B5C5E0F8E9BA5D07A261ECA3F26EE40D48" },
    { "FB811ACFC934A4", "2731E85667DF3DF9255EEBEC8FB1D4830939B8EA" },
    { "4E14338441A1166", "2E680524E8873363574778D78EFFE8C4D690D7A6" },
    { "785E1C24709234CA", "940F34C2D41245E4524E1D5434463B5F8AECEE76" },
    { "3041144A3113D5", "457B799050C64D09B68E2FDDB2F40BD0F7216A30" },
    { "5F3B2CE479CF3E9A", "F191F014E3204211EBB782445473F6637C277EDC" },
    { "8C8680A23AD385E", "D2BC447A6745048E7E91E6A9A573773B406C0EB6" },
    { "75A768FCD4F1AFB", "15E28C2087874A7076425385572F92817436F8C3" },
    { "4C8D7B02158E982", "89A10043BFA4FBAEC3FB4C4B8B30FC00B51C5EEF" },
    { "231A536D72E93624", "B8EA6126891D8ED83D8716DAE90D9D297BBF6220" },
    { "1B0C37D86C1C7F53", "50100256B8B3B827E2932017333B11F87390D3ED" },
    { "5FB359AE7EF46824", "CFC5A84B3CF3D04D1FDEE82B91972B90A5D8EAEE" },
    { "17A5AED69616FF8", "F21D89EB25B1263DE5871A60E9407C7A98D0381D" },
    { "1EB2302948CDD5B", "674AF510B26CB1F8E1835B396945C911CDB0F7F0" },
    { "1C1A13D343AC502C", "419C59FE61200D6E59F0774C7E4A0B9FEB0D8543" },
    { "48FE7EBB71546EFF", "A13E3C1959CB65EF9E524BD24F0FDCF8DD11DA1B" },
    { "7B1539D4740F43BF", "FC4F1E495122C86D1605F2C720D307A4F23DA704" },
    { "47A05D37796F7B98", "FB508D6CDDAFE62F088ECC07834BA165A6D9484B" },
    { "52A0106B14E27E13", "AFD221F0E28AB16BF0F3E9570034AB8F949F10FB" },
    { "1E461FE428864F83", "45841042BC967C22FDAE981407BBB57A7081DD45" },
    { "6EABBC9503F72A1", "B0BF1A5B7A3A0586917A84315D210B7DDB652FBC" },
    { "15C173FB306D73", "B7E04B553D665F0C4F59542491B45B9D2EB47248" },
    { "3B8A5525E845572", "FDAD5B7B2C5DC84BE49F07FD03C80EF634EA9C82" },
    { "76874C2A50DF6EB", "F8B0FD3504390C79256C984B7BADA1806D94EE3E" },
    { "4874101E14D46BA6", "87F8E07CE86EBCDD2B98E949F0C3CE79A7258DD7" },
    { "53321D13340634C6", "D64EC2AF19D96CC95D97CFF37C88463DC555EB1B" },
    { "5A051D793952BF0", "152AB05E3464C03EA308DF51B6D4771FAA8E2F8B" },
    { "62FF69CD4EF37FAF", "0DB9491C3D55DED81E3A3C72E40E88C8919758AE" },
    { "CBC716224F32F17", "B8F3114F4DA2779075CBA5574D4B6BDDEEC3CF7D" },
    { "6C5020627B2358A9", "BC293BD1438D6FB4C10D1A5BC778A88B9BD6C99D" },
    { "77E670F1B5C7077", "CA08EE157A0B0A775C3FC516762EE4EDBE90A0B3" },
    { "3F2B464FF972189", "918A09C3659C45882555011270B8460CE6EC9F92" },
    { "3E1156825E7E", "492927CD4E61C8BB6FEF9DF926C1C2D9EC925359" },
    { "3FB8750837546576", "F9E62B21E68DEF3CA5304096E80A45764A6CBAA6" },
    { "43A26388415D3B30", "93369612310EB002BFD93634D17B7A0BC929B30D" },
    { "C6267157E16170", "7AFAF784571E4BEA4CE07906B62A2D5F2D04C03B" },
    { "2D415D4011A64CA4", "E60B8A2D516AA68759272E807052FD3862D7B75C" },
    { "6E4751667CC5CC6", "76679E3836E8EAED90B6F10B121B52FE4F03339B" },
    { "45F1793655D5D84", "B8EDB6CC12E29A032A80A5F98D695B182B32556D" },
    { "7D056341525519F", "8143B0D90476820C30FBACD470D2C433B229CB84" },
    { "4956553B1C4D91", "CA8F2BD50C66A24FDC14506452DE038FC119D40A" },
    { "2B1A3B1E1C016572", "D4F20851FBB40B1606436C0B9D3B470DCC5743B5" },
    { "5E11B221B61721C", "96FD906C1EABF6570C4E43B5C1D31149CDB09C85" },
    { "6E8438D92CF9318E", "3411A0F24E8F3B93BC5E6533044B8423B86A2CDF" },
    { "6FA024E948068E", "C6183E48AADE981C56FE4F73AAE7CA43637B8EBA" },
    { "B8E17103A85B93", "21334344D203C35901E4793CB60F55C04FC89058" },
    { "1A9629656BAE6CE1", "6BA21F9467E0D25D2A940E5B8332367260799462" },
    { "595F58E14D307DF6", "358D0593F129EF431EDA37C1B93478A39CA49846" },
    { "3BAA6F1D2A407E25", "1B08562483516AD6B34E38131DC148742685D8AB" },
    { "5740395D7EB66081", "50132A03C5931C67565600527B145BF736F03238" },
    { "7B2930CE22CA2AFC", "EFCB215CAF75CCECE4925FBA0192F6F6ACC9F9D6" },
    { "471F780C3BE2D0", "E2089098F1CE0AB3729F62AFAE75F29EE5B88624" },
    { "574351E72FA82C1E", "A5737572A642FDF51B111E04E4E2F48C3336C4AE" },
    { "580E75674D606AD5", "92CDAB5A10FAFB4702DB000171DB011451F6C504" },
    { "1689630E21792D6", "EBF13AE17C7EAF5903E938B21519C138B6109E28" },
    { "60C11F5C27616156", "AD3D181D621E4778E4CE285FE5D177A31375F947" },
    { "4A7576994D9E4983", "2F2E103ECAD7EA2E5EADC0DD12DDD05898C934B4" },
    { "2A132DCE1B566A", "7AC50947A514153C84515C96C8B66354B11745C4" },
    { "61E345152CBF5418", "5CDB078FCFEB311A1B174A9EA47B6BA8875A0CA3" },
    { "49812F12764433FF", "A3D6F3D6B29810463835A320840D9E8482560BF1" },
    { "4B9021C2947209C", "91A4F3384BE48B758F14181BE065B4E523EA20E3" },
    { "BAD6473479B7457", "DF1753449635CD25656BCB47933BF05ABDF952C5" },
    { "28A23F143DE317AB", "334E33CEA02105B5AB9DD4290D20EF1CE2FD7955" },
    { "6EDC6EAF1760A86", "7F370CE0F99A3CAA9929A915DAF9175B7EC5437D" },
    { "F2043344071CF3", "3958D8F27C3E2CCF23DF30EF9826B8B4AE012345" },
    { "2D794A7844D71AF6", "7CE44A1AB70E70334A040BB97948F90F7529CA08" },
    { "5C752C7D52696FB7", "64EAD04A4FD68F0DB9A33F87FC3167C666BCD32C" },
    { "58901CF451C93E3", "A24C762722180B42D75D32641BA1F5BD5705498A" },
    { "17F16004B845B50", "C460C7370F9630FDC5DCE592E9D4673CBA0FA769" },
    { "254503D590235E0", "A7BF359223A37F0A3455636EBCF249E92E8A7EE8" },
    { "65486C013C1637A7", "C026C91160AFA9B715D50328CE3B71C802E59EEA" },
    { "789A7FE214D45C50", "3F22A66F651B682C24DEC04FE0F4006E8CFD154D" },
    { "711607829A339BC", "24035C6EC8AF3CEE8C58F90249C717BFE51D8338" },
    { "F5B3564759355ED", "752C905020077D8F524A8616402E12DDCBDE351C" },
    { "71486F9070F01423", "34C6031CEE4EE12BD62EB2F3472D56CA74E34D0F" },
    { "7B3674B948177D44", "1AD6B4FCEC9D0D05B4EFEFA11200B4D9892C6895" },
    { "6BCD242B74886781", "03679F4624B21CF0979722545B783845E3398306" },
    { "3BEC18CC6C3D4137", "ADC16DB34679B031D4B5BBEE184B50083BB9B8A0" },
    { "44E24C5C5B3A313", "5CFA7F8A5334D8F5AE4867A28A25586AEC4AAF4F" },
    { "16E27FFB1B66D7A", "4454AF95AF5B8460D4D5BD1798E2656ED174FB2F" },
    { "13B1DEC5E997528", "453B2549AF80225BC7F638DE7276DD0857B8032F" },
    { "21A28983EF6317", "35E8537F84FD7C1F0E2D5C3A1B50C39C6FFD34C3" },
    { "AC44AD6297B6BA2", "64CBAFE90E809320092A1FB98E1A14572C50CFEF" },
    { "5C5A5D6956D6EA", "73FA1B06A7159BF92AFB50FA455A0B82A28FF548" },
    { "5A8B55CA467A6479", "1A1C51732728D88F55C8AC2755E16B2C72081383" },
    { "5C5B12296FF37C25", "25DFE45F13CEC422AF94A4F8C4D5933CFBD855B9" },
    { "23DA76AE1E762E37", "C92B05E20B8728F1C9A337920EE73A9810CBAF2E" },
    { "69B10042ABD28CD", "FD7E61C06C9036F8845E982C8560964B8A143E87" },
    { "7A683F18712387D", "53BB915F5ECEED59F0C92AB903135E9FDF60BF6C" },
    { "5A34521E51A9723B", "F80C175C517ADDAF3C70E4F3F884026081A0B0EC" },
    { "77512ED66132347D", "F835ADB252311789710633D89D13AEF8742D342A" },
    { "4AE4720D7FAF439D", "63D38A575B69E435526A49EB0B2BF5376A903A7C" },
    { "5B952866478357E", "8FD27820677C48DF887A94BE76147ADACFE32513" },
    { "2B8B45A56CC2072", "259F8C16BA6468B3BE6C4456E5ECAE85574CE4FB" },
    { "4297357E4CF315B", "19B390F6FE5CB0192C289F53706C1542FB0A0D39" },
    { "28AF45044682D11", "E3926F6FFC7DDF8AEE8C6F41FFE706302A1470CC" },
    { "649F1B854B35104", "6E0893E674F14528FABB970FCF592E3F94C3DCFA" },
    { "52FB13FE62534723", "0A392FB24304303AE7DF4C83BAAADBFAF9E394BC" },
    { "69584018413E40FC", "C341AC8EAE4E3433FA4F63F7A4521FBEC4E33BB0" },
    { "39BB41AC48494E24", "334A90A161C6774DACA6C7C5E2D920D85D236056" },
    { "4A525D8641CB11D9", "6DAF4BB4110666179DD2278C364D544955CF8F67" },
    { "15622B6917723BE6", "2B25F4B09D664BD72971367601F92109D3B6C7AB" },
    { "458175471D958C7", "0C98F3E92244EA1C40E537D706AE3956BAAA76DE" },
    { "2052804765C6D0E", "E15406041F2A256A2209B47539E3FE5A5793CC39" },
    { "5FBC7EBA172B6005", "2DE29755238B3769C6565C379652B98C1BE612B4" },
    { "49DE443E299D7995", "FCC9B7CE8F1249F82BF6E98A48EED5393110090F" },
    { "773F572436C77766", "1BFA8AD640401259216B04A43F160A6810866163" },
    { "3FC94F4E3A4E7D48", "2393D160295BD5A03F42CA1D1932C0D8DE940FC9" },
    { "662A6AB2437964D2", "144A22F14F2EA79A588D22D78B3F1E462D152896" },
    { "29CA375D2C8A3048", "C848B7B9C0F4B31A84926C0A48E3A012E2FAB253" },
    { "35001EB85C4F24C2", "0EC322D4E644AB352B8C990EC66F9D6C707C8A1A" },
    { "3B85360B45F44F8B", "6A244AC02399BAC7D4ABCB6B87199AEF377B0177" },
    { "5D29783F1B1B7AC8", "6BE2A3D98D0A8C09417F42CBDDC358219EB33C92" },
    { "20C44CE8642E555A", "97E4E8B37712D2E6D9E0BEF95CD95951116381FB" },
    { "1B708372F45205", "AF76765DFA4A47E500092C55101D73CD31ECFB89" },
    { "17F511FB63631D1", "A55ACFA72989539D611ED1C03481C531D2689A7A" },
    { "528579731EB66DB0", "C51B71C95640001B3A91ADB393C90EEB3C60E7DB" },
    { "1CAA4C3F14BF3362", "80DD8A137A0F8D2B3027DAE1D7B0F2A22DC02BB3" },
    { "7D7C62331F7D6996", "95260B015F7763AA5111E2460BE7DAE82964C709" },
    { "2217602144F1F52", "AAF19B7400E7C12EFB19E0D58F300A9A51B7ADAB" },
    { "324C4B9D4B2EA94", "88E9316C6F822752A71A3337D90F49D29AD47298" },
    { "5D97C0352B07A3A", "E66595B115971A6AF745938D5D426AFBCD9FEA6D" },
    { "34517CBC6AEE5F1F", "246E6FF71D6CA74ADD99363E464A8F30BEEC04AC" },
    { "212774BC55326087", "E72876080826370E4348ED487BAD25175893AA48" },
    { "16CB674F69A92FBD", "FF802A0B2398CCCD3930E8C87F1C8A8DFCB82011" },
    { "45E921652B64EFA", "433558B16219F93CCD61E4CC8DE618DE407607C1" },
    { "6F5D6C4C128C3F89", "5444893CD7F73746EFAD8BF8133E5D57AAE6927B" },
    { "26A3B4E6FC5B29", "4E186672D64DB2A8028F0AEB6D3A767BB1F59CD1" },
    { "188F705B5F8B5CB3", "375D20FBD0A05DECCAB08367E15E5FA5A77A3729" },
    { "2A3E539839C85BFC", "9383AB215ED09F2B996F7DD905E6C69C4D6F734B" },
    { "44C405757DD40FD", "3FA7B99D35480465EC276CBA5B287595BE34BE98" },
    { "5E47569825637231", "B105A621B98C4DCEC677119FF8FB02237B662156" },
    { "1E6445D07E7B423F", "43A554222D382FA10936FA1A296A47CCF7A4726A" },
    { "217164EA6D1A60D9", "1ADFA13B14C69694CE4DA9F94EF6E5AF83BBF020" },
    { "3F0BB8C50A452E4", "8D684C13AC99106FD8FEA3E5626C12A6B97CD01A" },
    { "5E11109B23BD35D9", "269D40DF5E98637F665FFDEA8532ED5E02F360DB" },
    { "2D591201545C526B", "73D1386CBC760FA57BFD1504E47FDEA1F1EA59C6" },
    { "54A169AE6220426A", "74E2C35A7E54AFAC89209B4D0579877F5DA98F13" },
    { "31C873DC36E82DE7", "2C1BE06F66EF376632810A872519BFD18F2CEA6F" },
    { "763EA986D9E6399", "BA59F7B390B0D49CFC7E12D4A835AA337DF01F5E" },
    { "38B95A7E7B53507F", "7C322DB43AC0B97EBEA4881EF138B9DC528DC346" },
    { "4F2974C46E911AC7", "19C722C8D0EE4D1D228019AD003FBCF173AB05A3" },
    { "4EF427A38EC63EF", "83EDD7778BFB69DA52B86B3B5767726DD204DB34" },
    { "1493D0F36DB762A", "8426C0C07C699A93B12CF0B53661541726F6A555" },
    { "7211101869C66105", "59C0C177142EDCA420714A730E662CA182EAD9AC" },
    { "4EA884D1860494A", "B51D6195915BBA8A5CAC1581C6423B83E1B9D21B" },
    { "472D73D449377024", "50018A8C9C4551E01BE38DC675460357544E28B3" },
    { "33F257C34B8A3585", "FBC5E2EE7F584506302A9AC668275D84079AC51E" },
    { "19301EE0425A19C5", "98DCACE879EA22A55AAEB4DF557F0112750538C0" },
    { "12F925AB65C328C", "4867AEC4BC3405FEB6D1D612F0FE5FBACED0FF64" },
    { "3D6F479E6E8D46F1", "56EF5FC757D8080D0C33C00C2F96FAA00948734D" },
    { "6D3451B35FFF46E3", "9EDF0B6A2D06A6BB4DD592CC4FB910F5C071BF5B" },
    { "4C217E2723F311C8", "9C6AE1A930350CB45517994FC5B3D918F5003A96" },
    { "2E3B5D7D2D306065", "84B3C7FD6C1A4E8FC59297ECDFC286C298C246A5" },
    { "32E250C713F365FE", "5F574D100C5906595F1A53E1CAEDAE8B7D75321E" },
    { "64C792160CF3CBE", "52E6215ABA22F11801D868BEE5E0D1C706371D31" },
    { "5732407E79B77159", "C61CAD6746318287AF0BF5B2003868AF6982003C" },
    { "54C95FA7655A71EF", "5E5DE0E176014FEF75047CF57C62610D59F2AF59" },
    { "48F458058AF6434", "3480702908DB81BEBD2FF6DB0A7A3A276DFAB13D" },
    { "22B838D42D026CE", "029A6C2159DF3195513211DD5B7331A580AEA502" },
    { "4F023B64B0B1FE", "8521241F6EC0D396A9D50C1842DDBB9109167565" },
    { "2D42B4775364B83", "6F748D92EC9D010F16AEFB57759C678B4060C7E5" },
    { "47514F3B60441BB", "0A5C6E3E271360346C6FB1BF9E580EDC758525AD" },
    { "341DBA5316220C", "7A11536F824A625D4D88CC5EFC754F52F6C86AEC" },
    { "6E2820E0B933E82", "173AF0F1EE5EB356140E40182583F8C4E6C873E3" },
    { "20817EC03FB16B9", "5DEEBFA4CB6E03CD16C063E211D20640FACA4B4A" },
    { "6BE71AA9227837FE", "6DDFA1F4595A70906C71958B9CBF06582ECC8D3D" },
    { "46BE7BA86F768B1", "C1B8BDC35596EFB40826BE0DF6A7303A1459AD72" },
    { "49E67C4D6B3562ED", "78AF9E59782B0F590464CF9936138798B5F2CF48" },
    { "29A527B2491D226C", "D9C08A2FA2D0AC2A8FE53CCEBD4F2EC31474B9BA" },
    { "3AE167BA3B5149A4", "7704AC660058B05F1A3ECC308FA91A18FFFF84F9" },
    { "589368974C076236", "E4F9B6827BE31BDE47454E28560A9FAD8E3CDBA0" },
    { "3D78558F26167C89", "3EEC56001D9EA511C1F4CC8BBE830BC47BD8BB0A" },
    { "260A52FF5C5072B2", "9A05010BF879AD36D43327DAD911382D50903FF7" },
    { "4EAE59A03B135298", "B491A64DE34519D96E0D428880AD86B0B0F0735F" },
    { "222F5709581B3355", "45B0A24B64A678C5B6A434FCE00016C49A1C75F6" },
    { "2C6B3775549A79DF", "66B37BEFCF9399108C27DC72208E00AB827A7DD2" },
    { "254C3CC758B50E7", "193BC88CFAE3B57050BEAEA9E8AFA097BF8F3B5C" },
    { "27FA46CB76435801", "6D3B25F5B6A0DFD6146AF305781D34E57C5D682C" },
    { "6A4F70BF79494E06", "1A29C1131308250744818611B8329117EE594C1B" },
    { "88C28123B673AB6", "133F545A93DAC8A15DB200C6C0A21090D38541A0" },
    { "394A206BD015BA2", "C2F2FDF47C6BD12310FCDB3B564A5AAA4EE4C627" },
    { "7DB348ED55A6584A", "EE1869A8D0AF990C1D58EF7C683CC6E23BCF0BB1" },
    { "21F126BA65E5182", "E239256C1CB03967F46C9B24005FC8C0E20B1A0D" },
    { "21E738B51B62201A", "E88733B15DC9396B1B20EF094771CF44C818A69A" },
    { "461F488DB2916BD", "24BA1E079FF6BB3CF7F57DC63E1FBCA5C1C6A2F8" },
    { "D044CF726415B1A", "69FC69EE7C95738F917AEA73D1FB6B907AD91953" },
    { "3452413AC85A42", "D66A8D7BC90A297726DFED969FDAA6EF19939864" },
    { "76CA15F11AF1C51", "CB2084E4290C216DF0D0AD8A445A362F8799648A" },
    { "522A1F0C38B67B51", "2CB76D8A0993DE62D0C108D6814D4F7DE0D4C8D1" },
    { "695C2319695A15B", "F097275236912E370955E54509C0E554D28A45A5" },
    { "56EE6FC263FC21FF", "84DE0DD44B8155E58D8E324CBDE2D69E6FC6DD21" },
    { "3C2179617B442E7", "5BCA55150CE749AED97A0CE7B1A7EB205735E7E7" },
    { "56073D0C3E9E7B6", "E4CAADA710179666EA035F509681F9B8DE7FFCFC" },
    { "4C67EC424741536", "B64DC756FD081BBC106EA069708B3C6596222898" },
    { "598259113B62FB3", "BF6A3C57226D5BE61C0C20FD17FC935E37C57E64" },
    { "139C50B5731558A2", "982B1A75CCB87E2AD7EAB23F149A7FEB785BE875" },
    { "C927AEB6164F87", "D830239C33F736942B7FF97B7B12137465CEC08E" },
    { "661356AB31561A15", "74A8F21FB34D6A05C1C08B6BB8DE5712913E8A54" },
    { "760C2B29A0A3796", "0C2B97FC56DA10ADBB6D47A4F079ECDF7EA8BB95" },
    { "F641F9F65B8638D", "369C3F95D1A57DA57655B39807C47D685AA8BFC7" },
    { "42EA4C892B9A", "E34C0A3756936F4852CC6EC0035F5EF66562D477" },
    { "2C8C54BCE045C9F", "B62D5A37BC45ECC0C374C4905629F0D9392715C1" },
    { "20C92B4B3F892B21", "282B4A757C5C4E1DDBFA0D1C39EA53576AB91399" }
};

Pair<uint8_t, StringView> SAMPRakNet::GenerateAuth()
{
    const uint8_t rnd = rand();
    return Pair<uint8_t, StringView> { rnd, AuthTable[rnd].send };
}

bool SAMPRakNet::CheckAuth(uint8_t index, StringView auth)
{
    return AuthTable[index].recv == auth;
}

uint16_t cookies[2][256];

void SAMPRakNet::SeedCookie()
{
    for (int i = 0; i < 256; ++i) {
        cookies[0][i] = rand() % (USHRT_MAX + 1);
        cookies[1][i] = rand() % (USHRT_MAX + 1);
    }
}

uint16_t SAMPRakNet::GetCookie(unsigned int address)
{
    const uint8_t* addressSplit = (const uint8_t*)&address;
    return (cookies[0][addressSplit[0]] | cookies[1][addressSplit[3]] << 8) ^ ((addressSplit[1] << 8) | addressSplit[2]);
}

bool SAMPRakNet::OnConnectionRequest(
    SOCKET connectionSocket,
    RakNet::PlayerID& playerId,
    const char* data,
    RakNet::RakNetTime& minConnectionTick,
    RakNet::RakNetTime& minConnectionLogTick
)
{
	if (playerId.binaryAddress == LOCALHOST)
	{
		// Allow unlimited connections from localhost (testing and bots).
	}
	else
	{
		RakNet::RakNetTime tickTime = RakNet::GetTime();
		if (tickTime < gracePeriod_)
		{
			// Allow unlimited connections during the grace period
		}
		else if (SAMPRakNet::IsAlreadyRequestingConnection(playerId.binaryAddress))
		{
			return false;
		}
		else
		{
			RakNet::RakNetTime configuredMinConnTime = SAMPRakNet::GetMinConnectionTime();
			if (minConnectionTick && configuredMinConnTime > 0 && tickTime - minConnectionTick < configuredMinConnTime)
			{
				if (!minConnectionLogTick || tickTime - minConnectionLogTick > configuredMinConnTime)
				{
					core_->logLn(
						LogLevel::Warning,
						"Minimum time between new connections (%u) exceeded for %s. Ignoring the request.",
						configuredMinConnTime,
						playerId.ToString());
					minConnectionLogTick = tickTime;
				}
				return false;
			}

			minConnectionTick = tickTime;
		}
	}

    if ((*(uint16_t*)(data + 1) ^ 0x6969 /* Petarded [S04E06] */) != (uint16_t)(SAMPRakNet::GetCookie(playerId.binaryAddress)))
	{
#ifdef _DO_PRINTF
		if (SAMPRakNet::ShouldLogCookies())
		{
			core_->printLn("%s requests connection cookie", playerId.ToString());
		}
#endif
		char c[3];
		c[0] = RakNet::ID_OPEN_CONNECTION_COOKIE;
		*(uint16_t*)&c[1] = (uint16_t)(SAMPRakNet::GetCookie(playerId.binaryAddress));
		RakNet::SocketLayer::Instance()->SendTo(connectionSocket, (const char*)&c, 3, playerId.binaryAddress, playerId.port);
		return false;
	}

    return true;
}
#endif
