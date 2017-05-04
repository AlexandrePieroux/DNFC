#include "hypercuts.h"

/*             Hypercuts private functions         */

// Normalize a set of given rules
void normalize_rules(
    struct classifier_rule **rules,
    uint32_t nb_rules,
    struct hypercuts_dimension ***dimensions,
    uint32_t *nb_dimensions,
    struct classifier_field ***fields_set);

void insert_field_collection(
    struct classifier_field ***collection,
    struct classifier_field *field,
    uint32_t *inserted_element);

// Hypercuts heuristic that eliminate overlapping rules
void rules_overlap(
    struct classifier_rule ***rules,
    uint32_t *nb_rules,
    struct classifier_rule ***out_rules);

// Add a new dimensions to the collection of dimensions
void dimension_collection_add_field(
    struct hypercuts_dimension **dimensions,
    struct classifier_field *field,
    uint32_t *nb_dimensions);

// Build recursively the hypercuts trees
struct hypercuts_node *build_node(
    struct classifier_rule **rules,
    uint32_t nb_rules,
    struct hypercuts_node ***leaves,
    uint32_t *nb_leaves,
    struct hypercuts_dimension **dimensions,
    uint32_t nb_dimensions);

// Allocate a new leaf to be inserted in the tree
struct hypercuts_node *new_leaf(
    struct classifier_rule **rules,
    uint32_t nb_rules);

// Allocate a new node to be inserted in the tree
struct hypercuts_node *new_node(
    uint32_t nb_dimensions);

// Allocate a new dimension struture - It contain the information of the dimension in a node
struct hypercuts_dimension *new_dimension(
    uint32_t id,
    uint32_t cuts,
    uint32_t min,
    uint32_t max);

// Check if a leaf that contain the same set of rules already exist in the array of leaves
struct hypercuts_node *leaf_node_exist(
    struct hypercuts_node ***leaves,
    uint32_t nb_leaves,
    struct classifier_rule **rules);

// Add a leaf in the array of leaves if it does not already exist and return it.
struct hypercuts_node *add_leaf_node(
    struct hypercuts_node ***leaves,
    uint32_t *nb_leaves,
    struct classifier_rule **rules,
    uint32_t nb_rules);

// Agregate the h(value) and h(mask) to determine if the same rule already exist in 'table'
bool is_rule_unique(
    uint32_t value,
    uint32_t mask,
    struct hash_table *table);

// Determine which dimensions is going to be cut
uint32_t get_dimensions(
    struct hypercuts_node *node,
    struct hypercuts_dimension **dimensions,
    uint32_t nb_dimensions,
    struct classifier_rule **rules,
    uint32_t nb_rules);

// Set the number of cuts to be performed in the chosen dimensions
void set_nb_cuts(
    struct hypercuts_node *node,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t nb_dim_cut);

// Compute the optimal number of cut to ber performed in one dimensions (used in a greedy aproach)
uint32_t get_optimal_cut(
    struct hypercuts_node *node,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t dim_id);

// Determine (based on parameter OPT_THRESHOLD) if a given value has 'significantly' increased
bool has_significantly_increased(
    uint32_t prev,
    uint32_t current);

// Determine (based on parameter OPT_THRESHOLD) if a given value has 'significantly' decreased
bool has_significantly_decreased(
    uint32_t prev,
    uint32_t current);

// Determine the characteristics produced by performing a given number of cut in a given dimension
void get_cut_characteristics(
    struct hypercuts_dimension *dimension,
    uint32_t cut_count,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t *rule_mean,
    uint32_t *nb_max_rules,
    uint32_t *nb_empty_children);

// Compute the optimal number of cuts (based on the parameter VALUE_RAD) to perform in each dimensions based on the local optimum of each dimension
void get_optimal_cut_combination(
    uint32_t *cuts,
    uint32_t *cuts_min,
    uint32_t *cuts_max,
    struct hypercuts_dimension **dimensions,
    uint32_t nb_dim_cut,
    uint32_t *return_cuts,
    uint32_t sum_cuts,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t iteration,
    uint32_t *children_rules_sum,
    uint32_t *max_rules);

// Determine the characteristics produced by a given cut combination performed in multiple dimensions
void get_combination_cuts_characteristics(
    uint32_t *cuts,
    uint32_t nb_dim_cut,
    struct hypercuts_dimension **dimensions,
    uint32_t *return_cuts,
    uint32_t sum_cuts,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t *children_rules_sum,
    uint32_t *max_rules);

// Get the index of a given child in a multi-dimensional space represented in on a one dimension array
uint32_t get_multi_dimension_index(
    uint32_t *dimensions_indexes,
    uint32_t nb_dimensions,
    uint32_t *cuts_indexes);

// Return ceil(divident/divisor)
uint32_t integer_division_ceil(
    uint32_t dividend, 
    uint32_t divisor);

// Get the next index in a multi-dimensional space
bool get_next_dimension_index(
    uint32_t *dimension_indexes,
    uint32_t *dim_min,
    uint32_t *dim_max,
    uint32_t nb_dimensions);

// Get the next multi-dimensional index
void get_next_dimension(
    uint32_t *dimensions_index,
    uint32_t nb_dimensions,
    struct hypercuts_node *node);

// Retreive the leaf (node) that cover the region the packet is in, if it exist
struct hypercuts_node *get_leaf(
    struct hypercuts_node *node,
    uint32_t *header_values);

// Linear search amongst rules in a leaf node
void *linear_search(
    struct hypercuts_node *leaf,
    uint32_t *header_values,
    uint32_t nb_dimensions);

