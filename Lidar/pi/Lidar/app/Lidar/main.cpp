#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <wchar.h>
#include <wiringPi.h>
#include <fstream>

#include "libSharedMemory.h"
#include "sl_lidar.h" 
#include "sl_lidar_driver.h"


#ifndef 	_countof
#define 	_countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif


#define SSPRAY_LIB_NAME		"/home/pi/LibSharedMemory/Debug/LibSharedMemory.so"

/*
 +-----+-----+---------+------+---+---Pi 4B--+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
 |   2 |   8 |   SDA.1 |   IN | 1 |  3 || 4  |   |      | 5v      |     |     |
 |   3 |   9 |   SCL.1 |   IN | 1 |  5 || 6  |   |      | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 1 | IN   | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | IN   | RxD     | 16  | 15  |
 |  17 |   0 | GPIO. 0 |   IN | 0 | 11 || 12 | 0 | IN   | GPIO. 1 | 1   | 18  |
 |  27 |   2 | GPIO. 2 |   IN | 0 | 13 || 14 |   |      | 0v      |     |     |
 |  22 |   3 | GPIO. 3 |   IN | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |
 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
 |  10 |  12 |    MOSI |   IN | 0 | 19 || 20 |   |      | 0v      |     |     |
 |   9 |  13 |    MISO |   IN | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 1 | IN   | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 1 | OUT  | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
 |  19 |  24 | GPIO.24 |   IN | 0 | 35 || 36 | 0 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | IN   | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 0 | IN   | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+---Pi 4B--+---+------+---------+-----+-----+
*/

#define LED 		25		
#define SWITCH		5		

#ifdef 		_WIN32
	#include <Windows.h>
	#define 	delay(x)   ::Sleep(x)
#else
	#include <unistd.h>

using namespace sl;

bool ctrl_c_pressed;

/*
static inline void delay(sl_word_size_t ms)
{
   	while(ms>=1000)
	{
        	usleep(1000*1000);
        	ms-=1000;
    	};
    
	if(ms!=0)
        
	usleep(ms*1000);
}
*/

#endif


void *gLibHandle;

int (*fp_InitLib)(void);
int (*fp_DeinitLib)(void);

int (*fp_StoreMessage_LID)(wchar_t *);
int (*fp_GetSample_LID)(LIB_LID_DATA *);

int (*fp_LockLID)(void);
int (*fp_UnlockLID)(void);

int (*fp_GetTimestamp)(void);
int (*fp_StoreMessageBin_LID)(int timestamp, int nsamples, void *buffer);



static int LoadLibrary(void)
{
    gLibHandle = dlopen(SSPRAY_LIB_NAME, RTLD_LAZY);

    printf("\n gLibHandel = %d \n", (int)gLibHandle);

    if(gLibHandle)
    {
        dlerror();    // Clear any existing error

        fp_InitLib = (int (*)(void)) dlsym(gLibHandle, "InitLib");
        fp_DeinitLib = (int (*)(void)) dlsym(gLibHandle, "DeinitLib");
        
	fp_StoreMessage_LID = (int (*)(wchar_t *)) dlsym(gLibHandle, "StoreMessage_LID");
        fp_GetSample_LID = (int (*)(LIB_LID_DATA *)) dlsym(gLibHandle, "GetSample_LID");
        
        fp_LockLID = (int (*)(void))dlsym(gLibHandle, "LockArea_LID");
        fp_UnlockLID = (int (*)(void))dlsym(gLibHandle, "UnlockArea_LID");
  
 	fp_GetTimestamp = (int (*)( void ))dlsym(gLibHandle, "GetTimestamp");

	fp_StoreMessageBin_LID = (int (*)(int timestamp, int nsamples, void *buffer)) dlsym(gLibHandle, "StoreMessageBin_LID");
       
	

	if (fp_InitLib && fp_DeinitLib && fp_StoreMessage_LID && fp_GetSample_LID && fp_LockLID && fp_UnlockLID && fp_StoreMessageBin_LID)
	{
		return LRES_SUCCESS;
	}
    }

    return LRES_FAILURE;
}


static void UnloadLibrary(void)
{
    if(gLibHandle)
    dlclose(gLibHandle);
}


static int LoadLibrary(void);
static void UnloadLibrary(void);

