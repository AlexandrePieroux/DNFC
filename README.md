Pieroux Alexandre ULg 2015-2016

Dynamic Network Flow Classification
===================================

This project aim to implement a fast dynamic flow classification algorithm with hardware offloading.

Algorithm used in the project library.
   - Split ordered list
   - Concurent hash table lockfree
   - Hypercuts

The code is still in progress and not functional yet (But I'm working hard on it)

Prerequisite
============
In order to test the code several things are required:
   - netmap kernel module: 
      - available in regular FreeBSD linux distribution (release 9.0 and above)
      - available there https://github.com/luigirizzo/netmap
   - kernel configuration:
      - kernel have to be configured with 'device netmap' 
         
   - vagrant
   - virtualbox
   