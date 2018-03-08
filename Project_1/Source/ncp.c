///client

#include "net_include.h"
#include "sendto_dbg.h"

void send_next_k(int s, int k, Message pack);

//void selective_repeat(int s, int j);

int loss_rate;
struct sockaddr_in host;
unsigned int host_size = sizeof(host);
FILE* fr;
char destination_file[MAX_PACK_SIZE];
Message window[WINDOW_SIZE][sizeof(Message)];
int shift = WINDOW_SIZE;
int last_ack;
int ack = 0;
int windex = 0;
int pindex = 0;
int feof_flag = 0;

int main(int argc, char **argv) {
  //int loss_rate;
  
  //struct sockaddr_in host;
  struct hostent     h_ent, *p_h_ent;
  
  char               comp_name[80];
  char               *c;
  int                num;
  int                s;
  //int                ret;
  //int                pack_size;
  Message            pack;
  struct timeval     timeout;
  fd_set             mask, dummy_mask, temp_mask;

  if (argc != 4) {
    printf("Usage: ncp <loss_rate> <source_file_name> <dest_file_name>@<comp_name>\n");
    exit(0);
  }
  
  loss_rate = atoi(argv[1]);
  sendto_dbg_init(loss_rate);
  
  if((fr = fopen(argv[2],"rb+")) == NULL){
    printf("Could not open the file.\n");
    exit(0);
  }

  strcpy(destination_file, strtok(argv[3], "@"));
  strcpy(comp_name, strtok(NULL, "@"));
  
  s = socket(AF_INET, SOCK_DGRAM, 0); /* Create a socket (UDP) */
  if (s<0) {
    perror("Net_client: socket error");
    exit(1);
  }
  
  host.sin_family = AF_INET;
  host.sin_port   = htons(PORT);

  printf("%d\n", PORT);
  
  if ( comp_name == NULL ) {
    perror("net_client: Error reading server name.\n");
    exit(1);
  }
  c = strchr(comp_name,'\n'); /* remove new line */
  if ( c ) *c = '\0';
  c = strchr(comp_name,'\r'); /* remove carriage return */
  if ( c ) *c = '\0';
  printf("Your server is %s\n", comp_name);
  
  p_h_ent = gethostbyname(comp_name);
  if ( p_h_ent == NULL ) {
    printf("net_client: gethostbyname error.\n");
    exit(1);
  }

  memcpy( &h_ent, p_h_ent, sizeof(h_ent) );
  memcpy( &host.sin_addr, h_ent.h_addr_list[0],  sizeof(host.sin_addr) );
  
  int ret = connect(s, (struct sockaddr *)&host, sizeof(host) );
  if( ret < 0) {
    perror( "Net_client: could not connect to server"); 
    exit(1);
  }

  pack.wid = 0;
  pack.pid = 0;
  //char response[sizeof(unsigned int)];
  
  FD_ZERO(&mask);
  FD_ZERO(&dummy_mask);
  FD_SET(s, &mask);
  FD_SET( (long) 0, &mask);
  
  // send name of file to create on the other server
  for(;;) {
    memcpy(&pack.data, destination_file, sizeof(destination_file));
    int ret = sendto_dbg(s, (char *) &pack, sizeof(pack), 0, (struct sockaddr *)&host, sizeof(host));
    if(ret != sizeof(pack)) {
      perror("Net_client: error in writing\n");
    }

    unsigned int response;
    temp_mask = mask;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
    if(num > 0) {
      if (FD_ISSET(s, &temp_mask)) {
	printf("Made it in here\n");
	int len = recv(s, (char*)&response, sizeof(response), 0);
	if(len <= 0) {
	  perror("Net_client: error in receiving\n");
	} else if (pack.wid == response) {
	  printf("Message sent successfully. Response: %u\n", response);
	  windex++;      
	  break;
	} else {
	  printf("Error\n");
	}
      }
    } else {
      printf("It broke %d\n", num);
    }
  }

  //memcpy(&pack.data, destination_file, sizeof(destination_file));

  while(!feof(fr)) { // should only run until the file is completely sent
    printf("Sending a full window.\n");

    send_next_k(s, WINDOW_SIZE, pack);

    Response response;
    recv(s, (char *)&response, sizeof(response), 0);
    last_ack = ack;
    ack = response.ack + 1;
    printf("Nacks: %d Received Nack: %d Server Ack: %d\n", response.nacks, response.nack_pack[0], response.ack);
    //int last_index = 0;
    while (ack != pindex) {
      for (int n = 0; n < response.nacks; n++) {
	int index = response.nack_pack[n] % WINDOW_SIZE;
	sendto_dbg(s, (char*)&window[index], sizeof(Message), 0, (struct sockaddr *)&host, host_size);
	printf("Resent packet %02u.%03u\n", window[index]->wid, window[index]->pid);
      }
      shift = ack - last_ack;
      printf("Sending the next %d packets in the next window.\n", shift);
      
      send_next_k(s, shift, pack);
      recv(s, (char *)&response, sizeof(response), 0);

      if (response.ack == -2) { // Received EOF signal
	return 0;
      }
      
      last_ack = ack;
      ack = response.ack + 1;

      if (feof(fr) && response.nacks == 0) {
	if (ack != pindex) {
	  int lindex = (pindex - 1) % WINDOW_SIZE;
	  sendto_dbg(s, (char*)&window[lindex], sizeof(Message), 0, (struct sockaddr*)&host, host_size);
	}
	break;
      }
      printf("Ack: %d Pindex: %u\n", ack, pindex);
      //printf("Nacks: %d Received Nack: %d Server Ack: %d\n", response.nacks, response.nack_pack[0], response.ack);
    }
    /*}
      } else {
      printf("TIMED OUT\n");
      }*/
    //}   
  }
  
  fclose(fr);
  return 0;
}

