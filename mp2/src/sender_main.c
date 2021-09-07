/* 
 * File:   sender_main.c
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <netdb.h>
#include <fcntl.h>

#define SYN 0
#define SYN_ACK 3
#define ACK 1
#define FIN 2
#define FIN_ACK 4
#define DATA 5

#define DATA_MAX 10000  // size of data field in a packet
#define SEQ_MAX 10000 // max sequence number 
#define TIMEOUT 0.1 // timeout time in sec
#define TIMEOUT_FIN 10
#define PORT "8888" //REVIEW 


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

FILE *fp,*fp_debug;
// socket info variables si_this is the socket info of sender
struct sockaddr_in si_other;
struct sockaddr *si_me; //REVIEW: type of si_me
struct addrinfo hints;
struct addrinfo * serverinfo;
int s, slen;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct addrinfo hints, *servinfo, *p;
int yes=1;

// server properties
// WAIT_SYN: SYN packet has been sent, waitting for an ACK to this packet
enum senderStates {SLOW_START,CONGESTION_AVOIDANCE, FAST_RECOVERY};
int dupAck_cnt; // dupAck count across the board
int senderState = SLOW_START; // current state of transmitter 
float SST = 64.0; // sender threashold 
float CW = 1.0; // sender window size

// global viarbles regarding contents in the windows
int newest_ACK = -1; // last packet we have received ack on
int *received_ack; // an array showing if <seq_number>(index) has been acked
int leftIndex = 0;
int rightIndex = 1; // [leftIndex, floor(rightIndex)] is the current window, inclusive,
int send_next = 0; // point to next packet should be sent, should increment universally or be start of window ( start and after packet drop)

int tot_pkt; // total number of packets
packet * packets; // all packets build at this pointer before sending 
clock_t * times; // time[i] is the time which packet with seq_numer i is sent 

// functions
void openFile(char* filename);
void buildPackets(size_t bytes);
void grabSocket(char * hostname, unsigned short int hostUDPport);
void sendSYN();
void changeState(int ack,bool timeout);
void sendPacket();
void monitor();
void sendFIN();



void diep(char *s) {
    perror(s);
    exit(1);
}


void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    // grab receiver's info
    grabSocket(hostname,hostUDPport);
    //Open the file
    openFile(filename);
    // build a buffer with packets to send
    buildPackets(bytesToTransfer);
    sendSYN();
    if(!tot_pkt) return;
    sendPacket();
    // monitor and act correspondingly
    monitor();
    printf("ready to sendFIN\n");
    sendFIN();
    free(packets);
    free(times);
    free(received_ack);
    printf("Closing the socket\n");
    close(s);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);


    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}




/************************* Helper Functions *******************************/


/* 
* grabSocket: grab a socket, bind it to a server port,socket's file descriptor is in global var s
 */
void grabSocket(char * hostname, unsigned short int hostUDPport) {
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");
    fcntl(s, F_SETFL, O_NONBLOCK); // set socket to be non-blocking
    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    slen = sizeof (si_other);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    if ((getaddrinfo(NULL, PORT, &hints, &serverinfo)!= 0 )) diep("getaddrinfo sender");
    si_me = serverinfo->ai_addr;
    freeaddrinfo(serverinfo);
    printf("done grabsocket\n");
}

/* 
* openFile: openFile specified by filename, file descriptor is in global variable fp
 */
void openFile(char* filename){
    printf("open file\n");
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }
    // fp_debug = fopen("buglog.txt", "w");
}

/**
 * Build packets according to the file opened
 */
void buildPackets(size_t bytes) {
    printf("building packets\n");
    if (!fp) {
        perror("Could not access the file descriptor");
        exit(1);
    } 
    //get the size of the file
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    size = bytes < size ? bytes : size;
    // send a packet of around 1kb each time
    int packet_nums = size / DATA_MAX;
    bool is_multiple = true;
    if (size % DATA_MAX) {
        is_multiple = false;
        packet_nums++;
    }
    int bytes_read;
    // typedef struct{
    //     int     data_size;
    //     int     seq_num;
    //     int     msg_type;  
    //     char    data[DATA_MAX];
    // } packet;
    tot_pkt = packet_nums;
    printf("tot_pkt = %i\n",tot_pkt);
    packets = calloc(packet_nums, sizeof(packet));
    times = calloc(packet_nums,sizeof(clock_t));
    for (int i = 0; i < packet_nums; i++) {
        if (!is_multiple && i == packet_nums - 1) {
            bytes_read = size % DATA_MAX;
        } else {
            bytes_read = DATA_MAX;
        }
        packets[i].seq_num = i;
        packets[i].data_size = bytes_read;
        // fseek(fp, i + DATA_MAX * i, SEEK_SET);
        fread(packets[i].data, sizeof(char), bytes_read, fp);
        packets[i].msg_type = DATA;
    }
    received_ack = calloc((tot_pkt + 1),sizeof(int));
    printf("done building packets\n");
}

