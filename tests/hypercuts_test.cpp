#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "../src/classifiers/static_classifiers/hypercuts/hypercuts.h"
}

#include "gtest/gtest.h"
#define NB_RULES 50
#define NB_DIMENSIONS 5
#define HEADER_LENGTH 64

uint32_t *get_random_numbers(uint32_t size);
classifier_rule **get_random_rules(uint32_t size, uint32_t dimension_limit);
classifier_field **get_random_dimensions(uint32_t size);
u_char *get_header(classifier_rule rule);
uint32_t rand_interval(uint32_t min, uint32_t max);

TEST(Hypecuts, RandomRules)
{
  classifier_rule **rules = get_random_rules(NB_RULES, NB_DIMENSIONS);
  hypercuts_classifier *classifier = new_hypercuts_classifier(rules, NB_RULES);
  hypercuts_print(classifier);

  for (uint32_t i = 0; i < NB_RULES; ++i)
    EXPECT_EQ(*(uint32_t *)hypercuts_search(classifier, get_header(*rules[i]), HEADER_LENGTH), i);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

uint32_t *get_random_numbers(uint32_t size)
{
  /**
  Could be generalised to random over bigger range than 0 - 'size'
  **/
  uint32_t *result = new uint32_t[size];
  bool is_used[size];
  for (size_t i = 0; i < size; i++)
  {
    is_used[i] = false;
  }
  uint32_t im = 0;
  for (uint32_t in = 0; in < size && im < size; ++in)
  {
    uint32_t r = rand() % (in + 1); /* generate a random number 'r' */
    if (is_used[r])                 /* we already have 'r' */
      r = in;                       /* use 'in' instead of the generated number */

    result[im++] = r; /* +1 since your range begins from 1 */
    is_used[r] = true;
  }
  return result;
}

classifier_rule **get_random_rules(uint32_t size, uint32_t dimension_limit)
{
  classifier_rule **result = new classifier_rule *[size];
  classifier_field **dimensions = get_random_dimensions(dimension_limit);

  for (uint32_t i = 0; i < size; ++i)
  {
    result[i] = new classifier_rule;
    result[i]->id = i;
    uint32_t *action = new uint32_t;
    *action = i;

    uint32_t random_nb_dimensions = (rand() % dimension_limit) + 1;
    result[i]->fields = new classifier_field *[random_nb_dimensions];
    result[i]->nb_fields = random_nb_dimensions;
    result[i]->action = action;

    for (uint32_t j = 0; j < random_nb_dimensions; ++j)
    {
      // Get the random dimension generated
      result[i]->fields[j] = new classifier_field;
      result[i]->fields[j]->bit_length = dimensions[j]->bit_length;
      result[i]->fields[j]->offset = dimensions[j]->offset;

      // Generate random values for the dimension
      uint32_t max = (uint32_t)(0x1 << (result[i]->fields[j]->bit_length + 1)) - 1;
      uint32_t first = rand() % max;
      uint32_t second = rand() % max;
      result[i]->fields[j]->value = (first < second) ? first : second;
      result[i]->fields[j]->mask = (first < second) ? second : first;
    }
  }

  for (uint32_t i = 0; i < dimension_limit; ++i)
    free(dimensions[i]);
  free(dimensions);
  return result;
}

classifier_field **get_random_dimensions(uint32_t size)
{
  /**
  This could use 'get_random_numbers' to get unique dimensions
  **/
  classifier_field **result = new classifier_field *[size];

  uint32_t prev_offset = 0;
  uint32_t bit_length_limit = ((HEADER_LENGTH - 1) / size) + 1;
  for (uint32_t i = 0; i < size; ++i)
  {
    result[i] = new classifier_field;
    result[i]->bit_length = rand() % (bit_length_limit + 1);
    result[i]->offset = prev_offset + (rand() % (bit_length_limit - result[i]->bit_length + 1));
    prev_offset = result[i]->bit_length + result[i]->offset;
  }
  return result;
}

u_char *get_header(classifier_rule rule)
{
  int size = ((HEADER_LENGTH - 1) / 8) + 1;
  u_char *result = new u_char[size];
  for (size_t i = 0; i < rule.nb_fields; i++)
  {
    uint32_t value = rand_interval(rule.fields[i]->value, rule.fields[i]->mask);
    uint32_t index = rule.fields[i]->offset / 8;
    uint32_t shift = rule.fields[i]->offset % 8;

    value = value << (32 - rule.fields[i]->bit_length - shift - 1);
    result[index + 3] = value & 0xff;
    result[index + 2] = (value >> 8) & 0xff;
    result[index + 1] = (value >> 16) & 0xff;
    result[index] = (value >> 24) & 0xff;
  }
  return result;
}

uint32_t rand_interval(uint32_t min, uint32_t max)
{
  int r;
  const uint32_t range = 1 + max - min;
  const uint32_t buckets = RAND_MAX / range;
  const uint32_t limit = buckets * range;

  /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until it land in one of them. All buckets are equally
     * likely. If it land off the end of the line of buckets, try again. */
  do
  {
    r = rand();
  } while (r >= limit);

  return min + (r / buckets);
}
