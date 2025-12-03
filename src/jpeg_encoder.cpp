#include "jpeg_encoder.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>

// Standard JPEG Quantization Tables (K.1 and K.2)
const uint8_t JpegEncoder::QUANT_LUMA[64] = {
    16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,
    14, 13, 16, 24, 40,  57,  69,  56,  14, 17, 22, 29, 51,  87,  80,  62,
    18, 22, 37, 56, 68,  109, 103, 77,  24, 35, 55, 64, 81,  104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};

const uint8_t JpegEncoder::QUANT_CHROMA[64] = {
    17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};

// ZigZag order
// ZigZag order
const uint8_t JpegEncoder::ZIGZAG[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

JpegEncoder::HuffmanTable JpegEncoder::DC_LUMA;
JpegEncoder::HuffmanTable JpegEncoder::AC_LUMA;
JpegEncoder::HuffmanTable JpegEncoder::DC_CHROMA;
JpegEncoder::HuffmanTable JpegEncoder::AC_CHROMA;
bool JpegEncoder::tablesInitialized = false;

// Standard Huffman Tables (Annex K.3)
// DC Luminance
static const uint8_t STD_DC_LUMA_BITS[16] = {0, 1, 5, 1, 1, 1, 1, 1,
                                             1, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t STD_DC_LUMA_VAL[12] = {0, 1, 2, 3, 4,  5,
                                            6, 7, 8, 9, 10, 11};

// DC Chrominance
static const uint8_t STD_DC_CHROMA_BITS[16] = {0, 3, 1, 1, 1, 1, 1, 1,
                                               1, 1, 1, 0, 0, 0, 0, 0};
static const uint8_t STD_DC_CHROMA_VAL[12] = {0, 1, 2, 3, 4,  5,
                                              6, 7, 8, 9, 10, 11};

// AC Luminance
static const uint8_t STD_AC_LUMA_BITS[16] = {0, 2, 1, 3, 3, 2, 4, 3,
                                             5, 5, 4, 4, 0, 0, 1, 0x7d};
static const uint8_t STD_AC_LUMA_VAL[162] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
    0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72,
    0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
    0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
    0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4,
    0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};

// AC Chrominance
static const uint8_t STD_AC_CHROMA_BITS[16] = {0, 2, 1, 2, 4, 4, 3, 4,
                                               7, 5, 4, 4, 0, 1, 2, 0x77};
static const uint8_t STD_AC_CHROMA_VAL[162] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41,
    0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1,
    0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
    0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4,
    0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};

void JpegEncoder::initTables() {
  if (tablesInitialized)
    return;

  auto buildTable = [](HuffmanTable &table, const uint8_t *bits,
                       const uint8_t *val, int valCount) {
    table.bits.assign(bits, bits + 16);
    table.huffval.assign(val, val + valCount);

    // Generate codes
    table.codes.assign(256, 0);
    table.codeLengths.assign(256, 0);

    uint32_t code = 0;
    int k = 0;
    for (int i = 0; i < 16; ++i) {
      for (int j = 0; j < table.bits[i]; ++j) {
        uint8_t symbol = table.huffval[k++];
        table.codes[symbol] = code;
        table.codeLengths[symbol] = i + 1;
        code++;
      }
      code <<= 1;
    }
  };

  buildTable(DC_LUMA, STD_DC_LUMA_BITS, STD_DC_LUMA_VAL, 12);
  buildTable(AC_LUMA, STD_AC_LUMA_BITS, STD_AC_LUMA_VAL, 162);
  buildTable(DC_CHROMA, STD_DC_CHROMA_BITS, STD_DC_CHROMA_VAL, 12);
  buildTable(AC_CHROMA, STD_AC_CHROMA_BITS, STD_AC_CHROMA_VAL, 162);

  tablesInitialized = true;
}

