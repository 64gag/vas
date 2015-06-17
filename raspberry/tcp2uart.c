#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
	int i = 0;

	/* UART initialization */
	int uart0_filestream = -1;
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);

	if (uart0_filestream == -1){
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}

	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	options.c_iflag = 0;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);

	/* TCP initialization*/
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0){
		error("ERROR opening socket");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		error("ERROR on binding");
	}

	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

	if (newsockfd < 0){
		error("ERROR on accept");
	}

	bzero(buffer,256);

	if (uart0_filestream != -1)
	while(i++ < 50){
     		n = read(newsockfd,buffer,255);

		if (n < 0){
			error("ERROR reading from socket");
		}

		int count = write(uart0_filestream, (unsigned char *)(&buffer[0]), 3);

		if (count < 0) {
			printf("UART TX error\n");
		}
	}

 /* Close all */
 close(uart0_filestream);
 close(newsockfd);
 close(sockfd);

 return 0;
}
