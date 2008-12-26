#import "DownloadsViewController.h"


@implementation DownloadsViewController

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
	id value = nil;
	NSString *columnId = [aTableColumn identifier];
	if ( [columnId compare:@"filename"] == NSOrderedSame ) {
		value = @"val-for-filename";
	} else if ( [columnId compare:@"progress"] == NSOrderedSame ) {
		value = @"val-for-progress";
	} else if ( [columnId compare:@"size"] == NSOrderedSame ) {
		value = @"val-for-name";
	} else if ( [columnId compare:@"xferred"] == NSOrderedSame ) {
		value = @"val-for-size";
	} else if ( [columnId compare:@"completed"] == NSOrderedSame ) {
		value = @"val-for-completed";
	} else if ( [columnId compare:@"speed"] == NSOrderedSame ) {
		value = @"val-for-speed";
	} else if ( [columnId compare:@"prio"] == NSOrderedSame ) {
		value = @"val-for-prio";
	} else if ( [columnId compare:@"timerem"] == NSOrderedSame ) {
		value = @"val-for-timerem";
	} else if ( [columnId compare:@"lastcomp"] == NSOrderedSame ) {
		value = @"val-for-lastcomp";
	} else if ( [columnId compare:@"lastrx"] == NSOrderedSame ) {
		value = @"val-for-lastrx";
	} else {
		value = @"ERROR: bad column id";
	}
	return value;
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView {
	return 3;
}

- (void)awakeFromNib {
	[m_tableview setDelegate:self];
	[m_tableview setDataSource:self];
}

@end
