// Online C compiler to run C program online
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

sem_t *O_sem;
sem_t *O_sem2;
sem_t *H_sem;
sem_t *H_sem2;
sem_t *SharedMemory_sem;
sem_t *SharedMoleculeCount_sem;
sem_t *SharedHydrogenCount_sem;
sem_t *SharedOxygenCount_sem;
sem_t *WriteOut_sem;

void incSharedMemory(int *shared_memory)
{
    sem_wait(SharedMemory_sem);
    (*shared_memory)++;
    sem_post(SharedMemory_sem);
}




void decSharedHydrogen(int *SharedHydrogenCount)
{
   sem_wait(SharedHydrogenCount_sem);

    (*SharedHydrogenCount)--;
    sem_post(SharedHydrogenCount_sem);
}

void decSharedOxygen(int *SharedOxygenCount)
{
     sem_wait(SharedOxygenCount_sem);

    (*SharedOxygenCount)--;
    sem_post(SharedOxygenCount_sem);
}

void incMoleculeCount(int *SharedMoleculeCount)
{
    sem_wait(SharedMoleculeCount_sem);
    (*SharedMoleculeCount)++;
    sem_post(SharedMoleculeCount_sem);
}

void kyslik(int id, int TI, int *shared_memory, int *SharedMoleculeCount, int *SharedOxygenCount, int *SharedHydrogenCount,FILE *fp)
{
     sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp,"%d: O %d: started\n", *shared_memory, id);
    sem_post(WriteOut_sem);
    usleep(rand() % TI);
       sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp,"%d: O %d: going to queue\n", *shared_memory, id);
      sem_post(WriteOut_sem);
    sem_wait(O_sem);
    incMoleculeCount(SharedMoleculeCount);
    sem_post(H_sem);
    sem_post(H_sem);
    if(*SharedHydrogenCount<2){
          sem_wait(WriteOut_sem);
          incSharedMemory(shared_memory);
            fprintf(fp,"%d: O %d: not enough H\n", *shared_memory, id);
               sem_post(WriteOut_sem);
        decSharedOxygen(SharedOxygenCount);

            exit(0);
    }
    sem_wait(O_sem2);
    sem_wait(O_sem2);

     if(*SharedHydrogenCount<2){
           sem_wait(O_sem);
           incSharedMemory(shared_memory);
            fprintf(fp,"%d: O %d: not enough H\n", *shared_memory, id);
                       sem_post(WriteOut_sem);
        decSharedOxygen(SharedOxygenCount);

            exit(0);
    }
    incSharedMemory(shared_memory);
  sem_wait(WriteOut_sem);
   fprintf(fp,"%d: O %d: creating molecule %d\n", *shared_memory, id, *SharedMoleculeCount);
    sem_post(WriteOut_sem);
    usleep(rand() % TI);
    sem_post(H_sem2);
    sem_post(H_sem2);
      sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp,"%d: O %d: molecule %d created\n", *shared_memory, id, *SharedMoleculeCount);
        sem_post(WriteOut_sem);
    sem_post(O_sem);
    decSharedOxygen(SharedOxygenCount);
    if(*SharedOxygenCount==0){
          for(int i = 0; i<=*SharedHydrogenCount;i++) {
                sem_post(H_sem);
                  sem_post(H_sem2);
          }

    }
    exit(0);
}

void vodik(int id, int TI, int *shared_memory, int *SharedMoleculeCount, int *SharedOxygenCount, int *SharedHydrogenCount,FILE *fp)
{
         sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
   fprintf(fp,"%d: H %d: started\n", *shared_memory, id);
       sem_post(WriteOut_sem);
    usleep(rand() % TI);
        sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp,"%d: H %d: going to queue\n", *shared_memory, id);
           sem_post(WriteOut_sem);
    if(*SharedHydrogenCount<2||*SharedOxygenCount<1){
             sem_wait(WriteOut_sem);
            incSharedMemory(shared_memory);
             fprintf(fp,"%d: H %d: not enough O or H\n", *shared_memory, id);
              sem_post(WriteOut_sem);
        decSharedHydrogen(SharedHydrogenCount);

            exit(0);
    }
    sem_wait(H_sem);
    if(*SharedHydrogenCount<2||*SharedOxygenCount<1){
   sem_wait(WriteOut_sem);
            incSharedMemory(shared_memory);
            fprintf(fp,"%d: H %d: not enough O or H\n", *shared_memory, id);
                 sem_post(WriteOut_sem);
        decSharedHydrogen(SharedHydrogenCount);

            exit(0);
    }


   sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp,"%d: H %d: creating molecule %d\n", *shared_memory, id, *SharedMoleculeCount);
          sem_post(WriteOut_sem);
    sem_post(O_sem2);
    sem_wait(H_sem2);
      if(*SharedHydrogenCount<2||*SharedOxygenCount<1){
 sem_wait(WriteOut_sem);
            incSharedMemory(shared_memory);
            fprintf(fp,"%d: H %d: not enough O or H\n", *shared_memory, id);
                  sem_post(WriteOut_sem);
        decSharedHydrogen(SharedHydrogenCount);

            exit(0);
    }
 sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
   fprintf(fp,"%d: H %d: molecule %d created\n", *shared_memory, id, *SharedMoleculeCount);
             sem_post(WriteOut_sem);
    decSharedHydrogen(SharedHydrogenCount);
    if(*SharedHydrogenCount<2){
         for(int i = 0; i<=*SharedHydrogenCount;i++) {
                sem_post(H_sem);
                  sem_post(H_sem2);
          }
          for(int i = 0; i<=*SharedOxygenCount;i++) {
                sem_post(O_sem);
                  sem_post(O_sem2);
          }

    }

    exit(0);
}

