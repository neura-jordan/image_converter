# Phase Progress: Raw C++ PNG to JPG Converter

This document tracks the progress of the project. As the CTO, I have broken down the project into distinct phases to be executed by our engineering team.

## Phase 1: Foundation & Utilities
**Goal**: Establish basic data structures and utility functions required for image processing and file handling.
- [x] **Task 1.1**: Define `Image` struct (width, height, channels, pixel data).
- [x] **Task 1.2**: Implement `BitReader` and `BitWriter` classes for bit-level I/O.
- [x] **Task 1.3**: Implement CRC32 (for PNG) and Adler32 (for Zlib) checksum utilities.
- [x] **Task 1.4**: Set up project structure and `Makefile` / build script.

## Phase 2: PNG Decoder Implementation
**Goal**: Implement a fully functional PNG decoder capable of reading raw PNG files and converting them to raw pixel data.
- [x] **Task 2.1**: Implement PNG Chunk Parser (Signature, IHDR, IEND).
- [x] **Task 2.2**: Implement IDAT Chunk concatenation.
- [x] **Task 2.3**: Implement **Custom DEFLATE Decompressor**.
    - [x] Huffman Table parsing (Code lengths, dynamic trees).
    - [x] Huffman Decoding logic.
    - [x] LZ77 Decompression (Distance/Length pairs).
- [x] **Task 2.4**: Implement Scanline Unfiltering (None, Sub, Up, Average, Paeth).
- [x] **Task 2.5**: Integrate all parts to decode a PNG into the `Image` struct.

## Phase 3: JPEG Encoder Implementation
**Goal**: Implement a JPEG encoder to convert raw pixel data into a valid JPEG file.
- [x] **Task 3.1**: Implement Color Space Conversion (RGB to YCbCr).
- [x] **Task 3.2**: Implement Block Splitting (8x8 blocks) and Padding.
- [x] **Task 3.3**: Implement Forward Discrete Cosine Transform (FDCT).
- [x] **Task 3.4**: Implement Quantization (Luminance and Chrominance tables).
- [x] **Task 3.5**: Implement ZigZag reordering and Run-Length Encoding.
- [x] **Task 3.6**: Implement Huffman Encoding (Standard JPEG tables).
- [x] **Task 3.7**: Implement JPEG File Structure (SOI, APP0, DQT, SOF0, DHT, SOS, EOI).

## Phase 4: Integration & Delivery
**Goal**: Combine the decoder and encoder into a CLI tool and verify correctness.
- [x] **Task 4.1**: Implement `main.cpp` CLI logic (args parsing).
- [x] **Task 4.2**: Connect `PngDecoder` output to `JpegEncoder` input.
- [x] **Task 4.3**: Perform end-to-end testing with various PNG images.
- [x] **Task 4.4**: Optimize performance (optional).
