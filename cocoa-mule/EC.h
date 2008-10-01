#import <Cocoa/Cocoa.h>

#include "ECCodes.h"
#include "ECTagTypes.h"

@interface ECTag : NSObject {
	ECTagTypes m_type;
    ECTagNames m_name;
	
	int m_size;
	
	NSMutableArray *m_subtags;
}

+ (id)tagFromBuffer:(uint8_t **) buffer withLenght:(int) length;
+ (NSMutableArray *)readSubtags:(uint8_t **) buffer withLenght:(int) length;

- (void)initSubtags;
- (void)writeToSocket:(NSOutputStream *) socket;
- (void)writeSubtagsToSocket:(NSOutputStream *) socket;

- (int)getSize;

@property (readonly) ECTagTypes tagType;
@property (readonly) ECTagNames tagName;

@end

@interface ECTagInt8 : ECTag {
	uint8_t m_val;
}

+ (id)tagFromInt8:(uint8_t) value withName:(ECTagNames) name;
+ (id)tagFromBuffer:(uint8_t **) buffer;

@end

@interface ECTagInt16 : ECTag {
	uint16_t m_val;
}

+ (id)tagFromInt16:(uint16_t) value withName:(ECTagNames) name;
+ (id)tagFromBuffer:(uint8_t **) buffer;

@end

@interface ECTagInt32 : ECTag {
	uint32_t m_val;
}

+ (id)tagFromInt32:(uint32_t) value withName:(ECTagNames) name;
+ (id)tagFromBuffer:(uint8_t **) buffer;

@end

@interface ECTagInt64 : ECTag {
	uint64_t m_val;
}

+ (id)tagFromInt64:(uint64_t) value withName:(ECTagNames) name;
+ (id)tagFromBuffer:(uint8_t **) buffer;

@end

@interface ECTagData : ECTag {
	NSData *m_data;
}

- (void)writeToSocket:(NSOutputStream *) socket;
+ (id)tagFromBuffer:(uint8_t **) buffer;

@end

@interface ECTagMD5 : ECTagData {
}

+ tagFromString:(NSString *) string withName:(ECTagNames) name;


@end

@interface ECTagString : ECTagData {
}

+ tagFromString:(NSString *) string withName:(ECTagNames) name;

@end

@interface ECPacket : ECTag {
	ec_opcode_t m_opcode;
	uint32_t m_flags;
}

+ (id)packetWithOpcode:(ec_opcode_t) opcode;
+ (id)packetFromBuffer:(NSMutableData *) buffer;

- (void)initWithOpcode:(ec_opcode_t) opcode;

- (void)writeToSocket:(NSOutputStream *) socket;
 
@property (readonly) ec_opcode_t opcode;

@end

@interface ECLoginPacket : ECPacket {
}

+ (id)loginPacket:(NSString *) password withVersion:(NSString *) version;

@end	

@interface NSObject (ECRemoteConnection)
- (void)handlePacket:(ECPacket *) packet;
@end

@interface ECRemoteConnection : NSObject {
	NSInputStream *m_istream;
	NSOutputStream *m_ostream;
	
	NSMutableData *m_rxbuf;
	
	int m_rx_size;
	int m_remaining_size;


	NSData *m_txbuf;
	int m_tx_ptr;
	int m_tx_size;
	
	bool m_login_requested;
	bool m_login_ok;

    id delegate;
}

+ (id)remoteConnection;

- (void)stream:(NSStream *)stream handleEvent:(NSStreamEvent)eventCode;

- (void)connectToAddress:(NSString *) hostname withPort:(int)trgport;
- (void)sendLogin:(NSString *) password;

- (void)sendPacket:(ECPacket *) packet;

- (void)setDelegate:(id) val;
- (id)delegate;

@end

