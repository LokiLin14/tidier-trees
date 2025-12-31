#ifndef TIDIERTREES_TREES_H
#define TIDIERTREES_TREES_H

struct tree_t {
    int id;
    int x_pos, y_pos;
    struct tree_t *left_child, *right_child;
};

struct tree_t* tree_random(int min_height, float chance_to_continue);
struct tree_t* tree_copy(struct tree_t *tree);
void tree_free(struct tree_t* tree);
void tree_compute_layout(struct tree_t* tree);
char* tree_to_string(struct tree_t* tree);

// Definitions that configures compute_layout
#define MINIMUM_NODE_OFFSET 1

// Examples of trees
extern struct tree_t singleton_tree;
extern struct tree_t left_leaning_tree;
extern struct tree_t right_leaning_tree;

#endif //TIDIERTREES_TREES_H