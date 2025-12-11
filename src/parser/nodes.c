#include "nodes.h"

typedef struct {
    extends_Declaration;
    VariableDeclarationList arguments;
    Scope* body;
} FunctionDeclaration;

void comp_FunctionDeclaration(FunctionDeclaration*, str*, Compiler*);

typedef struct {
    extends_Type;
    TypeList signature;
    FunctionDeclaration* declaration;
    ssize_t typedef_id;
} FunctionType;

void comp_FunctionType(FunctionType*, str*, Compiler*);

typedef struct {
    extends_Node;
    Node* value;
} ReturnStatement;

void comp_ReturnStatement(ReturnStatement*, str*, Compiler*);

typedef struct {
    extends_Node;
    Node* function;
    NodeList arguments;
} FunctionCall;

void comp_FunctionCall(FunctionCall*, str*, Compiler*);

typedef struct {
    extends_Type;
    Type* base;
} PointerType;

void comp_PointerType(PointerType*, str*, Compiler*);

typedef struct GenericType {
    extends_Type;
    Declaration* declaration;
    size_t index;
} GenericType;

void comp_GenericType(GenericType*, str*, Compiler*);

typedef struct {
    extends_Type;
    VariableDeclarationList fields;
    Scope* body;
    VariableDeclaration* parent;
    unsigned compiled_declaration : 1;
} StructType;

void comp_StructType(StructType*, str*, Compiler*);

typedef struct {
    extends_Node;
    NodeList fields;
    strs field_names;
} StructLiteral;

void comp_structLiteral(StructLiteral*, str*, Compiler*);

typedef struct {
    extends_Node;
    str keyword;
    NodeList inputs;
    Scope* body;
} Control;

void comp_Control(Control*, str*, Compiler*);

typedef struct {
    extends_Node;
    str prefix;
    Node* child;
} Prefix;

void comp_Prefix(Prefix*, str*, Compiler*);

typedef struct {
    extends_Node;
    str postfix;
    Node* child;
    unsigned no_wrap : 1;
} Postfix;

void comp_Postfix(Postfix*, str*, Compiler*);

union Type {
    struct {
        extends_Type;
    };

    Wrapper Wrapper;
    External External;
    GenericType GenericType;
    Missing Missing;

    FunctionType FunctionType;
    PointerType PointerType;

    StructType StructType;
};

union Declaration {
    struct {
        extends_Declaration;
    };

    VariableDeclaration VariableDeclaration;
    FunctionDeclaration FunctionDeclaration;
};

union Node {
    struct {
        extends_Node;
    };

    NumericLiteral NumericLiteral;
    Identifier Identifier;
    Scope Scope;
    Missing Missing;
    External External;
    Wrapper Wrapper;
    Prefix Prefix;
    Postfix Postfix;

    Type Type;

    Declaration Declaration;
    VariableDeclaration VariableDeclaration;
    BinaryOperation BinaryOperation;
    FunctionDeclaration FunctionDeclaration;
    FunctionCall FunctionCall;

    Statement Statement;
    ReturnStatement ReturnStatement;
    StructLiteral StructLiteral;
    Control Control;
};

void comp_Ignore(Node* self, str* line, Compiler* compiler) {
}

enum {
    fType = 1 << 0,
    fConst = 1 << 1,
    fConstExpr = 1 << 2,
    fMutable = 1 << 3,
    fIgnoreStatment = 1 << 4,
    fStatementTerminated = 1 << 5,
    fExternal = 1 << 6,

    fSize,
    afStart = fSize - 1,

    tfNumeric = afStart << 1,
};

typedef Vector(Nodes) NodeArena;

NodeArena global_node_arena = {0};
NodeList global_node_unused = {0};

__attribute__ ((constructor))

void init_node_arena() {
    push(&global_node_arena, (Nodes){0});
    resv(global_node_arena.data, 1024);
}

Node* new_node(Node node) {
    if (global_node_unused.size)
    {
        Node* box = global_node_unused.data[--global_node_unused.size];
        *box = node;
        return box;
    }

    Nodes* vector = &last(global_node_arena);

    if (vector->cap == vector->size)
    {
        Nodes section = {0};
        resv(&section, vector->cap * 2);
        push(&global_node_arena, section);
        vector = &last(global_node_arena);
    }

    Node* box = vector->data + vector->size;
    push(vector, node);
    return box;
}

void unbox(Node* box) {
    if (box /* I'm not looking for where I unboxed null */)
        push(&global_node_unused, box);
}

Type* new_type(Type type) {
    type.flags |= fType;
    Type* box = (void*)new_node((Node){.Type = type});
    box->type = box;
    return box;
}
