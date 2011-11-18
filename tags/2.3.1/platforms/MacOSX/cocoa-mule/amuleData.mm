
#import "amuleData.h"

@implementation amuleFile

@synthesize name = m_name;
@synthesize size = m_size;

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

- (NSString *)convertWithPrefix:(uint64_t)number {
	NSString *res = nil;
	double dnum = number;
	if ( number < 1024 ) { // bytes
		res = [NSString stringWithFormat:@"%llu bytes", number]; 
	} else if ( number < 1024*1024 ) { // K
		res = [NSString stringWithFormat:@"%0.2f Kb", dnum / 1024.0]; 
	} else if ( number < 1024*1024*1024 ) { // M
		res = [NSString stringWithFormat:@"%.2f Mb", dnum / 1048576.0]; 
	} else {
		res = [NSString stringWithFormat:@"%.2f Gb", dnum / 1073741824.0]; 
	}
	return res;
}


@end

@implementation DownloadingFile

@synthesize src_count = m_src_count;
@synthesize non_current_src_count = m_non_current_src_count;
@synthesize xfer_src_count = m_xfer_src_count;
@synthesize a4af_src_count = m_a4af_src_count;

@synthesize speed = m_speed;

@synthesize size_done = m_size_done;
@synthesize size_xfer = m_size_xfer;

@synthesize prio = m_prio;

@dynamic sprio;

- (NSString *)sprio {
	return [self prioToString: m_prio];
}


+ (id)createFromEC: (ECTagMD5 *) tag {
	DownloadingFile *obj = [[DownloadingFile alloc] init];
	
	obj->m_hash = [tag getMD5Data];
	
	ECTag *nametag = [tag tagByName: EC_TAG_PARTFILE_NAME];
	ECTagString *stag = (ECTagString *)nametag;
	obj->m_name = stag.stringValue;;
	
	obj->m_size = [tag tagInt64ByName: EC_TAG_PARTFILE_SIZE_FULL];
	NSLog(@"[EC]: added partfile [%@]\n", obj->m_name);
	[obj updateFromEC:tag];
	
	return obj;
}

- (void)updateFromEC:(ECTagMD5 *) tag {
	NSLog(@"[EC]: updating partfile [%@]\n", m_name);
	
	m_size_done = [tag tagInt64ByName: EC_TAG_PARTFILE_SIZE_DONE];
	m_size_xfer = [tag tagInt64ByName:EC_TAG_PARTFILE_SIZE_XFER];
	
	m_speed = [tag tagInt64ByName:EC_TAG_PARTFILE_SPEED];
	
	m_src_count = [tag tagInt64ByName: EC_TAG_PARTFILE_SOURCE_COUNT];
	m_non_current_src_count = [tag tagInt64ByName: EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT];
	m_xfer_src_count = [tag tagInt64ByName: EC_TAG_PARTFILE_SOURCE_COUNT_XFER];
	m_a4af_src_count = [tag tagInt64ByName: EC_TAG_PARTFILE_SOURCE_COUNT_A4AF];

	m_prio = [tag tagInt64ByName:EC_TAG_PARTFILE_PRIO];
	if ( m_prio > 10 ) {
		m_auto_prio = true;
		m_prio -= 10;
	} else {
		m_auto_prio = false;
	}
}

- (NSString *)prioToString:(int)prio {
	NSString *s = nil;
	switch (prio) {
		case 0:
			s = @"Low";
			break;
		case 1:
			s = @"Normal";
			break;
		case 2:
			s = @"High";
			break;
		case 3:
			s = @"Very High";
			break;
		case 4:
			s = @"Very Low";
			break;
		case 5:
			s = @"Auto";
			break;
		case 6:
			s = @"PowerShare";
			break;
		default:
			break;
	}
	if ( m_auto_prio ) {
		s = [s stringByAppendingString:@" (auto)"];
	}
	return s;
}


@end

@implementation SearchFile

@synthesize src_count = m_src_count;
@synthesize complete_src_count = m_complete_src_count;

+ (id)createFromEC: (ECTagMD5 *) tag {
	SearchFile *obj = [[SearchFile alloc] init];
	
	obj->m_hash = [tag getMD5Data];

	ECTag *nametag = [tag tagByName: EC_TAG_PARTFILE_NAME];
	ECTagString *stag = (ECTagString *)nametag;
	obj->m_name = stag.stringValue;;

	obj->m_size = [tag tagInt64ByName: EC_TAG_PARTFILE_SIZE_FULL];

	obj->m_known = ([tag tagByName: EC_TAG_KNOWNFILE] == nil) ? false : true;
	
	[obj updateFromEC:tag];
	
	return obj;
}

- (void)updateFromEC:(ECTagMD5 *) tag {
	m_src_count = [tag tagInt64ByName: EC_TAG_PARTFILE_SOURCE_COUNT];
	m_complete_src_count = [tag tagInt64ByName: EC_TAG_PARTFILE_SOURCE_COUNT_XFER];

}
@end

