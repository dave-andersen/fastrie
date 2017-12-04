#include"global.h"
#include"ticker.h"
#include <algorithm>
#ifndef _WIN32
#include <errno.h>
#include <cstring>
#endif
#include <jansson.h>

#if JANSSON_MAJOR_VERSION >= 2
#define JSON_LOADS(str, err_ptr) json_loads((str), 0, (err_ptr))
#else
#define JSON_LOADS(str, err_ptr) json_loads((str), (err_ptr))
#endif

/*
 * Tries to establish a connection to the given ip:port
 * Uses a blocking connect operation
 */


#define STRATUM_STATE_INIT 0
#define STRATUM_STATE_SUBSCRIBE_SENT 1
#define STRATUM_STATE_SUBSCRIBE_RCVD 2
#define STRATUM_STATE_AUTHORIZE_SENT 3
#define STRATUM_STATE_AUTHORIZE_RCVD 4
#define STRATUM_STATE_INIT_DONE 5
#define STRATUM_STATE_SHARE_SENT 6

int stratumState = STRATUM_STATE_INIT;

void stratumClient_sendSubscribe(xptClient_t* sctx);


#ifdef _WIN32
SOCKET xptClient_openConnection(char *IP, int Port)
{
	SOCKET s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if( s == SOCKET_ERROR )
		return SOCKET_ERROR;
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(SOCKADDR_IN));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(Port);
	addr.sin_addr.s_addr=inet_addr(IP);
	int result = connect(s,(SOCKADDR*)&addr,sizeof(SOCKADDR_IN));
	if( result )
	{
		closesocket(s);
		return SOCKET_ERROR;
	}
#else
int xptClient_openConnection(char *IP, int Port)
{
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port=htons(Port);
  addr.sin_addr.s_addr = inet_addr(IP);
  int result = connect(s, (sockaddr*)&addr, sizeof(sockaddr_in));
  if (result) {
    perror("unablhe to connect");
    return -1;
  }
#endif

	return s;
}


#define bswap_32(x) ((((x) << 24) & 0xff000000u) | (((x) << 8) & 0x00ff0000u) \
                   | (((x) >> 8) & 0x0000ff00u) | (((x) >> 24) & 0x000000ffu))

static inline uint32_t swab32(uint32_t v)
{
	return bswap_32(v);
}

#if !HAVE_DECL_BE32DEC
static inline uint32_t be32dec(const void *pp)
{
	const uint8_t *p = (uint8_t const *)pp;
	return ((uint32_t)(p[3]) + ((uint32_t)(p[2]) << 8) +
	    ((uint32_t)(p[1]) << 16) + ((uint32_t)(p[0]) << 24));
}
#endif

#if !HAVE_DECL_LE32DEC
static inline uint32_t le32dec(const void *pp)
{
	const uint8_t *p = (uint8_t const *)pp;
	return ((uint32_t)(p[0]) + ((uint32_t)(p[1]) << 8) +
	    ((uint32_t)(p[2]) << 16) + ((uint32_t)(p[3]) << 24));
}
#endif

#if !HAVE_DECL_BE32ENC
static inline void be32enc(void *pp, uint32_t x)
{
	uint8_t *p = (uint8_t *)pp;
	p[3] = x & 0xff;
	p[2] = (x >> 8) & 0xff;
	p[1] = (x >> 16) & 0xff;
	p[0] = (x >> 24) & 0xff;
}
#endif

#if !HAVE_DECL_LE32ENC
static inline void le32enc(void *pp, uint32_t x)
{
	uint8_t *p = (uint8_t *)pp;
	p[0] = x & 0xff;
	p[1] = (x >> 8) & 0xff;
	p[2] = (x >> 16) & 0xff;
	p[3] = (x >> 24) & 0xff;
}
#endif


char *bin2hex(const unsigned char *p, size_t len)
{
	char *s = (char *)malloc((len * 2) + 1);
	if (!s)
	{
		printf("malloc failed in bin2hex\n");
		return NULL;
	}

	for (size_t i = 0; i < len; i++)
	{
		sprintf(s + (i * 2), "%02x", (unsigned int) p[i]);
	}

	return s;
}

bool hex2bin(unsigned char *p, const char *hexstr, size_t len)
{
	char hex_byte[3];
	char *ep;

	hex_byte[2] = '\0';

	while (*hexstr && len) {
		if (!hexstr[1]) {
			return false;
		}
		hex_byte[0] = hexstr[0];
		hex_byte[1] = hexstr[1];
		*p = (unsigned char) strtol(hex_byte, &ep, 16);
		if (*ep) {
			return false;
		}
		p++;
		hexstr += 2;
		len--;
	}

	return (len == 0 && *hexstr == 0) ? true : false;
}


/*
 * Creates a new xptClient connection object, does not initiate connection right away
 */
xptClient_t* xptClient_create()
{
	// create xpt connection object
	xptClient_t* xptClient = (xptClient_t*)malloc(sizeof(xptClient_t));
	memset(xptClient, 0x00, sizeof(xptClient_t));
	// initialize object
	xptClient->disconnected = true;
	xptClient->clientSocket = SOCKET_ERROR;
	xptClient->sendBuffer = xptPacketbuffer_create(256*1024);
	xptClient->recvBuffer = xptPacketbuffer_create(256*1024);
	InitializeCriticalSection(&xptClient->cs_shareSubmit);
	InitializeCriticalSection(&xptClient->cs_workAccess);
	xptClient->list_shareSubmitQueue = simpleList_create(4);
	xptClient->recvBuffer->buffer[0] = '\0';
	// return object
	return xptClient;
}

