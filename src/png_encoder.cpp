#include "png_encoder.hpp"
#include "utils/checksum.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

// Big-endian writing helper
static void writeU32(std::ofstream &file, uint32_t val) {
  uint8_t bytes[4];
  bytes[0] = (val >> 24) & 0xFF;
  bytes[1] = (val >> 16) & 0xFF;
  bytes[2] = (val >> 8) & 0xFF;
  bytes[3] = val & 0xFF;
  file.write((char *)bytes, 4);
}

void PngEncoder::encode(const Image &img, const std::string &filepath) {
  std::ofstream file(filepath, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file for writing: " + filepath);
  }

  // PNG Signature
  const uint8_t signature[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
  file.write((char *)signature, 8);

  writeIHDR(file, img.width, img.height, img.channels);
  writeIDAT(file, img);
  writeIEND(file);
}

void PngEncoder::writeChunk(std::ofstream &file, const char *type,
                            const std::vector<uint8_t> &data) {
  writeU32(file, (uint32_t)data.size());

  // Calculate CRC over Type + Data
  std::vector<uint8_t> crcData;
  crcData.resize(4 + data.size());
  std::memcpy(crcData.data(), type, 4);
  if (!data.empty()) {
    std::memcpy(crcData.data() + 4, data.data(), data.size());
  }

  file.write((char *)crcData.data(), crcData.size());

  uint32_t crc = Checksum::crc32(crcData.data(), crcData.size());
  writeU32(file, crc);
}

void PngEncoder::writeIHDR(std::ofstream &file, int width, int height,
                           int channels) {
  std::vector<uint8_t> data(13);

  // Width (4 bytes)
  data[0] = (width >> 24) & 0xFF;
  data[1] = (width >> 16) & 0xFF;
  data[2] = (width >> 8) & 0xFF;
  data[3] = width & 0xFF;

  // Height (4 bytes)
  data[4] = (height >> 24) & 0xFF;
  data[5] = (height >> 16) & 0xFF;
  data[6] = (height >> 8) & 0xFF;
  data[7] = height & 0xFF;

  // Bit depth (1 byte) - 8 bits per channel
  data[8] = 8;

  // Color type (1 byte) - 2 (RGB) or 6 (RGBA)
  data[9] = (channels == 4) ? 6 : 2;

  // Compression method (1 byte) - 0 (Deflate)
  data[10] = 0;

  // Filter method (1 byte) - 0 (Adaptive)
  data[11] = 0;

  // Interlace method (1 byte) - 0 (No interlace)
  data[12] = 0;

  writeChunk(file, "IHDR", data);
}

void PngEncoder::writeIDAT(std::ofstream &file, const Image &img) {
  // We are writing uncompressed DEFLATE blocks.
  // Each block can hold up to 65535 bytes.
  // Format of uncompressed block:
  // 1 byte header: BFINAL (1 bit) + BTYPE (2 bits) + Reserved (5 bits)
  //                BTYPE=00 for no compression.
  // 2 bytes LEN (little endian)
  // 2 bytes NLEN (one's complement of LEN, little endian)
  // LEN bytes of data

  // First, we need to prepare the scanline data with filter byte (0 = None)
  std::vector<uint8_t> rawData;
  int rowSize = img.width * img.channels;
  rawData.reserve(img.height * (rowSize + 1));

  for (int y = 0; y < img.height; ++y) {
    rawData.push_back(0); // Filter type 0 (None)
    const uint8_t *row = &img.data[y * rowSize];
    rawData.insert(rawData.end(), row, row + rowSize);
  }

  // Now wrap this in Zlib stream structure
  // Zlib Header: 0x78 0x01 (Default compression, no dictionary)
  std::vector<uint8_t> zlibData;
  zlibData.push_back(0x78);
  zlibData.push_back(0x01);

  size_t pos = 0;
  while (pos < rawData.size()) {
    size_t chunkLen = rawData.size() - pos;
    bool isFinal = false;
    if (chunkLen > 65535) {
      chunkLen = 65535;
    } else {
      isFinal = true;
    }

    // Deflate Block Header
    // BFINAL=1 if final, 0 otherwise. BTYPE=00
    uint8_t header = isFinal ? 0x01 : 0x00;
    zlibData.push_back(header);

    // LEN (Little Endian)
    zlibData.push_back(chunkLen & 0xFF);
    zlibData.push_back((chunkLen >> 8) & 0xFF);

    // NLEN (Little Endian)
    uint16_t nlen = ~((uint16_t)chunkLen);
    zlibData.push_back(nlen & 0xFF);
    zlibData.push_back((nlen >> 8) & 0xFF);

    // Data
    zlibData.insert(zlibData.end(), rawData.begin() + pos,
                    rawData.begin() + pos + chunkLen);

    pos += chunkLen;
  }

  // Adler32 Checksum of raw data (not zlib wrapped)
  uint32_t adler = Checksum::adler32(rawData.data(), rawData.size());
  zlibData.push_back((adler >> 24) & 0xFF);
  zlibData.push_back((adler >> 16) & 0xFF);
  zlibData.push_back((adler >> 8) & 0xFF);
  zlibData.push_back(adler & 0xFF);

  writeChunk(file, "IDAT", zlibData);
}

void PngEncoder::writeIEND(std::ofstream &file) {
  writeChunk(file, "IEND", {});
}
