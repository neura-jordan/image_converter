#include "jpeg_decoder.hpp"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

const uint8_t JpegDecoder::ZIGZAG[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

// BitReader for JPEG (MSB first, handles 0xFF00 stuffing)
struct JpegBitReader {
  const uint8_t *data;
  size_t size;
  size_t pos;
  uint8_t currentByte;
  int bitsLeft;

  JpegBitReader(const uint8_t *d, size_t s)
      : data(d), size(s), pos(0), currentByte(0), bitsLeft(0) {}

  int readBit() {
    if (bitsLeft == 0) {
      if (pos >= size)
        return -1; // End of stream
      currentByte = data[pos++];
      if (currentByte == 0xFF) {
        // Check for byte stuffing
        if (pos < size) {
          uint8_t next = data[pos];
          if (next == 0x00) {
            pos++; // Skip stuffed byte
          } else if (next >= 0xD0 && next <= 0xD7) {
            // RST marker - handled by caller usually, but here we might just
            // consume it? Actually, RST markers align to byte boundaries. For
            // now, let's assume we don't hit them inside bit stream or we
            // handle them. If we see a marker here, it's an issue if we are
            // expecting bits.
          }
        }
      }
      bitsLeft = 8;
    }
    int bit = (currentByte >> (bitsLeft - 1)) & 1;
    bitsLeft--;
    return bit;
  }

  int readBits(int n) {
    int result = 0;
    for (int i = 0; i < n; ++i) {
      int bit = readBit();
      if (bit == -1)
        return -1;
      result = (result << 1) | bit;
    }
    return result;
  }

  // Align to byte boundary (for RST markers)
  void align() { bitsLeft = 0; }
};

Image JpegDecoder::decode(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::binary | std::ios::ate);
  if (!file) {
    throw std::runtime_error("Could not open file: " + filepath);
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> data(size);
  if (!file.read((char *)data.data(), size)) {
    throw std::runtime_error("Failed to read file: " + filepath);
  }

  // Basic validation
  if (data.size() < 2 || data[0] != 0xFF || data[1] != 0xD8) {
    throw std::runtime_error("Not a valid JPEG file (missing SOI)");
  }

  int width = 0, height = 0;
  std::vector<QuantTable> quantTables(4);
  std::vector<HuffmanTable> dcTables(4);
  std::vector<HuffmanTable> acTables(4);
  std::vector<Component> components;
  const uint8_t *scanData = nullptr;
  size_t scanDataLen = 0;

  parseSegments(data, quantTables, dcTables, acTables, components, width,
                height, scanData, scanDataLen);

  if (!scanData) {
    throw std::runtime_error("No SOS marker found");
  }

  Image img;
  img.width = width;
  img.height = height;
  img.channels = 3;
  img.data.resize(width * height * 3);

  // MCU calculations
  int maxH = 0, maxV = 0;
  for (const auto &c : components) {
    if (c.hSampFactor > maxH)
      maxH = c.hSampFactor;
    if (c.vSampFactor > maxV)
      maxV = c.vSampFactor;
  }

  int mcuWidth = maxH * 8;
  int mcuHeight = maxV * 8;
  int mcusX = (width + mcuWidth - 1) / mcuWidth;
  int mcusY = (height + mcuHeight - 1) / mcuHeight;

  JpegBitReader reader(scanData, scanDataLen);

  // Helper to decode one symbol from stream
  auto decodeSymbol = [](JpegBitReader &reader,
                         const HuffmanTable &table) -> int {
    int code = 0;
    for (int i = 0; i < 16; ++i) {
      int bit = reader.readBit();
      if (bit == -1)
        return -1;
      code = (code << 1) | bit;
      if (code <= table.maxCode[i]) {
        int idx = table.valPtr[i] + (code - table.minCode[i]);
        return table.huffval[idx];
      }
    }
    return -1; // Not found
  };

  // Buffers for MCU blocks
  // We need to store full MCU data to handle upsampling
  // Structure: components[i].blocks[v][h][64]
  struct ComponentData {
    std::vector<std::vector<std::vector<float>>> blocks; // [v][h][64]
  };
  std::vector<ComponentData> mcuData(components.size());
  for (size_t i = 0; i < components.size(); ++i) {
    mcuData[i].blocks.resize(components[i].vSampFactor);
    for (int v = 0; v < components[i].vSampFactor; ++v) {
      mcuData[i].blocks[v].resize(components[i].hSampFactor);
      for (int h = 0; h < components[i].hSampFactor; ++h) {
        mcuData[i].blocks[v][h].resize(64);
      }
    }
  }

  for (int mcuY = 0; mcuY < mcusY; ++mcuY) {
    for (int mcuX = 0; mcuX < mcusX; ++mcuX) {

      // Decode MCU
      for (size_t i = 0; i < components.size(); ++i) {
        Component &c = components[i];
        HuffmanTable &dcTable = dcTables[c.dcTableId];
        HuffmanTable &acTable = acTables[c.acTableId];
        QuantTable &qTable = quantTables[c.quantTableId];

        for (int v = 0; v < c.vSampFactor; ++v) {
          for (int h = 0; h < c.hSampFactor; ++h) {
            float *block = mcuData[i].blocks[v][h].data();
            std::memset(block, 0, 64 * sizeof(float));

            // Decode DC
            int s = decodeSymbol(reader, dcTable);
            if (s == -1)
              throw std::runtime_error("Huffman decode error (DC)");
            int diff = 0;
            if (s > 0) {
              int bits = reader.readBits(s);
              if (bits == -1)
                throw std::runtime_error("Stream ended unexpectedly");
              if (bits < (1 << (s - 1))) {
                bits += ((-1) << s) + 1;
              }
              diff = bits;
            }
            c.prevDC += diff;
            block[0] = (float)c.prevDC * qTable.values[0];

            // Decode AC
            int k = 1;
            while (k < 64) {
              int s = decodeSymbol(reader, acTable);
              if (s == -1)
                throw std::runtime_error("Huffman decode error (AC)");
              int r = s >> 4;
              int num = s & 0x0F;

              if (s == 0x00) { // EOB
                break;
              } else if (s == 0xF0) { // ZRL
                k += 16;
              } else {
                k += r;
                int bits = reader.readBits(num);
                if (bits == -1)
                  throw std::runtime_error("Stream ended unexpectedly");
                if (bits < (1 << (num - 1))) {
                  bits += ((-1) << num) + 1;
                }
                block[ZIGZAG[k]] = (float)bits * qTable.values[k];
                k++;
              }
            }

            // IDCT
            idct(block);
          }
        }
      }

      // Color Conversion and Output
      // Iterate over pixels in the MCU
      for (int y = 0; y < mcuHeight; ++y) {
        int globalY = mcuY * mcuHeight + y;
        if (globalY >= height)
          break;

        for (int x = 0; x < mcuWidth; ++x) {
          int globalX = mcuX * mcuWidth + x;
          if (globalX >= width)
            break;

          // Get Y, Cb, Cr values
          // Handle subsampling by mapping pixel (x,y) in MCU to block
          // coordinates
          float Y = 0, Cb = 0, Cr = 0;

          // Y Component (assume ID 1 or index 0)
          // Assuming components are Y, Cb, Cr in order 0, 1, 2
          // TODO: Use component IDs to be sure

          auto getComponentValue = [&](int compIdx, int x, int y) {
            const Component &c = components[compIdx];
            // Map x, y (0..mcuWidth-1, 0..mcuHeight-1) to block coords
            // Effective scale factors relative to MCU size
            // MCU size is maxH*8 x maxV*8
            // Component size is c.hSamp * 8 x c.vSamp * 8

            // Scale x, y to component grid
            int cx = (x * c.hSampFactor) / maxH;
            int cy = (y * c.vSampFactor) / maxV;

            // Find block and offset
            int bx = cx / 8;
            int by = cy / 8;
            int ox = cx % 8;
            int oy = cy % 8;

            // Clamp block indices just in case
            if (bx >= c.hSampFactor)
              bx = c.hSampFactor - 1;
            if (by >= c.vSampFactor)
              by = c.vSampFactor - 1;

            return mcuData[compIdx].blocks[by][bx][oy * 8 + ox];
          };

          if (components.size() >= 1)
            Y = getComponentValue(0, x, y);
          if (components.size() >= 3) {
            Cb = getComponentValue(1, x, y);
            Cr = getComponentValue(2, x, y);
          }

          // Level shift (JPEG is usually centered around 0, but we added 128 in
          // encoder? Actually standard JPEG is YCbCr, where Y is [0,255] and
          // CbCr are centered at 128. The IDCT output is centered around 0 for
          // AC, but DC is unsigned. Wait, standard JPEG: Y = Y + 128 Cb = Cb +
          // 128 Cr = Cr + 128 But wait, our IDCT implementation assumes input
          // was shifted by -128 before DCT? Usually: Pixel -> -128 -> DCT ->
          // Quant Reverse: Dequant -> IDCT -> +128 -> Pixel

          Y += 128.0f;
          Cb += 128.0f;
          Cr += 128.0f;

          uint8_t r, g, b;
          ycbcrToRgb(Y, Cb, Cr, r, g, b);

          int pixelIdx = (globalY * width + globalX) * 3;
          img.data[pixelIdx] = r;
          img.data[pixelIdx + 1] = g;
          img.data[pixelIdx + 2] = b;
        }
      }
    }
  }

  return img;
}

void JpegDecoder::parseSegments(const std::vector<uint8_t> &data,
                                std::vector<QuantTable> &quantTables,
                                std::vector<HuffmanTable> &dcTables,
                                std::vector<HuffmanTable> &acTables,
                                std::vector<Component> &components, int &width,
                                int &height, const uint8_t *&scanData,
                                size_t &scanDataLen) {
  size_t pos = 2; // Skip SOI
  while (pos < data.size()) {
    if (data[pos] != 0xFF) {
      // Should be a marker
      pos++;
      continue;
    }

    uint8_t marker = data[pos + 1];
    uint16_t length = 0;
    if (marker != 0xD8 && marker != 0xD9 && (marker < 0xD0 || marker > 0xD7)) {
      if (pos + 3 >= data.size())
        break;
      length = (data[pos + 2] << 8) | data[pos + 3];
    }

    // Handle markers
    if (marker == 0xC0) { // SOF0
      // Parse Frame Header
      height = (data[pos + 5] << 8) | data[pos + 6];
      width = (data[pos + 7] << 8) | data[pos + 8];
      int numComponents = data[pos + 9];
      for (int i = 0; i < numComponents; ++i) {
        Component c;
        c.id = data[pos + 10 + i * 3];
        uint8_t samp = data[pos + 11 + i * 3];
        c.hSampFactor = samp >> 4;
        c.vSampFactor = samp & 0x0F;
        c.quantTableId = data[pos + 12 + i * 3];
        c.prevDC = 0;
        components.push_back(c);
      }
    } else if (marker == 0xC4) { // DHT
      // Parse Huffman Tables
      size_t tPos = pos + 4;
      size_t endPos = pos + 2 + length;
      while (tPos < endPos) {
        uint8_t info = data[tPos++];
        int tc = info >> 4; // 0=DC, 1=AC
        int th = info & 0x0F;
        HuffmanTable *table = (tc == 0) ? &dcTables[th] : &acTables[th];

        table->bits.resize(16);
        int totalSymbols = 0;
        for (int i = 0; i < 16; ++i) {
          table->bits[i] = data[tPos++];
          totalSymbols += table->bits[i];
        }
        table->huffval.resize(totalSymbols);
        for (int i = 0; i < totalSymbols; ++i) {
          table->huffval[i] = data[tPos++];
        }
        buildHuffmanTable(*table);
      }
    } else if (marker == 0xDB) { // DQT
      // Parse Quantization Tables
      size_t tPos = pos + 4;
      size_t endPos = pos + 2 + length;
      while (tPos < endPos) {
        uint8_t info = data[tPos++];
        int tq = info & 0x0F;
        // precision is info >> 4 (0=8bit, 1=16bit) - assuming 8bit for now
        for (int i = 0; i < 64; ++i) {
          quantTables[tq].values[i] = data[tPos++];
        }
      }
    } else if (marker == 0xDA) { // SOS
                                 // Start of Scan
      // Parse SOS header then break to decode scan data
      int numComponents = data[pos + 4];
      for (int i = 0; i < numComponents; ++i) {
        int id = data[pos + 5 + i * 2];
        uint8_t tableInfo = data[pos + 6 + i * 2];
        // Find component and assign tables
        for (auto &c : components) {
          if (c.id == id) {
            c.dcTableId = tableInfo >> 4;
            c.acTableId = tableInfo & 0x0F;
          }
        }
      }
      scanData = &data[pos + 2 + length];
      scanDataLen = data.size() - (pos + 2 + length);
      return;                    // Done parsing headers
    } else if (marker == 0xD9) { // EOI
      return;
    }

    pos += 2 + length;
  }
}

void JpegDecoder::buildHuffmanTable(HuffmanTable &table) {
  table.minCode.assign(16, 0);
  table.maxCode.assign(16, -1);
  table.valPtr.assign(16, 0);

  int code = 0;
  int idx = 0;
  for (int i = 0; i < 16; ++i) {
    if (table.bits[i] == 0) {
      table.maxCode[i] = -1;
    } else {
      table.valPtr[i] = idx;
      table.minCode[i] = code;
      table.maxCode[i] = code + table.bits[i] - 1;
      idx += table.bits[i];
      code += table.bits[i];
    }
    code <<= 1;
  }
}

// Placeholder implementations
void JpegDecoder::idct(float *block) {
  // AAN algorithm or similar 8x8 IDCT
  // For simplicity, we can use a basic separable IDCT or a faster integer
  // approximation if needed. Here is a standard floating point IDCT
  // implementation.

  float temp[64];

  // Rows
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      float sum = 0.0f;
      for (int k = 0; k < 8; ++k) {
        float cu =
            (k == 0) ? 0.70710678f : 1.0f; // 1/sqrt(2) for k=0, 1 for k>0
        sum += cu * block[i * 8 + k] * cos((2 * j + 1) * k * M_PI / 16.0);
      }
      temp[i * 8 + j] = sum * 0.5f;
    }
  }

  // Columns
  for (int j = 0; j < 8; ++j) {
    for (int i = 0; i < 8; ++i) {
      float sum = 0.0f;
      for (int k = 0; k < 8; ++k) {
        float cv =
            (k == 0) ? 0.70710678f : 1.0f; // 1/sqrt(2) for k=0, 1 for k>0
        sum += cv * temp[k * 8 + j] * cos((2 * i + 1) * k * M_PI / 16.0);
      }
      block[i * 8 + j] = sum * 0.5f;
    }
  }
}

void JpegDecoder::ycbcrToRgb(float y, float cb, float cr, uint8_t &r,
                             uint8_t &g, uint8_t &b) {
  // JPEG YCbCr to RGB conversion
  // R = Y + 1.402 * (Cr - 128)
  // G = Y - 0.344136 * (Cb - 128) - 0.714136 * (Cr - 128)
  // B = Y + 1.772 * (Cb - 128)

  float r_f = y + 1.402f * (cr - 128.0f);
  float g_f = y - 0.344136f * (cb - 128.0f) - 0.714136f * (cr - 128.0f);
  float b_f = y + 1.772f * (cb - 128.0f);

  r = clamp(static_cast<int>(r_f + 0.5f), 0, 255);
  g = clamp(static_cast<int>(g_f + 0.5f), 0, 255);
  b = clamp(static_cast<int>(b_f + 0.5f), 0, 255);
}
