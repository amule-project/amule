#import "SearchViewController.h"

@implementation SearchViewController

- (IBAction)startSearch:(id)sender {
    
}

- (void)linkAmuleData:(amuleData *)amuledata {
	m_amuledata = amuledata;
	m_fileset = m_amuledata.search_resuls;
	[amuledata.search_resuls setGuiController:self];
}

@end
