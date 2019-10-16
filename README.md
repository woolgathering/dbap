# DBAP
[![Build Status](https://travis-ci.com/woolgathering/dbap.svg?branch=master)](https://travis-ci.com/woolgathering/dbap)

Author: Jacob Sundstrom

__This is pre-alpha and should NOT be deployed in the field!__ It is in a highly experimental stage.

Distance-based ampltide panning (DBAP). Based on the [paper](https://pdfs.semanticscholar.org/132a/028b9febadd03f2c75e5f79ca500c2dd04fd.pdf?_ga=2.103137216.1512247688.1571200723-789701753.1569525663) by Trond Lossius, Pascal Baltazar, and Théo de la Hogue.

### Requirements

- CMake >= 3.5
- SuperCollider source code
- Boost Geometry >= 1.65.1.0

### Building

Clone the project:

    git clone https://woolgathering/dbap
    cd dbap
    mkdir build
    cd build

Then, use CMake to configure and build it:

    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
    cmake --build . --config Release --target install

You may want to manually specify the install location in the first step to point it at your
SuperCollider extensions directory: add the option `-DCMAKE_INSTALL_PREFIX=/path/to/extensions`.

It's expected that the SuperCollider repo is cloned at `../supercollider` relative to this repo. If
it's not: add the option `-DSC_PATH=/path/to/sc/source`.

### Developing

Use the command in `regenerate` to update CMakeLists.txt when you add or remove files from the
project. You don't need to run it if you only change the contents of existing files. You may need to
edit the command if you add, remove, or rename plugins, to match the new plugin paths. Run the
script with `--help` to see all available options.
