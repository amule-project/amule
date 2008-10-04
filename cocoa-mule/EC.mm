
#import "EC.h"

#import <CommonCrypto/CommonDigest.h>

@implementation ECTag

@synthesize tagName = m_name;
@synthesize tagType = m_type;
@synthesize subtags = m_subtags;

+ (id)tagFromBuffer:(uint8_t **) buffer withLenght:(int) length {
	ECTag *tag = nil; //[[ECTag alloc] init];

	if ( length < 4 ) {
		NSLog(@"[EC] buffer for tag is too short");
		return nil;
	}
	
	uint16_t name16 = *(uint16_t *)(*buffer);
	name16 = ntohs(name16);
	bool have_subtags = (name16 & 1) != 0;
	name16 >>= 1;
	ECTagNames tag_name = (ECTagNames)(name16);
	*buffer += sizeof(name16);
	
	uint8_t type8 = *(*buffer);
	ECTagTypes tag_type = (ECTagTypes)type8;
	*buffer += sizeof(type8);

	NSMutableArray *subtags = have_subtags ? [ECTag readSubtags:buffer withLenght:(length-3)] : nil;

	switch (tag_type) {
		case EC_TAGTYPE_UINT8:
			tag = [ECTagInt8 tagFromBuffer:buffer];
			break;
		case EC_TAGTYPE_UINT16:
			tag = [ECTagInt16 tagFromBuffer:buffer];
			break;
		case EC_TAGTYPE_UINT32:
			tag = [ECTagInt32 tagFromBuffer:buffer];
			break;
		case EC_TAGTYPE_UINT64:
			tag = [ECTagInt64 tagFromBuffer:buffer];
			break;
		default: ;
			break;
	}
	if ( tag != nil ) {
		tag->m_name = tag_name;
		tag->m_subtags = subtags;
	}
	return tag;	
}

+ (NSMutableArray *)readSubtags:(uint8_t **) buffer withLenght:(int) length {

	uint16_t count16 = *(uint16_t *)buffer;
	count16 = ntohs(count16);
	buffer += sizeof(count16);
	
	for(int i = 0; i < count16; i++) {
	}
	
	return nil;
}

- (void)writeToSocket:(NSOutputStream *) socket {
	uint16_t name16 = (uint16_t)m_name;
	name16 <<= 1;
	uint8_t type8 = (uint8_t)m_type;
	if ( [m_subtags count] ) {
		name16 |= 1;
	}
	name16 = htons(name16);
	[socket write:(uint8_t *)&name16 maxLength:sizeof(name16)];
	[socket write:&type8 maxLength:sizeof(type8)];
	uint32_t size32 = [self getSize];
	size32 = htonl(size32);
	[socket write:(uint8_t *)&size32 maxLength:sizeof(size32)];
}

- (void)writeSubtagsToSocket:(NSOutputStream *) socket {
	uint16_t count16 = [m_subtags count];
	count16 = htons(count16);
	[socket write:(uint8_t *)&count16 maxLength:sizeof(count16)];
	for (ECTag *t in m_subtags) {
		[t writeToSocket:socket];
	}
}

- (int)getSize {
	int total_size = m_size;
	for (ECTag *t in m_subtags) {
		total_size += [t getSize];
		// name + type + size
		total_size += (2 + 1 + 4);
		if ([t->m_subtags count]) {
			total_size += 2;
		}
	}
	return total_size;
}

- (void)initSubtags {
	m_subtags = [NSMutableArray array];
	[m_subtags retain];
}

- (void) dealloc {
	[m_subtags release];
	[super dealloc];
}

@end

@implementation ECTagInt8

+ (id)tagFromInt8:(uint8_t) value withName:(ECTagNames) name {
	ECTagInt8 *tag = [[ECTagInt8 alloc] init];
	tag->m_val = value;
	tag->m_size = 1;
	tag->m_type = EC_TAGTYPE_UINT8;
	tag->m_name = name;
	
	return tag;	
}

+ (id)tagFromBuffer:(uint8_t **) buffer {
	ECTagInt8 *tag = [[ECTagInt8 alloc] init];
	tag->m_val = **buffer;
	tag->m_size = 1;
	tag->m_type = EC_TAGTYPE_UINT8;
	
	*buffer += 1;
	
	return tag;
}

- (void)writeToSocket:(NSOutputStream *) socket {
	[super writeToSocket:socket];
	
	[socket write:&m_val maxLength:sizeof(m_val)];
}

@end

@implementation ECTagInt16

