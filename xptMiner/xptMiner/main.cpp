#include "global.h"
#include "ticker.h"
#include <signal.h>
#include <stdio.h>
#include <cstring>
#include <sys/time.h>
#define MAX_TRANSACTIONS	(4096)

// miner version string (for pool statistic)
const char* minerVersionString = "xptMiner 1.7dga-c17";

minerSettings_t minerSettings;

xptClient_t* xptClient = NULL;
CRITICAL_SECTION cs_xptClient;
volatile uint32 monitorCurrentBlockHeight; // used to notify worker threads of new block data
volatile uint32 monitorCurrentBlockTime; // keeps track of current block time, used to detect if current work data is outdated

// stats
volatile uint32 totalShareCount = 0;
volatile uint32 totalRejectedShareCount = 0;
volatile uint32 total2ChainCount = 0;
volatile uint32 total3ChainCount = 0;
volatile uint32 total4ChainCount = 0;


commandlineInput_t commandlineInput;


struct  
{
	CRITICAL_SECTION cs_work;
	uint32	algorithm;
	// block data
	uint32	version;
	uint32	height;
	uint32	nBits;
	uint32	timeBias;
	uint8	merkleRootOriginal[32]; // used to identify work in xpt
	uint8	job_id[STRATUM_JOB_ID_MAX_LEN+1]; // used to identify work in stratum
	uint8	prevBlockHash[32];
	uint8	target[32];
	uint8	targetShare[32];
	uint32  targetCompact;
	uint32  shareTargetCompact;
	// extra nonce info
	uint8	extraNonce1[1024];
	uint8	coinBase1[1024];
	uint8	coinBase2[1024];
	uint16	extraNonce1Len;
	uint16	coinBase1Size;
	uint16	coinBase2Size;
	// transaction hashes
	uint8	txHash[32*MAX_TRANSACTIONS];
	uint32	txHashCount;
}workDataSource;

uint32_t uniqueMerkleSeedGenerator;
uint32 miningStartTime = 0;


/*
 * Submit Riecoin share
 */
void xptMiner_submitShare(minerRiecoinBlock_t* block, uint8* nOffset)
{
	uint32 passedSeconds = (uint32)time(NULL) - miningStartTime;
	printf("[%02d:%02d:%02d] Share found! (Blockheight: %d)\n", (passedSeconds/3600)%60, (passedSeconds/60)%60, (passedSeconds)%60, block->height);
	EnterCriticalSection(&cs_xptClient);
	if( xptClient == NULL || xptClient_isDisconnected(xptClient, NULL) == true )
	{
		printf("Share submission failed - No connection to server\n");
		LeaveCriticalSection(&cs_xptClient);
		return;
	}
	// submit block
	xptShareToSubmit_t* xptShare = (xptShareToSubmit_t*)malloc(sizeof(xptShareToSubmit_t));
	memset(xptShare, 0x00, sizeof(xptShareToSubmit_t));
	xptShare->algorithm = ALGORITHM_RIECOIN;
	xptShare->version = block->version;
	xptShare->nTime = block->nTime;
	xptShare->nonce = 0;
	xptShare->nBits = block->nBits;
	memcpy(xptShare->prevBlockHash, block->prevBlockHash, 32);
	memcpy(xptShare->merkleRoot, block->merkleRoot, 32);
	memcpy(xptShare->merkleRootOriginal, block->merkleRootOriginal, 32);
	sint32 userExtraNonceLength = sizeof(uint32);
	uint8* userExtraNonceData = (uint8*)&block->uniqueMerkleSeed;
	xptShare->userExtraNonceLength = userExtraNonceLength;
	memcpy(xptShare->userExtraNonceData, userExtraNonceData, userExtraNonceLength);
	memcpy(xptShare->riecoin_nOffset, nOffset, 32);
	memcpy(xptShare->job_id, block->job_id, STRATUM_JOB_ID_MAX_LEN);
	xptClient_foundShare(xptClient, xptShare);
	LeaveCriticalSection(&cs_xptClient);
}

