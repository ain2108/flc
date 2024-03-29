#include "database_interactions.h"

/********* IP DATABASE MANAGMENT *********************/

// Function called in case IP is not in database
void create_IP_DBRec(char * ip_address){
  FILE * ip_db = fopen(IP_BAN_DB_NAME, "ab");
  IP_DBRec record;
  memset(&record, 0, sizeof(IP_DBRec));
  strcpy(record.IP, ip_address);
  time_t time_sec;
  time(&time_sec);
  record.ban_time = 0;

  fwrite(&record, sizeof(IP_DBRec), 1, ip_db);
  fclose(ip_db);
  return;
}

// Function load the contents of db file into an array
void load_ip_db(char * buffer){
  FILE * ip_db = fopen(IP_BAN_DB_NAME, "rb");
  fread(buffer, sizeof(IP_DBRec), MAX_IP_DB_REC_N, ip_db);
  return;
}

// Looks for the position of certain IP_DBRec in the database
int find_position_in_IPDB(IP_DBRec * ip_array, char * ip_address){
  int i = 0;
  while(i < MAX_IP_DB_REC_N){
    if(!strcmp(ip_array[i].IP, ip_address))
      return i;
    i++;
  }
  return -1; // If ip address not in database
}

void update_IPRec_in_dbfile(IP_DBRec * record, int offset){

  FILE * ip_db = fopen(IP_BAN_DB_NAME, "r+b"); //change from wb
  fseek(ip_db, offset, SEEK_SET);
  fwrite(record, sizeof(IP_DBRec), 1, ip_db);
   fclose(ip_db);
  return;
}

// Checks username and pass combo. If incorrect, returns 0
int check_validity(int socket, UsersDB * db){

  // Some buffers
  char line[READ_BUFFER_SIZE];
  char password[PASSWORD_LIMIT];
  char username[USERNAME_LIMIT];
  memset(line, 0, READ_BUFFER_SIZE);
  memset(password, 0, PASSWORD_LIMIT);
  memset(username, 0, USERNAME_LIMIT);
  char * word = NULL;
  
  // Need to extract username and password from the line
  sreadLine(socket, line, PASS_USRN_LENGTH - 1);
  
  // Extracting username
  word = strtok(line, " ");
  if(word == NULL) return -1;
  strcpy(username, word);

  // Extracting password
  word = strtok(NULL, "\0");
  if(word == NULL) return -1;
  strcpy(password, word);
   
  // Check if match occurs
  return match_pass_user(username, password, db);

}

// Tries to match username and password provided by the user with db
int match_pass_user(char * username, char * password, UsersDB * db){
  int i = 0;
  while(i < N_USERS){
    if(strcmp(username, db->records[i].login)){
      i++;
      continue;
    }
    if(strcmp(password, db->records[i].password)){
      i++;
      continue;
    }
    return i;
  }
  return -1;
}

// Authenticates the user. 
int authenticate(int sock, char * ip_address){

  // Extracting the enviroment variable
  int BLOCK_TIME;
  const char * env_var = getenv("BLOCK_TIME");
  if(env_var == NULL)
    BLOCK_TIME = 60; // In case was not set
  else
    BLOCK_TIME = atoi(env_var);

  
  time_t time_sec;
  time(&time_sec);

  fprintf(stderr, "%s attempts connection\n", ip_address);
  
  // Assume IP_BAN_DB was created before.
  // Check the ip against database of IP adresses.
  IP_DBRec * ip_db_array
    = (IP_DBRec *) malloc(sizeof(IP_DBRec) * MAX_IP_DB_REC_N);
  memset(ip_db_array, 0, sizeof(IP_DBRec) * MAX_IP_DB_REC_N);
  load_ip_db( (char *) ip_db_array); // db loaded
  int position;
  
  // If ip not found in db, create a new record, write it to db.
  if((position = find_position_in_IPDB(ip_db_array, ip_address)) == -1){
    create_IP_DBRec(ip_address);
    load_ip_db( (char *) ip_db_array);
    position = find_position_in_IPDB(ip_db_array, ip_address);
  }

  int offset = position * sizeof(IP_DBRec);

  // Check if user is still banned.
  time(&time_sec);
  if((time_sec - ip_db_array[position].ban_time) < BLOCK_TIME){
    fprintf(stdout, "%s attempts connection. Still banned\n",
	    ip_address);
    free(ip_db_array);
    close(sock);
    return -1;
  }
  // Reset ban time
  ip_db_array[position].ban_time = 0;
  
  // Load the database of passwords and usernames
  UsersDB * db = (UsersDB *) malloc(sizeof(UsersDBRec) * N_USERS);
  memset(db, 0, sizeof(UsersDB));
  FILE * db_file = fopen(DATABASE_NAME, "rb");
  fread(db, sizeof(UsersDBRec), N_USERS, db_file);


  // Attempt validation of user. Check for number of tries. 
  int attempts = 0;
  int position_in_users_db;

  // Give the user few attempts
  while(attempts < MAX_FAILS){
    if((position_in_users_db = check_validity(sock, db)) == -1){
      send(sock, "NO\n", 3, 0);     
      attempts++; 
    }else{
      return position_in_users_db;
    }
  }
  
  // Ban user's ip adress
  time(&time_sec);
  ip_db_array[position].ban_time = time_sec;
  fprintf(stderr, "%s banned at %lu\n", 
	  ip_address, ip_db_array[position].ban_time);
  update_IPRec_in_dbfile(ip_db_array + position, offset);
  close(sock);
  return -1;
}

/***********************************************************/

/************ USERS DATABASE MANAGMENT *********************/
void load_db_from_file(FILE * db_file, UsersDB * db){
  fread(db, 1, sizeof(UsersDBRec) * N_USERS, db_file);
  return;
}

// Read a UsersDBRec from databs
void read_UDBRec_from_file(UsersDBRec * record, int offset){

  FILE * users_db = fopen(DATABASE_NAME, "rb");
  fseek(users_db, offset, SEEK_SET);
  fread(record, sizeof(UsersDBRec), 1, users_db);
  fclose(users_db);
  return;
}

// Write a UsersDBRec from databs
void write_UDBRec_from_file(UsersDBRec * record, int offset){
  FILE * users_db = fopen(DATABASE_NAME, "r+b");
  fseek(users_db, offset, SEEK_SET);
  fwrite(record, sizeof(UsersDBRec), 1, users_db);
  fclose(users_db);
  return;
}

// Fills the user record with time data and ip
void fillin_UsersDBRec(UsersDBRec * record, char * ip_address){

  time_t time_sec;
  time(&time_sec);
  record->logged_in = 1;
  record->last_login_time = time_sec;
  strcpy(record->ip, ip_address);
  return;
}

 
