/*
 * SmartSprayer_runtime shared library
 *
 * Iselqui Technology S.r.l.
 *
 * Content: functions dedicated to the managing of runtime work of the sprayer
 *
 * Revisions
 *  29/12/2020: first emission
 *
 */
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdbool.h>
#include <time.h>

#include "shmem.h"
#include "lid_data.h"

static char *util_GetString( wchar_t *wcsStr, char *szStr, int maxlen );

int InitLib( void )
{
    if ( LRES_SUCCESS != shmem_InitSemaphore() )
    {
        //printf("\n*** shmem_CreateSemaphore FAILED! ***");
        return LRES_FAILURE;
    }
    /*
    else
        printf("\n*** shmem_CreateSemaphore SUCCESS! ***");
    */

    if ( LRES_SUCCESS != shmem_InitDataArea() )
    {
        //printf("\n*** shmem_CreateDataArea FAILED! ***");
        return LRES_FAILURE;
    }
    /*
    else
        printf("\n*** shmem_CreateDataArea SUCCESS! ***");
    */

    /*
    SHMEM_STRUCT *pdata = shmem_GetDataAreaPt();
    printf("\n*** pdata = %ld ***\n\n", (long)pdata );

    int res = shmem_ReleaseDataAreaPt( pdata );
    printf("\n*** shmem_ReleaseDataAreaPt returned %d ***\n\n", res );
    */
    return LRES_SUCCESS;
}

int DeinitLib( void )
{
    // removes the shared memory data
    if ( LRES_SUCCESS != shmem_RemoveDataArea() )
        return LRES_FAILURE;

    // ... rimuovere semafori ...

    return LRES_SUCCESS;
}

int StoreMessage_LID( wchar_t *szLidString )
{
    char szOut[1024];

    /* DEBUG - ELIMINARE ->
    {
        char *szBuff = (char *)szLidString;
	printf( "\nLID par: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
	    szBuff[0], szBuff[1], szBuff[2], szBuff[3], szBuff[4],
	    szBuff[5], szBuff[6], szBuff[7], szBuff[8], szBuff[9],
	    szBuff[10], szBuff[11], szBuff[12], szBuff[13], szBuff[14], szBuff[15] );
    }
    // DEBUG - ELIMINARE <-
    */
    // ... problemi dimensionamento massimo buffer??? VERIFICARE...
    util_GetString( szLidString, szOut, 1024 );
#ifdef DEBUG_PRINTF
    printf("\n[StoreMessage_LID]: szOut = '%s'\n", szOut );     // DEBUG - ELIMINARE
#endif
    return lid_StoreMessage( szOut );
}

int StoreMessageBin_LID( int timestamp, int nsamples, void *buffer )
{
    return lid_StoreMessageBin( timestamp, nsamples, (unsigned char *)buffer );
}

int GetSample_LID( LIB_LID_DATA *lid_data )
{
    LID_RECORD lid;
    int res = LRES_FAILURE;

    if ( lid_data != 0 )
    {
        res = lid_GetSample( &lid );
        if ( LRES_SUCCESS == res )
        {
            lid_data->smp_ts_ms = lid.ts_ms;
            lid_data->smp_dist = lid.dist;
            lid_data->smp_power = lid.power;
            lid_data->smp_degree = lid.deg;
        }
    }
    return res;
}


int LockArea_LID( void  )
{
    int res = shmem_lock( SHM_LID_ID );
    //printf( "\nLID LOCK: %d", res );
    return res;
}

int UnlockArea_LID( void )
{
    int res = shmem_unlock( SHM_LID_ID );
    //printf( "\nLID UNLOCK: %d", res );
    return res;
}



int SetTimestamp( int ts )
{
	return shmem_SetTimestamp( ts );
}

int GetTimestamp( void )
{
	return shmem_GetTimestamp();
}

static char *util_GetString( wchar_t *wcsStr, char *szStr, int maxlen )
{
    int pt = 0;
    //memset( szStr, 0, maxlen );
    while ( wcsStr[pt] > 0 && wcsStr[pt] < 128 && pt < (maxlen-1))
    {
        szStr[pt] = (char)wcsStr[pt];
        pt++;
    }
    szStr[pt] = '\0';   // string terminator
    return szStr;
}

