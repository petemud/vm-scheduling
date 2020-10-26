This is Petro Mudrievskyj's submission to [VM Scheduling Contest](https://www.algotester.com/vmsc/en) held by [Algotester](https://algotester.com) and sponsored by [Huawei](https://www.huawei.com/)

Here are [the task](statement.pdf "VM scheduling"), [the final scoreboard](https://algotester.com/en/Contest/ViewScoreboard/60331) and his [3rd place certificate](certificate.pdf).

### Best fit
First lets try to solve the folloving task: given the set of servers each described by a sequence `(ram1, cpu1, ram2, cpu2)` - the amount of free memory and cpu cores currently available on each of two nodes of the server - and the vm, which needs `vmram` memory and `vmcpu` cpu cores and `vmnodes` nodes, decide which server and/or node to allocate the vm on.

This task can be solved in many different ways, but here we've used the idea akin to `BEST FIT` studied by Johnson in [this doctoral thesis][1]. The idea is to allocate the vm on a server which it fits best according to some heruistic fitness function. The fitness function used is `min(ram1 + ram2, cpu1 + cpu2)` ([L39-L41](source.cpp#L39-L41)).

For each server we find the amount of memory and cpu cores that would be left after allocating the vm on it:

- if vm requires one node the amount left is either `(ram1 - vmram, cpu1 - vmcpu, ram2, cpu2)` or `(ram1, cpu1, ram2 - vmram, cpu2 - vmcpu)`;
- and if vm requires both nodes - `(ram1 - vmram, cpu1 - vmcpu, ram2 - vmram, cpu2 - vmcpu)`.

*Of course we try only those allocations which are possible (resulting amounts must be non-negative).*

For each resulting server we caltulate the value of a fitness function and select server with the lowest fitness value. If no allocation is possible then we create a new server.

The complexity of the algorithm implemented here ([L43-L93](source.cpp#L39-L41)) is `O(|S|)` where `S` is the set of servers.

### Offline
Now we can try to imply the algorithm above to solve the problem in offline.

First lets select the starting request number (here we start from the last request [L251](source.cpp#L251)). Now find the list of all vms that must be running at a point in time prior to the current request ([L146-L156](source.cpp#L146-L156)). Sort the list so that vms with more memory, cpu cores or nodes go first ([L167-L169](source.cpp#L167-169)). Allocate them on an empty set of servers in a best fit manner. Now process all requests forwards after the current ([L177-L191](source.cpp#L177-L191)) and backwards (create becomes delete and delete becomes create) ([L196-L211](source.cpp#L196-211)).

During all this process we are remembering the first request aftee which the number of servers became the highest. Finally, we repeat the process starting from that request ([L257-L261](source.cpp#L257-L261)) until the time runs out ([L262-L263](source.cpp#L262-L263)) and select the best answer.

### Further improvement
- Try other fitness functions (or switch between them during different runs)
- Sort vms differently ([L168](source.cpp#L168))
- Improve the complexity of best server selection algorithm (our hypothesis is that it can be done in `log`)

[1]: https://dspace.mit.edu/handle/1721.1/57819 "Near-optimal bin packing algorithms"
