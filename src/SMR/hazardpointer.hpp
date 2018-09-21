#ifndef _SMRH_
#define _SMRH_

#include <list>
#include <vector>
#include <algorithm>
#include <atomic>

template<typename T>
class HazardPointer
{
public:
  struct HazardPointerRecord
  {
    std::vector<std::atomic<T *>> hp;
    std::list<T *> rlist;
    std::atomic<bool> active;
    HazardPointerRecord *next;
  };

  void subscribe();
  void unsubscribe();
  T *get_pointer(const int &index);
  void delete_node(const T *node);

  HazardPointer(int a) : nbpointers(a), nbhp(0), head(NULL){};
  ~HazardPointer(){};

private:
  inline HazardPointer<T>::HazardPointerRecord *get_myhp();
  inline unsigned int get_batch_size();
  void scan();
  void help_scan();

  std::atomic<HazardPointerRecord*> head;
  std::atomic<int> nbhp;
  int nbpointers;
};

#endif