void Lidar_storeData(unsigned char* buffer, int nLidSamples)
{
    int timestamp = (*fp_GetTimestamp)();

    // storess all lidar data from shared memory
    if (LRES_SUCCESS == (*fp_LockLID)())
    {

	int result = (*fp_StoreMessageBin_LID)(timestamp, nLidSamples, (void *)buffer);

        if(LRES_SUCCESS == result)
        {
//		printf("\n storeData Success");
        	// ... SUCCESS: samples stored
        }
        else
        {
		printf("\n storeData Failure: %d", result);

        	// ... FAILURE
        }

        if(LRES_SUCCESS != (*fp_UnlockLID)())
        {
		printf("\n memory unlock Failure");

            // ... SHARED MEMORY UNLOCK FAILURE
        }
    }
    else
    {
	printf("\n memory lock Failure");

        // ... SHARED MEMORY LOCK FAILURE
    }
}

void print_usage(int argc, const char * argv[])
{
    printf("Simple LIDAR data grabber for SLAMTEC LIDAR.\n"
           "Version: %s \n"
           "Usage:\n"
           " For serial channel %s --channel --serial <com port> [baudrate]\n"
           "The baudrate is 115200(for A2) or 256000(for A3).\n"
		   " For udp channel %s --channel --udp <ipaddr> [port NO.]\n"
           "The LPX default ipaddr is 192.168.11.2,and the port NO.is 8089. Please refer to the datasheet for details.\n"
           , "SL_LIDAR_SDK_VERSION", argv[0], argv[0]);
}

bool checkSLAMTECLIDARHealth(ILidarDriver * drv)
{
    	sl_result op_result;
    	
	sl_lidar_response_device_health_t healthinfo;

	op_result = drv->getHealth(healthinfo);
    
	if(SL_IS_OK(op_result)) 
	{ 
		// the macro IS_OK is the preperred way to judge whether the operation is succeed.
        
//		printf("SLAMTEC Lidar health status : %d\n", healthinfo.status);
        
		if (healthinfo.status == SL_LIDAR_STATUS_ERROR) 
		{
//            		fprintf(stderr, "Error, slamtec lidar internal error detected. Please reboot the device to retry.\n");
            		
			// enable the following code if you want slamtec lidar to be reboot by software
            
			drv->reset();
            		
			return false;
        	} 
		else 
		{
            		return true;
        	}

    	} 
	else 
	{
//        	fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
        
		return false;
    	}
}

void ctrlc(int)
{
    ctrl_c_pressed = true;
}


// // // // // // // // // // // // // // // MAIN // // // // // // // // // // // // // // //