/* 
* timeOut: actions acted upon sender window when a timeout happen
* REVIEW: timeout always happen on packet[leftIndex]
 */
void timeOut(){
    // change sender parameters
    SST= CW/2;
    CW = 1;
    dupAck_cnt = 0;
    // change window
    rightIndex = leftIndex + 1 > tot_pkt? tot_pkt:leftIndex + 1;
    send_next = leftIndex;
    // fprintf(fp_debug,"in timeOut(), change leftIdx to %d,rightIdx to %d,send_next to %d\n",leftIndex,rightIndex,send_next);
    // fprintf(fp_debug,"timeout,CW = %f,SST =%f \n",CW,SST);
    return;
}

/* 
* dupAck: actions acted upon sender window when a dupack happen
*   REVIEW : dupAck always on packet[leftIndex]
 */
void dupAck(){
    // change sender parameters
     SST = CW/2;
     CW = CW +3.0;
     dupAck_cnt = 0;
    // change window
    rightIndex = leftIndex + (int)CW > tot_pkt? tot_pkt:leftIndex + (int)CW ; // REVIEW: ment to use floor
    send_next = leftIndex;
    // fprintf(fp_debug,"in dup ack, change leftIdx to %d,rightIdx to %d\n",leftIndex,rightIndex);
    return;
}

/* 
* newAck: actions on window (except CW) acted upon sender window when newAck come 
 */
void newAck(int ack){
    int i;
    newest_ACK = ack;
    // printf("SST: %d\n", SST);

    dupAck_cnt = 0;
    for(i = leftIndex;i <= ack;i++){
        // mark everything up to ack as acted
        received_ack[i] = 1;
    }
    leftIndex = ack + 1; // remove all consecutive acted packet from left of window
    rightIndex = leftIndex + (int)CW > tot_pkt? tot_pkt:leftIndex + (int)CW;
    // fprintf(fp_debug,"in newACK,act: %d,leftIndex : %d;rightidx : %d,CW : %f\n",ack,leftIndex,rightIndex,CW);
}

/* 
* changeState: manage tcp statemachine acoording to msg received and current state
 */
void changeState(int ack,bool timeout){
switch (senderState) {
        case SLOW_START:
            if (timeout) {
                timeOut();
                break;
            }
            else if (ack > newest_ACK) {
                // printf("SLOW_START NEW ACK!\n");
                CW = CW + 1;
                newAck(ack);
                if (CW >= SST) {
                    // fprintf(fp_debug,"SLOW_START -> CONGESTION_AVOIDANCE, current window size = %f\n",CW) ;
                    senderState = CONGESTION_AVOIDANCE;
                    break;
                }
            } else if (ack == newest_ACK) {
                dupAck_cnt++;
                if(dupAck_cnt == 3){
                    dupAck();
                    // fprintf(fp_debug,"SLOW_START -> FAST_RECOVERY, current window size = %f\n",CW) ;
                    senderState = FAST_RECOVERY;
                }
                break;
            }
        case CONGESTION_AVOIDANCE:
            if (timeout) {
                timeOut();
                // fprintf(fp_debug,"CONGESTION_AVOIDANCE -> SLOW_START, current window size = %f\n",CW);
                senderState = SLOW_START;
                break;
            }
            if (ack > newest_ACK) {
                CW = CW + 1.0/(int)CW;
                newAck(ack);
            } else if (ack == newest_ACK) {
                dupAck_cnt++;
                if(dupAck_cnt == 3){
                    dupAck();
                    // fprintf(fp_debug,"CONGESTION_AVOIDENCE -> FAST_RECOVERY, current window size = %f\n",CW) ;
                    senderState = FAST_RECOVERY;
                }
            }
            break;
        case FAST_RECOVERY:
            if (timeout) {
                timeOut();
                // fprintf(fp_debug,"FAST_RECOVERY -> SLOW_START, current window size = %f\n",CW);
                senderState = SLOW_START;
                break;
            }
            if (ack > newest_ACK) {
                CW = SST;
                newAck(ack);
                // fprintf(fp_debug,"FAST_RECOVERY -> CONGESTION_AVOIDANCE, current window size = %f\n",CW);
                senderState = CONGESTION_AVOIDANCE;
            } else {
                CW = CW + 1;
            }
            break;
        default:
            break;
    }
}

/* 
* sendSYN: send a SYN message to receiver and give receiver my info, return until we see a SYN_ACK; resend SYN if timeout
 */
