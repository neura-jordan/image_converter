#ifndef CHECKSUM_HPP
#define CHECKSUM_HPP

#include <array>
#include <cstddef>
#include <cstdint>

class Checksum {
public:
  // CRC32 implementation (standard polynomial 0xEDB88320)
  static uint32_t crc32(const uint8_t *data, size_t length) {
    static const auto table = generateCrc32Table();
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
      uint8_t index = (crc ^ data[i]) & 0xFF;
      crc = (crc >> 8) ^ table[index];
    }
    return crc ^ 0xFFFFFFFF;
  }

  // Adler32 implementation
  static uint32_t adler32(const uint8_t *data, size_t length) {
    const uint32_t MOD_ADLER = 65521;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < length; ++i) {
      a = (a + data[i]) % MOD_ADLER;
      b = (b + a) % MOD_ADLER;
    }
    return (b << 16) | a;
  }

private:
  static std::array<uint32_t, 256> generateCrc32Table() {
    std::array<uint32_t, 256> table;
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (int j = 0; j < 8; ++j) {
        if (c & 1) {
          c = 0xEDB88320 ^ (c >> 1);
        } else {
          c = c >> 1;
        }
      }
      table[i] = c;
    }
    return table;
  }
};

#endif // CHECKSUM_HPP
