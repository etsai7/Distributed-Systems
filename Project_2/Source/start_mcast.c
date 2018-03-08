#include "net_include.h"

int main()
{
    struct sockaddr_in name;
    struct sockaddr_in send_addr;

    int                mcast_addr;

    struct ip_mreq     mreq;
    unsigned char      ttl_val;
	
    int                ss,sr;
//    fd_set             mask;
//    fd_set             dummy_mask,temp_mask;
//    int                bytes;
//    int                num;
//    char               mess_buf[MAX_MESS_LEN];
//    char               input_buf[80];
    Tok                tok;

    mcast_addr = 225 << 24 | 1 << 16 | 2 << 8 | 115; /* (225.0.1.1) */

    sr = socket(AF_INET, SOCK_DGRAM, 0); /* socket for receiving */
    if (sr<0) {
        perror("Mcast: socket");
        exit(1);
    }

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
        sizeof(mreq)) < 0) 
    {
        perror("Mcast: problem in setsockopt to join multicast address" );
    }

    ss = socket(AF_INET, SOCK_DGRAM, 0); /* Socket for sending */
    if (ss<0) {
        perror("Mcast: socket");
        exit(1);
    }

    ttl_val = 1;
    if (setsockopt(ss, IPPROTO_IP, IP_MULTICAST_TTL, (void *)&ttl_val, 
        sizeof(ttl_val)) < 0) 
    {
        printf("Mcast: problem in setsockopt of multicast ttl %d - ignore in WinNT or Win95\n", ttl_val );
    }

	tok = (Tok) {TOKEN_ID, 1, -1, -1, 0};
	for(int r = 0; r < MAX_NACKS; r++) {
		tok.nacks[r] = EMPTY;
	}

	for(int m = 1; m < MAX_MACHINES + 1; m++) {
		tok.completed[m] = INCOMPLETE;
	}
	
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(mcast_addr);  /* mcast address */
    send_addr.sin_port = htons(PORT);

    //Maybe this works.
    unsigned long x;
    x = rand();
    x <<= 15;
    x ^= rand();
    x %= 1000001;

    printf("Next Sender:  %d\n",tok.next_sender);
    int ret;
	tok.max_id_sent = -1;
    ret = sendto( ss,(char *) &tok, sizeof(Tok), 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
    printf("\tret = %d, errmsg = %s\n", ret, strerror(errno));


#if 0
    FD_ZERO( &mask );
    FD_ZERO( &dummy_mask );
    FD_SET( sr, &mask );
    FD_SET( (long)0, &mask );    /* stdin */
    for(;;)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0) {
            if ( FD_ISSET( sr, &temp_mask) ) {
                bytes = recv( sr, mess_buf, sizeof(mess_buf), 0 );
                mess_buf[bytes] = 0;
                printf( "received : %s\n", mess_buf );
            }else if( FD_ISSET(0, &temp_mask) ) {
                bytes = read( 0, input_buf, sizeof(input_buf) );
                input_buf[bytes] = 0;
                printf( "there is an input: %s\n", input_buf );
#endif
                //sendto( ss, input_buf, strlen(input_buf), 0, 
                //    (struct sockaddr *)&send_addr, sizeof(send_addr) );
#if 0
            }
        }
    }
#endif

    return 0;
}
