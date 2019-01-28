#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <gtest/gtest.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "../linkedlistlf.hpp"

#define RANGE_NUMBERS 100000
#define NB_STEPS_NUMBERS 4

#define RANGE_THREADS 8
#define NB_STEPS_THREADS 2

using namespace LLLF;

int nb_numbers;
int nb_threads;
boost::asio::thread_pool pool(RANGE_THREADS);
LinkedListLf<int, int> *table;

struct arguments_t
{
  std::vector<int> *numbers;
  int start_index;
  bool active_comparison;
};

std::vector<int> *get_random_numbers(int size)
{
  std::vector<int> *result = new std::vector<int>(size);
  bool is_used[size];
  int im = 0;
  for (int in = 0; in < size && im < size; ++in)
  {
    int r = rand() % (in + 1);
    if (is_used[r])
      r = in;

    (*result)[im] = r;
    is_used[r] = 1;
    im++;
  }
  return result;
}

arguments_t **init(const bool &active_comparison)
{
  // Preparing the structure
  srand(time(NULL));
  std::vector<int> *numbers = get_random_numbers(nb_numbers);
  arguments_t **args = new arguments_t *[nb_threads];
  table = new LinkedListLf<int, int>;

  // We distribute the work per threads
  int divider = nb_numbers / nb_threads;
  int remain = nb_numbers % nb_threads;

  for (int p = 0; p < nb_threads; ++p)
  {
    int offset = p * divider;
    int size = (p + 1) * divider;
    args[p] = new arguments_t;
    if (p == (nb_threads - 1))
      size += remain;

    args[p]->numbers = new std::vector<int>(numbers->begin() + offset, numbers->begin() + size);
    args[p]->start_index = offset;
    args[p]->active_comparison = active_comparison;
  }
  return args;
}

template <class F>
void test_iterations(F &&fun, bool const &comparison)
{
  int steps_thread = RANGE_THREADS / NB_STEPS_THREADS;
  int steps_number = RANGE_NUMBERS / NB_STEPS_NUMBERS;

  for (nb_threads = steps_thread; nb_threads <= RANGE_THREADS; nb_threads += steps_thread)
  {
    for (nb_numbers = steps_number; nb_numbers <= RANGE_NUMBERS; nb_numbers += steps_number)
    {
      arguments_t **args = init(comparison);
      std::cout << "[ ITERATION] "
                << "Threads: " << nb_threads << " - Random numbers: " << nb_numbers;

      auto start = std::chrono::high_resolution_clock::now();
      fun(args);
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      std::cout << " - " << elapsed.count() << " s" << std::endl;
    }
  }
}

int job_insert(arguments_t *args)
{
  LinkedListLf<int, int> *result;
  for (int i = 0; i < args->numbers->size(); ++i)
  {
    result = table->insert((*args->numbers)[i], args->start_index + i);
    if (args->active_comparison)
    {
      EXPECT_EQ(result->data, (*args->numbers)[i]);
      EXPECT_EQ(result->key, args->start_index + i);
      EXPECT_EQ(result->hash, args->start_index + i);
    }
  }
  return 1;
}

int job_get(arguments_t *args)
{
  LinkedListLf<int, int> *item;
  for (int i = 0; i < args->numbers->size(); ++i)
  {
    item = table->get(args->start_index + i);
    if (args->active_comparison)
    {
        EXPECT_EQ(item->data, (*args->numbers)[i]);
        EXPECT_EQ(item->key, args->start_index + i);
        EXPECT_EQ(item->hash, args->start_index + i);
    }
  }
  return 1;
}

int job_remove(arguments_t *args)
{
  bool result;
  for (int i = 0; i < args->numbers->size(); ++i)
  {
    result = table->remove(args->start_index + i);
    if (args->active_comparison)
      EXPECT_TRUE(result);
  }
  return 1;
}

TEST(LinkedListTest, Insert)
{
  test_iterations([](arguments_t **args) {
    for (int i = 0; i < nb_threads; ++i)
      boost::asio::post(pool, boost::bind(job_insert, args[i]));
    pool.join();
  },
                  true);
}

TEST(LinkedListTest, Get)
{
  test_iterations([](arguments_t **args) {
    for (int i = 0; i < nb_threads; ++i)
      boost::asio::post(pool, boost::bind(job_insert, args[i]));
    pool.join();

    for (int i = 0; i < nb_threads; ++i)
      boost::asio::post(pool, boost::bind(job_get, args[i]));
    pool.join();
  },
                  true);
}

TEST(LinkedListTest, Remove)
{
  test_iterations([](arguments_t **args) {
    for (int i = 0; i < nb_threads; ++i)
      boost::asio::post(pool, boost::bind(job_insert, args[i]));
    pool.join();

    for (int i = 0; i < nb_threads; ++i)
      boost::asio::post(pool, boost::bind(job_remove, args[i]));
    pool.join();
  },
                  true);
}

TEST(LinkedListTest, ConcurrentUpdates)
{
  test_iterations([](arguments_t **args) {
    for (int i = 0; i < nb_threads; ++i)
    {
      boost::asio::post(pool, boost::bind(job_insert, args[i]));
      boost::asio::post(pool, boost::bind(job_get, args[i]));
      boost::asio::post(pool, boost::bind(job_remove, args[i]));
    }
    pool.join();
  },
                  false);
}

int main(int argc, char **argv)
{
  std::cout << std::boolalpha 
            << "LinkedListLf<int, int>* is trivially copyable ? " 
            << std::is_trivially_copyable<LinkedListLf<int, int>*>::value << " \n"
            << "std::atomic<LinkedListLf<int, int>*> is lock free? "
            << std::atomic<LinkedListLf<int, int>*>{}.is_lock_free() << "\n";
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
