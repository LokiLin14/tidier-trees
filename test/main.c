#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "trees.c"

bool tree_value_equal(struct tree_t *tree, struct tree_t *other) {
    if (!tree || !other) {
        return (tree == NULL) && (other == NULL);
    }
    return tree->id == other->id &&
        tree_value_equal(tree->left_child, other->left_child)
        && tree_value_equal(tree->right_child, other->right_child);
}

struct tree_t* trees[] = {&singleton_tree, &left_leaning_tree, &right_leaning_tree};
int num_trees = sizeof(trees) / sizeof(trees[0]);

static void test_tree_value_equal(void **state) {

    for (int i = 0; i < num_trees; i++) {
        for (int ii = 0; ii < num_trees; ii++) {
            if (i != ii) {
                assert_false(tree_value_equal(trees[i], trees[ii]));
            } else {
                assert_true(tree_value_equal(trees[i], trees[ii]));
            }
        }
    }
}

static void test_external_internal_roundtrip(void **state) {
    for (int i = 0; i < num_trees; i++) {
        struct tree_t* tree = trees[i], *copy = tree_copy(tree);
        to_external(to_internal(copy), 0, 0);
        assert_true(tree_value_equal(tree, copy));
    }
}

static void test_layout_leaves_data_unchanged(void **state) {
    struct tree_t* trees[] = {&singleton_tree, &left_leaning_tree, &right_leaning_tree};
    int num_trees = sizeof(trees) / sizeof(trees[0]);

    for (int i = 0; i < num_trees; i++) {
        struct tree_t* tree = trees[i], *copy = tree_copy(tree);
        tree_compute_layout(copy);
        assert_true(tree_value_equal(tree, copy));
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tree_value_equal),
        cmocka_unit_test(test_external_internal_roundtrip),
        cmocka_unit_test(test_layout_leaves_data_unchanged)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}