#include "setup.h"
#include "db_operations.h"
#include "handle_client.h"

int main(int argc, char ** argv){

  // Some correct usage checks
  if(argc != 2){
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // Server socket initiation
  unsigned short port = atoi(argv[1]); //need to make sure port numbers is right.
  // Already listening on the socket
  int listenSock = setup_server_socket(port);

  // Client socket declarations
  int acceptSock;                    
  struct sockaddr_in clntAddr; 
  unsigned int clntLen = sizeof(clntAddr);
  pid_t pid_handler;
  
  // Initialize database
  fprintf(stderr, "Starting...\n");
  UsersDB * db = (UsersDB *) malloc(sizeof(UsersDBRec) * N_USERS);
  memset(db, 0, sizeof(UsersDB));
  initialize_database(USERS_FILE_NAME, DATABASE_NAME, db);
  fprintf(stderr, "Database initialized:\n");
  // read_usersDB(db, stderr);

  // Need to initialize the databse of IP adresses
  FILE * ip_db = fopen(IP_BAN_DB_NAME, "wb");
  fclose(ip_db);

  // Infinite loop accepting connections
  for(;;){

    acceptSock = accept(listenSock, (struct sockaddr *) &clntAddr, &clntLen);
    if (acceptSock < 0){
      die_verbosely("accept() failed"); // Should not die necessarily.
    }
    if ((pid_handler = fork()) < 0){
      die_verbosely("fork() failed");  // Should not die necessarily. 
    }


    /*vvvvvvv THE HANDLER PROCESS BLOW vvvvvvv*/
    if (pid_handler == 0){
      close(listenSock);
      handle_client(acceptSock, inet_ntoa(clntAddr.sin_addr));
      exit(0);
    }
    /*^^^^^^^^^^^^^^ HANDLER ENDS ^^^^^^^^^^^^*/

    // need to close connection in listner proccess
    close(acceptSock); 
  }

  
  return 0;

}