void JpegEncoder::encode(const Image &img, const std::string &filepath,
                         int quality) {
  initTables();
  BitWriter writer;

  // Quality scaling
  if (quality < 1)
    quality = 1;
  if (quality > 100)
    quality = 100;

  int scale;
  if (quality < 50) {
    scale = 5000 / quality;
  } else {
    scale = 200 - 2 * quality;
  }

  auto generateQuantTable = [&](const uint8_t *baseTable, uint8_t *outTable) {
    for (int i = 0; i < 64; ++i) {
      long temp = (long)baseTable[i] * scale + 50;
      temp /= 100;
      if (temp < 1)
        temp = 1;
      if (temp > 255)
        temp = 255;
      outTable[i] = (uint8_t)temp;
    }
  };

  uint8_t scaledLuma[64];
  uint8_t scaledChroma[64];

  generateQuantTable(QUANT_LUMA, scaledLuma);
  generateQuantTable(QUANT_CHROMA, scaledChroma);

  writeHeaders(writer, img.width, img.height, scaledLuma, scaledChroma);

  int prevDC_Y = 0;
  int prevDC_Cb = 0;
  int prevDC_Cr = 0;

  // Process 8x8 blocks
  // Pad width/height to multiple of 8
  int paddedWidth = (img.width + 7) & ~7;
  int paddedHeight = (img.height + 7) & ~7;

  for (int y = 0; y < paddedHeight; y += 8) {
    for (int x = 0; x < paddedWidth; x += 8) {

      // Extract 8x8 block for Y, Cb, Cr
      float blockY[64], blockCb[64], blockCr[64];

      for (int by = 0; by < 8; ++by) {
        for (int bx = 0; bx < 8; ++bx) {
          int imgX = std::min(x + bx, img.width - 1);
          int imgY = std::min(y + by, img.height - 1);

          int pixelIdx = (imgY * img.width + imgX) * img.channels;
          // uint8_t r = img.data[pixelIdx];
          // uint8_t g = img.data[pixelIdx + 1];
          // uint8_t b = img.data[pixelIdx + 2];

          float Y, Cb, Cr;
          rgbToYcbcr(&img.data[pixelIdx], &Y, &Cb, &Cr);

          blockY[by * 8 + bx] = Y;
          blockCb[by * 8 + bx] = Cb;
          blockCr[by * 8 + bx] = Cr;
        }
      }

      // Process Y
      processBlock(writer, blockY, scaledLuma, prevDC_Y, DC_LUMA, AC_LUMA);
      // Process Cb
      processBlock(writer, blockCb, scaledChroma, prevDC_Cb, DC_CHROMA,
                   AC_CHROMA);
      // Process Cr
      processBlock(writer, blockCr, scaledChroma, prevDC_Cr, DC_CHROMA,
                   AC_CHROMA);
    }
  }

  writeFooter(writer);

  std::ofstream outFile(filepath, std::ios::binary);
  std::vector<uint8_t> data = writer.getData();
  outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
}

