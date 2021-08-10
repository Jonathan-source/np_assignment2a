#include <stdlib.h>
#include <stdio.h>   
#include <cstring>  
#include <arpa/inet.h>  
#include <sys/types.h> 		
#include <sys/socket.h>	
#include <unistd.h>	 
#include <netinet/in.h>	

/* You will to add includes here */
#define BUFFER_SIZE 256

// Included to get the support library
#include "calcLib.h"

int calculate_iresult(char * op, int ival1, int ival2);
float calculate_fresult(char * op, float fval1, float fval2);


//================================================
// Entry point of the client application.
//================================================
int main(int argc, char * argv[])
{   
    /*
    * Check command line arguments. 
    */
    if(argc != 2) 
	{
		printf("Syntax: %s <IP>:<PORT>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

    char delim[] = ":";
    char * pHost = strtok(argv[1], delim);
    char * pPort = strtok(NULL, delim);
    int iPort = atoi(pPort);
    printf("Host %s, and port %d.\n", pHost, iPort);


    /* 
    * Socket: create the parent TCP socket. 
    */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
	    perror("socket()");
        exit(EXIT_FAILURE);
    } 


    /*
    * Fill in the server's address and data.
    */
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_addr.s_addr = inet_addr(pHost);
    servAddr.sin_port = htons(iPort);
    servAddr.sin_family = AF_INET;


    /* 
    * Connect: connect to remote server
    */    
    int status = connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr));
    if(status < 0) {
	    perror("connect()");
        exit(EXIT_FAILURE);
    }


    /* 
    * Recv: recieve protocol from server.
    */
    char buffer[BUFFER_SIZE];
    memset(&buffer, 0, sizeof(buffer));
    status = recv(sockfd, buffer, sizeof(buffer), 0); 
    if(status < 0) {
	    perror("recv()");
        exit(EXIT_FAILURE);
    } 
    printf("%s", buffer);


    /* 
    * Send: reply to server that protocol is accepted.
    */
    status = send(sockfd, "OK\n", 3, 0);
    if(status < 0) {
	    perror("send()");
        exit(EXIT_FAILURE);
    } 


    /* 
    * Send: reply to server that protocol is accepted.
    */
    memset(&buffer, 0, sizeof(buffer));
    status = recv(sockfd, buffer, sizeof(buffer), 0);
    if(status < 0) {
	    perror("recv()");
        exit(EXIT_FAILURE);
    } 

    char type;
    char * op;
    int ival1, ival2, iresult;
    float fval1, fval2, fresult;

    // Parse and calculate assignemnt.
    if(buffer[0] == 'f')
    {
        type = 'f';
        op = strtok(buffer, " ");
        fval1 = atof(strtok(NULL, " "));
        fval2 = atof(strtok(NULL, " "));
        printf("%s %8.8g %8.8g\n", op, fval1, fval2);
        fresult = calculate_fresult(op, fval1, fval2);
    } 
    else 
    {
        op = strtok(buffer, " ");
        ival1 = atoll(strtok(NULL, " "));
        ival2 = atoll(strtok(NULL, " "));
        printf("%s %i %i\n", op, ival1, ival2);
        iresult = calculate_iresult(op, ival1, ival2);
    }
  

    /* 
    * Send: send result to server.
    */
    memset(&buffer, 0, sizeof(buffer));
    if(type == 'f') 
        sprintf(buffer, "%8.8g\n", fresult);
    else
        sprintf(buffer, "%i\n", iresult);

    status = send(sockfd, buffer, sizeof(buffer), 0);
    if(status < 0) {
	    perror("send()");
        exit(EXIT_FAILURE);
    } 


    /* 
    * Recv: recieve answer from server.
    */
    memset(&buffer, 0, sizeof(buffer));
    status = recv(sockfd, buffer, sizeof(buffer), 0);
    if(status < 0) {
	    perror("recv()");
        exit(EXIT_FAILURE);
    } 
    printf("%s", buffer);


    /* 
    * Cleanup.
    */
    close(sockfd);

    return EXIT_SUCCESS;
}


/* 
* Method for calculating the result (int).
*/
int calculate_iresult(char * op, int ival1, int ival2)
{
  int result;
  switch(op[0]) 
  {
    case 'a':
      result = ival1 + ival2; 
    break;
    case 'd':
      result = ival1 / ival2;
    break;
    case 'm':
      result = ival1 * ival2;
    break;
    case 's':
      result = ival1 - ival2;
    break;
  }
  return result;
}


/* 
* Method for calculating the result (float).
*/
float calculate_fresult(char * op, float fval1, float fval2)
{
  float result;
  switch(op[1])
  {
    case 'a':
      result = fval1 + fval2;
    break;
    case 'd':
      result = fval1 / fval2;
    break;
    case 'm':
      result = fval1 * fval2;
    break;
    case 's':
      result = fval1 - fval2;
    break;
  } 
  return result;
}