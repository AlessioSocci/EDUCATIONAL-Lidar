#ifndef LIDDATA_INCLUDED
#define LIDDATA_INCLUDED

#define MIN_LID_MSGLEN      19

int lid_StoreMessage( char *szAccString );
int lid_GetSample( LID_RECORD *acc );
int lid_StoreMessageBin( int timestamp, int nsamples, unsigned char *buffer );


#endif // SHMEM_INCLUDED
