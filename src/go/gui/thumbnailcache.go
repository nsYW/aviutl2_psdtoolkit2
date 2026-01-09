package gui

import (
	"image"

	"github.com/golang-ui/nuklear/nk"
	"github.com/pkg/errors"
	"golang.org/x/image/draw"

	"psdtoolkit/imgmgr/editing"
	"psdtoolkit/nkhelper"
)

const (
	thumbnailSize = 48
	textureSize   = 512
)

// ThumbnailCache manages the thumbnail texture sheet for the GUI.
// It is owned by the GUI thread and should only be accessed from there.
type ThumbnailCache struct {
	sheet   *image.NRGBA
	texture *nkhelper.Texture
	images  []nk.Image

	// Track last thumbnails to detect changes
	lastThumbnails []*image.NRGBA
}

// NewThumbnailCache creates a new ThumbnailCache.
func NewThumbnailCache() *ThumbnailCache {
	return &ThumbnailCache{}
}

// Update rebuilds the thumbnail sheet if needed and returns the nk.Image slice.
func (c *ThumbnailCache) Update(items []editing.Item) ([]nk.Image, error) {
	if !c.needsUpdate(items) {
		return c.images, nil
	}

	c.sheet = image.NewNRGBA(image.Rect(0, 0, textureSize, textureSize))
	rects := make([]nk.Rect, len(items))
	var tx, ty int

	for i, item := range items {
		if item.Thumbnail == nil {
			continue
		}
		draw.Draw(
			c.sheet,
			image.Rect(tx, ty, tx+thumbnailSize, ty+thumbnailSize),
			item.Thumbnail,
			image.Pt(-(thumbnailSize-item.Thumbnail.Rect.Dx())/2, -(thumbnailSize-item.Thumbnail.Rect.Dy())/2),
			draw.Over,
		)
		rects[i] = nk.NkRect(float32(tx), float32(ty), thumbnailSize, thumbnailSize)
		tx += thumbnailSize
		if tx+thumbnailSize >= textureSize {
			tx = 0
			ty += thumbnailSize
		}
	}

	var err error
	if c.texture == nil {
		c.texture, err = nkhelper.NewTexture(c.sheet)
	} else {
		err = c.texture.Update(c.sheet)
	}
	if err != nil {
		return nil, errors.Wrap(err, "thumbnailcache: cannot create texture")
	}

	c.images = make([]nk.Image, len(items))
	for i, rect := range rects {
		c.images[i] = c.texture.SubImage(rect)
	}

	// Update tracking
	c.lastThumbnails = make([]*image.NRGBA, len(items))
	for i, item := range items {
		c.lastThumbnails[i] = item.Thumbnail
	}

	return c.images, nil
}

func (c *ThumbnailCache) needsUpdate(items []editing.Item) bool {
	if len(items) != len(c.lastThumbnails) {
		return true
	}
	for i, item := range items {
		if item.Thumbnail != c.lastThumbnails[i] {
			return true
		}
	}
	return false
}