/*
 * Try to establish an active xpt connection
 * target is the server address and worker login data to use for connecting
 * Returns false on error or if already connected
 */
bool xptClient_connect(xptClient_t* xptClient, generalRequestTarget_t* target)
{
	// are we already connected?
	if( xptClient->disconnected == false )
		return false;
	// first try to connect to the given host/port
#ifdef _WIN32
	SOCKET clientSocket = xptClient_openConnection(target->ip, target->port);
	if( clientSocket == SOCKET_ERROR )
		return false;
#else
  int clientSocket = xptClient_openConnection(target->ip, target->port);
		if( clientSocket == 0 )
					return false;
#endif

#ifdef _WIN32
	// set socket as non-blocking
	unsigned int nonblocking=1;
	unsigned int cbRet;
	WSAIoctl(clientSocket, FIONBIO, &nonblocking, sizeof(nonblocking), NULL, 0, (LPDWORD)&cbRet, NULL, NULL);
#else
  int flags, err;
  flags = fcntl(clientSocket, F_GETFL, 0); 
  flags |= O_NONBLOCK;
  err = fcntl(clientSocket, F_SETFL, flags); //ignore errors for now..
#endif
	// initialize the connection details
	xptClient->clientSocket = clientSocket;

	strncpy(xptClient->username, target->authUser ,127);
	strncpy(xptClient->password, target->authPass, 127);
	// reset old work info
	memset(&xptClient->blockWorkInfo, 0x00, sizeof(xptBlockWorkInfo_t));
	// send worker login
	if( commandlineInput.protocol == PROTOCOL_XPT )
	{
		xptClient_sendWorkerLogin(xptClient);
	}
	else if( commandlineInput.protocol == PROTOCOL_STRATUM )
	{
		stratumClient_sendSubscribe(xptClient);
	}
	
	// mark as connected
	xptClient->disconnected = false;
	// return success
	return true;
}

/*
 * Forces xptClient into disconnected state
 */
void xptClient_forceDisconnect(xptClient_t* xptClient)
{
	if( xptClient->disconnected )
		return; // already disconnected
	if( xptClient->clientSocket != SOCKET_ERROR )
	{
		closesocket(xptClient->clientSocket);
		xptClient->clientSocket = SOCKET_ERROR;
	}
	xptClient->disconnected = true;
	// mark work as unavailable
	xptClient->hasWorkData = false;
}

/*
 * Disconnects and frees the xptClient instance
 */
void xptClient_free(xptClient_t* xptClient)
{
	xptPacketbuffer_free(xptClient->sendBuffer);
	xptPacketbuffer_free(xptClient->recvBuffer);
	if( xptClient->clientSocket != SOCKET_ERROR )
	{
		closesocket(xptClient->clientSocket);
	}
	simpleList_free(xptClient->list_shareSubmitQueue);
	free(xptClient);
}

const sint8 base58Decode[] =
{
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1, 0, 1, 2, 3, 4, 5, 6, 7, 8,-1,-1,-1,-1,-1,-1,
	-1, 9,10,11,12,13,14,15,16,-1,17,18,19,20,21,-1,
	22,23,24,25,26,27,28,29,30,31,32,-1,-1,-1,-1,-1,
	-1,33,34,35,36,37,38,39,40,41,42,43,-1,44,45,46,
	47,48,49,50,51,52,53,54,55,56,57,-1,-1,-1,-1,-1,
};

/*
 * Utility function to decode base58 wallet address
 * dataOut should have at least 1/2 the size of base58Input
 * inputLength must not exceed 200
 */
