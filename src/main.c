#include "fs.c"
#include "clargs.c"
#include "compiler/block.c"

typedef Vector(char*) CStrings;

int in_compiler_step = 0;

void create_std_type(Scope* scope, str name, str internal) {
	Type* type = new_type((Type) { .External = {
			.compiler = (void*) &comp_External,
			.flags = fConstExpr | tfNumeric,
			.trace = { internal },
			.data = internal,
	}});

	put(scope, name, (void*) new_node((Node) { .VariableDeclaration = {
				.compiler = (void*) &comp_VariableDeclaration,
				.flags = fConst | fType,
				.const_value = (void*) type,
				.type = type,
	}}));
}

void insert_std_numerics(Scope* std_scope) {
	create_std_type(std_scope, str("i8"), str("int8_t"));
	create_std_type(std_scope, str("u8"), str("uint8_t"));
	create_std_type(std_scope, str("i16"), str("int16_t"));
	create_std_type(std_scope, str("u16"), str("uint16_t"));
	create_std_type(std_scope, str("i32"), str("int32_t"));
	create_std_type(std_scope, str("u32"), str("uint32_t"));
	create_std_type(std_scope, str("i64"), str("int64_t"));
	create_std_type(std_scope, str("u64"), str("uint64_t"));

	create_std_type(std_scope, str("f32"), str("float"));
	create_std_type(std_scope, str("f64"), str("double"));

	create_std_type(std_scope, str("isize"), str("ssize_t"));
	create_std_type(std_scope, str("usize"), str("size_t"));

	create_std_type(std_scope, str("char"), str("char"));
	create_std_type(std_scope, str("ichar"), str("signed char"));
	create_std_type(std_scope, str("uchar"), str("unsigned char"));

	create_std_type(std_scope, str("Short"), str("short"));
	create_std_type(std_scope, str("UShort"), str("unsigned short"));
	create_std_type(std_scope, str("Int"), str("int"));
	create_std_type(std_scope, str("UInt"), str("unsigned"));
	create_std_type(std_scope, str("Long"), str("long"));
	create_std_type(std_scope, str("ULong"), str("unsigned long"));

	create_std_type(std_scope, str("void"), str("void"));
	create_std_type(std_scope, str("bool"), str("bool"));
	create_std_type(std_scope, str("File"), str("FILE"));
}

FunctionDeclaration* entry_declaration() {
	FunctionType* function_type =
		(void*) new_type((Type) { .FunctionType = {
				.compiler = (void*) &comp_FunctionType,
		}});
	push(&function_type->signature, new_type((Type) { .External = {
				.compiler = (void*) &comp_External,
				.data = str("int"),
	}}));

	FunctionDeclaration* declaration =
		(void*) new_node((Node) { .FunctionDeclaration = {
				.compiler = (void*) &comp_FunctionDeclaration,
				.type = (void*) function_type,
				.identifier = (void*) new_node((Node) {
						.Identifier = {
							.compiler = (void*) &comp_Identifier,
							.base = str("main"),
						}
				}),
				.body = new_scope(0),
		}});
	function_type->declaration = declaration;

	return declaration;
}

int main(int argc, char** argv) {
	char* name = clname(argc, argv);

	CStrings input_files = { 0 };
	char* output_file = "out.c";

	char flag;
	while((flag = clflag())) switch(flag) {
		case -1: 
			push(&input_files, clarg());
			break;
		case 'o':
			output_file = clarg();
			break;
		default:
			panicf("unknown flag '-%c'\n", flag);
	}

	if(input_files.size == 0)
		panicf("no input files provided\n");

	char* input_content = fs_readfile(input_files.data[0]);
	if(!input_content) {
		perror("read");
		panicf("unable to read file '%s'\n",
				input_files.data[0]);
	}

	Messages messages = { 0 };
	Tokenizer tokenizer = new_tokenizer(input_files.data[0], input_content, &messages);
	Parser parser = { &tokenizer };
	Compiler compiler = { .messages = &messages };

	push(&compiler.sections, (CompilerSection) { 0 });
	push(&compiler.sections, (CompilerSection) { 0 });
	push(&compiler.sections.data[0].lines, str("#include <stdint.h>"));
	push(&compiler.sections.data[0].lines, str("#include <stdio.h>"));
	push(&compiler.sections.data[0].lines, str("#include <string.h>"));
	push(&compiler.sections.data[0].lines, str("#include <stdlib.h>"));
	push(&compiler.sections.data[0].lines, str("#include <stdbool.h>"));

	push(&parser.stack, new_scope(0));
	insert_std_numerics(parser.stack.data[0]);

	FunctionDeclaration* entry = entry_declaration();
	push(&parser.stack, entry->body);

	entry->body->children = collect_until(&parser, &statement, 0, 0);

	in_compiler_step = 1;
	str temp_line = { 0 };
	puts("COMPILATION START");
	entry->compiler((void*) entry, &temp_line, &compiler);
	puts("COMPILATION COMPLETE");

	int error = 0;
	for(size_t i = 0; i < messages.size; i++) {
		if(print_message(messages.data[i])) error = 1;
	}

	if(error) return 1;

	FILE* out = fopen(output_file, "w+");
	if(!out) {
		perror("open");
		panicf("unable to output file '%s' to write\n",
				output_file);
	}

	for(size_t i = 0; i < compiler.sections.size; i++) {
		if(i) fprintf(out, "\n");
		for(size_t j = 0; j < compiler.sections.data[i].lines.size;
				j++) {
			fprintf(out, "%.*s\n",
					(int) compiler.sections.data[i].lines.data[j].size,
					compiler.sections.data[i].lines.data[j].data);
		}
	}
}
