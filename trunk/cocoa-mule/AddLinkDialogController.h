#import <Cocoa/Cocoa.h>

@interface AddLinkDialogController : NSObject {
    IBOutlet NSPopUpButton *m_cat;
    IBOutlet NSTextField *m_link;

    IBOutlet id m_dlg;

	bool m_dlg_result;
}

- (bool)showDlg:(NSWindow *)window;

- (IBAction)closeOK:(id)sender;

@end
