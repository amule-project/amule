#import <Cocoa/Cocoa.h>

#import "EC.h"
#import "amuleData.h"

@interface AppController : NSObject {
    IBOutlet id m_main_tabview;
	
	IBOutlet id m_dload_controller;
	IBOutlet id m_search_controller;
	IBOutlet id m_shared_controller;
	
	IBOutlet id m_add_link_dlg;
	
	IBOutlet id m_connection_status_text;
	
	ECRemoteConnection *m_connection;
	amuleData *m_data;
	
	int m_daemon_pid;

	NSString *m_targetaddr;
	int m_targetport;
	NSString *m_corepass;
}
- (IBAction)show_Networks:(id)sender;
- (IBAction)show_Search:(id)sender;
- (IBAction)show_Sharing:(id)sender;
- (IBAction)show_Stats:(id)sender;
- (IBAction)show_Xfers:(id)sender;
- (IBAction)show_Preferences:(id)sender;
- (IBAction)show_About:(id)sender;

- (IBAction)addLink:(id)sender;

- (int)startDaemon;

- (bool)askCoreParams;

@end