+ (id)tagFromInt16:(uint16_t) value withName:(ECTagNames) name {
	ECTagInt16 *tag = [[ECTagInt16 alloc] init];
	tag->m_val = value;
	tag->m_size = 2;
	tag->m_type = EC_TAGTYPE_UINT16;
	tag->m_name = name;
	
	return tag;	
}

+ (id)tagFromBuffer:(uint8_t **) buffer {
	ECTagInt16 *tag = [[ECTagInt16 alloc] init];
	
	tag->m_val = ntohs(*((uint16_t *)(*buffer)));
	tag->m_size = 2;
	tag->m_type = EC_TAGTYPE_UINT16;
	
	*buffer += 2;

	return tag;
}


@end

@implementation ECTagInt32

+ (id)tagFromInt32:(uint32_t) value withName:(ECTagNames) name {
	ECTagInt32 *tag = [[ECTagInt32 alloc] init];
	tag->m_val = value;
	tag->m_size = 4;
	tag->m_type = EC_TAGTYPE_UINT32;
	tag->m_name = name;
	
	return tag;	
}

+ (id)tagFromBuffer:(uint8_t **) buffer {
	ECTagInt32 *tag = [[ECTagInt32 alloc] init];
	
	tag->m_val = ntohs(*((uint32_t *)(*buffer)));
	tag->m_size = 4;
	tag->m_type = EC_TAGTYPE_UINT32;
	
	*buffer += 4;

	return tag;
}


@end

@implementation ECTagInt64

+ (id)tagFromInt64:(uint64_t) value withName:(ECTagNames) name {
	ECTagInt64 *tag = [[ECTagInt64 alloc] init];
	tag->m_val = value;
	tag->m_size = 8;
	tag->m_type = EC_TAGTYPE_UINT64;
	tag->m_name = name;
	
	return tag;	
}


+ (id)tagFromBuffer:(uint8_t **) buffer {
	ECTagInt64 *tag = [[ECTagInt64 alloc] init];
	uint64_t lo, hi;
	uint32 val32 = *((uint32_t *)(*buffer));
	lo = ntohl(val32);
	*buffer += 4;

	val32 = *((uint32_t *)(*buffer));
	hi = ntohl(val32);
	*buffer += 4;
	
	tag->m_val = (hi << 32) | lo;
	tag->m_size = 8;
	tag->m_type = EC_TAGTYPE_UINT64;
	
	return tag;
}


- (void)writeToSocket:(NSOutputStream *) socket {
	[super writeToSocket:socket];
	
	uint32_t val32 = m_val >> 32;
	val32 = htonl(val32);
	[socket write:(uint8_t *)&val32 maxLength:sizeof(val32)];

	val32 = m_val & 0xffffffff;
	val32 = htonl(val32);
	[socket write:(uint8_t *)&val32 maxLength:sizeof(val32)];
}

@end


@implementation ECTagData

- (void)writeToSocket:(NSOutputStream *) socket {
	[super writeToSocket:socket];
	
	[socket write:(const uint8_t *)[m_data bytes] maxLength:m_size];
}

- (void) dealloc {
	[m_data release];
	[super dealloc];
}

+ (id)tagFromBuffer:(uint8_t **) buffer {
	ECTagInt64 *tag = [[ECTagInt64 alloc] init];
	
	return tag;
}


@end

@implementation ECTagMD5

+ (id)tagFromString:(NSString *) string withName:(ECTagNames) name {
	ECTagMD5 *tag = [[ECTagMD5 alloc] init];

	CC_MD5_CTX ctx;
	unsigned char md5data[16];
	CC_MD5_Init(&ctx);
	CC_MD5_Update(&ctx, [string UTF8String], [string length]);
	CC_MD5_Final(md5data, &ctx);
	
	tag->m_data = [NSData dataWithBytes: md5data length: sizeof(md5data)];
	[tag->m_data retain];
	tag->m_size = 16;
	tag->m_type = EC_TAGTYPE_HASH16;
	tag->m_name = name;

	return tag;
}


- (MD5Data)getMD5Data {
	uint8_t *data_ptr = (uint8_t *)[m_data bytes];

	uint64_t hi = *(uint64_t *)data_ptr;
	data_ptr += 8;
	uint64_t lo = *(uint64_t *)data_ptr;
	MD5Data md5 = {hi, lo};
	return md5;
}

@end

@implementation ECTagString

+ tagFromString:(NSString *) string withName:(ECTagNames) name {
	ECTagString *tag = [[ECTagString alloc] init];

	const char *rawstring = [string UTF8String];
	tag->m_size = strlen(rawstring) + 1;
	tag->m_data = [NSData dataWithBytes: rawstring length: tag->m_size];
	[tag->m_data retain];
	tag->m_type = EC_TAGTYPE_STRING;
	tag->m_name = name;

	return tag;
}


