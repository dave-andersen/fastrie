#include"global.h"
#include"ticker.h"

/*
 * Called when a packet with the opcode XPT_OPC_S_AUTH_ACK is received
 */
bool xptClient_processPacket_authResponse(xptClient_t* xptClient)
{
	xptPacketbuffer_t* cpb = xptClient->recvBuffer;
	// read data from the packet
	xptPacketbuffer_beginReadPacket(cpb);
	// start parsing
	bool readError = false;
	// read error code field
	uint32 authErrorCode = xptPacketbuffer_readU32(cpb, &readError);
	if( readError )
		return false;
	// read reject reason / motd
	char rejectReason[512];
	xptPacketbuffer_readString(cpb, rejectReason, 512, &readError);
	rejectReason[511] = '\0';
	if( readError )
		return false;
	if( authErrorCode == 0 )
	{
		xptClient->clientState = XPT_CLIENT_STATE_LOGGED_IN;
		printf("xpt: Logged in with %s\n", xptClient->username);
		if( rejectReason[0] != '\0' )
			printf("Message from server: %s\n", rejectReason);
		// start ping mechanism
		xptClient->time_sendPing = (uint32)time(NULL) + 60; // first ping after one minute
	}
	else
	{
		// error logging in -> disconnect
		printf("xpt: Failed to log in with %s\n", xptClient->username);
		if( rejectReason[0] != '\0' )
			printf("Reason: %s\n", rejectReason);
		return false;
	}
	// get algorithm used by this worker
	xptClient->algorithm = xptPacketbuffer_readU8(cpb, &readError);
	return true;
}

/*
 * Called when a packet with the opcode XPT_OPC_S_WORKDATA1 is received
 * This is the first version of xpt 'getwork'
 */
bool xptClient_processPacket_blockData1(xptClient_t* xptClient)
{
	// parse block data
	bool recvError = false;
	xptPacketbuffer_beginReadPacket(xptClient->recvBuffer);
	// update work info, GBT style (sha256 & scrypt)
	xptClient->blockWorkInfo.version = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);				// version
	xptClient->blockWorkInfo.height = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);				// block height
	xptClient->blockWorkInfo.nBits = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);				// nBits
	// New in xpt version 6 - Targets are send in compact format (4 bytes instead of 32)
	uint32 targetCompact = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
	uint32 targetShareCompact = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);
	xptClient->blockWorkInfo.targetCompact = targetCompact;
	xptClient->blockWorkInfo.targetShareCompact = targetShareCompact;
	if( xptClient->algorithm != ALGORITHM_RIECOIN )
	{
		xptClient_getDifficultyTargetFromCompact(targetCompact, (uint32*)xptClient->blockWorkInfo.target);
		xptClient_getDifficultyTargetFromCompact(targetShareCompact, (uint32*)xptClient->blockWorkInfo.targetShare);
	}
	xptClient->blockWorkInfo.nTime = xptPacketbuffer_readU32(xptClient->recvBuffer, &recvError);				// nTimestamp
	xptPacketbuffer_readData(xptClient->recvBuffer, xptClient->blockWorkInfo.prevBlockHash, 32, &recvError);	// prevBlockHash
	xptPacketbuffer_readData(xptClient->recvBuffer, xptClient->blockWorkInfo.merkleRoot, 32, &recvError);		// merkleroot
	// coinbase part1 (16bit length + data)
	xptClient->blockWorkInfo.coinBase1Size = xptPacketbuffer_readU16(xptClient->recvBuffer, &recvError);
	xptPacketbuffer_readData(xptClient->recvBuffer, xptClient->blockWorkInfo.coinBase1, xptClient->blockWorkInfo.coinBase1Size, &recvError);
	// coinbase part2 (16bit length + data)
	xptClient->blockWorkInfo.coinBase2Size = xptPacketbuffer_readU16(xptClient->recvBuffer, &recvError);
	xptPacketbuffer_readData(xptClient->recvBuffer, xptClient->blockWorkInfo.coinBase2, xptClient->blockWorkInfo.coinBase2Size, &recvError);
	// information about remaining tx hashes
	xptClient->blockWorkInfo.txHashCount = xptPacketbuffer_readU16(xptClient->recvBuffer, &recvError);
	printf("New block data - height: %d tx count: %d\n", xptClient->blockWorkInfo.height, xptClient->blockWorkInfo.txHashCount);
	for(uint32 i=0; i<xptClient->blockWorkInfo.txHashCount; i++)
	{
		xptPacketbuffer_readData(xptClient->recvBuffer, xptClient->blockWorkInfo.txHashes+(32*i), 32, &recvError);
		// The first hash in xptClient->blockWorkInfo.txHashes is reserved for the coinbase transaction
	}
	xptClient->blockWorkInfo.timeWork = time(NULL);
	xptClient->blockWorkInfo.timeBias = xptClient->blockWorkInfo.nTime - (uint32)time(NULL);
	xptClient->hasWorkData = true;
	return true;
}