@implementation SharedFile

@synthesize size_xfer = m_size_xfer;
@synthesize size_xfer_all = m_size_xfer_all;

@synthesize req_count = m_req_count;
@synthesize req_count_all = m_req_count_all;

@synthesize accept_count = m_accept_count;
@synthesize accept_count_all = m_accept_count_all;

@synthesize prio = m_prio;

@dynamic sprio;

- (NSString *)sprio {
	return [self prioToString: m_prio];
}

+ (id)createFromEC: (ECTagMD5 *) tag {
	SharedFile *obj = [[SharedFile alloc] init];

	obj->m_hash = [tag getMD5Data];
	
	ECTag *nametag = [tag tagByName: EC_TAG_PARTFILE_NAME];
	ECTagString *stag = (ECTagString *)nametag;
	obj->m_name = stag.stringValue;;
	
	obj->m_size = [tag tagInt64ByName: EC_TAG_PARTFILE_SIZE_FULL];
		
	[obj updateFromEC:tag];
	
	return obj;
}

- (NSString *)prioToString:(int)prio {
	NSString *s = nil;
	switch (prio) {
		case 0:
			s = @"Low";
			break;
		case 1:
			s = @"Normal";
			break;
		case 2:
			s = @"High";
			break;
		case 3:
			s = @"Very High";
			break;
		case 4:
			s = @"Very Low";
			break;
		case 5:
			s = @"Auto";
			break;
		case 6:
			s = @"PowerShare";
			break;
		default:
			break;
	}
	if ( m_auto_prio ) {
		s = [s stringByAppendingString:@" (auto)"];
	}
	return s;
}


- (void)updateFromEC:(ECTagMD5 *) tag {
	m_req_count = [tag tagInt64ByName: EC_TAG_KNOWNFILE_REQ_COUNT];
	m_req_count_all = [tag tagInt64ByName: EC_TAG_KNOWNFILE_REQ_COUNT_ALL];
	m_accept_count = [tag tagInt64ByName: EC_TAG_KNOWNFILE_ACCEPT_COUNT];
	m_accept_count_all = [tag tagInt64ByName: EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL];
	
	m_size_xfer = [tag tagInt64ByName: EC_TAG_KNOWNFILE_XFERRED];
	m_size_xfer_all = [tag tagInt64ByName: EC_TAG_KNOWNFILE_XFERRED_ALL];
	
	m_prio = [tag tagInt64ByName:EC_TAG_PARTFILE_PRIO];
	if ( m_prio > 10 ) {
		m_auto_prio = true;
		m_prio -= 10;
	} else {
		m_auto_prio = false;
	}
	
}

@end

@implementation amuleFileSet

- (id)init {
    if ((self = [super init])) {
		m_file_dict = [NSMutableDictionary dictionaryWithCapacity:256];
		m_file_array = [[NSMutableArray alloc] init];
	}
	return self;
}

- (int)count {
	return [m_file_dict count];
}

- (void)insertObject:(id)object {
	id objKey = [object key];
	[m_file_dict setObject:object forKey:objKey];
	[m_file_array addObject:object];
	
	[self reloadGui];
}

- (void)reloadGui {
	if ([m_gui_controller respondsToSelector:@selector(reload:)]) {
		[m_gui_controller performSelector:@selector(reload:)];
	} else {
		NSLog(@"Internal error: gui controller doesnt respond to 'reload'\n");
	}
//	if ([m_gui_controller respondsToSelector:[m_gui_controller reload]) {
//		[m_gui_controller performSelector:@selector(reload)];
//	} else {
//		NSLog(@"Internal error: gui controller doesnt respond to 'reload'\n");
//	}
}

- (id)objectAtIndex:(int)index {
	return [m_file_array objectAtIndex:index];
}

- (id)objectForKey:(id)key {
	return [m_file_dict objectForKey:key];
}

- (void)removeAtIndex:(int)index {
	id curr_obj = [m_file_array objectAtIndex:index];
	id key = [curr_obj key];
	[m_file_dict removeObjectForKey:key];
	[m_file_array removeObjectAtIndex:index];

	[self reloadGui];
}

- (void)removeAtKey:(id)key {
	[m_file_dict removeObjectForKey:key];
	int index = -1;
	for (int i = 0; i < [m_file_array count]; i++) {
		id curr_obj = [m_file_array objectAtIndex:i];
		if ([curr_obj key] == key) {
			index = i;
			break;
		}
	}
	if ( index != -1 ) {
		[m_file_array removeObjectAtIndex:index];
	} else {
		NSLog(@"Internal error: object not found in array\n");
	}

	[self reloadGui];
}

- (void)setGuiController:(id)controller {
	m_gui_controller = controller;
}

@end

@implementation amuleData

@synthesize downloads = m_downloads;
@synthesize shared = m_shared;
@synthesize search_resuls = m_search_results;

