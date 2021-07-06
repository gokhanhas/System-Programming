/*
*   CSE 344 - SYSTEM PROGRAMMING - HW #03
*   Gokhan Has - 161044067
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/wait.h>
#include <signal.h>

// ########################### ASSAGIDAKI DEGERLER SINGULAR VALUE HESAPLAMADA KULLANILACAKTIR #########################
#define NR_END 1
#define FREE_ARG char*
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
static double dmaxarg1,dmaxarg2;
#define DMAX(a,b) (dmaxarg1=(a),dmaxarg2=(b),(dmaxarg1) > (dmaxarg2) ?\
(dmaxarg1) : (dmaxarg2))
static int iminarg1,iminarg2;
#define IMIN(a,b) (iminarg1=(a),iminarg2=(b),(iminarg1) < (iminarg2) ?\
(iminarg1) : (iminarg2))

// Singular value fonksiyonlari ...
double **dmatrix(int nrl, int nrh, int ncl, int nch);
double *dvector(int nl, int nh);
void free_dvector(double *v, int nl, int nh);
double pythag(double a, double b);
void svdcmp(double **a, int m, int n, double w[], double **v);
// ####################################################################################################################


// Global Variables ....
int pid_1 = -1, pid_2 = -1, pid_3 = -1, pid_4 = -1, parentProcessId;

// Functions define ...
// Bu fonksiyon verilen dosya isminin boyutunu return eder ...
size_t getFileSize(const char* fileName);

// Bu fonksiyon, programin kullanimini basar ...
void printUsage();

// Bu fonksiyon, dosyaları kapatir ...
void closeFiles(int fd1, int fd2);

// Bu fonksiyonlar sonuc matrix inin ceyrek degerlerini hesaplar ...
int* calculate_C11(char* str, int total);
int* calculate_C12(char* str, int total);
int* calculate_C21(char* str, int total);
int* calculate_C22(char* str, int total);

// Bu fonksiyon iki matrixi carpar ve tek boyutlu array olarak dondurur ...
int* matrix_multi(int n,int matrix1[][n], int matrix2[][n]);

// Bu fonksiyon iki matrix i toplayip, tek boyutlu array olarak dondurur ...
int* matrix_sum(int n, int* arr1, int* arr2);

void catcher(int signalNo) {
    if(signalNo == 2) {
        printf("SIGINT is caught\n");
        exit(EXIT_SUCCESS);
    }

    else if(signalNo == 17) {
        printf("SIGCHLD is caught \n");
    }
}


int main(int argc, char **argv) {
    
    parentProcessId = getpid();
    signal(SIGINT,catcher);
    signal(SIGCHLD,catcher);
    printf("\n");
    
    char *iValue = NULL;
    char *jValue = NULL;
    char *nValue = NULL;
    int n = 0;

    int iIndex = 0;
    int jIndex = 0;
    int nIndex = 0;

    int another = 0;
    int c;

    while ((c = getopt(argc, argv, "i:j:n:")) != -1)    
    switch (c) {
        case 'i':
            iIndex += 1;
            iValue = optarg;
            break;
        case 'j':
            jIndex += 1;
            jValue = optarg;
            break;
        case 'n':
            nIndex += 1;
            nValue = optarg;
            break;
        case '?':
            another += 1;
            break;
        default:
            abort();
    }

    // printf("inputPathA = %s, outputPathA = %s, time = %s\n", iValue, oValue, tValue);

    /* Error handling in command line arguments ... */
    if (iIndex == 1 && jIndex == 1 && nIndex == 1 && another == 0) {
        n = atoi(nValue);
        if (n <= 0) {
            errno = EINVAL;
            perror("ERROR (-n) ");
            exit(EXIT_FAILURE);
        }
    }
    else {
        errno = EIO;
        perror("ERROR ");
        printUsage();
        exit(EXIT_FAILURE);
    }

  
    // Input fileA operation ...
    int fdForInputA = open(iValue, O_RDONLY);
    if (fdForInputA == -1) {
        perror("ERROR ! Program InputPathA ");
        exit(EXIT_FAILURE);
    }

  
    // Input fileB operation ...
    int fdForInputB = open(jValue, O_RDONLY);
    if (fdForInputB  == -1) {
        perror("ERROR ! Program InputPathB ");
        if(close(fdForInputA) == -1) {
            perror("ERROR ! InputpathA does not close ");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    } 

    int totalReadedBytes = pow(2,n) * pow(2,n);

    // Dosyalarini basina gelinmesi ...
    lseek(fdForInputA, 0, SEEK_SET);
    if(getFileSize(iValue) < totalReadedBytes) {
        errno = EIO;
        perror("ERROR ! There is not enough bytes in inputPathA "); 
        closeFiles(fdForInputA,fdForInputB);
        exit(EXIT_FAILURE);
    }

    lseek(fdForInputB, 0, SEEK_SET);
    if(getFileSize(jValue) < totalReadedBytes) {
        errno = EIO;
        perror("ERROR ! There is not enough bytes in inputPathB ");
        closeFiles(fdForInputA,fdForInputB);
        exit(EXIT_FAILURE);
    }

    char * readedInputA = (char *) malloc(sizeof(char) * totalReadedBytes);
    char * readedInputB = (char *) malloc(sizeof(char) * totalReadedBytes);

    if(read(fdForInputA,readedInputA,totalReadedBytes) == -1) {
        perror("ERROR ! read inputPathA ");
        closeFiles(fdForInputA,fdForInputB);
        exit(EXIT_FAILURE);
    }

    lseek(fdForInputB, 0, SEEK_SET);
    if(read(fdForInputB,readedInputB,totalReadedBytes) == -1) {
        perror("ERROR ! read inputPathB ");
        closeFiles(fdForInputA,fdForInputB);
        exit(EXIT_FAILURE);
    }
    
    closeFiles(fdForInputA,fdForInputB);
    printf("Parent process started ...\n");

    char matrix_A[(int)(totalReadedBytes/2)][(int)(totalReadedBytes/2)];
    char matrix_B[(int)(totalReadedBytes/2)][(int)(totalReadedBytes/2)];

    int k=0;
    for(int i=0; i<pow(2,n); i++) {
        for(int j=0; j<pow(2,n); j++) {
            matrix_A[i][j] = readedInputA[k];
            matrix_B[i][j] = readedInputB[k];
            k++;
        }
    }

    printf("MATRIX A : \n");
    for(int i=0; i<(int)pow(2,n); i++) {
        for(int j=0; j<(int)pow(2,n); j++) {
            printf("%3d ",(unsigned int)matrix_A[i][j]);
        }
        printf("\n");
    }
    printf("\n\nMATRIX B : \n");
    for(int i=0; i<(int)pow(2,n); i++) {
        for(int j=0; j<(int)pow(2,n); j++) {
            printf("%3d ",(unsigned int)matrix_B[i][j]);
        }
        printf("\n");
    }
    printf("\n\n");
    
    // For parent and process2 ...
    int filePipes_1[2];
    int filePipes_2[2];
    if(pipe(filePipes_1) == -1) {
        perror("ERROR ! pipe ");
        exit(EXIT_FAILURE);
    }
    if(pipe(filePipes_2) == -1) {
        perror("ERROR ! pipe ");
        exit(EXIT_FAILURE);
    }


    // For parent and process3 ...
    int filePipes_3[2];
    int filePipes_4[2];
    if(pipe(filePipes_3) == -1) {
        perror("ERROR ! pipe ");
        exit(EXIT_FAILURE);
    }
    if(pipe(filePipes_4) == -1) {
        perror("ERROR ! pipe ");
        exit(EXIT_FAILURE);
    }
    
    // For parent and process4 ...
    int filePipes_5[2];
    int filePipes_6[2];
    if(pipe(filePipes_5) == -1) {
        perror("ERROR ! pipe ");
        exit(EXIT_FAILURE);
    }
    if(pipe(filePipes_6) == -1) {
        perror("ERROR ! pipe ");
        exit(EXIT_FAILURE);
    }
    
    // For parent and process5 ...
    int filePipes_7[2];
    int filePipes_8[2];
    if(pipe(filePipes_7) == -1) {
        perror("ERROR ! pipe ");
        exit(EXIT_FAILURE);
    }
    if(pipe(filePipes_8) == -1) {
        perror("ERROR ! pipe ");
        exit(EXIT_FAILURE);
    }
    
    // Process ID'ler ...
    
    
    int half = (int)(pow(2,n) / 2);
    int total = pow(2,n);
    
    int matrix_C[(int)(total)][(int)total];
    
    if((pid_1 = fork()) == 0) {
        printf("Child process 1 started ...\n");
        if(close(filePipes_1[1]) == -1){ // Yazmayi kapat ...
            perror("ERROR : Closed filePipes_1 ");
            exit(EXIT_FAILURE);
        }
        
        if(close(filePipes_2[0]) == -1) { // Okumayı kapat ... 
            perror("ERROR : Closed filePipes_2 ");
            exit(EXIT_FAILURE);
        }

        char* readedBytes = (char *) malloc(sizeof(char) * total * total);
        read(filePipes_1[0],readedBytes, total*total);


        int * resultArr;
        resultArr = calculate_C11(readedBytes,total*total);

        write(filePipes_2[1],resultArr,total*total/4*sizeof(int));
        
        free(resultArr);
    }
    else {

        if(close(filePipes_1[0]) == -1) { // Okumayi kapat ...
            perror("ERROR ! Closed filePipes_ 1 ");
            exit(EXIT_FAILURE);
        }

        char * process2 = (char *) malloc(sizeof(char) * ((int)(pow(2,n) * pow(2,n))));
        int k = 0;
        for(int i=0; i < half; i++) {
            for(int j=0; j < half; j++) {
                process2[k] = matrix_A[i][j];
                k++;
            }
        }
        for(int i=0; i < half; i++) {
            for(int j = half; j<total; j++) {
                process2[k] = matrix_A[i][j];
                k++;                
            }
        }
        for(int i=0; i < half; i++) {
            for(int j=0; j < half; j++) {
                process2[k] = matrix_B[i][j];
                k++;
            }
        }      
        for(int i=half; i < total; i++) {
            for(int j=0; j < half; j++) {
                process2[k] = matrix_B[i][j];
                k++;
            }
        } 
        write(filePipes_1[1],process2,strlen(process2));
        
        if((pid_2 = fork()) == 0) {
            printf("Child process 2 started ...\n");
            if(close(filePipes_3[1]) == -1){ // Yazmayi kapat ...
                perror("ERROR : Closed filePipes_3 ");
                exit(EXIT_FAILURE);
            }

            if(close(filePipes_4[0]) == -1) { // Okumayı kapat ... 
                perror("ERROR : Closed filePipes_4 ");
                exit(EXIT_FAILURE);
            }
            
            char* readedBytes = (char *) malloc(sizeof(char) * total * total);
            read(filePipes_3[0],readedBytes, total*total);

            int * resultArr;
            resultArr = calculate_C12(readedBytes,total*total);

            write(filePipes_4[1],resultArr,total*total/4*sizeof(int));
            free(resultArr);
        }
        else{
            if(close(filePipes_3[0]) == -1) { // Okumayi kapat ...
                perror("ERROR ! Closed filePipes_ 3 readed ");
                exit(EXIT_FAILURE);
            }

            char * process3 = (char *) malloc(sizeof(char) * ((int)(pow(2,n) * pow(2,n))));            
            int k = 0;
            for(int i=0; i < half; i++) {
                for(int j=0; j < half; j++) {
                    process3[k] = matrix_A[i][j];
                    k++;
                }
            }

            for(int i=0; i < half; i++) {
                for(int j = half; j<total; j++) {
                    process3[k] = matrix_A[i][j];
                    k++;                
                }
            }

            for(int i=0; i < half; i++) {
                for(int j=half; j < total; j++) {
                    process3[k] = matrix_B[i][j];
                    k++;
                }
            }      
                
            for(int i=half; i < total; i++) {
                for(int j=half; j < total; j++) {
                    process3[k] = matrix_B[i][j];
                    k++;
                }
            } 

            write(filePipes_3[1],process3,strlen(process3));

            if((pid_3 = fork()) == 0) {
                printf("Child process 3 started ...\n");
                if(close(filePipes_5[1]) == -1){ // Yazmayi kapat ...
                    perror("ERROR : Closed filePipes_5 ");
                    exit(EXIT_FAILURE);
                }
                if(close(filePipes_6[0]) == -1) { // Okumayı kapat ... 
                    perror("ERROR : Closed filePipes_2 ");
                    exit(EXIT_FAILURE);
                }
                
                char* readedBytes = (char *) malloc(sizeof(char) * total * total);
                read(filePipes_5[0],readedBytes, total*total);

                int * resultArr;
                resultArr = calculate_C21(readedBytes,total*total);

                write(filePipes_6[1],resultArr,total*total/4*sizeof(int));
                free(resultArr);
            }


            else {
                if(close(filePipes_5[0]) == -1) { // Okumayi kapat ...
                    perror("ERROR ! Closed filePipes_ 5 ");
                    exit(EXIT_FAILURE);
                }
                
                char * process4 = (char *) malloc(sizeof(char) * ((int)(pow(2,n) * pow(2,n))));            
                int k = 0;
                for(int i=half; i < total; i++) {
                    for(int j=0; j < half; j++) {
                        process4[k] = matrix_A[i][j];
                        k++;
                    }
                }

                for(int i=half; i < total; i++) {
                    for(int j = half; j<total; j++) {
                        process4[k] = matrix_A[i][j];
                        k++;                
                    }
                }

                for(int i=0; i < half; i++) {
                    for(int j=0; j < half; j++) {
                        process4[k] = matrix_B[i][j];
                        k++;
                    }
                }      
            
                for(int i=half; i < total; i++) {
                    for(int j=0; j < half; j++) {
                        process4[k] = matrix_B[i][j];
                        k++;
                    }
                } 

                write(filePipes_5[1],process4,strlen(process4));

                if((pid_4 = fork()) == 0) {
                    printf("Child process 4 started ...\n");
                    if(close(filePipes_7[1]) == -1){ // Yazmayi kapat ...
                        perror("ERROR : Closed filePipes_7 ");
                        exit(EXIT_FAILURE);
                    }
                    if(close(filePipes_8[0]) == -1) { // Okumayı kapat ... 
                        perror("ERROR : Closed filePipes_8 ");
                        exit(EXIT_FAILURE);
                    }
                    
                    char* readedBytes = (char *) malloc(sizeof(char) * total * total);
                    read(filePipes_7[0],readedBytes, total*total);

                    int * resultArr;
                    resultArr = calculate_C22(readedBytes,total*total);

                    write(filePipes_8[1],resultArr,total*total/4*sizeof(int));
                    free(resultArr);
                }
                else {
                    if(close(filePipes_7[0]) == -1) { // Okumayi kapat ...
                        perror("ERROR ! Closed filePipes_ 7 ");
                        exit(EXIT_FAILURE);
                    }

                    char * process5 = (char *) malloc(sizeof(char) * ((int)(pow(2,n) * pow(2,n))));            
                    int k = 0;
                    for(int i=half; i < total; i++) {
                        for(int j=0; j < half; j++) {
                            process5[k] = matrix_A[i][j];
                            k++;
                        }
                    }

                    for(int i=half; i < total; i++) {
                        for(int j = half; j<total; j++) {
                            process5[k] = matrix_A[i][j];
                            k++;                
                        }
                    }

                    for(int i=0; i < half; i++) {
                        for(int j=half; j < total; j++) {
                            process5[k] = matrix_B[i][j];
                            k++;
                        }
                    }      
                
                    for(int i=half; i < total; i++) {
                        for(int j=half; j < total; j++) {
                            process5[k] = matrix_B[i][j];
                            k++;
                        }
                    } 

                    write(filePipes_7[1],process5,strlen(process5));

                    free(process5);
                }
                free(process4);
            }
            free(process3);
        }
        
        if(close(filePipes_2[1]) == -1) { // Yazmayi kapat ... 
            perror("ERROR : Closed filePipes_2 ");
            exit(EXIT_FAILURE);
        }

        if(close(filePipes_4[1]) == -1) { // Yazmayi kapat ... 
            perror("ERROR : Closed filePipes_2 ");
            exit(EXIT_FAILURE);
        }

        if(close(filePipes_6[1]) == -1) { // Yazmayi kapat ... 
            perror("ERROR : Closed filePipes_2 ");
            exit(EXIT_FAILURE);
        }

        if(close(filePipes_8[1]) == -1) { // Yazmayi kapat ... 
            perror("ERROR : Closed filePipes_2 ");
            exit(EXIT_FAILURE);
        }
        
        
        pid_1 = wait(NULL);
        pid_2 = wait(NULL);
        pid_3 = wait(NULL);
        pid_4 = wait(NULL);
    
        if(pid_1 != -1) {
            int* quarter1 = (int *) malloc(sizeof(int) * total*total/4*sizeof(int));
            read(filePipes_2[0],quarter1,total*total/4*sizeof(int));

            int k=0;
            for(int i=0; i < half; i++) {
                for(int j=0; j< half; j++){
                    matrix_C[i][j] = quarter1[k];
                    k++;
                }

            }
            if(close(filePipes_2[0]) == -1) {
                perror("ERROR : Closed filePipes_2 ");
                exit(EXIT_FAILURE);
            }
            free(quarter1);
        }

        if(pid_2 != -1) {
            int* quarter2 = (int *) malloc(sizeof(int) * total*total/4*sizeof(int));
            read(filePipes_4[0],quarter2,total*total/4*sizeof(int));

            int k=0;
            for(int i=0; i < half; i++) {
                for(int j=half; j< total; j++){
                    matrix_C[i][j] = quarter2[k];
                    k++;
                }
            }
            if(close(filePipes_4[0]) == -1) {
                perror("ERROR : Closed filePipes_4 ");
                exit(EXIT_FAILURE);
            }
            free(quarter2);
        }

        if(pid_3 != -1) {
            int* quarter3 = (int *) malloc(sizeof(int) * total*total/4*sizeof(int));
            read(filePipes_6[0],quarter3,total*total/4*sizeof(int));
            int k=0;
            for(int i=half; i < total; i++) {
                for(int j=0; j< half; j++){
                    matrix_C[i][j] = quarter3[k];
                    k++;
                }
            }
            if(close(filePipes_6[0]) == -1) {
                perror("ERROR : Closed filePipes_6 ");
                exit(EXIT_FAILURE);
            }
            free(quarter3);
        }

        if(pid_4 != -1) {
            int* quarter4 = (int *) malloc(sizeof(int) * total*total/4*sizeof(int));
            read(filePipes_8[0],quarter4,total*total/4*sizeof(int));

            int k=0;
            for(int i=half; i < total; i++) {
                for(int j=half; j< total; j++){
                    matrix_C[i][j] = quarter4[k];
                    k++;
                }
            }
            if(close(filePipes_8[0]) == -1) {
                perror("ERROR : Closed filePipes_8 ");
                exit(EXIT_FAILURE);
            }
            free(quarter4);
        }
        free(process2);
    }

    if(parentProcessId == getpid() && pid_1 != -1 && pid_2 != -1 && pid_3 != -1 && pid_4 != -1) {        
        printf("\nMATRIX C : \n\n");
        for(int i=0; i< total; i++) {
            for(int j=0; j<total; j++) {
                printf("%d ",matrix_C[i][j]);
            }
            printf("\n");
        }

        int size = (int)(pow(2,n) + 1);

        double **a = (double **)malloc(size * sizeof(double *)); 
        for (int i=0; i < size; i++) 
            a[i] = (double *)malloc(size * sizeof(double));
        
        double w[size];
        double **v= (double **)malloc(size * sizeof(double *)); 
        for (int i=0; i < size; i++) 
            v[i] = (double *)malloc(size * sizeof(double));

        for(int i=1; i < size; i++) {
            for(int j=1;j < size; j++) {
                a[i][j] = (double) matrix_C[i-1][j-1];
            }
        }
        
        svdcmp(a,size-1,size-1,w,v);

        printf("\nSINGULAR VALUES : \n");
        for(int i=1;i < size; i++) {
            printf("%.3lf  ",w[i]);
        }
        printf("\n");

        for(int i=0; i<n; i++) {
            free(a[i]);
            free(v[i]);
        }
        free(a);
        free(v);
        
        free(readedInputA);
        free(readedInputB);
    }
}