// Retrieve a node based on its indexes
struct hypercuts_node *get_children(
    uint32_t *dimensions_index,
    struct hypercuts_node *node,
    uint32_t nb_dimensions);

// Count the number of dimensions in which a node has been cut
uint32_t count_dimension(
    struct hypercuts_node *node);

// Cast a 'n' u_char values in a 32 non-signed bits integer
uint32_t get_uint32_value(
    u_char *header,
    uint32_t offset,
    uint32_t length);

// Hypercuts heuristic that shrink the hyperspace around the contained rules
void shrink_space_node(
    struct hypercuts_dimension **dimensions,
    uint32_t nb_dimensions,
    struct classifier_rule **rules_in_node,
    uint32_t nb_rules);

// Transform a given interval into a hash value
uint32_t hash_interval(
    uint32_t value,
    uint32_t mask);

// Return true if the rule have a field for the dimensions 'id'
bool get_field_id(struct classifier_rule *rule,
                  uint32_t id,
                  uint32_t *out);

// Print recursively a given hypercuts tree
void print_tree(
    struct hypercuts_node *tree,
    uint32_t nb_dim,
    char *string,
    size_t string_size,
    bool last);

/*             Hypercuts private functions         */

struct hypercuts_classifier *new_hypercuts_classifier(struct classifier_rule **rules, uint32_t nb_rules)
{
    if (!rules || (nb_rules == 0))
        return NULL;

    // Create and init the classifier
    hash_table_init();
    struct hypercuts_classifier *classifier = chkmalloc(sizeof(*classifier));
    struct hypercuts_node ***leaves = chkcalloc(1, sizeof(struct hypercuts_node ***));
    struct hypercuts_dimension **dimensions;
    struct classifier_rule **out_rules;
    struct classifier_field **fields_set;
    uint32_t nb_dimensions = 0;

    // Normalize rules: getting the total of unique dimensions.
    normalize_rules(rules, nb_rules, &dimensions, &nb_dimensions, &fields_set);

    // Second heuristic: eliminate overlapping rules (rules that will never be matched because of former bigger rules).
    rules_overlap(&rules, &nb_rules, &out_rules);

    // Third heuristic: we shrink the space covered by the node
    shrink_space_node(dimensions, nb_dimensions, out_rules, nb_rules);

    // Recursive node construction
    uint32_t *nb_leaves = chkmalloc(sizeof(*nb_leaves));
    *nb_leaves = 0;
    classifier->root = build_node(out_rules, nb_rules, leaves, nb_leaves, dimensions, nb_dimensions);
    classifier->fields_set = fields_set;
    classifier->nb_dimensions = nb_dimensions;
    for(uint32_t i = 0; i < nb_dimensions; ++i)
        free(dimensions[i]);
    free(dimensions);
    return classifier;
}

void *hypercuts_search(struct hypercuts_classifier *classifier, u_char *header, size_t header_length)
{
    uint32_t header_values[classifier->nb_dimensions];
    struct classifier_field **fields = classifier->fields_set;
    for (uint32_t i = 0; i < classifier->nb_dimensions; ++i)
    {
        if (header_length < (fields[i]->offset + fields[i]->bit_length - 1))
            header_values[i] = 0;
        else
            header_values[i] = get_uint32_value(header, fields[i]->offset, fields[i]->bit_length);
    }
    struct hypercuts_node *leaf = get_leaf(classifier->root, header_values);
    if (!leaf)
        return NULL;
    return linear_search(leaf, header_values, classifier->nb_dimensions);
}

void hypercuts_print(struct hypercuts_classifier *classifier)
{
    char start = '\0';
    print_tree(classifier->root, classifier->nb_dimensions, &start, 1, true);
}

/*             Hypercuts private functions         */

void normalize_rules(
    struct classifier_rule **rules,
    uint32_t nb_rules,
    struct hypercuts_dimension ***dimensions,
    uint32_t *nb_dimensions,
    struct classifier_field ***fields_set)
{
    struct classifier_field **field_collection = chkcalloc(NB_MAX_UNIQUE_DIMENSIONS, sizeof(*field_collection));
    uint32_t inserted_element = 1;
    for (uint32_t i = 0; i < nb_rules; ++i)
    {
        for (uint32_t j = 0; j < rules[i]->nb_fields; ++j)
        {
            // Check if rule is coherent
            uint32_t max = ((uint32_t)0x1 << rules[i]->fields[j]->bit_length) - 1;
            uint32_t field_max = rules[i]->fields[j]->value | rules[i]->fields[j]->mask;
            if (max < field_max)
            {
                fprintf(stderr, "Field %d of rule %d is not coherent (bit length max: %d < field max value:%d)\n", j, i, max, field_max);
                exit(-1);
            }
                
            insert_field_collection(&field_collection, rules[i]->fields[j], &inserted_element);
            if (inserted_element >= NB_MAX_UNIQUE_DIMENSIONS)
            {
                fprintf(stderr, "Limit of unique dimensions exceeded: %d (limit: %d)\n", inserted_element, NB_MAX_UNIQUE_DIMENSIONS);
                exit(-1);
            }
        }
    }

    inserted_element--;
    (*dimensions) = chkmalloc(sizeof(**dimensions) * inserted_element);
    (*fields_set) = chkmalloc(sizeof(**fields_set) * inserted_element);
    for (uint32_t k = 0; k < inserted_element; ++k)
    {
        (*fields_set)[k] = field_collection[k];
        uint32_t max_field = ((uint32_t)0x1 << field_collection[k]->bit_length) - 1;
        (*dimensions)[k] = new_dimension(k, 0, 0, max_field);
    }
    *nb_dimensions = inserted_element;
}

