#include "net_include.h"
#include "recv_dbg.h"

void print_token(Tok tok);

int main(int argc, char **argv)
{
	struct sockaddr_in name;
	struct sockaddr_in send_addr;
	int                mcast_addr;
	struct ip_mreq     mreq;
	unsigned char      ttl_val;
    int                ss,sr;
    fd_set             mask;
    fd_set             dummy_mask,temp_mask;
    int                bytes;
    int                num;
    struct timeval     timeout;
    int                num_packets; //Number of packets to send.
    int                machine_index;
    int                num_machines;
    int                loss_rate;
    Tok                tok;
    FILE               *fw;
    int                local_seq = -1; // Local sequence number.
    int                local_nacks[MAX_NACKS];
    int                local_ack = -1;
	int                last_aru;
    Message            pack;
    int                num_nacks_this_round;
    Message            window[MAX_NACKS];
	int                sequential_mode = 0;
    int                finished = 0;
	int                pack_flag = 0;
  /*
	struct sockaddr_in usend_addr;
	char               my_name[NAME_LENGTH];
	struct hostent     h_ent;
	struct hostent     *p_h_ent;
	int                my_ip;

	gethostname(my_name, NAME_LENGTH);

	p_h_ent = gethostbyname(my_name);
	if ( p_h_ent == NULL ) {
        printf("myip: gethostbyname error.\n");
        exit(1);
    }

	memcpy( &h_ent, p_h_ent, sizeof(h_ent));
    memcpy( &my_ip, h_ent.h_addr_list[0], sizeof(my_ip) );
  */
	
    for(int r = 0; r < MAX_NACKS; r++) {
		local_nacks[r] = EMPTY;
		window[r] = (Message) {MESS_ID, EMPTY, EMPTY, EMPTY};
    }

	srand(time(NULL));

    if( argc != 5){
        printf("Usage: mcast <num_of_packets> <machine_index> <number of machines> <loss rate>\n");
        exit(0);
    }

    num_packets = atoi(argv[1]);
    machine_index = atoi(argv[2]);
    num_machines = atoi(argv[3]);

    if(num_machines > MAX_MACHINES){
        printf("Error: Too many machines.\n");
        exit(0);
    }

    loss_rate = atoi(argv[4]);

    printf("Calling recv_dbg_init\n");
    
    printf("Made it past recv_dbg_init\n");

    mcast_addr = 225 << 24 | 1 << 16 | 2 << 8 | 115; /* (225.1.2.115) */

    printf("Mcast_addr is set\n");

    sr = socket(AF_INET, SOCK_DGRAM, 0); /* socket for receiving */
    if (sr<0) {
        perror("Mcast: socket");
        exit(1);
    }

    printf("Setting sr socket.\n");

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( sr, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Mcast: bind");
        exit(1);
    }

    mreq.imr_multiaddr.s_addr = htonl( mcast_addr );

    /* the interface could be changed to a specific interface if needed */
    mreq.imr_interface.s_addr = htonl( INADDR_ANY );

    if (setsockopt(sr, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&mreq, 
        sizeof(mreq)) < 0) {
        perror("Mcast: problem in setsockopt to join multicast address" );
    }

    ss = socket(AF_INET, SOCK_DGRAM, 0); /* Socket for sending */
    if (ss<0) {
        perror("Mcast: socket");
        exit(1);
    }
	
	recv_dbg_init(loss_rate, machine_index);
	
    ttl_val = 1;
    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val, 
        sizeof(ttl_val)) < 0) {
        printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val );
    }

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(mcast_addr);  /* mcast address */
    send_addr.sin_port = htons(PORT);
	
	/*usend_addr.sin_family = AF_INET;
	usend_addr.sin_addr.s_addr = ; 
	usend_addr.sin_port = htons(PORT);*/
	
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    //This is the activation for loop
    printf("Waiting to be Activated\n");
    for(;;) {
        printf("Loop\n");
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0) {
            printf("alive\n");
            if ( FD_ISSET( sr, &temp_mask) ) {
                bytes = recv( sr, (char*)&tok, sizeof(tok), 0 );
                printf("received %d with %d bytes\n", tok.next_sender, bytes);
                char filename[10];
                snprintf(filename, sizeof(filename), "%d.out", machine_index);
                fw = fopen(filename,"wb"); 
                break;
            }
        }
    }

	/*int info[2] = {machine_index, htonl(my_ip)};
	
	// Setup loop to get address of to send token to
	for(;;) {
		// Send everyone my machine_index and IP address
		sendto(ss, (char *)&info, 2*sizeof(int), 0,
						   (struct sockaddr *)&send_addr, sizeof(send_addr));
		// Wait to receive everyone's machine_index and IP address
		temp_mask = mask;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		num = select(FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
		if(num > 0){
			
		}
		}*/
