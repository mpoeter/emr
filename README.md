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
of the interfaces proposed by Robison in "Policy-Based Design
for Safe Destruction in Concurrent Containers".

It also contains implementations of lock-free queue, list and
hash-map data structure, that can be used with the various
reclamation schemes.

The benchmark folder contains a set of benchmarks that were
used to analyze the performance impact of the various reclamation
schemes in different scenarios and on different architectures.