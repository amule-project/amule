#import <Cocoa/Cocoa.h>

@interface BaseItem : NSObject {
	NSString *m_text;
	
	NSString *m_view_id;
	
	NSMutableArray *m_subitems;
	int m_subitems_count;
	
	NSImage *m_icon;
}

+ (BaseItem *) initWithText:(NSString *)text withViewId:(NSString *) viewId;

- (void)activateView:(NSTabView *) view;

- (int)subitemsCount;
- (BaseItem *)itemAtIndex: (int) index;

- (void)addSubItem:(BaseItem *) item;

- (NSString *)text;
- (NSString *)viewId;
- (NSImage *)icon;

@end

@interface CategoryItemFactory : BaseItem
{
	
}

+ (BaseItem *)initFilesItem;
+ (BaseItem *)initNetworksItem;
+ (BaseItem *)initSearchItem;

@end

enum {
	RootItemNetwork = 0,
	RootItemFiles,
	RootItemSearch,
	
	RootItemLast
};


@interface amuleSourceListDataSource : NSObject
{
	BaseItem *g_root_items[RootItemLast];   
}

+ (amuleSourceListDataSource *)initWithData;

- (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item;
- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item;
- (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item;
- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;

@end

@interface CellWithIcon : NSTextFieldCell {
@private
    NSImage *m_icon;
}

- (NSImage *)icon;
- (void)setIcon:(NSImage *)icon;



@end

@interface SourceViewController : NSObject {
    IBOutlet id m_outline_view;
    IBOutlet id m_tab_view;
}

@end
