/* 
  CSE 344 - SYSTEN PROGRAMMING - HOMEWORK #2
  Gokhan Has - 161044067 
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <math.h>

// Function define ...

// Bu fonksiyon kullanıcıya programın kullanımı hakkında bilgi verir ...
void printUsage();

// Bu fonksiyon verilen 10 noktadan gecen bir doğru denklemi bulur ...
void least_square_method(unsigned int *xArray, unsigned int *yArray, int number, char str[]);

// Bu fonksiyon hata parametrelerini hesaplar ...
void calculate_MAE_MSE_RMSE(char *str, char *new, double meanArr[], double sdArr[], int line_count);

// Verilen stringin bir barçasını baska parca ile degistirir ...
char *replaceWord(const char *s, const char *oldW, const char *newW);

// Hata paremetrelerini hesaplayan fonksiyonlar ...
double calculate_MAE(double arr[]);
double calculate_MSE(double arr[]);
double calculate_RMSE(double MSE);

// Parent process'in kac byte okudugunu vs ekrana yazan bilgi fonksiyonudur ...
void parentProcessTerminated(int number, int szx);

// Standart sapma hesaplayan fonksiyondur ...
double find_SD(double data[], int number);

// Child process error metriclerini ekrana yazan fonksiyondur ...
void printErrorMetric(double *meanArr, double *sdArr, int number);

// Verilen dosyanın kac satırdan olustugunu döndürür ...
int getNewLineCount(int fd, char *fileName);

// Verilen dosya isminin boyutunu döndürür ...
size_t getFileSize(const char *fileName);

// Dosyalarini kapatir ...
void closeFiles(int filePointer1, int filePointer2);

int fdForInput, fdForOutput, fdTempFile;
char tempFile[] = "tempFileXXXXXX";
char *iValue;
char *oValue;
static int isDeleted = 0;
static int howManyBytes = 0;
int signalIndex = 0;
int signals[50];

void catcher(int signum)
{
  switch (signum)
  {
  case SIGUSR1:
  {
    isDeleted = 1;
    break;
  }
  case SIGTERM:
  {
    close(fdForInput);
    close(fdForOutput);
    close(fdTempFile);
    if (iValue != NULL)
      unlink(iValue);
    unlink(tempFile);
    printf("\n---> SIGTERM IS CATCHED <---\n");
    exit(0);
  }
  default:
    break;
  }
}

void criticalRegionCatch(int signum)
{
  signals[signalIndex] == signum;
  signalIndex++;
}

int main(int argc, char **argv)
{

  struct flock lock;
  struct stat stats;
  int durum;
  iValue = NULL;
  oValue = NULL;
  int Count = 0;
  int timer = 0;

  int iIndex = 0;
  int oIndex = 0;

  int another = 0;
  int c;

  // opterr = 0;

  while ((c = getopt(argc, argv, "i:o:")) != -1)
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
    case '?':
      another += 1;
      break;
    default:
      abort();
    }

  // printf("inputPathA = %s, outputPathA = %s, time = %s\n", iValue, oValue, tValue);

  /* Error handling in command line arguments ... */
  if (iIndex == 1 && oIndex == 1 && another == 0)
  {
    // DO NOTHING ... ;
  }
  else
  {
    errno = EIO;
    perror("ERROR ");
    printUsage();
    return 0;
  }

  // Input file operation ...
  fdForInput = open(iValue, O_RDONLY);
  if (fdForInput == -1)
  {
    perror("ERROR ! Program Input ");
    close(fdForInput);
    return 0;
  }

  // Output file operation ...
  fdForOutput = open(oValue, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
  if (fdForOutput == -1)
  {
    perror("ERROR ! Program Output ");
    closeFiles(fdForInput, fdForOutput);
    return 0;
  }

  // Temporary File operation ...
  fdTempFile = mkstemp(tempFile);
  if (fdTempFile == -1)
  {
    perror("ERROR ! Program Temporary File ");
    closeFiles(fdForInput, fdForOutput);
    close(fdTempFile);
    return 0;
  }

  // GLOBAL VARIABLES ....
  int sz;
  unsigned int x_array[10] = {0};
  unsigned int y_array[10] = {0};
  unsigned char *charArr20 = (unsigned char *)calloc(20, sizeof(char));
  char *result;
  char *newline;

  sigset_t sigset;
  struct sigaction sact;

  // FORK, CREATE CHILD PROCESS ...
  signal(SIGTERM, catcher);
  pid_t pID = fork();
  if (pID == 0)
  {
    // CHILD PROCESS ... (P2)
    printf("\n### CHILD PROCESS IS STARTED ###\n");
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = catcher;

    if (sigaction(SIGUSR1, &sact, NULL) != 0)
      perror("ERROR : SIGACTION \n");

    sigfillset(&sigset);
    sigdelset(&sigset, SIGUSR1);
    sigsuspend(&sigset);
    //close(fdForInput);
    //unlink(iValue);

    char oneCharacter;
    int sz;
    char charArr[1];
    char *lineStr;

    int counterLine = getNewLineCount(fdTempFile, tempFile);
    double mean_arr[counterLine];
    double sd_arr[counterLine];
    
    lseek(fdTempFile, 0, SEEK_SET);
    int lineCount = 0;

    while (getFileSize(tempFile) > 0)
    {

      lseek(fdTempFile, 0, SEEK_SET);
      memset(&lock, 0, sizeof(lock));
      lock.l_type = F_RDLCK;
      lock.l_start = 0;
      lock.l_whence = SEEK_END;
      stat(tempFile, &stats);
      lock.l_len = stats.st_size;
      fcntl(fdTempFile, F_SETLKW, &lock);
      //printf("\n CHILD KİTLEDİ \n");

      lineStr = (char *)malloc(500 * sizeof(char));
      int counter = 0;
      newline = (char *)malloc(500 * sizeof(char));
      lseek(fdTempFile, 0, SEEK_SET);
      do
      {
        sz = read(fdTempFile, charArr, sizeof(charArr));
        if ((int)charArr[0] >= 0)
          lineStr[counter] = charArr[0];
        counter++;
      } while ((int)charArr[0] != 10);
      lineStr[counter - 1] = ',';
      lineStr[counter] = '\0';
      // CRITICAL REGION STARTS ...
      calculate_MAE_MSE_RMSE(lineStr, newline, mean_arr, sd_arr, lineCount);
      strcat(lineStr, newline);
      //CTRITICAL REGION ENDS ...
      lineCount++;

      lseek(fdForOutput, 0, SEEK_END);
      write(fdForOutput, lineStr, strlen(lineStr));

      lseek(fdTempFile, 0, SEEK_SET);
      int fsize = getFileSize(tempFile);

      if (fsize - counter - 1 < 0)
      {
        truncate(tempFile, 0);
        break;
      }
      char *tempStr = (char *)malloc((fsize - counter - 1) * sizeof(char));

      lseek(fdTempFile, counter, SEEK_SET);
      sz = read(fdTempFile, tempStr, (fsize - counter - 1));

      truncate(tempFile, 0);

      lseek(fdTempFile, 0, SEEK_SET);
      write(fdTempFile, tempStr, strlen(tempStr));
      write(fdTempFile, "\n", strlen("\n"));

      // DOSYAYI ACMA ...
      lock.l_type = F_UNLCK;
      fcntl(fdTempFile, F_SETLKW, &lock);
      //printf("\n CHILD ACILDI \n");

      lseek(fdTempFile, 0, SEEK_SET);
    }

    printErrorMetric(mean_arr, sd_arr, counterLine);

    free(newline);
    free(lineStr);
    printf("### CHILD PROCESS IS FINISHED ###\n");
  }
  else if (pID == -1)
  {
    perror("ERROR : FORK PROBLEM");
    exit(EXIT_FAILURE);
  }
  else
  {
    // PARENT PROCESS ... (P1)
    printf("\n### PARENT PROCESS IS STARTED ### \n");
    int count = 0;

    do
    {
      sz = read(fdForInput, charArr20, 20);
      
      if (sz < 20)
      {
        howManyBytes += sz;
        free(result);
        if (signalIndex != 0) {
          printf("#### CRITICAL REGION SIGNAL HANDLER ###\n");
          for (int j = 0; j < signalIndex; j++) {
            printf("Signal No : %d\n", signals[j]);
          }
        }
        else {
          printf("\n--> No signal in critical region \n");
        }
        kill(pID, SIGUSR1);
        break;
      }
      count++;
      howManyBytes += 20;
      int x_index = 0, y_index = 0;
      for (int i = 0; i < sz; i++)
      {
        if (i % 2 == 0)
        {
          x_array[x_index] = (unsigned int)charArr20[i];
          x_index++;
        }
        else
        {
          y_array[y_index] = (unsigned int)charArr20[i];
          y_index++;
        }
      }

      char line[20], coord[30];
      least_square_method(x_array, y_array, 10, line);

      result = (char *)malloc(150 * sizeof(char));

      for (int i = 0; i < 10; i++)
      {
        sprintf(coord, "(%d,%d),", x_array[i], y_array[i]);
        strcat(result, coord);
      }
      strcat(result, line);

      lseek(fdTempFile, 0, SEEK_SET);
      memset(&lock, 0, sizeof(lock));
      lock.l_type = F_RDLCK;
      lock.l_start = 0;
      lock.l_whence = SEEK_END;
      stat(tempFile, &stats);
      lock.l_len = stats.st_size;
      fcntl(fdTempFile, F_SETLKW, &lock);

      lseek(fdTempFile, 0, SEEK_END);
      write(fdTempFile, result, strlen(result));

      lock.l_type = F_UNLCK;
      fcntl(fdTempFile, F_SETLKW, &lock);

      if (Count == 2)
      {
        kill(pID, SIGUSR2);
      }

    } while (sz == 20);
    parentProcessTerminated(howManyBytes, sz);
    pID = wait(NULL);

    // Parentı artık sonlandırıyorum, kendi yazdığım catcher fonksiyonu sayesinde ...
    kill(getpid(), SIGTERM);
  }
  return 0;
}