bool xptClient_decodeBase58(char* base58Input, sint32 inputLength, uint8* dataOut, sint32* dataOutLength)
{
	if( inputLength == 0 )
		return false;
	if( inputLength > 200 )
		return false;
	sint32 writeIndex = 0;
	uint32 baseArray[32];
	uint32 baseTrack[32];
	memset(baseArray, 0x00, sizeof(baseArray));
	memset(baseTrack, 0x00, sizeof(baseTrack));
	uint32 baseArraySize = 1;
	baseArray[0] = 0;
	baseTrack[0] = 57;
	// calculate exact size of output
	for(sint32 i=0; i<inputLength-1; i++)
	{
		// multiply baseTrack with 58
		for(sint32 b=baseArraySize-1; b>=0; b--)
		{
			uint64 multiplyWithCarry = (uint64)baseTrack[b] * 58ULL;
			baseTrack[b] = (uint32)(multiplyWithCarry&0xFFFFFFFFUL);
			multiplyWithCarry >>= 32;
			if( multiplyWithCarry != 0 )
			{
				// add carry
				for(sint32 carryIndex=b+1; carryIndex<baseArraySize; carryIndex++)
				{
					multiplyWithCarry += (uint64)baseTrack[carryIndex];
					baseTrack[carryIndex] = (uint32)(multiplyWithCarry&0xFFFFFFFFUL);
					multiplyWithCarry >>= 32;
					if( multiplyWithCarry == 0 )
						break;
				}
				if( multiplyWithCarry )
				{
					// extend
					baseTrack[baseArraySize] = (uint32)multiplyWithCarry;
					baseArraySize++;
				}
			}
		}
	}
	// get length of output data
	sint32 outputLength = 0;
	uint64 last = baseTrack[baseArraySize-1];
	if( last&0xFF000000 )
		outputLength = baseArraySize*4;
	else if( last&0xFF0000 )
		outputLength = baseArraySize*4-1;
	else if( last&0xFF00 )
		outputLength = baseArraySize*4-2;
	else
		outputLength = baseArraySize*4-3;
	// convert base
	for(sint32 i=0; i<inputLength; i++)
	{
		if( base58Input[i] >= sizeof(base58Decode)/sizeof(base58Decode[0]) )
			return false;
		sint8 digit = base58Decode[base58Input[i]];
		if( digit == -1 )
			return false;
		// multiply baseArray with 58
		for(sint32 b=baseArraySize-1; b>=0; b--)
		{
			uint64 multiplyWithCarry = (uint64)baseArray[b] * 58ULL;
			baseArray[b] = (uint32)(multiplyWithCarry&0xFFFFFFFFUL);
			multiplyWithCarry >>= 32;
			if( multiplyWithCarry != 0 )
			{
				// add carry
				for(sint32 carryIndex=b+1; carryIndex<baseArraySize; carryIndex++)
				{
					multiplyWithCarry += (uint64)baseArray[carryIndex];
					baseArray[carryIndex] = (uint32)(multiplyWithCarry&0xFFFFFFFFUL);
					multiplyWithCarry >>= 32;
					if( multiplyWithCarry == 0 )
						break;
				}
				if( multiplyWithCarry )
				{
					// extend
					baseArray[baseArraySize] = (uint32)multiplyWithCarry;
					baseArraySize++;
				}
			}
		}
		// add base58 digit to baseArray with carry
		uint64 addWithCarry = (uint64)digit;
		for(sint32 b=0; addWithCarry != 0 && b<baseArraySize; b++)
		{
			addWithCarry += (uint64)baseArray[b];
			baseArray[b] = (uint32)(addWithCarry&0xFFFFFFFFUL);
			addWithCarry >>= 32;
		}
		if( addWithCarry )
		{
			// extend
			baseArray[baseArraySize] = (uint32)addWithCarry;
			baseArraySize++;
		}
	}
	*dataOutLength = outputLength;
	// write bytes to about
	for(sint32 i=0; i<outputLength; i++)
	{
		dataOut[outputLength-i-1] = (uint8)(baseArray[i>>2]>>8*(i&3));
	}
	return true;
}

/*
 * Converts a wallet address (any coin) to the coin-independent format that xpt requires and
 * adds it to the list of developer fees.
 *
 * integerFee is a fixed size integer representation of the fee percentage. Where 65535 equals 131.07% (1 = 0.002%)
 * Newer versions of xpt try to stay integer-only to support devices that have no FPU.
 * 
 * You may want to consider re-implementing this mechanism in a different way if you plan to
 * have at least some basic level of protection from reverse engineers that try to remove your fee (if closed source)
 */
void xptClient_addDeveloperFeeEntry(xptClient_t* xptClient, char* walletAddress, uint16 integerFee, bool isMaxCoinAddress)
{
	uint8 walletAddressRaw[256];
	sint32 walletAddressRawLength = sizeof(walletAddressRaw);
	if( xptClient_decodeBase58(walletAddress, strlen(walletAddress), walletAddressRaw, &walletAddressRawLength) == false )
	{
		printf("xptClient_addDeveloperFeeEntry(): Failed to decode wallet address\n");
		return;
	}
	// is length valid?
	if( walletAddressRawLength != 25 )
	{
		printf("xptClient_addDeveloperFeeEntry(): Invalid length of decoded address\n");
		return;
	}
	// validate checksum
	uint8 addressHash[32];
	{
		sha256_ctx s256c;
		sha256_init(&s256c);
		sha256_update(&s256c, walletAddressRaw, walletAddressRawLength-4);
		sha256_final(&s256c, addressHash);
		sha256_init(&s256c);
		sha256_update(&s256c, addressHash, 32);
		sha256_final(&s256c, addressHash);
	}
	if( *(uint32*)(walletAddressRaw+21) != *(uint32*)addressHash )
	{
		printf("xptClient_addDeveloperFeeEntry(): Invalid checksum\n");
		return;
	}
	// address ok, check if there is still free space in the list
	if( xptClient->developerFeeCount >= XPT_DEVELOPER_FEE_MAX_ENTRIES )
	{
		printf("xptClient_addDeveloperFeeEntry(): Maximum number of developer fee entries exceeded\n");
		return;
	}
	// add entry
	memcpy(xptClient->developerFeeEntry[xptClient->developerFeeCount].pubKeyHash, walletAddressRaw+1, 20);
	xptClient->developerFeeEntry[xptClient->developerFeeCount].devFee = integerFee;
	xptClient->developerFeeCount++;
}


/*
 * Bitcoin's .setCompact() method without Bignum dependency
 * Does not support negative values
 */
