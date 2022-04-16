/**
 * @file proj2
 * @author RENE CESKA xceska06
 */

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
#include <stdbool.h>

sem_t *O_sem;
sem_t *O_sem2;
sem_t *H_sem;
sem_t *H_sem2;
sem_t *SharedMemory_sem;
sem_t *SharedMoleculeCount_sem;
sem_t *SharedHydrogenCount_sem;
sem_t *SharedOxygenCount_sem;
sem_t *WriteOut_sem;

/**
 * @brief Converts string to int
 *
 * @param str
 * @param number
 * @return true on succes
 * @return false on failer
 */
bool convertToInt(char str[], int *number)
{
    char *endptr;
    *number = strtoul(str, &endptr, 10);
    if (endptr[0] != 0 || str[0] == 0)
    {
        return false;
    }
    return true;
}

/**
 * @brief Safely increases shared memory
 *
 * @param SharedWriteoutCount
 */
void incSharedMemory(int *SharedWriteoutCount)
{
    sem_wait(SharedMemory_sem);
    (*SharedWriteoutCount)++;
    sem_post(SharedMemory_sem);
}
/**
 * @brief Safely decreases Hydrogen count
 *
 * @param SharedWriteoutCount
 */
void decSharedHydrogen(int *SharedHydrogenCount)
{
    sem_wait(SharedHydrogenCount_sem);

    (*SharedHydrogenCount)--;
    sem_post(SharedHydrogenCount_sem);
}
/**
 * @brief Safely decreases Oxygen count
 *
 * @param SharedWriteoutCount
 */
void decSharedOxygen(int *SharedOxygenCount)
{
    sem_wait(SharedOxygenCount_sem);

    (*SharedOxygenCount)--;
    sem_post(SharedOxygenCount_sem);
}
/**
 * @brief Safely increases Molecule count
 *
 * @param SharedWriteoutCount
 */
void incMoleculeCount(int *SharedMoleculeCount)
{
    sem_wait(SharedMoleculeCount_sem);
    (*SharedMoleculeCount)++;
    sem_post(SharedMoleculeCount_sem);
}
/**
 * @brief Oxygen process
 *
 * @param id oxygen id
 * @param TI maximum time for which it will wait until it goes to queue
 * @param TB maximum time for which it will be creating molecule
 * @param SharedWriteoutCount shared varriable which stores number of writes to output
 * @param SharedMoleculeCount shared varriable which stores number of molecules
 * @param SharedOxygenCount shared varriable which stores number of oxygens
 * @param SharedHydrogenCount shared varriable which stores number of hydrogens
 * @param fp pointer to output file
 */
void Oxygen(int id, int TI, int TB, int *SharedWriteoutCount, int *SharedMoleculeCount, int *SharedOxygenCount, int *SharedHydrogenCount, FILE *fp)
{

    // safely increses SharedWriteoutCount and writes to output file
    sem_wait(WriteOut_sem);
    incSharedMemory(SharedWriteoutCount);
    fprintf(fp, "%d: O %d: started\n", *SharedWriteoutCount, id);
    sem_post(WriteOut_sem);

    // waits for random time in intervall <0,TI>
    usleep(rand() % TI);

    // safely increses SharedWriteoutCount and writes to output file
    sem_wait(WriteOut_sem);
    incSharedMemory(SharedWriteoutCount);
    fprintf(fp, "%d: O %d: going to queue\n", *SharedWriteoutCount, id);
    sem_post(WriteOut_sem);

    // if there is not enough hydrogens, it writes message and ends
    if (*SharedHydrogenCount < 2)
    {
        sem_wait(WriteOut_sem);
        incSharedMemory(SharedWriteoutCount);
        fprintf(fp, "%d: O %d: not enough H\n", *SharedWriteoutCount, id);
        sem_post(WriteOut_sem);
        decSharedOxygen(SharedOxygenCount);

        exit(0);
    }

    // enters queue (waits at semaphore)
    sem_wait(O_sem);

    incMoleculeCount(SharedMoleculeCount);

    // takes two hydrogens from queue
    sem_post(H_sem);
    sem_post(H_sem);

    // if there is not enough hydrogens, it writes message and ends
    if (*SharedHydrogenCount < 2)
    {
        sem_wait(WriteOut_sem);
        incSharedMemory(SharedWriteoutCount);
        fprintf(fp, "%d: O %d: not enough H\n", *SharedWriteoutCount, id);
        sem_post(WriteOut_sem);
        decSharedOxygen(SharedOxygenCount);

        exit(0);
    }
    // waits until 2 hydrogens start creating molecule
    sem_wait(O_sem2);
    sem_wait(O_sem2);

    // if there is not enough hydrogens, it writes message and ends
    if (*SharedHydrogenCount < 2)
    {
        sem_wait(O_sem);
        incSharedMemory(SharedWriteoutCount);
        fprintf(fp, "%d: O %d: not enough H\n", *SharedWriteoutCount, id);
        sem_post(WriteOut_sem);
        decSharedOxygen(SharedOxygenCount);

        exit(0);
    }

    incSharedMemory(SharedWriteoutCount);

    // starts creating molecule
    sem_wait(WriteOut_sem);
    fprintf(fp, "%d: O %d: creating molecule %d\n", *SharedWriteoutCount, id, *SharedMoleculeCount);
    sem_post(WriteOut_sem);
    usleep(rand() % TB);
    // when finished lets hydrogens know that molecule is finished
    sem_post(H_sem2);
    sem_post(H_sem2);

    // writes that molecule is finished
    sem_wait(WriteOut_sem);
    incSharedMemory(SharedWriteoutCount);
    fprintf(fp, "%d: O %d: molecule %d created\n", *SharedWriteoutCount, id, *SharedMoleculeCount);
    sem_post(WriteOut_sem);

    // lets next oxygen out from queu
    sem_post(O_sem);

    decSharedOxygen(SharedOxygenCount);

    // if there are no more oxygens it lets all hydrogens free from queue
    if (*SharedOxygenCount == 0)
    {
        for (int i = 0; i <= *SharedHydrogenCount; i++)
        {
            sem_post(H_sem);
            sem_post(H_sem2);
        }
    }
    exit(0);
}

