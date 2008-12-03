#import <Cocoa/Cocoa.h>

#import "EC.h"

@interface amuleFile : NSObject {
	NSString *m_name;
	MD5Data m_hash;
}

- (unsigned)hash;
- (NSString *)key;
- (BOOL)isEqual: (id) object;

@end

@interface DownloadingFile : amuleFile {	
	int m_src_count;
	int m_non_current_src_count;
	int m_xfer_src_count;
	int m_a4af_src_count;
	
	uint64_t m_size;
	uint64_t m_size_done;
	uint64_t m_size_xfer;
	
	int m_speed;
	
}

+ (id)createFromEC:(ECTagMD5 *) tag;

- (void)updateFromEC:(ECTagMD5 *) tag;

@end


@interface amuleData : NSObject {

	//
	// Core status
	//
	uint32_t m_ed2k_id;
	bool m_ed2k_connected;
	bool m_kad_connected;
	NSString *m_ed2k_server;
	int m_down_speed;
	int m_up_speed;

	NSMutableDictionary *m_downloads;

	ECRemoteConnection *m_connection;
}

+ (id)initWithConnection:(ECRemoteConnection *) connection;

//
// Binding to EC
//
- (void)handlePacket:(ECPacket *) packet;

- (void)handleDownloadQueueUpdate:(ECPacket *) packet;
- (void)handleStatusUpdate:(ECPacket *) packet;

@end
