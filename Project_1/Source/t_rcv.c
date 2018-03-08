#include "net_include.h"

int main()
{
    struct sockaddr_in name;
    int                s;
    fd_set             mask;
    int                recv_s[10];
    int                valid[10];  
    fd_set             dummy_mask,temp_mask;
    int                i,j,num;
    int                mess_len;
    int                neto_len;
    char               mess_buf[MAX_MESS_LEN];
    long               on=1;
    FILE               *fw;
    
    printf("I am at the beginning\n");
        
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) {
        perror("Net_server: socket");
        exit(1);
    }

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        perror("Net_server: setsockopt error \n");
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(PORT);

    if ( bind( s, (struct sockaddr *)&name, sizeof(name) ) < 0 ) {
        perror("Net_server: bind");
        exit(1);
    }
 
    if (listen(s, 4) < 0) {
        perror("Net_server: listen");
        exit(1);
    }

    i = 0;
    FD_ZERO(&mask);
    FD_ZERO(&dummy_mask);
    FD_SET(s,&mask);
    
    printf("About to open the file,\n");
    fw = fopen("Received.txt","wb");
    printf("Opened the file\n");
    
    for(;;)
    {
        temp_mask = mask;
        num = select( FD_SETSIZE, &temp_mask, &dummy_mask, &dummy_mask, NULL);
        if (num > 0) {
            printf("Received something\n");
            if ( FD_ISSET(s,&temp_mask) ) {
                recv_s[i] = accept(s, 0, 0) ;
                FD_SET(recv_s[i], &mask);
                valid[i] = 1;
                i++;
                printf("First if Statement\n");
            }
            for(j=0; j<i ; j++)
            {   printf("In for loop\n");
                if (valid[j])    
                    printf("In for loop first if\n");
                if ( FD_ISSET(recv_s[j],&temp_mask) ) {
                    printf("In for loop second if\n");
                    if( recv(recv_s[j],&mess_len,sizeof(mess_len),0) > 0) {
                        printf("In for loop third if \n");
                        neto_len = mess_len - sizeof(mess_len);
                        printf("Can get neto_len\n");
                        recv(recv_s[j], mess_buf, neto_len, 0 );
                        printf("Received something \n");
                        mess_buf[neto_len] = '\0';
                        printf("Got mes_buf at end\n");
                        printf("\nneto_len: %s\n",mess_buf);
                        printf("Message: %s\n",mess_buf);
                        fwrite(mess_buf, sizeof(char),sizeof(mess_buf)/sizeof(char), fw);
                        
                        printf("socket is %d ",j);
                        //printf("len is :%d  message is : %s \n ",
                        //       mess_len,mess_buf); 
                        printf("---------------- \n");
                    }
                    else
                    {
                        printf("closing %d \n",j);
                        FD_CLR(recv_s[j], &mask);
                        close(recv_s[j]);
                        valid[j] = 0;  
                    }
                }
            }
        }
    }

    return 0;

}

