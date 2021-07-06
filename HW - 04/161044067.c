/*
 *  Gokhan HAS - 161044067
 *  CSE 344 - System Programming - HW #04
 */ 

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0
#define CHEF_SIZE 6

// Global Variables ....
char* chefNeeds[6] = {"SW", "FW", "MS", "FS", "MW", "MF"};
char* chefNeeds_str[6] = {"sugar and walnuts", "flour and walnuts", "milk and sugar", 
    "flour and sugar", "milk and walnuts", "milk and flour"};

// Synchronization problems solves Posix Unnamed Semaphores ....
sem_t mutex_chefAndWholesaler;
sem_t mutex_finished;
sem_t mutex_chefs;


// Chefs number's between 1 to 6 ...
int chef_numbers[6] = {1,2,3,4,5,6};

char* ingredients = NULL;

int size;

// Funtions Define ...

void errorExit(char* error);
void printUsage();
int getValueFromSem(sem_t* sem);
size_t getFileSize(const char* fileName);
int getIngredients(char* str);
int whichChef(char* str);
void which(int chef, char* print_message);
void* chef(void* chefId);
int isThereAtLeastTenLine(int fd, int file_size);
int getStringSize(char* str);

int main(int argc, char **argv) {
 
  char *iValue = NULL;
  int iIndex = 0;

  int another = 0;
  int c;

  while ((c = getopt(argc, argv, "i:")) != -1)
  switch (c){
    case 'i':
      iIndex += 1;
      iValue = optarg;
      break;
   
    default:
      printUsage();
      abort();
  }

  /* Error handling in command line arguments ... */
  if (iIndex == 1 && another == 0); // CHECKS THE PARAMETER CONTROL ...
  else {
    errno = EIO;
    printUsage(); 
    errorExit("ERROR ");
  }

  // Opening file ...
  int inputFd  = open(iValue, O_RDONLY);
  if (inputFd  == -1) {
    perror("ERROR ! Program filePath (-i) ");
    exit(EXIT_FAILURE);
  }


  lseek(inputFd, 0, SEEK_SET);
  int fileSize = getFileSize(iValue);
  size = (fileSize + 1) / 3;
  
  
  if(isThereAtLeastTenLine(inputFd, fileSize) == FALSE) {
    errno = EIO;
    if(close(inputFd) == -1) {
      errorExit("ERROR close input file (in at least 10 control) ");
    }
    errorExit("ERROR the input file is at least 10 INVALID line ");
  }
  
  // Initialize mutex ...
  sem_init(&mutex_chefAndWholesaler, 0, 0);
  sem_init(&mutex_chefs, 0, 1);
  sem_init(&mutex_finished, 0, 0);

  int i=0;
  pthread_t chef_threads[6];
  
  for(i = 0; i < CHEF_SIZE; i++) {
    if(pthread_create(&chef_threads[i], NULL, chef, &chef_numbers[i]) != 0) {
      errorExit("ERROR pthread_create ");
    }
  }
  
  lseek(inputFd, 0, SEEK_SET);
  // Wholesalar threads is main thread ...
  char* readedByte;
  char readNewLine[1];
  int readedCount = 0;
  do{
    readedByte = (char *) malloc(sizeof(char) * 2);
    int sz = read(inputFd, readedByte, 2);
    
    readedCount += sz;
    if(strcmp(readedByte,"WS") == 0)
      strcpy(readedByte,"SW");
    else if(strcmp(readedByte,"WF") == 0)
      strcpy(readedByte,"FW"); 
    else if(strcmp(readedByte,"SM") == 0)
      strcpy(readedByte,"MS"); 
    else if(strcmp(readedByte,"SF") == 0)
      strcpy(readedByte,"FS"); 
    else if(strcmp(readedByte,"WM") == 0)
      strcpy(readedByte,"MW");
    else if(strcmp(readedByte,"FM") == 0)
      strcpy(readedByte,"MF");

    ingredients = readedByte;
    
    printf("the wholesaler delivers %s\n",chefNeeds_str[getIngredients(readedByte)]);
    printf("the wholesaler is waiting for the dessert\n");
    sem_wait(&mutex_chefAndWholesaler);
    printf("the wholesaler has obtained the dessert and left to sell it\n");
    
    // read '\n' chracters ... 
    sz = read(inputFd,readNewLine,1);
    if(sz != 0)
      readedCount += sz;
    
    free(readedByte);
  } while(readedCount != fileSize);
  sem_post(&mutex_finished);

  // Waits all threads ...
  for(i = 0; i < CHEF_SIZE; i++) {
    if(pthread_join(chef_threads[i], NULL) != 0) {
      errorExit("ERROR pthread_join ");
    }
  }

  // Close the file ...
  if(close(inputFd) == -1) {
    errorExit("ERROR close input file ");
  }

  // destroy the semaphores ...
  if(sem_destroy(&mutex_chefAndWholesaler) == -1) {
    errorExit("ERROR sem_destroy mutex_chefAndWholesaler ");
  }

  if(sem_destroy(&mutex_finished) == -1) {
    errorExit("ERROR sem_destroy mutex_finished ");
  }

  if(sem_destroy(&mutex_chefs) == -1) {
    errorExit("ERROR sem_destroy mutex_chefs ");
  }

  exit(EXIT_SUCCESS);
}