void insert_field_collection(
    struct classifier_field ***collection,
    struct classifier_field *field,
    uint32_t *inserted_element)
{
    for (uint32_t i = 0; i < *inserted_element; ++i)
    {
        if ((*collection)[i] != NULL &&
            (*collection)[i]->offset == field->offset &&
            (*collection)[i]->bit_length == field->bit_length)
        {
            field->id = i;
            return;
        }
    }
    (*collection)[(*inserted_element) - 1] = field;
    field->id = (*inserted_element) - 1;
    (*inserted_element)++;
}

void rules_overlap(
    struct classifier_rule ***rules,
    uint32_t *nb_rules,
    struct classifier_rule ***out_rules)
{
    // For each rules
    uint32_t count = *nb_rules;
    for (uint32_t i = 0; i < *nb_rules; ++i)
    {
        // We compare each rules
        struct classifier_rule *current_rule = (*rules)[i];
        if (!current_rule)
            continue;

        for (uint32_t j = i+1; j < *nb_rules; ++j)
        {
            struct classifier_rule *tested_rule = (*rules)[j];
            if (!tested_rule)
                continue;

            // For each fields in the rule
            bool overlap = true;
            bool common_fields = false;
            for (uint32_t k = 0; k < current_rule->nb_fields; ++k)
            {
                // We test each fields in the tested rule
                uint32_t current_rule_max = current_rule->fields[k]->mask | current_rule->fields[k]->value;
                for (uint32_t l = 0; l < tested_rule->nb_fields; ++l)
                {
                    // If it is not the same field id we go to the next field
                    if (current_rule->fields[k]->id != tested_rule->fields[l]->id)
                        continue;
                    common_fields = true;

                    // We check if at least one field/dimension does not shadow another
                    uint32_t tested_rule_max = tested_rule->fields[l]->mask | tested_rule->fields[l]->value;
                    if (tested_rule->fields[l]->value < current_rule->fields[k]->value ||
                        tested_rule_max > current_rule_max)
                    {
                        overlap = false;
                        break;
                    }
                }
                // If the rule does not shadow the other we break to the other rule
                if (!overlap)
                    break;
            }

            // If a rule completely overlap another, we suppress the second one as it will never be used
            if (overlap && common_fields)
            {
                fprintf(stderr, "Warning: Rule %u shadow rule %u (Deleting rule %d from the set)\n", current_rule->id, tested_rule->id, tested_rule->id);
                (*rules)[j] = NULL;
                count--;
                break;
            }
        }
    }

    // Setting the set of rules without overlaps
    (*out_rules) = chkmalloc(sizeof(*out_rules) * count);
    uint32_t index = 0;
    for (uint32_t i = 0; i < *nb_rules; ++i)
    {
        if ((*rules)[i])
        {
            (*out_rules)[index] = (*rules)[i];
            index++;
        }
    }
    *nb_rules = count;
}

void shrink_space_node(
   struct hypercuts_dimension **dimensions,
   uint32_t nb_dimensions,
   struct classifier_rule **rules_in_node,
   uint32_t nb_rules)
{
    if (!rules_in_node)
        return;
    
    uint32_t mins[nb_dimensions];
    uint32_t maxes[nb_dimensions];
    
    for(uint32_t i = 0; i < nb_dimensions; ++i)
    {
        mins[i] = (uint32_t)~0x0;
        maxes[i] = 0;
    }
    
    for (uint32_t i = 0; i < nb_rules; ++i)
    {
        for (uint32_t j = 0; j < nb_dimensions; ++j)
        {
            uint32_t id = dimensions[j]->id;
            if(!get_field_id(rules_in_node[i], dimensions[j]->id, &id))
            {
                // Get the min and the max of the normalized field
                mins[id] = dimensions[j]->min_dim;
                mins[id] = dimensions[j]->max_dim;
            } else {
                // Get the min
                uint32_t field_min = rules_in_node[i]->fields[id]->value;
                if (mins[id] > field_min)
                    mins[id] = field_min;
                
                // Get the max
                uint32_t field_max = rules_in_node[i]->fields[id]->value | rules_in_node[i]->fields[id]->mask;
                if (maxes[id] < field_max)
                    maxes[id] = field_max;
            }
        }
    }
}

