       
The purpose of this assignment was to create a specific MPI network using 
a set topology consisting of 3 set coordinators and a variable number of
workers. The network is able to communicate its topology between the
coordinators which then, in turn, send it further along to their assigned 
workers. Afterwards, it can evenly distribute an array among the network's
nodes for further processing. The requirement of the assignment was a basic
multiplication by two.
The first step is determining the topology of the network. This is done
by each coordinator discovering which workers are assigned to himself by
reading from an input file. After each worker's id is read, a message
informing it that this current coordinator is its parent, is sent.
Once all coordinators have finished reading and informing their assigned
workers, the topology is centralized in coordinator #2 which then in turn
informs the other two of their respective missing parts of the network.
After the transfer is over, the coordinators print the entire topology
and then inform their assigned workers of the network's structure, which 
then also print it.
The algorithm's second stage consists of the transmission of the array
between the first coordinator, #0, and the other two, with the first and 
last ones having to evenly distribute between their workers and the 
other coordinators which have not yet received their own workload. The order
in which this is done is 0 -> 2 -> 1.
Each coordinator has a slightly different job:
    > Coordinator #0 has the task of first determining the default size of a
task a worker from the network should perform its multiplication upon based
on the number of elements it reads from the command line. It then distributes
workloads evenly among its own workers. Afterwards, it must send the rest of 
the array left over to cooordinator #2, along with the default worker segment
size. It then receives the results his assigned workers have sent back, as 
well as the rest of the array sent over by coordinator #2, after it has
finished its job. The last thing it must do is print the result.
    > Coordinator #1 has the simplest task of simply receiving the default 
worker size and the array to be multiplied from coordinator #2, after which
distributing workloads evenly among its workers, receiving the results and 
sending them back to #2.
    > Coordinator #2 receives the default worker size and the portion of the
array to be processed by itself and coordinator #1, sends the section of the
array meant for #1's workers to be processed over to it, and then sends each
of its assigned workers, its required workload. Once the workers are done, 
it appends the returned values to the result array, as well as the array 
returned by #1. It must then send the entire resulting array back to #0.

Technical notes:
The topology is represented in memory as a 2D array. Each line represents
a single coordinator. The first element, for example topology[1][0] contains
the number of assigned workers coordinator #1 has. The next values are
the workers assigned to said coordinator. An arbitrary max size of 
<number_of_tasks>/2 was set, as it didn't make much sense for the value to be
any larger.
In order not to clutter the output unnecesarily, when sending multiple
packets to the same node consecutively, I have printed a single M(x,y) message.
It also helped make the code a little bit more readable, since it lacked in
this regard.
Due to inexact division between the number of elements in the array and
number of worker threads, extensive use of the ceil() function was made.
This usually resulted in the last worker of a coordinator receiving slightly
less elements to process, however, in my opinion this is sort of a feature,
since the thread would probably start computing the result later on than the
rest, for the simple fact that it receives its workload last.
The multiple consecutive MPI_Recv() or MPI_Send() calls could have been
replaced by a structure and a single call, however, this would have, I feel, 
made development more difficult, and the code harder to debug.
