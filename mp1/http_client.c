/*
** http_client.c: simple http client able to retrive packet from given address
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define BUF_SIZE  100

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void send_header(int sockfd, char* page, char* host) {
	char header[1024];
	sprintf(header, "GET %s HTTP/1.0\r\n", page);
	send(sockfd, header, strlen(header), 0);
	sprintf(header, "Host: %s\r\n\r\n", host);
	send(sockfd, header, strlen(header), 0);
}
// read from socket until new line
int read_line(int sockfd, char * buf, int maxLen) {
    int ret = 0;
    char temp;
    int count = 0;
    while ((ret = read(sockfd, &temp, 1) == 1)) {
        *buf++ = temp;
        count++;
        if (temp == '\n') {
            break;
        }
    }
    if (ret == 0 && count == 0) {
        return 0;
    }
    if (ret != 0 && ret != 1) {
        return -1;
    }
    *buf = '\0';
    return count;
}


int main(int argc, char *argv[])
{
    char port[5] = "80"; // port to connect,default to 80
    int parse; // 1 = success, 0 =fail 
    int have_page = 0;

    char host[100];
    char page[200];
    
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    int sockfd;
    char s[INET6_ADDRSTRLEN];

    int bytes_sent = 0;
    int total_bytes_sent = 0;
    int bytes_to_send = 0;
    char msg[BUF_SIZE+1];

    int bytes_received = 0;
    char data[BUF_SIZE];

    if (argc != 2) {
	    fprintf(stderr,"uage: http <url>\n");
	    exit(1);
	}

    memset(host, 0, 100);
    memset(page, 0, 200);
    page[0] = '/';
	
    int portTemp = 80;
    // parse url,borrowed from https://github.com/luismartingil/scripts/blob/master/c_parse_http_url/parse_http_uri.c
    if (sscanf(argv[1], "http://%99[^:]:%i/%199[^\n]", host, &portTemp, &page[1]) == 3) { parse = 1; have_page = 1;} 
      else if (sscanf(argv[1], "http://%99[^/]/%199[^\n]", host, &page[1]) == 2) { parse = 1; have_page = 1;}
      else if (sscanf(argv[1], "http://%99[^:]:%i[^\n]", host, &portTemp) == 2) { parse = 1;}
      else if (sscanf(argv[1], "http://%99[^\n]", host) == 1) { parse = 1;}

    if(!parse){
        fprintf(stderr,"failed to parse arguement \n");
        exit(1);
    }

    if (portTemp) {
    	sprintf(port, "%d", portTemp);
    }

    printf("parsed info; host:%s\n, page:%s\n, port:%s\n", host, page, port);

    // prepare serverinfo datastructure
    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(host,port,&hints,&servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}

    if((sockfd = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol)) == -1){
        perror("client: socket");
        return 1;
    }

    if ((connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
        close(sockfd);
        perror("client: connect");
        return 1;
	}
    if (servinfo == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(servinfo->ai_family, get_in_addr((struct sockaddr *)servinfo->ai_addr),
			s, sizeof s);
	
	freeaddrinfo(servinfo);

	printf("client: connecting to %s\n", s);
       
	send_header(sockfd, page, host);
    
    // filtering headers 
    size_t size;
    while((bytes_received = read_line(sockfd, data, BUF_SIZE))){
       fprintf(stderr, "%d\n", bytes_received);
       if (bytes_received == -1 ){
            perror("Readline Error");
            exit(1);
        } else if (bytes_received == 2 && !strncmp(data, "\r\n", 2)){
            printf("Filterd header\n");
            break;
        } else if (!strncmp(data, "Content-Length: ", 16)) {
	    char * size_string = data + 16;
	    sscanf(size_string, "%zu", &size);
	    printf("%s\n", size_string);
	}
    }
    // char output_content [2048];
    // int total_bytes_received = 0;
    // while((bytes_received = recv(sockfd, output_content + total_bytes_received, 2048, 0))){
    //     fprintf(stderr, "receiving content..., bytes_received = %d\n", bytes_received);
    //     if (bytes_received == -1 ){
    //         perror("recv");
    //         exit(1);
    //     } 
    //     total_bytes_received += bytes_received;
    // }
    // printf("Finished retrieving file content!");
    // output_content[total_bytes_received] = '\0';

    // int fd = open("output", O_RDWR | O_CREAT | O_TRUNC, 0666);
    
    FILE * fp = fopen("output", "w");
    // fputs(output_content,fp);
    char file_data[size];
    size_t nbytes = 0;
    size_t total_bytes = 0;
    printf("what the fuck %zu\n", size);
    while (total_bytes < size){
	    nbytes = read(sockfd, file_data, size);
	    size_t write_bytes = fwrite(file_data, 1, nbytes, fp);
	    if (nbytes > 0) {
	    	total_bytes += nbytes;
	    }
    }
    printf("%zu\n", total_bytes);
   
    close(sockfd);
    return 0;
}
