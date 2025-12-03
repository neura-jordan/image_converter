# Task: Phase 2 - PNG Decoder Implementation

## Role
You are a Senior C++ Systems Engineer. You are an expert in file formats and compression algorithms.

## Context
We have completed Phase 1 (Foundation). Now we need to implement the **PNG Decoder**. This is the most complex part of the project because it requires a custom implementation of the DEFLATE algorithm (RFC 1951) and PNG filtering (RFC 2083).

## Objective
Implement a fully functional PNG decoder that reads a `.png` file and produces a raw `Image` struct (RGB/RGBA).

## Requirements
- **No external libraries** (no zlib, no libpng).
- **Input**: Path to a PNG file.
- **Output**: Populated `Image` struct.
- **Scope**:
    - Support **Truecolor (RGB)** and **Truecolor+Alpha (RGBA)** (Color types 2 and 6).
    - Support **8 bits per channel**.
    - Support **all 5 filter types** (None, Sub, Up, Average, Paeth).
    - **DEFLATE**: Support Dynamic Huffman codes (BTYPE=10) and Fixed Huffman codes (BTYPE=01). Uncompressed blocks (BTYPE=00) are nice to have but less critical for typical PNGs.
    - **Interlacing**: You may SKIP interlacing for now (assume non-interlaced).

## Detailed Instructions

### 1. PNG Structure (`src/png_decoder.hpp`, `src/png_decoder.cpp`)
Create a `PngDecoder` class.
- **Signature Check**: Verify the first 8 bytes (`89 50 4E 47 0D 0A 1A 0A`).
- **Chunk Parsing**: Loop through chunks.
    - `IHDR`: Read width, height, bit depth, color type, compression method, filter method, interlace method.
    - `IDAT`: Concatenate the data from multiple IDAT chunks into one large buffer (this is the Zlib stream).
    - `IEND`: Stop parsing.
    - Ignore ancillary chunks (e.g., `gAMA`, `tEXt`) for now.

### 2. Zlib Wrapper
The `IDAT` data is a Zlib stream (RFC 1950).
- **Header**: Read CMF and FLG bytes (2 bytes). Verify check bits.
- **Data**: The rest is the DEFLATE stream.
- **Adler32**: At the end of the stream, verify the Adler32 checksum (using `src/utils/checksum.hpp`).

### 3. DEFLATE Decompression (The Hard Part)
Implement `inflate()` logic.
- Iterate through blocks. Read BFINAL and BTYPE.
- **BTYPE=00 (No Compression)**: Read LEN, NLEN, copy bytes.
- **BTYPE=01 (Fixed Huffman)**: Use pre-defined Huffman codes (defined in RFC 1951).
- **BTYPE=10 (Dynamic Huffman)**:
    - Read code length codes.
    - Build the code length Huffman tree.
    - Read Literal/Length and Distance code lengths.
    - Build the Literal/Length tree and Distance tree.
    - **Decode Loop**:
        - Decode symbol.
        - If < 256: Literal byte. Output it.
        - If == 256: End of block.
        - If > 256: It's a Length. Decode extra bits to get length. Decode Distance code and extra bits to get distance. Copy `length` bytes from `history` buffer at `distance` back.

### 4. Scanline Unfiltering
The output of DEFLATE is a stream of filtered scanlines.
- Structure: `[FilterType][R][G][B]... [FilterType][R][G][B]...`
- For each scanline:
    - Read the filter byte (0-4).
    - Reconstruct raw bytes based on the filter type and the previous reconstructed scanline.
    - **Paeth Filter**: Implement the `PaethPredictor` function exactly as specified in the PNG spec.

### 5. Pixel Conversion
- Convert the reconstructed scanline bytes into pixels in the `Image` struct.
- Handle RGB (3 bytes/pixel) and RGBA (4 bytes/pixel).

## Deliverables
- `src/png_decoder.hpp` and `src/png_decoder.cpp`.
- Update `src/main.cpp` to test the decoder with a sample PNG (you might need to generate a tiny raw PNG hex array in the test or read a file).
- **Verification**: The test should decode a known PNG and verify the pixel values match expected.

## Tips
- Use `BitReader` for all DEFLATE bit parsing.
- Debugging DEFLATE is hard. Print the decoded symbols if it fails.
- Start with a very simple non-compressed PNG if possible, or a Fixed Huffman one.