/* Close the files ...*/
void closeFiles(int filePointer1, int filePointer2)
{
  close(filePointer1);
  close(filePointer2);
}

void printUsage()
{
  printf("\n");
  printf("####################### PROGRAM USAGE #######################\n");
  printf("#              Programs argument is -i and -o               #\n");
  printf("#  -i : read the contents of the file denoted by inputPath  #\n");
  printf("#  -o :        write output to the file denoted             #\n");
  printf("#############################################################\n\n");
}

void least_square_method(unsigned int *xArray, unsigned int *yArray, int number, char str[]){

  // SİNYALLER BURAYA GELECEK ...
  sigset_t int_mask;
  struct sigaction sact;


  if ((sigemptyset(&int_mask) == -1) || (sigaddset(&int_mask, SIGINT) == -1) || (sigaddset(&int_mask, SIGSTOP) == -1))
  {
    perror("ERROR : INITIALIZE THE SIGNAL MASK :  FAILED !!");
    exit(EXIT_FAILURE);
  }

  if (sigprocmask(SIG_BLOCK, &int_mask, NULL) == -1)
  {
    perror("ERROR : SIGNALS ARE NOT BLOCKED !!");
    exit(EXIT_FAILURE);
  }
  //printf("\nCRITICAL REGION STARTS : PARENT PROCESS\n");
  // CRITICAL REGION STARTS ...

  for(int j=1; j < 32; j++) {
    if(j != 15) // SIGTERM IS NOT CATCHED IN HERE ....
      signal(j,criticalRegionCatch);
  }
    
 
 
 
  double sum_x = 0, sum_x_square = 0;
  double sum_y = 0, sum_xy = 0;

  int i = 0;
  while (i < number)
  {
    sum_x += xArray[i];
    sum_x_square += xArray[i] * xArray[i];

    sum_y += yArray[i];
    sum_xy += xArray[i] * yArray[i];

    i++;
  }

  // ax + b ...
  double a = (1.0 / (number * sum_x_square - sum_x * sum_x)) * (sum_xy * number - sum_y * sum_x);
  double b = (1.0 / (number * sum_x_square - sum_x * sum_x)) * (sum_x_square * sum_y - sum_xy * sum_x);

  sprintf(str, "%.3lfx+%.3lf\n", a, b);

  //printf("\nCRITICAL REGION ENDS   : PARENT PROCESS\n");
  // CRITICAL REGION ENDS ...

  if (sigprocmask(SIG_UNBLOCK, &int_mask, NULL) == -1)
  {
    perror("ERROR : SIGNALS ARE NOT UNBLOCKED !!");
    exit(EXIT_FAILURE);
  }
}

