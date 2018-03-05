# Efficient Memory Reclamation
This repository contains the C++ implementations of various
lock-free reclamation schemes that I have implemented in
the course of my Master's Thesis.

The implemented schemes are:
* Lock-Free Reference Counting 
* Hazard Pointers
* Quiescent State Based Reclamation
* Epoch Based Reclamation
* New Epoch Based Reclamation
* Stamp-it

The last one (Stamp-it) is a new reclamation scheme that
I have developed as part my thesis.

All scheme implementations are based on an adapted version
of the interfaces proposed by Robison in [Policy-Based Design
for Safe Destruction in Concurrent Containers](http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2013/n3712.pdf).
A detailed list of how my version differs from the original proposal
can be found in the Thesis.

This repository also contains implementations of lock-free queue, list
and hash-map data structures that also use said interface and can
therefore be used with the different reclamation schemes.

The benchmark folder contains a set of benchmarks that were
used to analyze the performance impact of the various reclamation
schemes in different scenarios and on different architectures;
the results can be found in https://github.com/mpoeter/emr-benchmarks