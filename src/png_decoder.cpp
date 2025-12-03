#include "png_decoder.hpp"
#include "utils/bit_reader.hpp"
#include <cmath> // For std::abs
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

// PNG Signature: 89 50 4E 47 0D 0A 1A 0A
const std::vector<uint8_t> PNG_SIGNATURE = {0x89, 0x50, 0x4E, 0x47,
                                            0x0D, 0x0A, 0x1A, 0x0A};

uint32_t PngDecoder::readBigEndian(const uint8_t *buffer) {
  return (static_cast<uint32_t>(buffer[0]) << 24) |
         (static_cast<uint32_t>(buffer[1]) << 16) |
         (static_cast<uint32_t>(buffer[2]) << 8) |
         static_cast<uint32_t>(buffer[3]);
}

void PngDecoder::parseIHDR(const std::vector<uint8_t> &data, int &width,
                           int &height, uint8_t &bitDepth, uint8_t &colorType,
                           uint8_t &compressionMethod, uint8_t &filterMethod,
                           uint8_t &interlaceMethod) {
  if (data.size() < 13) {
    throw std::runtime_error("Invalid IHDR chunk size");
  }
  width = readBigEndian(data.data());
  height = readBigEndian(data.data() + 4);
  bitDepth = data[8];
  colorType = data[9];
  compressionMethod = data[10];
  filterMethod = data[11];
  interlaceMethod = data[12];

  std::cout << "PNG Info: " << width << "x" << height
            << ", Depth: " << (int)bitDepth << ", Color: " << (int)colorType
            << std::endl;

  if (compressionMethod != 0)
    throw std::runtime_error("Unsupported compression method");
  if (filterMethod != 0)
    throw std::runtime_error("Unsupported filter method");
  if (interlaceMethod != 0)
    throw std::runtime_error("Interlacing not supported yet");
  if (bitDepth != 8)
    throw std::runtime_error("Only 8-bit depth supported for now");
  if (colorType != 2 && colorType != 6)
    throw std::runtime_error(
        "Only Truecolor (2) and Truecolor+Alpha (6) supported");
}

Image PngDecoder::decode(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + filepath);
  }

  // Check signature
  std::vector<uint8_t> sig(8);
  file.read(reinterpret_cast<char *>(sig.data()), 8);
  if (sig != PNG_SIGNATURE) {
    throw std::runtime_error("Invalid PNG signature");
  }

  std::vector<uint8_t> idatBuffer;
  int width = 0, height = 0;
  uint8_t bitDepth = 0, colorType = 0, compression = 0, filter = 0,
          interlace = 0;
  bool headerFound = false;

  while (file) {
    // Read Chunk Length (4 bytes)
    uint8_t lenBuf[4];
    file.read(reinterpret_cast<char *>(lenBuf), 4);
    if (file.gcount() != 4)
      break; // End of file
    uint32_t length = readBigEndian(lenBuf);

    // Read Chunk Type (4 bytes)
    char typeBuf[5] = {0};
    file.read(typeBuf, 4);
    std::string type = typeBuf;

    // Read Chunk Data
    std::vector<uint8_t> data(length);
    if (length > 0) {
      file.read(reinterpret_cast<char *>(data.data()), length);
    }

    // Read CRC (4 bytes) - ignoring verification for now
    char crcBuf[4];
    file.read(crcBuf, 4);

    // Process Chunk
    if (type == "IHDR") {
      parseIHDR(data, width, height, bitDepth, colorType, compression, filter,
                interlace);
      headerFound = true;
    } else if (type == "IDAT") {
      idatBuffer.insert(idatBuffer.end(), data.begin(), data.end());
    } else if (type == "IEND") {
      break;
    } else {
      // Ignore ancillary chunks
    }
  }

  if (!headerFound) {
    throw std::runtime_error("No IHDR chunk found");
  }

  if (idatBuffer.empty()) {
    throw std::runtime_error("No IDAT chunks found");
  }

  std::cout << "Total IDAT size: " << idatBuffer.size() << " bytes"
            << std::endl;

  // Decompress IDAT (Zlib/DEFLATE)
  std::vector<uint8_t> decompressedData = inflate(idatBuffer);
  std::cout << "Decompressed size: " << decompressedData.size() << " bytes"
            << std::endl;

  // Unfilter scanlines
  int bytesPerPixel = (colorType == 6 ? 4 : 3);
  std::vector<uint8_t> rawData =
      unfilterScanlines(decompressedData, width, height, bytesPerPixel);

  // Convert to Image
  Image img(width, height, bytesPerPixel);
  img.data = rawData;

  return img;
}

