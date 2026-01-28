# CLAUDE.md - AI Assistant Guide for DBAP

## Project Overview

**DBAP (Distance-Based Amplitude Panning)** is a SuperCollider server plugin implementing spatial audio panning across multiple speakers based on source distance. This is a C++/SuperCollider project that improves upon the original DBAP algorithm by Lossius et al. to fix spatial distortion issues when sources move outside the convex hull of speakers.

**Author:** Jacob Sundstrom (jacob.sundstrom@gmail.com)
**License:** GNU General Public License v3
**Paper:** [Speaker Placement Agnosticism: Improving the Distance-based Amplitude Panning Algorithm](https://arxiv.org/abs/2109.08704) (arXiv:2109.08704)

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

## Algorithm Reference (from Paper)

This section documents the complete algorithm from the Sundstrom paper for implementation reference.

### Original DBAP (Lossius et al., 2009)

The gain `v_i` for the i-th loudspeaker:
```
v_i = (k * w_i) / d_i^a
```

Where:
```
k = 1 / sqrt(Σ w_i² / d_i^(2a))        # normalization constant
a = R / (20 * log10(2))                 # rolloff coefficient (R in dB)
d_i = sqrt((x_i - x_s)² + (y_i - y_s)² + r_s²)   # distance with spatial blur
```

- `N` = number of loudspeakers
- `w_i` = weight for i-th loudspeaker (typically 1)
- `r_s` = spatial blur factor
- `R` = rolloff in decibels (typically 6 dB)

### Problems with Original DBAP

1. **Nonunique projections**: Sources outside the hull projecting to the same vertex become spatially indistinguishable
2. **Shaded region problem**: Areas beyond vertices where movement appears to stop
3. **Power discontinuities**: Undulating power when sources cross the hull boundary
4. **Convex hull complexity**: Resource-intensive in 3D

### Sundstrom's Improvements

#### 1. Variable `p` for Power Scaling

Modified normalization constant:
```
k = p^(2a) / sqrt(Σ w_i² / d_i^(2a))
```

Where:
```
p = min(1, q)
q = max(d_s) / d_rs
```

- `max(d_s)` = distance from reference point to the most distant speaker
- `d_rs` = distance from reference point to the virtual source
- Reference point is typically the field centroid

This creates a virtual boundary circle, eliminating the need for convex hull calculation.

#### 2. Biasing Parameter `b_i` for Far Sources

For sources far outside the field:
```
v_i = (k * w_i * b_i) / d_i^a
k = p^(2a) / sqrt(Σ b_i² * w_i² / d_i^(2a))
```

Where:
```
b_i = ((u_i / u_m) * (1/p - 1))² + 1
u_i = (d_i - max(d))_normalized² + ε
```

- `m` = index of median-distance loudspeaker from source
- `max(d)` = distance to farthest loudspeaker
- `ε` = small value to avoid zero gain (typically `r_s / N`)

Speakers closer to the source than the median get more weight.

#### 3. Automatic Spatial Blur

```
r_s = (Σ d_ic / N) * r_scalar
```

- `d_ic` = distance from centroid to i-th loudspeaker
- `r_scalar` = typically 0.2 to 0.5

## Current Implementation Status

### What's Implemented (DBAP.cpp)

| Feature | Location | Notes |
|---------|----------|-------|
| Basic gain formula `v_i = kw_i/d_i^a` | Line 132 | `calcGainWithK()` |
| Original k formula | Line 91 | `calcK()` uses `sqrtSumOfDists` |
| Rolloff coefficient `a` | Line 86 | `calcA()` uses `R_20LOG` constant |
| Distance with blur | Line 103 | Adds +1 to prevent division issues |
| Convex hull detection | Line 70, 225 | Uses `boost::geometry::within()` |
| Projection onto hull | Lines 155-202 | `getNearestPoint()`, `projectPoint()` |
| Geometric mean for outside | Line 124 | `sqrt(projectedDist * realDist)` |

### What's NOT Implemented (Paper Features)

| Feature | Paper Equation | Priority |
|---------|---------------|----------|
| Variable `p` | `k = p^(2a) / sqrt(...)` | **High** - Core improvement |
| Biasing `b_i` | Full equations above | **Medium** - For far sources |
| Reference point/centroid | Used in `p` calculation | **High** - Required for `p` |
| Auto spatial blur | `r_s = (Σd_ic/N) * r_scalar` | **Low** - Convenience |

### Implementation Roadmap

To align the code with the paper:

1. **Add centroid calculation** in constructor or `DBAPSpeakerArray`
2. **Add member variables**: `centroid`, `maxSpeakerDist`, `p`
3. **Implement `calcP()`**: Calculate `p = min(1, max(d_s) / d_rs)`
4. **Modify `calcK()`**: Include `p^(2a)` in numerator
5. **Implement `calcBias()`**: Calculate `b_i` for each speaker
6. **Modify `getDists()`**: Include biasing in sum calculation
7. **Optionally remove convex hull code**: The `p` method eliminates this need

### Key Code Locations for Modifications

- **DBAP.hpp:77**: Add `p`, `centroid`, `maxSpeakerDist` member variables
- **DBAP.hpp:27-33**: Add `bias` field to `speaker` struct
- **DBAP.cpp:85-92**: Modify `calcA()` and `calcK()` for new formulas
- **DBAP.cpp:95-118**: Modify `getDists()` to calculate `p` and biasing
- **DBAP.cpp:121-128**: Modify `calcGain()` to use new method

### Code to Remove (No Longer Needed)

With the `p` method, convex hull calculation is eliminated:

- **array.sc**: Graham Scan algorithm (or make optional for visualization)
- **DBAP.hpp:35-40**: `convexHullStruct` struct
- **DBAP.hpp:46-47**: `convexHull`, `nearestSegment` members
- **DBAP.cpp**: `insideConvexHull()`, `getNearestPoint()`, `projectPoint()` methods
- **DBAP.sc**: Convex hull buffer creation in `makeBuffer()`

## Future Enhancements

### Variable Reference Point

The reference point used in `p` calculation is typically the field centroid, but the paper notes it can be modulated. Potential use cases:

- **Listener tracking**: Reference follows a listener moving through the field
- **Dynamic focus**: Shift the "center" of the spatial field in real-time
- **Asymmetric layouts**: Use a point other than geometric centroid

Implementation would add a `referencePoint` parameter (defaulting to centroid) that can be updated at control rate.

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
- Uses Boost Geometry types (`point`, `polygon`, `segment`)

**DBAP.cpp** - Implementation:
- `calcA()`: Calculate rolloff parameter `a = R / (20 * log10(2))`
- `calcK()`: Calculate normalization constant
- `getDists()`: Compute distances from source to speakers
- `calcGain()`: Calculate amplitude gains (currently uses geometric mean for outside-hull)
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

## Coding Conventions

### C++

- Namespace wrapping: `namespace DBAP { ... }`
- Member variables use contextual prefixes (e.g., `m_fbufnum`)
- Boost Geometry typedefs for readability (`point`, `polygon`, `segment`)
- Standard: C++11
- Constants defined as macros: `MAX_SPEAKERS`, `R_20LOG`

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
5. **Current implementation doesn't fully match paper** - see Implementation Status above

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

- [Sundstrom Paper](https://arxiv.org/abs/2109.08704) - Speaker Placement Agnosticism (arXiv:2109.08704)
- [Original DBAP Paper](https://pdfs.semanticscholar.org/132a/028b9febadd03f2c75e5f79ca500c2dd04fd.pdf) - Lossius et al.
- [SuperCollider](https://supercollider.github.io/) - Audio synthesis platform
- [Boost Geometry](https://www.boost.org/doc/libs/release/libs/geometry/) - Computational geometry library
