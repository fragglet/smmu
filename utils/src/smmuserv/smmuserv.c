// Emacs style mode select -*- C++ -*-
//---------------------------------------------------------------------------
//
// SMMU Internet Server Program
//
// smmuserv runs on some pc somewhere, 24/7.
// SMMU internet players can then register new servers with the server,
// or search for active servers by querying the server
//
// By Simon Howard
//
//--------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

#define DEFAULT_SOCKET 1234

#ifndef DJGPP
#define lsck_perror perror
#endif

typedef enum
  {
    false,
    true
  } boolean;

boolean debug = true;
clock_t startup_time;

////////////////////////////////////////////////////////////////////////////
//
// Connection
//

int port = DEFAULT_SOCKET;

int listen_sock;               // socket to listen for new connections
struct sockaddr_in in;

boolean connected = false;
int comms_sock;                // set once we have a client connected
struct sockaddr_in remote_addr;
int remote_addr_size;
unsigned long remote_ip;       // remote ip addr

void CreateSocket()
{
  listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  in.sin_family = AF_INET;
  in.sin_addr.s_addr = INADDR_ANY;
  in.sin_port = htons(port);
  bind(listen_sock, &in, sizeof(in));

  if(listen(listen_sock, 10))
    {
      lsck_perror("WaitForConnection");
      exit(-1);
    }

}

void CloseConnection()
{
  printf("disconnected\n");
  //  close(listen_sock);
  close(comms_sock);
  connected = false;
}

void GetConnection()
{
  if(connected)
    CloseConnection();

  // have to re-create socket each time
  // this should _not_ be neccesary
  
  //  CreateSocket();
  
  // listen for a connection


  // got connection
  // accept connection
  
  remote_addr_size = sizeof(remote_addr);
  comms_sock = accept(listen_sock, &remote_addr, &remote_addr_size);

  connected = true;

  remote_ip = remote_addr.sin_addr.s_addr;

  if(debug)
    {
      printf("connection from %i.%i.%i.%i\n",
	     remote_ip & 255, (remote_ip >> 8) & 255,
	     (remote_ip >> 16) & 255, (remote_ip >> 24) & 255);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Converse with remote node, send/recv appropriate info
//

#define SERVER_LIST_SIZE 16

unsigned long server_list[SERVER_LIST_SIZE];
int server_list_num = 0;

// stats 
int stats_listings, stats_adds;

// read line from socket: end when newline (\n) is received
// NOTE: we stop as soon as we receive a \n: any extra data
// on new lines is ignored

#define READ_BUFFER_SIZE 128

char *ReadLine()
{
  static char read_buffer[READ_BUFFER_SIZE]; // read buffer returned 
  int read_bytes = 0;
  char temp_buffer[READ_BUFFER_SIZE];      // temp buffer to read from recv()
  int temp_bytes;
  int i;

  while(1)
    {
      temp_bytes = recv(comms_sock, temp_buffer, READ_BUFFER_SIZE-1, 0);

      for(i=0; i<temp_bytes; i++)
	{
	  if(temp_buffer[i] == '\n')
	    {
	      // add end-of-string marker
	      read_buffer[read_bytes] = '\0';
	      return read_buffer;
	    }
	  if(isprint(temp_buffer[i]))
	    read_buffer[read_bytes++] = temp_buffer[i];
	}
    }
}

void SendServerList()
{
  int i;

  // go backwards thru list: send latest first

  i = server_list_num;

  do
    {
      // loop around back to end of list

      i = (i ? i : SERVER_LIST_SIZE) - 1;

      // dont send empty entries
      if(server_list[i])
	{
	  char tempstr[128];
	  sprintf(tempstr, "%i.%i.%i.%i\n", 
		  server_list[i] & 255, (server_list[i] >> 8) & 255,
		  (server_list[i] >> 16) & 255, (server_list[i] >> 24) & 255);
	  write(comms_sock, tempstr, strlen(tempstr) + 1);
	}

    } while(i != server_list_num);
  
  stats_listings++;

  if(debug)
    printf("send servers list\n");
}

void AddToServerList()
{
  int i;

  // check if server is already in list
  
  for(i=0; i<SERVER_LIST_SIZE; i++)
    if(server_list[i] == remote_ip)
      {
	if(debug)
	  printf("remote node already in list\n");
	return;
      }

  // add to list at latest point

  server_list[server_list_num++] = remote_ip;

  stats_adds++;

  if(debug)
    printf("added to servers list\n");
}

void SendStats()
{
  char buffer[128];
  int uptime;          // in seconds

  uptime = (clock()-startup_time) / CLOCKS_PER_SEC;

  sprintf(buffer,
	  "smmuserv stats\n\r"
	  "--------------\n\r"
	  "uptime %02i:%02i:%02i\n\r"
	  "%i adds to list\n\r"
	  "%i reads from list\n\r",
	  uptime / (60*60), (uptime / 60) % 60, uptime % 60,
	  stats_adds, stats_listings);
  write(comms_sock, buffer, strlen(buffer) + 1);	  

  if(debug)
    printf("sent stats\n");
}

// kill server, but _only_ if kill request came locally

void KillServer()
{
  if(remote_ip == INADDR_LOOPBACK)
    {      
      printf("remote termination\n");
      exit(0);
    }
  else
    {
      char error_msg[] = "yeah right, sucker :)";
      write(comms_sock, error_msg, strlen(error_msg));

      if(debug)
	printf("unauthorised kill attempt\n");
    }
}

// get command from remote node and send back appropriate data

void GetCommand()
{
  //  char read_buffer[READ_BUFFER_SIZE];
  //  int read_bytes;
  char *read_buffer;

  // make sure we are connected

  if(!connected)
    {
      printf("GetCommand: not connected!\n");
      exit(-1);
    }

  // read command

  //  read_bytes = recv(comms_sock, read_buffer, READ_BUFFER_SIZE-1, 0);
  //  read_buffer[read_bytes] = '\0';
  read_buffer = ReadLine();

  if(debug)
    printf("command > \"%s\"\n", read_buffer);

  // check command type

  if(!strcasecmp(read_buffer, "list"))            // get server list
    SendServerList();
  else if(!strcasecmp(read_buffer, "add"))        // add to server list
    AddToServerList();
  else if(!strcasecmp(read_buffer, "stats"))      // get usage stats
    SendStats();
  else if(!strcasecmp(read_buffer, "kill"))       // kill server
    KillServer();
  else
    {
      char error_response[] = "say what, say what?\n";
      write(comms_sock, error_response, strlen(error_response)+1);
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Main
//

int main(int argc, char *argv[])
{
  // init
  startup_time = clock();
  CreateSocket();
  
  // get connections

  while(1)
    {
      GetConnection();
      GetCommand();
      CloseConnection();
    }
}
