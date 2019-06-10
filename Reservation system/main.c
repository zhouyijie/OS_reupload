#define _SVID_SOURCE
#define _BSD_SOURCE
#define _XOPEN_SOURCE 500
#define _XOPEN_SORUCE 600
#define _XOPEN_SORUCE 600

#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define BUFF_SIZE 20
#define BUFF_SHM "/yijiezhou_BUFF"
#define BUFF_MUTEX_A "/yijiezhou_MUTEX_A"
#define BUFF_MUTEX_B "/yijiezhou_MUTEX_B"

//declaring semaphores names for local usage
sem_t *mutexA;
sem_t *mutexB;

//declaring the shared memory and base address
int shm_fd;
void *base;

//structure for indivdual table
struct table
{
    int num;
    char name[10];
};

void initTables(struct table *base)
{
    /*
    int value;
    sem_getvalue(mutexA,&value);
    printf("%d\n",value);
    sem_getvalue(mutexB,&value);
    printf("%d\n",value);
    */
    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);
    //initialise the tables with table numbers
    for(int i=0;i<BUFF_SIZE;i++){
        memcpy(base[i].name,"empty     ",10);
        if(i<10){
            base[i].num = 100+i; 
        }
        else{
            base[i].num = 190+i;
        }
    }
    //perform a random sleep  
    sleep(rand() % 10);

    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);
    return;
}

void printTableInfo(struct table *base)
{
    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);
    //print the tables with table numbers and name
    for(int i=0;i<BUFF_SIZE;i++){
        printf("table %d: %s\n",base[i].num,base[i].name);
    }
    //perform a random sleep  
    sleep(rand() % 10);
    
    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);
    return; 
}

void reserveSpecificTable(struct table *base, char *nameHld, char *section, int tableNo)
{
    switch (section[0])
    {
    case 'A':
        
        sem_wait(mutexA);
        
        if(tableNo>99 && tableNo <110){
            for(int i=0;i<BUFF_SIZE;i++){
                if(base[i].num == tableNo){
                    if(!strcmp(base[i].name,"empty     ")){
                        memcpy(base[i].name,nameHld,10);
                    }else{
                        printf("Cannot reserve table\n");
                    }
                }
            }
        }else{
            printf("Invalid table number\n");
        }
        
        sem_post(mutexA);
        break;
    case 'B':
        
        sem_wait(mutexB);
        
        if(tableNo>199 && tableNo <210){
            for(int i=0;i<BUFF_SIZE;i++){
                if(base[i].num == tableNo){
                    if(!strcmp(base[i].name,"empty     ")){
                        memcpy(base[i].name,nameHld,10);
                    }else{
                        printf("Cannot reserve table\n");
                    }
                }
            }
        }else{
            printf("Invalid table number\n");
        }
        
        sem_post(mutexB);
        break;
    }
    return;
}

void reserveSomeTable(struct table *base, char *nameHld, char *section)
{
    //int idx = -1;
    //int i;
    int fail = 1;
    switch (section[0])
    {
    case 'A':
        
        sem_wait(mutexA);
        
        for(int i=0;i<BUFF_SIZE;i++){
            if(base[i].num <200){
                if(!strcmp(base[i].name,"empty     ")){
                    memcpy(base[i].name,nameHld,10);
                    fail = 0;
                    break;
                }
            }
        }
        if(fail){
            printf("Cannot find empty table\n");
        }
        
        sem_post(mutexA);
        break;
    case 'B':
        sem_wait(mutexB);
    
        for(int i=0;i<BUFF_SIZE;i++){
            if(base[i].num >199){
                if(!strcmp(base[i].name,"empty     ")){
                    memcpy(base[i].name,nameHld,10);
                    fail = 0;
                    break;
                }
            }
        }
        if(fail){
            printf("Cannot find empty table\n");
        }
    
        sem_post(mutexB);
        break;
    }
}

int processCmd(char *cmd, struct table *base)
{
    char *token;
    char *nameHld;
    char *section;
    char *tableChar;
    int tableNo;
    token = strtok(cmd, " ");
    switch (token[0])
    {
    case 'r':
        nameHld = strtok(NULL, " ");
        section = strtok(NULL, " ");
        tableChar = strtok(NULL, " ");
        if (tableChar != NULL)
        {
            tableNo = atoi(tableChar);
            reserveSpecificTable(base, nameHld, section, tableNo);
        }
        else
        {
            reserveSomeTable(base, nameHld, section);
        }
        sleep(rand() % 10);
        break;
    case 's':
        printTableInfo(base);
        break;
    case 'i':
        initTables(base);
        break;
    case 'e':
        return 0;
    }
    return 1;
}

int main(int argc, char * argv[])
{

    
    int fdstdin;
    // file name specifed then rewire fd 0 to file 
    if(argc>1)
    {
        //store actual stdin before rewiring using dup in fdstdin
        char* filename = argv[1];
        //perform stdin rewiring as done in assign 1
        fdstdin = open(filename,O_RDONLY,0);
        close(0);
        dup(fdstdin);
       
    }
    //open mutex BUFF_MUTEX_A and BUFF_MUTEX_B with inital value 1 using sem_open
    mutexA = sem_open(BUFF_MUTEX_A,O_CREAT,S_IRUSR | S_IWUSR, 1);
    mutexB = sem_open(BUFF_MUTEX_B,O_CREAT,S_IRUSR | S_IWUSR, 1);

    //opening the shared memory buffer ie BUFF_SHM using shm open
    shm_fd = shm_open(BUFF_SHM,O_CREAT|O_RDWR,S_IRWXU);
    if (shm_fd == -1)
    {
        printf("prod: Shared memory failed: %s\n", strerror(errno));
        exit(1);
    }

    //configuring the size of the shared memory to sizeof(struct table) * BUFF_SIZE usinf ftruncate
    ftruncate(shm_fd,sizeof(struct table) * BUFF_SIZE);

    //map this shared memory to kernel space
    base = mmap(NULL,sizeof(struct table) * BUFF_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    if (base == MAP_FAILED)
    {
        printf("prod: Map failed: %s\n", strerror(errno));
        // close and shm_unlink?
        shm_unlink(BUFF_SHM);
        exit(1);
    }

    //intialising random number generator
    time_t now;
    srand((unsigned int)(time(&now)));

    //array in which the user command is held
    char cmd[100];
    int cmdType;
    int ret = 1;
    initTables(base);
    while (ret)
    {
        printf("\n>>");
        fgets(cmd,100,stdin);
        if(argc>1)
        {
            printf("Executing Command : %s\n",cmd);
        }
        ret = processCmd(cmd, base);
    }
    
    //close the semphores
    if(sem_close(mutexA) == -1){
        printf("sem_close mutexA failed\n");
    }
    if(sem_close(mutexB)==-1){
        printf("sem_close mutexB failed\n");
    }
    //sem_unlink(BUFF_MUTEX_A);
    //sem_unlink(BUFF_MUTEX_B);

    //reset the standard input
    if(argc>1)
    {
        //using dup2
        dup2(0,STDIN_FILENO);
    }

    //unmap the shared memory
    munmap(base,sizeof(struct table) * BUFF_SIZE);
    close(shm_fd);
    return 0;
}