size_t getFileSize(const char* fileName) {
    struct stat statX;
    stat(fileName, &statX);
    size_t size = statX.st_size;
    return size;
}

void printUsage() {
    printf("\n");
    printf("####################### PROGRAM USAGE ########################\n");
    printf("#              Programs argument is -i -j and -n             #\n");
    printf("#  -i : read the contents of the file denoted by inputPathA  #\n");
    printf("#  -j : read the contents of the file denoted by inputPathB  #\n");
    printf("#  -n :         2^n x 2^n  positive integer n                #\n");
    printf("##############################################################\n\n");
}

void closeFiles(int fd1, int fd2) {
    if(close(fd1) == -1) {
        perror("ERROR ! InputpathA does not close ");
        exit(EXIT_FAILURE);
    }
    if(close(fd2) == -1) {
        perror("ERROR ! InputpathB does not close ");
        exit(EXIT_FAILURE);
    }
}

int* calculate_C11(char* str, int total) {
    //printf("%d\n",total);

    int dividedFour = total / 4;
    int row = sqrt(dividedFour);
    int column = row;
    //printf("%d\n",row);

    int matrix_A11[row][column];
    int matrix_A12[row][column];
    int matrix_B11[row][column];
    int matrix_B21[row][column];

    int array_A11[dividedFour];
    int array_A12[dividedFour];
    int array_B11[dividedFour];
    int array_B21[dividedFour];

    for(int i= 0; i < dividedFour; i++) 
        array_A11[i] = (unsigned int) str[i];
    int z = 0;
    for(int i = dividedFour; i < 2 * dividedFour; i++) {
        array_A12[z] = (unsigned int) str[i];
        z++; 
    } 
    z = 0;
    for(int i= 2*dividedFour; i < 3 * dividedFour; i++) {
        array_B11[z] = (unsigned int) str[i];
        z++; 
    }
    z = 0;
    for(int i= 3*dividedFour; i < 4 * dividedFour; i++) {
        array_B21[z] = (unsigned int) str[i];
        z++;
    }
        
    
    //printf("\n");
    int k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_A11[i][j] = array_A11[k];
            k++;
            //printf("%d ",matrix_A11[i][j]);
        }
        //printf("\n");
    } 
    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_A12[i][j] = array_A12[k];
            k++;
            //printf("%d ",matrix_A12[i][j]);
        }
        //printf("\n");
    } 

    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_B11[i][j] = array_B11[k];
            k++;
            //printf("%d ",matrix_B11[i][j]);
        }
        //printf("\n");
    } 

    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_B21[i][j] = array_B21[k];
            k++;
            //printf("%d ",matrix_B21[i][j]);
        }
        //printf("\n");
    } 

    int * sumArr1 = (int *) malloc(sizeof(int) * row * column);
    int * sumArr2 = (int *) malloc(sizeof(int) * row * column);
    
    sumArr1 = matrix_multi(row,matrix_A11,matrix_B11);
    sumArr2 = matrix_multi(row,matrix_A12,matrix_B21);

    int * resultArr = (int*) malloc(sizeof(int) * row * column);
    resultArr = matrix_sum(row, sumArr1, sumArr2);

    /*
    for(int i = 0 ; i < row*row; i++)
        printf("%d ", resultArr[i]);
    printf("\n");
    */


    free(sumArr1);
    free(sumArr2);

    return resultArr;
}

