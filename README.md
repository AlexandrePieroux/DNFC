Pieroux Alexandre ULg 2016-2017

Dynamic Network Flow Classification
===================================

This project aim to implement a fast dynamic flow classification algorithm with hardware offloading.

Algorithm used in the project library.
   - Safe Memory Reclamation        (http://www.research.ibm.com/people/m/michael/podc-2002.pdf) 
   - Lock-free linked list          (https://www.cl.cam.ac.uk/research/srg/netos/papers/2001-caslists.pdf)
   - Concurent lockfree hash table  (http://people.csail.mit.edu/shanir/publications/Split-Ordered_Lists.pdf)
   - Flow Table (hatable based flow classification)
   - Concurent lockfree queue       (https://www.research.ibm.com/people/m/michael/podc-1996.pdf)
   - Hypercuts packet classificator (http://cseweb.ucsd.edu/~susingh/papers/hyp-sigcomm03.pdf)
   - Thread pool with blocking queue for jobs

The code is still in progress and not functional yet (But I'm working hard on it)

Requirements
============
- netmap kernel module: 
    - available in regular FreeBSD linux distribution (release 9.0 and above)
    - available there https://github.com/luigirizzo/netmap
- FreeBSD kernel configuration (for FreeBSD users):
    - kernel have to be configured with 'device netmap' 
   
