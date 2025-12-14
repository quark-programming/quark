#include "../nodes.h"

NodeArena global_node_arena = { 0 };
NodeVector global_unused_nodes = { 0 };

void init_node_arena(const size_t initial_size) {
    push(&global_node_arena, (AbsoluteNodeVector) { 0 });
    resv(global_node_arena.data, initial_size);
}

Node* new_node(const Node node) {
    if(global_unused_nodes.size) {
        Node* box = global_unused_nodes.data[--global_unused_nodes.size];
        *box = node;
        return box;
    }

    AbsoluteNodeVector* vector = &last(global_node_arena);

    if(vector->cap == vector->size) {
        AbsoluteNodeVector section = { 0 };
        resv(&section, vector->cap * 2);
        push(&global_node_arena, section);
        vector = &last(global_node_arena);
    }

    Node* box = vector->data + vector->size;
    push(vector, node);
    return box;
}

void unbox(Node* box) {
    if(box) {
        push(&global_unused_nodes, box);
    }
}
