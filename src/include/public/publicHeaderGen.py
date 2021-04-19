def getFileContents(file) -> str:
    with open(file, "r") as file:
        return file.read()


prefix = '../'
files = [
    'HashMasm.h',
    'binaryFile.h',
    'loadCommands.h',
    'stringTable.h',
    'machoBinFile.h',
    'machoStructure.h',
    'relocateStruct.h',
    'objectGenerator.h'
]

result = ""

for file in files:
    result += "//\n//File contents: " + file + "\n//\n"
    result += getFileContents(prefix + file)

template = getFileContents("MachOBuilder_template.h")
template = template.replace("{{CONTENT}}", result)

with open("MachOBuilder.h", "wb") as outFile:
    outFile.write(template.encode("utf-8"))
