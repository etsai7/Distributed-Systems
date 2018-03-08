#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "net_include.h"
#include "sp.h"
// Put in header file
#define MAX_MESSLEN 1300
#define MAX_VSSETS 10
#define MAX_MEMBERS 10
#define MAX_LOAD 2742 // 342 //914 * 3

static int MAX_BURST;
static  void	Bye();
static	void	Read_message();

static char    User[2];
static char    Private_group[MAX_GROUP_NAME];
static char    Spread_name[80];
static int     To_exit = 0;
static int     process_id;
static int     num_messages;
static int     num_members;
static int     joined_members;
static mailbox Mbox;

int     pack_num = 1;
int     done = 0;
Message pack;
FILE    *fw;
int     fin[MAX_MEMBERS];
int     num_finished = 0;

int main( int argc, char* argv[] ) {
	int ret;
	
	sp_time start, finish;
	
    for(int i =0; i<MAX_MEMBERS; i++){
        fin[i] = 0;
    }

	// Check for correct arguments
	if(argc != 4) {
		printf("Usage : mcast <num_of_messages> <process_index> <num_of_processes>\n");
		exit(1);
	}

	sprintf(Spread_name, "4803");
	
	//test_timeout.sec = 5;
	//test_timeout.usec = 0;

	num_messages = atoi(argv[1]);
	process_id = atoi(argv[2]);
	sprintf(User, "user");
	num_members = atoi(argv[3]);
    MAX_BURST = MAX_LOAD / num_members;
    
	//ret = SP_connect_timeout("nickeric", (char *)&process_id, 0, 1, &Mbox, Private_group, test_timeout);
	ret = SP_connect(Spread_name, User, 0, 1, &Mbox, Private_group);

	if(ret != ACCEPT_SESSION) {
		SP_error(ret);
		Bye();
	}

    //Join a group
	ret = SP_join(Mbox, Spread_name);
	if(ret == 0) {
		printf("Successfully joined.\n");
	} else if(ret < 0) {
		printf("Error in joining. Exiting.\n");
		Bye();
	}
    
    // Open up the file for writing
    char filename[10];
    snprintf(filename,sizeof(filename),"%d.out",process_id);
	fw = fopen(filename,"wb");
    
    pack.sender_id = process_id;
    pack.message_index = pack_num;
    pack.rand_num = 0;
	// Wait for all members to join
	joined_members = 0;
	while(joined_members < num_members) {
		
        Read_message();
		
        //printf("%d %d\n", joined_members, num_members);
	}

	printf("All members joined succesfully.\n");

	start = E_get_time();
	
	while(!done || pack_num <= num_messages) {
		
        if(pack_num <= num_messages){
            int send_count = 0;
            while(pack_num <= num_messages && send_count < MAX_BURST){
               pack.sender_id = process_id;
               pack.message_index = pack_num;
               int x = rand() + 1;
               pack.rand_num = x;
               ret = SP_multicast(Mbox, AGREED_MESS, Spread_name, 0, sizeof(pack), (char*)&pack);
               pack_num++;
               send_count++;            
            }
        }  
        
        if(pack_num > num_messages) {
            pack.sender_id = process_id;
            pack.message_index = -1;
            ret = SP_multicast(Mbox, AGREED_MESS, Spread_name, 0, sizeof(pack), (char*)&pack); 
        }
        
        int recv_count = 0;
        while(!done && pack_num > num_messages){
            Read_message();    
        } 
        while(!done && recv_count < (MAX_BURST * (num_members - num_finished))  ) {
            Read_message();
            recv_count++;
		}
	}

	finish = E_get_time();
	sp_time total = E_sub_time(finish, start);
	printf("Completed in %lu.%06lu seconds.\n", total.sec, total.usec);
	
	return 0;
}

