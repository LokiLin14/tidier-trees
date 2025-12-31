#include "trees.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct tree_t* new_tree_node(void) {
    static int next_tree_id = 0;
    struct tree_t* tree = (struct tree_t*) malloc(sizeof(struct tree_t));
    *tree = (struct tree_t){
        .left_child = NULL,
        .right_child = NULL,
        .id = next_tree_id++
    };
    return tree;
}

struct tree_t* tree_random(int min_height, float chance_to_continue) {
    struct tree_t* tree = NULL;
    if (min_height > 0) {
        tree = new_tree_node();
        assert(tree != NULL);
        tree->left_child = tree_random(min_height - 1, chance_to_continue);
        tree->right_child = tree_random(min_height - 1, chance_to_continue);
    } else if (((float)rand() / (float)RAND_MAX ) < chance_to_continue) {
        tree = new_tree_node();
        assert(tree != NULL);
        tree->left_child = tree_random(0, chance_to_continue);
        tree->right_child = tree_random(0, chance_to_continue);
    }
    return tree;
}

struct tree_t* tree_copy(struct tree_t *tree) {
    if (tree == NULL) {
        return NULL;
    }
    struct tree_t* root = malloc(sizeof(struct tree_t));
    struct tree_t* left_child = tree_copy(tree->left_child);
    struct tree_t* right_child = tree_copy(tree->right_child);
    if (!root || (tree->left_child && !left_child) || (tree->right_child && !right_child)) {
        tree_free(root);
        tree_free(left_child);
        tree_free(right_child);
        return NULL;
    }
    *root = (struct tree_t) {
        .id = tree->id,
        .x_pos = tree->x_pos,
        .y_pos = tree->y_pos,
        .left_child = left_child,
        .right_child = right_child,
    };
    return root;
}

void tree_free(struct tree_t* tree) {
    if (tree != NULL) {
        tree_free(tree->left_child);
        tree_free(tree->right_child);
        free(tree);
    }
}

// An internal tree struct that repurposes the space of the tree to calculate tree
// layout without memory allocations
struct tree_internal_t {
    int id;

    int x_offset;

    bool is_leaf;
    union {
        struct {
            struct tree_internal_t *left_child, *right_child;
        };
        struct {
            struct tree_internal_t *next_contour;
            int contour_offset;
        };
    };
};

_Static_assert(sizeof(struct tree_t) >= sizeof(struct tree_internal_t),
               "tree_t and tree_internal_t must have same size");

static struct tree_internal_t* to_internal(struct tree_t* tree) {
    if (!tree) {
        return NULL;
    }
    struct tree_internal_t internal_tree = {
        .id = tree->id,
        .x_offset = 0,
        .is_leaf = tree->left_child != NULL && tree->right_child != NULL,
    };
    if (!internal_tree.is_leaf) {
        internal_tree.left_child = to_internal(tree->left_child);
        internal_tree.right_child = to_internal(tree->right_child);
    }
    if (internal_tree.is_leaf) {
        internal_tree.next_contour = NULL;
        internal_tree.contour_offset = 0;
    }
    memcpy(tree, &internal_tree, sizeof(struct tree_t));
    return (struct tree_internal_t*)tree;
}

static struct tree_t* to_external(struct tree_internal_t* tree, int x_loc, int y_loc) {
    if (!tree) {
        return NULL;
    }
    x_loc += tree->x_offset;
    const struct tree_t external_tree = {
        .id = tree->id,
        .x_pos = x_loc, .y_pos = y_loc,
        .left_child = tree->is_leaf ? NULL : to_external(tree->left_child, x_loc, y_loc - 1),
        .right_child = tree->is_leaf ? NULL : to_external(tree->right_child, x_loc, y_loc - 1)
    };
    memcpy(tree, &external_tree, sizeof(struct tree_t));
    return (struct tree_t*)tree;
}

// I also should figure out a way to safely optimise away copying of id, (and in the future bigger chunks of data)
// since they share the same memory location, the memcpy of the id is unnecessary
// really all internal functions for calculating the layout should be somehow statically verified to not touch the data

// test that to_external(to_internal(tree)) is an identity on the memory location of tree

static void next_left_contour(struct tree_internal_t** tree_ptr, int* offset_ptr) {
    if (*tree_ptr == NULL) {
        return;
    }
    struct tree_internal_t tree = **tree_ptr;
    if (tree.is_leaf) {
        *tree_ptr = tree.next_contour;
        *offset_ptr += tree.next_contour ? tree.next_contour->x_offset : 0;
    } else {
        *tree_ptr = tree.left_child;
        *offset_ptr += tree.left_child ? tree.left_child->x_offset : 0;
    }
}

static void next_right_contour(struct tree_internal_t** tree_ptr, int* offset_ptr) {
    if (*tree_ptr == NULL) {
        return;
    }
    struct tree_internal_t tree = **tree_ptr;
    if (tree.is_leaf) {
        *tree_ptr = tree.next_contour;
        *offset_ptr += tree.next_contour ? tree.next_contour->x_offset : 0;
    } else {
        *tree_ptr = tree.right_child;
        *offset_ptr += tree.right_child ? tree.right_child->x_offset : 0;
    }
}

static bool is_end_of_contour(struct tree_internal_t* tree) {
    return !tree || (tree->is_leaf && tree->next_contour == NULL);
}

