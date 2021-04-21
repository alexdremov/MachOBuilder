//
// Created by Александр Дремов on 11.04.2021.
//

#ifndef MACHOBUILDER_MACHOBUILDER_TEMPLATE_H
#define MACHOBUILDER_MACHOBUILDER_TEMPLATE_H
#define PUBLIC_HEADER
#include <mach-o/loader.h>
#include <mach/machine.h>
#include <mach/mach.h>
#include <mach-o/reloc.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "FastList.h"

{{CONTENT}}

#endif //MACHOBUILDER_MACHOBUILDER_TEMPLATE_H
