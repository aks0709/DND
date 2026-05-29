#ifndef COMMON_H
#define COMMON_H

//Colours for the client interface//
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_CYAN    "\033[0;36m"


//for socket programming
#include <sys/socket.h> //socket function -> socket(),bind(),listen(),accept(),send(),recv(),connect()

#include <netinet/in.h> //Internet address strucutres and constants like sockaddr_in,htons,INADDR_ANY
#include <arpa/inet.h> //for conversions like inet_ntoa(),inet_pton() etc
#include <netdb.h> //gethostbyname(),getaddrinfo()


// Input/Output
#include <stdio.h>
#include <stdlib.h>  //DMA
#include <unistd.h>  //POSIX SYSTEM CALLS like read(),write(),exec(),fork() etc
#include <fcntl.h>  //file control like open(),file descriptor flags like O_RDONLY

//Memory and string operations
#include <string.h>  //string manipulation
#include <strings.h> //strcasecmp(),strncasecmp()
#include <ctype.h>  //character checks and comparisons

//  Math and numeric utilities
#include <math.h>
#include <limits.h>
#include <float.h>

//Threading and concurrency
#include <pthread.h>  //pthread_create(),pthread_join(),mutexes
#include <semaphore.h>  //sem_inti(),sem_wait()  ->thread sync

#include <stdbool.h>
// system calls and process control
#include <sys/types.h>   //pid_t,size_t
#include <sys/stat.h>  //file status and permission ,stat(),chmod()
#include <sys/wait.h>  //process control wait()
#include <errno.h>  //error codes
#include <signal.h>  //signal handling
#include <time.h>   //time and date functions

//File and directory handling
#include <dirent.h>

#endif // COMMON_H
