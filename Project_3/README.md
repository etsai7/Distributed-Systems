# Reliable Multicast Tool using Spread

## Overview
Using Spread toolkit, we wrote a tool that reliably multicasts messages between n processes that reside on the same Spread network. 

* [Part 1](#part1): Using Spread
* [Part 2](#part2): Output

<a name="part1"></a>
## Part 1: Using Spread
The tool consists of only one program: 
* `mcast` - the tool's process. 

The mcast process should be run as follows:

`mcast <num_of_messages> <process_index> <num_of_processes>`
* `num_of_processes` indicates how many mcast processes will be run. The maximal number of num_of_processes is 10.
* `num_of_messages` indicates how many messages THIS mcast has to initiate. 
This number may be on the order of 100000. Each message should contain the creating `process_index`, the `message_index`, and a `random number`, each stored in a 4 bytes integer. The `random number` is an integer, picked at the time of the creation of the message, randomly selected from the range 1-1,000,000. In addition, each message will include 1200 additional payload bytes. Therefore, each message should contain at least 1212 bytes. The `message_index` is a per-process counter that is incremented each time a process initiates a message.

After all the mcast processes have been run, they get membership messages and wait until all of them are running (thus, there is no need for a separate start_mcast program).

<a name="part2"></a>
## Part 2: Output
The requirement is for all of the processes to deliver all of the messages in AGREED order. On delivery of a message the mcast should write to a file, in ASCII format, the message id as follows:

'fprintf( fd, "%2d, %8d, %8d\n", message->process_index, message->message_index, message->random_number );''

The output file name will be `<process_index>.out`

### [Project 3 Source](http://www.cnds.jhu.edu/courses/cs437/exercises/Ex3_2016.txt)