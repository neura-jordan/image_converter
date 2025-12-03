# Task: Phase 5 - Advanced JPEG Encoding (Quality Control)

## Role
You are a Senior C++ Systems Engineer.

## Context
We have a working JPEG encoder, but it uses fixed standard quantization tables (roughly quality 50). We need to support variable quality settings (1-100) to allow users to trade off between file size and image fidelity.

## Objective
Implement quality scaling for the JPEG encoder and expose it via the CLI.

## Requirements
- **Quality Factor**: Integer from 1 to 100.
- **Algorithm**: Standard JPEG quality scaling formula (as used by IJG libjpeg).
    - If quality < 50: `scale = 5000 / quality`
    - If quality >= 50: `scale = 200 - 2 * quality`
    - `NewTable[i] = (StdTable[i] * scale + 50) / 100`
    - Clamp values between 1 and 255.
- **CLI**: Update `main.cpp` to accept `-q <quality>` or `--quality <quality>`. Default to 50 if not specified.

## Detailed Instructions

### 1. Update `src/jpeg_encoder.hpp` & `src/jpeg_encoder.cpp`
- Modify `encode` method to accept a `quality` integer: `static void encode(const Image& img, const std::string& filepath, int quality = 50);`
- Implement a helper function `generateQuantTable(const uint8_t* baseTable, int quality, uint8_t* outTable)`.
    - Apply the scaling formula above.
- In `encode`, generate the Luma and Chroma tables based on the requested quality before processing blocks.
- **Important**: The `DQT` marker writing must also use these new tables, not the static const ones.

### 2. Update `src/main.cpp`
- Update argument parsing to handle optional flags.
    - Support: `./converter input.png output.jpg` (default q=50)
    - Support: `./converter input.png output.jpg -q 90`
    - Support: `./converter input.png output.jpg --quality 90`
    - Handle invalid quality values (must be 1-100).

### 3. Verification
- Generate a high-quality JPEG (q=90) and a low-quality JPEG (q=10).
- Verify that the q=90 file is larger and looks better.
- Verify that the q=10 file is smaller and has more artifacts.

## Deliverables
- Updated `src/jpeg_encoder.hpp` and `src/jpeg_encoder.cpp`.
- Updated `src/main.cpp`.
