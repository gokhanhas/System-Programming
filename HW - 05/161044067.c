/*
 * Gokhan Has - 161044067
 * System Programming - HW #05
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#define FALSE 0
#define TRUE 1

// GLOBAL VARIABLES ...
// Florists ...

//  NOT, STRUCT KULLANIMINDA SINYAL GELDIGINDE DUZGUNCE TEMIZLENMEDIGI ICIN 
// GLOBAL POINTERLAR ILE ISLERI COZDUM ...

double* x_coord_for_florist = NULL;
double* y_coord_for_florist = NULL;
double* speedArray = NULL;
char** florists_Name = NULL;
char*** florists_sell_names = NULL;

typedef struct {
    int totalSells;
    int totalTime;
} FloristEndInfo;

//FloristEndInfo* infoArr = NULL;
int* floristIndexArr = NULL; 

// Clients
double* x_coord_for_clienst = NULL;
double* y_coord_for_clienst = NULL;
char** clients_name = NULL;
char** clients_flower = NULL;
int* distance = NULL;

// Condition Variables ...
pthread_cond_t* conditonArr = NULL;
pthread_mutex_t* mutexArr = NULL;
pthread_t* florist_threads = NULL;

pthread_mutex_t lock;

// ClientsQueue ...
int** clientsQueue = NULL;

static int totalClient = 0;
static int clientNo = 0;
static int floristCount_ = 0;
static int maxFlowerCount_ = 0;
static int whichThread = 0;
static int ifSIGINT = 0;
FILE* inputFileP = NULL;

struct sigaction new_sig_action;
sigset_t mask;


// Function define ...

// Hatali kullanim olursa programin düzgün kullanimini basar ...
void printUsage();

// Programin hata mesaji verip, temiz bir sekilde sonlandirilmasi gerekmektedir ...
void errorExit(char* error);

// Dosyadan cicekci sayisi alinir ... 
int getFloristCount(FILE * fptr, int* countMaxFLower);

// Arrayler initialize edilir. Her birine mallocla yer acilir ...
void initializeArrays(int count, int countClient, int maxFlowerCount);

// Program bittiginde veya ctrl+c ile sonlandirildiginda alinan yerler bu fonksiyon sayesinde geri verilir ...
void freeArrays(int count, int clientCount, int maxFlowerCount);

// Müsteri sayisi dondurulur ...
int getClientCount(FILE * fptr); 

// Dosya okunur ve arraylere alma islemi yapilir. Unutmamalidirki, istek listesi yine teker teker mainde olusturulacaktir ...
void readFile(FILE* fptr, int floristCount, int clientCount);

// Stringin icinde parametre olarak girilen degerden kac tane varsa o dondurulur ...
int getChCount(char* line, char ch);

// Threadlerin calistiracagi fonksiyondur ...
void* florist(void* index);

//  Istek listesi kisa olursa veya bazen maindeki istek kismi yapilip, interrupt girmeyip threadler calişmayabilir ...
// Bunu onlemek icin tekrardan threadlerdeki condition variable uyandirilmalidir ...
void endSignals(int floristCount);

// Chebyshev mesafesi hesaplanir, mainde en kisa olan mesafe ile islem yapilir ...
double getChebyShevDistance(double x1, double y1, double x2, double y2);

// Istek listesinde eleman var mi yok mu diye bakilir, eger yoksa thread uyutulmalidir ...
int isQueueHasElement(int index, int clientCount);

// En sondaki bilgilendirme mesajlari basilir ...
void printInfo(int count, FloristEndInfo* infoArr);

// Sinyal yakalama fonksiyonudur, SIGINT sinyali yakalanir...
void signal_catcher(int sigNo);


int main(int argc, char ** argv) {

    new_sig_action.sa_handler = signal_catcher;
    sigemptyset(&new_sig_action.sa_mask);
    new_sig_action.sa_flags = 0; // Sigaction set edilir ...
    
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


    inputFileP = fopen(iValue,"r");
    if(inputFileP == NULL) {
        perror("ERROR ! Program filePath (-i) ");
        exit(EXIT_FAILURE);
    }
    int i,j,k;
    FloristEndInfo* infoArr = NULL;
    srand(time(NULL));
    printf("Florist application initializing from file: %s\n", iValue);
    int maxFlowerCount = 0;
    int floristCount = getFloristCount(inputFileP, &maxFlowerCount);    
    int clientCount = getClientCount(inputFileP);
    totalClient = clientCount;
    floristCount_ = floristCount;
    maxFlowerCount_ = maxFlowerCount;
    infoArr = (FloristEndInfo*)malloc(sizeof(FloristEndInfo) * floristCount);
    for(i = 0; i < floristCount; i++) {
        infoArr[i].totalSells = 0;
        infoArr[i].totalTime = 0;
    }
    initializeArrays(floristCount, clientCount, maxFlowerCount);
    readFile(inputFileP,floristCount,clientCount);
    printf("%d florists have been created.\n", floristCount);
    
    // THREADLER YARATILACAK ...
    
    florist_threads = (pthread_t*) malloc(sizeof(pthread_t) * floristCount);
    floristIndexArr = (int*) malloc(sizeof(int) * floristCount);

    // INITIALIZE CONTION VARIABLE INIT ...
    for(i = 0; i < floristCount; i++) {
        if(pthread_cond_init(&conditonArr[i], NULL) != 0) {
            errorExit("ERROR pthread_cond_init ");
        }
        if(pthread_mutex_init(&mutexArr[i], NULL) != 0) {
            errorExit("ERROR pthread_mutex_init ");
        }
        floristIndexArr[i] = i;
    }
    
    for(i = 0; i < floristCount; i++) {
        for(j = 0; j < clientCount; j++) {
            clientsQueue[i][j] = -1;
        }
    }  

    if(pthread_mutex_init(&lock, NULL) != 0) {
        errorExit("ERROR pthread_mutex_init lock ");
    }

    for(i = 0; i < floristCount; i++) { // THREADLER OLUSTURULUR ...
        if(pthread_create(&florist_threads[i], NULL, florist, &floristIndexArr[i]) != 0) {
            errorExit("ERROR pthread_create ");
        }   
    }

    
    printf("Processing request\n");
    // TEKER TEKER REQUEST QUEUE OLUSTURMA ...
    sigfillset(&mask);
    sigprocmask(SIG_SETMASK,&mask,NULL);
    sigaction(SIGINT,&new_sig_action, NULL);
        
    for(i = 0; i < clientCount; i++) {
        int maxIndex = -1; 
        double maxVal = 9999999999.999;
        for(j = 0; j < floristCount; j++) {
            for(k = 0; k < maxFlowerCount; k++) {
                if(florists_sell_names[j][k] != NULL) {
                    if(strcmp(clients_flower[i],florists_sell_names[j][k]) == 0) {
                        double val = getChebyShevDistance(x_coord_for_clienst[i], y_coord_for_clienst[i], 
                                x_coord_for_florist[j], y_coord_for_florist[j]);
                        if(val < maxVal) {
                            maxIndex = j;
                            maxVal = val;
                        }
                    }
                }
            }
        }
        
        if(maxIndex != -1){
            // HANGI CICEKCI OLDUGU BULUNUR VE O THREADE SINYAL GONDERILIR ...
            pthread_mutex_lock(&mutexArr[maxIndex]); 
            pthread_mutex_lock(&lock); // CLIENT QUEUE E MUDAHALE ICIN AYRI BIR MUTEX TUTULMUSTUR ...
            clientsQueue[maxIndex][i] = 1;
            distance[i] = maxVal;
            pthread_mutex_unlock(&lock);       
            pthread_cond_signal(&conditonArr[maxIndex]); // HANGI CICEKCI CALISACAKSA, O THREADE SINYAL GONDERME ...
            pthread_mutex_unlock(&mutexArr[maxIndex]);   
        }
        else {
            // MUSTERININ ISTEDIGI CICEGI SATAN CICEKCI YOKTUR ....
            clientNo += 1;
        }
        
    }
    endSignals(floristCount);
    if(ifSIGINT == 0)
        printf("All requests processed.\n");
    void* arrInfo = NULL;
    for(i = 0; i < floristCount; i++) {
        // THREADLER BEKLENIR ...
        if(pthread_join(florist_threads[i], &arrInfo) != 0) {
            errorExit("ERROR pthread_join ");
        }
        infoArr[i].totalSells = ((FloristEndInfo*) arrInfo)->totalSells;
        infoArr[i].totalTime = ((FloristEndInfo*) arrInfo)->totalTime; 
        if(arrInfo != NULL)
            free(arrInfo);
        if(ifSIGINT == 0)
            printf("%s closing shop.\n",florists_Name[i]);
    }
    if(ifSIGINT == 0)
        printInfo(floristCount, infoArr);
    for(i = 0; i < floristCount; i++) {
        // C0ND VARIABLE DESTROY ...
        if(pthread_cond_destroy(&conditonArr[i]) != 0) {
            errorExit("ERROR pthread_cond_destroy ");
        }
        // MUTEX DESTROY ...
        if(errno = pthread_mutex_destroy(&mutexArr[i]) != 0) {
            errorExit("ERROR pthread_mutex_destroy ");
        }
    }

    // MUTEX DESTROY ...
    if(pthread_mutex_destroy(&lock) != 0) {
        errorExit("ERROR pthread_mutex_destroy lock ");
    }

    // CLOSE THE FILE ...
    if(errno = fclose(inputFileP) != 0) {
        errorExit("ERROR : fclose ");
    }

    if(infoArr != NULL)
        free(infoArr);
    // FREE ALL RESOURCES ...
    freeArrays(floristCount, clientCount, maxFlowerCount);
    return 0;
}

void* florist(void* index) {
    sigaction(SIGINT,&new_sig_action, NULL);
    FloristEndInfo* floristInfo = (FloristEndInfo*) malloc(sizeof(FloristEndInfo) * 1);
    floristInfo->totalTime = 0;
    floristInfo->totalSells= 0;
    int indexFlorist= *(int *) index;
    whichThread = indexFlorist;
    int clientIndex;
    
    // ILK DEFA CALISINCA BEKLEME KOSULU ...
    pthread_mutex_lock(&mutexArr[indexFlorist]);
    pthread_cond_wait(&conditonArr[indexFlorist], &mutexArr[indexFlorist]);
    pthread_mutex_unlock(&mutexArr[indexFlorist]);
    
    while(clientNo < totalClient) { // ISLENEN MUSTERI TOPLAM MUSTERIDEN ESIT VEYA BUYUKSE ISLEM YAPILMISTIR, CIKILMALIDIR ...
        while((isQueueHasElement(indexFlorist,totalClient) != -1)) { 
            pthread_mutex_lock(&mutexArr[indexFlorist]);
            
            if(ifSIGINT == 0) {
                // COND VARIABLE WAIT ...
                pthread_cond_wait(&conditonArr[indexFlorist], &mutexArr[indexFlorist]);
            }
                
            if(ifSIGINT == 1) {
                // CTRL C SINYALI GELMISTIR THREAD CIKMALIDIR ...
                pthread_mutex_unlock(&mutexArr[indexFlorist]);
                break;
            }
            
            if(ifSIGINT == 0) {
                // HESAFELERIN HESAPLANMASI VE EKRANA BASILMASI ... 
                clientIndex = isQueueHasElement(indexFlorist,totalClient);
                int preTime = rand() % 250 + 1;
                int deliverTime =  distance[clientIndex] / (int)speedArray[indexFlorist];
                floristInfo->totalSells += 1;
                floristInfo->totalTime += (preTime + deliverTime);
                usleep((preTime+deliverTime) * 1000);
                if(ifSIGINT == 0) 
                    printf("Florist %7s has delivered a %8s to %10s in %4dms\n",florists_Name[indexFlorist],
                        clients_flower[clientIndex], clients_name[clientIndex], (preTime+deliverTime));    
                fflush(stdout);    
                pthread_mutex_lock(&lock);
                clientsQueue[indexFlorist][clientIndex] = -1; // CLIENT QUEUE DEN YAPILAN ISLEM SILINIR ...
                pthread_mutex_unlock(&lock);
            }
            clientNo += 1;
            pthread_mutex_unlock (&mutexArr[indexFlorist]);
        } 
        if(ifSIGINT == 1) // CTRL+C SINYALI GELDIGINDE DONGUDEN CIKILMALIDIR ...
            break;
    }
    return floristInfo;
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
  print = "#  -i : Data file parameter to floristApp   #\n";
  write(STDOUT_FILENO,print,strlen(print));
  print = "#############################################\n\n";
  write(STDOUT_FILENO,print,strlen(print));  
  print = NULL;
}

int getFloristCount(FILE * fptr, int* countMaxFLower) {
    char line[255];
    int flowerCount = 0;
    int max = 0;
    while(fgets(line,255,fptr)) {
        if(!strcmp(line,"\n")) {
            break;
        }
        int returnCount = getChCount(line, ',');
        if(returnCount > max) {
            max = returnCount;
        }
        flowerCount++;
    }
    *countMaxFLower = max;
    return flowerCount;
}

int getClientCount(FILE * fptr) {
    char line[255];
    int clientCount = 0;
    while(fgets(line,255,fptr)) {
        if(!strcmp(line,"\n"))
            break;
        clientCount++;
    }
    fseek(fptr, 0, SEEK_SET);
    return clientCount;
}

void initializeArrays(int count, int countClient, int maxFlowerCount) {
    
    // FOR FLORISTS ...
    x_coord_for_florist = (double*) malloc(sizeof(double) * count);
    y_coord_for_florist = (double*) malloc(sizeof(double) * count);
    speedArray = (double*) malloc(sizeof(double) * count);
    conditonArr = (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * count);
    mutexArr = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * count);
    int i = 0, j = 0;
    florists_Name = (char**) malloc(sizeof(char*) * count);
    for(i = 0; i < count; i++)     
        florists_Name[i]  = (char*) malloc(sizeof(char) * 20);
    
    florists_sell_names = (char***) malloc(count * sizeof(char **));
    for(i = 0; i < count; i++) {
        florists_sell_names[i] = (char**) malloc(sizeof(char*) * maxFlowerCount);
    }
    
    for(i = 0; i < count; i++) {
        for(j = 0; j < maxFlowerCount; j++) {
            florists_sell_names[i][j] = (char*) malloc(sizeof(char) * 20);
            strcat(florists_sell_names[i][j],"none");
        }
    }

    // FOR CLIENTS ...
    x_coord_for_clienst = (double*) malloc(sizeof(double) * countClient);
    y_coord_for_clienst = (double*) malloc(sizeof(double) * countClient);
    distance = (int *) malloc(sizeof(int) * countClient);


    clients_name = (char**) malloc(sizeof(char*) * countClient);
    for(i = 0; i < countClient; i++) 
        clients_name[i] = (char*) malloc(sizeof(char) * 50);
    
    clients_flower = (char**) malloc(sizeof(char*) * countClient);
    for(i = 0; i < countClient; i++) 
        clients_flower[i] = (char*) malloc(sizeof(char) * 50);

    
    clientsQueue = (int**) malloc(sizeof(int*) * count);
    for(i = 0; i < count; i++)
        clientsQueue[i] = (int*) malloc(sizeof(int) * countClient);
}

void freeArrays(int count, int clientCount, int maxFlowerCount) {
    
    if(x_coord_for_florist != NULL)
        free(x_coord_for_florist);
    if(y_coord_for_florist != NULL)
        free(y_coord_for_florist);
    if(speedArray != NULL)
        free(speedArray);
    if(conditonArr != NULL)
        free(conditonArr);
    int i = 0, j = 0;

    if(mutexArr != NULL)
        free(mutexArr);
    
   
    for(i = 0; i < count; i++) {
        if(florists_Name[i] != NULL)
            free(florists_Name[i]);
    }
    if(florists_Name != NULL)
        free(florists_Name);


    for(i = 0; i < count; i++) {
        for(j = 0; j < maxFlowerCount; j++) {
            if(florists_sell_names[i][j] != NULL)
                free(florists_sell_names[i][j]);
        }
        
        free(florists_sell_names[i]);
    }

    if(florists_sell_names != NULL)    
        free(florists_sell_names);

    if(x_coord_for_clienst != NULL)
        free(x_coord_for_clienst);
    if(y_coord_for_clienst != NULL)
        free(y_coord_for_clienst);

    for(i = 0; i < clientCount; i++) {
        if(clients_name[i] != NULL)
            free(clients_name[i]);
        if(clients_flower[i] != NULL)
            free(clients_flower[i]);
    }
    if(clients_name != NULL)
        free(clients_name);
    if(clients_flower != NULL)
        free(clients_flower);

    
    for(i = 0; i < count; i++) {
        if(clientsQueue[i] != NULL)
            free(clientsQueue[i]);
    }
        
    if(clientsQueue!= NULL)
        free(clientsQueue);
    if(distance != NULL)
        free(distance);
    

    if(florist_threads != NULL)
        free(florist_threads);
    if(floristIndexArr != NULL)
        free(floristIndexArr);
    
}

void readFile(FILE* fptr, int floristCount, int clientCount) {
    char line[400];
    
    int index = 0;
    while(fgets(line,255,fptr)) {
     
        if(!strcmp(line,"\n")) 
            break;
        char name[20];
        double xCoord = 0, yCoord = 0;
        double avgSpeed = 0.0;
        
        sscanf(line,"%s (%lf,%lf; %lf) :",name, &xCoord, &yCoord, &avgSpeed);

        x_coord_for_florist[index] = xCoord;
        y_coord_for_florist[index] = yCoord;
        speedArray[index] = avgSpeed;
        strcpy(florists_Name[index], name);
        
        int i, j = 0;
        for (i = 0; line[i] != ':'; ++i) {
            j++;
        }
        j += 2;
        char flowerOneName[20];
        int k = 0, kIndex = 0;;
        for (i = j; line[i] != '\n'; ++i) {            
            if(line[i] == ',') {
                strncpy(florists_sell_names[index][kIndex], flowerOneName, k);    
                kIndex++; 
                k = 0;
                i++;   
            } else if(line[i+1] == '\n') {
                flowerOneName[k] = line[i];
                k++;
                strncpy(florists_sell_names[index][kIndex], flowerOneName, k);    
            } 
            else {
                flowerOneName[k] = line[i];
                k++;
            }
        }
        index++;
    }
    index = 0;
    while(fgets(line,255,fptr)) {
        if(!strcmp(line,"\n"))
            break;
        char name[40], flowerName[20];
        double xCoord, yCoord;
        sscanf(line,"%s (%lf,%lf): %s\n",name, &xCoord, &yCoord, flowerName);
        strcpy(clients_name[index], name);
        x_coord_for_clienst[index] = xCoord;
        y_coord_for_clienst[index] = yCoord;
        strcpy(clients_flower[index], flowerName);
        index++;
    }
}

int getChCount(char* line, char ch) {
    int count = 0;
    for (int i = 0; line[i] != '\0'; ++i) {
        if (ch == line[i])
            ++count;
    }
    return count;
}

double getChebyShevDistance(double x1, double y1, double x2, double y2) {
    
    double val1 = fabs(x1 - x2);
    double val2 = fabs(y1 - y2);

    if(val1 > val2)
        return val1;
    return val2;
}

int isQueueHasElement(int index, int clientCount) {
    int i;
    for(i = 0; i < clientCount; i++) {
        if(clientsQueue[index][i] != -1)
            return i;
    }
    return -1;
}

void endSignals(int floristCount) {
    int i;
    while(!(clientNo == totalClient)) {
        if(ifSIGINT == 1) break; 
        for(i = 0; i < floristCount; i++) {
            pthread_cond_signal(&conditonArr[i]);
        }
    }
    // BAZI THREADLER CTRL+C GELDIGINDE HALA UYUYOR OLABILIRLER, ONLARIN UYANMASI ICIN GEREKLIDIR ....
    for(i = 0; i < floristCount; i++) {
        pthread_cond_signal(&conditonArr[i]);
    }
}

void printInfo(int count, FloristEndInfo* infoArr) {
    printf("Sale statistics for today: \n");
    printf("---------------------------------------------\n");
    printf("Florist        # of sales         Total Time \n");
    printf("---------------------------------------------\n");
    int i;
    for(i = 0; i < count; i++) {
        printf("%s\t\t %4d\t\t  %6dms\n", florists_Name[i], infoArr[i].totalSells, infoArr[i].totalTime);
    }
    printf("---------------------------------------------\n");
}

void signal_catcher(int sigNo) {
    if(sigNo == SIGINT) {
        printf("SIGINT WILL BE CAUGHT\n");        
        ifSIGINT = 1; // FLAG ISARETLERNIR VE ARTIK THREADLERIN DUZGUNCE BITMESI GEREKMEKTEDIR ...
        printf("PROGRAM WILL BE FINISHED, PLEASE BE PATIENT ...\n");
    }
}