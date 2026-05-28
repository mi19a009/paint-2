// Copyright (C) 2026 Taichi Murakami.
#import "ImageView.h"
#define ZOOM_MAXIMUM            10.00
#define ZOOM_MINIMUM            0.01
#define ZOOM_CLAMP(zoom)        MIN(MAX((zoom), ZOOM_MINIMUM), ZOOM_MAXIMUM)

@interface ImageView () {
	NSSize     _frameSize;
	NSImage   *_image;
	NSInteger  _imageIndex;
	CGImageRef _imageRef;
	CGFloat    _zoom;
}

@end
@implementation ImageView

// 現在のビューに画像を描画します。
- (void)drawRect:(NSRect)dirtyRect {
	[super drawRect:dirtyRect];
	CGContextRef contextRef = [[NSGraphicsContext currentContext] CGContext];

	if (contextRef) {
		if (_imageRef) {
			const CGRect imageRect = {{ 0, 0 }, _frameSize };
			CGContextDrawImage(contextRef, imageRect, _imageRef);
		}
	}
}

// NSImage を設定します。
- (NSImage *)image {
	return _image;
}

// NSImage に含まれる画像の目録を設定します。
- (NSInteger)imageIndex {
	return _imageIndex;
}

// インスタンスを初期化します。
- (instancetype)init {
	self = [super init];
	[self initialize];
	return self;
}

// インスタンスを初期化します。
- (instancetype)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];
	[self initialize];
	return self;
}

// プロパティに既定値を設定します。
- (void)initialize {
	_zoom = 1.0;
}

// NSImage を設定します。
- (void)setImage:(NSImage *)image {
	_image = image;
	[self updateImageRef];
	[self updateFrameSize];
	[self setNeedsDisplay:YES];
}

// NSImage に含まれる画像の目録を設定します。
- (void)setImageIndex:(NSInteger)imageIndex {
	_imageIndex = imageIndex;
	[self updateImageRef];
	[self updateFrameSize];
	[self setNeedsDisplay:YES];
}

// 拡大率を設定します。
- (void)setZoom:(CGFloat)zoom {
	_zoom = ZOOM_CLAMP(zoom);
	[self updateFrameSize];
}

// 現在のビューの大きさを決定します。
- (void)updateFrameSize {
	if (_imageRef) {
		_frameSize.width = CGImageGetWidth(_imageRef) * _zoom;
		_frameSize.height = CGImageGetHeight(_imageRef) * _zoom;
	} else {
		_frameSize.width = 0;
		_frameSize.height = 0;
	}

	[self setFrameSize:_frameSize];
}

// NSImage から CGImageRef を取得します。
- (void)updateImageRef {
	_imageRef = NULL;

	if (_image) {
		NSArray<NSImageRep *> *imageReps = [_image representations];
		
		if ((0 <= _imageIndex) && (_imageIndex < [imageReps count])) {
			NSImageRep *imageRep = [imageReps objectAtIndex:_imageIndex];
			NSRect imageRect = { 0, 0, [imageRep pixelsWide], [imageRep pixelsHigh]};
			_imageRef = [_image CGImageForProposedRect:&imageRect context:nil hints:nil];
		}
	}
}

// 拡大率を取得します。
- (CGFloat)zoom {
	return _zoom;
}
@end
