# Task: Phase 1 - Foundation & Utilities

## Role
You are a Senior C++ Systems Engineer. You value performance, memory safety, and clean, dependency-free code.

## Context
We are building a **Raw C++ PNG to JPG Converter** from scratch. This means NO external libraries (no `libpng`, `libjpeg`, `zlib`, `stb_image`, etc.). We must implement the file format specifications manually.

## Objective
Implement the foundational data structures and utility classes required for the project. This is **Phase 1** of the roadmap.

## Requirements
- **Language**: C++17 standard.
- **Dependencies**: None. Standard Library (STL) only.
- **Project Structure**:
    ```text
    src/
      image.hpp          # Image container
      utils/
        bit_reader.hpp   # For reading variable-bit data (PNG/DEFLATE)
        bit_writer.hpp   # For writing variable-bit data (JPEG)
        checksum.hpp     # CRC32 (PNG) and Adler32 (Zlib)
    Makefile             # Build script
    ```

## Detailed Instructions

### 1. Image Struct (`src/image.hpp`)
Create a simple container for image data.
- Members: `width` (int), `height` (int), `channels` (int, usually 3 for RGB or 4 for RGBA), `data` (std::vector<uint8_t>).
- The `data` should store pixels in row-major order.

### 2. BitReader (`src/utils/bit_reader.hpp`)
Create a class to read bits from a byte buffer. This is crucial for parsing PNG/DEFLATE streams which are not byte-aligned.
- Methods:
    - `uint32_t readBits(int n)`: Reads `n` bits and returns them.
    - `void advanceBits(int n)`: Skips `n` bits.
    - `bool hasMore()`: Checks if stream is valid.
    - **Note**: PNG/DEFLATE reads bits from *LSB to MSB* within a byte (usually), but verify the spec. (Actually, DEFLATE is LSB-first).

### 3. BitWriter (`src/utils/bit_writer.hpp`)
Create a class to write bits to a buffer. Crucial for JPEG Huffman coding.
- Methods:
    - `void writeBits(uint32_t value, int n)`: Writes `n` bits of `value`.
    - `std::vector<uint8_t> getData()`: Flushes pending bits and returns the buffer.
    - **Note**: JPEG writes bits from *MSB to LSB*.

### 4. Checksums (`src/utils/checksum.hpp`)
Implement CRC32 and Adler32 algorithms.
- `uint32_t crc32(const uint8_t* data, size_t length)`: Used for PNG chunks.
- `uint32_t adler32(const uint8_t* data, size_t length)`: Used for Zlib streams (inside IDAT).
- Use precomputed tables for CRC32 if possible, or generate them at runtime for simplicity.

### 5. Makefile
Create a `Makefile` to compile the project.
- Compiler: `clang++` or `g++`.
- Flags: `-std=c++17 -O3 -Wall -Wextra`.
- Target: `converter` (though we don't have a main yet, create a dummy `main.cpp` in root to test compilation if needed, or just ensure object files build).

## Definition of Done
- All files created in the specified paths.
- Code compiles without errors or warnings.
- Basic unit tests (can be simple `assert`s in a temporary main) verify that `BitReader` and `BitWriter` work correctly (e.g., write 0xDEADBEEF bits, read them back).
