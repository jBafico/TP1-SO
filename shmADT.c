// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "shmADT.h"

struct shmCDT{
    sem_t *mutexSem;
    int shmFd;
    int shmSize;
    int currentPos;
    bool creator;
    char *shmName;
    char *shmPtr;
    char *semName;
};

shmADT initiateSharedData(char * shmName, char * semName, int shmSize) {
    shmADT sharedData = malloc(sizeof(struct shmCDT));

    if(sharedData == NULL){
        errno = ENOMEM;
        return NULL;
    }

    sharedData->shmName = shmName;
    sharedData->semName = semName;
    sharedData->shmSize = shmSize;
    sharedData->currentPos=0;
    sharedData->creator = true;

    //Just in case there is something at that path I need to unlink it first
    shm_unlink(shmName);
    sem_unlink(semName);

    //SHM CREATION
    sharedData->shmFd=shm_open(shmName, O_CREAT | O_RDWR | O_EXCL, S_IWUSR | S_IRUSR );
    if(sharedData->shmFd==ERROR){
        errno=ENOMEM;
        free(sharedData);
        return NULL;
    }

    if(ftruncate(sharedData->shmFd,sharedData->shmSize)==ERROR){
        errno=ENOMEM;
        free(sharedData);
        return NULL;
    }

    sharedData->shmPtr = mmap(NULL, sharedData->shmSize, PROT_READ|PROT_WRITE, MAP_SHARED, sharedData->shmFd, 0);
    if(sharedData->shmPtr == MAP_FAILED){
        errno=ENOMEM;
        free(sharedData);
        return NULL;
    }
    //SEM CREATION
    sharedData->mutexSem = sem_open(semName, O_CREAT |  O_EXCL ,  S_IRUSR| S_IWUSR | S_IROTH| S_IWOTH, 0);
    if(sharedData->mutexSem==SEM_FAILED){
        errno=ENOMEM;
        free(sharedData);
        return NULL;
    }

    return sharedData;
}

shmADT openSharedData(char * shmName, char * semName, int shmSize) {
    shmADT sharedData = malloc(sizeof(struct shmCDT));

    if(sharedData == NULL){
        errno = ENOMEM;
        return NULL;
    }

    sharedData->shmName = shmName;
    sharedData->semName = semName;
    sharedData->shmSize=shmSize;
    sharedData->creator = false;
    //SHM CREATION
    sharedData->shmFd=shm_open(shmName, O_RDWR, S_IWUSR | S_IRUSR );
    if(sharedData->shmFd==ERROR){
        errno=ENOMEM;
        freeShm(sharedData);
        return NULL;
    }
    char *shmPtr = mmap(NULL, sharedData->shmSize, PROT_READ|PROT_WRITE, MAP_SHARED, sharedData->shmFd, 0);
    if(shmPtr == MAP_FAILED){
        errno=ENOMEM;
        freeShm(sharedData);
        return NULL;
    }
    sharedData->shmPtr=shmPtr;

    //SEM CREATION
    sharedData->mutexSem = sem_open(semName, 0);
    if(sharedData->mutexSem==SEM_FAILED){
        errno=ENOMEM;
        freeShm(sharedData);
        return NULL;
    }
    return sharedData;
}


int shmWriter(shmADT data, char * buff){
    if(data == NULL || buff == NULL){
        errno=EINVAL;
        return ERROR;
    }

    int bytesWritten;
    bytesWritten = sprintf(&(data->shmPtr[data->currentPos]), "%s", buff);

    if(bytesWritten > 0)
        data->currentPos += bytesWritten + 1;

    return bytesWritten;
}

//Returns qty of bytes read
int shmReader(shmADT data, char * buff){
    if(data == NULL || buff == NULL){
        errno=EINVAL;
        return ERROR;
    }

    int bytesRead;
    bytesRead = sprintf(buff, "%s", &(data->shmPtr[data->currentPos]));

    if(bytesRead > 0)
        data->currentPos += bytesRead + 1;

    return bytesRead;
}

void freeShm(shmADT data){
    free(data);
}

static int unlinkShmAndSem(shmADT data){
    if(shm_unlink(data->shmName)<0 || sem_unlink(data->semName)<0){
        errno=EINVAL;
        freeShm(data);
        return ERROR;
    }
    freeShm(data);
    return 0;
}

int closeShm(shmADT data){
    if(data == NULL){
        errno=EINVAL;
        return ERROR;
    }

    if(munmap(data->shmPtr, data->shmSize) == ERROR || close(data->shmFd) == ERROR || sem_close(data->mutexSem) == ERROR){
        unlinkShmAndSem(data);
        errno=EINVAL;
        return ERROR;
    }

    if(data->creator)
        return unlinkShmAndSem(data);

    return 0;
}

sem_t *getSem(shmADT data){
    return data->mutexSem;
}

