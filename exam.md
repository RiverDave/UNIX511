
# Quiz on mutexes and queues:


### kernel queues











=====================




### MUTEXES

Lock and release mechanism to coordinate multiple threads accessing shared resources in
order to avoid race conditions.

* Mutex vs critical Section
    Mutexes seem to be bound to the kernel level and can be shared amongst many multiple processes.
    Critical section seem to be able to be shared PER process - these are typically faster