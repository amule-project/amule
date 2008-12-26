#import <Cocoa/Cocoa.h>

#import "amuleData.h"

@interface DownloadsViewController : NSObject {

	IBOutlet NSTableView *m_tableview;
}

// TableView datasource methods
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;

@end