static void	Read_message() {
	static char	     mess[MAX_MESSLEN];
	char		     sender[MAX_GROUP_NAME];
	char		     target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	membership_info  memb_info;
	vs_set_info      vssets[MAX_VSSETS];
	unsigned int     my_vsset_index;
	int              num_vs_sets;
	char             members[MAX_MEMBERS][MAX_GROUP_NAME];
	int		         num_groups;
	int		         service_type;
	int16		     mess_type;
	int		         endian_mismatch;
	int		         i,j;
	int		         ret;

	service_type = 0;
    
    int canFin = 1;
    for(int i = 0; i < num_members; i++){
        if(fin[i] == 0){
            canFin = 0;
        }
    }
    if(canFin){
        printf("\nIn the Read_Message() and should be completely finished\n");
        done = 1;
        return;
    }

	ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups, 
					  &mess_type, &endian_mismatch, sizeof(mess), mess );
	
	if( ret < 0 ) {
		if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
			service_type = DROP_RECV;
			printf("\n========Buffers or Groups too Short=======\n");
			ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups, 
							  &mess_type, &endian_mismatch, sizeof(mess), mess );
		}
	}
	if (ret < 0 ) {
		if( ! To_exit ) {
			SP_error( ret );
			printf("\n============================\n");
			printf("\nBye.\n");
		}
		exit( 0 );
	}
	
	if( Is_regular_mess( service_type ) ) {
		mess[ret] = 0;		
    	memcpy(&pack, mess, ret);
        if(pack.message_index == -1){
            fin[pack.sender_id-1] = 1;
            num_finished++;
            for(int i = 0; i < num_members; i++){
                if(fin[i] == 0){ 
                    return;
                }
            }
            done = 1;
        }else {
            fprintf(fw, "%2d, %8d, %8d\n", pack.sender_id, pack.message_index,pack.rand_num);
        }
	} else if( Is_membership_mess( service_type ) ) {
		ret = SP_get_memb_info( mess, service_type, &memb_info );
		if (ret < 0) {
			printf("BUG: membership message does not have valid body\n");
			SP_error( ret );
			exit( 1 );
		}
	    if( Is_reg_memb_mess( service_type ) ) {
			joined_members = num_groups;
			if( Is_caused_join_mess( service_type ) ) {
				printf("Due to the JOIN of %s\n", memb_info.changed_member );
			}else if( Is_caused_leave_mess( service_type ) ){
				printf("Due to the LEAVE of %s\n", memb_info.changed_member );
			}else if( Is_caused_disconnect_mess( service_type ) ){
				printf("Due to the DISCONNECT of %s\n", memb_info.changed_member );
			}else if( Is_caused_network_mess( service_type ) ) {
				printf("Due to NETWORK change with %u VS sets\n", memb_info.num_vs_sets);
				num_vs_sets = SP_get_vs_sets_info( mess, &vssets[0], MAX_VSSETS, &my_vsset_index );
				if (num_vs_sets < 0) {
					SP_error( num_vs_sets );
					exit( 1 );
				}
				for( i = 0; i < num_vs_sets; i++ ) {
					printf("%s VS set %d has %u members:\n",
						   (i  == my_vsset_index) ?
						   ("LOCAL") : ("OTHER"), i, vssets[i].num_members );
					ret = SP_get_vs_set_members(mess, &vssets[i], members, MAX_MEMBERS);
					if (ret < 0) {
						printf("VS Set has more then %d members. Recompile with larger MAX_MEMBERS\n", MAX_MEMBERS);
						SP_error( ret );
						exit( 1 );
					}
					for( j = 0; j < vssets[i].num_members; j++ )
						printf("\t%s\n", members[j] );
				}
	        }
		}else if( Is_transition_mess(   service_type ) ) {
			printf("received TRANSITIONAL membership for group %s\n", sender );
		}else if( Is_caused_leave_mess( service_type ) ){
			printf("received membership message that left group %s\n", sender );
		}else printf("received incorrecty membership message of type 0x%x\n", service_type );
        } else if ( Is_reject_mess( service_type ) ) {
		printf("REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
			   sender, service_type, mess_type, endian_mismatch, num_groups, ret, mess );
	    }else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);

	fflush(stdout);
}


static void Bye() {
	To_exit = 1;
	printf("\nBye.\n");
	SP_disconnect( Mbox );
	exit( 0 );
}
