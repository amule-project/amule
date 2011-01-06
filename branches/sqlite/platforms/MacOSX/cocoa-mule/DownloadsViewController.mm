#import "DownloadsViewController.h"


@implementation DownloadsViewController

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
	id value = nil;
	NSString *columnId = [aTableColumn identifier];
	DownloadingFile *i = [m_fileset objectAtIndex:rowIndex];
//	if ( [columnId compare:@"filename"] == NSOrderedSame ) {
//		value = @"val-for-filename";
//	} else if ( [columnId compare:@"progress"] == NSOrderedSame ) {
//		value = @"val-for-progress";
//	} else if ( [columnId compare:@"size"] == NSOrderedSame ) {
//		value = @"val-for-name";
//	} else if ( [columnId compare:@"xferred"] == NSOrderedSame ) {
//		value = @"val-for-size";
//	} else if ( [columnId compare:@"completed"] == NSOrderedSame ) {
//		value = @"val-for-completed";
//	} else if ( [columnId compare:@"speed"] == NSOrderedSame ) {
//		value = @"val-for-speed";
//	} else if ( [columnId compare:@"prio"] == NSOrderedSame ) {
//		value = @"val-for-prio";
//	} else if ( [columnId compare:@"timerem"] == NSOrderedSame ) {
//		value = @"val-for-timerem";
//	} else if ( [columnId compare:@"lastcomp"] == NSOrderedSame ) {
//		value = @"val-for-lastcomp";
//	} else if ( [columnId compare:@"lastrx"] == NSOrderedSame ) {
//		value = @"val-for-lastrx";
//	} else {
//		value = @"ERROR: bad column id";
//	}
	if ( [columnId compare:@"filename"] == NSOrderedSame ) {
		value = i.name;
	} else if ( [columnId compare:@"progress"] == NSOrderedSame ) {
		value = @"progress-colored-bar";
	} else if ( [columnId compare:@"size"] == NSOrderedSame ) {
		value = [i convertWithPrefix: i.size];
	} else if ( [columnId compare:@"xferred"] == NSOrderedSame ) {
		value = [i convertWithPrefix: i.size_xfer];
	} else if ( [columnId compare:@"completed"] == NSOrderedSame ) {
		value = [i convertWithPrefix: i.size_done];
	} else if ( [columnId compare:@"speed"] == NSOrderedSame ) {
		value = ( i.speed ) ?
			[[i convertWithPrefix: i.speed] stringByAppendingString: @"/sec"] : @"";
	} else if ( [columnId compare:@"prio"] == NSOrderedSame ) {
		value = i.sprio;
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
//	return 3;
	return [m_fileset count];
}

- (void)linkAmuleData:(amuleData *)amuledata {
	m_amuledata = amuledata;
	m_fileset = m_amuledata.downloads;
	[amuledata.downloads setGuiController:self];
}

- (void)reload {
	[m_tableview reloadData];
}

- (void)awakeFromNib {
	[m_tableview setDelegate:self];
	[m_tableview setDataSource:self];
	
	//
	// load column status
	//
	for (NSTableColumn *c in [m_tableview tableColumns]) {
		NSString *columnId = [c identifier];
		NSString *keyWidth = [NSString stringWithFormat:@"DownloadView.Column_%@_Width", columnId];
		int width = [[NSUserDefaults standardUserDefaults] integerForKey:keyWidth];
		if ( width ) {
			NSLog(@"Column %@ setting width %d\n", columnId, width);
			[c setWidth:width];
		}
		NSString *keyHide = [NSString stringWithFormat:@"DownloadView.Column_%@_Hide", columnId];
		int hide = [[NSUserDefaults standardUserDefaults] integerForKey:keyHide];
		[c setHidden:hide];
	}
}

- (void)saveGui {
	for (NSTableColumn *c in [m_tableview tableColumns]) {
		NSString *columnId = [c identifier];

		NSString *keyWidth = [NSString stringWithFormat:@"DownloadView.Column_%@_Width", columnId];
		[[NSUserDefaults standardUserDefaults] setInteger:c.width forKey:keyWidth];

		NSString *keyHide = [NSString stringWithFormat:@"DownloadView.Column_%@_Hide", columnId];
		[[NSUserDefaults standardUserDefaults] setInteger:[c isHidden] forKey:keyHide];
	}
}

@end
