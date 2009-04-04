#import <Cocoa/Cocoa.h>

#import "amuleData.h"

@interface SharedFilesViewController : NSObject {
    IBOutlet NSTableView *m_table_view;

	amuleFileSet *m_fileset;
	amuleData *m_amuledata;
}

// TableView datasource methods
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;

- (void)linkAmuleData:(amuleData *)amuledata;

- (void)reload;

@end
