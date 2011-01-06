
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

	uint32_t size32 = *(uint32_t *)(*buffer);
	size32 = ntohl(size32);
	*buffer += sizeof(uint32_t);
	
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
		case EC_TAGTYPE_HASH16:
			tag = [ECTagMD5 tagFromBuffer:buffer];
			break;
		case EC_TAGTYPE_STRING:
			tag = [ECTagString tagFromBuffer:buffer];
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

	uint16_t count16 = *(uint16_t *)(*buffer);
	count16 = ntohs(count16);
	*buffer += sizeof(count16);
	NSMutableArray *array = [[NSMutableArray alloc] init];
	[array retain];
	for(int i = 0; i < count16; i++) {
		id tag = [ECTag tagFromBuffer:buffer withLenght:length];
		if ( tag != nil ) {
			[array addObject:tag];
		}
	}
	
	return array;
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

- (id)tagByName:(ECTagNames) tagname {
	ECTag *mytag = nil;
	for (ECTag *t in m_subtags) {
		if (t.tagName == tagname) {
			mytag = t;
			break;
		}
	}
	return mytag;
}

- (void)initSubtags {
	m_subtags = [NSMutableArray array];
	[m_subtags retain];
}

- (void) dealloc {
	[m_subtags release];
	[super dealloc];
}

- (uint64_t)tagInt64ByName: (ECTagNames) tagname {
	ECTag *st = [self tagByName: tagname];
	if (st == nil) {
		return 0;
	}
	uint64_t value = 0;
	switch ([st getSize]) {
		case 1: {
			ECTagInt8 *t = (ECTagInt8 *)st;
			value = t.uint8Value;
			break;
			}
		case 2: {
			ECTagInt16 *t = (ECTagInt16 *)st;
			value = t.uint16Value;
			break;
			}
		case 4: {
			ECTagInt32 *t = (ECTagInt32 *)st;
			value = t.uint32Value;
			break;
			}
		case 8: {
			ECTagInt64 *t = (ECTagInt64 *)st;
			value = t.uint64Value;
			break;
			}
	}
	return value;
}

- (int)tagCount {
	return [m_subtags count];
}

@end

@implementation ECTagInt8

@synthesize uint8Value = m_val;

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

@synthesize uint16Value = m_val;

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

@synthesize uint32Value = m_val;

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
	
	tag->m_val = ntohl(*((uint32_t *)(*buffer)));
	tag->m_size = 4;
	tag->m_type = EC_TAGTYPE_UINT32;
	
	*buffer += 4;

	return tag;
}


@end

@implementation ECTagInt64

@synthesize uint64Value = m_val;

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
	hi = ntohl(val32);
	*buffer += 4;
	
	val32 = *((uint32_t *)(*buffer));
	lo = ntohl(val32);
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

