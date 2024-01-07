#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdbool.h>
#include "libSmartSprayer.h"
#include "shmem.h"

static int shmem_InitDataAreaStructs( void );

static int shmem_Semid_GPS = -1;
static int shmem_Semid_ACC = -1;
static int shmem_Semid_LID = -1;
static int shmem_Semid_VLV = -1;

static SHMEM_STRUCT *shmemData = 0;
static int shmem_Dataid = -1;

int shmem_InitSingleSemaphore( int *sem_id, int sem_key )
{
    // try to get the semaphore supposing it already exists
    *sem_id = semget((key_t)sem_key, 1, 0666 );
    if ( -1 == *sem_id )
    {
        // the semaphore doesn't exist: try to create it
        *sem_id = semget((key_t)sem_key, 1, 0666 | IPC_CREAT | IPC_EXCL );
        if ( -1 != *sem_id )
        {
            //Initialize the semaphore using the SETVAL command in a semctl call (required before it can be used)
            SEMNUM_STRUCT sem_union_init;
            sem_union_init.val = 1;
            if ( -1 == semctl(*sem_id, 0, SETVAL, sem_union_init) )
            {
                return LRES_FAILURE;
            }
        }
        else
        {
            return LRES_FAILURE;
        }
    }
    return LRES_SUCCESS;
}

int shmem_InitSemaphore( void )
{
    // Creation of a sempaphore for every sensor data.
    // In particular:
    // - GPS data (SHM_GPS_ID sub-area identifier)
    // - ACC data (SHM_ACC_ID sub-area identifier)
    // - LID data (SHM_LID_ID sub-area identifier)
    // - VLV data (SHM_VLV_ID sub-area identifier)

    // GPS semaphore initialization
    if ( LRES_SUCCESS != shmem_InitSingleSemaphore( &shmem_Semid_GPS, SEM_KEY_GPS ) )
    {
        return LRES_FAILURE;
    }

    // ACC semaphore initialization
    if ( LRES_SUCCESS != shmem_InitSingleSemaphore( &shmem_Semid_ACC, SEM_KEY_ACC ) )
    {
        return LRES_FAILURE;
    }

    // LID semaphore initialization
    if ( LRES_SUCCESS != shmem_InitSingleSemaphore( &shmem_Semid_LID, SEM_KEY_LID ) )
    {
        return LRES_FAILURE;
    }

    // VLV semaphore initialization
    if ( LRES_SUCCESS != shmem_InitSingleSemaphore( &shmem_Semid_VLV, SEM_KEY_VLV ) )
    {
        return LRES_FAILURE;
    }

    return LRES_SUCCESS;
}

int shmem_InitDataArea( void )
{
    // try to get the shared memory, supposing it exists
	shmem_Dataid = shmget( (key_t)SHMEM_KEY, sizeof(SHMEM_STRUCT), 0666 );
	if ( -1 == shmem_Dataid )
    {
        //printf("\n... Shared area doesn't exist: creates it");
        // shared memory doesn't exists: try to create it
    	shmem_Dataid = shmget( (key_t)SHMEM_KEY, sizeof(SHMEM_STRUCT), 0666 | IPC_CREAT | IPC_EXCL);    // if the area exists, it fails
        if ( -1 == shmem_Dataid )
        {
            //printf( "\n... UNABLE TO GET/CREATE THE SHARED MEMORY!!!\n" );
            return LRES_FAILURE;
        }

        if ( LRES_SUCCESS != shmem_InitDataAreaStructs() )
        {
            //printf( "\n... UNABLE TO INITIALIZE THE SHARED MEMORY DATA!!!\n" );
            return LRES_FAILURE;
        }
    }
    /*
    else
        printf("\n... Shared area already exists");
	*/
    printf( "\n... DataId = %d", shmem_Dataid );
    return LRES_SUCCESS;
}


int shmem_RemoveDataArea( void )
{
    // try to get the shared memory, supposing it exists
	shmem_Dataid = shmget( (key_t)SHMEM_KEY, sizeof(SHMEM_STRUCT), 0666 );
	if ( -1 == shmem_Dataid )
	{
        //printf("\n... Shared area doesn't exist: nothing to do!!!");
    }
    else
    {
        //printf("\nRemoving shared area ...");

        // shared memory removing
        if ( shmctl(shmem_Dataid, IPC_RMID, NULL) < 0 )
        {
            //printf( "\n... UNABLE TO REMOVE THE SHARED MEMORY!!!\n" );
            return LRES_FAILURE;
        }
        /*
        else
            printf( "\n... Shared memory removed!" );
        */
    }
    return LRES_SUCCESS;
}

SHMEM_STRUCT *shmem_GetDataAreaPt( void )
{
    /*
    printf( "\n--------------------");
    printf( "\nshmem_GetDataAreaPt");
    printf( "\nshmem_DataId = %d", shmem_Dataid );
    */
    if ( -1 != shmem_Dataid )
    {
        //Makes the shared memory accessible to the program
        shmemData = (SHMEM_STRUCT *)shmat(shmem_Dataid, (void *)0, 0);
        if (shmemData == (void *)-1)
            shmemData = 0;      // failure
    }
    /*
    printf( "\nshmemData = %ld", (long int)shmemData );
    printf( "\n--------------------");
    */
    return shmemData;
}