void JpegEncoder::writeHeaders(BitWriter &writer, int width, int height,
                               const uint8_t *lumaTable,
                               const uint8_t *chromaTable) {
  // SOI
  writer.writeMarker(0xD8);

  // APP0 (JFIF)
  writer.writeMarker(0xE0);
  writer.writeBits(16, 16);         // Length
  writer.writeBits(0x4A464946, 32); // "JFIF"
  writer.writeBits(0x00, 8);        // null
  writer.writeBits(0x0101, 16);     // Version 1.01
  writer.writeBits(0x00, 8);        // Units (none)
  writer.writeBits(0x0001, 16);     // X density
  writer.writeBits(0x0001, 16);     // Y density
  writer.writeBits(0x00, 8);        // X thumb
  writer.writeBits(0x00, 8);        // Y thumb

  // DQT (Define Quantization Tables)
  writer.writeMarker(0xDB);
  writer.writeBits(132, 16); // Length (2 + 65 + 65)

  // Luma Table (ID 0)
  writer.writeBits(0x00, 8); // Precision 0, ID 0
  for (int i = 0; i < 64; ++i)
    writer.writeBits(lumaTable[ZIGZAG[i]], 8);

  // Chroma Table (ID 1)
  writer.writeBits(0x01, 8); // Precision 0, ID 1
  for (int i = 0; i < 64; ++i)
    writer.writeBits(chromaTable[ZIGZAG[i]], 8);

  // SOF0 (Start of Frame)
  writer.writeMarker(0xC0);
  writer.writeBits(17, 16); // Length
  writer.writeBits(8, 8);   // Precision
  writer.writeBits(height, 16);
  writer.writeBits(width, 16);
  writer.writeBits(3, 8); // Components

  // Y
  writer.writeBits(1, 8);    // ID
  writer.writeBits(0x11, 8); // Sampling factors (1x1)
  writer.writeBits(0, 8);    // Quant table ID

  // Cb
  writer.writeBits(2, 8);    // ID
  writer.writeBits(0x11, 8); // Sampling factors (1x1)
  writer.writeBits(1, 8);    // Quant table ID

  // Cr
  writer.writeBits(3, 8);    // ID
  writer.writeBits(0x11, 8); // Sampling factors (1x1)
  writer.writeBits(1, 8);    // Quant table ID

  // DHT (Define Huffman Tables)
  writer.writeMarker(0xC4);
  // Calculate length: 2 + (1+16+12) + (1+16+162) + (1+16+12) + (1+16+162) = 2 +
  // 29 + 179 + 29 + 179 = 418
  writer.writeBits(418, 16);

  auto writeDHT = [&](const HuffmanTable &table, int id, int ac) {
    writer.writeBits((ac << 4) | id, 8);
    for (int i = 0; i < 16; ++i)
      writer.writeBits(table.bits[i], 8);
    for (uint8_t val : table.huffval)
      writer.writeBits(val, 8);
  };

  writeDHT(DC_LUMA, 0, 0);
  writeDHT(AC_LUMA, 0, 1);
  writeDHT(DC_CHROMA, 1, 0);
  writeDHT(AC_CHROMA, 1, 1);

  // SOS (Start of Scan)
  writer.writeMarker(0xDA);
  writer.writeBits(12, 16); // Length
  writer.writeBits(3, 8);   // Components

  writer.writeBits(1, 8);    // Y
  writer.writeBits(0x00, 8); // DC/AC table 0/0

  writer.writeBits(2, 8);    // Cb
  writer.writeBits(0x11, 8); // DC/AC table 1/1

  writer.writeBits(3, 8);    // Cr
  writer.writeBits(0x11, 8); // DC/AC table 1/1

  writer.writeBits(0, 8);  // Start spectral
  writer.writeBits(63, 8); // End spectral
  writer.writeBits(0, 8);  // Approx high/low

  // Enable byte stuffing for entropy coded data
  writer.enableByteStuffing(true);
}

void JpegEncoder::writeFooter(BitWriter &writer) {
  writer.writeMarker(0xD9); // EOI
}

void JpegEncoder::processBlock(BitWriter &writer, const float *blockData,
                               const uint8_t *quantTable, int &prevDC,
                               const HuffmanTable &dcTable,
                               const HuffmanTable &acTable) {
  float dctBlock[64];
  std::copy(blockData, blockData + 64, dctBlock);

  fdct(dctBlock);
  quantize(dctBlock, quantTable);

  float zigzagBlock[64];
  zigzag(dctBlock, zigzagBlock);

  encodeBlock(writer, zigzagBlock, prevDC, dcTable, acTable);
}