int* calculate_C12(char* str, int total) {
    //printf("%d\n",total);

    int dividedFour = total / 4;
    int row = sqrt(dividedFour);
    int column = row;
    //printf("%d\n",row);

    int matrix_A11[row][column];
    int matrix_A12[row][column];
    int matrix_B12[row][column];
    int matrix_B22[row][column];

    int array_A11[dividedFour];
    int array_A12[dividedFour];
    int array_B12[dividedFour];
    int array_B22[dividedFour];

    for(int i= 0; i < dividedFour; i++) 
        array_A11[i] = (unsigned int) str[i];
    int z = 0;
    for(int i = dividedFour; i < 2 * dividedFour; i++) {
        array_A12[z] = (unsigned int) str[i];
        z++; 
    } 
    z = 0;
    for(int i= 2*dividedFour; i < 3 * dividedFour; i++) {
        array_B12[z] = (unsigned int) str[i];
        z++; 
    }
    z = 0;
    for(int i= 3*dividedFour; i < 4 * dividedFour; i++) {
        array_B22[z] = (unsigned int) str[i];
        z++;
    }
        
    
    //printf("\n");
    int k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_A11[i][j] = array_A11[k];
            k++;
            //printf("%d ",matrix_A11[i][j]);
        }
        //printf("\n");
    } 
    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_A12[i][j] = array_A12[k];
            k++;
            //printf("%d ",matrix_A12[i][j]);
        }
        //printf("\n");
    } 

    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_B12[i][j] = array_B12[k];
            k++;
            //printf("%d ",matrix_B12[i][j]);
        }
        //printf("\n");
    } 

    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_B22[i][j] = array_B22[k];
            k++;
            //printf("%d ",matrix_B22[i][j]);
        }
        //printf("\n");
    } 

    int * sumArr1 = (int *) malloc(sizeof(int) * row * column);
    int * sumArr2 = (int *) malloc(sizeof(int) * row * column);
    
    sumArr1 = matrix_multi(row,matrix_A11,matrix_B12);
    sumArr2 = matrix_multi(row,matrix_A12,matrix_B22);

    int * resultArr = (int*) malloc(sizeof(int) * row * column);
    resultArr = matrix_sum(row, sumArr1, sumArr2);

    /*
    for(int i = 0 ; i < row*row; i++)
        printf("%d ", resultArr[i]);
    printf("\n");
    */

    free(sumArr1);
    free(sumArr2);

    return resultArr;
}

