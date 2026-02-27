#include "modules.h"

#include "scope.h"
#include "statement.h"

String build_import_path(Trace* trace, Parser* parser, ModuleExtension* extension, str* top) {
    String path = NULL;

    do {
        if(try(parser->tokenizer, '*', NULL)) {
            *extension = ExtensionWildcard;
            break;
        }

        if(try(parser->tokenizer, '{', NULL)) {
            *extension = ExtensionVerbose;
            break;
        }

        const Trace section = expect(parser->tokenizer, TokenIdentifier).trace;
        *top = section.source;
        strf(&path, "/%.*s", fmtof(section.source));
        *trace = stretch(*trace, section);
    } while(try(parser->tokenizer, TokenDoubleColon, NULL));

    return strf(&path, ".qk").as_owned;
}

Parser find_import(String relative_path, str identifier, Trace trace, Parser* parser) {
    global_library_paths[0] = parser->dir_path;

    String import_path = NULL;
    for(size_t i = 0; i < len(global_library_paths); i++) {
        strf(&import_path, "%.*s%.*s%c", fmtof(global_library_paths[i]), fmtof(relative_path), 0);
        char* input_content = fs_readfile(import_path);

        if(!input_content) {
            len(import_path) = 0;
            continue;
        }

        Module** const module = get(global_modules, as_str(import_path));
        if(module) {
            return (Parser) { .module = *module };
        }

        const Tokenizer tokenizer = new_tokenizer(import_path, input_content, parser->tokenizer->messages);
        return create_parser((void*) new_node(hard_cast(Node, tokenizer)), as_str(import_path), false, identifier);
    }

    push(parser->tokenizer->messages,
         MERROR(trace, strf(0, "unable to open or read '%s'", import_path)));
    free(vbase(import_path));
    return (Parser) { 0 };
}

void import_wildcard(char* path, str identifier, Parser* parser, Vec(Node*)* collector) {
    Parser imported_parser = find_import(path, identifier, (Trace) { 0 }, parser);
    if(!imported_parser.module) panicf("Unable to import '%s'\n", path);
    collect_into(&imported_parser, &statement, 0, 0, collector);
    imported_parser.module->scope->flags = fPrivate;
    push(&last(parser->stack)->wildcards, imported_parser.module->scope);
}