// ============================================================================
// Filtering Implementation
// ============================================================================

uint8_t PngDecoder::paethPredictor(uint8_t a, uint8_t b, uint8_t c) {
  int p = (int)a + (int)b - (int)c;
  int pa = std::abs(p - (int)a);
  int pb = std::abs(p - (int)b);
  int pc = std::abs(p - (int)c);

  if (pa <= pb && pa <= pc)
    return a;
  if (pb <= pc)
    return b;
  return c;
}

std::vector<uint8_t>
PngDecoder::unfilterScanlines(const std::vector<uint8_t> &data, int width,
                              int height, int bytesPerPixel) {
  std::vector<uint8_t> out;
  out.reserve(width * height * bytesPerPixel);

  int stride = width * bytesPerPixel;
  size_t inputIndex = 0;

  // Previous scanline (initialized to 0)
  std::vector<uint8_t> prevScanline(stride, 0);
  std::vector<uint8_t> currScanline(stride);

  for (int y = 0; y < height; ++y) {
    if (inputIndex >= data.size()) {
      throw std::runtime_error("Not enough data for scanlines");
    }

    uint8_t filterType = data[inputIndex++];

    for (int x = 0; x < stride; ++x) {
      if (inputIndex >= data.size()) {
        throw std::runtime_error("Scanline data truncated");
      }

      uint8_t xVal = data[inputIndex++];
      uint8_t a = (x >= bytesPerPixel) ? currScanline[x - bytesPerPixel] : 0;
      uint8_t b = prevScanline[x];
      uint8_t c = (x >= bytesPerPixel) ? prevScanline[x - bytesPerPixel] : 0;

      uint8_t recon = 0;
      switch (filterType) {
      case 0: // None
        recon = xVal;
        break;
      case 1: // Sub
        recon = xVal + a;
        break;
      case 2: // Up
        recon = xVal + b;
        break;
      case 3: // Average
        recon = xVal + (a + b) / 2;
        break;
      case 4: // Paeth
        recon = xVal + paethPredictor(a, b, c);
        break;
      default:
        throw std::runtime_error("Invalid filter type");
      }
      currScanline[x] = recon;
    }

    out.insert(out.end(), currScanline.begin(), currScanline.end());
    prevScanline = currScanline;
  }

  return out;
}

// ============================================================================
// DEFLATE / Zlib Implementation
// ============================================================================

std::vector<uint8_t>
PngDecoder::inflate(const std::vector<uint8_t> &compressedData) {
  if (compressedData.size() < 6) { // 2 bytes header + 4 bytes adler32
    throw std::runtime_error("Invalid Zlib stream: too short");
  }

  BitReader reader(compressedData.data(), compressedData.size());

  // 1. Zlib Header
  uint8_t cmf = reader.readBits(8);
  uint8_t flg = reader.readBits(8);

  uint8_t cm = cmf & 0x0F;
  uint8_t cinfo = (cmf >> 4) & 0x0F;

  if (cm != 8)
    throw std::runtime_error("Unsupported Zlib compression method (must be 8)");
  if (cinfo > 7)
    throw std::runtime_error("Invalid Zlib window size");
  if (((cmf * 256 + flg) % 31) != 0)
    throw std::runtime_error("Invalid Zlib header checksum");
  if (flg & 0x20)
    throw std::runtime_error("Zlib preset dictionary not supported");

  std::vector<uint8_t> out;
  bool bfinal = false;
  while (!bfinal) {
    bfinal = reader.readBits(1);
    uint8_t btype = reader.readBits(2);

    if (btype == 0) {
      decodeUncompressedBlock(reader, out);
    } else if (btype == 1) {
      decodeFixedHuffmanBlock(reader, out);
    } else if (btype == 2) {
      decodeDynamicHuffmanBlock(reader, out);
    } else {
      throw std::runtime_error("Invalid DEFLATE block type");
    }
  }

  // Skip alignment bits to byte boundary if needed?
  // Actually BitReader handles bit offsets.
  // The Adler32 is at the byte boundary after the bit stream.
  reader.alignToByte();

  // Check if we have enough bytes for Adler32
  // if (reader.getByteOffset() + 4 > compressedData.size()) ...

  return out;
}

