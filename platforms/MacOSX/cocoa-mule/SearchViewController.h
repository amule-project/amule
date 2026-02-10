#import <Cocoa/Cocoa.h>

#import "amuleData.h"

@interface SearchViewController : NSObject {
    IBOutlet NSOutlineView *m_result_list;
    IBOutlet NSTextField *m_search_text;

	amuleFileSet *m_fileset;
	amuleData *m_amuledata;
}

- (IBAction)startSearch:(id)sender;

- (void)linkAmuleData:(amuleData *)amuledata;

@end
