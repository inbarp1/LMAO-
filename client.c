#include "networking.h"


/*=========================
  client
  args: char * serverIP
  connects to server at serverIP and serves as user interface,
  handling client requests, and pushing from/pulling into client side files
  =========================*/
void client(char * serverIP){
  char buffer[BUFFER_SIZE];//handles user responses
  char ans[BUFFER_SIZE];//handles user responses(similar to buffer)
  char file[BUFFER_SIZE];//file name
  char filePath[BUFFER_SIZE];//path to file
  char fileContent[PACKET_SIZE];//up to 32KB for transferring file data between client/server

  //connects to server
  int server_socket = client_setup(serverIP);
  while (1) {
    printf("\nWould you like to push or pull (a file), \nview available files in the FTP, add collaborators, or exit? \n(push/pull/view/share/exit): ");
    fgets(buffer, sizeof(buffer), stdin);
    *strchr(buffer, '\n') = 0;
    /*-----------------
            push request
            Asks user for a file to push into in database(doesn't have to exist yet)
            Asks user for a path to local file to push contents from(has to exist with contents inside)
	    -----------------*/
    if(!strcmp("push",buffer)){ //push file code
      //sending push request
      write(server_socket, "PUSH", sizeof("PUSH")); //push request sent
      wait_response("1", server_socket);

      //sending file name
      printf("\nWhat is the name of the file you are pushing into?(if it doesnt exist yet one will be created): ");
      fgets(file, sizeof(file), stdin);
      *strchr(file, '\n') = 0;
      write(server_socket, file, sizeof(file)); //file name sent

      if(!wait_response("2", server_socket)){//wait for confirmation to send file contents
	//file transfer
	printf("\nWhat is the path to this file?(must point to existing file on your computer)\n");
	fgets(filePath, sizeof(filePath), stdin);
	*strchr(filePath, '\n') = 0;
	//accessing file contents
	int fd;
	if ((fd = open(filePath, O_RDONLY)) < 0) //checks if file exists
	  handle_error();
	memset(fileContent,0,sizeof(fileContent));
	read(fd, fileContent, sizeof(fileContent));
	close(fd);
	//sending file contents up to NULL
	write(server_socket, fileContent, num_non_null_bytes(fileContent));
	printf("Pushed from '%s' to '%s'\n", filePath, file);
      }
    }
    /*-----------------
            pull request
            Asks user for a file to pull
            Asks user for a path to local file to pull contents(doesn't have to exist yet)
	    -----------------*/
    else if(!strcmp("pull",buffer)){//pull file code
      //sending pull request
      write(server_socket, "PULL", sizeof("PULL")); //pull request sent
      wait_response("1", server_socket);

      //sending file name
      printf("\nWhat is the name of the file you are pulling?: ");
      fgets(file, sizeof(file), stdin);
      *strchr(file, '\n') = 0;
      write(server_socket, file, sizeof(file)); //file name sent

      if(!wait_response("2", server_socket)){//wait for confirmation to send file contents
	//receiving file contents
	memset(fileContent,0,sizeof(fileContent));
	write(server_socket, "3", sizeof("3"));//responds with a ready to read signal
	read(server_socket, fileContent, sizeof(fileContent));
	//storing file contents in client-side file
	printf("\nWhere would you like the file contents to be pulled?(creates new file if one doesnt exist)\n(enter a path to file): ");
	fgets(filePath, sizeof(filePath), stdin);
	*strchr(filePath, '\n') = 0;
	int fd = open(filePath, O_CREAT|O_WRONLY|O_TRUNC, 0664);
	//writing into fd up to NULL
	write(fd, fileContent, num_non_null_bytes(fileContent));
	close(fd);

	printf("Pulled from '%s' to '%s'\n", file,filePath);
      }
    }
    /*-----------------
            exit request
            terminates client program
	    -----------------*/
    else if(!strcmp("exit",buffer)) {
      printf("Thank you for using FTP. Goodbye\n");
      close(server_socket);
      exit(0);
    }
    else{
      printf("Please type in 'push', 'pull', 'view', 'share', or 'exit'.\n");
    }
  }
}

/*=========================
  client_setup
  args: int * to_server
  to_server is a string representing the server address
  create and connect a socket to a server socket that is
  in the listening state
  returns the file descriptor for the socket
  =========================*/
int client_setup(char * server) {
  sleep(1);
  int sd, i;

  //create the socket
  sd = socket( AF_INET, SOCK_STREAM, 0 );
  error_check( sd, "client socket" );

  //run getaddrinfo
  /* hints->ai_flags not needed because the client
     specifies the desired address. */
  struct addrinfo * hints, * results;
  hints = (struct addrinfo *)calloc(1, sizeof(struct addrinfo));
  hints->ai_family = AF_INET;  //IPv4
  hints->ai_socktype = SOCK_STREAM;  //TCP socket
  getaddrinfo(server, PORT, hints, &results);

  //connect to the server
  //connect will bind the socket for us
  i = connect( sd, results->ai_addr, results->ai_addrlen );
  error_check( i, "client connect" );

  free(hints);
  freeaddrinfo(results);

  return sd;
}

/*=========================
  error_check
  args: int i, char* s
  Prints error in errno with s if i is less than 0
  =========================*/
void error_check( int i, char *s ) {
  if ( i < 0 ) {
    printf("[%s] error %d: %s\n", s, errno, strerror(errno) );
    exit(1);
  }
}



/*=========================
  wait_response
  args: char * message, int server_socket
  Waits for message from server_socket as confirmation
  If ERROR_RESPONSE received, prints out error message
  Returns -1 if receives error and 0 if receives confirmation
  =========================*/
int wait_response(char * message, int server_socket){
  char buffer[BUFFER_SIZE];
  while(strcmp(buffer,message)) {
    read(server_socket, buffer, sizeof(buffer));

    //if it gets error message instead of confirmation
    if(strstr(buffer,ERROR_RESPONSE)) {
      write(server_socket, ERROR_WAIT, sizeof(ERROR_WAIT));
      read(server_socket, buffer, sizeof(buffer)); //reading follow up error message
      printf("%s",buffer);
      return -1;
    }
  }
  return 0;
}

/*=========================
  num_non_null_bytes
  args: char* s
  Counts how much of s is non null up to the first null byte
  Returns number of non null bytes in s
  =========================*/
int num_non_null_bytes(char* s){
  char * nul_pos = strchr(s,'\0');
  //checks if NULL is actually in the string
  if(nul_pos != NULL)
    return sizeof(char)* (int)(nul_pos - s);
  else
    return sizeof(char)*PACKET_SIZE;
}
