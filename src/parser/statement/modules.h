#ifndef MODULES_H
#define MODULES_H

#include "../parser.h"

typedef enum {
    ExtensionNone,
    ExtensionWildcard,
    ExtensionVerbose,
} ModuleExtension;

String build_import_path(Trace* trace, Parser* parser, ModuleExtension* extension, str* top);

Parser find_import(String relative_path, Trace trace, Parser* parser);

#endif