@end

@implementation ECPacket

@synthesize opcode = m_opcode;

+ (id)packetWithOpcode:(ec_opcode_t) opcode {
	ECPacket *p = [[ECPacket alloc] init];

	[p initWithOpcode:opcode];
	
	return p;
}


- (void)initWithOpcode:(ec_opcode_t) opcode {
	m_opcode = opcode;
	m_flags = 0x20;
	[self initSubtags];
}

+ (id)packetFromBuffer:(NSMutableData *) buffer {
	if ( [buffer length] < 11 ) {
		return nil;
	}
	ECPacket *p = [[ECPacket alloc] init];
	uint8_t *data = (uint8_t *)[buffer mutableBytes];
	
	p->m_flags = ntohl(*((uint32_t *)data));
	data += 4;
	uint32_t packet_size = ntohl(*((uint32_t *)data));
	data += 4;
	if ( packet_size > 1024*1024 ) {
		return nil;
	}
	p->m_opcode = (ec_opcode_t)(*data);
	data++;

	uint16_t tag_count = ntohl(*((uint16_t *)data));
	uint8_t *start_ptr = data;
	for(int i = 0; i < tag_count; i++) {
		ECTag *tag = [ECTag tagFromBuffer:&data withLenght:([buffer length] - (data - start_ptr))];
		[p->m_subtags addObject:tag];
	}
	data += 2;
	
	return p;
}

- (void)writeToSocket:(NSOutputStream *) socket {
	// 1 (command) + 2 (tag count)
	int packet_size = [self getSize] + 1 + 2;

// No need for zlib on client side	
//	if ( packet_size > MaxUncompressedPacket ) {
//		m_flags |= (Int32)ECFlags.EC_FLAG_ZLIB;
//	}
	uint32_t val32 = htonl(m_flags);
	[socket write:(uint8_t *)&val32 maxLength:sizeof(val32)];
	if ( m_flags & EC_FLAG_ACCEPTS ) {
		[socket write:(uint8_t *)&val32 maxLength:sizeof(val32)];
	}
	val32 = htonl(packet_size);
	[socket write:(uint8_t *)&val32 maxLength:sizeof(val32)];
	[socket write:(uint8_t *)&m_opcode maxLength:sizeof(m_opcode)];

	if ( [m_subtags count] ) {
		[self writeSubtagsToSocket:socket];
	} else {
		uint16_t val16 = 0;
		[socket write:(uint8_t *)&val16 maxLength:sizeof(val16)];
	}
}

@end

@implementation ECLoginPacket

+ (id)loginPacket:(NSString *) password withVersion:(NSString *) version {
	ECLoginPacket *p = [[ECLoginPacket alloc] init];

	[p initWithOpcode:EC_OP_AUTH_REQ];

	ECTagMD5 *passtag = [ECTagMD5 tagFromString: password withName:EC_TAG_PASSWD_HASH];
	[p->m_subtags addObject:passtag];

	ECTagString *version_tag = [ECTagString tagFromString:version withName:EC_TAG_CLIENT_VERSION];
	[p->m_subtags addObject:version_tag];
	
	ECTagString *client_name_tag = [ECTagString tagFromString:@"cocoa-frontend" withName:EC_TAG_CLIENT_NAME];
	[p->m_subtags addObject:client_name_tag];
	
	ECTagInt64 *proto_version_tag = [ECTagInt64 tagFromInt64:EC_CURRENT_PROTOCOL_VERSION withName:EC_TAG_PROTOCOL_VERSION];
	[p->m_subtags addObject:proto_version_tag];
	
	// allow notification push to my client
	p->m_flags |= EC_FLAG_NOTIFY;
	
	return p;
}


@end


@implementation ECRemoteConnection

+ (id)remoteConnection {
	ECRemoteConnection *p = [[ECRemoteConnection alloc] init];
	
	//
	// rx buffer can be resized as needed
	//
	p->m_rxbuf = [NSMutableData dataWithLength:1024];
	[p->m_rxbuf retain];
	
	//
	// client only transmit commands, which are
	// quite small in size. "big enough" buffer will do the trick
	//
//	p->m_txbuf = [NSMutableData dataWithLength:1024];
//	[p->m_txbuf retain];
	p->m_txbuf = nil;
	
	return p;
}

- (void) dealloc {
	[m_rxbuf release];
	[m_txbuf release];

	[m_istream release];
	[m_ostream release];
	
	[super dealloc];
}


