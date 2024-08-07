# Summary of PA2

## IP-based Stride Prefetcher

This prefetcher maintains a tracker table having entries corresponding to IP. For every cache miss, the table is checked and/or updated. Then, a structure called lookahead is created, which stores the address corresponding to the cache lines to be prefetched. In every prefetcher clock cycle, this structure is checked, and one-by-one, prefetches are requested.

The prefetcher trains on the stride of cache accesses and prefetches data accordingly.

There was a bug in the code provided (cache line address was used for assigning lookahead), which we fixed by assigning the lookahead with an actual address (not block address).

## Stream Prefetcher

In this prefetcher, we maintain 64 streams, monitoring a group of address whose start and end pointer are separated by a prefetch distance. There are four steps in prefetching - 'Invalid', 'Allocated', 'Training' and 'Monitor and Request'. We follow the steps mentioned in the paper [Feedback Directed Prefetching:Improving the Performance and Bandwidth-Efficiency of Hardware Prefetchers](http://hps.ece.utexas.edu/pub/srinath_hpca07.pdf) to implement this prefetcher. The assignment of lookahead and requesting mechanism is same as ip-stride prefetcher. The only difference is that stride is 1 (block) here.

## IP-based Stride Prefetcher (Throttling)

In this hardware prefetcher implementation, we used the metric 'Accuracy' as a feedback to dynamically change aggressiveness of the prefetcher. We monitor the accuracy whenever a block is filled in the cache. If the accuracy is higher than a fixed threshold, we optimistically increase the degree of prefetching, thus increasing the aggressiveness. Similarly, if it is lower than the threshold, we decrease the degree.

## Stream Prefetcher (Throttling)

In this hardware prefetcher implementation, we used the metric 'Accuracy' as a feedback to dynamically change aggressiveness of the prefetcher. As prescribed in the paper [Feedback Directed Prefetching:Improving the Performance and Bandwidth-Efficiency of Hardware Prefetchers](http://hps.ece.utexas.edu/pub/srinath_hpca07.pdf), we create a configuration table having entries (degree, distance). The table is indexed by a 'Dynamic Configuration Counter' (DCC). Low values of this counter correspond to a conservative prefetcher, while higher values correspond to aggressive. We monitor the accuracy whenever a block is filled in the cache. If the accuracy is higher than a certain threshold, we optimistically increase the DCC. Similarly, if it is lower than a certain threshold, we decrease the DCC.


## Stream + Stride Prefetcher
In this prefetcher, we maintain 64 streams, monitoring a group of address whose start and end pointer are separated by a prefetch distance. There are four steps in prefetching - 'Invalid', 'Allocated', 'Training' and 'Monitor and Request'. We follow the steps mentioned in the paper [Feedback Directed Prefetching:Improving the Performance and Bandwidth-Efficiency of Hardware Prefetchers](http://hps.ece.utexas.edu/pub/srinath_hpca07.pdf). In addition, we also monitor the stride between requests, and prefetch with a stride ahead, instead of prefetching the next line.

## Results 

### Task 1

![Q1 a](Q1a.png)
![Q1 b](Q1b.png)
![Q1 c](Q1c.png)

### Task 2

![Q2 a](Q2_a.png)
![Q3 a](Q2_b.png)

### Task 3

![Q3 a](Q3_a.png)
![Q3 b](Q3_b.png)

### Bonus Task

![Bonus](Bonus.png)



## References

1. [Feedback Directed Prefetching:Improving the Performance and Bandwidth-Efficiency of Hardware Prefetchers:Santhosh Srinath, Onur Mutlu, Hyesoon Kim, Yale N. Patt](http://hps.ece.utexas.edu/pub/srinath_hpca07.pdf)

2. [The Championship Simulator: Architectural Simulation for Education and Competition: Nathan Gober, Gino Chacon, Lei Wang, Paul V. Gratz, Daniel A. Jimenez, Elvira Teran, Seth Pugsley, Jinchun Kim](https://arxiv.org/pdf/2210.14324.pdf)

3. Prof. Biswa's slides
