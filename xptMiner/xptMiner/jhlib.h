typedef unsigned long long 	uint64;
typedef signed long long	sint64;
typedef unsigned int 		uint32;
typedef signed int 			sint32;
typedef unsigned short		uint16;
typedef signed short 		sint16;
typedef unsigned char 		uint8;
typedef signed char 		sint8;

typedef struct  
{
	void** objects;
	uint32 objectCount;
	uint32 objectLimit;
	uint32 stepScaler;
	bool isPreallocated;
	bool doNotFreeRawData;
}simpleList_t;


simpleList_t* simpleList_create(sint32 initialLimit);
void simpleList_create(simpleList_t* simpleList, sint32 initialLimit);
void simpleList_create(simpleList_t* simpleList, sint32 initialLimit, void** rawArray);

void simpleList_free(simpleList_t* simpleList);
void simpleList_add(simpleList_t* simpleList, void* object);

typedef struct _stream_t stream_t;

typedef struct  
{
	uint32 (*readData)(void *object, void *buffer, uint32 len);
	uint32 (*writeData)(void *object, void *buffer, uint32 len);
	uint32 (*getSize)(void *object);
	void (*setSize)(void *object, uint32 size);
	uint32 (*getSeek)(void *object);
	void (*setSeek)(void *object, sint32 seek, bool relative);
	void (*initStream)(void *object, stream_t *stream);
	void (*destroyStream)(void *object, stream_t *stream);
	// general settings
	bool allowCaching;
}streamSettings_t;

typedef struct _stream_t
{
	void *object;
	//_stream_t *substream;
	streamSettings_t *settings;
	// bit access ( write )
	uint8 bitBuffer[8];
	uint8 bitIndex;
	// bit access ( read )
	uint8 bitReadBuffer[8];
	uint8 bitReadBufferState;
	uint8 bitReadIndex;
}stream_t;


stream_t*	stream_create	(streamSettings_t *settings, void *object);
void		stream_destroy	(stream_t *stream);

// stream reading

char stream_readS8(stream_t *stream);
int stream_readS32(stream_t *stream);
float stream_readFloat(stream_t *stream);
uint32 stream_readData(stream_t *stream, void *data, int len);
// stream writing
void stream_writeU8(stream_t *stream, uint8 value);
void stream_writeU16(stream_t *stream, uint16 value);
void stream_writeU32(stream_t *stream, uint32 value);
uint32 stream_writeData(stream_t *stream, void *data, int len);
// stream other
void stream_setSeek(stream_t *stream, uint32 seek);
uint32 stream_getSize(stream_t *stream);

/* stream ex */

stream_t* streamEx_fromMemoryRange(void *mem, uint32 memoryLimit);
stream_t* streamEx_fromDynamicMemoryRange(uint32 memoryLimit);
stream_t* streamEx_createSubstream(stream_t* mainstream, sint32 startOffset, sint32 size);

// misc
void* streamEx_map(stream_t* stream, sint32* size);