void xptClient_getDifficultyTargetFromCompact(uint32 nCompact, uint32* hashTarget)
{
    unsigned int nSize = nCompact >> 24;
    bool fNegative     = (nCompact & 0x00800000) != 0;
    unsigned int nWord = nCompact & 0x007fffff;
    memset(hashTarget, 0x00, 32); // 32 byte -> 8 uint32
    if (nSize <= 3)
    {
        nWord >>= 8*(3-nSize);
        hashTarget[0] = nWord;
    }
    else
    {
        hashTarget[0] = nWord;
        for(uint32 f=0; f<(nSize-3); f++)
        {
            // shift by one byte
            hashTarget[7] = (hashTarget[7]<<8)|(hashTarget[6]>>24);
            hashTarget[6] = (hashTarget[6]<<8)|(hashTarget[5]>>24);
            hashTarget[5] = (hashTarget[5]<<8)|(hashTarget[4]>>24);
            hashTarget[4] = (hashTarget[4]<<8)|(hashTarget[3]>>24);
            hashTarget[3] = (hashTarget[3]<<8)|(hashTarget[2]>>24);
            hashTarget[2] = (hashTarget[2]<<8)|(hashTarget[1]>>24);
            hashTarget[1] = (hashTarget[1]<<8)|(hashTarget[0]>>24);
            hashTarget[0] = (hashTarget[0]<<8);
        }
    }
    if( fNegative )
    {
        // if negative bit set, set zero hash
        for(uint32 i=0; i<8; i++)
                hashTarget[i] = 0;
    }
}

/*
 * Sends the worker login packet
 */
void xptClient_sendWorkerLogin(xptClient_t* xptClient)
{
//	uint32 usernameLength = std::min(127, (uint32)strlen(xptClient->username));
//	uint32 passwordLength = std::min(127, (uint32)strlen(xptClient->password));
	// build the packet
	bool sendError = false;
	xptPacketbuffer_beginWritePacket(xptClient->sendBuffer, XPT_OPC_C_AUTH_REQ);
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, 6);								// version
	xptPacketbuffer_writeString(xptClient->sendBuffer, xptClient->username, 128, &sendError);	// username
	xptPacketbuffer_writeString(xptClient->sendBuffer, xptClient->password, 128, &sendError);	// password
	//xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, 1);							// payloadNum (removed in version 6)
	// write worker version to server
	xptPacketbuffer_writeString(xptClient->sendBuffer, minerVersionString, 45, &sendError);		// minerVersionString
	// developer fee (xpt version 6 and above)
	xptPacketbuffer_writeU8(xptClient->sendBuffer, &sendError, xptClient->developerFeeCount);
	for(sint32 i=0; i<xptClient->developerFeeCount; i++)
	{
		xptPacketbuffer_writeU16(xptClient->sendBuffer, &sendError, xptClient->developerFeeEntry[i].devFee);
		xptPacketbuffer_writeData(xptClient->sendBuffer, xptClient->developerFeeEntry[i].pubKeyHash, 20, &sendError);
	}
	// finalize
	xptPacketbuffer_finalizeWritePacket(xptClient->sendBuffer);
	// send to client
	send(xptClient->clientSocket, (const char*)(xptClient->sendBuffer->buffer), xptClient->sendBuffer->parserIndex, 0);
}


#define USER_AGENT "xptminer"

static const char *get_stratum_session_id(json_t *val)
{
	json_t *arr_val;
	int i, n;

	arr_val = json_array_get(val, 0);
	if (!arr_val || !json_is_array(arr_val))
		return NULL;
	n = json_array_size(arr_val);
	for (i = 0; i < n; i++) {
		const char *notify;
		json_t *arr = json_array_get(arr_val, i);

		if (!arr || !json_is_array(arr))
			break;
		notify = json_string_value(json_array_get(arr, 0));
		if (!notify)
			continue;
		if (!strcasecmp(notify, "mining.notify"))
			return json_string_value(json_array_get(arr, 1));
	}
	return NULL;
}