void sendSYN(){
    int bytes_read;
    packet recvd;
    meta_packet *syn_pkt;
    syn_pkt = malloc(sizeof(meta_packet));
    clock_t syn_time,curr_time;

    syn_pkt -> msg_type = SYN;
    syn_pkt -> packet_tot = tot_pkt;
    syn_pkt -> si = *((struct sockaddr_in *)si_me); //REVIEW: Cursed casting

    // struct timeval r_timeout;
    // r_timeout.tv_sec = TIMEOUT;
    // r_timeout.tv_usec = 0;
    // setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &r_timeout, sizeof r_timeout);
    sendto(s,syn_pkt,sizeof(meta_packet),0,(struct sockaddr*)&si_other,slen);
    syn_time = clock();
    // printf("sending SYN packet \n");
     while(1){
        if ((float)((curr_time = clock()) - syn_time)/CLOCKS_PER_SEC >= TIMEOUT){
            // printf("TIMEOUT, re-sending SYN packet\n");
            sendto(s,syn_pkt,sizeof(meta_packet),0,(struct sockaddr*)&si_other,slen);
            syn_time = clock();
            continue;
        }
        if (!(bytes_read = recvfrom(s,&recvd,sizeof(packet),0,&si_other,&slen))) {
            //  printf("nothing received\n");
            continue;
        }else{
            if(recvd.msg_type == SYN_ACK){
                // printf("Get SYN_ACK\n");
                free(syn_pkt);
                return; 
            }
        }
    }
}

/* 
* sendFIN: send FIN and wait for FIN_ACK, return until we confirm a FIN_ACK; resend FIN if timeout
 */
void sendFIN(){
    int bytes_read;
    packet recvd;
    meta_packet *fin_pkt;
    fin_pkt = malloc(sizeof(meta_packet));
    clock_t fin_time,curr_time;

    fin_pkt -> msg_type = FIN;

    sendto(s,fin_pkt,sizeof(meta_packet),MSG_DONTWAIT,(struct sockaddr*)&si_other,slen);
    fin_time = clock();
    while(1){
        if ((float)(curr_time = clock() - fin_time)/CLOCKS_PER_SEC >= TIMEOUT){
            sendto(s,fin_pkt,sizeof(meta_packet),MSG_DONTWAIT,(struct sockaddr*)&si_other,slen);
            fin_time = clock();
            continue;
        }else if((float)(curr_time = clock() - fin_time)/CLOCKS_PER_SEC >= TIMEOUT_FIN){
            //haven't heard from receiver for an extended period of time, should be able to safely terminate
            // printf("senFIN: timeout returnning\n");
            free(fin_pkt);
            return;
        }
         if (!(bytes_read = recvfrom(s,&recvd,sizeof(packet), 0,&si_other,&slen))) {
            continue;
        }else{
            if(recvd.msg_type == FIN_ACK) {
                // printf("senFIN: get FIN_ACK returnning\n");
                free(fin_pkt);
                return;
            } 
        }
    }
}

/* 
* sendPacket: send new packets according to current window
 */
void sendPacket(){
    int i = 0;
    // pre-send management
    // send packet in udp style 
    // if(rightIndex) fprintf(fp_debug,"in send pkt, leftIdex = %d,rightIdx = %d,i=%d,ack[i] = %d\n",leftIndex,rightIndex,send_next,received_ack[send_next]);
    for(i = send_next;i < rightIndex;++i){
        if(received_ack[i] != 1){
            times[i] = clock(); // record time which packet i is sent 
            // fprintf(fp_debug,"sending pkt %d\n",i);
            sendto(s,&packets[i],sizeof(packet),MSG_DONTWAIT,(struct sockaddr*)&si_other,slen);
        }   
    }
    // update datastructures
    send_next = i; 
}

/* monitor: running on a separate thread: listening to acks <- can be our main func 
 * for every ack and timeout, do the following:
 *  1: change state
 *  2: change sender's window 
 *  3: send packets allowed by the window
*/
void monitor() {
    // int ACK_MAX = 16;
    // printf("in monitor()\n");
    while(1) {
        // char buffer[ACK_MAX];
        int bytes_read;
        packet recvd;
        int seq_num;
        clock_t curr_time;

        //catch socket timeout 
        if ((float)(curr_time = clock() - times[leftIndex])/CLOCKS_PER_SEC >= TIMEOUT){
            // printf("monitor:timeout\n");
            changeState(-1,true);
            sendPacket(); 
        }
        // if no packet received, keep going
        if (!(bytes_read = recvfrom(s,&recvd,sizeof(packet),0,&si_other,&slen))) {
            continue;
        }else{  
        // deal with new packet
            // get seq_num from data
            if(bytes_read >0 && recvd.msg_type == ACK) {
                seq_num = recvd.seq_num;
                if (seq_num > leftIndex) {
                    leftIndex = seq_num;
                }
                // if last ACK, return
                if(seq_num == tot_pkt - 1) return;
                // check seq_num valid, if out of sender's widow, abondon it 
                if(!(seq_num <= rightIndex && seq_num >= leftIndex)) continue;

                changeState(seq_num, false); 
                sendPacket();
            }
        }
    }
}   

/************************* END Helper Functions *******************************/