int* calculate_C21(char* str, int total) {
    //printf("%d\n",total);

    int dividedFour = total / 4;
    int row = sqrt(dividedFour);
    int column = row;
    //printf("%d\n",row);

    int matrix_A21[row][column];
    int matrix_A22[row][column];
    int matrix_B11[row][column];
    int matrix_B21[row][column];

    int array_A21[dividedFour];
    int array_A22[dividedFour];
    int array_B11[dividedFour];
    int array_B21[dividedFour];

    for(int i= 0; i < dividedFour; i++) 
        array_A21[i] = (unsigned int) str[i];
    int z = 0;
    for(int i = dividedFour; i < 2 * dividedFour; i++) {
        array_A22[z] = (unsigned int) str[i];
        z++; 
    } 
    z = 0;
    for(int i= 2*dividedFour; i < 3 * dividedFour; i++) {
        array_B11[z] = (unsigned int) str[i];
        z++; 
    }
    z = 0;
    for(int i= 3*dividedFour; i < 4 * dividedFour; i++) {
        array_B21[z] = (unsigned int) str[i];
        z++;
    }
        
    
    //printf("\n");
    int k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_A21[i][j] = array_A21[k];
            k++;
            //printf("%d ",matrix_A21[i][j]);
        }
        //printf("\n");
    } 
    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_A22[i][j] = array_A22[k];
            k++;
            //printf("%d ",matrix_A22[i][j]);
        }
        //printf("\n");
    } 

    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_B11[i][j] = array_B11[k];
            k++;
            //printf("%d ",matrix_B11[i][j]);
        }
        //printf("\n");
    } 

    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_B21[i][j] = array_B21[k];
            k++;
            //printf("%d ",matrix_B21[i][j]);
        }
        //printf("\n");
    } 

    int * sumArr1 = (int *) malloc(sizeof(int) * row * column);
    int * sumArr2 = (int *) malloc(sizeof(int) * row * column);
    
    sumArr1 = matrix_multi(row,matrix_A21,matrix_B11);
    sumArr2 = matrix_multi(row,matrix_A22,matrix_B21);

    int * resultArr = (int*) malloc(sizeof(int) * row * column);
    resultArr = matrix_sum(row, sumArr1, sumArr2);

    /*
    for(int i = 0 ; i < row*row; i++)
        printf("%d ", resultArr[i]);
    printf("\n");
    */

    free(sumArr1);
    free(sumArr2);

    return resultArr;
}