void send_next_k(int s, int k, Message pack) {
  int start = pindex % WINDOW_SIZE;
  for(int p = start; p < start + k; p++) {
    if(feof(fr)){
      pack.wid = 0;
      pack.pid = pindex;
      pack.data[0] = 0;
      printf("Sending last packet\n");
      memcpy(window[p % WINDOW_SIZE], &pack, sizeof(Message));
      sendto_dbg(s, (char *)&pack, sizeof(pack), 0, (struct sockaddr *)&host, host_size);
      pindex++;
      break;
    }
   
    int read_len = fread(pack.data, sizeof(char), MAX_PACK_SIZE/sizeof(char), fr);

    pack.size = read_len;
    /*if (read_len != MAX_PACK_SIZE) {
      memset(window[p % WINDOW_SIZE], 0, sizeof(Message));
      }*/
    
    pack.wid = windex;
    pack.pid = pindex;
    //size_t pack_size = read_len + (2 * sizeof(unsigned int));
    memcpy(window[p % WINDOW_SIZE], &pack, sizeof(pack));

    //printf("%s", &(pack_buf[sizeof(pack_id)]));
    //printf("Sent packet %d.%d\n", pack.wid, pack.pid);
    int ret;
    if(!feof(fr) || !feof_flag) {
      ret = sendto_dbg(s, (char*)&pack, sizeof(pack), 0, (struct sockaddr *)&host, host_size);
      if(feof(fr)) {
	feof_flag = 1;
      }
    } else {
      ret = sizeof(pack);
    }
    printf("Sent packet %u.%u\n", pack.wid, pack.pid);
    //printf("RET: %d\n",ret);
    if(ret != sizeof(pack)) {
      perror("Net_client: error in writing");
      printf("Socket %d, s_addr %ul, sin_port %d\n", s, host.sin_addr.s_addr, host.sin_port);
      exit(1);
    }
    pindex++;
    if (pindex % WINDOW_SIZE == 0) {
      windex++;
    }
  }
  //printf("In send_up_to: %d\n", pack_id[0]);*/
}

/*void selective_repeat(int s, int j) {
    int ret = sendto_dbg(s,window[j],sizeof(window[j]),0,(struct sockaddr*)&host, sizeof(host));   
    printf("Resent Packet: %d\t Returned:%d\n",j,ret);
    }*/
