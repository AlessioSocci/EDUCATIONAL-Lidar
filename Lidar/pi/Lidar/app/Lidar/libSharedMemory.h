
#ifndef LIBSMARTSPRAYER_INCLUDED
#define LIBSMARTSPRAYER_INCLUDED

// Exported functions return codes
#define LRES_SUCCESS        0
#define LRES_FAILURE        1
#define LRES_NOMOREDATA     2
#define LRES_OVERRUN        3
#define LRES_SEMLOCKED      4
// ...

// Common data structures
typedef struct
{
    unsigned int    smp_ts_ms;
    int             smp_deg_x;      // x 1000
    int             smp_deg_y;      // x 1000
    int             smp_deg_z;      // x 1000
} LIB_ACC_DATA;

typedef struct
{
    unsigned int    smp_ts_ms;
    int             smp_dist;       // mm
    int             smp_power;      // integer
    int             smp_degree;     // x 1000
} LIB_LID_DATA;

typedef struct
{
    unsigned int    smp_ts_ms;
    double          smp_lat;
    double          smp_lon;
    int             smp_height_mm;
    //int             hdop;
    //int             nsat;
} LIB_GPS_DATA;


int InitLib(void);
int DeinitLib(void);
int StoreMessage_LID(wchar_t *szLidString);
int GetSample_LID(LIB_LID_DATA *lid_data);
int LockArea_LID(void);
int UnlockArea_LID(void);
int SetTimestamp(int ts);
int GetTimestamp(void);
int lid_StoreMessageBin(int timestamp, int nsamples, unsigned char *buffer);



#endif 
