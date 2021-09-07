#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	 // how many pending connections queue will hold
#define CHUNK_SIZE 512
// handling client requests where the new_fd is a socket
void handle_request(int new_fd);


void send_back(int sock, char *content) {
	int ret = send(sock, content, strlen(content), 0);
	if (ret < 0) {
		perror("send_back error");
	}
}

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	// check port number
	if (argc != 2) {
		fprintf(stderr, "usage: ./http_server <port_number>");
		exit(1);
	}

	char *PORT = (char *)malloc(strlen(argv[1]) + 1);
	strcpy(PORT, argv[1]);
	
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server %s: waiting for connections...\n", PORT);

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}


		if (!fork()) { // this is the child process
			// close(sockfd); // child doesn't need the listener
			char *client_ip = (char *)malloc(sizeof s);
			inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			client_ip, sizeof s);
			// print ip address
			printf("server: got connection from %s\n", client_ip);
			
			handle_request(new_fd);

			close(new_fd);
			exit(0);
	}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

void handle_request(int new_fd) {
	// read from the client
	char client_request[8096];
	int byte_read = recv(new_fd, client_request, 8096, 0);

	client_request[byte_read] = '\0';
	
	printf("Received request: \n%s", client_request);
	
	char *data;
	char request_type [32];
	char path[256]; 
	data = strtok(client_request, " ");
	strcpy(request_type, data);
	printf("request type: %s\n", request_type);
	data = strtok(NULL, " ");
	strcpy(path, data);

	char buffer[1024];
	if (!strncmp(request_type, "GET", 3) || !strncmp(request_type, "get", 3)) {
		
		fprintf(stderr, "path: %s\n", path);
		// if path exists
		char currentPath [512];
		if (getcwd(currentPath, sizeof(currentPath)) == NULL) {
			perror("getcwd() error");
		}
		char finalPath [1024];
		snprintf(finalPath, 1024, "%s%s", currentPath, path);
		FILE * fp = fopen(finalPath, "r");
		// get file size
		
		if (fp) {
			fseek(fp, 0, SEEK_END);
			size_t size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			send_back(new_fd, (char *)"HTTP/1.0 200 OK\r\n");
			send_back(new_fd, (char *)"Content-Type: text/html\r\n");
			char contentLength[100];
			snprintf(contentLength, 100, "Content-Length: %ld\r\n\r\n", size); 
			send_back(new_fd, contentLength);
			fprintf(stderr, "%s\n", contentLength);
			//write file content back to the socket
			fprintf(stderr, "%s: 200\n", path);
			// sending file data back, error free for binary files
			// referenced from stackoverflow.com/questions/5594042/c-send-file-to-socket
			char file_data[CHUNK_SIZE];
			size_t nbytes = 0;
			size_t total_bytes = 0;
			while ((nbytes = fread(file_data, sizeof(char), CHUNK_SIZE, fp)) > 0){
				int offset = 0;
				int sent = 0;
				while ((sent = send(new_fd, file_data + offset, nbytes, 0)) > 0 || (sent == -1 && errno == EINTR)){
					total_bytes += sent;
					if (sent >0) {
					offset += sent;
					nbytes -= sent;
					}
				}
			}
			printf("%ld\n", total_bytes);
		        fclose(fp);	
		
		} else{
			// if not: returns 404
			fprintf(stderr, "%s: 404\n", path); 
			send_back(new_fd, (char *)"HTTP/1.0 404 Not Found\r\n");
		}

	} else {
		// only supports GET request, return 400 if not
		send_back(new_fd, (char *)"HTTP/1.0 400 Bad Request\r\n");

	}
	
	return;

}
