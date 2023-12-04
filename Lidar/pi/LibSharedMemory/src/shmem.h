/*
 * SmartSprayer_runtime shared library
 *
 * Iselqui Technology S.r.l.
 *
 * File:    shmem.h
 * Content: header file for the shmem.c one
 *
 * Revisions
 *  04/01/2021: first emission
 *
 */

#ifndef SHMEM_INCLUDED
#define SHMEM_INCLUDED

#define SHM_GPS_ID  1
#define SHM_ACC_ID  2
#define SHM_LID_ID  3
#define SHM_VLV_ID  4

#define SEM_KEY_GPS 12345
#define SEM_KEY_ACC 13579
#define SEM_KEY_LID 54321
#define SEM_KEY_VLV 97531

#define SHMEM_KEY   67890

typedef union semun
{
   int              val;
   struct semid_ds *buf;
   unsigned short  *array;
   struct seminfo  *__buf;
} SEMNUM_STRUCT;

// shared memory structure
#define GPS_NMAX_RECS   16  //128
#define ACC_NMAX_RECS   16  //128
#define LID_NMAX_RECS   1024  //128
#define NMAX_VALVES     32


typedef struct
{
    unsigned int    ts_ms;
    //char            meas[128];
    double          lat;
    double          lon;
    int             height_mm;
    int             hdop;
    int             nsat;
} GPS_RECORD;

typedef struct
{
    unsigned int    ts_ms;
    //            meas[128];
    int             deg_x;      // x1000
    int             deg_y;      // x1000
    int             deg_z;      // x1000
} ACC_RECORD;

typedef struct
{
    unsigned int    ts_ms;
    unsigned int    dist;
    int             deg;        // x1000
    int             power;
} LID_RECORD;

typedef struct
{
    bool            overrun;
    int             nrecs;
    int             pt_wr;
    int             pt_rd;
    GPS_RECORD      sample[GPS_NMAX_RECS];
} SHMEM_STRUCT_GPS;

typedef struct
{
    bool            overrun;
    int             nrecs;
    int             pt_wr;
    int             pt_rd;
    ACC_RECORD      sample[ACC_NMAX_RECS];
} SHMEM_STRUCT_ACC;

typedef struct
{
    bool            overrun;
    int             nrecs;
    int             pt_wr;
    int             pt_rd;
    LID_RECORD      sample[LID_NMAX_RECS];
} SHMEM_STRUCT_LID;

typedef struct
{
    int             num;
    int             status[32];
} SHMEN_STRUCT_VLV;

typedef struct
{
    // ... serve un header? (se si devo proteggerlo con un semaforo)
	unsigned int		timestamp;
	SHMEM_STRUCT_GPS    data_GPS;
	SHMEM_STRUCT_ACC    data_ACC;
	SHMEM_STRUCT_LID    data_LID;
	SHMEN_STRUCT_VLV    valves;
} SHMEM_STRUCT;

// functions prototypes
int shmem_InitSemaphore( void );
int shmem_InitDataArea( void );
int shmem_RemoveDataArea( void );
SHMEM_STRUCT *shmem_GetDataAreaPt( void );
int shmem_ReleaseDataAreaPt( SHMEM_STRUCT *pdata );
int shmem_lock( int area_id );              // ... rimuovere: deve diventare static
int shmem_unlock( int area_id );            // ... rimuovere: deve diventare static
void *shmem_GetSensDataPt( int area_id );
int shmem_ReleaseSensDataPt( void *pt_shm );
int shmem_SetTimestamp( int ts );
int shmem_GetTimestamp( void );
// ...

#endif // SHMEM_INCLUDED