void JpegEncoder::encodeBlock(BitWriter &writer, const float *quantizedBlock,
                              int &prevDC, const HuffmanTable &dcTable,
                              const HuffmanTable &acTable) {
  // DC Coefficient
  int dcVal = static_cast<int>(quantizedBlock[0]);
  int diff = dcVal - prevDC;
  prevDC = dcVal;

  int size = 0;
  int temp = diff;
  if (temp < 0) {
    temp = -temp;
    size = 0;
    while (temp > 0) {
      temp >>= 1;
      size++;
    }
  } else {
    size = 0;
    while (temp > 0) {
      temp >>= 1;
      size++;
    }
  }

  // Write DC code
  uint32_t code = dcTable.codes[size];
  int len = dcTable.codeLengths[size];
  writer.writeBits(code, len);

  // Write DC value
  if (size > 0) {
    if (diff < 0) {
      diff = diff + (1 << size) - 1; // Wait, diff is negative.
                                     // if diff is -3, size 2. -3 + 4 - 1 = 0.
                                     // if diff is -2, size 2. -2 + 4 - 1 = 1.
                                     // if diff is -1, size 1. -1 + 2 - 1 = 0.
                                     // This seems correct.
    }
    writer.writeBits(diff, size);
  }

  // AC Coefficients
  int rle = 0;
  for (int i = 1; i < 64; ++i) {
    int val = static_cast<int>(quantizedBlock[i]);
    if (val == 0) {
      rle++;
    } else {
      while (rle > 15) {
        // ZRL (15 zeros, then 0? No, 16 zeros)
        // F0 is ZRL.
        uint32_t zrlCode = acTable.codes[0xF0];
        int zrlLen = acTable.codeLengths[0xF0];
        writer.writeBits(zrlCode, zrlLen);
        rle -= 16;
      }

      int acSize = 0;
      int acTemp = std::abs(val);
      while (acTemp > 0) {
        acTemp >>= 1;
        acSize++;
      }

      int symbol = (rle << 4) | acSize;
      uint32_t acCode = acTable.codes[symbol];
      int acLen = acTable.codeLengths[symbol];
      writer.writeBits(acCode, acLen);

      if (val < 0) {
        val = val + (1 << acSize) - 1;
      }
      writer.writeBits(val, acSize);

      rle = 0;
    }
  }

  if (rle > 0) {
    // EOB (End of Block)
    uint32_t eobCode = acTable.codes[0x00];
    int eobLen = acTable.codeLengths[0x00];
    writer.writeBits(eobCode, eobLen);
  }
}

void JpegEncoder::rgbToYcbcr(const uint8_t *rgb, float *y, float *cb,
                             float *cr) {
  float r = rgb[0];
  float g = rgb[1];
  float b = rgb[2];

  // Standard JPEG conversion
  *y = 0.299f * r + 0.587f * g + 0.114f * b;
  *cb = -0.1687f * r - 0.3313f * g + 0.5f * b + 128.0f;
  *cr = 0.5f * r - 0.4187f * g - 0.0813f * b + 128.0f;

  // Level shift Y to [-128, 127] for DCT
  *y -= 128.0f;
  *cb -= 128.0f;
  *cr -= 128.0f;
}

void JpegEncoder::fdct(float *block) {
  float temp[64];
  const float PI = 3.14159265358979323846f;

  for (int u = 0; u < 8; ++u) {
    for (int v = 0; v < 8; ++v) {
      float sum = 0.0f;
      float cu = (u == 0) ? 0.70710678118f : 1.0f; // 1/sqrt(2)
      float cv = (v == 0) ? 0.70710678118f : 1.0f;

      for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
          sum += block[y * 8 + x] * std::cos((2 * x + 1) * u * PI / 16.0f) *
                 std::cos((2 * y + 1) * v * PI / 16.0f);
        }
      }
      temp[v * 8 + u] = 0.25f * cu * cv * sum;
    }
  }

  // Copy back
  for (int i = 0; i < 64; ++i) {
    block[i] = temp[i];
  }
}

void JpegEncoder::quantize(float *block, const uint8_t *quantTable) {
  for (int i = 0; i < 64; ++i) {
    // Round to nearest integer
    block[i] = std::round(block[i] / quantTable[i]);
  }
}

void JpegEncoder::zigzag(const float *input, float *output) {
  for (int i = 0; i < 64; ++i) {
    output[i] = input[ZIGZAG[i]];
  }
}

// Removed writeMarker helper as it's now in BitWriter