int main(int argc, char **argv)
{
    FILE *fp;

     fp = fopen("proj2.out", "w");
     setbuf(fp,NULL);
    if(fp == NULL) {
        printf("file can't be opened\n");
        exit(1);
    }


    sem_unlink("O_sem_filee");
    sem_unlink("O_sem2_filee");
    sem_unlink("H_sem_filee");
    sem_unlink("H_sem2_filee");
    sem_unlink("SharedMemory_sem_filee");
    sem_unlink("SharedMoleculeCount_sem");
    sem_unlink("SharedOxygenCount_sem");
    sem_unlink("SharedHydrogenCount_sem");
    sem_unlink("WriteOut_sem");


    if ((WriteOut_sem = sem_open("WriteOut_sem", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }
    if ((SharedMoleculeCount_sem = sem_open("SharedMoleculeCount_sem", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }
    if ((SharedOxygenCount_sem = sem_open("SharedOxygenCount_sem", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }
    if ((SharedHydrogenCount_sem = sem_open("SharedHydrogenCount_sem", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }
    if ((O_sem = sem_open("O_sem_filee", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }
    if ((O_sem2 = sem_open("O_sem2_filee", O_CREAT, 0777, 0)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }

    if ((H_sem = sem_open("H_sem_filee", O_CREAT, 0660, 0)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }
    if ((H_sem2 = sem_open("H_sem2_filee", O_CREAT, 0660, 0)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }
    if ((SharedMemory_sem = sem_open("SharedMemory_sem_filee", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        exit(EXIT_FAILURE);
    }

    int *shared_memory = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *SharedMoleculeCount = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
     int *SharedHydrogenCount = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      int *SharedOxygenCount = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *shared_memory = 0;
    *SharedMoleculeCount = 0;
    *SharedOxygenCount= atoi(argv[1]);
    *SharedHydrogenCount= atoi(argv[2]);
    time_t t;
    srand((unsigned)time(&t));

    for (int i = 0; i < atoi(argv[1]); i++) // loop will run n times (n=5)
    {
        if (fork() == 0)
        {
            kyslik(i + 1, atoi(argv[3]), shared_memory, SharedMoleculeCount,SharedOxygenCount,SharedHydrogenCount, fp);
        }
    }

    for (int i = 0; i < atoi(argv[2]); i++) // loop will run n times (n=5)
    {
        if (fork() == 0)
        {
            vodik(i + 1, atoi(argv[3]), shared_memory, SharedMoleculeCount,SharedOxygenCount,SharedHydrogenCount, fp);
        }
    }

 for (int i = 0; i < atoi(argv[1])+ atoi(argv[2]); i++) // loop will run n times (n=5)
        wait(NULL);

    sem_close(O_sem);
    sem_close(O_sem2);
    sem_close(H_sem);
    sem_close(H_sem2);
    sem_close(SharedMemory_sem);
    sem_close(SharedMoleculeCount_sem);
      sem_close(SharedHydrogenCount_sem);
        sem_close(SharedOxygenCount_sem);
           sem_close(WriteOut_sem);
    sem_unlink("SharedMemory_sem_filee");
    sem_unlink("WriteOut_sem");
    sem_unlink("SharedMoleculeCount_sem");
    sem_unlink("O_sem_file");
    sem_unlink("H_sem_file");
    sem_unlink("H_sem2_file");
    sem_unlink("O_sem2_filee");
        sem_unlink("SharedOxygenCount_sem");
    sem_unlink("SharedHydrogenCount_sem");
    fclose(fp);
     return 0;
}