void PngDecoder::decodeUncompressedBlock(BitReader &reader,
                                         std::vector<uint8_t> &out) {
  reader.alignToByte();
  uint16_t len = reader.readBits(16);
  uint16_t nlen = reader.readBits(16);

  if ((uint16_t)len != (uint16_t)(~nlen)) {
    throw std::runtime_error("Invalid uncompressed block length");
  }

  for (int i = 0; i < len; ++i) {
    out.push_back(reader.readBits(8));
  }
}

void PngDecoder::HuffmanTree::build(const std::vector<int> &codeLengths) {
  counts.clear();
  symbols.clear();

  int maxLen = 0;
  for (int len : codeLengths) {
    if (len > maxLen)
      maxLen = len;
  }

  if (maxLen == 0)
    return;

  counts.resize(maxLen + 1, 0);
  for (int len : codeLengths) {
    if (len > 0)
      counts[len]++;
  }

  // Compute offsets
  std::vector<int> offsets(maxLen + 1, 0);
  int code = 0;
  for (int i = 1; i <= maxLen; ++i) {
    code = (code + counts[i - 1]) << 1;
    offsets[i] = code;
  }

  // Assign codes to symbols (we don't store the codes, just the mapping logic)
  // Actually, for decoding, we need a way to map code -> symbol.
  // Standard way:
  // 1. Count codes of each length (bl_count)
  // 2. Find numerical value of smallest code for each length (next_code)
  // 3. Assign codes.

  // But for decoding, we can use a lookup table for small lengths or a tree
  // traversal. Given we are in C++, let's use a simple canonical decoding
  // approach: Read bits until we match a code. Or better: Use the "counts" and
  // "sorted symbols" approach.

  // Sort symbols by length, then by value (lexicographically).
  // Actually, the symbols are just indices in codeLengths.
  // We need to store them sorted by length.

  // Let's rebuild the structure to be decoding-friendly.
  // We need:
  // - counts[len]: number of codes of length len
  // - symbols: list of symbols sorted by length.

  // Re-count to be sure
  counts.assign(maxLen + 1, 0);
  for (int len : codeLengths) {
    if (len > 0)
      counts[len]++;
  }

  symbols.clear(); // This will store symbols sorted by length
  // To sort by length, we iterate lengths 1..maxLen, and for each length,
  // iterate all symbols to find those with that length.
  for (int len = 1; len <= maxLen; ++len) {
    for (size_t sym = 0; sym < codeLengths.size(); ++sym) {
      if (codeLengths[sym] == len) {
        symbols.push_back(sym);
      }
    }
  }
}

int PngDecoder::HuffmanTree::decode(BitReader &reader) const {
  int code = 0;
  int first = 0;
  int index = 0;

  for (size_t len = 1; len < counts.size(); ++len) {
    code |= reader.readBits(1);
    int count = counts[len];

    if (code - first < count) {
      return symbols[index + (code - first)];
    }

    index += count;
    first += count;
    first <<= 1;
    code <<= 1;
  }
  throw std::runtime_error("Invalid Huffman code");
}