/**
 * @brief Oxygen process
 *
 * @param id oxygen id
 * @param TI maximum time for which it will wait until it goes to queue
 * @param SharedWriteoutCount shared varriable which stores number of writes to output
 * @param SharedMoleculeCount shared varriable which stores number of molecules
 * @param SharedOxygenCount shared varriable which stores number of oxygens
 * @param SharedHydrogenCount shared varriable which stores number of hydrogens
 * @param fp pointer to output file
 */
void Hydrogen(int id, int TI, int *shared_memory, int *SharedMoleculeCount, int *SharedOxygenCount, int *SharedHydrogenCount, FILE *fp)
{
    // safely increses SharedWriteoutCount and writes to output file
    sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp, "%d: H %d: started\n", *shared_memory, id);
    sem_post(WriteOut_sem);
    // waits for random time in intervall <0,TI>
    usleep(rand() % TI);

    // safely increses SharedWriteoutCount and writes to output file
    sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp, "%d: H %d: going to queue\n", *shared_memory, id);
    sem_post(WriteOut_sem);

    // if there is not enough hydrogens or oxygens, it writes message and ends
    if (*SharedHydrogenCount < 2 || *SharedOxygenCount < 1)
    {
        sem_wait(WriteOut_sem);
        incSharedMemory(shared_memory);
        fprintf(fp, "%d: H %d: not enough O or H\n", *shared_memory, id);
        sem_post(WriteOut_sem);
        decSharedHydrogen(SharedHydrogenCount);

        exit(0);
    }

    // waits until oxygen lets it to create molecule
    sem_wait(H_sem);

    // if there is not enough hydrogens or oxygens, it writes message and ends
    if (*SharedHydrogenCount < 2 || *SharedOxygenCount < 1)
    {
        sem_wait(WriteOut_sem);
        incSharedMemory(shared_memory);
        fprintf(fp, "%d: H %d: not enough O or H\n", *shared_memory, id);
        sem_post(WriteOut_sem);
        decSharedHydrogen(SharedHydrogenCount);

        exit(0);
    }
    // safely writes that it is creting molecule
    sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp, "%d: H %d: creating molecule %d\n", *shared_memory, id, *SharedMoleculeCount);
    sem_post(WriteOut_sem);
    // lets oxygen know that it is ready to create molecule
    sem_post(O_sem2);
    // waits until hydrogen says that molecule is created
    sem_wait(H_sem2);
    // writes that molecule is finished
    sem_wait(WriteOut_sem);
    incSharedMemory(shared_memory);
    fprintf(fp, "%d: H %d: molecule %d created\n", *shared_memory, id, *SharedMoleculeCount);
    sem_post(WriteOut_sem);

    decSharedHydrogen(SharedHydrogenCount);
    // if there is not enought hydrogens, it lets all hydrogens and oxygens free from queue
    if (*SharedHydrogenCount < 2)
    {
        for (int i = 0; i <= *SharedHydrogenCount; i++)
        {
            sem_post(H_sem);
            sem_post(H_sem2);
        }
        for (int i = 0; i <= *SharedOxygenCount; i++)
        {
            sem_post(O_sem);
            sem_post(O_sem2);
        }
    }

    exit(0);
}

