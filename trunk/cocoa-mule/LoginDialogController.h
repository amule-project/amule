#import <Cocoa/Cocoa.h>

@interface LoginDialogController : NSObject {
    IBOutlet id m_dlg;
	
	
    IBOutlet NSTextField *m_host_field;
    IBOutlet NSTextField *m_port_field;
    IBOutlet NSTextField *m_pass_field;

	bool m_dlg_result;
	NSString *m_host, *m_pass;
	int m_port;
}

- (bool)showDlg:(NSWindow *)window;

- (IBAction)closeCancel:(id)sender;
- (IBAction)closeOK:(id)sender;

@property (readonly) NSString *host;
@property (readonly) int port;
@property (readonly) NSString *pass;

@end
