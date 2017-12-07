#ifndef __XPTCLIENT_H__
#define __XPTCLIENT_H__

#define XPT_DEVELOPER_FEE_MAX_ENTRIES	(8)

typedef struct  
{
	uint8* buffer;
	uint32 parserIndex;
	uint32 bufferLimit; // current maximal size of buffer
	uint32 bufferSize; // current effective size of buffer
}xptPacketbuffer_t;

typedef struct  
{
	uint8 merkleRoot[32];
	uint32 seed;
}xptWorkData_t;

typedef struct  
{
	uint32 height;
	uint32 version;
	uint32 nTime;
	uint32 nBits;
	uint32 nBitsShare;
	uint8 merkleRoot[32];
	uint8 prevBlockHash[32];
	// timeBias (difference between local time and server time)
	uint32 timeBias;
	// protoshares
	uint32 nBirthdayA;
	uint32 nBirthdayB;
	// target (from compact format)
	uint8 target[32];
	uint8 targetShare[32];
	// target (compact format, only needed for Riecoin)
	uint32 targetCompact;
	uint32 targetShareCompact;
	// coinbase & tx info
	uint8	extraNonce1[1024];
	uint8	extraNonce2[1024];
	uint16	extraNonce1Len;
	uint16	extraNonce2Len;
	uint8 *session_id;
	uint16 coinBase1Size;
	uint8 coinBase1[1024];
	uint16 coinBase2Size;
	uint8 coinBase2[1024];
	uint32 txHashCount;
	uint8 txHashes[32*4096UL]; // space for 4096 tx hashes
	// time (set to current value of time(NULL) when we receive the work)
	uint32 timeWork;
	uint8 job_id[STRATUM_JOB_ID_MAX_LEN+1];
}xptBlockWorkInfo_t;

// client states
#define XPT_CLIENT_STATE_NEW		(0)
#define XPT_CLIENT_STATE_LOGGED_IN	(1)


// list of known opcodes

#define XPT_OPC_C_AUTH_REQ		1
#define XPT_OPC_S_AUTH_ACK		2
#define XPT_OPC_S_WORKDATA1		3
#define XPT_OPC_C_SUBMIT_SHARE	4
#define XPT_OPC_S_SHARE_ACK		5
//#define XPT_OPC_C_SUBMIT_POW	6
#define XPT_OPC_S_MESSAGE		7
#define XPT_OPC_C_PING			8
#define XPT_OPC_S_PING			8

// algorithms
#define ALGORITHM_RIECOIN		7

typedef struct  
{
	uint8 algorithm;
	uint8 merkleRoot[32];
	uint8 prevBlockHash[32];
	uint32 version;
	uint32 nonce;
	uint32 nTime;
	uint32 nBits;
	// primecoin specific
	uint32 sieveSize;
	uint32 sieveCandidate; // index of sieveCandidate for this share
	uint8 fixedMultiplierSize;
	uint8 fixedMultiplier[201];
	uint8 chainMultiplierSize;
	uint8 chainMultiplier[201];
	// protoshare specific
	uint32 nBirthdayA;
	uint32 nBirthdayB;
	// riecoin specific
	uint8  riecoin_nOffset[32];
	// gbt stuff
	uint8 merkleRootOriginal[32];
	uint32 userExtraNonceLength;
	uint8 userExtraNonceData[16];
	uint8 job_id[STRATUM_JOB_ID_MAX_LEN+1];
}xptShareToSubmit_t;

typedef struct  
{
	uint16 devFee;
	uint8  pubKeyHash[20]; // RIPEMD160 hash of public key (retrieved from wallet address without prefix byte and without checksum)
}xptDevFeeEntry_t;