/**
 * @brief main process
 *
 * @return int
 */
int main(int argc, char **argv)
{
    FILE *fp;
    int TB = -1;
    int TI = -1;
    int NO = -1;
    int NH = -1;

    // checking if arguments are correct
    if (argc != 5)
    {
        fprintf(stderr, "Wrong argument count\n");
        return 1;
    }
    if (!convertToInt(argv[1], &NO))
    {
        fprintf(stderr, "Invalid argument NO\n");
        return 1;
    }
    if (!convertToInt(argv[2], &NH))
    {
        fprintf(stderr, "Invalid argument NH\n");
        return 1;
    }
    if (!convertToInt(argv[3], &TI))
    {
        fprintf(stderr, "Invalid argument TI\n");
        return 1;
    }
    if (!convertToInt(argv[4], &TB))
    {
        fprintf(stderr, "Invalid argument TB\n");
        return 1;
    }
    if (TB < 0 || TI < 0 || NH < 0 || NO < 0)
    {
        fprintf(stderr, "No negative numbers aloved in any argument\n");
        return 1;
    }
    if (TB > 1000 || TI > 1000)
    {
        fprintf(stderr, "TB or TI have to be less or equal to 1000");
        return 1;
    }

    // converts miliseconds to microseconds
    TI = 1000 * TI;
    TB = 1000 * TB;

    // tries to create output file
    fp = fopen("proj2.out", "w");
    setbuf(fp, NULL);
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open proj2.out\n");
        return 1;
    }

    // unlinks all semaphores who may be there from previous runs
    sem_unlink("SharedMemory_sem_filee");
    sem_unlink("WriteOut_sem");
    sem_unlink("SharedMoleculeCount_sem");
    sem_unlink("O_sem_file");
    sem_unlink("H_sem_file");
    sem_unlink("H_sem2_file");
    sem_unlink("O_sem2_file");
    sem_unlink("SharedOxygenCount_sem");
    sem_unlink("SharedHydrogenCount_sem");

    // opens all needed sempahores
    if ((WriteOut_sem = sem_open("WriteOut_sem", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }
    if ((SharedMoleculeCount_sem = sem_open("SharedMoleculeCount_sem", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }
    if ((SharedOxygenCount_sem = sem_open("SharedOxygenCount_sem", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }
    if ((SharedHydrogenCount_sem = sem_open("SharedHydrogenCount_sem", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }
    if ((O_sem = sem_open("O_sem_file", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }
    if ((O_sem2 = sem_open("O_sem2_file", O_CREAT, 0777, 0)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }

    if ((H_sem = sem_open("H_sem_file", O_CREAT, 0660, 0)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }
    if ((H_sem2 = sem_open("H_sem2_file", O_CREAT, 0660, 0)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }
    if ((SharedMemory_sem = sem_open("SharedMemory_sem_filee", O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        printf("%d", errno);
        return 1;
    }

    // creates all shared varriables
    int *shared_memory = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *SharedMoleculeCount = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *SharedHydrogenCount = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *SharedOxygenCount = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *shared_memory = 0;
    *SharedMoleculeCount = 0;
    *SharedOxygenCount = atoi(argv[1]);
    *SharedHydrogenCount = atoi(argv[2]);

    // inicializes random number generator with time as seed
    time_t t;
    srand((unsigned)time(&t));

    // creates oxygen child proceses
    for (int i = 0; i < NO; i++)
    {
        if (fork() == 0)
        {
            Oxygen(i + 1, TI, TB, shared_memory, SharedMoleculeCount, SharedOxygenCount, SharedHydrogenCount, fp);
        }
    }
    // creates hydrogen child proceses
    for (int i = 0; i < NH; i++)
    {
        if (fork() == 0)
        {
            Hydrogen(i + 1, TI, shared_memory, SharedMoleculeCount, SharedOxygenCount, SharedHydrogenCount, fp);
        }
    }

    int status = 0;
    pid_t pid;
    // parrent waits for all his children to die
    while ((pid = wait(&status)) > 0)
        ;
    // cleans and closes all files,semaphores and shared varriables
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
    sem_unlink("O_sem2_file");
    sem_unlink("SharedOxygenCount_sem");
    sem_unlink("SharedHydrogenCount_sem");
    munmap(shared_memory, 4);
    munmap(SharedMoleculeCount, 4);
    munmap(SharedHydrogenCount, 4);
    munmap(SharedOxygenCount, 4);
    fclose(fp);
    return 0;
}
