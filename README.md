Pieroux Alexandre

Dynamic Network Flow Classification
===================================

This project aim to implement a fast dynamic flow classification algorithm in networks.

Algorithm used in the project:
   - Safe Memory Reclamation        (http://www.research.ibm.com/people/m/michael/podc-2002.pdf) 
   - Lock-free linked list          (http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.114.5854&rep=rep1&type=pdf)
   - Concurent lock-free hash table (http://people.csail.mit.edu/shanir/publications/Split-Ordered_Lists.pdf)
   - Flow Table                     (lock-free hash table based algorithm for dynamic flows classification)
   - Concurent lockfree queue       (https://www.research.ibm.com/people/m/michael/podc-1996.pdf)
   - Hypercuts packet classificator (http://cseweb.ucsd.edu/~susingh/papers/hyp-sigcomm03.pdf)
   - Thread pool with blocking queue for jobs
   
The code is being ported form C to C++.
