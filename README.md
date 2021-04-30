# MachOBuilder

Executable and object mach-o file builder.

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

```java
FILE *res = fopen(..., "wb");
binaryFile binary = {};
binary.init(res);
...
binary.dest();
```

The executable itself is generated using  ```MachoFileBin```

```java
MachoFileBin machoFile = {};
machoFile.init();
...
machoFile.binWrite(&binary);
machoFile.dest();
```

### Simple return zero executable

```java
FILE *res = fopen("machoRetZeroApp", "wb"); // executable
binaryFile binary = {};
binary.init(res);

MachoFileBin machoFile = {};
machoFile.init();

machoFile.header = machHeader64::executable(); // standard header
machoFile.loadCommands.pushBack(loadCommand::pageZero()); // add PAGEZERO load command 

auto codeSection = loadCommand::code();                // __TEXT segment
codeSection.sections.pushBack(segmentSection::code()); // push __text section to __TEXT segment
codeSection.payloads.pushBack(0);                      // linked to first payload
machoFile.loadCommands.pushBack(codeSection);

machoFile.loadCommands.pushBack(loadCommand::thread(0));           // Add UNIXTHREAD load command referencing to first section (codeSection)

unsigned char asmCode[] = {
        0x48, 0x89, 0xC7, 0xB8, 0x01, 0x00, 0x00, 0x02, 0x0F, 0x05 // asm code to execute return 0 syscall
};

binPayload codePayload = {};
codePayload.payload = (char *) asmCode;
codePayload.size = sizeof(asmCode) / sizeof(asmCode[1]);
codePayload.freeable = false;             // static array asmCode, no free() required
codePayload.align = 1;                    // 2^1 alignment of payload block
machoFile.payload.pushBack(codePayload);


machoFile.binWrite(&binary); // generate executable and write to the binary

binary.dest();
machoFile.dest();
```

Or as simple as this:

```java
FILE *res = fopen("machoRetZeroApp", "wb");
binaryFile binary = {};
binary.init(res);

unsigned char asmCode[] = {
        0x48, 0x89, 0xC7, 0xB8, 0x01, 0x00, 0x00, 0x02, 0x0F, 0x05
};
MachoFileBin::simpleExe(binary, (char*) asmCode, sizeof(asmCode));

binary.dest();
```

### Simple object file

There is an API for creating an object file that can be linked using ld / gcc / clang

```java
FILE *res = fopen("machoObjectAuto.o", "wb");
binaryFile binary = {};
binary.init(res);

ObjectMachOGen mgen = {};
mgen.init();

unsigned char asmCode[] = {
        0x55, 0x48, 0x89, 0xE5,
        0xE8, 0x00, 0x00, 0x00, 0x00, // call __Z8printTenv
        0xE8, 0x00, 0x00, 0x00, 0x00, // call __Z8printTenv
        0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, // mov eax, dword ptr [rip + offset globalVar ]
        0x31, 0xC0, 0x5D,
        0xE8, 0x00, 0x00, 0x00, 0x00,
        0xC3
};

unsigned char data[] = {
        0xDE, 0xD3, 0x2D, 0xED, 0x32, 0xDE, 0xD3, 0x2D, 0xED, 0x32,
        0xDE, 0xD3, 0x2D, 0xED, 0x32, 0xDE, 0xD3, 0x2D, 0xED, 0x32,
};

mgen.addCode(asmCode, sizeof(asmCode));
mgen.addData(data, sizeof(data));

mgen.addInternalCodeSymbol("_main", 0);
mgen.addInternalDataSymbol("globalVar", 0);

mgen.bindBranchExt("__Z5printi", 0x5);
mgen.bindBranchExt("__Z5printi", 0xA);
mgen.bindSignedOffset("globalVar", 16);

mgen.dumpFile(binary);

mgen.dest();
binary.dest();
```

It will create such structure:

<img width="400px" src="https://github.com/AlexRoar/MachOBuilder/raw/main/assets/struct.png">

Several essential functions are used in the listing.

`addInternalCodeSymbol` defines symbol at *offset* in code section
- symbol - symbol name
- offset - offset from the start of the section

`addInternalDataSymbol` defines symbol at *offset* in data section
- symbol - symbol name
- offset - offset from the start of the section

`bindBranchExt` defines address position for relocation for external symbol
- symbol - symbol name
- offset - address offset from the start of the code section

`bindSignedOffset` defines address position for relocation for internal symbol
- symbol - symbol name
- offset - address offset from the start of the code section
