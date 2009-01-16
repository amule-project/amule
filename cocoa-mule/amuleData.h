#import <Cocoa/Cocoa.h>

#import "EC.h"

@interface amuleFile : NSObject {
	NSString *m_name;
	uint64_t m_size;

	MD5Data m_hash;
}

- (unsigned)hash;
- (NSString *)key;
- (BOOL)isEqual: (id) object;

- (NSString *)convertWithPrefix:(uint64_t)number;

@property (readonly) NSString *name;
@property (readonly) uint64_t size;

@end

@interface DownloadingFile : amuleFile {	
	int m_src_count;
	int m_non_current_src_count;
	int m_xfer_src_count;
	int m_a4af_src_count;
	
	uint64_t m_size_done;
	uint64_t m_size_xfer;
	
	int m_prio;
	
	int m_speed;
	
}

+ (id)createFromEC:(ECTagMD5 *) tag;

- (void)updateFromEC:(ECTagMD5 *) tag;

- (NSString *)prioToString:(int)prio;

@property (readonly) int src_count;
@property (readonly) int non_current_src_count;
@property (readonly) int xfer_src_count;
@property (readonly) int a4af_src_count;

@property (readonly) int speed;

@property (readonly) uint64_t size_done;
@property (readonly) uint64_t size_xfer;

@end

@interface SearchFile : amuleFile {
	int m_src_count;
}

+ (id)createFromEC:(ECTagMD5 *) tag;

- (void)updateFromEC:(ECTagMD5 *) tag;

@property (readonly) int src_count;

@end

@interface amuleFileSet : NSObject {
	NSMutableDictionary *m_file_dict;
	NSMutableArray *m_file_array;
	
	id m_gui_controller;
}

- (id)init;

- (int)count;

- (void)insertObject:(id)object;

- (id)objectAtIndex:(int)index;
- (id)objectForKey:(id)key;

- (void)removeAtIndex:(int)index;
- (void)removeAtKey:(id)key;

- (void)setGuiController:(id)controller;

- (void)reloadGui;

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

	amuleFileSet *m_downloads;
	amuleFileSet *m_shared;

	//
	// Search info
	//
	bool m_search_running;
	amuleFileSet *m_search_results;

	ECRemoteConnection *m_connection;
}

+ (id)initWithConnection:(ECRemoteConnection *) connection;

//
// Binding to EC
//
- (void)handlePacket:(ECPacket *) packet;
- (void)handleError;

- (void)handleDownloadQueueUpdate:(ECPacket *) packet;
- (void)handleStatusUpdate:(ECPacket *) packet;

- (void)startSearch:(NSString *)text searchType:(EC_SEARCH_TYPE)searchType
	minSize:(uint64_t)minSize maxSize:(uint64_t)maxSize avail:(uint32_t)avail;
- (void)stopSearch;

@property (readonly) amuleFileSet *downloads;
@property (readonly) amuleFileSet *shared;
@property (readonly) amuleFileSet *search_resuls;

@end