+ (id)initWithConnection:(ECRemoteConnection *) connection {
	amuleData *obj = [[amuleData alloc] init];
	connection.delegate = obj;
	
	obj->m_connection = connection;
	
	obj->m_downloads = [[amuleFileSet alloc] init];
	obj->m_shared = [[amuleFileSet alloc] init];
	obj->m_search_results = [[amuleFileSet alloc] init];
	 
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
		case EC_OP_SHARED_FILES: {
			[self handleSharedFilesUpdate: packet];
			break;
		}
		case EC_OP_SEARCH_RESULTS: {
			[self handleSearchUpdate: packet];
			break;
		}
		default: {
			NSLog(@"[EC] packet with opcode %d not handled\n", packet.opcode);
		}
	}
}

- (void)handleError {
		NSRunCriticalAlertPanel(@"Daemon communication error", 
						@"Connection with core daemon (amuled) failed",
						@"Quit", nil,nil);
		exit(-1);
}

- (void)handleDownloadQueueUpdate:(ECPacket *) packet {
	for (ECTag *t in packet.subtags) {
		if ( [t isKindOfClass:[ECTagMD5 class]] ) {
			ECTagMD5 *tag = (ECTagMD5 *)t;
			if ( [tag tagCount] == 0 ) {
				//
				// Only hash here - indication to remove the object
				//
				[m_downloads removeAtKey:[tag stringKey]];
			} else {
				NSLog(@"[EC] filehash=[%@]\n", [tag stringKey]);
				DownloadingFile *file = [m_downloads objectForKey:[tag stringKey]];
				if ( file == nil ) {
					file = [DownloadingFile createFromEC:tag];
					[m_downloads insertObject:file];
				} else {
					DownloadingFile *file = [m_downloads objectForKey:[tag stringKey]];
					[file updateFromEC:tag];
				}
			}
		} else {
			NSLog(@"[EC] bad tag type '%s'\n", [t class]);
		}
	}
}

- (void)handleSharedFilesUpdate:(ECPacket *) packet {
	for (ECTag *t in packet.subtags) {
		if ( [t isKindOfClass:[ECTagMD5 class]] ) {
			ECTagMD5 *tag = (ECTagMD5 *)t;
			if ( [tag tagCount] == 0 ) {
				//
				// Only hash here - indication to remove the object
				//
				[m_shared removeAtKey:[tag stringKey]];
			} else {
				NSLog(@"[EC] filehash=[%@]\n", [tag stringKey]);
				SharedFile *file = [m_shared objectForKey:[tag stringKey]];
				if ( file == nil ) {
					file = [SharedFile createFromEC:tag];
					[m_shared insertObject:file];
				} else {
					SharedFile *file = [m_shared objectForKey:[tag stringKey]];
					[file updateFromEC:tag];
				}
			}
		} else {
			NSLog(@"[EC] bad tag type '%s'\n", [t class]);
		}
	}
}

- (void)handleSearchUpdate:(ECPacket *) packet {
	for (ECTag *t in packet.subtags) {
		if ( [t isKindOfClass:[ECTagMD5 class]] ) {
			ECTagMD5 *tag = (ECTagMD5 *)t;
			SearchFile *file = [m_search_results objectForKey:[tag stringKey]];
			if ( file == nil ) {
				file = [SearchFile createFromEC:tag];
				[m_search_results insertObject:file];
			} else {
				SearchFile *file = [m_search_results objectForKey:[tag stringKey]];
				[file updateFromEC:tag];
			}
		}
	}
}

- (void)handleStatusUpdate:(ECPacket *) packet {
}

- (void)startSearch:(NSString *)text searchType:(EC_SEARCH_TYPE)searchType
	minSize:(uint64_t)minSize maxSize:(uint64_t)maxSize avail:(uint32_t)avail {

	ECPacket *packet = [ECPacket packetWithOpcode:EC_OP_SEARCH_START];

	ECTagInt8 *searchtag = [ECTagInt8 tagFromInt8:searchType withName:EC_TAG_SEARCH_TYPE];

	[searchtag.subtags addObject:[ECTagString tagFromString:text withName:EC_TAG_SEARCH_NAME]];
	if ( minSize ) {
		[searchtag.subtags addObject:[ECTagInt64 tagFromInt64:minSize withName:EC_TAG_SEARCH_MIN_SIZE]];
	}
	if ( maxSize ) {
		[searchtag.subtags addObject:[ECTagInt64 tagFromInt64:maxSize withName:EC_TAG_SEARCH_MAX_SIZE]];
	}
	if ( avail ) {
		[searchtag.subtags addObject:[ECTagInt32 tagFromInt32:avail withName:EC_TAG_SEARCH_AVAILABILITY]];
	}
	
	[packet.subtags addObject:searchtag];
	
	[m_connection sendPacket:packet];
	
	m_search_running = true;
}

- (void)stopSearch {
	ECPacket *packet = [ECPacket packetWithOpcode:EC_OP_SEARCH_STOP];

	[m_connection sendPacket:packet];
	
	m_search_running = false;
}

@end
