# CLAUDE.md - AI Assistant Guide for DBAP

## Project Overview

**DBAP (Distance-Based Amplitude Panning)** is a SuperCollider server plugin implementing spatial audio panning across multiple speakers based on source distance. This is a C++/SuperCollider project that improves upon the original DBAP algorithm by Lossius et al. to fix spatial distortion issues when sources move outside the convex hull of speakers.

**Author:** Jacob Sundstrom (jacob.sundstrom@gmail.com)
**License:** GNU General Public License v3

## Repository Structure

```
dbap/
├── plugins/DBAP/           # Main plugin source code
│   ├── DBAP.cpp            # Core algorithm implementation (267 lines)
│   ├── DBAP.hpp            # Class and method definitions (85 lines)
│   ├── DBAP.sc             # SuperCollider UGen wrapper (173 lines)
│   ├── array.sc            # Graham Scan convex hull algorithm (73 lines)
│   ├── DBAP.schelp         # DBAP UGen documentation
│   ├── DBAPSpeakerArray.schelp  # Array class documentation
│   └── convexHull.png      # Help file image
├── cmake_modules/          # CMake build helpers
│   ├── SuperColliderServerPlugin.cmake
│   └── SuperColliderCompilerConfig.cmake
├── images/                 # Algorithm documentation figures
├── CMakeLists.txt          # Build configuration
├── CMakeSettings.json      # IDE build settings
├── .travis.yml             # Travis CI (Linux/macOS)
├── .appveyor.yml           # AppVeyor CI (Windows)
├── regenerate              # Script to update CMakeLists.txt
├── README.md               # Project documentation
└── LICENSE                 # GPLv3 license
```

## Build Instructions

### Requirements

- CMake >= 3.5
- SuperCollider source code
- Boost Geometry >= 1.65.1.0
- C++11 compatible compiler

### Building

```bash
# Clone and setup
mkdir build && cd build

# Configure (adjust paths as needed)
cmake .. -DCMAKE_BUILD_TYPE=Release -DSC_PATH=/path/to/supercollider

# Build and install
cmake --build . --config Release
cmake --build . --config Release --target install
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `SUPERNOVA` | ON | Build for multi-threaded server |
| `SCSYNTH` | ON | Build for single-threaded server |
| `NATIVE` | OFF | Optimize for native architecture |
| `STRICT` | OFF | Use strict warning flags |
| `NOVA_SIMD` | ON | Build with SIMD optimizations |
| `SC_PATH` | `../supercollider` | Path to SuperCollider source |
| `CMAKE_INSTALL_PREFIX` | `build/install` | Installation directory |

### Build Targets

- `DBAP_scsynth` - Plugin for scsynth (single-threaded server)
- `DBAP_supernova` - Plugin for Supernova (multi-threaded server)

## Code Architecture

### C++ Core (plugins/DBAP/)

**DBAP.hpp** - Class definition:
- `speaker` struct: position, gain, weight, distance calculations
- `convexHullStruct`: polygon representation for spatial boundaries
- Uses Boost Geometry types (`point`, `polygon`)

**DBAP.cpp** - Implementation:
- `calcA()`: Calculate rolloff parameter
- `calcK()`: Calculate normalization constant
- `getDists()`: Compute distances from source to speakers
- `calcGain()`: Calculate amplitude gains using geometric mean for outside-hull sources
- `getNearestPoint()`: Find closest point on convex hull
- `projectPoint()`: Orthogonal projection onto segment
- `next()`: Audio-rate processing with gain interpolation

### SuperCollider Classes (plugins/DBAP/)

**DBAP.sc**:
- `DBAP` class: MultiOutUGen wrapper for audio-rate panning
- `DBAPSpeakerArray` class: Speaker configuration and visualization
  - `makeBuffer()`: Create buffer with speaker data
  - `plot()`: Visualize speaker array and sources
  - `addSource()`, `modifySource()`, `removeSource()`: Source management
  - `scale_`: Scaling factor for visualization

**array.sc**:
- Array extension with `grahamScan2D` method for convex hull computation

## Key Technical Details

### Algorithm Constants

- **MAX_SPEAKERS**: 50 (defined in DBAP.hpp)
- **Minimum distance**: 1.0 (prevents division issues)
- **Position precision**: 0.01 (centimeter resolution)

### Algorithm Modifications from Original DBAP

The implementation differs from Lossius et al. in handling sources outside the convex hull:

1. Calculates both real distance (`d_ir`) and projected distance (`d_ip`)
2. Uses composite distance: `d_ic = sqrt(d_ir * d_ip)` for outside-hull sources
3. Prevents spatial collapse at vertices (the "shaded region problem")

### Performance Considerations

- Pre-computed convex hull (calculated once at initialization)
- Conditional distance calculations (skipped if parameters unchanged)
- Gain interpolation smooths control-rate changes
- Optional SIMD support via nova-simd

## Coding Conventions

### C++

- Namespace wrapping: `namespace DBAP { ... }`
- Member variables use contextual prefixes (e.g., `m_fbufnum`)
- Boost Geometry typedefs for readability
- Standard: C++11

### SuperCollider

- Class naming: `DBAP` (UGen), `DBAPSpeakerArray` (utility)
- Method naming: camelCase
- Class extensions: `+ ClassName { ... }` syntax
- Use `defer` for GUI operations

## Testing

There is no dedicated test suite. Testing is performed through:

1. **CI/CD builds** - Verify compilation on all platforms
2. **Manual testing** - Using SuperCollider IDE with help file examples

### CI/CD Platforms

- **Travis CI**: Linux (Ubuntu Trusty) and macOS
- **AppVeyor**: Windows (Visual Studio 2017, x86/x64)

Both automatically deploy release artifacts on git tags.

## Development Workflow

### Adding/Removing Files

Run the `regenerate` script to update CMakeLists.txt:
```bash
./regenerate
```

### File Organization

- C++ source files: `plugins/DBAP/*.cpp`, `plugins/DBAP/*.hpp`
- SuperCollider files: `plugins/DBAP/*.sc`
- Help files: `plugins/DBAP/*.schelp`

### Git Ignored

- `build*/` - All build directories
- `.vimrc` - Editor config

## Known Limitations

1. Graham Scan doesn't work with Point objects (use arrays instead)
2. No delay compensation between speakers (by design)
3. Positions rounded to 0.01 precision
4. Maximum 50 speakers supported

## Common Tasks

### Building for Development

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSC_PATH=/path/to/supercollider
cmake --build .
```

### Installing to SuperCollider Extensions

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=~/.local/share/SuperCollider/Extensions
cmake --build . --target install
```

### Running Tests

Manual testing through SuperCollider:
```supercollider
// Boot server and test
s.boot;
// Use examples from DBAP.schelp
```

## Dependencies Reference

| Dependency | Version | Purpose |
|------------|---------|---------|
| CMake | >= 3.5 | Build system |
| SuperCollider | source | Plugin host API |
| Boost Geometry | >= 1.65.1.0 | Convex hull, projections |
| Nova-SIMD | optional | SIMD optimizations |

## External Resources

- [Original DBAP Paper](https://pdfs.semanticscholar.org/132a/028b9febadd03f2c75e5f79ca500c2dd04fd.pdf) - Lossius et al.
- [SuperCollider](https://supercollider.github.io/) - Audio synthesis platform
- [Boost Geometry](https://www.boost.org/doc/libs/release/libs/geometry/) - Computational geometry library
