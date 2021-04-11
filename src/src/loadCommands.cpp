//
// Created by Александр Дремов on 11.04.2021.
//

#include "loadCommands.h"
#include "binaryFile.h"

/*
 * Segment section
 */
void segmentSection::binWrite(binaryFile *out) {
    BINFILE_WRITE_STRUCT(section);
}

segmentSection segmentSection::code() {
    segmentSection sec = {};
    strcpy(sec.section.segname, SEG_TEXT);
    strcpy(sec.section.sectname, SECT_TEXT);
    sec.section.align = 1;
    sec.section.flags = S_REGULAR | S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;
    return sec;
}