int* calculate_C22(char* str, int total) {
    //printf("%d\n",total);

    int dividedFour = total / 4;
    int row = sqrt(dividedFour);
    int column = row;
    //printf("%d\n",row);

    int matrix_A21[row][column];
    int matrix_A22[row][column];
    int matrix_B12[row][column];
    int matrix_B22[row][column];

    int array_A21[dividedFour];
    int array_A22[dividedFour];
    int array_B12[dividedFour];
    int array_B22[dividedFour];

    for(int i= 0; i < dividedFour; i++) 
        array_A21[i] = (unsigned int) str[i];
    int z = 0;
    for(int i = dividedFour; i < 2 * dividedFour; i++) {
        array_A22[z] = (unsigned int) str[i];
        z++; 
    } 
    z = 0;
    for(int i= 2*dividedFour; i < 3 * dividedFour; i++) {
        array_B12[z] = (unsigned int) str[i];
        z++; 
    }
    z = 0;
    for(int i= 3*dividedFour; i < 4 * dividedFour; i++) {
        array_B22[z] = (unsigned int) str[i];
        z++;
    }
        
    
    //printf("\n");
    int k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_A21[i][j] = array_A21[k];
            k++;
            //printf("%d ",matrix_A21[i][j]);
        }
        //printf("\n");
    } 
    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_A22[i][j] = array_A22[k];
            k++;
            //printf("%d ",matrix_A22[i][j]);
        }
        //printf("\n");
    } 

    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_B12[i][j] = array_B12[k];
            k++;
            //printf("%d ",matrix_B12[i][j]);
        }
        //printf("\n");
    } 

    //printf("\n\n\n");
    k = 0;
    for(int i=0; i<row; i++) {
        for(int j=0;j<column; j++) {
            matrix_B22[i][j] = array_B22[k];
            k++;
            //printf("%d ",matrix_B22[i][j]);
        }
        //printf("\n");
    } 

    int * sumArr1 = (int *) malloc(sizeof(int) * row * column);
    int * sumArr2 = (int *) malloc(sizeof(int) * row * column);
    
    sumArr1 = matrix_multi(row,matrix_A21,matrix_B12);
    sumArr2 = matrix_multi(row,matrix_A22,matrix_B22);

    int * resultArr = (int*) malloc(sizeof(int) * row * column);
    resultArr = matrix_sum(row, sumArr1, sumArr2);

    /*
    for(int i = 0 ; i < row*row; i++)
        printf("%d ", resultArr[i]);
    printf("\n");
    */

    free(sumArr1);
    free(sumArr2);

    return resultArr;
}


