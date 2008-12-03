
#import "amuleData.h"

@implementation amuleFile


- (unsigned)hash {
	return m_hash.lo ^ m_hash.hi;
}

- (NSString *)key {
	NSString *s = [NSString stringWithFormat:@"%qx%qx", m_hash.hi, m_hash.lo];
	return s;
}

- (BOOL)isEqual: (id) object {
	if ([object isKindOfClass:[amuleFile class]]) {
		amuleFile *myobj = (amuleFile *)object;
		return ((myobj->m_hash.lo == m_hash.lo) && (myobj->m_hash.hi == m_hash.hi));
	} else {
		return false;
	}
}

@end

@implementation DownloadingFile

+ (id)createFromEC: (ECTagMD5 *) tag {
	DownloadingFile *obj = [[DownloadingFile alloc] init];
	
	obj->m_hash = [tag getMD5Data];
	
	ECTag *nametag = [tag tagByName: EC_TAG_PARTFILE_NAME];
	ECTagString *stag = (ECTagString *)nametag;
	obj->m_name = stag.stringValue;;
	
	return obj;
}

- (void)updateFromEC:(ECTagMD5 *) tag {
}

@end


@implementation amuleData

+ (id)initWithConnection:(ECRemoteConnection *) connection {
	amuleData *obj = [[amuleData alloc] init];
	connection.delegate = obj;
	
	obj->m_connection = connection;
	
	obj->m_downloads = [NSMutableDictionary dictionaryWithCapacity:256];
	 
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
		if ( [t isKindOfClass:[ECTagMD5 class]] ) {
			ECTagMD5 *tag = (ECTagMD5 *)t;
			ECTag *nametag = [tag tagByName: EC_TAG_PARTFILE_NAME];
			if ( nametag != nil ) {
				ECTagString *stag = (ECTagString *)nametag;
				NSString *str = stag.stringValue;
				printf("FILE: %s\n", [str UTF8String]);
				DownloadingFile *file = [DownloadingFile createFromEC:tag];
				[m_downloads setObject:file forKey:[file key]];
			} else {
				DownloadingFile *file = [m_downloads objectForKey:[tag stringKey]];
				[file updateFromEC:tag];
				printf("FILE: no-name-received\n");
			}
		} else {
			NSLog(@"[EC] bad tag type '%s'\n", [t class]);
		}
	}
}

- (void)handleStatusUpdate:(ECPacket *) packet {
}

@end
