#include <stdlib.h>
#include <stdio.h>  
#include <cstring> 
#include <math.h>
#include <errno.h>
#include <arpa/inet.h>  
#include <sys/types.h> 		
#include <sys/socket.h>	
#include <unistd.h>	 
#include <netinet/in.h>	
#include <sys/time.h>

// Included to get the support library
#include <calcLib.h>

#define BUFFER_SIZE 256
#define SERVER_BACKLOG 5
#define SUCCESS 0 

typedef struct client {
  struct sockaddr_in addr;
  char * op;
  int connfd;
  int ival1;
  int ival2;
  int iresult;
  float fval1;
  float fval2;
  float fresult;
} client_t;


int handle_connection(client_t * client);
void generate_assignment(client_t * client);
int send_assignment(client_t * client);
int confirm_result(client_t * client);
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
    * Reuse socket address.
    */
    int enable = 1;    
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(enable)) != SUCCESS){
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }  


    /*
    * Bind: bind socket.
    */
    int status = bind(sockfd, (struct sockaddr * ) &servAddr, sizeof(servAddr));
    if(status < 0){
	    perror("bind()");
        exit(EXIT_FAILURE);
    }


    /*
    * Listen: listen for connections...
    */
    status = listen(sockfd, SERVER_BACKLOG);
    if(status < 0){
	    perror("listen()");
        exit(EXIT_FAILURE);
    }
    printf("Listening for connections...\n");

    // Initialize timeout.
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) != SUCCESS){
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    } 

    // Initialize libs.
    initCalcLib(); 
    initCalcLib_seed(0);

    client_t client;
    socklen_t clientLen = sizeof(client.addr);


    /*
    * Main loop: handle connections...
    */
    while(1)
    {
        memset(&client, 0, sizeof(client_t));

        // Accept connection.
        client.connfd = accept(sockfd, (struct sockaddr *) &client.addr, &clientLen);
        if(client.connfd <= 0) 
        {
            if(errno == EAGAIN)
                continue;      
            else
                perror("accept()");
        }
        else
        {     
            // Handle new connection.
            status = handle_connection(&client);
            if(status == SUCCESS)
            {
                // Generate, send and confirm assignment.
                generate_assignment(&client);
                status = send_assignment(&client);
                if(status == SUCCESS){
                    status = confirm_result(&client);
                }
            }

            // Some error or connection timeout occured.
            // Notify client and move on.
            if(status != SUCCESS){
                printf("[INFO]: client connection timeout, client notified.\n\n");
                send(client.connfd, "ERROR TO\n", sizeof("ERROR TO\n"), 0); 
            }

            // Close connection with client.
            close(client.connfd);
        }


    } // end of loop.


    /*
    * Cleanup.
    */
    close(sockfd);

    return EXIT_SUCCESS;
}





/*
* Method for handling a new connection.
*/
int handle_connection(client_t * client)
{   
    // Send protocol.	
    int status = send(client->connfd, "TEXT TCP 1.0\n\n", sizeof("TEXT TCP 1.0\n\n"), 0);
    if(status <= 0){
        return -1;
	}
    printf("Client %s:%d connected, waiting for confirmation...\n", inet_ntoa(client->addr.sin_addr), client->addr.sin_port);

    char buffer[BUFFER_SIZE];
    memset(&buffer, 0, sizeof(buffer));

    // Recieve ACK from client.
	status = recv(client->connfd, buffer, sizeof(buffer), 0); 
    if(status <= 0){
        return -1;
    }
	printf("%s", buffer);    

    // Everything is OK.
    return 0;
}


/*
* Method for generating a new assignment.
*/
void generate_assignment(client_t * client)
{
    // Get operation-type
    client->op = randomType();
    if(client->op[0] == 'f')
    {
        client->fval1 = randomFloat();
        client->fval2 = randomFloat();
        client->fresult = calculate_fresult(client->op, client->fval1, client->fval2);
    }
    else
    {
        client->ival1 = randomInt();
        client->ival2 = randomInt();
        client->iresult = calculate_iresult(client->op, client->ival1, client->ival2);
    }
}


/*
* Method for sending given assignment to client.
*/
int send_assignment(client_t * client)
{
    char buffer[BUFFER_SIZE];
    memset(&buffer, 0, sizeof(buffer));

    if(client->op[0] == 'f')
        sprintf(buffer, "%s %8.8g %8.8g\n", client->op, client->fval1, client->fval2);
    else
        sprintf(buffer, "%s %i %i\n", client->op, client->ival1, client->ival2);

    int status = send(client->connfd, buffer, strlen(buffer), 0);
    if(status <= 0){
        return -1;
    }

    return 0;
}


/*
* Method for confirming the results given by the client.
*/
int confirm_result(client_t * client)
{
    char buffer[BUFFER_SIZE];
    memset(&buffer, 0, sizeof(buffer));

    int status = recv(client->connfd, buffer, sizeof(buffer), 0);
    if(status <= 0){
        return -1;
    }
    printf("%s\n", buffer); 

    if(client->op[0] == 'f')
    {
        float result_from_server = atof(strtok(buffer, " "));
        memset(&buffer, 0, sizeof(buffer));

        int dDelta = fabs(result_from_server - client->fresult);
		if (dDelta <= 0.0001) 
            sprintf(buffer, "OK\n");
        else 
            sprintf(buffer, "ERROR\n");
    }
    else
    {  
        int result_from_server = atoll(strtok(buffer, " "));
        memset(&buffer, 0, sizeof(buffer));

        if(result_from_server == client->iresult) 
        sprintf(buffer, "OK\n");
        else 
        sprintf(buffer, "ERROR\n");
    }

    status = send(client->connfd, buffer, strlen(buffer), 0);
    if(status <= 0){
        return -1;
    }

    // Everything OK.
    return 0;
}


/*
* Method for calculating results (int).
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
* Method for calculating results (float).
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