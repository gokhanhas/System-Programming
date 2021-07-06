/* Gokhan Has - 161044067 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <complex.h>
double PI;

// Function 
void closeFiles(int filePointer1, int filePointer2);
int newLineCount(int fd, int size);
size_t getFileSize(const char* fileName);
void readLine(int fd, int randomLine, int fileSize, char* inputName, int timer, int outputFile, struct flock lock); 
int getArrSize(char* arr, int length);
void FFTAlgorithm(complex input[], complex arr[], int n, int count);
void algorithmFFT(complex arr[], int n);
char* remchar(char *s, char chr);
int replacechar(char *str, char orig, char rep);
char *replaceWord(const char *s, const char *oldW, const char *newW); 



int main(int argc, char **argv) {

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

    //printf("inputPathA = %s, outputPathA = %s, time = %s\n", iValue, oValue, tValue);

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
    int fdForInput = open(iValue, O_RDWR);
    if (fdForInput == -1)
    {
        do {
          usleep(timer*1000);
          fdForInput = open(iValue, O_RDWR);      
        } while(fdForInput == -1 );
    }
    
    
    // Output file operation ...
    int fdForOutput = open(oValue, O_RDWR | O_CREAT | O_SYNC | O_APPEND , S_IWUSR | S_IRUSR);
    if (fdForOutput == -1) {
        perror("ERROR ! ProgramB Output ");
        closeFiles(fdForInput, fdForOutput);
        return 0;
    }

    // Degiskenler ...
    int count = 0, randomLineNumber = 0;

    // Kac tane line oldugunu bulma ...
    count = newLineCount(fdForInput, getFileSize(iValue)); 
    //printf("COUNT : %d\n", count);
    
    // Rastgele sayi olusturma ...
    do {
        srand(time(NULL));
        randomLineNumber = rand() % count;
        //printf("RANDOM LINE NUMBER : %d\n",randomLineNumber);
        if(randomLineNumber == 0);
            randomLineNumber++;
    } while(randomLineNumber == 0);
    

    // Rastgele Line'ı okuma ...
    
    memset(&lock,0,sizeof(lock));
    lock.l_type = F_UNLCK;
    lock.l_start = SEEK_SET;
    lock.l_whence = SEEK_END;
    lock.l_len = getFileSize(iValue);
    fcntl(fdForInput,F_SETLKW,&lock);

    do {
        int size = 0;
        int controlFile = open("xxx.txt",O_RDONLY,S_IWUSR | S_IRUSR);
        size = getFileSize("xxx.txt");
        //printf("SIZE : %d\n",size);
        if(size == 2) {
            write(controlFile,"X",strlen("X"));
            //ftruncate(fdForInput,0);
            return 0;
        }
        size = getFileSize("xxx.txt");
        if(size == 3) {
            close(controlFile);
            remove("xxx.txt");
            ftruncate(fdForInput,0);
            return 0;
        }
        readLine(fdForInput,randomLineNumber,getFileSize(iValue),iValue,timer, fdForOutput, lock);
    } while(1);
}


/* Close the files ...*/
void closeFiles(int filePointer1, int filePointer2)
{
  close(filePointer1);
  close(filePointer2);
}



/* Return the file line count ...*/
int newLineCount(int fd, int size) {
    int sz;
    char ch[1];
    int i= 0;
    int count = 0;
    size_t newLineIndex;
    lseek(fd, 0, SEEK_SET);
    do{
        sz = read(fd, ch, sizeof(ch));
        
        if((int)ch[0] == 10)
            count++;
    
        i++;
    } while (i < size);
  
  return count;
}

