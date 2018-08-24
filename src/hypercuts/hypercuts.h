/**
Inheritance header
**/

#include <math.h>
#include <string.h>
#include "../classifier_rule/classifier_rule.h"
#include "../byte_stream/byte_stream.h"
#include "../hash_table/hash_table.h"

/*H**********************************************************************
* FILENAME :        hypercuts.h
*
* DESCRIPTION :
*        A hash table implementation that use integer hashing and dynamic
*        incremental resiszing. The colisions are handled with linked_list structures.
*
* PUBLIC FUNCTIONS :
*       struct hypercuts*       new_hypercuts( size_t )
*
* PUBLIC STRUCTURE :
*       struct hypercuts
*
*
* AUTHOR :    Pieroux Alexandre
*H*/

#define BINTH 10   // Maximum rules in node threshold (best effort)
#define SPFAC 1.4   //  Maximum number of children space factor

#define VALUE_RAD 10       // Radius around the value of the number of optimal cuts to test (tradeoff the optimisation level and time to build the tree)
#define OPT_THRESHOLD 0.1 // Define in percent the difference threashold before the optimisation stop (tradeoff the optimisation level and time to build of the tree)
#define NB_MAX_UNIQUE_DIMENSIONS 32 // Define the maximum number of unique dimensions (Beyond this limit, the algorithm cannot hold in memory O(2^32 * nb cuts * size child))

typedef unsigned char u_char; // Defining u_char type for convenient display

struct hypercuts_classifier
{
   struct hypercuts_node *root;
   struct classifier_field **fields_set;
   uint32_t nb_dimensions;
};

struct hypercuts_node
{
   struct hypercuts_dimension **dimensions;
   struct hypercuts_node **children;
   struct classifier_rule **rules;
};

struct hypercuts_dimension
{
   uint32_t id;
   uint32_t cuts;
   uint32_t min_dim;
   uint32_t max_dim;
};

struct hypercuts_classifier *new_hypercuts_classifier(struct classifier_rule ***rules, uint32_t *nb_rules, bool verbose);

bool hypercuts_search(struct hypercuts_classifier *classifier, u_char *header, size_t header_length, void** out);

void hypercuts_print(struct hypercuts_classifier *classifier);
