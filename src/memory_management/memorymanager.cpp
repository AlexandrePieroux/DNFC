#include <stdio.h>
#include <vector>
#include <string>
using namespace std;

#include "memorymanager.hpp"
#include "classes.h"

MemoryManager gMemoryManager;

void *MemoryManager::allocate(size_t size)
{
  void *base = 0;
  switch (size)
  {
  case JOB_SCHEDULER_SIZE: //28
  {
    if (Byte32PtrList.empty())
    {
      base = new char[32 * POOL_SIZE];
      MemoryPoolList.push_back(base);
      InitialiseByte32List(base);
    }
    void *blockPtr = Byte32PtrList.front();
    *((static_cast<char *>(blockPtr)) + 30) = 32; //size of block set
    *((static_cast<char *>(blockPtr)) + 31) = 0;  //block is no longer free
    Byte32PtrList.pop_front();
    return blockPtr;
  }
  case COORDINATE_SIZE: //36
  {
    if (Byte40PtrList.empty())
    {
      base = new char[40 * POOL_SIZE];
      MemoryPoolList.push_back(base);
      InitialiseByte40List(base);
    }
    void *blockPtr = Byte40PtrList.front();
    *((static_cast<char *>(blockPtr)) + 38) = 40; //size of block set
    *((static_cast<char *>(blockPtr)) + 39) = 0;  //block is no longer free
    Byte40PtrList.pop_front();
    return blockPtr;
  }
  case COMPLEX_SIZE: //16
  {
    if (Byte24PtrList.empty())
    {
      base = new char[24 * POOL_SIZE];
      MemoryPoolList.push_back(base);
      InitialiseByte24List(base);
    }
    void *blockPtr = Byte24PtrList.front();
    *((static_cast<char *>(blockPtr)) + 22) = 32; //size of block set
    *((static_cast<char *>(blockPtr)) + 23) = 0;  //block is no longer free
    Byte24PtrList.pop_front();
    return blockPtr;
  }
  default:
    break;
  }
  return 0;
}

void MemoryManager::free(void *object)
{
  char *init = static_cast<char *>(object);

  while (1)
  {
    int count = 0;
    while (*init != 0xde) //this loop shall never iterate more than
    {                     // MAX_BLOCK_SIZE times and hence is O(1)
      init++;
      count++;
      if (count > MAX_BLOCK_SIZE)
      {
        printf("runtime heap memory corruption near %d", object);
        exit(1);
      }
    }
    if (*(++init) == 0xad) // we have hit the guard bytes
      break;               // from the outer while
  }
  init++; // ignore size byte
  init++;
  *init = 1; // set free/available byte
}

void MemoryManager::InitialiseByte24List(void *base)
{
  for (int i = 0; i < POOL_SIZE; ++i)
  {
    char *guardByteStart = &(static_cast<char *>(base)[i * 24]) + 20;
    *guardByteStart = 0xde;
    guardByteStart++;
    *guardByteStart = 0xad; //end of block
    guardByteStart++;
    *guardByteStart = 24; //sizeof block
    guardByteStart++;
    *guardByteStart = 1; //block  available
    Byte24PtrList.push_front(&(static_cast<char *>(base)[i * 24]));
  }
}

void MemoryManager::InitialiseByte32List(void *base)
{
  for (int i = 0; i < POOL_SIZE; ++i)
  {
    char *guardByteStart = &(static_cast<char *>(base)[i * 32]) + 28;
    *guardByteStart = 0xde;
    guardByteStart++;
    *guardByteStart = 0xad; //end of block
    guardByteStart++;
    *guardByteStart = 32; //sizeof block
    guardByteStart++;
    *guardByteStart = 1; //block  available
    Byte32PtrList.push_front(&(static_cast<char *>(base)[i * 32]));
  }
}

void MemoryManager::InitialiseByte40List(void *base)
{
  for (int i = 0; i < POOL_SIZE; ++i)
  {
    char *guardByteStart = &(static_cast<char *>(base)[i * 40]) + 36;
    *guardByteStart = 0xde;
    guardByteStart++;
    *guardByteStart = 0xad; //end of block
    guardByteStart++;
    *guardByteStart = 40; //sizeof block
    guardByteStart++;
    *guardByteStart = 1; //block  available
    Byte40PtrList.push_front(&(static_cast<char *>(base)[i * 36]));
  }
}
