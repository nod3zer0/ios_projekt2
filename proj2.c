/**
 * @file proj2
 * @author RENE CESKA xceska06
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
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
 * @brief writes to file safely
 *
 */
#define write_out(SharedWriteoutCount, file, format, ...) \
    sem_wait(WriteOut_sem);                               \
    incSharedMemory(SharedWriteoutCount);                 \
    fprintf(file, format, __VA_ARGS__);                   \
    sem_post(WriteOut_sem);

/**
 * @brief inicializes semaphore with name
 *
 */
#define initSem(name, initNumber)                                          \
    if ((name = sem_open(#name, O_CREAT, 0777, initNumber)) == SEM_FAILED) \
    {                                                                      \
        printf("%d", errno);                                               \
        cleanSemaphores();                                                 \
        return 1;                                                          \
    }

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
 * @brief Checks if there are anz hydrogens left, If not it writes message and exits
 *
 * @param SharedHydrogenCount
 * @param fp
 * @param SharedWriteoutCount
 * @param id
 * @param SharedOxygenCount
 */
void checkForHydrogenRemoveOxygen(int *SharedHydrogenCount, FILE *fp, int *SharedWriteoutCount, int id, int *SharedOxygenCount)
{
    if (*SharedHydrogenCount < 2)
    {
        for (int i = 0; i <= *SharedHydrogenCount; i++)
        {
            sem_post(H_sem);
            sem_post(H_sem2);
            sem_post(H_sem);
            sem_post(H_sem2);
        }
        write_out(SharedWriteoutCount, fp, "%d: O %d: not enough H\n", *SharedWriteoutCount, id);
        decSharedOxygen(SharedOxygenCount);

        exit(0);
    }
}

/**
 * @brief Checks if there are any hydrogens or oxygens left, If not it writes message and exits
 *
 * @param SharedHydrogenCount
 * @param fp
 * @param SharedWriteoutCount
 * @param id
 * @param SharedOxygenCount
 */
void checkForOxygenAndHydrogenRemoveHydrogen(int *SharedHydrogenCount, FILE *fp, int *SharedWriteoutCount, int id, int *SharedOxygenCount)
{
    if (*SharedHydrogenCount < 2 || *SharedOxygenCount < 1)
    {
        if (*SharedHydrogenCount < 2)
        {
            for (int i = 0; i <= *SharedHydrogenCount; i++)
            {
                sem_post(H_sem);
                sem_post(H_sem2);
                sem_post(H_sem);
                sem_post(H_sem2);
            }
            for (int i = 0; i <= *SharedOxygenCount; i++)
            {
                sem_post(O_sem);
                sem_post(O_sem2);
                sem_post(O_sem);
                sem_post(O_sem2);
            }
        }

        write_out(SharedWriteoutCount, fp, "%d: H %d: not enough O or H\n", *SharedWriteoutCount, id);
        decSharedHydrogen(SharedHydrogenCount);
        exit(0);
    }
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
    // inicializes random number generator with time as seed
    time_t t;
    srand((unsigned)time(&t) ^ getpid());
    // safely increses SharedWriteoutCount and writes to output file
    write_out(SharedWriteoutCount, fp, "%d: O %d: started\n", *SharedWriteoutCount, id);

    // waits for random time in intervall <0,TI>
    if (TI != 0)
    {
        usleep((rand() % TI)*1000);
    }
    else
    {
        usleep(0);
    }

    // safely increses SharedWriteoutCount and writes to output file
    write_out(SharedWriteoutCount, fp, "%d: O %d: going to queue\n", *SharedWriteoutCount, id);

    // if there is not enough hydrogens, it writes message and ends
    checkForHydrogenRemoveOxygen(SharedHydrogenCount, fp, SharedWriteoutCount, id, SharedOxygenCount);

    // enters queue (waits at semaphore)
    sem_wait(O_sem);

    // if there is not enough hydrogens, it writes message and ends
    checkForHydrogenRemoveOxygen(SharedHydrogenCount, fp, SharedWriteoutCount, id, SharedOxygenCount);

    incMoleculeCount(SharedMoleculeCount);

    // takes two hydrogens from queue
    sem_post(H_sem);
    sem_post(H_sem);

    // if there is not enough hydrogens, it writes message and ends
    checkForHydrogenRemoveOxygen(SharedHydrogenCount, fp, SharedWriteoutCount, id, SharedOxygenCount);
    // waits until 2 hydrogens start creating molecule
    sem_wait(O_sem2);
    sem_wait(O_sem2);

    // if there is not enough hydrogens, it writes message and ends
    checkForHydrogenRemoveOxygen(SharedHydrogenCount, fp, SharedWriteoutCount, id, SharedOxygenCount);

    // starts creating molecule
    write_out(SharedWriteoutCount, fp, "%d: O %d: creating molecule %d\n", *SharedWriteoutCount, id, *SharedMoleculeCount);
    if (TI != 0)
    {
        usleep((rand() % TB)*1000);
    }
    else
    {
        usleep(0);
    }
    // when finished lets hydrogens know that molecule is finished
    sem_post(H_sem2);
    sem_post(H_sem2);

    // writes that molecule is finished
    write_out(SharedWriteoutCount, fp, "%d: O %d: molecule %d created\n", *SharedWriteoutCount, id, *SharedMoleculeCount);
    // waits until 2 hydrogens are finished
    sem_wait(O_sem2);
    sem_wait(O_sem2);
    // lets next oxygen out from queue
    sem_post(O_sem);

    decSharedOxygen(SharedOxygenCount);

    // if there are no more oxygens it lets all hydrogens free from queue
    if (*SharedOxygenCount == 0)
    {
        for (int i = 0; i <= *SharedHydrogenCount; i++)
        {
            sem_post(H_sem);
            sem_post(H_sem2);
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
void Hydrogen(int id, int TI, int *SharedWriteoutCount, int *SharedMoleculeCount, int *SharedOxygenCount, int *SharedHydrogenCount, FILE *fp)
{
    // inicializes random number generator with time as seed
    time_t t;
    srand((unsigned)time(&t) ^ getpid());
    // safely increses SharedWriteoutCount and writes to output file
    write_out(SharedWriteoutCount, fp, "%d: H %d: started\n", *SharedWriteoutCount, id);
    // waits for random time in intervall <0,TI>
    if (TI != 0)
    {
        usleep((rand() % TI)*1000);
    }
    else
    {
        usleep(0);
    }

    // safely increses SharedWriteoutCount and writes to output file
    write_out(SharedWriteoutCount, fp, "%d: H %d: going to queue\n", *SharedWriteoutCount, id);

    // if there is not enough hydrogens or oxygens, it writes message and ends
    checkForOxygenAndHydrogenRemoveHydrogen(SharedHydrogenCount, fp, SharedWriteoutCount, id, SharedOxygenCount);

    // waits until oxygen lets it to create molecule
    sem_wait(H_sem);

    // if there is not enough hydrogens or oxygens, it writes message and ends
    checkForOxygenAndHydrogenRemoveHydrogen(SharedHydrogenCount, fp, SharedWriteoutCount, id, SharedOxygenCount);
    // safely writes that it is creting molecule
    write_out(SharedWriteoutCount, fp, "%d: H %d: creating molecule %d\n", *SharedWriteoutCount, id, *SharedMoleculeCount);
    // lets oxygen know that it is ready to create molecule
    sem_post(O_sem2);
    // waits until hydrogen says that molecule is created
    sem_wait(H_sem2);
    // writes that molecule is finished
    write_out(SharedWriteoutCount, fp, "%d: H %d: molecule %d created\n", *SharedWriteoutCount, id, *SharedMoleculeCount);

    decSharedHydrogen(SharedHydrogenCount);

    // lets oxygen go
    sem_post(O_sem2);
    // if there is not enought hydrogens, it lets all hydrogens and oxygens free from queue
    if (*SharedHydrogenCount < 2)
    {
        for (int i = 0; i <= *SharedHydrogenCount; i++)
        {
            sem_post(H_sem);
            sem_post(H_sem2);
            sem_post(H_sem);
            sem_post(H_sem2);
        }
        for (int i = 0; i <= *SharedOxygenCount; i++)
        {
            sem_post(O_sem);
            sem_post(O_sem2);
            sem_post(O_sem);
            sem_post(O_sem2);
        }
    }

    exit(0);
}

void cleanSemaphores()
{
    sem_close(O_sem);
    sem_close(O_sem2);
    sem_close(H_sem);
    sem_close(H_sem2);
    sem_close(SharedMemory_sem);
    sem_close(SharedMoleculeCount_sem);
    sem_close(SharedHydrogenCount_sem);
    sem_close(SharedOxygenCount_sem);
    sem_close(WriteOut_sem);
    sem_unlink("O_sem");
    sem_unlink("O_sem2");
    sem_unlink("H_sem");
    sem_unlink("H_sem2");
    sem_unlink("SharedMemory_sem");
    sem_unlink("SharedMoleculeCount_sem");
    sem_unlink("SharedHydrogenCount_sem");
    sem_unlink("SharedOxygenCount_sem");
    sem_unlink("WriteOut_sem");
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
     if (NH == 0 && NO == 0)
    {
        fprintf(stderr, "No atoms inputed\n");
        return 1;
    }
    if (TB > 1000 || TI > 1000)
    {
        fprintf(stderr, "TB or TI have to be less or equal to 1000");
        return 1;
    }

    // converts miliseconds to microseconds
    TI =  TI;
    TB =  TB;

    // tries to create output file
    fp = fopen("proj2.out", "w");
    setbuf(fp, NULL);
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open proj2.out\n");
        return 1;
    }

    // unlinks all semaphores who may be there from previous runs
    cleanSemaphores();

    // opens all needed semaphores
    initSem(WriteOut_sem, 1);
    initSem(SharedMoleculeCount_sem, 1);
    initSem(SharedOxygenCount_sem, 1);
    initSem(SharedHydrogenCount_sem, 1);
    initSem(O_sem, 1);
    initSem(O_sem2, 0);
    initSem(H_sem, 0);
    initSem(H_sem2, 0);
    initSem(SharedMemory_sem, 1);

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
    srand((unsigned)time(&t) ^ getpid());

    // creates oxygen child proceses
    for (int i = 0; i < NO; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            Oxygen(i + 1, TI, TB, shared_memory, SharedMoleculeCount, SharedOxygenCount, SharedHydrogenCount, fp);
        }
        else if (pid < 0)
        {
            decSharedOxygen(SharedOxygenCount);
            fprintf(stderr, "Fork failed, exiting main proces, waiting for alredy created atoms to finish.\n");
            break;
        }
    }
    // creates hydrogen child proceses
    for (int i = 0; i < NH; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            Hydrogen(i + 1, TI, shared_memory, SharedMoleculeCount, SharedOxygenCount, SharedHydrogenCount, fp);
        }
        else if (pid < 0)
        {
            decSharedHydrogen(SharedHydrogenCount);
            fprintf(stderr, "Fork failed, exiting main proces, waiting for alredy created atoms to finish.\n");
            break;
        }
    }

    int status = 0;
    pid_t pid;
    // parrent waits for all his children to die before dying himself
    while ((pid = wait(&status)) > 0)
        ;
    // cleans and closes all files,semaphores and shared varriables
    cleanSemaphores();
    munmap(shared_memory, 4);
    munmap(SharedMoleculeCount, 4);
    munmap(SharedHydrogenCount, 4);
    munmap(SharedOxygenCount, 4);
    fclose(fp);
    return 0;
}