void compute_offsets(struct tree_internal_t* tree) {
    if (!tree || tree->is_leaf) {
        return;
    }
    // layouts the subtrees
    compute_offsets(tree->left_child);
    compute_offsets(tree->right_child);

    // computes how much to shift the left and right subtrees
    int required_offset = 2;
    int left_left_offset = 0, left_right_offset = 0, right_left_offset = 0, right_right_offset = 0;
    struct tree_internal_t* left_left_tree = tree->left_child, *left_right_tree = tree->left_child,
                            *right_left_tree = tree->right_child, *right_right_tree = tree->right_child;
    while (left_right_tree != NULL && right_left_tree != NULL) {
        // change the minimum offset if overlap
        if (right_left_offset - left_left_offset < MINIMUM_NODE_OFFSET) {
            required_offset = max_int(required_offset, right_left_offset - left_left_offset);
        }
        // go to next layer
        if (is_end_of_contour(left_right_tree) || is_end_of_contour(right_left_tree)) {
            break;
        }
        next_left_contour(&left_left_tree, &left_left_offset);
        next_right_contour(&left_right_tree, &left_right_offset);
        next_left_contour(&right_left_tree, &right_left_offset);
        next_right_contour(&right_right_tree, &right_right_offset);
    }
    required_offset = (required_offset + 1) / 2;
    if (tree->left_child) {
        tree->left_child->x_offset -= required_offset;
        left_left_offset -= required_offset; left_right_offset -= required_offset;
    }
    if (tree->right_child) {
        tree->right_child->x_offset += required_offset;
        right_left_offset += required_offset; right_right_offset += required_offset;
    }

    // stitches the subtree contours together
    next_right_contour(&left_right_tree, &left_right_offset);
    next_left_contour(&right_left_tree, &right_left_offset);
    if (left_left_tree != NULL && right_left_tree != NULL) {
        assert(left_left_tree->is_leaf);
        left_left_tree->next_contour = right_left_tree;
        left_left_tree->contour_offset = right_left_offset - left_left_offset;
    }
    if (right_right_tree != NULL && left_right_tree != NULL) {
        assert(left_right_tree->is_leaf);
        right_right_tree->next_contour = left_right_tree;
        right_right_tree->contour_offset = left_right_offset - right_right_offset;
    }
}

void tree_compute_layout(struct tree_t* tree) {
    if (!tree) {
        return;
    }
    struct tree_internal_t *internal_tree = to_internal(tree);
    compute_offsets(internal_tree);
    to_external(internal_tree, 0, 0);
}

// Helper function to count digits in an integer
static int count_digits(int n) {
    if (n == 0) return 1;
    int count = 0;
    if (n < 0) {
        count = 1; // for minus sign
        n = -n;
    }
    while (n > 0) {
        count++;
        n /= 10;
    }
    return count;
}

// Helper function to calculate required buffer size
static size_t calculate_size(struct tree_t* tree) {
    if (!tree) return 4; // "NULL"

    // Count characters for field names and formatting
    size_t size = 0;
    size += 8;  // "{ .id = "
    size += count_digits(tree->id);
    size += 11; // ", .x_pos = "
    size += count_digits(tree->x_pos);
    size += 11; // ", .y_pos = "
    size += count_digits(tree->y_pos);
    size += 16; // ", .left_child = "
    size += calculate_size(tree->left_child);
    size += 17; // ", .right_child = "
    size += calculate_size(tree->right_child);
    size += 2;  // " }"

    return size;
}

// Helper function to append string and update position
static void append_str(char** buf, size_t* pos, const char* str) {
    size_t len = strlen(str);
    memcpy(*buf + *pos, str, len);
    *pos += len;
}

// Recursive helper function
static void tree_to_string_helper(struct tree_t* tree, char** buf, size_t* pos) {
    if (!tree) {
        append_str(buf, pos, "NULL");
        return;
    }

    char temp[200];

    append_str(buf, pos, "{ .id = ");
    snprintf(temp, sizeof(temp), "%d", tree->id);
    append_str(buf, pos, temp);

    append_str(buf, pos, ", .x_pos = ");
    snprintf(temp, sizeof(temp), "%d", tree->x_pos);
    append_str(buf, pos, temp);

    append_str(buf, pos, ", .y_pos = ");
    snprintf(temp, sizeof(temp), "%d", tree->y_pos);
    append_str(buf, pos, temp);

    append_str(buf, pos, ", .left_child = ");
    tree_to_string_helper(tree->left_child, buf, pos);

    append_str(buf, pos, ", .right_child = ");
    tree_to_string_helper(tree->right_child, buf, pos);

    append_str(buf, pos, " }");
}

char* tree_to_string(struct tree_t* tree) {
    size_t size = calculate_size(tree) + 1;
    char* buf = malloc(size);
    if (!buf) return NULL;

    size_t pos = 0;
    tree_to_string_helper(tree, &buf, &pos);
    buf[pos] = '\0';

    return buf;
}

// Examples of trees
struct tree_t singleton_tree = {
    .id = 0,
    .left_child = NULL,
    .right_child = NULL,
};
struct tree_t left_leaning_tree = {
    .id = 1,
    .left_child = &singleton_tree,
    .right_child = NULL,
};
struct tree_t right_leaning_tree = {
    .id = 2,
    .left_child = NULL,
    .right_child = &singleton_tree
};