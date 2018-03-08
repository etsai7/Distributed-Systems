# Reliable Point-to-Point File Transfer Tool

## Overview
Constructing a reliable point-to-point basic file transfer tool using UDP/IP.

* [Part 1](#part1): Using UDP/IP
* [Part 2](#part2): Using TCP/IP

<a name="part1"></a>
## Part 1: Using Using UDP/IP
Using the unreliable UDP/IP protocol, we wrote a tool that reliably copies a file to another machine.

The tool consists of two programs: 
	`ncp` - the sender process.
	`rcv` - the receiver process.

In order to perform a file transfer operation, a receiver process (`rcv`) should be run on the target machine.  

A sender process (`ncp`) should be run on the source machine using the following interface:

`ncp <loss_rate_percent> <source_file_name> <dest_file_name>@<comp_name>` 

where `<comp_name>` is the name of the computer where the server runs (ugrad1, ugrad2, etc.)

A receiver process (`rcv`) handles an UNLIMITED number of file-transfer operations, but it is allowed to handle one operation at a time (sequence them). If a sender comes along while the receiver is busy, the sender is blocked until the receiver completes the current transfer. The sender should know about this and report this. The receiver process has the following interface:

`rcv <loss_rate_percent>`

where loss rate is the percentage loss of sent packets. e.g., rcv 10 will drop 10 percent before sending.

A sender process (`npc`) handles one operation and then terminates. We assume that the source file name represents a specific single file.

To check the software, each process (both `rcv` and `npc`) should be calling a coat routine named `sendto_dbg` instead of the `sendto` system call. The coat routine will allow control of an emulated network loss percentage for each sent packet.

**Note** THIS SHOULD BE DONE FOR EVERY SEND OPERATION IN THE CODE (both for the sender and the receiver).

<a name="part2"></a>
## Part 1: Using Using TCP/IP