struct hypercuts_node *build_node(
    struct classifier_rule **rules,
    uint32_t nb_rules,
    struct hypercuts_node ***leaves,
    uint32_t *nb_leaves,
    struct hypercuts_dimension **dimensions,
    uint32_t nb_dimensions)
{
    // We create a leaf if there are less rules than BINTH
    if (nb_rules <= BINTH)
        return add_leaf_node(leaves, nb_leaves, rules, nb_rules);

    // Otherwise we create a node
    struct hypercuts_node *node = new_node(nb_dimensions);

    // get the dimensions used to cut the node
    uint32_t nb_dim_cut = get_dimensions(node,
                                         dimensions,
                                         nb_dimensions,
                                         rules,
                                         nb_rules);
    if (nb_dim_cut == 0)
    {
        free(node->dimensions);
        free(node);
        return add_leaf_node(leaves, nb_leaves, rules, nb_rules);
    }

    // Set the number of cuts to do in each dimensions
    set_nb_cuts(node,
                rules,
                nb_rules,
                nb_dim_cut);
    
    // Memory optimization
    uint32_t nb_dim_after_cuts = 0;
    struct hypercuts_dimension* dimensions_after_cuts[nb_dim_cut];
    for(uint32_t i = 0; i < nb_dim_cut; i++)
    {
        if(node->dimensions[i]->cuts > 0)
        {
            dimensions_after_cuts[nb_dim_after_cuts] = node->dimensions[i];
            nb_dim_after_cuts++;
        }
    }
    for (uint32_t i = 0; i < nb_dim_after_cuts; i++)
        node->dimensions[i] = dimensions_after_cuts[i];
    node->dimensions = chkrealloc(node->dimensions, sizeof(node->dimensions) * nb_dim_after_cuts + 1);
    node->dimensions[nb_dim_after_cuts] = NULL;
    nb_dim_cut = nb_dim_after_cuts;

    // Compute the number of children after the cuts
    uint32_t nb_children = 0;
    for (uint32_t i = 0; i < nb_dim_cut; ++i)
        nb_children += node->dimensions[i]->cuts;
    nb_children = (uint32_t)0x1 << nb_children;
    if (nb_children == 1)
    {
        free(node->dimensions);
        free(node);
        return add_leaf_node(leaves, nb_leaves, rules, nb_rules);
    }
    
    // Allocate space for the children
    node->children = chkcalloc(nb_children, sizeof node->children);

    // Compute the size of all cuts and keep track of dimensions indexes
    uint32_t dimension_region_size[nb_dim_cut];
    uint32_t dimensions_indexes[nb_dim_cut];
    for (uint32_t i = 0; i < nb_dim_cut; ++i)
    {
        uint32_t nb_subregions = (uint32_t)0x1 << node->dimensions[i]->cuts;
        dimension_region_size[i] = node->dimensions[i]->max_dim - node->dimensions[i]->min_dim;
        if(dimension_region_size[i] < (uint32_t)~0x0)
            dimension_region_size[i]++;
        dimension_region_size[i] = integer_division_ceil(dimension_region_size[i], nb_subregions);
        dimensions_indexes[i] = 0;
    }

    // Build each children
    struct classifier_rule *subset_rules[nb_rules];
    for (uint32_t i = 0; i < nb_children; ++i)
    {
        // Copy the dimension array
        struct hypercuts_dimension *dimensions_child[nb_dimensions];
        for (uint32_t j = 0; j < nb_dimensions; ++j)
        {
            dimensions_child[j] = chkmalloc(sizeof(dimensions_child[j]));
            dimensions_child[j]->id = dimensions[j]->id;
            dimensions_child[j]->cuts = dimensions[j]->cuts;
            dimensions_child[j]->min_dim = dimensions[j]->min_dim;
            dimensions_child[j]->max_dim = dimensions[j]->max_dim;
        }

        // Get the subregions
        uint32_t subset_rules_count = 0;
        for (uint32_t j = 0; j < nb_dim_cut; ++j)
        {
            uint32_t index = node->dimensions[j]->id;
            dimensions_child[index]->min_dim = node->dimensions[j]->min_dim + (dimension_region_size[j] * dimensions_indexes[j]);
            dimensions_child[index]->max_dim = dimensions_child[index]->min_dim + dimension_region_size[j] - 1;
        }

        // Check the rules to be in the child
        for (uint32_t j = 0; j < nb_rules; ++j)
        {
            bool inside_child = true;
            for (uint32_t k = 0; k < nb_dim_cut; ++k)
            {
                uint32_t index;
                uint32_t min_rule_dim;
                uint32_t max_rule_dim;
                if (!get_field_id(rules[j], node->dimensions[k]->id, &index))
                {
                    min_rule_dim = node->dimensions[k]->min_dim;
                    max_rule_dim = node->dimensions[k]->max_dim;
                }
                else
                {
                    min_rule_dim = rules[j]->fields[index]->value;
                    max_rule_dim = rules[j]->fields[index]->value | rules[j]->fields[index]->mask;
                }
                uint32_t child_min = dimensions_child[node->dimensions[k]->id]->min_dim;
                uint32_t child_max = dimensions_child[node->dimensions[k]->id]->max_dim;

                if (max_rule_dim < child_min || min_rule_dim > child_max)
                {
                    inside_child = false;
                    break;
                }
            }
            
            // If the rule fit in the child, we add it to the subset of rules
            if (inside_child)
            {
                subset_rules[subset_rules_count] = rules[j];
                subset_rules_count++;
            }
        }
        // Recursion on the new child
        node->children[i] = build_node(subset_rules,
                                       subset_rules_count, 
                                       leaves, 
                                       nb_leaves, 
                                       dimensions_child, 
                                       nb_dimensions);
        get_next_dimension(dimensions_indexes, nb_dim_cut, node);
        for (uint32_t j = 0; j < nb_dimensions; ++j)
            free(dimensions_child[j]);
    }
    return node;
}

struct hypercuts_node *new_leaf(
    struct classifier_rule **rules,
    uint32_t nb_rules)
{
    struct hypercuts_node *leaf = chkmalloc(sizeof(*leaf));
    leaf->dimensions = NULL;
    leaf->children = NULL;
    leaf->rules = chkmalloc(sizeof(struct classifier_rule **) * (nb_rules + 1));
    for (uint32_t i = 0; i < nb_rules; ++i)
        leaf->rules[i] = rules[i];
    leaf->rules[nb_rules] = NULL;
    return leaf;
}

struct hypercuts_node *new_node(
    uint32_t nb_dimensions)
{
    struct hypercuts_node *node = chkmalloc(sizeof(*node));
    node->dimensions = chkcalloc(nb_dimensions, sizeof node->dimensions);
    node->rules = NULL;
    return node;
}

struct hypercuts_dimension *new_dimension(
    uint32_t id,
    uint32_t cuts,
    uint32_t min,
    uint32_t max)
{
    struct hypercuts_dimension *dimension = chkmalloc(sizeof *dimension);
    dimension->id = id;
    dimension->cuts = cuts;
    dimension->min_dim = min;
    dimension->max_dim = max;
    return dimension;
}

