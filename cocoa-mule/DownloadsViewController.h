#import <Cocoa/Cocoa.h>

#import "amuleData.h"

@interface DownloadsViewController : NSObject {

	IBOutlet NSTableView *m_tableview;
	
	amuleFileSet *m_fileset;
	amuleData *m_amuledata;
}

- (void)saveGui;

// TableView datasource methods
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;

- (void)linkAmuleData:(amuleData *)amuledata;

- (void)reload;

@end
