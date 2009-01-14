#import "LoginDialogController.h"

@implementation LoginDialogController

@synthesize host = m_host;
@synthesize pass = m_pass;
@synthesize port = m_port;

- (bool)showDlg:(NSWindow *)window {
	if ( m_dlg == nil ) {
		[NSBundle loadNibNamed: @"LoginDialog" owner: self];
	}

	NSString *default_host = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastTargetHost"];
	if ( default_host != nil ) {
		[m_host_field setStringValue: default_host];
	}
	
	NSString *default_port = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastTargetPort"];
	if ( default_port != nil ) {
		[m_port_field setStringValue:[[NSUserDefaults standardUserDefaults] stringForKey:@"LastTargetPort"]];
	}
	[NSApp beginSheet: m_dlg modalForWindow: window
		modalDelegate: nil didEndSelector: nil contextInfo: nil];
					
	[NSApp runModalForWindow: m_dlg];
	
	[NSApp endSheet: m_dlg];

    [m_dlg orderOut: self];
	
	return m_dlg_result;
}


- (IBAction)closeCancel:(id)sender {
	m_dlg_result = false;
    [NSApp stopModal];
}

- (IBAction)closeOK:(id)sender {
	m_dlg_result = true;
	
	m_host = [m_host_field stringValue];
	NSString *port = [m_port_field stringValue];
	m_port = [port intValue];
	m_pass = [m_pass_field stringValue];
	
    [NSApp stopModal];
}


@end
