### I have started a new project based on this work: https://github.com/mpoeter/xenium
**All further development is done there!**

---

# Effective Memory Reclamation
This repository contains the C++ implementations of various
lock-free reclamation schemes that I have implemented in
the course of my [Master's Thesis](http://www.ub.tuwien.ac.at/dipl/VL/51367.pdf).

The implemented schemes are:
* Lock-Free Reference Counting 
* [Hazard Pointers](http://www.cs.otago.ac.nz/cosc440/readings/hazard-pointers.pdf)
* Quiescent State Based Reclamation
* [Epoch Based Reclamation](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-579.pdf)
* [New Epoch Based Reclamation](http://csng.cs.toronto.edu/publication_files/0000/0159/jpdc07.pdf)
* [DEBRA](http://www.cs.utoronto.ca/~tabrown/debra/paper.podc15.pdf)
* Stamp-it

The last one (Stamp-it) is a new reclamation scheme that
I have developed as part my thesis. All schemes and their
implementations are described in detail in the thesis.

The implementations are based on an adapted version
of the interfaces proposed by Robison in [Policy-Based Design
for Safe Destruction in Concurrent Containers](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2013/n3712.pdf).
A detailed list of how my version differs from the original proposal
can be found in the Thesis.

This repository also contains implementations of Michael and Scott's
lock-free queue, as well as Michael and Harris' list based set
and hash-map. These data structures are implemented using said
interface and can therefore be used with the different reclamation
schemes.

The benchmark folder contains a set of benchmarks that were
used to analyze the performance impact of the various reclamation
schemes in different scenarios and on different architectures;
the results can be found in https://github.com/mpoeter/emr-benchmarks
