# Task: Phase 6 - Bidirectional Conversion (JPG -> PNG)

## Role
You are a Senior C++ Systems Engineer.

## Context
We have a working PNG -> JPG converter with quality control. Now we need to implement the reverse: **JPG -> PNG**. This requires writing a JPEG decoder from scratch.

## Objective
Implement a `JpegDecoder` class and update the CLI to support bidirectional conversion.

## Requirements
- **Input**: Path to a JPG file.
- **Output**: Path to a PNG file.
- **Auto-detection**: The CLI should detect the operation based on file extensions.
- **JPEG Decoder**:
    - Support Baseline DCT JPEGs.
    - Support 4:4:4, 4:2:2, and 4:2:0 subsampling (standard JPEGs often use 4:2:0).
    - **No external libraries**.

## Detailed Instructions

### 1. JPEG Decoder (`src/jpeg_decoder.hpp`, `src/jpeg_decoder.cpp`)
Create a `JpegDecoder` class.
- `static Image decode(const std::string& filepath);`
- **Parsing**:
    - Read markers: `SOI`, `SOF0` (Frame info), `DHT` (Huffman tables), `DQT` (Quant tables), `SOS` (Scan start), `EOI`.
    - Handle `APPn` and `COM` markers by skipping them.
- **Huffman Decoding**:
    - Implement decoding using the tables read from `DHT`.
- **Inverse Quantization**:
    - Multiply coefficients by the quantization table.
- **Inverse DCT (IDCT)**:
    - Implement 8x8 IDCT.
- **Color Conversion**:
    - YCbCr to RGB.
    - Handle subsampling (upsample Cb/Cr if needed).

### 2. Update `src/main.cpp`
- Detect mode:
    - If input ends in `.png` and output in `.jpg` -> Encode (existing).
    - If input ends in `.jpg`/`.jpeg` and output in `.png` -> Decode (new).
    - Else -> Error.
- Call `JpegDecoder::decode` for JPG inputs.
- Use a simple PNG encoder to save the result.
    - **Wait**: We don't have a PNG encoder yet!
    - **Scope Change**: For this task, just output a **Raw PPM** file (P6 format) if PNG encoding is too much work, OR implement a very simple uncompressed PNG encoder.
    - **Decision**: Implement a **Simple Uncompressed PNG Encoder**.
        - Write Signature.
        - Write IHDR.
        - Write IDAT (Uncompressed blocks, BTYPE=00). This avoids implementing DEFLATE compression logic.
        - Write IEND.
        - Calculate CRC32 for chunks.

## Deliverables
- `src/jpeg_decoder.hpp` / `.cpp`.
- Updated `src/main.cpp`.
- **Verification**: Convert a JPG to PNG and verify it opens.