//#if 0    
    FD_ZERO(&mask);
    FD_ZERO(&dummy_mask);
    FD_SET(sr,&mask);
    FD_SET((long)0, &mask);
    for(;;){
//Retransmission--------------------------------------------
		printf("Token up top. Next_Sender: %d,Aru: %d, Max_Id_Sent: %d, Last Lowerer: %d.\n",tok.next_sender, tok.aru, tok.max_id_sent, tok.last_lowerer);
        if(tok.next_sender == machine_index){ // If I have the token
			// Initialize pack for sending
			pack = (Message) {MESS_ID, machine_index, machine_index, tok.max_id_sent};
			printf("Sending retransmissions.\n");
			num_nacks_this_round= 0;
			/*printf(" ");
			for(int c = 0; c < 32; c++) {
				printf("%d ", c);
			}		   
			
			printf("\n");*/
			
			int min_nack = -1;
			
			for(int i = 0; i < MAX_NACKS; i++) {
				if (tok.nacks[i] < min_nack || min_nack == -1) {
					min_nack = tok.nacks[i];
				}
				if(i % 32 == 0)
					printf("\n%d", i/32);
				else
					printf(" ");
					printf("%d", tok.nacks[i]);
				if(tok.nacks[i] != EMPTY && window[i].seq == tok.nacks[i]){					
					window[i].sender_id = machine_index;
					sendto(ss, (char *)&window[i], sizeof(Message), 0,
						   (struct sockaddr *)&send_addr, sizeof(send_addr));
					num_nacks_this_round++;
					//printf("*");
					tok.nacks[i] = EMPTY;
				}
			}
			//printf("\n");
			printf("Sent out %d retransmissions.\n", num_nacks_this_round);

//Token Maintenance-----------------------------------------
//#if 0        
			printf("Performing token maintenance.\n");
			// If I last decreased aru, increase aru
            if(tok.last_lowerer == machine_index) {
				printf("I was the last lowerer. Raising token ack to ");
				if(local_ack > tok.aru) {
					tok.last_lowerer = 0;
				}
				if(min_nack == -1) { // Increase to local ack if there are no nacks
					printf("local ack %d.\n", local_ack);
					tok.aru = local_ack;
				} else {
					printf("minimum nack %d.\n", min_nack);
					tok.aru = min_nack - 1; // Increase to minimum nack otherwise
				}
				//tok.last_lowerer = 0;
			} // Set myself as the last lowerer
			else if((tok.aru == local_ack && tok.aru < tok.max_id_sent) || last_aru == tok.aru){
				printf("Setting myself as the last lowerer.");
				tok.last_lowerer = machine_index;
			} else if(tok.aru > local_ack) {        // Lower aru if it's higher
				printf("Lowering aru.\n");
				tok.aru = local_ack;
				tok.last_lowerer = machine_index;
			}
			// Everyone's received everything so far
			if(tok.aru == tok.max_id_sent && local_ack == tok.aru) {
				sequential_mode = 1;
                printf("Sequential mode Activated.\n");
			} else {                                // Not everyone has everything
				sequential_mode = 0;
			}

//#endif
//Sending out Packs-------------------------------------
//#if 0
			// I moved this up here b/c it shouldn't affect anything going on below
			tok.next_sender %= num_machines;
			tok.next_sender++;
			
			if(num_packets <= 0 && tok.aru > local_seq){ // No more packets to send but still receiving packets
				printf("No more packets to send; still receiving packets.\n");
				tok.completed[machine_index] = INCOMPLETE;
			} else if(num_packets <= 0 && tok.aru == local_seq){ // No more packets to send and no more retransmissions. Complete
				printf("No more packets to send or receive.\n");
                tok.completed[machine_index] = COMPLETE; 
                int can_terminate = 1;
                for(int i = 1; i <= num_machines; i++){
                    if(tok.completed[i] != COMPLETE){
                        can_terminate = 0;
                        break;
                    }
                }
                if(can_terminate){
					printf("Everyone has finished.\n");
                    finished = 1;
                }
            } else { // Not done sending out packets
				// Send until the token we're about to send - tok.aru == MAX_NACKS
				// or until we have sent MAX_BURST packets
				//printf("Sending packets on %d.\n", ss);
				int sent = 0;
				while((tok.max_id_sent + 1) - tok.aru < MAX_NACKS && sent < MAX_BURST && num_packets > 0) {
					// Increase seq on pack and token
					pack.seq++;
					tok.max_id_sent++;
                    pack.sender_id = machine_index;
					pack.originator_id = machine_index;
                    
					// Generate random number
					unsigned long x;
					x = rand();
					x %= 1000000;
					x++;
					pack.rand_id = (int) x;
					//printf("Sending packet %d with random ID of %d.\n", pack.seq, pack.rand_id);
					// Send packet
					sendto(ss, (char *)&pack, sizeof(Message), 0,
						   (struct sockaddr *)&send_addr, sizeof(send_addr));

					// Increase aru if sequential
                	if(sequential_mode) {
						tok.aru++;
						local_ack++;
						fprintf(fw, "%2d, %8d, %8d\n", pack.originator_id, pack.seq, pack.rand_id);
						window[pack.seq % MAX_NACKS] = pack;
					}
					sent++;
					num_packets--;
					int index = pack.seq % MAX_NACKS;
					window[index] = pack;
				}
				printf("Sent out packets %d-%d.\n", pack.seq - sent, pack.seq);
				if (local_seq < pack.seq) {
					local_seq = pack.seq;
				}
			}
			// Update last aru
			last_aru = tok.aru;
			// Multicast token
			printf("Sending token {%d, %d, %d} to %d. %d packets left.\n", tok.aru, tok.max_id_sent, tok.last_lowerer, tok.next_sender, num_packets);
			sendto(ss, (char *)&tok, sizeof(Tok), 0, (struct sockaddr *)&send_addr,
			sizeof(send_addr));
			
			/*for(;;){
				temp_mask = mask;
				timeout.tv_sec = 1;
				timeout.tv_usec = 0;
				num = select(FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
				if(num > 0){ // Wait for token ack or packet from who I sent token to
					int recv;
					recv_dbg(sr, (char *)&recv, sizeof(Message), 0);
					if(recv == TOKEN_ACK_ID) {
						break;
					} else if(recv == MESS_ID) {
						Message temp;
						memcpy(&temp, &recv, sizeof(Message));

						int next_sender = machine_index % num_machines;
						next_sender++;
						
						if(temp.sender_id == next_sender) {
							pack = temp;
							pack_flag = 1; // Set flag to not receive
						}
					}
				} else { // Didn't receive token ack; resend token
					
				}
				}*/
		} // end of if(tok.next_sender == machine_index)
        
		if(finished == 1){
			break;
		}
//#endif        
//Receiving----------------------------------------------        
//#if 0
        for(;;){
			//Received Something
            temp_mask = mask;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            num = select(FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
            if(num > 0){
				char recvd[sizeof(Tok)];
				if (!pack_flag){
					bytes = recv_dbg( sr, recvd, sizeof(Tok), 0 );
				} else {				
					recvd[0] = MESS_ID;
				}
				
				int id = (int) recvd[0];	// Get first int in message received
				// CASE 1: Received a token
				if(id == TOKEN_ID) { // If I received a token
					memcpy(&tok, recvd, bytes); // Update my token				   									
					if(tok.next_sender == machine_index){ // If I have the token
						printf("Token:\n");
						print_token(tok);
						
						// Send token ack
						int token_ack = TOKEN_ACK_ID;
						sendto(ss, (char *)&token_ack, sizeof(int), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
						
						if(tok.aru > tok.max_id_sent) {
							tok.max_id_sent = tok.aru;
						}
						
						// If last message received is not last message sent,
						// add all the missed packets to the nack list.
						if (tok.max_id_sent > local_seq) {
							for(int n = local_seq+1; n <= tok.max_id_sent; n++) {
								int index = n % MAX_NACKS;
								local_nacks[index] = n;
							}
							// Update local_seq
							local_seq = tok.max_id_sent;
						}
					
						printf("I am the next sender. My local_ack is %d and local_seq is %d\n", local_ack, local_seq);
						// Add local nacks to token
						printf("Adding nacks ");
						for(int i = 0; i < MAX_NACKS; i++) {
							if(local_nacks[i] != EMPTY) {
								tok.nacks[i] = local_nacks[i];
								printf("%d ", local_nacks[i]);
							}
						}
						printf("to token.\n");
						
						break;	// Goes back to the top and transmits things.
					} else {
						printf("Token. I am not the next sender. The next sender is %d\n", tok.next_sender);
					}
					// CASE 2: Received a message
				} else if(id == MESS_ID){ // Received a data packet
					// Convert it to data pack
					memcpy(&pack, recvd, bytes);
					
					int index = pack.seq % MAX_NACKS;
					// CAN WRITE-------------------------------------------
					// If we've received everything so far
					if(pack.seq == local_ack + 1 && local_ack == local_seq) {
						printf("We are writing to file and everything is complete\n");
						fprintf(fw, "%2d, %8d, %8d\n", pack.originator_id, pack.seq,
								pack.rand_id);
						local_ack++;
						local_seq++;
					} else if (pack.seq == local_seq + 1 && local_ack  < local_seq){
						window[index] = pack;
						local_seq = pack.seq;
					}
					// If we receive a pack we missed earlier and is the first nack.
					else if(pack.seq == local_ack + 1 && local_ack < local_seq) {
						//printf("Somehow missing and recovering packets\n");
						// Write the pack we just received
						fprintf(fw, "%2d, %8d, %8d\n", pack.originator_id, pack.seq,
								pack.rand_id);
						local_nacks[index] = EMPTY;
						window[index] = pack;
						local_ack++;
						index = (local_ack + 1) % MAX_NACKS;
						int iter = 0;
						// Write up to the next packet we are missing
						while(iter < MAX_NACKS && local_nacks[index] == EMPTY && local_ack < local_seq) {
							fprintf(fw, "%2d, %8d, %8d\n", window[index].originator_id,
									window[index].seq, window[index].rand_id);
							local_ack++;
							iter++;
							index++;
							index %= MAX_NACKS;
						}
					}
					// CANNOT WRITE-----------------------------
					// If we receive a packet we missed earlier out of order
					else if(pack.seq > local_ack + 1 && pack.seq < local_seq) {
						local_nacks[index] = EMPTY;
						window[index] = pack;
					}
					// If we receive a packet out of order
					else if(pack.seq > local_seq + 1) {
						// Add all missed seq numbers to nack list
						for(int i = local_seq + 1; i < pack.seq; i++){
							index = i % MAX_NACKS;
							local_nacks[index] = i;
							//printf("Requesting nack for %d.\n", i);
						}
						// Update local_seq
						local_seq = pack.seq;
						// Store packet to window so we can write it later
						window[pack.seq % MAX_NACKS] = pack;
					}
					//printf("<--- %d from %d. Local_ack: %d and local_seq: %d\n", pack.seq, pack.sender_id, local_ack, local_seq);
					// No matter what, store received pack in buffer
					window[pack.seq % MAX_NACKS] = pack;
				} // end of else if (id == MESS_ID)
			}
			else { //If it times out, check to see if it needs to resend token, otherwise wait.
				printf("Timed out waiting to receive. ");
				int next_sender = machine_index;
				next_sender %= num_machines;
				next_sender++;
				// If I sent the last packet and I last had the token
				
                if(pack.sender_id == machine_index)
					printf("I was the last sender by pack_id.\n");
				if(tok.next_sender == next_sender)
					printf("I was the last sender by token.\n");
				
                if( tok.next_sender == next_sender && pack.sender_id == machine_index){
					printf("I am the last sender. Resending token.\n");
					sendto( ss, (char *)&tok, sizeof(Tok), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
				} else printf("I am not the last sender.\n");
				// Will only fail if I missed all token passes AND all packets since I passed the token
			}
        }
    }
    //#endif 
    
    //#endif
    return 0;
}

void print_token(Tok tok) {
	printf("Token ID: %d\nNext sender: %d\nToken ARU: %d\nToken SEQ: %d\nLast lowerer: %d\n",
		   tok.token_id, tok.next_sender, tok.aru, tok.max_id_sent, tok.last_lowerer);
}