void calculate_MAE_MSE_RMSE(char *str, char *new, double meanArr[], double sdArr[], int line_count)
{

  char *newString = (char *)malloc(500 * sizeof(char));

  int x_array[10], y_array[10];
  double MAE = 0.0, MSE = 0.0, RMSE = 0.0;
  double resultArr[10];
  double A = 0.0, B = 0.0;

  str = replaceWord(str, ",(", "");
  str = replaceWord(str, "(", "");
  str = replaceWord(str, ")", " ");

  char *xVal = (char *)malloc(3 * sizeof(char));
  char *yVal = (char *)malloc(3 * sizeof(char));
  char *AVal = (char *)malloc(10 * sizeof(char));
  char *BVal = (char *)malloc(10 * sizeof(char));
  int commaCount = 0;
  int control = 0, x_index = 0, y_index = 0;
  int xIndex = 0, yIndex = 0;
  int aIndex = 0, bIndex = 0;

  for (int i = 0; i < strlen(str); i++)
  {
    if (str[i] == ',')
    {
      // Y DEGERINE GEC ...
      commaCount++;
      control = 1;

      if (x_index <= 2)
        xVal[x_index] = '\0';

      x_array[xIndex] = atoi(xVal);
      xIndex++;

      x_index = 0;
      y_index = 0;
    }
    else if (str[i] == ' ')
    {
      // Tekrar X'e geç ...
      control = 0;

      if (y_index <= 2)
        yVal[y_index] = '\0';

      y_array[yIndex] = atoi(yVal);
      yIndex++;

      x_index = 0;
      y_index = 0;
    }

    else if (commaCount == 11)
    {
      // Artık AX+B denklemine geç ...
      int controlAorB = 0;
      int size = strlen(str);
      while (i < size)
      {
        if (str[i] == '+')
        {

          controlAorB = 1; // B'YE GEC ...

          if (aIndex <= 9)
            AVal[aIndex] = '\0';

          A = atof(AVal);
          bIndex = 0;
        }
        else
        {
          if (controlAorB == 0)
          {
            // A'YI YAP ...
            AVal[aIndex] = str[i];
            aIndex++;
          }
          else
          {
            BVal[bIndex] = str[i];
            bIndex++;
          }
        }
        i++;
      }

      if (bIndex <= 9)
        BVal[bIndex] = '\0';

      B = atof(BVal);
      break;
    }
    else
    {
      if (control == 0)
      {
        xVal[x_index] = str[i];
        x_index++;
      }
      else
      {
        yVal[y_index] = str[i];
        y_index++;
      }
    }
  }

  for (int i = 0; i < 10; i++)
  {
    double y = A * (x_array[i]) + B;

    resultArr[i] = y_array[i] - y;
  }

  char *calculateString = (char *)malloc(50 * sizeof(char));

  sigset_t int_mask;

  // CRITICAL REGION STARTS ....
  if ((sigemptyset(&int_mask) == -1) || (sigaddset(&int_mask, SIGINT) == -1) || (sigaddset(&int_mask, SIGSTOP) == -1))
  {
    perror("ERROR : INITIALIZE THE SIGNAL MASK :  FAILED !!");
    exit(EXIT_FAILURE);
  }

  if (sigprocmask(SIG_BLOCK, &int_mask, NULL) == -1)
  {
    perror("ERROR : SIGNALS ARE NOT BLOCKED !!");
    exit(EXIT_FAILURE);
  }

  // CALCULATE MAE, MSE and RMSE ...
  MAE = calculate_MAE(resultArr);
  MSE = calculate_MSE(resultArr);
  RMSE = calculate_RMSE(MSE);

  if (sigprocmask(SIG_UNBLOCK, &int_mask, NULL) == -1)
  {
    perror("ERROR : SIGNALS ARE NOT UNBLOCKED !!");
    exit(EXIT_FAILURE);
  }
  // CRITICAL REGION ENDS ...

  double data[3];
  data[0] = MAE;
  data[1] = MSE;
  data[2] = RMSE;

  meanArr[line_count] = (MAE + MSE + RMSE) / 3.0;
  sdArr[line_count] = find_SD(data, 3);

  //printf("ERROR MEAN : %10.3lf\t\tERROR SD : %10.3lf\n",meanArr[line_count],sdArr[line_count]);

  sprintf(calculateString, " %.3lf, %.3lf, %.3lf\n", MAE, MSE, RMSE);
  strcat(new, calculateString);

  free(xVal);
  free(yVal);
  free(AVal);
  free(BVal);
  free(calculateString);
}