typedef struct  
{
	SOCKET clientSocket;
	xptPacketbuffer_t* sendBuffer; // buffer for sending data
	xptPacketbuffer_t* recvBuffer; // buffer for receiving data
	// worker info
	char username[128];
	char password[128];
	uint32 clientState;
	uint8 algorithm; // see ALGORITHM_* constants
	// recv info
	uint32 recvSize;
	uint32 recvIndex;
	uint32 opcode;
	// disconnect info
	bool disconnected;
	// work data
	CRITICAL_SECTION cs_workAccess;
	xptBlockWorkInfo_t blockWorkInfo;
	bool hasWorkData;
	float earnedShareValue; // this value is sent by the server with each new block that is sent
	// shares to submit
	CRITICAL_SECTION cs_shareSubmit;
	simpleList_t* list_shareSubmitQueue;
	// timers
	uint32 time_sendPing;
	uint64 pingSum;
	uint32 pingCount;
	// developer fee
	xptDevFeeEntry_t developerFeeEntry[XPT_DEVELOPER_FEE_MAX_ENTRIES];
	sint32 developerFeeCount; // number of developer fee entries
}xptClient_t;

// packetbuffer
xptPacketbuffer_t* xptPacketbuffer_create(uint32 initialSize);
void xptPacketbuffer_free(xptPacketbuffer_t* pb);
void xptPacketbuffer_changeSizeLimit(xptPacketbuffer_t* pb, uint32 sizeLimit);


void xptPacketbuffer_beginReadPacket(xptPacketbuffer_t* pb);
uint32 xptPacketbuffer_getReadSize(xptPacketbuffer_t* pb);
float xptPacketbuffer_readFloat(xptPacketbuffer_t* pb, bool* error);
uint64 xptPacketbuffer_readU64(xptPacketbuffer_t* pb, bool* error);
uint32 xptPacketbuffer_readU32(xptPacketbuffer_t* pb, bool* error);
uint16 xptPacketbuffer_readU16(xptPacketbuffer_t* pb, bool* error);
uint8 xptPacketbuffer_readU8(xptPacketbuffer_t* pb, bool* error);
void xptPacketbuffer_readData(xptPacketbuffer_t* pb, uint8* data, uint32 length, bool* error);

void xptPacketbuffer_beginWritePacket(xptPacketbuffer_t* pb, uint8 opcode);
void xptPacketbuffer_writeFloat(xptPacketbuffer_t* pb, bool* error, float v);
void xptPacketbuffer_writeU64(xptPacketbuffer_t* pb, bool* error, uint64 v);
void xptPacketbuffer_writeU32(xptPacketbuffer_t* pb, bool* error, uint32 v);
void xptPacketbuffer_writeU16(xptPacketbuffer_t* pb, bool* error, uint16 v);
void xptPacketbuffer_writeU8(xptPacketbuffer_t* pb, bool* error, uint8 v);
void xptPacketbuffer_writeData(xptPacketbuffer_t* pb, uint8* data, uint32 length, bool* error);
void xptPacketbuffer_finalizeWritePacket(xptPacketbuffer_t* pb);

void xptPacketbuffer_writeString(xptPacketbuffer_t* pb, char* stringData, uint32 maxStringLength, bool* error);
void xptPacketbuffer_readString(xptPacketbuffer_t* pb, char* stringData, uint32 maxStringLength, bool* error);

// connection setup
xptClient_t* xptClient_create();
bool xptClient_connect(xptClient_t* xptClient, generalRequestTarget_t* target);
void xptClient_addDeveloperFeeEntry(xptClient_t* xptClient, const char* walletAddress, uint16 integerFee);
void xptClient_forceDisconnect(xptClient_t* xptClient);

// connection processing
bool xptClient_process(xptClient_t* xptClient); // needs to be called in a loop
bool xptClient_isDisconnected(xptClient_t* xptClient);
void xptClient_foundShare(xptClient_t* xptClient, xptShareToSubmit_t* xptShareToSubmit);

// never send this directly
void xptClient_sendWorkerLogin(xptClient_t* xptClient);

// packet handlers
bool xptClient_processPacket_authResponse(xptClient_t* xptClient);
bool xptClient_processPacket_blockData1(xptClient_t* xptClient);
bool xptClient_processPacket_shareAck(xptClient_t* xptClient);
bool xptClient_processPacket_message(xptClient_t* xptClient);
bool xptClient_processPacket_ping(xptClient_t* xptClient);

// util
void xptClient_getDifficultyTargetFromCompact(uint32 nCompact, uint32* hashTarget);

// miner version string (needs to be defined somewhere in the project, max 45 characters)
extern const char* minerVersionString;

#endif // __XPTCLIENT_H__