void stratumClient_sendSubscribe(xptClient_t* sctx)
{
	char *s = malloc(128 + (sctx->blockWorkInfo.session_id ? strlen(sctx->blockWorkInfo.session_id) : 0));
	if (sctx->blockWorkInfo.session_id)
		sprintf(s, "{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": [\"" USER_AGENT "\", \"%s\"]}\n", sctx->blockWorkInfo.session_id);
	else
		sprintf(s, "{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": [\"" USER_AGENT "\"]}\n");

	send(sctx->clientSocket, (const char*)(s), strlen(s), 0);
	free(s);
	
	stratumState = STRATUM_STATE_SUBSCRIBE_SENT;

}

void stratumSendAuthorize(xptClient_t* sctx)
{
	char *s = (char *)malloc(80 + strlen(sctx->username) + strlen(sctx->password));
	sprintf(s, "{\"id\": 2, \"method\": \"mining.authorize\", \"params\": [\"%s\", \"%s\"]}\n",
	        sctx->username, sctx->password);

	send(sctx->clientSocket, (const char*)(s), strlen(s), 0);
	free(s);
	
	stratumState = STRATUM_STATE_AUTHORIZE_SENT;
}

/*
 * Sends the share packet
 */
void stratumClient_sendShare(xptClient_t* xptClient, xptShareToSubmit_t* xptShareToSubmit)
{
	char s[345];
	uint64_t ntime = 0;
	uint32_t nonce[8];
	char *ntimestr, *noncestr, *xnonce2str;
	int i;
	be32enc( &ntime, xptShareToSubmit->nTime );
	ntime <<= 32;

	for (i = 0; i < 8; i++)
		nonce[i] = swab32( *(((uint32_t *)xptShareToSubmit->riecoin_nOffset) + 8 - 1 - i) );

	ntimestr = bin2hex((const unsigned char *)&ntime, 8);
	noncestr = bin2hex((const unsigned char *)nonce, 32);
	xnonce2str = bin2hex(xptShareToSubmit->userExtraNonceData, xptShareToSubmit->userExtraNonceLength);

	sprintf(s,
		"{\"method\": \"mining.submit\", \"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"], \"id\":4}\n",
		xptClient->username, xptShareToSubmit->job_id, xnonce2str, ntimestr, noncestr);
	free(ntimestr);
	free(noncestr);
	free(xnonce2str);
	printf("tx: %s\n", s);

	stratumState = STRATUM_STATE_SHARE_SENT;
	send(xptClient->clientSocket, (const char*)(s), strlen(s), 0);
}


void xptClient_sendShare(xptClient_t* xptClient, xptShareToSubmit_t* xptShareToSubmit)
{
	if( commandlineInput.protocol == PROTOCOL_STRATUM )
	{
		stratumClient_sendShare(xptClient, xptShareToSubmit);
		return;
	}

	// build the packet
	bool sendError = false;
	xptPacketbuffer_beginWritePacket(xptClient->sendBuffer, XPT_OPC_C_SUBMIT_SHARE);
	xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->merkleRoot, 32, &sendError);		// merkleRoot
	xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->prevBlockHash, 32, &sendError);	// prevBlock
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->version);				// version
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->nTime);				// nTime
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->nonce);				// nNonce
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, xptShareToSubmit->nBits);				// nBits
	// algorithm specific
	if( xptShareToSubmit->algorithm == ALGORITHM_RIECOIN )
	{
		// nOffset
		xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->riecoin_nOffset, 32, &sendError);
		// original merkleroot (used to identify work)
		xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->merkleRootOriginal, 32, &sendError);
		// user extra nonce (up to 16 bytes)
		xptPacketbuffer_writeU8(xptClient->sendBuffer, &sendError, xptShareToSubmit->userExtraNonceLength);
		xptPacketbuffer_writeData(xptClient->sendBuffer, xptShareToSubmit->userExtraNonceData, xptShareToSubmit->userExtraNonceLength, &sendError);
	} else {
	  fprintf(stderr, "Eek:  Unknown algorithm %d!\n", xptShareToSubmit->algorithm);
	  return;
	}
	// share id (server sends this back in shareAck, so we can identify share response)
	xptPacketbuffer_writeU32(xptClient->sendBuffer, &sendError, 0);
	// finalize
	xptPacketbuffer_finalizeWritePacket(xptClient->sendBuffer);
	// send to client
	send(xptClient->clientSocket, (const char*)(xptClient->sendBuffer->buffer), xptClient->sendBuffer->parserIndex, 0);
}

/*
 * Sends a ping request, the server will respond with the same data as fast as possible
 * To measure latency we send a high precision timestamp
 */
void xptClient_sendPing(xptClient_t* xptClient)
{
	// Windows only for now
 

	uint64 timestamp = getTimeHighRes();
	// build the packet
	bool sendError = false;
	xptPacketbuffer_beginWritePacket(xptClient->sendBuffer, XPT_OPC_C_PING);
	// timestamp
	xptPacketbuffer_writeU64(xptClient->sendBuffer, &sendError, timestamp);
	// finalize
	xptPacketbuffer_finalizeWritePacket(xptClient->sendBuffer);
	// send to client
	send(xptClient->clientSocket, (const char*)(xptClient->sendBuffer->buffer), xptClient->sendBuffer->parserIndex, 0);
}

/*
 * Processes a fully received packet
 */
bool xptClient_processPacket(xptClient_t* xptClient)
{
	// printf("Received packet with opcode %d and size %d\n", xptClient->opcode, xptClient->recvSize+4);
	if( xptClient->opcode == XPT_OPC_S_AUTH_ACK )
		return xptClient_processPacket_authResponse(xptClient);
	else if( xptClient->opcode == XPT_OPC_S_WORKDATA1 )
		return xptClient_processPacket_blockData1(xptClient);
	else if( xptClient->opcode == XPT_OPC_S_SHARE_ACK )
		return xptClient_processPacket_shareAck(xptClient);
	else if( xptClient->opcode == XPT_OPC_S_MESSAGE )
		return xptClient_processPacket_message(xptClient);
	else if( xptClient->opcode == XPT_OPC_S_PING )
		return xptClient_processPacket_ping(xptClient);
	// unknown opcodes are accepted too, for later backward compatibility
	return true;
}

/*
 * Checks for new received packets and connection events (e.g. closed connection)
 */
#define RBUFSIZE 2048
#define RECVSIZE (RBUFSIZE - 4)

static void stratum_buffer_append(xptClient_t *sctx, const char *s)
{
	size_t old, newSize;

	old = strlen(sctx->recvBuffer->buffer);
	newSize = old + strlen(s) + 1;
	if (newSize >= sctx->recvIndex) {
		sctx->recvIndex = newSize + (RBUFSIZE - (newSize % RBUFSIZE));
		sctx->recvBuffer->buffer = realloc(sctx->recvBuffer->buffer, sctx->recvIndex);
	}
	strcpy(sctx->recvBuffer->buffer + old, s);
}

