#include "../parser/block.c"

typedef struct {
	strs lines;
	str indent;
} CompilerSection;
typedef Vector(CompilerSection) CompilerSections;

struct Compiler {
	CompilerSections sections;
	size_t open_section;
	Messages* messages;
};

str new_line(Compiler* compiler) {
	const str indent = compiler->sections.data[compiler->open_section].indent;
	return strf(0, "%.*s", (int) indent.size, indent.data);
}
