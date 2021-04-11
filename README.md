# MachOBuilder

Executable mach-o file builder.

## Install

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make install
```

Check out console output to find install directory.

To use in cmake (as an example):

```cmake
SET(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}         -I/usr/local/include") # check your installation directory
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -L/usr/local/lib")
```

## Mach-O format quick guide

Mach-O file is structured like this:

<img width="400px" src="https://github.com/AlexRoar/MachOBuilder/raw/main/assets/machostruct.png">

### Important tips: 
- Load command size must be divisible by 8. It can be padded by zeros
- Minimal structure: PAGEZERO segment + TEXT segment(+text section) + UNIXTHREAD + code payload
- Mach-O file must be at least one page size. Framework automatically achives that.
- Segments virtual size nust be rounded to the page size.
- Segments payload must be page-aligned 
- UNIXTHREAD entry point type does not require anything appart from the code itself
- MAIN segment require: PAGEZERO + TEXT + LINKEDIT + LC_DYLD_INFO_ONLY + LC_DYSYMTAB + LC_LOAD_DYLINKER + LC_LOAD_DYLIB
## Usage

The main interface uses ```binaryFile``` as it's code container:

```C++
FILE *res = fopen(..., "wb");
binaryFile binary = {};
binary.init(res);
...
binary.dest();
```

The executable itself is generated using  ```MachoFileBin```

```C++
MachoFileBin machoFile = {};
machoFile.init();
...
machoFile.binWrite(&binary);
machoFile.dest();
```
