
#import "amuleData.h"


@implementation amuleData

+ (id)initWithConnection:(ECRemoteConnection *) connection {
	amuleData *obj = [[amuleData alloc] init];
	connection.delegate = obj;
	
	obj->m_connection = connection;
	
	return obj;
}

- (void)handlePacket:(ECPacket *) packet {
	switch(packet.opcode) {
		case EC_OP_STATS : {
			break;
		}
		case EC_OP_DLOAD_QUEUE: {
			[self handleDownloadQueueUpdate: packet];
			break;
		}
		default: {
			NSLog(@"[EC] packet with opcode %d not handled\n", packet.opcode);
		}
	}
}


- (void)handleDownloadQueueUpdate:(ECPacket *) packet {
	for (ECTag *t in packet.subtags) {
		//
		// FIXME: some kind of custom hash should be used to locate
		// object for given MD5.
		for (DownloadingFile *o in m_downloads) {
		}
	}
}

- (void)handleStatusUpdate:(ECPacket *) packet {
}

@end