- (void)connectToAddress:(NSString *) hostname withPort:(int)trgport {
//	NSHost *host = [NSHost hostWithName:hostname];
	NSHost *host = [NSHost hostWithAddress:hostname];
	
	[NSStream getStreamsToHost:host port:trgport inputStream:&m_istream outputStream:&m_ostream];

	[m_istream retain];
	[m_ostream retain];
	
	[m_istream setDelegate:self];
	[m_ostream setDelegate:self];
	
	[m_istream scheduleInRunLoop:[NSRunLoop currentRunLoop]
            forMode:NSDefaultRunLoopMode];

	[m_ostream scheduleInRunLoop:[NSRunLoop currentRunLoop]
            forMode:NSDefaultRunLoopMode];

	[m_istream open];
	[m_ostream open];
	
	m_remaining_size = 8;
	m_rx_size = 0;
}

- (void)sendLogin:(NSString *) password {
	m_login_requested = true;
	m_login_ok = false;
	ECLoginPacket *p = [ECLoginPacket loginPacket:password withVersion:@"0.1"];
	[self sendPacket:p];
}

- (void)sendPacket:(ECPacket *) packet {
	NSOutputStream *memstream = [NSOutputStream outputStreamToMemory];
	[memstream open];
	
	[packet writeToSocket:memstream];
	id data = [memstream propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
	
 	m_tx_size = [data length];
	NSLog(@"[EC] sending packet %d bytes\n", m_tx_size);
	m_tx_ptr = [m_ostream write:(const uint8_t *)[data bytes] maxLength:[data length]];
	NSLog(@"[EC] %d bytes sent\n", m_tx_ptr);
	if ( m_tx_ptr == m_tx_size ) {
		m_txbuf = nil;
	} else {
		m_txbuf = (NSData *)data;
		[m_txbuf retain];
	}
}

- (void)stream:(NSStream *)stream handleEvent:(NSStreamEvent)eventCode {
    switch(eventCode) {
		case NSStreamEventOpenCompleted:
		{
			NSLog(@"[EC] open complete\n");
			break;
		}
		case NSStreamEventErrorOccurred:
		{
			NSError *e = [stream streamError];
			NSString *description = [e localizedDescription];
			NSLog(@"[EC] socket error [%s]\n", [description UTF8String]);
			break;
		}
        case NSStreamEventHasBytesAvailable:
        {
			uint8_t *data_ptr = (uint8_t *)[m_rxbuf mutableBytes];
			data_ptr += m_rx_size;
			unsigned int len = [m_rxbuf length];
            len = [(NSInputStream *)stream read:data_ptr maxLength:len];
			NSLog(@"[EC] receiving %d bytes, %d in total, %d remaining\n", len, m_rx_size, m_remaining_size);
			if ( len == 0 ) {
				//
				// Remote side must be closed connection
				//
			}
			m_remaining_size -= len;
			// are we still waiting for flags and size?
			if ( m_rx_size < 8 ) {
				if ( (m_rx_size + len) >= 8 ) {
					// got flags and packet size - may proceed
					//uint32_t flags = *(((uint32_t *)[m_rxbuf mutableBytes]) + 0);
					uint32_t val32 = *((uint32_t *)(data_ptr + 4));

					int delta = 8 - m_rx_size;

					m_remaining_size = ntohl(val32) - (len - delta);
					NSLog(@"[EC] rx got flags+size, remaining count %d\n", m_remaining_size);
				} else {
					m_remaining_size -= len;
				}
			} else {
				m_remaining_size -= len;
			}
			m_rx_size += len;
			if ( m_remaining_size == 0 ) {
				//
				// full packet received, call handler
				//
				ECPacket *packet = [ECPacket packetFromBuffer:m_rxbuf];
				if ( m_login_requested ) {
					m_login_requested = false;
					m_login_ok = packet.opcode == EC_OP_AUTH_OK;
				} else {
					if ( [delegate respondsToSelector:@selector(handlePacket:)] ) {
						[delegate handlePacket: packet];
					}
				}

			}
            break;
        }
		
        case NSStreamEventHasSpaceAvailable:
		{
			if ( m_txbuf != nil ) {
				int remaining = [m_txbuf length] - m_tx_ptr;
				if ( remaining ) {
					const uint8_t *txdata = ((const uint8_t *)[m_txbuf bytes]) + m_tx_ptr;
					int txcount = [m_ostream write:txdata maxLength:remaining];
					m_tx_ptr += txcount;
				} else {
					[m_txbuf release];
					m_txbuf = nil;
				}

			}
			break;
		}
	}
}

- (void)setDelegate:(id)val
{
    delegate = val;
}

- (id)delegate
{
    return delegate;
}


@end