int main(int argc, const char* argv[]) 
{
	const char* opt_is_channel = NULL; 
	const char* opt_channel = NULL;
	const char* opt_channel_param_first = NULL;
	
	sl_u32 opt_channel_param_second = 0;
    	sl_u32 baudrateArray[2] = {115200, 256000};
    	sl_result op_result;
	
	int opt_channel_type = CHANNEL_TYPE_SERIALPORT;

	bool useArgcBaudrate = false;


	std::ofstream output_file;	


	output_file.open("/home/pi/Lidar/lidar_data.txt", std::ios_base::app);	

	std::vector <LidarScanMode> scanModes; //aggiunto


	if (LRES_SUCCESS == LoadLibrary())
   	 {
		 printf("\n LibSharedMemory OK Load");
    	   	
		 // call library initialization function
     	  	 if (LRES_SUCCESS == (*fp_InitLib)())
      	 	 {
			printf("\n LibSharedMemory Init OK Load");

       		 }
       	}
  	else
    	{
		printf("\n LibSharedMemory ERROR Load");
    	}


	if(wiringPiSetup() == -1)		// Setup the library
	{
		printf("\n wiringPI ERROR Load");
	}
	else
	{
		printf("\n wiringPI OK Load");
	}

	delay(500);

	pinMode(LED, OUTPUT);		
	
//	pullUpDnControl(LED, PUD_UP);	

	pinMode(SWITCH, INPUT);		

	for(int i = 0; i < 4; i++) // test led
	{	
		digitalWrite(LED, HIGH);

		delay(500);

		digitalWrite(LED, LOW);

		delay(500);
	}


	IChannel* _channel;

//   	printf("Ultra simple LIDAR data grabber for SLAMTEC LIDAR.\n" "Version: %s\n", "SL_LIDAR_SDK_VERSION");

	
	if (argc>1)
	{ 
		opt_is_channel = argv[1];
	}
	else
	{
		print_usage(argc, argv);
		return -1;
	}

	if(strcmp(opt_is_channel, "--channel")==0)
	{
		opt_channel = argv[2];
	
		if(strcmp(opt_channel, "-s")==0||strcmp(opt_channel, "--serial")==0)
		{
			// read serial port from the command line...
			opt_channel_param_first = argv[3];// or set to a fixed value: e.g. "com3"
			
			// read baud rate from the command line if specified...
			if (argc>4) opt_channel_param_second = strtoul(argv[4], NULL, 10);	
			
			useArgcBaudrate = true;
		}
		else if(strcmp(opt_channel, "-u")==0||strcmp(opt_channel, "--udp")==0)
		{
			// read ip addr from the command line...
			opt_channel_param_first = argv[3];//or set to a fixed value: e.g. "192.168.11.2"
			if (argc>4) opt_channel_param_second = strtoul(argv[4], NULL, 10);//e.g. "8089"
			opt_channel_type = CHANNEL_TYPE_UDP;
		}
		else
		{
			print_usage(argc, argv);
			return -1;
		}
	}
	else
	{
		print_usage(argc, argv);

        	return -1;
	}


	if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
	{
		if (!opt_channel_param_first) 
		{
#ifdef _WIN32
			// use default com port
			opt_channel_param_first = "\\\\.\\com3";
#elif __APPLE__
			opt_channel_param_first = "/dev/tty.SLAB_USBtoUART";
#else
			opt_channel_param_first = "/dev/ttyUSB0";
#endif
		}
	}


	on_started:  //aggiunto
    
    	// create the driver instance
	ILidarDriver* drv = *createLidarDriver();

    	if (!drv) 
	{
        	fprintf(stderr, "insufficent memory, exit\n");
        	
		exit(-2);
    	}

// aggiunto <--

    	sl_lidar_response_device_info_t devinfo;
   
	bool connectSuccess = false;

    	if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
	{
        	if(useArgcBaudrate)
		{
            		_channel = (*createSerialPortChannel(opt_channel_param_first, opt_channel_param_second));
            
			if (SL_IS_OK((drv)->connect(_channel))) 
			{
				op_result = drv->getDeviceInfo(devinfo);

                		if (SL_IS_OK(op_result)) 
                		{
	                		connectSuccess = true;
                		}
                		else
				{
                    			delete drv;
					drv = NULL;
                		}
            		}
        	}
        	else
		{
            		size_t baudRateArraySize = (sizeof(baudrateArray))/ (sizeof(baudrateArray[0]));
			
			for(size_t i = 0; i < baudRateArraySize; ++i)
			{
				_channel = (*createSerialPortChannel(opt_channel_param_first, baudrateArray[i]));
				                		
				if (SL_IS_OK((drv)->connect(_channel))) 
				{
                    			op_result = drv->getDeviceInfo(devinfo);

                    			if (SL_IS_OK(op_result)) 
                    			{
	                    			connectSuccess = true;
                        			break;
                    			}
                    			else
					{
                        			delete drv;
						drv = NULL;
                    			}
                		}
			}
        	}
	}
/*
  	else if(opt_channel_type == CHANNEL_TYPE_UDP)
	{
        	_channel = *createUdpChannel(opt_channel_param_first, opt_channel_param_second);
        
		if (SL_IS_OK((drv)->connect(_channel))) 
		{
            		op_result = drv->getDeviceInfo(devinfo);

            		if (SL_IS_OK(op_result)) 
            		{
	        	    connectSuccess = true;
			}
			else
			{
           		    	delete drv;
				drv = NULL;
			}
		}
	}
*/

/*
    	if (!connectSuccess) 
	{
        	(opt_channel_type == CHANNEL_TYPE_SERIALPORT)?
		(fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
				, opt_channel_param_first)):(fprintf(stderr, "Error, cannot connect to the specified ip addr %s.\n"
				, opt_channel_param_first));
		
		goto on_finished;
    	}
*/

/*
    	// print out the device serial number, firmware and hardware version number..
	printf("SLAMTEC LIDAR S/N: ");

    	for (int pos = 0; pos < 16 ;++pos) 
	{
        	printf("\n %02X", devinfo.serialnum[pos]);
    	}

    	printf("\n"
            "Firmware Ver: %d.%02d\n"
            "Hardware Rev: %d\n"
            , devinfo.firmware_version>>8
            , devinfo.firmware_version & 0xFF
            , (int)devinfo.hardware_version);

    	// check health...
    	if (!checkSLAMTECLIDARHealth(drv)) 
	{
        	goto on_finished;
    	}
*/

    	signal(SIGINT, ctrlc);

/*

// aggiunto -->

	MotorCtrlSupport motorSupport;

	drv->checkMotorCtrlSupport(motorSupport);

	printf("\n motor support = %d", motorSupport);

// <-- aggiunto


// aggiunto -->

/*
	sl_result res;

	LidarMotorInfo motorInfo;

	res = drv->getMotorInfo(motorInfo); //aggiunto

	printf("\n motor info --> %d %d %d", motorInfo.desired_speed, motorInfo.max_speed, motorInfo.min_speed);

*/

	drv->getAllSupportedScanModes(scanModes);

// <-- aggiunto

/*
//aggiunto -->

	for (int i = 0; i < scanModes.size(); i++) // 1412 sono tutti gli elementi del Vector di tipo <LidarScanMode>
	{
		printf("%d \n", scanModes[i].id);
		printf("%f \n", scanModes[i].us_per_sample);
		printf("%f \n", scanModes[i].max_distance);
	
		printf("%s \n", scanModes[i].scan_mode);
	}

	printf("\n \n" "--> scan mode printed <-- " "\n \n");

// <-- aggiunto
*/

//	if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
//	drv->setMotorSpeed(1000);
   
	// start scan...
//   	drv->startScan(0,1);

	
//aggiunto -->

	drv->startScanExpress(0, scanModes[0].id, 0, nullptr);


	drv->setMotorSpeed(1200);

// <-- aggiunto


// // // // // // // // // // // // // // // LOOP ENTER // // // // // // // // // // // // // // //

    	while (1) 
	{
        	sl_lidar_response_measurement_node_hq_t nodes[1024];  // original nodes[8192];
        
		size_t   count = _countof(nodes);

        	op_result = drv->grabScanDataHq(nodes, count);

        	if (SL_IS_OK(op_result) && digitalRead(SWITCH)) 
		{
			digitalWrite(LED, HIGH);

			drv->ascendScanData(nodes, count);
				
			Lidar_storeData((unsigned char*)nodes, (int)count);
					
			for (int pos = 0; pos < (int)count ; ++pos) 
			{	
				//printf("%d \n", pos); //aggiunto
		
                		/*
				printf("\n %s theta: %03.2f Dist: %08.2f Q: %d", // print data from lodar
                    		(nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ?"S ":"  ", 
                    		(nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                    		nodes[pos].dist_mm_q2/4.0f,
                    		nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
				*/
				/*			
				output_file << "Lidar - > theta: "<< ((nodes[pos].angle_z_q14 * 90.f) / 16384.f) << "     ";
				output_file << "Lidar - > Dist: " << (nodes[pos].dist_mm_q2 / 4.0f) << "     ";
				output_file << "Lidar - > Quality: " << (nodes[pos].quality>> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) << " \n ";
				*/
				{
					LIB_LID_DATA data;
				
					(*fp_GetSample_LID)(&data); // get data from shared mem
					
						
					printf("\n smp_ts_ms = %d", data.smp_ts_ms); // print data from shared mem
					printf(" smp_degree = %03.2f", (data.smp_degree * 90.f) / 16384.f);
					printf(" smp_dist = %08.2f", data.smp_dist / 4.0f);
					printf(" smp_power = %d", data.smp_power >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
					
	
					output_file << "SMem - > theta:" << (data.smp_degree * 90.f) / 16384.f << "     ";
					output_file << "SMem - > Dist:" << (data.smp_dist / 4.0f) << "     ";
					output_file << "SMem - > Quality:" << (data.smp_power >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) << " \n ";
				}
            		}
        	}
		else //aggiunto
		{
			digitalWrite(LED, LOW);
			
			drv->stop();
	
			if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
//			drv->setMotorSpeed(0);
        
			if(drv) 
			{
        			delete drv;
        			drv = NULL;
    			}

			goto on_started;
		}

        	if (ctrl_c_pressed)
		{ 
			digitalWrite(LED, LOW);

           		break;
        	}
    	}

// // // // // // // // // // // // // // // LOOP ESC // // // // // // // // // // // // // // //
    	drv->stop();
	
	output_file.close();

	if(opt_channel_type == CHANNEL_TYPE_SERIALPORT)
        
//	drv->setMotorSpeed(0);
    
	// done!
	on_finished:
    
	if(drv) 
	{
        	delete drv;
        	drv = NULL;
    	}
    	
	return 0;
}

