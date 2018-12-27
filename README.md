Pieroux Alexandre

Dynamic Network Flow Classification
===================================

This project aim to implement a fast dynamic flow classification algorithm in networks.
The code is being ported form C to C++.

Algorithm used in the project library.
   - Safe Memory Reclamation        (http://www.research.ibm.com/people/m/michael/podc-2002.pdf) 
   - Lock-free linked list          (https://www.cl.cam.ac.uk/research/srg/netos/papers/2001-caslists.pdf)
   - Concurent lock-free hash table (http://people.csail.mit.edu/shanir/publications/Split-Ordered_Lists.pdf)
   - Flow Table                     (lock-free hash table based algorithm for network flows classification)
   - Concurent lockfree queue       (https://www.research.ibm.com/people/m/michael/podc-1996.pdf)
   - Hypercuts packet classificator (http://cseweb.ucsd.edu/~susingh/papers/hyp-sigcomm03.pdf)
   - Thread pool with blocking queue for jobs

===================================