int shmem_ReleaseDataAreaPt( SHMEM_STRUCT *pdata )
{
    /*
    printf( "\n--------------------");
    printf( "\nshmem_ReleaseDataAreaPt");
    printf( "\nshmem_DataId = %d", shmem_Dataid );
    printf( "\npdata = %ld", (long int)pdata );
    */
    if ( 0 != pdata )
    {
        //Makes the shared memory accessible to the program
        if ( 0 != shmdt( pdata ) )
        {
            //printf("\nshmdt FAILED!!!");
            return LRES_FAILURE;
        }
    }
    //printf( "\n--------------------");

    return LRES_SUCCESS;
}

void *shmem_GetSensDataPt( int area_id )
{
    SHMEM_STRUCT *shmemData = 0;
    // tries to lock the semaphore
    if ( -1 != shmem_Dataid )
    {
        //if ( LRES_SUCCESS == shmem_lock( area_id ))
        {
            shmemData = (SHMEM_STRUCT *)shmat(shmem_Dataid, (void *)0, 0);
            if (shmemData != (void *)-1)
            {
				/*
                switch ( area_id )
                {
                    case SHM_GPS_ID:    return (void *)&shmemData->data_GPS;
                    case SHM_ACC_ID:    return (void *)&shmemData->data_ACC;
                    case SHM_LID_ID:    return (void *)&shmemData->data_LID;
                    case SHM_VLV_ID:    return (void *)&shmemData->valves;
                    default:	// invalid area id -> already checked by "shmem_lock"
						printf("\n... area_id INVALID !!!");		// DEBUG - ELIMINARE !!!
                }
				*/
				return (void *)shmemData;
            }
			else
			{
				printf("\n... shmat FAILURE !!! (%d)", shmem_Dataid );		// DEBUG - ELIMINARE !!!
			}
            // failure: unlocks the semaphore
            //shmem_unlock( area_id );
        }
    }
	else
		printf("\n... shmem_Dataid = -1 !!!");		// DEBUG - ELIMINARE !!!
    return 0;   // failure
}

int shmem_ReleaseSensDataPt( /*int area_id*/ void *pt_shm )
{
    //return shmem_unlock( area_id );
	if ( 0 != shmdt( pt_shm ) )
		return LRES_FAILURE;
	return LRES_SUCCESS;
}


int shmem_lock( int area_id )
{
	struct sembuf sem_b;
	int semid = -1;
    switch ( area_id )
    {
        case SHM_GPS_ID:    semid = shmem_Semid_GPS;    break;
        case SHM_ACC_ID:    semid = shmem_Semid_ACC;    break;
        case SHM_LID_ID:    semid = shmem_Semid_LID;    break;
        case SHM_VLV_ID:    semid = shmem_Semid_VLV;    break;
        default:            return LRES_FAILURE;
    }
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;
	sem_b.sem_flg = IPC_NOWAIT | SEM_UNDO;
	if ( semop(semid, &sem_b, 1 ) == -1 )
	{
		return LRES_FAILURE;
	}
	return LRES_SUCCESS;
}


int shmem_unlock( int area_id )
{
	struct sembuf sem_b;
	int semid = -1;
	int semstat;
    switch ( area_id )
    {
        case SHM_GPS_ID:    semid = shmem_Semid_GPS;    break;
        case SHM_ACC_ID:    semid = shmem_Semid_ACC;    break;
        case SHM_LID_ID:    semid = shmem_Semid_LID;    break;
        case SHM_VLV_ID:    semid = shmem_Semid_VLV;    break;
        default:            return LRES_FAILURE;
    }

	// check for sempahore status to avoid multiple unlocks
	semstat = semctl(semid, 0, GETVAL, 0);
	if ( semstat > 0 )
	{
        return LRES_SUCCESS;    // semaphore already unlocked
    }
    else if ( semstat < 0 )
    {
        return LRES_FAILURE;    // unable to get semaphore status
    }

	sem_b.sem_num = 0;
	sem_b.sem_op = 1;
	sem_b.sem_flg = IPC_NOWAIT | SEM_UNDO;
	if ( semop(semid, &sem_b, 1) == -1 )
	{
		return LRES_FAILURE;
	}
	return LRES_SUCCESS;
}


int shmem_SetTimestamp( int ts )
{
	int ret = LRES_FAILURE;
    SHMEM_STRUCT *pt_shm = 0;
    // tries to lock the semaphore
    if ( -1 != shmem_Dataid )
    {
		pt_shm = (SHMEM_STRUCT *)shmat(shmem_Dataid, (void *)0, 0);
		if ( pt_shm != (void *)-1 )
		{
			pt_shm->timestamp = ts;
			shmdt( pt_shm );
			ret = LRES_SUCCESS;
		}
	}
	return ret;
}


int shmem_GetTimestamp( void )
{
    SHMEM_STRUCT *pt_shm = 0;
	int ts = 0;
    // tries to lock the semaphore
    if ( -1 != shmem_Dataid )
    {
		pt_shm = (SHMEM_STRUCT *)shmat(shmem_Dataid, (void *)0, 0);
		if ( pt_shm != (void *)-1 )
		{
			ts = pt_shm->timestamp;
			shmdt( pt_shm );
		}
	}
	return ts;
}


static int shmem_InitDataAreaStructs( void )
{
    SHMEM_STRUCT *pt = shmem_GetDataAreaPt();
    if ( pt != 0 )
    {
        memset( &pt->data_GPS, 0, sizeof(SHMEM_STRUCT_GPS) );
        memset( &pt->data_ACC, 0, sizeof(SHMEM_STRUCT_ACC) );
        memset( &pt->data_LID, 0, sizeof(SHMEM_STRUCT_LID) );
        memset( &pt->valves,   0, sizeof(SHMEN_STRUCT_VLV) );
        shmem_ReleaseDataAreaPt( pt );
        return LRES_SUCCESS;
    }
    return LRES_FAILURE;
}
