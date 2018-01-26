#include "networking.h"

void subserver(int from_client);
void print_packet(char *s);


/*=========================
  handle_error
  args:
  Prints error in errno if there is one
  =========================*/
void handle_error(){
  printf("Error: %s\n", strerror(errno));
  exit(1);
}
/*=========================
  print_packet
  args: s
  Prints packet(s) that server recieved(not used with uppercase requests)
  =========================*/
void print_packet(char *s){
  printf("[Server]: received [%s]\n", s);
}

/*=========================
  forking_server
  args:
  starts up forking server and forks off subserver whenever a client connects
  =========================*/
int forking_server() {

  int listen_socket;
  int f;
  listen_socket = server_setup();

  while (1) {

    int client_socket = server_connect(listen_socket);
    f = fork();
    if (f == 0)
      subserver(client_socket);
  }
}

/*=========================
  subserver
  args: int client_socket
  subserver code for handling client through client_socket
  deals with client requests, account, permission, and file handling
  =========================*/
void subserver(int client_socket) {
  char temp_buffer[BUFFER_SIZE];
  char buffer[BUFFER_SIZE];
  char file[BUFFER_SIZE];//file name
  char filePath[BUFFER_SIZE];//file path
  char fileContent[PACKET_SIZE];//transfers up to 32KB of file content
  char *init_file_pos; //pointer to where a file permissions begin


  while (read(client_socket, buffer, sizeof(buffer))) {
    /*-----------------
        PUSH request
        Checks if requested file exists in database
        If file hasn't been created, then create one and append permissions for owner into push_perm.txt and pull_perm.txt
        (and creates a semaphore KEY for that file, setting its value to 1)
        If file already exists verify users push permissions and sets semaphore to 1 if all checks out
        If push request is granted than send confirmation to client and read file data from
          client_socket into file in database
	  -----------------*/
    if(!strcmp(buffer, "PUSH")){//dealing with push request
      write(client_socket, "1", sizeof("1")); //responds to client

      read(client_socket, file, sizeof(file)); //receives file name
      print_packet(file);

      //file transfer code
      strcpy(filePath, "./fileDir/");
      strcat(filePath,file);
      strcat(file, ";"); //adds ; to the end of file name
      int fd;//of file being pushed into
      fd = open(filePath, O_CREAT|O_EXCL|O_WRONLY, 0664);
      write(client_socket,"2",sizeof("2"));//confirms push access
      //receiving file contents
      memset(fileContent,0,sizeof(fileContent));
      read(client_socket, fileContent, sizeof(fileContent));
      print_packet(fileContent);
      //writing into fd up to NULL
      write(fd, fileContent, num_non_null_bytes(fileContent));
      close(fd);
      printf("[Server]: pushed to '%s'\n", file);
    }
 
    /*-----------------
        PULL request
        Checks if requested file exists
        Checks if client has pull permissions to file(downs file's semaphore by 1 if all checks out)
        If pull request is granted than send confirmation to client and send file data to client_socket
	-----------------*/
  else if(!strcmp(buffer,"PULL")){ //dealing with a pull request
      write(client_socket, "1", sizeof("1")); //responds to client

      read(client_socket, file, sizeof(file)); //receives file name
      print_packet(file);

      strcpy(filePath, "./fileDir/");
      strcat(filePath,file);
      strcat(file, ";"); //adds ; to the end of file name

      write(client_socket,"2",sizeof("2"));
      //accessing file contents
      int fd;
      fd = open(filePath, O_CREAT|O_EXCL|O_WRONLY, 0664);
      memset(fileContent,0,sizeof(fileContent));
      read(fd, fileContent, sizeof(fileContent));
      //sending file contents up to NULL
      wait_response("3", client_socket);
      write(client_socket, fileContent, num_non_null_bytes(fileContent));
      close(fd);
      printf("[Server]: pulled from '%s'\n", file);
  } 	   
}//end read loop
  close(client_socket);
  exit(0);
}


/*=========================
  server_setup
  args:
  creates, binds a server side socket
  and sets it to the listening state
  returns the socket descriptor
  =========================*/
int server_setup() {
  int sd, i;

  //create the socket
  sd = socket( AF_INET, SOCK_STREAM, 0 );
  error_check( sd, "server socket" );
  printf("[server] socket created\n");

  //setup structs for getaddrinfo
  struct addrinfo * hints, * results;
  hints = (struct addrinfo *)calloc(1, sizeof(struct addrinfo));
  hints->ai_family = AF_INET;  //IPv4 address
  hints->ai_socktype = SOCK_STREAM;  //TCP socket
  hints->ai_flags = AI_PASSIVE;  //Use all valid addresses
  getaddrinfo(NULL, PORT, hints, &results); //NULL means use local address

  //bind the socket to address and port
  i = bind( sd, results->ai_addr, results->ai_addrlen );
  error_check( i, "server bind" );
  printf("[server] socket bound\n");

  //set socket to listen state
  i = listen(sd, 10);
  error_check( i, "server listen" );
  printf("[server] socket in listen state\n");

  //free the structs used by getaddrinfo
  free(hints);
  freeaddrinfo(results);
  return sd;
}


/*=========================
  server_connect
  args: int sd
  sd should refer to a socket in the listening state
  run the accept call
  returns the socket descriptor for the new socket connected
  to the client.
  =========================*/
int server_connect(int sd) {
  int client_socket;
  socklen_t sock_size;
  struct sockaddr_in client_address;

  // client_socket = accept(sd, (struct sockaddr *)&client_address, &sock_size);

  sock_size = sizeof(struct sockaddr_in);

  //accept connection from an incoming client
  client_socket = accept(sd, (struct sockaddr *)&client_address, &sock_size);

  error_check(client_socket, "server accept");

  if(client_socket > 0){
    printf("[server] connection established: client_socket[%d]\n", client_socket);
  }
  return client_socket;
}
