

(please do ignore this, this is just to infuck my understanding)
So a couple of things in advance:


1. master socket should be opened from main to which the clients should attach to.
    - `socket`
    - `bind`
    - `listen`

2. How to determine if a connection has been recieved?
- `listen` instructs the kernel to queue that number of connections we pass.
- then accept handles each of the connections enqued to that fd we specified for the previous command.
   It is important that we persist each connFd as each one is bound to a thread per client.

   The way that persist would work is that we'd keep track of the count of each connection and based on that idx we can essentially have an array with that data.

- Once that connfd is retrieved or persisted, we can kickstart a thread. with that fd as argument. At that point this thread would look for the data written by the client to the new redirected fd.


3. Handling queues:

We know that the bytes written from the client side need to be handled by each receiver thread and also
main needs to print whatever is inserted into the queue.

From the receiver:

it makes sense that we store what resides in this fd into an `std::string` and store its entry in the global queue.
to avoid race conditions we need to take the mutex into account, before we write the entry, we lock the mutex and release after.

From main:

 - If the queue is empty, we know that there are no messages in-bound therefore sleep 1 secs. (I'd like to adjust this though) otherwise pop any messages preceded by the mutex handling similar to what we already did from the reciever side.

 
4. Signal handling, 
    we flip our `is_running` flag and Basically main sends "Quit" to all connection fds once we exit the main loop.


I might be missing some extra stuff but that's all I can remember.