void errorExit(char* error) {
  perror(error);
  exit(EXIT_FAILURE);
}

void printUsage() {
  char* print = NULL; 
  print = "\n";
  write(STDOUT_FILENO,print,strlen(print));
  print = "################## USAGE ####################\n";
  write(STDOUT_FILENO,print,strlen(print));
  print = "#  -i : filePath to wholesaler will read    #\n";
  write(STDOUT_FILENO,print,strlen(print));
  print = "#############################################\n\n";
  write(STDOUT_FILENO,print,strlen(print));  
  print = NULL;
}

void* chef(void* chefId) {
  printf("chef%d is waiting for %s\n",*(int *) chefId, chefNeeds_str[(*(int *) chefId) - 1]);
  while(TRUE) {
    if(ingredients != NULL && (getStringSize(ingredients) != 5)) {
      srand(time(NULL));
      sem_wait(&mutex_chefs);
      int chef_Id = *(int *) chefId;
      if(chef_Id == (whichChef(ingredients) + 1)) {
        printf("chef%d is waiting for %s\n",*(int *) chefId, chefNeeds_str[(*(int *) chefId) - 1]);
        char* print_message = (char*)malloc(sizeof(char)*50);
        which(chef_Id, print_message);
        printf("%s", print_message);
        free(print_message);
        printf("chef%d is preparing the dessert\n",chef_Id);
        ingredients = "empty";
        sleep((rand() % 5) + 1);
        printf("chef%d has delivered the dessert to the wholesaler\n",chef_Id);
        sem_post(&mutex_chefAndWholesaler);
        sem_post(&mutex_chefs);
       
      } else {
        sem_post(&mutex_chefs);  
      }
    }
    
    if(getValueFromSem(&mutex_finished) == 1)
      break;

  }
  return NULL;
}

int getStringSize(char* str) {
  int i = 0, control = 0;
  while(control == 0) {
    if(str[i] == '\0')
      return i;
    i++;
  }
  return -1;
}

int getValueFromSem(sem_t* sem) {
  int a;
  sem_getvalue(sem,&a);
  return a;
}

size_t getFileSize(const char* fileName) {
  struct stat statX;
  stat(fileName, &statX);
  size_t size = statX.st_size;
  return size;
}

int getIngredients(char* str) {
  for(int i=0; i < CHEF_SIZE; i++) {
    if(strcmp(str,chefNeeds[i]) == 0)
      return i;
  }
  return -1;
}

int whichChef(char* str) {
  for(int i=0; i < CHEF_SIZE; i++) {
    if(strcmp(str,chefNeeds[i]) == 0)
      return i;
  }
  return -1;
}

void which(int chef, char* print_message) {
  
  if(strcmp(ingredients, "SW") == 0) 
    sprintf(print_message,"chef%d has taken sugar\nchef%d has taken walnuts\n",chef,chef);
  else if(strcmp(ingredients, "FW") == 0) 
    sprintf(print_message,"chef%d has taken flour\nchef%d has taken walnuts\n",chef,chef);
  else if(strcmp(ingredients, "MS") == 0) 
    sprintf(print_message,"chef%d has taken milk\nchef%d has taken sugar\n",chef,chef);
  else if(strcmp(ingredients, "FS") == 0) 
    sprintf(print_message,"chef%d has taken flour\nchef%d has taken sugar\n",chef,chef);
  else if(strcmp(ingredients, "MW") == 0) 
    sprintf(print_message,"chef%d has taken milk\nchef%d has taken walnuts\n",chef,chef);
  else if(strcmp(ingredients, "MF") == 0) 
    sprintf(print_message,"chef%d has taken milk\nchef%d has taken flour\n",chef,chef);
  
}

int isThereAtLeastTenLine(int fd, int file_size) {
  lseek(fd,0,SEEK_SET);
  char* readOneByte;
  int sz, count = 0, atleast10 = 0;
  do {
    readOneByte = (char*)malloc(sizeof(char) * 1);
    sz = read(fd,readOneByte,1);
    if(sz < 1)
      errorExit("ERROR There is an error while reading input file ");
    if(strcmp(readOneByte,"\n") == 0)
      atleast10 += 1;
    count += sz;
    free(readOneByte);
  } while(count != file_size);
  lseek(fd,0,SEEK_SET);

  if(atleast10 >= 9)
    return TRUE;
  return FALSE;
}