void stratumSubscribeResponse( xptClient_t* xptClient, char *sret )
{
//sret = "{\"error\": null, \"id\": 1, \"result\": [[\"mining.notify\", \"ae6812eb4cd7735a302a8a9dd95cf71f\"], \"f800000d\", 4]}";
	xptClient->algorithm = ALGORITHM_RIECOIN;

	char *s = (char *)malloc(128);
	json_t *val = NULL, *res_val, *err_val;
	json_error_t err;
	const char *sid, *xnonce1;
	int xn2_size;

	
	val = JSON_LOADS(sret, &err);
	free(sret);
	if (!val) {
//		applog(LOG_ERR, "JSON decode failed(%d): %s", err.line, err.text);
		goto out;
	}

	res_val = json_object_get(val, "result");
	err_val = json_object_get(val, "error");

	if (!res_val || json_is_null(res_val) ||
	    (err_val && !json_is_null(err_val))) {
		{
			free(s);
			if (err_val)
				s = json_dumps(err_val, JSON_INDENT(3));
			else
				s = strdup("(unknown reason)");
			printf ("%s\n",s);
		}
		goto out;
	}

	sid = get_stratum_session_id(res_val);
	xnonce1 = json_string_value(json_array_get(res_val, 1));
	xn2_size = json_integer_value(json_array_get(res_val, 2));

	free(xptClient->blockWorkInfo.session_id);

	xptClient->blockWorkInfo.session_id = sid ? strdup(sid) : NULL;
	xptClient->blockWorkInfo.extraNonce1Len = strlen(xnonce1) / 2;
	hex2bin(xptClient->blockWorkInfo.extraNonce1, xnonce1, xptClient->blockWorkInfo.extraNonce1Len);
	xptClient->blockWorkInfo.extraNonce2Len = xn2_size;
	
	stratumState = STRATUM_STATE_SUBSCRIBE_RCVD;
	
	printf("session id %s\n", xptClient->blockWorkInfo.session_id);
	printf("extraNonce1 %d %x\n", xptClient->blockWorkInfo.extraNonce1Len, *(unsigned int *)xptClient->blockWorkInfo.extraNonce1);
	printf("extraNonce2Len %d\n", xptClient->blockWorkInfo.extraNonce2Len);

	stratumSendAuthorize( xptClient );
	
out:
	free(s);

	return;
}



void stratumSubmitShareResponse( xptClient_t* xptClient, char *sret )
{
	char *s = (char *)malloc(128);
	json_t *val = NULL, *res_val, *err_val;
	json_error_t err;
	
	val = JSON_LOADS(sret, &err);
	free(sret);
	if (!val) {
//		applog(LOG_ERR, "JSON decode failed(%d): %s", err.line, err.text);
		goto out;
	}

	res_val = json_object_get(val, "result");
	err_val = json_object_get(val, "error");

	if (!res_val || json_is_null(res_val) ||
	    (err_val && !json_is_null(err_val))) {
		{
			free(s);
			if (err_val)
				s = json_dumps(err_val, JSON_INDENT(3));
			else
				s = strdup("(unknown reason)");
			printf("Invalid share, reason: %s\n", s);
			totalRejectedShareCount++;
		}
		goto out;
	}
out:
	free(s);
}

void stratumAuthorizeResponse( xptClient_t* xptClient, char *sret )
{
	stratumState = STRATUM_STATE_INIT_DONE;
	return;
}


uint32 getCompact( uint32 nCompact)
{
	uint32 p;
        unsigned int nSize = nCompact >> 24;
        //bool fNegative     =(nCompact & 0x00800000) != 0;
        unsigned int nWord = nCompact & 0x007fffff;
        if (nSize <= 3)
        {
            nWord >>= 8*(3-nSize);
            p = nWord;
        }
        else
        {
            p = nWord;
            p <<= 8*(nSize-3); // warning: this has problems if difficulty (uncompacted) ever goes past the 2^32 boundary
        }
        return p;
}