/* Read random line ...*/
void readLine(int fd, int randomLine, int fileSize, char* inputName, int timer, int outputFile, struct flock lock) {
    int sz,firstLine = 0,endLine = 0;
    char ch[1];
    int i= 0;
    int count = 0,control= 0,control2=0;
    lseek(fd, 0, SEEK_SET);
   
    lock.l_type = F_WRLCK; //ekledim
    //fcntl(fd,F_SETLKW,&lock);
    if(randomLine == 1) {
        sz = read(fd, ch, sizeof(ch));
        if((int)ch[0] == 10) {
            randomLine++; 
        } else {
            firstLine = 0;
            lseek(fd, 0, SEEK_SET);
            do {
                sz = read(fd, ch, sizeof(ch));
                if((int)ch[0] == 10)
                    break;

                i++;
            } while (i < fileSize);
            endLine = i + 1;
            control2 = 1;
        }
    }
    lseek(fd,i,SEEK_SET);
    i=0;
    if(control2 == 0) {
        do {
            sz = read(fd, ch, sizeof(ch));
            if((int)ch[0] == 10) {
                count++;
                if(count == randomLine - 1) {
                    do {
                        sz = read(fd, ch, sizeof(ch));
                        if((int)ch[0] != 10) {
                            control = 1;
                            break;
                        }
                        i++;
                    } while (i < fileSize);
                    if(control == 1)
                        break;
                }
            }
            i++;
        } while(i < fileSize);
        
        firstLine = i;
    
        lseek(fd, i+1, SEEK_SET);
        do{
            sz = read(fd, ch, sizeof(ch));
            if((int)ch[0] == 10)
                break;    
            i++;
        } while (i < fileSize);
        endLine = i+1;

        if(endLine - firstLine == 1) {
            lock.l_type = F_UNLCK; //ekledim
            fcntl(fd,F_SETLKW,&lock);
            return;
        }
    }
    
    lock.l_type = F_UNLCK; //ekledim
    //fcntl(fd,F_SETLKW,&lock);

    lock.l_type = F_WRLCK; //ekledim
    //fcntl(fd,F_SETLKW,&lock);

    // Dosyadan silinen satırı okuma sadece ...
    char* line = (char*) malloc(sizeof(char) * 150);
    lseek(fd,(off_t) firstLine,SEEK_SET);
    sz = read(fd, line, endLine - firstLine);
    
    lock.l_type = F_UNLCK; //ekledim
    //fcntl(fd,F_SETLKW,&lock);

    // FFT HESAPLAMA ISLEMLERI ...
    char str[10];
    PI = atan2(1, 1) * 4;
    complex double complexNumber[16];
    
    line = remchar(line, '+');
    line = remchar(line, 'i');
    

    line = replaceWord(line,", ", ",");
    line = replaceWord(line," ", ".");

    int j=0,k = 0;;
    for(int i=0; i< strlen(line); i++) {
        if(line[i] == ',') {
            
            char *ptr;
            double ret;
            ret = strtod(str, &ptr);
            // printf("SAYI : %lf\n",ret);
            complexNumber[k] = ret;
            j = 0;
            k++;
        } else {
            str[j] = line[i];
            j++;
        }
    }
    algorithmFFT(complexNumber,16);
    char *writeArr = (char*) malloc(sizeof(char)*500);
    for(int i = 0 ; i < 16; i++) {
        char* result2 = malloc(20*sizeof(char));
        if (!cimag(complexNumber[i]))
            sprintf(result2,"%.3lf, ", creal(complexNumber[i]));
        else
            sprintf(result2,"%.3lf +i%.3lf, ", creal(complexNumber[i]),cimag(complexNumber[i]));
        strcat(writeArr,result2);
    }

    lock.l_type = F_WRLCK; //ekledim
    //fcntl(fd,F_SETLKW,&lock);

    write(outputFile,writeArr,strlen(writeArr));
    write(outputFile,"\n",strlen("\n"));

    lock.l_type = F_UNLCK; //ekledim
    //fcntl(fd,F_SETLKW,&lock);

    //printf("\n");
    //printf("FirstLine : %d\nEndLiNE : %d\n",firstLine,endLine);

    // Dosyadan okuma yapıldı, dosyadan silme yapılması lazım ...
    // IF ELSE KALDIRILABİLİR ...
    
    lock.l_type = F_WRLCK; //ekledim
    //fcntl(fd,F_SETLKW,&lock);

    char* fileStr = (char *) malloc(sizeof(char) * (getFileSize(inputName) - endLine));
    lseek(fd,(off_t) endLine, SEEK_SET);
    read(fd ,fileStr,getFileSize(inputName) - endLine);
    ftruncate(fd,firstLine);
    lseek(fd,(off_t) firstLine, SEEK_SET);
    write(fd,"\n",strlen("\n"));
    write(fd,fileStr,strlen(fileStr));
    
    lock.l_type = F_UNLCK; //ekledim
    //fcntl(fd,F_SETLKW,&lock);
    
    /*
    lockRead.l_type = F_UNLCK;
    fcntl(fd,F_SETLKW,&lockRead);
    */
    /*
    lockWrite.l_type = F_UNLCK;
    fcntl(outputFile,F_SETLKW,&lockWrite);
    */

    printf("\n*****************************\n");
    printf("   Sleeping %d miliseconds    \n", timer);
    printf("*****************************\n");
    usleep(timer*1000);

    // free the array ...
    //free(line);
    //free(fileStr);
}

/* Get file size to file name ... */
size_t getFileSize(const char* fileName) {
  struct stat statX;
  stat(fileName, &statX);
  size_t size = statX.st_size;
  return size;
}

void FFTAlgorithm(complex input[], complex arr[], int n, int count) {
    if (count < n) {
        FFTAlgorithm(arr, input, n, count * 2);
        FFTAlgorithm(arr + count, input + count, n, count * 2);

        for (int i = 0; i < n; i += 2 * count) {
            complex t = cexp(-I * PI * i / n) * arr[i + count];
            input[i / 2]     = arr[i] + t;
            input[(i + n)/2] = arr[i] - t;
        }
    }
}

void algorithmFFT(complex arr[], int n) {
    complex ArrOut[n];
    for (int i = 0; i < n; i++) {
        ArrOut[i] = arr[i];
    }
    FFTAlgorithm(arr, ArrOut, n, 1);
}

char* remchar(char *s, char chr) {
   int i, j = 0;
   for ( i = 0; s[i] != '\0'; i++ ) {
      if ( s[i] != chr ) {
         s[j++] = s[i]; 
      }
   }
   s[j] = '\0'; 
   return s;
}

int replacechar(char *str, char orig, char rep) {
    for(int i=0; i<strlen(str); ++i) {
        if(str[i] == orig)
            str[i] = rep;
    }
    return 0;
}

char *replaceWord(const char *s, const char *oldW, 
                                 const char *newW) 
{ 
    char *result; 
    int i, cnt = 0; 
    int newWlen = strlen(newW); 
    int oldWlen = strlen(oldW); 
  
    // Counting the number of times old word 
    // occur in the string 
    for (i = 0; s[i] != '\0'; i++) 
    { 
        if (strstr(&s[i], oldW) == &s[i]) 
        { 
            cnt++; 
  
            // Jumping to index after the old word. 
            i += oldWlen - 1; 
        } 
    } 
  
    // Making new string of enough length 
    result = (char *)malloc(i + cnt * (newWlen - oldWlen) + 1); 
  
    i = 0; 
    while (*s) 
    { 
        // compare the substring with the result 
        if (strstr(s, oldW) == s) 
        { 
            strcpy(&result[i], newW); 
            i += newWlen; 
            s += oldWlen; 
        } 
        else
            result[i++] = *s++; 
    } 
  
    result[i] = '\0'; 
    return result; 
}