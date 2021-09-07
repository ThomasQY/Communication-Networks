/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#define SYN 0
#define SYN_ACK 3
#define ACK 1
#define FIN 2
#define FIN_ACK 4
#define DATA 5

#define DATA_MAX 10000  // size of data field in a packet
#define TIMEOUT_FIN 3 


// when msg_type is a ack, seq_num is the packet this ACK is ackting to, when msg_type is DATA, seq_num is the packet number 
typedef struct{
    int     msg_type;  
    int     data_size;
    int     seq_num;
    char    data[DATA_MAX];
}packet;

typedef struct{
    int     msg_type;  
    int     packet_tot;         // total number of packets in upcoming transmission
    struct  sockaddr_in si;     // my addr info, let receiver know where to throw its ack
}meta_packet; // used for SYN and FIN packet


FILE *fp,*r_log;
struct sockaddr_in si_me, si_other;
int s, slen;

packet * buffer; // buffer for all packets
int * received; // received[i] set to 1 if buffer[i] has some content 
int current_print;
int tot_packet = -1;
int inorder = -1; // last inorder packet we have received
int total_acted = 0;

int get_syn = 0;
int wait_fin = 0;

// functions
void grabSocket(unsigned short int myUDPport);
void receive();
void fileWrite(char * destinationFile);


void diep(char *s) {
    perror(s);
    exit(1);
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    grabSocket(myUDPport);
    // open file to write
    fp = fopen(destinationFile,"wb");
    // r_log = fopen("log.txt","w");
    // open file for read and write
    if(fp == NULL){
        diep("file open");
        return;
    }
    receive();
    // initialize other's socket structure
	/* Now receive data and send acknowledgements */    
    //fileWrite(destinationFile);
    close(s);
    free(received);
    free(buffer);
    fclose(fp);
	printf("%s received.", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        // fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);
    reliablyReceive(udpPort, argv[2]);
}

/************************** Helper Functions ********************************/



void grabSocket(unsigned short int myUDPport){
     slen = sizeof (si_other);
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");
    fcntl(s, F_SETFL, O_NONBLOCK); // set socket to be non-blocking
    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");
}


/* 
* receive: Main file receiving loop, store all packet in buffer and record which packet have been received in received
*  terminate when received a FIN and send FIN_ACK or haven't heard from sender for an extended period of time 
 */
void receive(){
    packet recvd;
    packet ack;
    int bytes_read;
    int seq = -1;
    clock_t lastpkt_time,curr_time;
    // printf("In receive()\n");
    while(1){
            // if no FIN come for a long time and we get last pkt, end receiving
            if ((wait_fin | !tot_packet )&& ((float)(curr_time = clock() - lastpkt_time)/CLOCKS_PER_SEC >= TIMEOUT_FIN)) return;
            // getting syn packet 
            bytes_read = recvfrom(s,&recvd,sizeof(packet), 0,&si_other,&slen);
            if(bytes_read == -1){
                if(errno == EWOULDBLOCK | errno == EAGAIN ) continue;
                perror(recvfrom);
            }else{
                // getting new packet
                if (recvd.msg_type == SYN){
                    if(!get_syn){
                        meta_packet * temp = &recvd;
                        si_other = temp->si;
                        tot_packet = temp->packet_tot;
                        buffer = calloc(tot_packet, sizeof(packet));
                        received = calloc(tot_packet,sizeof(int));
                        current_print = 0;
                        get_syn = 1;
                    }
                    // cover dropped SYN_ACK
                    ack.msg_type = SYN_ACK;
                    sendto(s,&ack,sizeof(packet),MSG_DONTWAIT,&si_other,slen);
                }else if(recvd.msg_type == DATA){
                    seq = recvd.seq_num;
                    // fprintf(r_log,"get a pkt seq = %d\n",seq);
                    // if last packet,and all previous pkt received start timeout timer
                    if(seq == tot_packet -1 && total_acted == tot_packet-1){
                        lastpkt_time = clock();
                        wait_fin = 1;
                    }
                    if(!received[seq]){
                        received[seq] = 1;
                        buffer[seq] = recvd;
                        total_acted ++;
                    }
                    //find largest inorder packet we have ever
                    int i;
                    for(i = inorder + 1;i <tot_packet; i++){
                        if(!received[i]) break;
                    }
                    int temp = inorder;
                    inorder = i - 1;

                    if (received[current_print]) {
                        fwrite(buffer[current_print].data,sizeof(char),buffer[current_print].data_size,fp);
                        current_print += 1;
                    }
                    // printf("inorder updated from %d to %d , i = %d\n",temp,inorder,i);
                    ack.msg_type = ACK;
                    ack.seq_num = inorder; // REVIEW: ACK seq_num is last inorder packet
                    //
                    // fprintf(r_log,"send ack to with seq = %d\n",ack.seq_num);
                    sendto(s,&ack,sizeof(packet),MSG_DONTWAIT,&si_other,slen);
                }else if(recvd.msg_type == FIN){
                    ack.msg_type = FIN_ACK;
                    sendto(s,&ack,sizeof(packet),MSG_DONTWAIT,&si_other,slen);
                    int i;
                    for (i = current_print; i < tot_packet; i++) {
                        fwrite(buffer[i].data,sizeof(char),buffer[i].data_size,fp);
                    }
                    return;
                }
            }
    }
}

/* 
* fileWrite: dump all data field in of received packets into given file
 */
void fileWrite(char * destinationFile){
    int tot_received = 0;
    printf("writtng to file\n");
    fp = fopen(destinationFile,"w");
    if(fp == NULL){
        diep("file open");
        return;
    }
    for(int i = 0;i < tot_packet;++i){
        fwrite(buffer[i].data,sizeof(char),buffer[i].data_size,fp);
        tot_received += buffer[i].data_size;
    }
    fclose(fp);
    printf("in tot %d bytes write to file.", tot_received);
    return;
}

/************************* END Helper Functions *******************************/