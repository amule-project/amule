#import <Cocoa/Cocoa.h>

#import "amuleData.h"

@interface DownloadsViewController : NSObject {

	IBOutlet NSTableView *m_tableview;
	
	amuleFileSet *m_fileset;
}

// TableView datasource methods
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;

- (void)setFileSet:(amuleFileSet *)fileset;

- (void)reload;

@end
