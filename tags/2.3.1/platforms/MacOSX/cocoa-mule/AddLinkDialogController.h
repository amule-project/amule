#import <Cocoa/Cocoa.h>

@interface AddLinkDialogController : NSObject {
    IBOutlet NSPopUpButton *m_cat;
    IBOutlet NSTextField *m_link;

    IBOutlet id m_dlg;

	bool m_dlg_result;
	
	NSString *m_link_val;
}

- (bool)showDlg:(NSWindow *)window;

- (IBAction)closeOK:(id)sender;

- (NSString *)link;

@end
