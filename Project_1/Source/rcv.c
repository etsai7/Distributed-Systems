#include "net_include.h"
#include "sendto_dbg.h"

#define NAME_LENGTH 80

int gethostname(char*, size_t);

void PromptForHostName(char *my_name, char *host_name, size_t max_len);
void remove_nack(unsigned int pid);

struct sockaddr_in from_addr;
socklen_t          from_len;
int                srs; //Socket receives and sends
Message            window[WINDOW_SIZE][sizeof(Message)];
Response           response;

int main()
{
  struct sockaddr_in name;
  fd_set             mask;
  fd_set             dummy_mask,temp_mask;
  int                num;
  int                bytes;
  struct timeval     timeout;
  FILE 	             *fw;
  Message            pack;
  int                loss_rate;

  for (int i = 0; i < WINDOW_SIZE; i++) {
    response.nack_pack[i] = -1;
  }
  loss_rate = 0;
  sendto_dbg_init(loss_rate);

  srs = socket(AF_INET, SOCK_DGRAM, 0);
  if(srs<0) {
      perror("UcastL socket");
      exit(1);
  }
    
  name.sin_family = AF_INET;
  name.sin_addr.s_addr = INADDR_ANY;
  name.sin_port = htons(PORT);

  if ( bind(srs, (struct sockaddr *)&name, sizeof(name) ) < 0 ){
      perror("Ucast: bind");
      exit(1);
  }
  
  printf("NAME: Socket %d, s_addr %ul, sin_port %d\n", srs, name.sin_addr.s_addr, name.sin_port);
  
  FD_ZERO( &mask );
  FD_ZERO( &dummy_mask );
  FD_SET( srs, &mask );
  FD_SET( (long) 0, &mask );

  int ack_value = -1;
  response.nacks = 0;
  
  unsigned int last_id = 0;
  int pid_mod;
  int from_ip = 0;
  int valid = 1;
  for(;;) {
    temp_mask = mask;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;
    num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, &timeout);
    if(num > 0) {
      if( FD_ISSET(srs, &temp_mask) ) {
	from_len = sizeof(from_addr);
	memset(&pack, 0, sizeof(pack));
	bytes = recvfrom(srs, (char *)&pack, sizeof(pack),0,
			 (struct sockaddr *)&from_addr, &from_len);

	int pack_ip = from_addr.sin_addr.s_addr;
	
	if (from_ip == 0) {
	  ack_value = -1;
	  last_id = 0;
	  response.nacks = 0;
	  printf("Receiving from new address\n");
	  from_ip = pack_ip;
	} else if (from_ip != pack_ip){
	  printf("Received packet from incorrect address\n");
	  Response busy = (Response){.busy = 1};
	  sendto_dbg(srs, (char *)&busy, sizeof(busy), 0, (struct sockaddr*)&from_addr, from_len);
	  valid = 0;
	} else {
	  valid = 1;
	}
	
	if (pack.size < MAX_PACK_SIZE) {
	  printf("Last packet: %u %u\n", pack.wid, pack.pid);
	}
			    
	//pack.data[bytes] = '\0';
	
	pid_mod = pack.pid % WINDOW_SIZE;
	
	if (pack.wid == 0 && pack.size != 0 && valid) { // Open new file
	  printf("Opened %s for writing.\n", pack.data);
	  fw = fopen(pack.data, "wb");
	  int ret = sendto_dbg(srs, (char *)&pack.wid, sizeof(unsigned int), 0, (struct sockaddr *)&from_addr, from_len);
	  printf("Sent through sendto_dbg:%d\n", ret);
	} else if (pack.wid > 0 && pack.size != 0 && valid) {
	  //memcpy(&window[pid_mod], pack.data, bytes+1);
	  memcpy(&window[pid_mod], (char*)&pack, sizeof(pack));
	  printf("Package %02u.%02u: ", pack.wid, pack.pid);
	  printf("Ack = %2d ", ack_value);
	  if (pack.pid == ack_value + 1) { // If it is lowest unreceived packet
	    fwrite(pack.data, sizeof(char), pack.size, fw);
	    printf(" Wrote lowest packet %02u.%02u\n", pack.wid, pack.pid);
	    ack_value++;
	    if ((ack_value) == response.nack_pack[0]) { // If we receive first nack
	      printf("Received first nack.");
	      int up_to;
	      if (response.nacks == 1) { // If it's the only nack
		printf(" It is the only nack.\n");
		up_to = (int) last_id;
	      } else { // If it's not the only nack
		up_to = response.nack_pack[1];
	      }
	      for (int i = ack_value + 1; i < up_to; i++) {
		int i_mod = i % WINDOW_SIZE;
		if (window[i_mod]->size < MAX_PACK_SIZE) { // EOF
		  printf("We have reached the end of the file writing from window.\n");
		  fwrite(window[i_mod]->data, sizeof(char), window[i_mod]->size, fw);
		  if (fw != NULL) {
		    fclose(fw);
		    fw = NULL;
		    printf("Closed the file.\n");
		  }
		  printf("Resetting IP 1.\n");
		  from_ip = 0;
		  sendto_dbg(srs, (char*)&response, sizeof(response), 0, (struct sockaddr*)&from_addr, from_len);
		  break;
		}
		fwrite(window[i_mod]->data, sizeof(char), window[i_mod]->size, fw);
		printf("Wrote packet %02u.%02u\n", pack.wid, i);
		ack_value++;
	      }
	    }
	  } else {
	    printf(" Didn't write to file.\n");
	  }
	}
	if (pack.pid > last_id && (pack.pid - last_id) < WINDOW_SIZE) { // If missed pack(s)
	  // Store all the missed packets in nacks
	  for (int n = last_id; n < pack.pid; n++) {
	    response.nack_pack[response.nacks] = n;
	    response.nacks++;
	  }
	  printf("Missing %d packets: ", response.nacks);
	  for (int i = 0; i < response.nacks; i++) {
	    printf("%d ", response.nack_pack[i]);
	  }
	  printf("\n");
	  last_id = pack.pid + 1;
	} else if (((int)( pack.pid - last_id) >= (int) WINDOW_SIZE)) {
	  printf("%u - %u = %d\n", pack.pid, last_id, (int)pack.pid - last_id);
	  if (fw != NULL) {
	    fclose(fw);
	    fw = NULL;
	  }
	  from_ip = 0;
	  printf("Resetting IP 2\n");
	  response.ack = -2;
	  sendto_dbg(srs, (char*)&response, sizeof(response), 0, (struct sockaddr*)&from_addr, from_len);
	} else { // If we didn't miss a packet
	  if (pack.pid < last_id) {
	    remove_nack(pack.pid);
	  } else if (pack.pid == last_id && pack.wid != 0) {
	    last_id++;
	  }
	}
	if (pack.size < MAX_PACK_SIZE && pack.wid != 0 && valid) {
	  //if (pack.wid == 0 && pack.data[0] == 0) { // EOF
	  memset(&window[pid_mod], 0, sizeof(window[pid_mod]));
	  printf("LAST PACKET RECEIVED\n");
	  if (response.nacks == 0) {
	    if (fw != NULL) {
	      fclose(fw);
	      fw = NULL;
	      printf("Closed the file.\n");
	    }
	    from_ip = 0;
	    printf("Reset IP 3\n");
	    response.ack = -2;
	    sendto_dbg(srs, (char*)&response, sizeof(response), 0, (struct sockaddr*)&from_addr, from_len);
	  } else {
	    num = -1;
	  }
	  //}
	  printf("EOF or window\n");
	}
      }
      //printf("Got to end of if\n");
    } else { // If we timeout
      //printf("Timed out from receiving from client.Nacks: %d\n ",response.nacks);
      response.ack = ack_value;
      sendto_dbg(srs, (char *) &response, sizeof(response), 0, (struct sockaddr *)&from_addr, from_len);
    }
  }
  //printf("Infinite For loop\n");
  //printf("In between other for loop\n");
  //For loop for requesting missing packages
  printf("Biggest infinite for loop.\n");
  return 0;
}

/*void PromptForHostName( char *my_name, char *host_name, size_t max_len ) {
    
  char *c;

  gethostname(my_name, max_len);
  printf("My host name is %s. \n", my_name);

  printf("\nEnter host to send to: \n");
  if ( fgets(host_name, max_len, stdin) == NULL ) {
        perror("Ucast: read_name");
        exit(1);
	}
  
  c = strchr(host_name, '\n');
  if ( c ) *c = '\0';
  c = strchr(host_name, '\r');
  if ( c ) *c = '\0';
    
    //printf("Sending from %s to %s. \n", my_name, host_name);
}*/

void remove_nack (unsigned int pid) {
  int ind;
  int found = 0;
  // Search for received packed in nack list
  for (int x = 0; x < response.nacks; x++) {
    if (response.nack_pack[x] == pid) {
      found = 1;
      ind = x;
      break;
    }
  } // If it was found, remove it from nack list
  if (found) {
    printf("Nack found; ");
    found = 0;
    for (int n = ind; n < response.nacks - 1; n++) {
      response.nack_pack[n] = response.nack_pack[n+1];
    }
    //response.nack_pack[response.nacks] = -1;
    response.nacks--;
    printf("Nack removed. There are %d nacks left.\n", response.nacks);
  }
}