int* matrix_multi(int n,int matrix1[][n], int matrix2[][n]) {
    int result[n][n]; 
    for (int i = 0; i < n; i++) { 
        for (int j = 0; j < n; j++) { 
            result[i][j] = 0; 
            for (int k = 0; k < n; k++) { 
                *(*(result + i) + j) += *(*(matrix1 + i) + k) * *(*(matrix2 + k) + j); 
            } 
        } 
    }

    int * resultArr = (int *) malloc(sizeof(int) * n * n);
    int k = 0;
    for(int i = 0; i < n; i++) {
        for(int j=0; j < n; j++) {
            resultArr[k] = result[i][j];
            k++;
        }
    }   
    
    return resultArr;
}

int* matrix_sum(int n, int* arr1, int* arr2) {
    int* resultArr = (int *) malloc(sizeof(int) * n *n);

    for(int i=0; i < n*n; i++)
        resultArr[i] = arr1[i] + arr2[i];
    
    return resultArr;
} 


// ASSAGIDAKI FONKSIYONLAR SINGULAR VALUE HESAPLAMAK ICIN KULLANILMISTIR ...
/*******************************************************************************
Singular value decomposition program, svdcmp, from "Numerical Recipes in C"
(Cambridge Univ. Press) by W.H. Press, S.A. Teukolsky, W.T. Vetterling,
and B.P. Flannery
*******************************************************************************/
double **dmatrix(int nrl, int nrh, int ncl, int nch) {
	int i,nrow=nrh-nrl+1,ncol=nch-ncl+1;
	double **m;
	m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
	m += NR_END;
	m -= nrl;
	m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
	m[nrl] += NR_END;
	m[nrl] -= ncl;
	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
	return m;
}