#ifdef _WIN32
int xptMiner_minerThread(int threadIndex)
{
#else
void *xptMiner_minerThread(void *arg)
{
#endif

	// local work data

	minerRiecoinBlock_t minerRiecoinBlock; 

	while( true )
	{
		// has work?
		bool hasValidWork = false;
		EnterCriticalSection(&workDataSource.cs_work);
		if( workDataSource.height > 0 )
		{
			memset(&minerRiecoinBlock, 0x00, sizeof(minerRiecoinBlock));
			minerRiecoinBlock.version = workDataSource.version;
			minerRiecoinBlock.nTime = (uint64)time(NULL) + (uint64)(sint64)(sint32)workDataSource.timeBias; // Riecoin uses 64bit timestamp
			minerRiecoinBlock.nBits = workDataSource.nBits;
			minerRiecoinBlock.targetCompact = workDataSource.targetCompact;
			minerRiecoinBlock.shareTargetCompact = workDataSource.shareTargetCompact;

			minerRiecoinBlock.height = workDataSource.height;
			memcpy(minerRiecoinBlock.merkleRootOriginal, workDataSource.merkleRootOriginal, 32);
			memcpy(minerRiecoinBlock.prevBlockHash, workDataSource.prevBlockHash, 32);
			minerRiecoinBlock.uniqueMerkleSeed = ++uniqueMerkleSeedGenerator;
			memcpy(minerRiecoinBlock.job_id, workDataSource.job_id, sizeof(minerRiecoinBlock.job_id) );
			// generate merkle root transaction
			if( commandlineInput.protocol == PROTOCOL_STRATUM )
				bitclient_generateTxHash(workDataSource.extraNonce1Len, workDataSource.extraNonce1, sizeof(uint32), (uint8*)&minerRiecoinBlock.uniqueMerkleSeed, workDataSource.coinBase1Size, workDataSource.coinBase1, workDataSource.coinBase2Size, workDataSource.coinBase2, workDataSource.txHash, TX_MODE_DOUBLE_SHA256);
			else
				bitclient_generateTxHash(0, NULL, sizeof(uint32), (uint8*)&minerRiecoinBlock.uniqueMerkleSeed, workDataSource.coinBase1Size, workDataSource.coinBase1, workDataSource.coinBase2Size, workDataSource.coinBase2, workDataSource.txHash, TX_MODE_DOUBLE_SHA256);
			bitclient_calculateMerkleRoot(workDataSource.txHash, workDataSource.txHashCount+1, minerRiecoinBlock.merkleRoot, TX_MODE_DOUBLE_SHA256);
			hasValidWork = true;
		}
		LeaveCriticalSection(&workDataSource.cs_work);
		if( hasValidWork == false )
		{
			Sleep(1);
			continue;
		}
		// valid work data present, start processing workload

		{
#define DEBUG_TIMING 0
#if DEBUG_TIMING
		  struct timeval tv_start, tv_end;
		  gettimeofday(&tv_start, NULL);
#endif
			riecoin_process(&minerRiecoinBlock);
#if DEBUG_TIMING
		  gettimeofday(&tv_end, NULL);
		  double d = (double)tv_end.tv_sec;
		  d += ((double)tv_end.tv_usec)/1000000.0;
		  d -= tv_start.tv_sec;
		  d -= ((double)tv_start.tv_usec)/1000000.0;
		  printf("riecoin loop %.2f seconds\n", d);
#endif
		}
	}
	return 0;
}

uint8 algorithmInited[32] = {0};

/*
 * Reads data from the xpt connection state and writes it to the universal workDataSource struct
 */
void xptMiner_getWorkFromXPTConnection(xptClient_t* xptClient)
{
	EnterCriticalSection(&workDataSource.cs_work);
	if( xptClient->algorithm >= 0 && xptClient->algorithm < 32 && xptClient->blockWorkInfo.height > 0 )
	{
		if( xptClient->algorithm == ALGORITHM_RIECOIN && algorithmInited[xptClient->algorithm] == 0 )
		{
		  riecoin_init(commandlineInput.sieveMax, commandlineInput.numThreads);
			algorithmInited[xptClient->algorithm] = 1;
		}
	}
	workDataSource.algorithm = xptClient->algorithm;
	workDataSource.version = xptClient->blockWorkInfo.version;
	workDataSource.timeBias = xptClient->blockWorkInfo.timeBias;
	workDataSource.nBits = xptClient->blockWorkInfo.nBits;
	memcpy(workDataSource.merkleRootOriginal, xptClient->blockWorkInfo.merkleRoot, 32);
	memcpy(workDataSource.prevBlockHash, xptClient->blockWorkInfo.prevBlockHash, 32);
	memcpy(workDataSource.target, xptClient->blockWorkInfo.target, 32);
	memcpy(workDataSource.targetShare, xptClient->blockWorkInfo.targetShare, 32);
	workDataSource.targetCompact = xptClient->blockWorkInfo.targetCompact;
	workDataSource.shareTargetCompact = xptClient->blockWorkInfo.targetShareCompact;
	workDataSource.extraNonce1Len = xptClient->blockWorkInfo.extraNonce1Len;
	workDataSource.coinBase1Size = xptClient->blockWorkInfo.coinBase1Size;
	workDataSource.coinBase2Size = xptClient->blockWorkInfo.coinBase2Size;
	if( commandlineInput.protocol == PROTOCOL_STRATUM )
		memcpy(workDataSource.extraNonce1, xptClient->blockWorkInfo.extraNonce1, xptClient->blockWorkInfo.extraNonce1Len);
	memcpy(workDataSource.coinBase1, xptClient->blockWorkInfo.coinBase1, xptClient->blockWorkInfo.coinBase1Size);
	memcpy(workDataSource.coinBase2, xptClient->blockWorkInfo.coinBase2, xptClient->blockWorkInfo.coinBase2Size);

	// get hashes
	if( xptClient->blockWorkInfo.txHashCount > MAX_TRANSACTIONS )
	{
		printf("Too many transaction hashes\n"); 
		workDataSource.txHashCount = 0;
	}
	else
		workDataSource.txHashCount = xptClient->blockWorkInfo.txHashCount;
	for(uint32 i=0; i<xptClient->blockWorkInfo.txHashCount; i++)
		memcpy(workDataSource.txHash+32*(i+1), xptClient->blockWorkInfo.txHashes+32*i, 32);
	// set blockheight last since it triggers reload of work
	if( workDataSource.height == 0 && xptClient->blockWorkInfo.height != 0 )
	{
		miningStartTime = (uint32)time(NULL);
		printf("[00:00:00] Start mining\n");
	}
	workDataSource.height = xptClient->blockWorkInfo.height;
	memcpy( workDataSource.job_id, xptClient->blockWorkInfo.job_id, sizeof(workDataSource.job_id) );
	LeaveCriticalSection(&workDataSource.cs_work);
	monitorCurrentBlockHeight = workDataSource.height;
        __sync_synchronize(); /* memory barrier needed if this isn't done in crit */
}

#define getFeeFromDouble(_x) ((uint16)((double)(_x)/0.002)) // integer 1 = 0.002%
/*
 * Initiates a new xpt connection object and sets up developer fee
 * The new object will be in disconnected state until xptClient_connect() is called
 */
xptClient_t* xptMiner_initateNewXptConnectionObject()
{
	xptClient_t* xptClient = xptClient_create();
	if( xptClient == NULL )
		return NULL;

	return xptClient;
}

void xptMiner_xptQueryWorkLoop()
{
	// init xpt connection object once
	xptClient = xptMiner_initateNewXptConnectionObject();
	uint32 timerPrintDetails = getTimeMilliseconds() + 8000;

	if(minerSettings.requestTarget.donationPercent > 0.1f)
	{
	  float donAmount = minerSettings.requestTarget.donationPercent;
	  xptClient_addDeveloperFeeEntry(xptClient, "RUhMA8bvsr48aC3WVj3aGf5p1zytPSz59o", getFeeFromDouble(donAmount), false);  // dga
	}
	while( true )
	{
		uint32 currentTick = getTimeMilliseconds();
		if( currentTick >= timerPrintDetails )
		{
			// print details only when connected
			if( xptClient_isDisconnected(xptClient, NULL) == false )
			{
				uint32 passedSeconds = (uint32)time(NULL) - miningStartTime;

				if( workDataSource.algorithm == ALGORITHM_RIECOIN )
				{
					// speed is represented as knumber/s (in steps of 0x1000)
					float speedRate_2ch = 0.0f;
					float speedRate_3ch = 0.0f;
					float speedRate_4ch = 0.0f;

					if( passedSeconds > 5 )
					{
						speedRate_2ch = (double)total2ChainCount * 4096.0 / (double)passedSeconds / 1000.0;
						speedRate_3ch = (double)total3ChainCount * 4096.0 / (double)passedSeconds / 1000.0;
						speedRate_4ch = (double)total4ChainCount * 4096.0 / (double)passedSeconds / 1000.0;
					}
					printf("[%02d:%02d:%02d] 2ch/s: %.4lf 3ch/s: %.4lf 4ch/s: %.4lf Shares total: %d / %d\n", (passedSeconds/3600)%60, (passedSeconds/60)%60, (passedSeconds)%60, speedRate_2ch, speedRate_3ch, speedRate_4ch, totalShareCount, totalShareCount-totalRejectedShareCount);
					fflush(stdout);
				}

			}
			timerPrintDetails = currentTick + 8000;
		}
		// check stats
		if( xptClient_isDisconnected(xptClient, NULL) == false )
		{
			EnterCriticalSection(&cs_xptClient);
			xptClient_process(xptClient);
			if( xptClient->disconnected )
			{
				// mark work as invalid
				EnterCriticalSection(&workDataSource.cs_work);
				workDataSource.height = 0;
				monitorCurrentBlockHeight = 0;
				LeaveCriticalSection(&workDataSource.cs_work);
				// we lost connection :(
				printf("Connection to server lost - Reconnect in 45 seconds\n");
				xptClient_forceDisconnect(xptClient);
				LeaveCriticalSection(&cs_xptClient);
				// pause 45 seconds
				Sleep(45000);
			}
			else
			{
				// is known algorithm?
				if( xptClient->clientState == XPT_CLIENT_STATE_LOGGED_IN && (xptClient->algorithm != ALGORITHM_RIECOIN) )
				{
					printf("The login is configured for an unsupported algorithm.\n");
					printf("Make sure you miner login details are correct\n");
					// force disconnect
					//xptClient_free(xptClient);
					//xptClient = NULL;
					xptClient_forceDisconnect(xptClient);
					LeaveCriticalSection(&cs_xptClient);
					// pause 45 seconds
					Sleep(45000);
				}
				else if( xptClient->blockWorkInfo.height != workDataSource.height || memcmp(xptClient->blockWorkInfo.merkleRoot, workDataSource.merkleRootOriginal, 32) != 0  )
				{
					// update work
					xptMiner_getWorkFromXPTConnection(xptClient);
					LeaveCriticalSection(&cs_xptClient);
				}
				else
					LeaveCriticalSection(&cs_xptClient);
				// update time monitor
				if( workDataSource.height > 0 )
					monitorCurrentBlockTime = (uint32)time(NULL) + workDataSource.timeBias;
				Sleep(1);
			}
		}
		else
		{
			// initiate new connection
			EnterCriticalSection(&cs_xptClient);
			xptClient = xptMiner_initateNewXptConnectionObject();

	if(minerSettings.requestTarget.donationPercent > 0.1f)
	{
	  float donAmount = minerSettings.requestTarget.donationPercent;
	  if (donAmount > 1.5) { 
	    donAmount -= 0.25f;
	    xptClient_addDeveloperFeeEntry(xptClient, "RDrQYV7VHbnzUDX8BmcjoradKGVQaBcXXi", getFeeFromDouble(0.10f), false);  // jh00
	    xptClient_addDeveloperFeeEntry(xptClient, "RNh5PSLpPmkNxB3PgoLnKzpM75rmkzfz5y", getFeeFromDouble(0.15f), false);  // clintar, windows port
	  }
	  xptClient_addDeveloperFeeEntry(xptClient, "RUhMA8bvsr48aC3WVj3aGf5p1zytPSz59o", getFeeFromDouble(donAmount), false);  // dga
	}
			if( xptClient_connect(xptClient, &minerSettings.requestTarget) == false )
			{
				LeaveCriticalSection(&cs_xptClient);
				printf("Connection attempt failed, retry in 45 seconds\n");
				Sleep(45000);
			}
			else
			{
				LeaveCriticalSection(&cs_xptClient);
				if( commandlineInput.protocol == PROTOCOL_STRATUM )
				{
					printf("Connected to server using stratum protocol. DEVELOPER FEES NOT SUPPORTED\n");
				}
				else
				{
					printf("Connected to server using x.pushthrough(xpt) protocol\n");
				}
				total2ChainCount = 0;
				total3ChainCount = 0;
				total4ChainCount = 0;
			}
			Sleep(1);
		}
	}
}


void xptMiner_printHelp()
{
#ifdef _WIN32
	puts("Usage: xptMiner.exe [options]");
#else
	puts("Usage: ./xptMiner [options]");
#endif
	puts("General options:");
	puts("   -o, -O                        The miner will connect to this url");
	puts("                                 You can specify a port after the url using -o url:port");
	puts("   -u                            The username (workername) used for login");
	puts("   -p                            The password used for login");
	puts("   -t <num>                      The number of threads for mining (default is set to number of cores)");
	puts("                                 For most efficient mining, set to number of virtual cores if you have memory");
	puts("   -s <num>                      Prime sieve max (default: 900000000)");
	puts("   -m                            Use stratum protocol instead of xpt");
	puts("Example usage:");
#ifdef _WIN32
	puts("   xptMiner.exe -o http://poolurl.com:10034 -u workername.ric_1 -p workerpass -t 4");
#else
	puts("   ./xptMiner -o http://poolurl.com:10034 -u workername.ric_1 -p workerpass -t 4");
#endif
}

void xptMiner_parseCommandline(int argc, char **argv)
{
	sint32 cIdx = 1;
	commandlineInput.donationPercent = 2.0f;
	commandlineInput.sieveMax = 900000000ULL;
	commandlineInput.protocol = PROTOCOL_XPT;

	while( cIdx < argc )
	{
		char* argument = argv[cIdx];
		cIdx++;
		if (memcmp(argument, "-s", 3)==0 || memcmp(argument, "-s", 3)==0 ) {
		  if (cIdx > argc) {
		    printf("Missing sieve size after -s\n");
		    exit(0);
		  }
		  commandlineInput.sieveMax = strtoull(argv[cIdx], NULL, 10);
		  if (commandlineInput.sieveMax < 100000 || commandlineInput.sieveMax > 0xffffffffULL) {
		    printf("Bad sieve size:  Must be between 100000 and 4.1 billion\n");
		    exit(0);
		  }
		  cIdx++;
		}
		else if( memcmp(argument, "-o", 3)==0 || memcmp(argument, "-O", 3)==0 )
		{
			// -o
			if( cIdx >= argc )
			{
				printf("Missing URL after -o option\n");
				exit(0);
			}
			if( strstr(argv[cIdx], "http://") )
				commandlineInput.host = _strdup(strstr(argv[cIdx], "http://")+7);
			else
				commandlineInput.host = _strdup(argv[cIdx]);
			char* portStr = strstr(commandlineInput.host, ":");
			if( portStr )
			{
				*portStr = '\0';
				commandlineInput.port = atoi(portStr+1);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-u", 3)==0 )
		{
			// -u
			if( cIdx >= argc )
			{
				printf("Missing username/workername after -u option\n");
				exit(0);
			}
			commandlineInput.workername = _strdup(argv[cIdx]);
			cIdx++;
		}
		else if( memcmp(argument, "-p", 3)==0 )
		{
			// -p
			if( cIdx >= argc )
			{
				printf("Missing password after -p option\n");
				exit(0);
			}
			commandlineInput.workerpass = _strdup(argv[cIdx]);
			cIdx++;
		}
		else if( memcmp(argument, "-t", 3)==0 )
		{
			// -t
			if( cIdx >= argc )
			{
				printf("Missing thread number after -t option\n");
				exit(0);
			}
			commandlineInput.numThreads = atoi(argv[cIdx]);
			if( commandlineInput.numThreads < 1 || commandlineInput.numThreads > 128 )
			{
				printf("-t parameter out of range");
				exit(0);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-gpu", 5)==0 )
		{
			commandlineInput.useGPU = true;
		}
		else if( memcmp(argument, "-m", 3)==0 )
		{
			commandlineInput.protocol = PROTOCOL_STRATUM;
		}
		else if( memcmp(argument, "-d", 3)==0 )
		{
			if( cIdx >= argc )
			{
				printf("Missing amount number after -d option\n");
				exit(0);
			}
			commandlineInput.donationPercent = atof(argv[cIdx]);
			if( commandlineInput.donationPercent < 0.0f || commandlineInput.donationPercent > 100.0f )
			{
				printf("-d parameter out of range");
				exit(0);
			}
			cIdx++;
		}
		else if( memcmp(argument, "-help", 6)==0 || memcmp(argument, "--help", 7)==0 )
		{
			xptMiner_printHelp();
			exit(0);
		}
		else
		{
			printf("'%s' is an unknown option.\nUse the --help parameter for more info\n", argument); 
			exit(-1);
		}
	}
	if( argc <= 1 )
	{
		xptMiner_printHelp();
		exit(0);
	}
}


int main(int argc, char** argv)
{

	commandlineInput.host = "ypool.net";
	srand(getTimeMilliseconds());
	commandlineInput.port = 8080 + (rand()%8); // use random port between 8080 and 8087
	commandlineInput.useGPU = false;
  uint32_t numcpu = 1; // in case we fall through;	
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
  int mib[4];
  size_t len = sizeof(numcpu); 

  /* set the mib for hw.ncpu */
  mib[0] = CTL_HW;
#ifdef HW_AVAILCPU
  mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;
#else
  mib[1] = HW_NCPU;
#endif
  /* get the number of CPUs from the system */
sysctl(mib, 2, &numcpu, &len, NULL, 0);
 signal(SIGPIPE, SIG_IGN);


    if( numcpu < 1 )
    {
      numcpu = 1;
    }

#elif defined(__linux__) || defined(sun) || defined(__APPLE__)
  numcpu = static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
#elif defined(_SYSTYPE_SVR4)
  numcpu = sysconf( _SC_NPROC_ONLN );
#elif defined(hpux)
  numcpu = mpctl(MPC_GETNUMSPUS, NULL, NULL);
#elif defined(_WIN32)
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
  numcpu = sysinfo.dwNumberOfProcessors;
#endif

	commandlineInput.numThreads = numcpu;
	commandlineInput.numThreads = std::min(std::max(commandlineInput.numThreads, 1), 4);
	xptMiner_parseCommandline(argc, argv);
	minerSettings.useGPU = commandlineInput.useGPU;
	printf("----------------------------\n");
	printf("  xptMiner/ric/dga (%s)\n", minerVersionString);
	printf("  author: jh00 (xptminer originally for http://ypool.net) dga (ric core)\n");
	printf("----------------------------\n");
	if( commandlineInput.protocol == PROTOCOL_XPT )
	{
		printf("Launching miner...  using XPT\n");
	}
	else if( commandlineInput.protocol == PROTOCOL_STRATUM )
	{
		printf("Launching miner...  using STRATUM\n");
	}
	else
	{
		printf("error: invalid protocol\n");
		exit(0);
	}
	
	printf("Using %d +1 CPU threads\n", commandlineInput.numThreads);
	commandlineInput.numThreads += 1;

	if( commandlineInput.useGPU ) {
		printf("Using GPU if possible\n");
	}
	
	printf("\nFee Percentage:  %.2f%%. To set, use \"-d\" flag e.g. \"-d 2.5\" is 2.5%% donation\n\n", commandlineInput.donationPercent);
#ifdef _WIN32
	// set priority to below normal
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	// init winsock
	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2),&wsa);
#endif
	// get IP of pool url (default ypool.net)
	char* poolURL = commandlineInput.host;//"ypool.net";
	hostent* hostInfo = gethostbyname(poolURL);
	if( hostInfo == NULL )
	{
		printf("Cannot resolve '%s'. Is it a valid URL?\n", poolURL);
		exit(-1);
	}
	void** ipListPtr = (void**)hostInfo->h_addr_list;
	uint32 ip = 0xFFFFFFFF;
	if( ipListPtr[0] )
	{
		ip = *(uint32*)ipListPtr[0];
	}
	char* ipText = (char*)malloc(32);
	sprintf(ipText, "%d.%d.%d.%d", ((ip>>0)&0xFF), ((ip>>8)&0xFF), ((ip>>16)&0xFF), ((ip>>24)&0xFF));
	// init work source
	InitializeCriticalSection(&workDataSource.cs_work);
	InitializeCriticalSection(&cs_xptClient);
	// setup connection info
	minerSettings.requestTarget.ip = ipText;
	minerSettings.requestTarget.port = commandlineInput.port;
	minerSettings.requestTarget.authUser = commandlineInput.workername;
	minerSettings.requestTarget.authPass = commandlineInput.workerpass;
	minerSettings.requestTarget.donationPercent = commandlineInput.donationPercent;
	// start miner threads
#ifndef _WIN32
	
	pthread_t threads[commandlineInput.numThreads];
	pthread_attr_t threadAttr;
	pthread_attr_init(&threadAttr);
	// Set the stack size of the thread
	pthread_attr_setstacksize(&threadAttr, 120*1024);
	// free resources of thread upon return
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
#endif
	printf("Launching the %d threads\n", commandlineInput.numThreads);
	for(uint32 i=0; i<commandlineInput.numThreads; i++)
#ifdef _WIN32
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)xptMiner_minerThread, (LPVOID)0, 0, NULL);
#else
		pthread_create(&threads[i], &threadAttr, xptMiner_minerThread, (void *)i);
#endif
	// enter work management loop
	xptMiner_xptQueryWorkLoop();
	return 0;
}
