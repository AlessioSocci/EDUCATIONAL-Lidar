#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shmem.h"
#include "lid_data.h"

extern void dbg_StoreSamples( char *szStr, int type );

static int lid_ParseMessage( char *szLidString, LID_RECORD *acc );

int lid_StoreMessage( char *szLidString )
{
    LID_RECORD lid;
    int res = LRES_FAILURE;
    char *pt = strchr( szLidString, '$' );      // start of first message
	char *pt_next = 0;

    int dbg_msg = 0;            // DEBUG - ELIMINARE
    int dbg_parsed = 0;         // DEBUG - ELIMINARE
    int dbg_stored = 0;         // DEBUG - ELIMINARE

    dbg_StoreSamples( szLidString, 1 );

	SHMEM_STRUCT *pt_shm = (SHMEM_STRUCT *)shmem_GetSensDataPt( SHM_LID_ID );
	if ( 0 != pt_shm )
	{
		SHMEM_STRUCT_LID *shm_lid = &pt_shm->data_LID;
		while ( pt != 0 )
		{
			pt_next = strchr( pt+1, '$' );
		//printf("\n... next: '%s'", pt_next );		
			// possible message available
			dbg_msg++;

			// ... CHECK SULLA LUNGHEZZA DELLA STRINGA ?
			memset( &lid, 0, sizeof(LID_RECORD) );
			if ( LRES_SUCCESS == lid_ParseMessage( pt, &lid ) )
			{
				dbg_parsed++;
				// stores the message data on the shared memory
				//SHMEM_STRUCT_LID *shm_lid = (SHMEM_STRUCT_LID *)shmem_GetSensDataPt( SHM_LID_ID );
				//if ( 0 != shm_lid )
				{
					dbg_stored++;
					res = LRES_SUCCESS;

					// updates the LID shared area
					memcpy( &shm_lid->sample[shm_lid->pt_wr++], &lid, sizeof(LID_RECORD) );
					if ( shm_lid->pt_wr >= LID_NMAX_RECS )
						shm_lid->pt_wr = 0;     // recirculate writing position to the start of the buffer

					if ( shm_lid->nrecs < LID_NMAX_RECS )
						shm_lid->nrecs++;
					else
					{
						// it shall be turned to false in the reading process
						shm_lid->overrun = true;

						// moves forward the reading pointer (one sample lost)
						shm_lid->pt_rd++;
						if ( shm_lid->pt_rd >= LID_NMAX_RECS )
							shm_lid->pt_rd = 0;

						res = LRES_OVERRUN;
					}

					//shmem_ReleaseSensDataPt( SHM_LID_ID );
				}
			}
			//pt = strchr( pt+1, '$' );
			pt = pt_next;
		} // end while
		shmem_ReleaseSensDataPt( pt_shm );
	}
	else
	{
		printf( "\nLID: SHMEM FAIL!!!");
	}
	
    //printf("\nLID: %d - %d - %d", dbg_msg, dbg_parsed, dbg_stored );    // DEBUG - ELIMINARE
    //if ( dbg_parsed == 0 )
    //    printf( "\nLID msg: '%s'", szLidString );

    return res;
}

int lid_StoreMessageBin( int timestamp, int nsamples, unsigned char *buffer )
{
    LID_RECORD lid;
    int res = LRES_FAILURE;

    if ( nsamples <= 0 || 0 == buffer )
    {
    	return res;		// failure: invalid parameters
    }

	SHMEM_STRUCT *pt_shm = (SHMEM_STRUCT *)shmem_GetSensDataPt( SHM_LID_ID );
	if ( 0 != pt_shm )
	{
		SHMEM_STRUCT_LID *shm_lid = &pt_shm->data_LID;
		int pt = 0;
		for ( int idx=0; idx<nsamples; idx++ )
		{
			lid.deg = *((unsigned short *)&buffer[pt]);
			lid.dist = *((unsigned int *)&buffer[pt+2]);
			lid.power = buffer[pt+6];
			lid.ts_ms = timestamp;

			//dbg_msg++;
			//dbg_stored++;

			res = LRES_SUCCESS;

			// updates the LID shared area
			memcpy( &shm_lid->sample[shm_lid->pt_wr++], &lid, sizeof(LID_RECORD) );
			if ( shm_lid->pt_wr >= LID_NMAX_RECS )
				shm_lid->pt_wr = 0;     // recirculate writing position to the start of the buffer

			if ( shm_lid->nrecs < LID_NMAX_RECS )
				shm_lid->nrecs++;
			else
			{
				// it shall be turned to false in the reading process
				shm_lid->overrun = true;

				// moves forward the reading pointer (one sample lost)
				shm_lid->pt_rd++;
				if ( shm_lid->pt_rd >= LID_NMAX_RECS )
					shm_lid->pt_rd = 0;

				res = LRES_OVERRUN;
			}
			pt += 8;
		} // end for
		shmem_ReleaseSensDataPt( pt_shm );
	}
	else
	{
		printf( "\nLID: SHMEM FAIL!!!");
	}

	return res;
}

int lid_GetSample( LID_RECORD *lid )
{
    int res = LRES_FAILURE;
    if ( lid != 0 )
    {
        // gets the record from the shared memory
        //SHMEM_STRUCT_LID *shm_lid = (SHMEM_STRUCT_LID *)shmem_GetSensDataPt( SHM_LID_ID );
        //if ( 0 != shm_lid )
        SHMEM_STRUCT *pt_shm = (SHMEM_STRUCT *)shmem_GetSensDataPt( SHM_LID_ID );
        if ( 0 != pt_shm )
        {
			SHMEM_STRUCT_LID *shm_lid = &pt_shm->data_LID;
            if ( shm_lid->nrecs > 0 )
            {
                // gets the LID shared area
                memcpy( lid, &shm_lid->sample[shm_lid->pt_rd++], sizeof(LID_RECORD) );
                if ( shm_lid->pt_rd >= LID_NMAX_RECS )
                    shm_lid->pt_rd = 0;     // recirculate writing position to the start of the buffer
                shm_lid->nrecs--;
                shm_lid->overrun = false;
                res = LRES_SUCCESS;
            }
            else
            {
                res = LRES_NOMOREDATA;
            }
            shmem_ReleaseSensDataPt( pt_shm );
        }
        /*
        else
            res = LRES_SEMLOCKED;
        */
    }
    return res;
}


static int lid_ParseMessage( char *szLidString, LID_RECORD *lid )
{
    if ( szLidString != 0 && strlen(szLidString) > MIN_LID_MSGLEN )
    {
        if ( szLidString[0] == '$' )
        {
            double app;
            char *start = szLidString+1;        // bypass '$'
            char *p = strchr( start, ';');
            if ( p != 0 )
            {
                *p = '\0';
                lid->ts_ms = atoi( start );
                start = p+1;

                // distance
                p = strchr( start, ';' );
                if ( p != 0 )
                {
                    *p = '\0';
                    app = atof( start );
                    lid->dist = (int)app;
                    start = p+1;

                    // signal power (always not valid)
                    p = strchr( start, ';' );
                    if ( p != 0 )
                    {
                        *p = '\0';
                        //app = 1000 * atof( start );
                        lid->power = atoi( start );
                        start = p+1;

                        // degree
                        app = 1000 * atof( start );
                        lid->deg = (int)app;

                        return LRES_SUCCESS;
                    }
                }
            }
        }
    }
    memset( lid, 0, sizeof(LID_RECORD) );
    return LRES_FAILURE;
}