double *dvector(int nl, int nh) {
	double *v;
	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	return v-nl+NR_END;
}

void free_dvector(double *v, int nl, int nh) {
	free((FREE_ARG) (v+nl-NR_END));
}

double pythag(double a, double b) {
	double absa,absb;
	absa=fabs(a);
	absb=fabs(b);
	if (absa > absb) return absa*sqrt(1.0+(absb/absa)*(absb/absa));
	else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+(absa/absb)*(absa/absb)));
}

void svdcmp(double **a, int m, int n, double w[], double **v) {
	int flag,i,its,j,jj,k,l,nm;
	double anorm,c,f,g,h,s,scale,x,y,z,*rv1;

	rv1=dvector(1,n);
	g=scale=anorm=0.0;
	for (i=1;i<=n;i++) {
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i <= m) {
			for (k=i;k<=m;k++) scale += fabs(a[k][i]);
			if (scale) {
				for (k=i;k<=m;k++) {
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f=a[i][i];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
				}
				for (k=i;k<=m;k++) a[k][i] *= scale;
			}
		}
		w[i]=scale *g;
		g=s=scale=0.0;
		if (i <= m && i != n) {
			for (k=l;k<=n;k++) scale += fabs(a[i][k]);
			if (scale) {
				for (k=l;k<=n;k++) {
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][l]=f-g;
				for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
				for (j=l;j<=m;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
					for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
				}
				for (k=l;k<=n;k++) a[i][k] *= scale;
			}
		}
		anorm = DMAX(anorm,(fabs(w[i])+fabs(rv1[i])));
	}
	for (i=n;i>=1;i--) {
		if (i < n) {
			if (g) {
				for (j=l;j<=n;j++) /* Double division to avoid possible underflow. */
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
					for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=IMIN(m,n);i>=1;i--) {
        l=i+1;
		g=w[i];
		for (j=l;j<=n;j++) a[i][j]=0.0;
		if (g) {
			g=1.0/g;
			for (j=l;j<=n;j++) {
				for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
			}
			for (j=i;j<=m;j++) a[j][i] *= g;
		} else for (j=i;j<=m;j++) a[j][i]=0.0;
		++a[i][i];
	}
	for (k=n;k>=1;k--) {
		for (its=1;its<=30;its++) {
			flag=1;
			for (l=k;l>=1;l--) {
				nm=l-1;
				if ((double)(fabs(rv1[l])+anorm) == anorm) {
					flag=0;
					break;
				}
				if ((double)(fabs(w[nm])+anorm) == anorm) break;
			}
			if (flag) {
				c=0.0;
				s=1.0;
				for (i=l;i<=k;i++) {
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if ((double)(fabs(f)+anorm) == anorm) break;
					g=w[i];
					h=pythag(f,g);
					w[i]=h;
					h=1.0/h;
					c=g*h;
					s = -f*h;
					for (j=1;j<=m;j++) {
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}
			z=w[k];
			if (l == k) {
				if (z < 0.0) { 
					w[k] = -z;
					for (j=1;j<=n;j++) v[j][k] = -v[j][k];
				}
				break;
			}
			if (its == 30) printf("no convergence in 30 svdcmp iterations");
			x=w[l];
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=pythag(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0; /* Next QR transformation: */
			for (j=l;j<=nm;j++) {
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=1;jj<=n;jj++) {
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				w[j]=z; /* Rotation can be arbitrary if z = 0. */
				if (z) {
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=1;jj<=m;jj++) {
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}
	free_dvector(rv1,1,n);
}