struct hypercuts_node *leaf_node_exist(
    struct hypercuts_node ***leaves,
    uint32_t nb_leaves,
    struct classifier_rule **rules)
{
    if (!(*leaves))
        return NULL;

    struct hypercuts_node **item = *leaves;
    struct hypercuts_node *result = NULL;

    for (uint32_t i = 0; i < nb_leaves; ++i)
    {
        result = item[i];
        struct classifier_rule **leaves_rules = item[i]->rules;
        uint32_t rule_index = 0;

        while (rules[rule_index] && leaves_rules[rule_index])
        {
            if (rules[rule_index] != leaves_rules[rule_index])
            {
                result = NULL;
                break;
            }

            rule_index++;
        }

        if (result == item[i])
            return result;
    }
    return result;
}

struct hypercuts_node *add_leaf_node(
    struct hypercuts_node ***leaves,
    uint32_t *nb_leaves,
    struct classifier_rule **rules,
    uint32_t nb_rules)
{
    // First heurisitc "node merging"
    struct hypercuts_node *result = leaf_node_exist(leaves, *nb_leaves, rules);
    if (result)
        return result;

    result = new_leaf(rules, nb_rules);
    *leaves = chkrealloc((*leaves), sizeof(struct hypercuts_node **) * ((*nb_leaves) + 1));
    (*leaves)[*nb_leaves] = result;
    (*nb_leaves)++;
    return result;
}

bool is_rule_unique(
    uint32_t value,
    uint32_t mask,
    struct hash_table *table)
{
    if (hash_table_put(table, hash_interval(value, mask), NULL))
        return true;
    return false;
}