bool stratum_notify(xptClient_t *sctx, json_t *params)
{
	const char *job_id, *prevhash, *coinb1, *coinb2, *version, *nbits, *ntime;
	size_t coinb1_size, coinb2_size;
	bool clean;
	int merkle_count, i;
	json_t *merkle_arr;
	bool ret = false;

	job_id = json_string_value(json_array_get(params, 0));
	prevhash = json_string_value(json_array_get(params, 1));
	coinb1 = json_string_value(json_array_get(params, 2));
	coinb2 = json_string_value(json_array_get(params, 3));
	merkle_arr = json_array_get(params, 4);
	if (!merkle_arr || !json_is_array(merkle_arr))
		goto out;
	merkle_count = json_array_size(merkle_arr);
	version = json_string_value(json_array_get(params, 5));
	nbits = json_string_value(json_array_get(params, 6));
	ntime = json_string_value(json_array_get(params, 7));
	clean = json_is_true(json_array_get(params, 8));

	if (!job_id || !prevhash || !coinb1 || !coinb2 || !version || !nbits || !ntime ||
	    strlen(prevhash) != 64 || strlen(version) != 8 ||
	    strlen(nbits) != 8 || strlen(ntime) != 8) {
		printf("Stratum notify: invalid parameters");
		goto out;
	}

	for (i = 0; i < merkle_count; i++) {
		const char *s = json_string_value(json_array_get(merkle_arr, i));
		if (!s || strlen(s) != 64) {
			printf("Stratum notify: invalid Merkle branch");
			goto out;
		}
		hex2bin(sctx->blockWorkInfo.txHashes + i*32, s, 32);
	}

	coinb1_size = strlen(coinb1) / 2;
	coinb2_size = strlen(coinb2) / 2;
	hex2bin(sctx->blockWorkInfo.coinBase1, coinb1, coinb1_size);

	if (strcmp(sctx->blockWorkInfo.job_id, job_id))
		memset(sctx->blockWorkInfo.extraNonce2, 0, sctx->blockWorkInfo.extraNonce2Len);
	hex2bin(sctx->blockWorkInfo.coinBase2, coinb2, coinb2_size);

	sctx->blockWorkInfo.coinBase1Size = coinb1_size;
	sctx->blockWorkInfo.coinBase2Size = coinb2_size;

	strncpy( sctx->blockWorkInfo.job_id, job_id, STRATUM_JOB_ID_MAX_LEN);
	hex2bin(sctx->blockWorkInfo.prevBlockHash, prevhash, 32);
	for (i = 0; i < 8; i++)
		*(((uint32 *)sctx->blockWorkInfo.prevBlockHash) + i) = be32dec(((uint32 *)sctx->blockWorkInfo.prevBlockHash) + i);

	sctx->blockWorkInfo.txHashCount = merkle_count;

	hex2bin((unsigned char *)&sctx->blockWorkInfo.version, version, 4);
	sctx->blockWorkInfo.version = swab32(sctx->blockWorkInfo.version);
	hex2bin((unsigned char *)&sctx->blockWorkInfo.nBits, nbits, 4);
	sctx->blockWorkInfo.nBits = swab32(sctx->blockWorkInfo.nBits);
	sctx->blockWorkInfo.targetCompact = getCompact( sctx->blockWorkInfo.nBits );
	

	unsigned char ntimeAux[4];
	hex2bin(ntimeAux, ntime, 4);
	sctx->blockWorkInfo.nTime = be32dec(ntimeAux);

	sctx->clientState = XPT_CLIENT_STATE_LOGGED_IN;
	sctx->algorithm = ALGORITHM_RIECOIN;

	sctx->blockWorkInfo.timeWork = time(NULL);
	sctx->blockWorkInfo.timeBias = sctx->blockWorkInfo.nTime - (uint32)time(NULL);
	if( clean || sctx->blockWorkInfo.height == 0 )
		sctx->blockWorkInfo.height++;
	sctx->hasWorkData = true;
	ret = true;

out:
	return ret;
}


bool stratum_handle_method(xptClient_t *sctx, const char *s)
{
	json_t *val, *id, *params;
	json_error_t err;
	const char *method;
	bool ret = false;

	val = JSON_LOADS(s, &err);
	if (!val) {
		//applog(LOG_ERR, "JSON decode failed(%d): %s", err.line, err.text);
		goto out;
	}

	method = json_string_value(json_object_get(val, "method"));
	if (!method)
	{
		if( stratumState == STRATUM_STATE_SHARE_SENT )
		{
			stratumSubmitShareResponse( sctx, s );
			stratumState = STRATUM_STATE_INIT_DONE;
		}
		goto out;
	}
	id = json_object_get(val, "id");
	params = json_object_get(val, "params");

	printf("received: %s\n", method );

	if (!strcasecmp(method, "mining.notify")) {
		ret = stratum_notify(sctx, params);
		goto out;
	}
	if (!strcasecmp(method, "mining.set_difficulty")) {
//		ret = stratum_set_difficulty(sctx, params);
		goto out;
	}
	if (!strcasecmp(method, "client.reconnect")) {
//		ret = stratum_reconnect(sctx, params);
		goto out;
	}
	if (!strcasecmp(method, "client.get_version")) {
//		ret = stratum_get_version(sctx, id);
		goto out;
	}
	if (!strcasecmp(method, "client.show_message")) {
//		ret = stratum_show_message(sctx, id, params);
		goto out;
	}

out:
	if (val)
		json_decref(val);

	return ret;
}

void stratumProcess( xptClient_t* xptClient, char *sret )
{
	// printf("incoming: %s\n", sret);
	if( stratumState == STRATUM_STATE_SUBSCRIBE_SENT )
	{
		stratumSubscribeResponse( xptClient, sret );
	} else
	if( stratumState == STRATUM_STATE_AUTHORIZE_SENT )
	{
		stratumAuthorizeResponse( xptClient, sret );
	}
	else
	{
		stratum_handle_method( xptClient, sret );
	}
}

