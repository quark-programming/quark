#include "nodes.h"

Vec(Vec(Node)) global_node_arena = { 0 };
Vec(Node*) global_unused_nodes = { 0 };

void init_node_arena(const size_t initial_size) {
    push(&global_node_arena, NULL);
    resv(global_node_arena, initial_size);
}

Node* new_node(const Node node) {
    if(len(global_unused_nodes)) {
        Node* box = *pop(&global_unused_nodes);
        *box = node;
        return box;
    }

    Vec(Node)* vector = &last(global_node_arena);

    if(vhead(*vector)->byte_capacity == len(vector) * sizeof(Node)) {
        Vec(Node) section = { 0 };
        resv(&section, vhead(*vector)->byte_capacity / sizeof(Node) * 2);
        push(&global_node_arena, section);
        vector = &last(global_node_arena);
    }

    Node* box = *vector + len(*vector);
    push(vector, node);
    return box;
}

void unbox(Node* box) {
    if(!box) return;
    push(&global_unused_nodes, box);
}