/*
 * Called when a packet with the opcode XPT_OPC_S_SHARE_ACK is received
 */
bool xptClient_processPacket_shareAck(xptClient_t* xptClient)
{
	xptPacketbuffer_t* cpb = xptClient->recvBuffer;
	// read data from the packet
	xptPacketbuffer_beginReadPacket(cpb);
	// start parsing
	bool readError = false;
	// read error code field
	uint32 shareErrorCode = xptPacketbuffer_readU32(cpb, &readError);
	if( readError )
		return false;
	// read reject reason
	char rejectReason[512];
	xptPacketbuffer_readString(cpb, rejectReason, 512, &readError);
	rejectReason[511] = '\0';
	float shareValue = xptPacketbuffer_readFloat(cpb, &readError);
	if( readError )
		return false;
	if( shareErrorCode == 0 )
	{
		time_t now = time(0);
		char* dt = ctime(&now);
		//printf("Share accepted by server\n");
		//printf(" [ %d / %d val: %.6f] %s", valid_shares, total_shares, shareValue, dt);
		//primeStats.fShareValue += shareValue;
	}
	else
	{
		// share not accepted by server
		printf("Invalid share\n");
		if( rejectReason[0] != '\0' )
			printf("Reason: %s\n", rejectReason);
		totalRejectedShareCount++;
	}
	return true;
}

/*
 * Called when a packet with the opcode XPT_OPC_S_MESSAGE is received
 */
bool xptClient_processPacket_message(xptClient_t* xptClient)
{
	xptPacketbuffer_t* cpb = xptClient->recvBuffer;
	// read data from the packet
	xptPacketbuffer_beginReadPacket(cpb);
	// start parsing
	bool readError = false;
	// read type field (not used yet)
	uint32 messageType = xptPacketbuffer_readU8(cpb, &readError);
	if( readError )
		return false;
	// read message text (up to 1024 bytes)
	char messageText[1024];
	xptPacketbuffer_readString(cpb, messageText, 1024, &readError);
	messageText[1023] = '\0';
	if( readError )
		return false;
	printf("Server message: %s\n", messageText);
	return true;
}

/*
 * Called when a packet with the opcode XPT_OPC_S_PING is received
 */
bool xptClient_processPacket_ping(xptClient_t* xptClient)
{
	xptPacketbuffer_t* cpb = xptClient->recvBuffer;
	// read data from the packet
	xptPacketbuffer_beginReadPacket(cpb);
	// start parsing
	bool readError = false;
	// read timestamp
	uint64 timestamp = xptPacketbuffer_readU64(cpb, &readError);
	if( readError )
		return false;
	// get current high precision time and frequency
	uint64 timestampNow = getTimeHighRes();
	// calculate time difference in ms
	uint64 timeDif = timestampNow - timestamp;
#ifdef _WIN32
	timeDif *= 10000ULL;
	timeDif /= getTimerRes();
#else
	timeDif /= 100000;
#endif
	// update and calculate simple average
	xptClient->pingSum += timeDif;
	xptClient->pingCount++;
	double averagePing = (double)xptClient->pingSum / (double)xptClient->pingCount / 10.0;
	printf("Ping %d.%dms (Average %.1lf)\n", (sint32)(timeDif/10), (sint32)(timeDif%10), averagePing);
	return true;
}