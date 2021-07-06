/* Gokhan Has - 161044067 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// Function define ...
void closeFiles(int filePointer1, int filePointer2);
char *complexNumber(char *array, int size);
size_t findFirstNewline(int fd, size_t size);
size_t getFileSize(const char* fileName);
int getArrSize(char* arr, int length);


int main(int argc, char **argv)
{
  struct flock lock;
  char *iValue = NULL;
  char *oValue = NULL;
  char *tValue = NULL;
  int timer = 0;

  int iIndex = 0;
  int oIndex = 0;
  int tIndex = 0;

  int another = 0;
  int c;

  // opterr = 0;

  while ((c = getopt(argc, argv, "i:o:t:")) != -1)
    switch (c)
    {
    case 'i':
      iIndex += 1;
      iValue = optarg;
      break;
    case 'o':
      oIndex += 1;
      oValue = optarg;
      break;
    case 't':
      tIndex += 1;
      tValue = optarg;
      break;
    case '?':
      another += 1;
      break;
    default:
      abort();
    }

  // printf("inputPathA = %s, outputPathA = %s, time = %s\n", iValue, oValue, tValue);

  /* Error handling in command line arguments ... */
  if (iIndex == 1 && oIndex == 1 && tIndex == 1 && another == 0)
  {
    timer = atoi(tValue);
    if (timer < 1 || timer > 50)
    {
      errno = EINVAL;
      perror("ERROR (-t) ");
      return 0;
    }
  }
  else
  {
    errno = EIO;
    perror("ERROR ");
    return 0;
  }

  

  // Input file operation ...
  int fdForInput = open(iValue, O_RDONLY);
  if (fdForInput == -1) {
    perror("ERROR ! ProgramA Input ");
    close(fdForInput);
    return 0;
  }

  
  

  // Output file operation ...
  int fdForOutput = open(oValue, O_RDWR | O_CREAT , S_IWUSR | S_IRUSR);
  if (fdForOutput == -1) {
    perror("ERROR ! ProgramA Output ");
    closeFiles(fdForInput, fdForOutput);
    return 0;
  } 

  int controlFile = open("xxx.txt", O_WRONLY | O_CREAT , S_IWUSR | S_IRUSR);
  if (controlFile == -1) {
    perror("ERROR ! ProgramA Output ");
    closeFiles(fdForInput, fdForOutput);
    return 0;
  } 

  int sz;
  
  char *charArr32 = (char *)calloc(32, sizeof(char));
  do
  {
    lock.l_type = F_WRLCK; //ekledim
    fcntl(fdForOutput,F_SETLKW,&lock);
    /* Read the next line's worth of bytes*/
    sz = read(fdForInput, charArr32, 32);
    //printf("SZ : %d\n", sz);

    lock.l_type = F_WRLCK; //ekledim
    fcntl(fdForOutput,F_SETLKW,&lock);
    if (sz < 32) {
      lock.l_type = F_WRLCK; //ekledim
      fcntl(fdForOutput,F_SETLKW,&lock);
      lseek(controlFile,0,SEEK_END);
      write(controlFile,"X",strlen("X"));
      lock.l_type = F_UNLCK; //ekledim
      fcntl(fdForOutput,F_SETLKW,&lock);
      close(fdForInput);
      free(charArr32);
      return 0;
    }

    char* arr;
    arr = complexNumber(charArr32, sz);
    
    //printf("Arr --> %s\n",arr);
    //printf("ArrSize --> %d\n",strlen(arr));

    size_t cursor = findFirstNewline(fdForOutput, getFileSize(oValue));
    //printf("Cursor    : %d\n",cursor);
    //printf("FileSize  : %d\n",getFileSize(oValue));
    lseek(fdForOutput, (off_t) cursor + 1, SEEK_SET);
    
    // Şurada kopyalancak ...
    // Yazmaya kitleme ...
    memset(&lock,0,sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_start = SEEK_SET;
    lock.l_whence = SEEK_END;
    lock.l_len = getFileSize(oValue);
    fcntl(fdForOutput,F_SETLKW,&lock);

    char* fileStr;
    if(cursor == 0) { // Başa

      if(getFileSize(oValue) == 0) {
        lseek(fdForOutput, (off_t) cursor, SEEK_SET);
        
        lock.l_type = F_WRLCK; //ekledim
        fcntl(fdForOutput,F_SETLKW,&lock);
        
        write(fdForOutput,arr,strlen(arr));
        
        lseek(fdForOutput,strlen(arr)-1,SEEK_SET);
        char* x = "\n";
        write(fdForOutput,x,strlen(x));
        
        lock.l_type = F_UNLCK; //ekledim
        fcntl(fdForOutput,F_SETLKW,&lock);
      } 
      else {
        fileStr = (char *)malloc(sizeof(char) * (getFileSize(oValue) - cursor));
        lseek(fdForOutput, (off_t) cursor, SEEK_SET);
        read(fdForOutput,fileStr,getFileSize(oValue) - cursor);

        lock.l_type = F_WRLCK; //ekledim
        fcntl(fdForOutput,F_SETLKW,&lock);


        lseek(fdForOutput, (off_t) cursor, SEEK_SET);
        write(fdForOutput,arr,strlen(arr));

        lseek(fdForOutput,(off_t) (cursor  + strlen(arr) ), SEEK_SET);
        write(fdForOutput,fileStr,(getFileSize(oValue) - cursor));

        lock.l_type = F_UNLCK; //ekledim
        fcntl(fdForOutput,F_SETLKW,&lock);

      }
    }
    else if(getFileSize(oValue) - cursor  > 1) {
      fileStr = (char *)malloc(sizeof(char) * (getFileSize(oValue) - cursor));
      read(fdForOutput,fileStr,getFileSize(oValue) - cursor);

      lock.l_type = F_WRLCK; //ekledim
      fcntl(fdForOutput,F_SETLKW,&lock);


      lseek(fdForOutput,(off_t) (cursor + 1), SEEK_SET);
      write(fdForOutput,arr,strlen(arr));
      lseek(fdForOutput,(off_t) (cursor + 1 + strlen(arr) ), SEEK_SET);
      write(fdForOutput,fileStr,(getFileSize(oValue) - cursor - 1));

      lock.l_type = F_UNLCK; //ekledim
      fcntl(fdForOutput,F_SETLKW,&lock);
    } else {
      lock.l_type = F_WRLCK; //ekledim
      fcntl(fdForOutput,F_SETLKW,&lock);
      lseek(fdForOutput,getFileSize(oValue),SEEK_SET);
      write(fdForOutput,arr,strlen(arr));
      lseek(fdForOutput,getFileSize(oValue),SEEK_SET);
      char* x = "\n";
      write(fdForOutput,x,strlen(x));
      lock.l_type = F_WRLCK; //ekledim
      fcntl(fdForOutput,F_SETLKW,&lock);
      //printf("ELSE\n");
    }

    lock.l_type = F_UNLCK;
    fcntl(fdForOutput,F_SETLKW,&lock);
    

    // Kopyaladığını yaz ...
    lseek(fdForOutput, 0, SEEK_SET);
    
    // Write is done and sleep by the timer ...
    printf("\n*****************************\n");
    printf("   Sleeping %d miliseconds    \n", timer);
    printf("*****************************\n");
    //cfree(fileStr);
    usleep(timer*1000);
  } while (sz == 32);

  lseek(controlFile,0,SEEK_END);
  write(controlFile,"X",strlen("X"));
  //close(controlFile);
  /* Close the files ...*/
  closeFiles(fdForInput, fdForOutput);
  return 0;
}