bool xptClient_process(xptClient_t* xptClient)
{
	if( xptClient == NULL )
		return false;
	// are there shares to submit?
	EnterCriticalSection(&xptClient->cs_shareSubmit);
	if( xptClient->list_shareSubmitQueue->objectCount > 0 )
	{
		for(uint32 i=0; i<xptClient->list_shareSubmitQueue->objectCount; i++)
		{
			xptShareToSubmit_t* xptShareToSubmit = (xptShareToSubmit_t*)xptClient->list_shareSubmitQueue->objects[i];
			xptClient_sendShare(xptClient, xptShareToSubmit);
			free(xptShareToSubmit);
		}
		// clear list
		xptClient->list_shareSubmitQueue->objectCount = 0;
	}
	LeaveCriticalSection(&xptClient->cs_shareSubmit);

	if( commandlineInput.protocol == PROTOCOL_STRATUM )
	{
		ssize_t len, buflen;
		char *tok, *sret = NULL;

		char s[RBUFSIZE];
		ssize_t n;

		memset(s, 0, RBUFSIZE);
		n = recv(xptClient->clientSocket, s, RECVSIZE, 0);
		if( n <= 0 )
		{
		#ifdef _WIN32
			// receive error, is it a real error or just because of non blocking sockets?
			if( WSAGetLastError() != WSAEWOULDBLOCK || n == 0)
			{
				xptClient->disconnected = true;
				return false;
			}
		#else
    			if(errno != EAGAIN || n == 0)
    			{
				xptClient->disconnected = true;
				return false;
    			}
		#endif
			return true;
		}
		stratum_buffer_append(xptClient, s);

		if (strstr((char *)xptClient->recvBuffer->buffer, "\n"))
		{
			buflen = strlen(xptClient->recvBuffer->buffer);
			tok = strtok(xptClient->recvBuffer->buffer, "\n");
			if (!tok) {
				printf("stratum_recv_line failed to parse a newline-terminated string\n");
				return false;
			}
			sret = strdup(tok);
			len = strlen(sret);

			if (buflen > len + 1)
				memmove(xptClient->recvBuffer->buffer, xptClient->recvBuffer->buffer + len + 1, buflen - len + 1);
			else
				xptClient->recvBuffer->buffer[0] = '\0';
				
			stratumProcess(xptClient, sret);
			xptClient->recvBuffer->buffer[0] = '\0';
		}

		return true;
	}

	// check if we need to send ping
	uint32 currentTime = (uint32)time(NULL);
	if( xptClient->time_sendPing != 0 && currentTime >= xptClient->time_sendPing )
	{
		xptClient_sendPing(xptClient);
		xptClient->time_sendPing = currentTime + 120; // ping every 4 minutes
	}
	// check for packets
	sint32 packetFullSize = 4; // the packet always has at least the size of the header
	if( xptClient->recvSize > 0 )
		packetFullSize += xptClient->recvSize;
	sint32 bytesToReceive = (sint32)(packetFullSize - xptClient->recvIndex);
	// packet buffer is always large enough at this point
	sint32 r = recv(xptClient->clientSocket, (char*)(xptClient->recvBuffer->buffer+xptClient->recvIndex), bytesToReceive, 0);
	if( r <= 0 )
	{
#ifdef _WIN32
		// receive error, is it a real error or just because of non blocking sockets?
		if( WSAGetLastError() != WSAEWOULDBLOCK || r == 0)
		{
			xptClient->disconnected = true;
			return false;
		}
#else
    if(errno != EAGAIN || r == 0)
    {
		xptClient->disconnected = true;
		return false;
    }
#endif
		return true;
	}
	xptClient->recvIndex += r;
	
	// header just received?
	if( xptClient->recvIndex == packetFullSize && packetFullSize == 4 )
	{
		// process header
		uint32 headerVal = *(uint32*)xptClient->recvBuffer->buffer;
		uint32 opcode = (headerVal&0xFF);
		uint32 packetDataSize = (headerVal>>8)&0xFFFFFF;
		// validate header size
		if( packetDataSize >= (1024*1024*2-4) )
		{
			// packets larger than 4mb are not allowed
			printf("xptServer_receiveData(): Packet exceeds 2mb size limit\n");
			return false;
		}
		xptClient->recvSize = packetDataSize;
		xptClient->opcode = opcode;
		// enlarge packetBuffer if too small
		if( (xptClient->recvSize+4) > xptClient->recvBuffer->bufferLimit )
		{
			xptPacketbuffer_changeSizeLimit(xptClient->recvBuffer, (xptClient->recvSize+4));
		}
	}
	// have we received the full packet?
	if( xptClient->recvIndex >= (xptClient->recvSize+4) )
	{
		// process packet
		xptClient->recvBuffer->bufferSize = (xptClient->recvSize+4);
		if( xptClient_processPacket(xptClient) == false )
		{
			xptClient->recvIndex = 0;
			xptClient->recvSize = 0;
			xptClient->opcode = 0;
			// disconnect
			if( xptClient->clientSocket != 0 )
			{
				closesocket(xptClient->clientSocket);
				xptClient->clientSocket = 0;
			}
			xptClient->disconnected = true;
			return false;
		}
		xptClient->recvIndex = 0;
		xptClient->recvSize = 0;
		xptClient->opcode = 0;
	}
	// return
	return true;
}

/*
 * Returns true if the xptClient connection was disconnected from the server or should disconnect because login was invalid or awkward data received
 * Parameter reason is currently unused.
 */
bool xptClient_isDisconnected(xptClient_t* xptClient, char** reason)
{
	return xptClient->disconnected;
}

/*
 * Returns true if the worker login was successful
 */
bool xptClient_isAuthenticated(xptClient_t* xptClient)
{
	return (xptClient->clientState == XPT_CLIENT_STATE_LOGGED_IN);
}

void xptClient_foundShare(xptClient_t* xptClient, xptShareToSubmit_t* xptShareToSubmit)
{
	EnterCriticalSection(&xptClient->cs_shareSubmit);
	simpleList_add(xptClient->list_shareSubmitQueue, xptShareToSubmit);
	LeaveCriticalSection(&xptClient->cs_shareSubmit);
}