double calculate_MAE(double arr[])
{
  double MAE = 0.0;
  for (int i = 0; i < 10; i++)
  {

    double result = arr[i];
    if (result < 0)
      result = result * (-1.0);

    MAE += result;
  }
  MAE = (1.0 / 10.0) * MAE;
  return MAE;
}

double calculate_MSE(double arr[])
{
  double MSE = 0.0;
  for (int i = 0; i < 10; i++)
  {
    double result = arr[i];
    MSE += result * result;
  }
  MSE = (1.0 / 10.0) * MSE;
  return MSE;
}

double calculate_RMSE(double MSE)
{
  return sqrt(MSE);
}

void printErrorMetric(double *meanArr, double *sdArr, int number)
{
  printf("\n");
  for (int i = 0; i < number; i++)
  {
    printf("Mean Err : %9.3lf\t\tSD Err : %9.3lf\n", meanArr[i], sdArr[i]);
  }
  printf("\n");
}

void parentProcessTerminated(int number, int szx)
{
  printf("\n##################################################\n");
  printf("#              READED BYTES  : %5d             #\n", number);
  printf("#        ESTIMATED LINE EQUATION  : %5d        #\n", (int)((number - szx) / 20));
  printf("##################################################\n\n");
}

/* Get file size to file name ... */
size_t getFileSize(const char *fileName)
{
  struct stat statX;
  stat(fileName, &statX);
  size_t size = statX.st_size;
  return size;
}

char *replaceWord(const char *s, const char *oldW, const char *newW)
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

double find_SD(double data[], int number)
{
  double sum = 0.0, mean, SD = 0.0;
  int i;
  for (i = 0; i < number; ++i)
  {
    sum += data[i];
  }
  mean = (double)(sum / 3.0);
  for (i = 0; i < number; ++i)
    SD += pow(data[i] - mean, 2);
  return sqrt(SD / number);
}

int getNewLineCount(int fd, char *fileName)
{
  lseek(fd, 0, SEEK_SET);
  int counter = 0, fileSize = 0;
  char readedBytes[1];
  while (fileSize != getFileSize(fileName))
  {
    int sz = read(fd, readedBytes, 1);
    if (readedBytes[0] == '\n')
      counter++;
    fileSize++;
  }
  return counter;
}