/* Close the files ...*/
void closeFiles(int filePointer1, int filePointer2)
{
  close(filePointer1);
  close(filePointer2);
}

/* Given parameter 32array, convert char to complex numbers and return char pointer ...*/
/* FX : a5 is 9753 and complex is "97 +i53"*/
char* complexNumber(char *array, int size) {
  
  char* returnArr = (char *)malloc(500*sizeof(char));
  int j = 0;
  
  while (j < size)
  {
    char* result = malloc(10*sizeof(char));
    char* result2 = malloc(10*sizeof(char));
    char* result3 = malloc(30*sizeof(char));

    int ch1 = (int) array[j];
    sprintf(result, "%d", ch1); 
    
    int ch2 = (int) array[j+1];
    sprintf(result2, "%d", ch2); 

    sprintf(result3, "%s +i%s, ",result,result2);
    strcat(returnArr, result3);
    
    free(result);
    free(result2);
    free(result3);

    j+=2;
  }
  return returnArr;
}

/* Ilk boslugun yerini donduren fonksiyon ...*/
size_t findFirstNewline(int fd, size_t size) {
  int sz;
  char ch[1];
  int i = 0;
  int previous = 0;
  size_t newLineIndex;
  lseek(fd, 0, SEEK_SET);
  do{
    sz = read(fd, ch, sizeof(ch));
    if((int) ch[0] == 0 && i == 0) {
      return 0;
    }
    if((int) ch[0] == 10 && i == 0) {
      return 0;
    }
    if((int) ch[0] == 10) {
      if(previous == 1 && (i-1) == newLineIndex) {
        return newLineIndex;
      } 
      newLineIndex = i;
      previous = 1;
    }
    i++;
  } while (i < size);
  return i;
}

/* Get file size to file name ... */
size_t getFileSize(const char* fileName) {
  struct stat statX;
  stat(fileName, &statX);
  size_t size = statX.st_size;
  return size;
}