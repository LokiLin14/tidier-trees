#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdint.h>

#include "trees.c"

bool tree_value_equal(struct tree_t *tree, struct tree_t *other) {
    if (!tree || !other) {
        return (tree == NULL) && (other == NULL);
    }
    return tree->id == other->id &&
        tree_value_equal(tree->left_child, other->left_child)
        && tree_value_equal(tree->right_child, other->right_child);
}

int tree_height(struct tree_t *tree) {
    if (!tree) {
        return 0;
    }
    return 1 + max_int(tree_height(tree->left_child), tree_height(tree->right_child));
}

struct tree_t* trees[] = {&singleton_tree, &left_leaning_tree, &right_leaning_tree, &broken_contour_tree};
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

static void test_layout_leaves_data_unchanged(void **state) {
    for (int i = 0; i < num_trees; i++) {
        struct tree_t* tree = trees[i], *copy = tree_copy(tree);
        tree_compute_layout(copy);
        assert_true(tree_value_equal(tree, copy));
        tree_free(copy);
    }

    const int random_tree_tests = 1000;
    for (int i = 0; i < random_tree_tests; i++) {
        struct tree_t* tree = tree_random(1, 10, 0.2f), *copy = tree_copy(tree);;
        tree_compute_layout(copy);
        assert_true(tree_value_equal(tree, copy));
        tree_free(tree);
        tree_free(copy);
    }
}

static void test_tree_free_works(void **state) {
    for (int i = 0; i < num_trees; i++) {
        struct tree_t* tree = tree_copy(trees[i]);
        tree_free(tree);
    }
}

static void test_tree_height_measures_max_depth(void **state) {
    assert_int_equal(tree_height(&singleton_tree), 1);
    assert_int_equal(tree_height(&left_leaning_tree), 2);
    assert_int_equal(tree_height(&right_leaning_tree), 2);
}

int left_contour_length(struct tree_internal_t* tree) {
    int length = 0, offset = 0;
    while (tree) {
        next_left_contour(&tree, &offset);
        length++;
    }
    return length;
}

int right_contour_length(struct tree_internal_t* tree) {
    int length = 0, offset = 0;
    while (tree) {
        next_right_contour(&tree, &offset);
        length++;
    }
    return length;
}

static void test_tree_random_respects_max_and_min_heights(void **state) {
    for (int i = 0; i < 5; i++) {
        const int min_height = i;
        const int max_height = i + 5;
        const float chance_to_continue = 0.0f;
        struct tree_t* tree = tree_random(min_height, max_height, chance_to_continue);
        assert_true(tree_height(tree) >= min_height);
        assert_true(tree_height(tree) <= max_height);
        tree_free(tree);
    }
}

static void test_contour_has_length_equal_to_height_of_full_tree(void **state) {
    const int max_depth = 10;
    for (int i = 0; i < max_depth; i++) {
        const int min_height = i, max_height = i;
        struct tree_t* tree = tree_random(min_height, max_height, 0.0f);
        struct tree_internal_t* internal_tree = to_internal(tree);
        assert_int_equal(left_contour_length(internal_tree), right_contour_length(internal_tree));
        struct tree_t* external_tree = to_external(internal_tree, 0, 0);
        tree_free(external_tree);
    }
}

static void test_compute_layout_of_random_tree_has_same_length_contour(void **state) {
    for (int i = 0; i < num_trees; i++) {
        struct tree_t* tree = tree_copy(trees[i]);
        struct tree_internal_t* internal_tree = to_internal(tree);
        compute_offsets(internal_tree);

        // Print error for debugging
        if (left_contour_length(internal_tree) != right_contour_length(internal_tree)) {
            char *tree_str = tree_to_string(trees[i]);
            if (tree_str) {
                printf("%dth tree: %s\n", i, tree_str);
            }
            free(tree_str);
        }

        assert_int_equal(left_contour_length(internal_tree), right_contour_length(internal_tree));
        tree_free(to_external(internal_tree, 0, 0));
    }

    const int random_tree_tests = 1000;
    for (int i = 0; i < random_tree_tests; i++) {
        const int min_height = 2, max_height = 5; float chance_to_continue = 0.3f;
        struct tree_t* tree = tree_random(min_height, max_height, chance_to_continue);
        struct tree_internal_t* internal_tree = to_internal(tree_copy(tree));
        compute_offsets(internal_tree);

        // print tree for debugging, TODO: print_tree function
        if (left_contour_length(internal_tree) != right_contour_length(internal_tree)) {
            char *tree_str = tree_to_string(tree);
            if (tree_str) {
                printf("%s\n", tree_str);
            }
            free(tree_str);
        }

        assert_int_equal(left_contour_length(internal_tree), right_contour_length(internal_tree));
        struct tree_t* external_tree = to_external(internal_tree, 0, 0);
        assert_ptr_equal((uintptr_t)internal_tree, (uintptr_t)external_tree);
        tree_free(tree);
        tree_free(external_tree);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tree_value_equal),
        cmocka_unit_test(test_tree_free_works),
        cmocka_unit_test(test_layout_leaves_data_unchanged),
        cmocka_unit_test(test_tree_height_measures_max_depth),
        cmocka_unit_test(test_tree_random_respects_max_and_min_heights),
        cmocka_unit_test(test_contour_has_length_equal_to_height_of_full_tree),
        cmocka_unit_test(test_compute_layout_of_random_tree_has_same_length_contour)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}