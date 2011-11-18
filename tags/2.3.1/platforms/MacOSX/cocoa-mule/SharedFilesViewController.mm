#import "SharedFilesViewController.h"

@implementation SharedFilesViewController

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
	id value = nil;
	NSString *columnId = [aTableColumn identifier];
	SharedFile *i = [m_fileset objectAtIndex:rowIndex];
	if ( [columnId compare:@"filename"] == NSOrderedSame ) {
		value = i.name;
	} else if ( [columnId compare:@"size"] == NSOrderedSame ) {
		value = [i convertWithPrefix: i.size];
	} else if ( [columnId compare:@"xfer"] == NSOrderedSame ) {
		value = [i convertWithPrefix: i.size_xfer];
	} else if ( [columnId compare:@"xfer_all"] == NSOrderedSame ) {
		value = [i convertWithPrefix: i.size_xfer_all];
	} else if ( [columnId compare:@"prio"] == NSOrderedSame ) {
		value = i.sprio;
	} else {
		value = @"ERROR: bad column id";
	}
	return value;
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView {
	return [m_fileset count];
}

- (void)linkAmuleData:(amuleData *)amuledata {
	m_amuledata = amuledata;
	m_fileset = m_amuledata.shared;
	[amuledata.shared setGuiController:self];
}

- (void)reload {
	[m_table_view reloadData];
}

- (void)awakeFromNib {
	[m_table_view setDelegate:self];
	[m_table_view setDataSource:self];
	
	//
	// load column status
	//
}

@end
