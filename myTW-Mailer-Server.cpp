#include <filesystem>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <string>

///////////////////////////////////////////////////////////////////////////////

#define BUF 1024
#define PORT 6543

///////////////////////////////////////////////////////////////////////////////

int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;
std::string directory = "";

///////////////////////////////////////////////////////////////////////////////

void *clientCommunication(void *data);
void signalHandler(int sig);
void child(void *data);


///////////////////////////////////////////////////////////////////////////////

// Prints usage of programm
void print_usage(char *programm_name){
   printf("Usage: %s <ip> <mail-spool-directoryname>\n\n", programm_name);
   EXIT_FAILURE;
}


int main(int argc, char **argv)
{
   //pid_t pid;
   socklen_t addrlen;
   struct sockaddr_in address, cliaddress;
   int reuseValue = 1;
   char *programm_name;
   programm_name = argv[0];
   unsigned short port;

   // Gets IP and Port if the correct number of arguments is given
   if(argc == 3){
      std::stringstream intPort(argv[1]);
         intPort >> port;
      directory = argv[2];
   }else{
      print_usage(programm_name);
   }

   // Checks if the port is valid
   if(port <= 0 || port > 65535){
      printf("Invalid Port!\n");
      print_usage(programm_name);
   }
   
   // Prints the selectet Directory and Port
   std::cout << "Directory: " << directory << "\n";
   printf("Port: %d\n", port);

   ////////////////////////////////////////////////////////////////////////////
   // SIGNAL HANDLER
   // SIGINT (Interrup: ctrl+c)
   // https://man7.org/linux/man-pages/man2/signal.2.html
   if (signal(SIGINT, signalHandler) == SIG_ERR)
   {
      perror("signal can not be registered");
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // CREATE A SOCKET
   // https://man7.org/linux/man-pages/man2/socket.2.html
   // https://man7.org/linux/man-pages/man7/ip.7.html
   // https://man7.org/linux/man-pages/man7/tcp.7.html
   // IPv4, TCP (connection oriented), IP (same as client)
   if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      perror("Socket error"); // errno set by socket()
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // SET SOCKET OPTIONS
   // https://man7.org/linux/man-pages/man2/setsockopt.2.html
   // https://man7.org/linux/man-pages/man7/socket.7.html
   // socket, level, optname, optvalue, optlen
   if (setsockopt(create_socket,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  &reuseValue,
                  sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reuseAddr");
      return EXIT_FAILURE;
   }

   if (setsockopt(create_socket,
                  SOL_SOCKET,
                  SO_REUSEPORT,
                  &reuseValue,
                  sizeof(reuseValue)) == -1)
   {
      perror("set socket options - reusePort");
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // INIT ADDRESS
   // Attention: network byte order => big endian
   memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(port);

   ////////////////////////////////////////////////////////////////////////////
   // ASSIGN AN ADDRESS WITH PORT TO SOCKET
   if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
   {
      perror("bind error");
      return EXIT_FAILURE;
   }

   ////////////////////////////////////////////////////////////////////////////
   // ALLOW CONNECTION ESTABLISHING
   // Socket, Backlog (= count of waiting connections allowed)
   if (listen(create_socket, 5) == -1)
   {
      perror("listen error");
      return EXIT_FAILURE;
   }

   while (!abortRequested)
   {
      /////////////////////////////////////////////////////////////////////////
      // ignore errors here... because only information message
      // https://linux.die.net/man/3/printf
      printf("Waiting for connections...\n");

      /////////////////////////////////////////////////////////////////////////
      // ACCEPTS CONNECTION SETUP
      // blocking, might have an accept-error on ctrl+c
      addrlen = sizeof(struct sockaddr_in);
      if ((new_socket = accept(create_socket,
                               (struct sockaddr *)&cliaddress,
                               &addrlen)) == -1)
      {
         if (abortRequested)
         {
            perror("accept error after aborted");
         }
         else
         {
            perror("accept error");
         }
         break;
      }

      /////////////////////////////////////////////////////////////////////////
      // START CLIENT
      // ignore printf error handling
      printf("Client connected from %s:%d...\n",
             inet_ntoa(cliaddress.sin_addr),
             ntohs(cliaddress.sin_port));
      clientCommunication(&new_socket);
      //pid = fork();
      //switch(pid){
      //    case -1:
      //       printf("Child could not be started");
      //       exit(EXIT_FAILURE);
      //       break;
      //    case 0:
      //       child(&new_socket);
      //       exit(EXIT_SUCCESS);
      //       break;
      //    default:
      //       break;
      //}
      new_socket = -1;
   }

   // frees the descriptor
   if (create_socket != -1)
   {
      if (shutdown(create_socket, SHUT_RDWR) == -1)
      {
         perror("shutdown create_socket");
      }
      if (close(create_socket) == -1)
      {
         perror("close create_socket");
      }
      create_socket = -1;
   }

   return EXIT_SUCCESS;
}

std::string SplitString(std::string& str, const char c)
{
    const size_t index = str.find(c);
    const std::string ret = str.substr(0, index);
    str.erase(0, index + 1);
    return ret;
}

bool sendCommand(std::string message){
    const std::string sender = SplitString(message, '\n');
     
    const std::string reciever = SplitString(message, '\n');
     
    const std::string topic = SplitString(message, '\n');
    if(topic.length() > 80){
        return false;
    }
    
    const std::string content = SplitString(message, '.');

    if(directory.back() != '/'){
        directory.append("/"); 
    }
    std::string finaldirectory = directory + reciever;

    if(!std::filesystem::exists(finaldirectory)){
        if (!std::filesystem::create_directory(finaldirectory)){
            std::cout << "Error creating directory" << std::endl;
            return false;
        }
    }
    std::string input = "";

    std::ofstream newMessage(finaldirectory + "/" + "datastructure");
    if (newMessage.is_open())
        newMessage << content;
    else
    {
        std::cout << "Error saving Message" << std::endl;
        newMessage.close();
        return false;
    }
    newMessage.close();

    return true;
}


bool readCommand(std::string message){
    return true;
}

bool listCommand(std::string message){
    return true;
}

bool delCommand(std::string message){
    return true;
}


void *clientCommunication(void *data)
{
   char buffer[BUF];
   int size;
   int *current_socket = (int *)data;
   //FILE * file;
   std::vector<std::string> command;
   std::string word;

   ////////////////////////////////////////////////////////////////////////////
   // SEND welcome message
   strcpy(buffer, "Welcome to myserver!\r\nPlease enter your commands...\r\n");
   if (send(*current_socket, buffer, strlen(buffer), 0) == -1)
   {
      perror("send failed");
      return NULL;
   }

   do
   {
      /////////////////////////////////////////////////////////////////////////
      // RECEIVE
      memset(buffer, 0 ,BUF);
      size = recv(*current_socket, buffer, BUF - 1, 0);
      if (size == -1)
      {
         if (abortRequested)
         {
            perror("recv error after aborted");
         }
         else
         {
            perror("recv error");
         }
         break;
      }

      if (size == 0)
      {
         printf("Client closed remote socket\n"); // ignore error
         break;
      }

      // remove ugly debug message, because of the sent newline of client
      if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
      {
         size -= 2;
      }
      else if (buffer[size - 1] == '\n')
      {
         --size;
      }
      
      
      
     buffer[size] = '\0';

     std::string message = buffer;

     std::string command = SplitString(message, '\n');



     
     if(command == "SEND"){
            std::cout << "Command" << command << std::endl;
            
            sendCommand(message);

      }else if(command == "LIST"){
            std::cout << "Command" << command << std::endl;;
           
      }else if(command == "READ"){
            std::cout << "Command" << command << std::endl;;
            
      }else if(command == "DEL"){    
            std::cout << "Command" << command << std::endl;;
            
      }else{
           printf("Message received: %s\n", buffer); // ignore error
      }



      
      if (send(*current_socket, "OK", 3, 0) == -1)
      {
         perror("send answer failed");
         return NULL;
      }
      
       command.clear();
      
   } while (strcmp(buffer, "quit") != 0 && !abortRequested);

   // closes/frees the descriptor if not already
   if (*current_socket != -1)
   {
      if (shutdown(*current_socket, SHUT_RDWR) == -1)
      {
         perror("shutdown new_socket");
      }
      if (close(*current_socket) == -1)
      {
         perror("close new_socket");
      }
      *current_socket = -1;
   }

   return NULL;
}

void signalHandler(int sig)
{
   if (sig == SIGINT)
   {
      printf("abort Requested... "); // ignore error
      abortRequested = 1;
      /////////////////////////////////////////////////////////////////////////
      // With shutdown() one can initiate normal TCP close sequence ignoring
      // the reference count.
      // https://beej.us/guide/bgnet/html/#close-and-shutdownget-outta-my-face
      // https://linux.die.net/man/3/shutdown
      if (new_socket != -1)
      {
         if (shutdown(new_socket, SHUT_RDWR) == -1)
         {
            perror("shutdown new_socket");
         }
         if (close(new_socket) == -1)
         {
            perror("close new_socket");
         }
         new_socket = -1;
      }

      if (create_socket != -1)
      {
         if (shutdown(create_socket, SHUT_RDWR) == -1)
         {
            perror("shutdown create_socket");
         }
         if (close(create_socket) == -1)
         {
            perror("close create_socket");
         }
         create_socket = -1;
      }
   }
   else
   {
      exit(sig);
   }
}
