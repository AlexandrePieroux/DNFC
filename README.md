Pieroux Alexandre ULg 2016-2017

Dynamic Network Flow Classification
===================================

This project aim to implement a fast dynamic flow classification algorithm with hardware offloading.

Algorithm used in the project library.
   - Split ordered list             (https://www.cl.cam.ac.uk/research/srg/netos/papers/2001-caslists.pdf)
   - Concurent lockfree hash table  (http://people.csail.mit.edu/shanir/publications/Split-Ordered_Lists.pdf)
   - Hypercuts packet classificator (http://cseweb.ucsd.edu/~susingh/papers/hyp-sigcomm03.pdf)

The code is still in progress and not functional yet (But I'm working hard on it)

Requirements
============
- netmap kernel module: 
    - available in regular FreeBSD linux distribution (release 9.0 and above)
    - available there https://github.com/luigirizzo/netmap
- FreeBSD kernel configuration (for FreeBSD users):
    - kernel have to be configured with 'device netmap' 
   
