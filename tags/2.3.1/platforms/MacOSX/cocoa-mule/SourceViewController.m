#import "SourceViewController.h"

@implementation BaseItem

+(BaseItem*)initWithText:(NSString *)text withViewId:(NSString *) viewId {
	BaseItem *node = [[BaseItem alloc] init];
	node->m_text = [NSString stringWithString:text];
	[node->m_text retain];
	
	node->m_subitems = [NSMutableArray arrayWithCapacity:16];
	[node->m_subitems retain];
	node->m_subitems_count = 0;
	
	node->m_view_id = viewId;
	if ( viewId != nil ) {
		[node->m_view_id retain];
	}       
	
	
//	NSString* imageName = [[NSBundle mainBundle] pathForResource:@"Client_Transfer" ofType:@"png"];
//	NSImage* imageObj = [[NSImage alloc] initWithContentsOfFile:imageName];
	NSString *imageName = @"Client_Transfer";
	NSImage* imageObj = [NSImage imageNamed:imageName];
	[imageObj retain];
	assert(imageObj);
	node->m_icon = imageObj;
	
	return node;
}

- (void)dealloc
{
	[m_text release];
	[m_subitems release];
	
	[super dealloc];
}

-(int)subitemsCount {
	return m_subitems_count;
}

- (NSString *)text {
	return m_text;
}

- (NSString *)viewId {
	return m_view_id;
}

- (NSImage *)icon {
	return m_icon;
}

- (BaseItem*)itemAtIndex: (int) index {
	if ( m_subitems_count ) {
		return [m_subitems objectAtIndex:index];
	}
	return nil;
}

- (void)activateView:(NSTabView *) view {
}

- (void)addSubItem:(BaseItem *) item {
	[m_subitems insertObject:item atIndex:m_subitems_count];
	m_subitems_count++;
}

@end

@implementation CategoryItemFactory

+ (BaseItem *)initFilesItem {
	BaseItem *it = [BaseItem initWithText:@"Files" withViewId:nil];
	
	BaseItem *all = [BaseItem initWithText:@"All files" withViewId:@"all"];
	BaseItem *dl = [BaseItem initWithText:@"Downloads" withViewId:@"downloads"];
	BaseItem *ul = [BaseItem initWithText:@"Uploads" withViewId:@"upload"];
	BaseItem *sh = [BaseItem initWithText:@"Shared" withViewId:@"shared"];
	
	[it addSubItem:all];
	[it addSubItem:dl];
	[it addSubItem:ul];
	[it addSubItem:sh];
	
	return it;
}

+ (BaseItem *)initNetworksItem {
	BaseItem *it = [BaseItem initWithText:@"Networks" withViewId:nil];
	return it;
}

+ (BaseItem *)initSearchItem {
	BaseItem *it = [BaseItem initWithText:@"Search" withViewId:nil];
	return it;
}


@end

@implementation amuleSourceListDataSource

+ (amuleSourceListDataSource *)initWithData {
	amuleSourceListDataSource *obj = [[amuleSourceListDataSource alloc] init];
	obj->g_root_items[RootItemFiles] = [CategoryItemFactory initFilesItem];
	obj->g_root_items[RootItemNetwork] = [CategoryItemFactory initNetworksItem];
	obj->g_root_items[RootItemSearch] = [CategoryItemFactory initSearchItem];
	
	return obj;
}

- (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
	if ( item == nil ) {
		return RootItemLast;
	}
	return [item subitemsCount];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
	if ( item == nil ) {
		return YES;
	}
	return [item subitemsCount] ? YES : NO;
}

- (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item
{
	if ( item == nil ) {
		return g_root_items[index];
	}
	return [item itemAtIndex: index];
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item 
{
	NSString *columnId = [tableColumn identifier];
	if ( [columnId compare:@"status"] == NSOrderedSame ) {
		return nil;
	}
	NSString *s = [item text];
	return s;
}
@end


@implementation CellWithIcon

- (id)init {
    self = [super init];
    if (self != nil) {
        [self setLineBreakMode:NSLineBreakByTruncatingTail];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    return self;
}

- (id)copyWithZone:(NSZone *)zone {
    CellWithIcon *result = [super copyWithZone:zone];
    if (result != nil) {
        result->m_icon = nil;
        [result setIcon:[self icon]];
    }
    return result;
}

- (void)dealloc {
    [m_icon release];
    [super dealloc];
}

- (NSImage *)icon {
    return m_icon;
}

- (void)setIcon:(NSImage *)icon {
    if (icon != m_icon) {
        [m_icon release];
        m_icon = [icon retain];
    }
}


#define PADDING_BEFORE_IMAGE 5.0
#define PADDING_BETWEEN_TITLE_AND_IMAGE 4.0


- (NSRect)imageRectForBounds:(NSRect)bounds {
    NSRect result = bounds;

    result.origin.x += PADDING_BEFORE_IMAGE;

	result.size = [m_icon size];
	
    return result;
}

- (NSRect)titleRectForBounds:(NSRect)bounds {
    NSAttributedString *text = [self attributedStringValue];
    NSRect result = bounds;
	NSRect icon_size;
	icon_size.size = [m_icon size];

    result.origin.x += PADDING_BEFORE_IMAGE + icon_size.size.width + PADDING_BETWEEN_TITLE_AND_IMAGE;

    if (text != nil) {
        result.size = [text size];
    } else {
        result.size = NSZeroSize;
    }

    CGFloat maxX = NSMaxX(bounds) - icon_size.size.height;
    CGFloat maxWidth = maxX - NSMinX(result);
    if (maxWidth < 0) {
		maxWidth = 0;
	}
    result.size.width = MIN(NSWidth(result), maxWidth);
    return result;
}

- (void)drawInteriorWithFrame:(NSRect)bounds inView:(NSView *)controlView {
	
    NSRect imageRect = [self imageRectForBounds:bounds];

	[m_icon setFlipped:[controlView isFlipped]];
	[m_icon drawInRect:imageRect fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
	
    NSRect titleRect = [self titleRectForBounds:bounds];
    NSAttributedString *title = [self attributedStringValue];
    if ([title length] > 0) {
        [title drawInRect:titleRect];
    }
	
}

@end

@implementation SourceViewController

- (void)awakeFromNib {
	[m_outline_view setDelegate:self];
	
	id datasource = [amuleSourceListDataSource initWithData];

	[m_outline_view setDataSource:datasource];
	[m_outline_view reloadData];

	int i;
	for(i = 0; i < [m_outline_view numberOfRows];i++) {
		id item = [m_outline_view itemAtRow:i];
		[m_outline_view expandItem:item];
	}
	
}

- (void)outlineViewSelectionDidChange:(NSNotification *)notification {
	int selRow = [m_outline_view selectedRow];
	id item = [m_outline_view itemAtRow:selRow];
	id viewId = [item viewId];
	NSLog(@"select tab=%@\n", viewId);
	
	[m_tab_view selectTabViewItemWithIdentifier:viewId]; 
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldSelectItem:(id)item {
	id viewId = [item viewId];
	return (viewId == nil) ? NO : YES;
}

- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item {
//    if (tableColumn && [[tableColumn identifier] isEqualToString:@"status"]) {
	NSImage *icon = [item icon];
	if ( [cell isKindOfClass:[CellWithIcon class]] ) {
		[cell setIcon:icon];
    }
}

@end
