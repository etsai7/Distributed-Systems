# Reliable Multicast Tool

## Overview
We construct a reliable multicast tool using UDP/IP.  The reliable multicast assumes no network partitions or processor crashes, but it WILL handle all kinds of message omissions over a local area network.

* [Part 1](#part1): UDP/IP Multicast Tool
* [Part 2](#part2): Output
* [Part 3](#part3): Artificial Loss

<a name="part1"></a>
## Part 1: UDP/IP Multicast Tool
Using the unreliable UDP/IP protocol, we wrote a tool that reliably multicasts packets between `N` machines that reside on the same network segment, so that one multicast message may get to all of them. The tool consists of two programs: 
* `mcast` - the tool's process. 
* `start_mcast` - a process signaling the mcast processes to start.

The mcast process should be run as follows:
`mcast <num_of_packets> <machine_index> <number of machines> <loss rate>`

* `<number of machines>` indicates how many mcast machines will be run. The maximal number of `<number of machines>` should be a constant and should be set to 10. Note that in a particular execution there may be less than or equal to 10 `<number of machines>`.  
* `<loss rate>` is the percentage loss rate for this execution ([0..20] see below). Note also that `machine_index` will be in the range [1..`<number of machines>`] and that you can assume exactly `<number of machines>` mcast programs, each with a different index, will be ready to run before `start_mcast` is executed.
* `num_of_packets` indicates how many packets THIS mcast has to initiate. This number may be on the order of 200,000 packets and even more. Each packet should contain the creating `machine_index`, a `packet_index` and a `random number`, each stored in a 4 byte integer. The `random number` is an integer, picked at the time of the creation of the packet, randomly selected from the range 1-1,000,000. In addition, each packet will include 1300 additional payload bytes. If your protocol requires additional information, that can be added to the packet. Therefore, each packet should include at least 1312 bytes, and possibly more, depending on the information used by the protocol. 

After all the `mcast` processes have been started, the `start_mcast` process is executed, signaling the `mcast` processes to start the protocol by sending a single special multicast message.

<a name="part2"></a>
## Part 2: Output
On delivery of a packet, the mcast should write to a file, in ASCII format, the packet data as follows:
  
`fprintf(fd, "%2d, %8d, %8d\n", pack->machine_index, pack->index, pack->random_data);`

The output file name will be `<machine_index>.out`

<a name="part3"></a>
## Artifical Loss
In order to be able to check the software, the mcast program should call a coat routine named `recv_dbg` instead of the `recv` or `recvfrom` system calls. The coat routine has the same interface as `recv` (see `man 2 recv`). The recv_dbg routine randomly decides to discard packets in order to create network message omissions.  

EVERY RECV CALL in the mcast program MUST use the `recv_dbg` call EXCEPT the first one which receives the special start_mcast packet.  At the beginning of the mcast program it should wait for the `start_mcast` packet to arrive. This will be done by calling `recv` directly (without the coat routine). Only after that packet arrives, the reliable protocol should be invoked. The `start_mcast` process purpose is to help us avoid membership issues.