uint32_t hash_interval(
    uint32_t value,
    uint32_t mask)
{
    uint32_t seed = 0;
    seed = seed ^ (FNV1_hash(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    seed = seed ^ (FNV1_hash(mask) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    return seed;
}

uint32_t get_dimensions(
    struct hypercuts_node *node,
    struct hypercuts_dimension **dimensions,
    uint32_t nb_dimensions,
    struct classifier_rule **rules,
    uint32_t nb_rules)
{
    // Table to count the number of unique element in each dimension
    struct hash_table **tables = chkmalloc(sizeof(**tables) * nb_dimensions);
    uint32_t counters[nb_dimensions];
    for (uint32_t i = 0; i < nb_dimensions; ++i)
    {
        tables[i] = new_hash_table(FNV_1);
        counters[i] = 0;
    }

    float mean = 0;
    for (uint32_t i = 0; i < nb_rules; ++i)
    {
        for (uint32_t j = 0; j < rules[i]->nb_fields; ++j)
        {
            // Count the number of unique item in dimension j
            if (is_rule_unique(rules[i]->fields[j]->value, rules[i]->fields[j]->mask, tables[rules[i]->fields[j]->id]))
            {
                counters[rules[i]->fields[j]->id]++;
                mean += (1 / (float)nb_dimensions);
            }
        }
    }

    // Ceil the mean
    mean += (float)0.5;

    // Setting the dimensions to be cut (NULL is left at the end of the array)
    uint32_t count = 0;
    for (uint32_t i = 0; i < nb_dimensions; ++i)
    {
        if (counters[i] >= (uint32_t)mean)
        {
            node->dimensions[count] = new_dimension(dimensions[i]->id, 0, dimensions[i]->min_dim, dimensions[i]->max_dim);
            count++;
        }
        hash_table_free(&tables[i]);
    }
    return count;
}

void set_nb_cuts(
    struct hypercuts_node *node,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t nb_dim_cut)
{
    // Get the local optimal number of cut for each dimension individually
    uint32_t *cuts = chkmalloc(sizeof(*cuts) * nb_dim_cut);
    uint32_t *cuts_min = chkmalloc(sizeof(*cuts_min) * nb_dim_cut);
    uint32_t *cuts_max = chkmalloc(sizeof(*cuts_max) * nb_dim_cut);
    uint32_t sum_cuts = 0;
    for (uint32_t i = 0; i < nb_dim_cut; ++i)
    {
        uint32_t optimal_cut = get_optimal_cut(node, rules, nb_rules, i);
        cuts[i] = (optimal_cut < VALUE_RAD) ? 0 : (optimal_cut - VALUE_RAD);
        cuts_min[i] = cuts[i];
        cuts_max[i] = optimal_cut + VALUE_RAD;
        sum_cuts += cuts[i];
    }

    // Find the optimum number of cuts around the optimal individual cuts
    uint32_t *optimal_cuts = chkmalloc(sizeof(*optimal_cuts) * nb_dim_cut);
    uint32_t children_rules_sum = (uint32_t)~0;
    uint32_t max_rules = nb_rules;
    get_optimal_cut_combination(cuts,
                                cuts_min,
                                cuts_max,
                                node->dimensions,
                                nb_dim_cut,
                                optimal_cuts,
                                sum_cuts,
                                rules,
                                nb_rules,
                                0,
                                &children_rules_sum,
                                &max_rules);

    // Set the cuts in the node
    for (uint32_t i = 0; i < nb_dim_cut; ++i)
        node->dimensions[i]->cuts = optimal_cuts[i];

    free(cuts);
    free(cuts_min);
    free(cuts_max);
    free(optimal_cuts);
}

uint32_t get_optimal_cut(
    struct hypercuts_node *node,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t dim_id)
{
    struct hypercuts_dimension **dimensions = node->dimensions;
    uint32_t optimal_cut = 0;

    uint32_t rule_mean = nb_rules;
    uint32_t prev_rule_mean = (uint32_t)~0x0;

    uint32_t nb_max_rules = nb_rules;
    uint32_t prev_nb_max_rules = (uint32_t)~0x0;

    uint32_t nb_empty_children = 0;
    uint32_t prev_nb_empty_children = 0;

    uint32_t cut_count = 1;

    // Find the optimal number of cuts for that dimension
    while (!has_significantly_increased(prev_nb_empty_children, nb_empty_children) &&
           (has_significantly_decreased(prev_rule_mean, rule_mean) ||
            has_significantly_decreased(prev_nb_max_rules, nb_max_rules)))
    {
        // Check if the cut was better
        if (rule_mean < prev_rule_mean || nb_max_rules < prev_nb_max_rules)
            optimal_cut = cut_count;

        // Rotate the variables
        prev_rule_mean = rule_mean;
        prev_nb_max_rules = nb_max_rules;
        prev_nb_empty_children = nb_empty_children;

        // Get the mean number of rules, the maximum number of rules in the children and the number of empty children with this cut
        get_cut_characteristics(dimensions[dim_id],
                                cut_count,
                                rules,
                                nb_rules,
                                &rule_mean,
                                &nb_max_rules,
                                &nb_empty_children);

        cut_count++;

        // If the number of cuts exceed the size of the region
        if (((uint32_t)0x1 << cut_count) > (dimensions[dim_id]->max_dim - dimensions[dim_id]->min_dim + 1))
            return optimal_cut;
    }
    return optimal_cut;
}

bool has_significantly_increased(
    uint32_t prev,
    uint32_t current)
{
    uint32_t range = (uint32_t)(prev * OPT_THRESHOLD);
    if (current > (prev + range))
        return true;
    return false;
}

bool has_significantly_decreased(
    uint32_t prev,
    uint32_t current)
{
    uint32_t range = (uint32_t)(prev * OPT_THRESHOLD);
    if (current < (prev - range))
        return true;
    return false;
}

void get_cut_characteristics(
    struct hypercuts_dimension *dimension,
    uint32_t cut_count,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t *rule_mean,
    uint32_t *nb_max_rules,
    uint32_t *nb_empty_children)
{

    // we cut
    uint32_t nb_children = (uint32_t)1 << cut_count;
    uint32_t subregion_size = (dimension->max_dim - dimension->min_dim) + 1;
    subregion_size = subregion_size / nb_children;

    // For each potential children
    float child_rule_mean = 0;
    uint32_t child_rule_count = 0;
    *nb_empty_children = 0;
    *nb_max_rules = 0;

    for (uint32_t j = 0; j < nb_children; ++j)
    {
        // The size of the children and the size of the dimensions is always a power of two, 
        // so no need to check if the max is outside the dimension
        uint32_t child_subregion_min = dimension->min_dim + (subregion_size * j);
        uint32_t child_subregion_max = child_subregion_min + (subregion_size - 1);

        // For each rules in the parent node
        for (uint32_t k = 0; k < nb_rules; ++k)
        {
            uint32_t index;
            uint32_t rule_statement_start;
            uint32_t rule_statement_end;
            if (!get_field_id(rules[k], dimension->id, &index))
            {
                rule_statement_start = 0;
                rule_statement_end = (uint32_t)~0x0;
            }
            else
            {
                rule_statement_start = rules[k]->fields[index]->value;
                rule_statement_end = rules[k]->fields[index]->value | rules[k]->fields[index]->mask;
            }
            if (rule_statement_start >= child_subregion_min || rule_statement_end <= child_subregion_max)
            {
                child_rule_mean += (1 / (float)nb_children);
                child_rule_count++;
            }
        }

        // No rules go in the child
        if (child_rule_count == 0)
        {
            (*nb_empty_children)++;
            continue;
        }

        // Check if this is the child that have the maximum number of rules
        if (*nb_max_rules < child_rule_count)
            *nb_max_rules = child_rule_count;
    }

    *rule_mean = (uint32_t)child_rule_mean;
}

void get_optimal_cut_combination(
    uint32_t *cuts,
    uint32_t *cuts_min,
    uint32_t *cuts_max,
    struct hypercuts_dimension **dimensions,
    uint32_t nb_dim_cut,
    uint32_t *return_cuts,
    uint32_t sum_cuts,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t iteration,
    uint32_t *children_rules_sum,
    uint32_t *max_rules)
{

    // If we are at the end of the recursion
    uint32_t max_nb_children = (uint32_t)((SPFAC * sqrt(nb_rules)) + 0.5);
    if (iteration == nb_dim_cut || ((uint32_t)0x1 << sum_cuts) >= max_nb_children)
    {
        // Get the cut combination results
        get_combination_cuts_characteristics(cuts,
                                             nb_dim_cut,
                                             dimensions,
                                             return_cuts,
                                             sum_cuts,
                                             rules,
                                             nb_rules,
                                             children_rules_sum,
                                             max_rules);
        return;
    }

    // Recursion for each dimensions
    for (; cuts[iteration] <= cuts_max[iteration]; cuts[iteration]++)
    {
        get_optimal_cut_combination(cuts,
                                    cuts_min,
                                    cuts_max,
                                    dimensions,
                                    nb_dim_cut,
                                    return_cuts,
                                    sum_cuts,
                                    rules,
                                    nb_rules,
                                    iteration + 1,
                                    children_rules_sum,
                                    max_rules);
        sum_cuts++;
        if (((uint32_t)0x1 << sum_cuts) > max_nb_children)
            break;
    }
    cuts[iteration] = cuts_min[iteration];
}

void get_combination_cuts_characteristics(
    uint32_t *cuts,
    uint32_t nb_dim_cut,
    struct hypercuts_dimension **dimensions,
    uint32_t *return_cuts,
    uint32_t sum_cuts,
    struct classifier_rule **rules,
    uint32_t nb_rules,
    uint32_t *children_rules_sum,
    uint32_t *max_rules)
{
    // Check if we exceed the limit
    if (nb_dim_cut > 31)
    {
        fprintf(stderr, "[ERROR] The number of dimensions to cut exceed the limit (limit:31 dimensions to cut:%d).", nb_dim_cut);
        exit(-1);
    }
    // Array of children
    uint32_t nb_children = (uint32_t)0x1 << sum_cuts;
    uint32_t min_index[nb_dim_cut];
    uint32_t max_index[nb_dim_cut];
    uint32_t current_index[nb_dim_cut];
    uint32_t children_array[nb_children];
    for (uint32_t i = 0; i < nb_children; ++i)
        children_array[i] = 0;

    // For each rules we compute the number of rule each child get.
    uint32_t min_value;
    uint32_t max_value;
    uint32_t nb_subregions;
    uint32_t subregion_size;
    uint32_t index;
    for (uint32_t i = 0; i < nb_rules; ++i)
    {
        for (uint32_t j = 0; j < nb_dim_cut; ++j)
        {
            uint32_t field_index;
            // If the rule does not have the dimension we replace it by a wildcard
            if (!get_field_id(rules[i], dimensions[j]->id, &field_index))
            {
                min_value = dimensions[j]->min_dim;
                max_value = dimensions[j]->max_dim;
            }
            else
            {
                min_value = rules[i]->fields[field_index]->value;
                max_value = rules[i]->fields[field_index]->value | rules[i]->fields[field_index]->mask;
            }
            
            // Check if the rule land in the child
            if(min_value > dimensions[j]->max_dim || max_value < dimensions[j]->min_dim)
                continue;
            
            nb_subregions = (uint32_t)0x1 << cuts[j];
            subregion_size = dimensions[j]->max_dim - dimensions[j]->min_dim;
            if(subregion_size < (uint32_t)~0x0)
                subregion_size++;
            subregion_size = integer_division_ceil(subregion_size, nb_subregions);

            // Minimum subregion size
            if (subregion_size == 0)
                subregion_size = 1;

            // Fit the interval in the region of the dimension
            if (min_value < dimensions[j]->min_dim)
                min_value = dimensions[j]->min_dim;

            if (max_value > dimensions[j]->max_dim)
                max_value = dimensions[j]->max_dim;

            // Compute the minimal and maximal index of the rule in this dimension
            min_index[j] = (min_value - dimensions[j]->min_dim) / subregion_size;

            uint32_t max_interval = max_value - dimensions[j]->min_dim;
            if (max_interval > 0)
                max_interval--;
            max_index[j] = max_interval / subregion_size;
            if(max_index[j] > 0)
                max_index[j]--;
            current_index[j] = min_index[j];
        }

        // Locate the first child
        index = get_multi_dimension_index(min_index, nb_dim_cut, cuts);
        children_array[index]++;

        // Locate all the other children that the rule span
        uint32_t counter = 0;
        while (get_next_dimension_index(current_index, min_index, max_index, nb_dim_cut))
        {
            index = get_multi_dimension_index(current_index, nb_dim_cut, cuts);
            children_array[index]++;
            counter++;
        }
    }

    // Set the return variables
    uint32_t rules_sum = 0;
    uint32_t max_rule_child = 0;

    // Compute maximum number of children a node can get and the sum of all the childrens
    for (uint32_t i = 0; i < nb_children; ++i)
    {
        rules_sum += children_array[i];
        if (max_rule_child < children_array[i])
            max_rule_child = children_array[i];
    }

    if (max_rule_child < *max_rules || ((max_rule_child == *max_rules) && (rules_sum < *children_rules_sum)))
    {
        *max_rules = max_rule_child;
        *children_rules_sum = rules_sum;
        for (uint32_t i = 0; i < nb_dim_cut; ++i)
            return_cuts[i] = cuts[i];
    }
}

uint32_t integer_division_ceil(
    uint32_t dividend, 
    uint32_t divisor)
{
    return (dividend + (divisor - 1)) / divisor;
}

bool get_next_dimension_index(
    uint32_t *dimension_indexes,
    uint32_t *dim_min,
    uint32_t *dim_max,
    uint32_t nb_dimensions)
{
    for (uint32_t i = 0; i < nb_dimensions; ++i)
    {
        if ((dimension_indexes[i] + 1) > dim_max[i])
            dimension_indexes[i] = dim_min[i];
        else
        {
            dimension_indexes[i]++;
            return true;
        }
    }
    return false;
}

uint32_t get_multi_dimension_index(
    uint32_t *dimensions_indexes,
    uint32_t nb_dimensions,
    uint32_t *cuts_indexes)
{
    uint32_t dim_size = 1;
    uint32_t index = 0;
    for (uint32_t i = 0; i < nb_dimensions; ++i)
    {
        index += (dimensions_indexes[i] * dim_size);
        dim_size *= (uint32_t)0x1 << cuts_indexes[i];
    }
    return index;
}

void get_next_dimension(
    uint32_t *dimensions_index,
    uint32_t nb_dimensions,
    struct hypercuts_node *node)
{
    for (uint32_t i = 0; i < nb_dimensions; ++i)
    {
        if (dimensions_index[i] + 1 >= ((uint32_t)0x1 << node->dimensions[i]->cuts))
        {
            dimensions_index[i] = 0;
        }
        else
        {
            dimensions_index[i]++;
            return;
        }
    }
}

struct hypercuts_node *get_leaf(
    struct hypercuts_node *node,
    uint32_t *header_values)
{
    while (!node->rules)
    {
        uint32_t nb_dimensions_node = count_dimension(node);
        uint32_t dimensions_index[nb_dimensions_node];
        for (uint32_t i = 0; i < nb_dimensions_node; ++i)
        {
            // If the packet is not in the shrinked space
            uint32_t index = node->dimensions[i]->id;
            if (header_values[index] < node->dimensions[i]->min_dim ||
                header_values[index] > node->dimensions[i]->max_dim)
                return NULL;

            // Else we compute the index of the child
            uint32_t nb_cuts = (uint32_t)0x1 << node->dimensions[i]->cuts;
            
            uint32_t subregion_size = node->dimensions[i]->max_dim - node->dimensions[i]->min_dim;
            if(subregion_size < (uint32_t)~0x0)
                subregion_size++;
            subregion_size = integer_division_ceil(subregion_size, nb_cuts);
            dimensions_index[i] = (header_values[index] - node->dimensions[i]->min_dim) / subregion_size;
        }
        node = get_children(dimensions_index, node, nb_dimensions_node);
    }
    return node;
}

void *linear_search(
    struct hypercuts_node *leaf,
    uint32_t *header_values,
    uint32_t nb_dimensions)
{
    if (!leaf->rules)
        return NULL;

    uint32_t i = 0;
    struct classifier_rule *current_rule = leaf->rules[0];
    while (current_rule)
    {
        bool match = true;
        for (uint32_t i = 0; i < nb_dimensions; ++i)
        {
            // Get the value, mask it then compare it to the value: if it does not match then break
            uint32_t header_32 = header_values[i] & ~(current_rule->fields[i]->mask);
            if (header_32 != current_rule->fields[i]->value)
            {
                match = false;
                break;
            }
        }

        // If it matched return action pointer
        if (match)
            return current_rule->action;

        // Next rule
        i++;
        current_rule = leaf->rules[i];
    }
    return NULL;
}

struct hypercuts_node *get_children(
    uint32_t *dimensions_index,
    struct hypercuts_node *node,
    uint32_t nb_dimensions)
{
    uint32_t nb_cuts[nb_dimensions];
    for (uint32_t i = 0; i < nb_dimensions; ++i)
        nb_cuts[i] = node->dimensions[i]->cuts;

    uint32_t index = get_multi_dimension_index(dimensions_index, nb_dimensions, nb_cuts);
    return node->children[index];
}

uint32_t count_dimension(
    struct hypercuts_node *node)
{
    uint32_t counter = 0;
    while (node->dimensions[counter])
        counter++;
    return counter;
}

uint32_t get_uint32_value(
    u_char *header,
    uint32_t offset,
    uint32_t length)
{
    uint32_t char_index_start = offset / 8;
    uint32_t char_index_end = (offset + length - 1) / 8;

    // Getting the first part of the value and cleaning leading bits
    uint32_t result = header[char_index_start];
    uint32_t cleaning_mask = (uint32_t)0x1 << (8 - (offset % 8));
    cleaning_mask--;
    result = result & cleaning_mask;
    char_index_start++;

    // Getting the rest of the value
    for (; char_index_start <= char_index_end; ++char_index_start)
    {
        result = result << 8;
        result = result | header[char_index_start];
    }

    // Cleaning the end
    uint32_t rest = (offset + length) % 8;
    if (rest != 0)
        result = result >> (8 - rest);
    return result;
}

bool get_field_id(struct classifier_rule *rule, uint32_t id, uint32_t *out)
{
    for (uint32_t i = 0; i < rule->nb_fields; ++i)
    {
        if (rule->fields[i]->id == id)
        {
            *out = i;
            return true;
        }
        if (rule->fields[i]->id > id)
            return false;
    }
    return false;
}

void print_tree(
    struct hypercuts_node *tree,
    uint32_t nb_dim,
    char *string,
    size_t string_size,
    bool last)
{
    if (!tree)
        return;

    char string_cpy[string_size];
    strncpy(string_cpy, string, string_size);

    fprintf(stderr, "%s", string_cpy);
    string_size += 3;
    if (last)
    {
        fprintf(stderr, "└─ ");
        strncat(string_cpy, "  ", string_size);
    }
    else
    {
        fprintf(stderr, "├─ ");
        strncat(string_cpy, "| ", string_size);
    }

    // If it is a leaf we print the list of rules in it on one line
    if (tree->rules)
    {
        struct classifier_rule **rules = tree->rules;
        while (rules && *rules)
        {
            fprintf(stderr, "%d, ", (*rules)->id);
            rules++;
        }
        fprintf(stderr, "\n");
        return;
    }

    // Otherwise it is a node and we print the dimensions that was cut
    struct hypercuts_dimension **dimensions = tree->dimensions;
    fprintf(stderr, "node: ");
    uint32_t nb_children = 0;
    for (uint32_t i = 0; dimensions[i] && i < nb_dim; ++i)
    {
        fprintf(stderr, "[d:%d c:%d]   ", dimensions[i]->id, dimensions[i]->cuts);
        nb_children += dimensions[i]->cuts;
    }
    nb_children = (uint32_t)0x1 << nb_children;
    fprintf(stderr, "\n");

    // Print the children
    struct hypercuts_node **children = tree->children;
    for (uint32_t i = 0; i < nb_children; ++i)
        print_tree(children[i], nb_dim, string_cpy, string_size, i == (nb_children - 1));
}

/*             Hypercuts private functions         */