void PngDecoder::decodeFixedHuffmanBlock(BitReader &reader,
                                         std::vector<uint8_t> &out) {
  // RFC 1951 Fixed Huffman codes
  // Lit/Len:
  // 0-143: 8 bits, 00110000-10111111
  // 144-255: 9 bits, 110010000-111111111
  // 256-279: 7 bits, 0000000-0010111
  // 280-287: 8 bits, 11000000-11000111

  std::vector<int> litLenLengths(288);
  for (int i = 0; i <= 143; ++i)
    litLenLengths[i] = 8;
  for (int i = 144; i <= 255; ++i)
    litLenLengths[i] = 9;
  for (int i = 256; i <= 279; ++i)
    litLenLengths[i] = 7;
  for (int i = 280; i <= 287; ++i)
    litLenLengths[i] = 8;

  HuffmanTree litLenTree;
  litLenTree.build(litLenLengths);

  std::vector<int> distLengths(32);
  for (int i = 0; i < 32; ++i)
    distLengths[i] = 5;

  HuffmanTree distTree;
  distTree.build(distLengths);

  // decodeHuffmanBlock(reader, out); // Wait, I need to pass the trees to
  // decodeHuffmanBlock or make it generic.
  // decodeHuffmanBlock should take trees as arguments.
  // But I defined it as taking just reader and out.
  // I should refactor decodeHuffmanBlock to take trees, or have a common
  // "decodeBlockData" helper.

  // Let's implement the loop here for now, or change the signature.
  // I'll change the signature in the next step or just inline the loop logic in
  // a helper. Actually, decodeDynamicHuffmanBlock builds trees then decodes.
  // decodeFixedHuffmanBlock builds trees then decodes.
  // So the decoding loop is common.

  // I'll implement a helper `decodeBlockData` that takes the trees.
  // But I can't change the header right now easily.
  // I'll just put the loop logic in a private static helper that is not in the
  // header, or just duplicate it for now (it's not that big). Or better, I'll
  // implement `decodeHuffmanBlock` to take trees, but I need to update header.
  // I'll update header later if needed. For now, I'll just implement
  // `decodeBlockData` as a free function or static method inside cpp.

  // Let's use a lambda or local helper.
  auto decodeLoop = [&](const HuffmanTree &llTree, const HuffmanTree &dTree) {
    while (true) {
      int sym = llTree.decode(reader);
      if (sym < 256) {
        out.push_back(static_cast<uint8_t>(sym));
      } else if (sym == 256) {
        break; // End of block
      } else {
        // Length
        int lenCode = sym - 257;
        int length = 0;
        int extraBits = 0;

        // Length codes 257..285
        static const int lenBase[] = {
            3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
            31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
        static const int lenExtra[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                       1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                       4, 4, 4, 4, 5, 5, 5, 5, 0};

        if (lenCode >= 29)
          throw std::runtime_error("Invalid length code");

        length = lenBase[lenCode];
        extraBits = lenExtra[lenCode];
        if (extraBits > 0) {
          length += reader.readBits(extraBits);
        }

        // Distance
        int distCode = dTree.decode(reader);
        int distance = 0;
        int distExtra = 0;

        // Distance codes 0..29
        static const int distBase[] = {
            1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
            33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
            1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
        static const int distExtraBits[] = {
            0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
            6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

        if (distCode >= 30)
          throw std::runtime_error("Invalid distance code");

        distance = distBase[distCode];
        distExtra = distExtraBits[distCode];
        if (distExtra > 0) {
          distance += reader.readBits(distExtra);
        }

        // Copy
        if (distance > out.size())
          throw std::runtime_error("Invalid distance (too far back)");

        size_t copyStart = out.size() - distance;
        for (int i = 0; i < length; ++i) {
          out.push_back(out[copyStart + i]);
        }
      }
    }
  };

  decodeLoop(litLenTree, distTree);
}

void PngDecoder::decodeDynamicHuffmanBlock(BitReader &reader,
                                           std::vector<uint8_t> &out) {
  int hlit = reader.readBits(5) + 257;
  int hdist = reader.readBits(5) + 1;
  int hclen = reader.readBits(4) + 4;

  std::vector<int> codeLenLengths(19, 0);
  static const int clenOrder[] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                  11, 4,  12, 3, 13, 2, 14, 1, 15};

  for (int i = 0; i < hclen; ++i) {
    codeLenLengths[clenOrder[i]] = reader.readBits(3);
  }

  HuffmanTree codeLenTree;
  codeLenTree.build(codeLenLengths);

  // Decode Lit/Len and Dist lengths
  std::vector<int> allLengths;
  allLengths.reserve(hlit + hdist);

  while (allLengths.size() < hlit + hdist) {
    int sym = codeLenTree.decode(reader);
    if (sym < 16) {
      allLengths.push_back(sym);
    } else if (sym == 16) {
      int copyLen = reader.readBits(2) + 3;
      if (allLengths.empty())
        throw std::runtime_error("Repeat code 16 with no previous");
      int prev = allLengths.back();
      for (int i = 0; i < copyLen; ++i)
        allLengths.push_back(prev);
    } else if (sym == 17) {
      int zeroLen = reader.readBits(3) + 3;
      for (int i = 0; i < zeroLen; ++i)
        allLengths.push_back(0);
    } else if (sym == 18) {
      int zeroLen = reader.readBits(7) + 11;
      for (int i = 0; i < zeroLen; ++i)
        allLengths.push_back(0);
    }
  }

  std::vector<int> litLenLengths(allLengths.begin(), allLengths.begin() + hlit);
  std::vector<int> distLengths(allLengths.begin() + hlit, allLengths.end());

  HuffmanTree litLenTree;
  litLenTree.build(litLenLengths);

  HuffmanTree distTree;
  distTree.build(distLengths);

  // Duplicate logic from FixedHuffmanBlock...
  // I should really refactor this.
  // For now, I'll copy-paste the decode loop logic or call a helper if I had
  // one. I'll copy paste for safety in this tool call, then refactor if needed.

  while (true) {
    int sym = litLenTree.decode(reader);
    if (sym < 256) {
      out.push_back(static_cast<uint8_t>(sym));
    } else if (sym == 256) {
      break; // End of block
    } else {
      // Length
      int lenCode = sym - 257;
      int length = 0;
      int extraBits = 0;

      static const int lenBase[] = {3,  4,  5,  6,   7,   8,   9,   10,  11, 13,
                                    15, 17, 19, 23,  27,  31,  35,  43,  51, 59,
                                    67, 83, 99, 115, 131, 163, 195, 227, 258};
      static const int lenExtra[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                     1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                     4, 4, 4, 4, 5, 5, 5, 5, 0};

      if (lenCode >= 29)
        throw std::runtime_error("Invalid length code");

      length = lenBase[lenCode];
      extraBits = lenExtra[lenCode];
      if (extraBits > 0) {
        length += reader.readBits(extraBits);
      }

      // Distance
      int distCode = distTree.decode(reader);
      int distance = 0;
      int distExtra = 0;

      static const int distBase[] = {
          1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
          33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
          1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
      static const int distExtraBits[] = {0, 0, 0,  0,  1,  1,  2,  2,  3,  3,
                                          4, 4, 5,  5,  6,  6,  7,  7,  8,  8,
                                          9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

      if (distCode >= 30)
        throw std::runtime_error("Invalid distance code");

      distance = distBase[distCode];
      distExtra = distExtraBits[distCode];
      if (distExtra > 0) {
        distance += reader.readBits(distExtra);
      }

      // Copy
      if (distance > out.size())
        throw std::runtime_error("Invalid distance (too far back)");

      size_t copyStart = out.size() - distance;
      for (int i = 0; i < length; ++i) {
        out.push_back(out[copyStart + i]);
      }
    }
  }
}

// Stub for the declared method to avoid linker error, though I implemented
// logic inside others.
void PngDecoder::decodeHuffmanBlock(BitReader & /*reader*/,
                                    std::vector<uint8_t> & /*out*/) {
  // This was intended to be the common loop.
  // Since I duplicated the loop, I can leave this empty or remove it from
  // header. I'll leave it empty for now to satisfy the header declaration.
  throw std::runtime_error("Should not be called directly");
}