+ (id)tagFromBuffer:(uint8_t **) buffer withLenght:(int) length {
	ECTagData *tag = [[ECTagData alloc] init];

	tag->m_data = [NSData dataWithBytes: *buffer length: length];
	[tag->m_data retain];
	
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

+ (id)tagFromBuffer:(uint8_t **) buffer {
	ECTagMD5 *tag = [[ECTagMD5 alloc] init];
	
	tag->m_data = 0;
	tag->m_val.lo = *((uint64_t *)(*buffer));
	(*buffer) += 8;
	tag->m_val.hi = *((uint64_t *)(*buffer));
	(*buffer) += 8;
	
	return tag;
}

- (MD5Data)getMD5Data {
	if ( m_data ) {
		uint8_t *data_ptr = (uint8_t *)[m_data bytes];

		uint64_t hi = *(uint64_t *)data_ptr;
		data_ptr += 8;
		uint64_t lo = *(uint64_t *)data_ptr;
		MD5Data md5 = {hi, lo};
		return md5;
	} else {
		return m_val;
	}
}
- (NSString *)stringKey {
	NSString *s = [NSString stringWithFormat:@"%qx%qx", m_val.hi, m_val.lo];
	return s;
}

@end

@implementation ECTagString

@synthesize stringValue = m_val;

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

+ (id)tagFromBuffer:(uint8_t **) buffer {
	ECTagString *tag = [[ECTagString alloc] init];

	tag->m_val = [NSString stringWithCString:(char *)(*buffer) encoding:NSUTF8StringEncoding];
	*buffer += [tag->m_val length] + 1;
	
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

	// allow notification push to my client
	m_flags |= EC_FLAG_NOTIFY | EC_FLAG_ACCEPTS;

	[self initSubtags];
}

+ (id)packetFromBuffer:(uint8_t *) buffer withLength:(int)length {
	if ( length < 11 ) {
		return nil;
	}
	ECPacket *p = [[ECPacket alloc] init];
	uint8_t *data = buffer;
	
	p->m_flags = ntohl(*((uint32_t *)data));
	data += 4;
	uint32_t packet_size = ntohl(*((uint32_t *)data));
	data += 4;
	if ( packet_size > 1024*1024 ) {
		return nil;
	}
	p->m_opcode = (ec_opcode_t)(*data);
	data++;

	uint16_t tag_count = ntohs(*((uint16_t *)data));
	data += 2;
	if ( tag_count ) {
		p->m_subtags = [[NSMutableArray alloc] init];
		[p->m_subtags retain];
		uint8_t *start_ptr = data;
		for(int i = 0; i < tag_count; i++) {
			ECTag *tag = [ECTag tagFromBuffer:&data withLenght:(length - (data - start_ptr))];
			// some tags are not supported yet
			if ( tag != nil ) {
				[p->m_subtags addObject:tag];
			}
		}
	}
	
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

@implementation ECLoginAuthPacket

- (NSString *)getMD5_Str:(NSString *) str {
	CC_MD5_CTX ctx;
	unsigned char md5data[16];
	CC_MD5_Init(&ctx);
	CC_MD5_Update(&ctx, [str UTF8String], [str length]);
	CC_MD5_Final(md5data, &ctx);
	NSString *MD5str = [NSString stringWithFormat:@"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
						 md5data[0], md5data[1],md5data[2],md5data[3],
						 md5data[4],md5data[5],md5data[6],md5data[7],
						 md5data[8],md5data[9],md5data[10],md5data[11],
						 md5data[12],md5data[13],md5data[14],md5data[15]
						 ];
	return MD5str;
}

+ (id)loginPacket:(NSString *) password withSalt:(uint64_t) salt {
	ECLoginAuthPacket *p = [[ECLoginAuthPacket alloc] init];

	[p initWithOpcode:EC_OP_AUTH_PASSWD];

	NSString *saltStr = [NSString stringWithFormat:@"%llX", salt];
//	CC_MD5_CTX ctx;
//	unsigned char md5data[16];
//	CC_MD5_Init(&ctx);
//	CC_MD5_Update(&ctx, [saltStr UTF8String], [saltStr length]);
//	CC_MD5_Final(md5data, &ctx);
//	NSString *saltMD5 = [NSString stringWithFormat:@"%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x",
//						 md5data[0], md5data[1],md5data[2],md5data[3],
//						 md5data[4],md5data[5],md5data[6],md5data[7],
//						 md5data[8],md5data[9],md5data[10],md5data[11],
//						 md5data[12],md5data[13],md5data[14],md5data[15]
//						 ];
	NSString *saltMD5 = [p getMD5_Str: saltStr];
	
	NSString *newPass = [NSString stringWithFormat:@"%@%@", [p getMD5_Str: password], saltMD5];

	NSLog(@"[EC] using salt=%@ saltHash=%@ newPass=%@\n", saltStr, saltMD5, newPass);

	ECTagMD5 *passtag = [ECTagMD5 tagFromString: newPass withName:EC_TAG_PASSWD_HASH];
	[p->m_subtags addObject:passtag];

	return p;
}


@end

@implementation ECLoginRequestPacket

+ (id)loginPacket:(NSString *) version {
	ECLoginRequestPacket *p = [[ECLoginRequestPacket alloc] init];
	
	[p initWithOpcode:EC_OP_AUTH_REQ];
	
	ECTagString *version_tag = [ECTagString tagFromString:version withName:EC_TAG_CLIENT_VERSION];
	[p->m_subtags addObject:version_tag];
	
	ECTagString *client_name_tag = [ECTagString tagFromString:@"cocoa-frontend" withName:EC_TAG_CLIENT_NAME];
	[p->m_subtags addObject:client_name_tag];
	
	ECTagInt64 *proto_version_tag = [ECTagInt64 tagFromInt64:EC_CURRENT_PROTOCOL_VERSION withName:EC_TAG_PROTOCOL_VERSION];
	[p->m_subtags addObject:proto_version_tag];
	
	return p;
}


@end


@implementation ECRemoteConnection

@synthesize error = m_error;

+ (id)remoteConnection {
	ECRemoteConnection *p = [[ECRemoteConnection alloc] init];
	
	//
	// rx buffer can be resized as needed
	//
	p->m_rxbuf = [NSMutableData dataWithLength:1024];
	[p->m_rxbuf retain];
	
	p->m_login_handler = [amuleLoginHandler initWithConnection:p];
	[p->m_login_handler retain];
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

- (bool)isIpv4Address:(NSString *) address {
	NSArray *ar = [address componentsSeparatedByString:@"."];
	if ( [ar count] != 4 ) {
		return false;
	}
	for (NSString *s in ar) {
		const char *p = [s UTF8String];
		while ( *p ) {
			if ( !isdigit(*p) ) {
				return false;
			}
			p++;
		}
	}
	return true;
}

- (void)connectToAddress:(NSString *) hostname withPort:(int)trgport {
	m_error = false;
	
	NSHost *host = [NSHost hostWithName:hostname];
	NSString *addr = nil;
	
	//
	// On Mac localhost has ipv6 address (linklocal), but amuled listen
	// only on ipv4
	//
	for (NSString *ad in host.addresses) {
		NSLog(@"host have address=%@ is_ipv4=%d\n", ad, [self isIpv4Address:ad]);
		if ( [self isIpv4Address:ad] ) {
			addr = ad;
			break;
		}
	}
	if ( addr == nil ) {
		return;
	}
	host = [NSHost hostWithAddress:addr];

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

	[m_login_handler usePass: password];
	
	ECLoginRequestPacket *p = [ECLoginRequestPacket loginPacket:@"0.1"];
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
	NSStreamStatus stream_status = [m_ostream streamStatus];
	switch ( stream_status ) {
		case NSStreamStatusNotOpen:
		case NSStreamStatusClosed:
		case NSStreamStatusError:
			NSLog(@"[EC] error in output stream\n");
			m_error = true;
			break;
		default:;
	}
//	NSLog(@"[EC] status in output stream=%d\n", stream_status);

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
			unsigned int len = [m_rxbuf length];

            len = [(NSInputStream *)stream read:data_ptr + m_rx_size maxLength:len];
#ifdef EC_RX_DEBUG
			NSLog(@"[EC] receiving %d bytes, %d in total, %d remaining\n", len, m_rx_size, m_remaining_size);
#endif
			if ( len == 0 ) {
				//
				// Remote side must be closed connection
				//
				m_error = true;
				if ( [delegate respondsToSelector:@selector(handleError)] ) {
					[delegate performSelector:@selector(handleError)];
				}
			}
			int total_len = len;
			int packet_offset = 0;
			while ( total_len != 0 ) {
				len = ( m_remaining_size > total_len ) ? total_len : m_remaining_size;
				total_len -= len;

				// are we still waiting for flags and size?
				if ( m_rx_size < 8 ) {
					if ( (m_rx_size + len) >= 8 ) {
						// got flags and packet size - may proceed
						//uint32_t flags = *(((uint32_t *)[m_rxbuf mutableBytes]) + 0);
						uint32_t val32 = *((uint32_t *)(data_ptr + 4 + packet_offset));

						int delta = 8 - m_rx_size;

						m_remaining_size = ntohl(val32) - (len - delta);
#ifdef EC_RX_DEBUG
						NSLog(@"[EC] rx got flags+size, remaining count %d\n", m_remaining_size);
#endif
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
					uint8_t *packet_start = data_ptr + packet_offset;
					int packet_length = [m_rxbuf length] - packet_offset;
					ECPacket *packet = [ECPacket packetFromBuffer:packet_start withLength:packet_length];
					packet_offset += m_rx_size;
					
					if ( [m_login_handler loginOK] ) {
#ifdef EC_RX_DEBUG
						NSLog(@"[EC] calling delegate\n");
#endif
						if ( [delegate respondsToSelector:@selector(handlePacket:)] ) {
							[delegate performSelector:@selector(handlePacket:) withObject:packet];
						}
					} else {
						NSLog(@"[EC] login handler\n");
						[m_login_handler handlePacket: packet];
					}
					m_remaining_size = 8;
					m_rx_size = 0;
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

- (void)setDelegate:(id)val {
    delegate = val;
}

- (id)delegate {
    return delegate;
}


@end

@implementation amuleLoginHandler

+ (id)initWithConnection:(ECRemoteConnection *) connection {

	amuleLoginHandler *obj = [[amuleLoginHandler alloc] init]; 
	obj->m_connection = connection;
	
	obj->m_state = LOGIN_REQUEST_SENT;

	return obj;
}

- (void)usePass:(NSString *) pass {
	m_pass = pass;
}

- (void)handlePacket:(ECPacket *) packet {
	switch(m_state) {
		case LOGIN_IDLE:
			NSLog(@"[EC]: error - no packet should come until request is sent\n");
			break;
		case LOGIN_REQUEST_SENT:
			if ( packet.opcode == EC_OP_AUTH_SALT ) {
				
				uint64_t salt = [packet tagInt64ByName:EC_TAG_PASSWD_SALT];
				ECLoginAuthPacket *auth_packet = [ECLoginAuthPacket loginPacket:m_pass withSalt:salt];
				[m_connection sendPacket:auth_packet];
				
				m_state = LOGIN_PASS_SENT;
			} else {
				NSLog(@"[EC]: error - expecting packet with EC_OP_AUTH_SALT, not [%d]\n", packet.opcode);
				m_state = LOGIN_IDLE;
			} 
			break;
		case LOGIN_PASS_SENT:
			if ( packet.opcode == EC_OP_AUTH_OK ) {
				m_state = LOGIN_OK;
			} else {
				NSLog(@"[EC]: error - login failed, core replied with code=[%d]\n", packet.opcode);
			}
			break;
		case LOGIN_OK:
			NSLog(@"[EC]: error - this delegate should be replaced after login completed\n");
			break;
	}
}
- (void)reset {
	m_state = LOGIN_IDLE;
}

- (bool)loginOK {
	return m_state == LOGIN_OK;
}


@end
