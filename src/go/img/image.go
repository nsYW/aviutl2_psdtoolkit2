package img

import (
	"context"
	"image"
	"time"

	"github.com/disintegration/gift"
	"github.com/oov/downscale"
	"github.com/oov/psd/composite"
	"github.com/pkg/errors"

	"psdtoolkit/warn"
)

type Flip int

const (
	FlipNone Flip = iota
	FlipX
	FlipY
	FlipXY
)

// ScaleQuality represents the quality of image downscaling
type ScaleQuality int

const (
	// ScaleQualityFast uses nearest neighbor for fast but lower quality downscaling
	ScaleQualityFast ScaleQuality = iota
	// ScaleQualityBeautiful uses gamma-corrected area averaging for high quality downscaling
	ScaleQualityBeautiful
)

type Toucher interface {
	Touch()
	LastAccess() time.Time
}

type Image struct {
	FilePath *string
	FileHash uint32
	Toucher  Toucher

	PSD    *composite.Tree
	image  *image.NRGBA
	Layers *LayerManager

	InitialLayerState *string

	Modified bool

	Scale        float32
	ScaleQuality ScaleQuality
	OffsetX      int
	OffsetY      int

	// scaledImages caches downscaled images by quality
	// Key is ScaleQuality, value is the downscaled image at img.Scale
	scaledImages map[ScaleQuality]*image.NRGBA
	scaledScale  float32 // The scale value when scaledImages was generated

	PFV *PFV
}

func (img *Image) Touch() {
	img.Toucher.Touch()
}

func (img *Image) LastAccess() time.Time {
	return img.Toucher.LastAccess()
}

func (img *Image) Clone() *Image {
	r := *img
	r.image = nil
	return &r
}

func (img *Image) FlipX() bool {
	return img.Layers.Flip == FlipX || img.Layers.Flip == FlipXY
}

func (img *Image) FlipY() bool {
	return img.Layers.Flip == FlipY || img.Layers.Flip == FlipXY
}

func (img *Image) SetFlipX(v bool) bool {
	if (img.Layers.Flip&FlipX != 0) == v {
		return false
	}
	f := img.Layers.Flip
	if v {
		f |= FlipX
	} else {
		f &= ^FlipX
	}
	img.Layers.SetFlip(f)
	img.Layers.Flip = f
	return true
}

func (img *Image) SetFlipY(v bool) bool {
	if (img.Layers.Flip&FlipY != 0) == v {
		return false
	}
	f := img.Layers.Flip
	if v {
		f |= FlipY
	} else {
		f &= ^FlipY
	}
	img.Layers.SetFlip(f)
	return true
}

func (img *Image) ScaledCanvasRect() image.Rectangle {
	r := img.PSD.CanvasRect
	r.Max.X = r.Min.X + int(float32(r.Dx())*img.Scale+0.5)
	r.Max.Y = r.Min.Y + int(float32(r.Dy())*img.Scale+0.5)
	if r.Dx() < 1 {
		r.Max.X = r.Min.X + 1
	}
	if r.Dy() < 1 {
		r.Max.Y = r.Min.Y + 1
	}
	return r
}

func (img *Image) Render(ctx context.Context) (*image.NRGBA, error) {
	return img.RenderWithScale(ctx, float64(img.Scale), img.ScaleQuality)
}

// RenderWithScale renders the image at a specific scale with the given quality.
// This is the main rendering entry point that handles both PSD rendering and downscaling.
func (img *Image) RenderWithScale(ctx context.Context, scale float64, quality ScaleQuality) (*image.NRGBA, error) {
	var err error
	if img.image == nil {
		img.image = image.NewNRGBA(img.PSD.CanvasRect)
		err = img.PSD.Renderer.Render(ctx, img.image)
		// Clear scaled cache on initial render
		img.scaledImages = nil
	} else {
		err = img.PSD.Renderer.RenderDiff(ctx, img.image)
		// TODO: Phase 2 will add differential downscaling here
		// For now, invalidate entire scaled cache on any change
		img.scaledImages = nil
	}
	if err != nil {
		return nil, errors.Wrap(err, "img: render failed")
	}
	img.Modified = false

	nrgba := img.image

	// Handle downscaling if scale < 1
	if scale < 1 {
		// Calculate scaled rect
		r := img.PSD.CanvasRect
		r.Max.X = r.Min.X + int(float64(r.Dx())*scale+0.5)
		r.Max.Y = r.Min.Y + int(float64(r.Dy())*scale+0.5)
		if r.Dx() < 1 {
			r.Max.X = r.Min.X + 1
		}
		if r.Dy() < 1 {
			r.Max.Y = r.Min.Y + 1
		}

		// Check if we need to reset cache (scale changed)
		if img.scaledScale != float32(scale) {
			img.scaledImages = nil
			img.scaledScale = float32(scale)
		}

		// Check cache
		if img.scaledImages == nil {
			img.scaledImages = make(map[ScaleQuality]*image.NRGBA)
		}
		if cached, ok := img.scaledImages[quality]; ok {
			nrgba = cached
		} else {
			// Perform downscaling
			tmp := image.NewNRGBA(r)
			switch quality {
			case ScaleQualityFast:
				if err = downscale.NRGBAFast(ctx, tmp, img.image); err != nil {
					return nil, errors.Wrap(err, "img: downscale failed")
				}
			case ScaleQualityBeautiful:
				if err = downscale.NRGBAGamma(ctx, tmp, img.image, 2.2); err != nil {
					return nil, errors.Wrap(err, "img: downscale failed")
				}
			}
			img.scaledImages[quality] = tmp
			nrgba = tmp
		}
	}

	// Apply flip
	f := img.Layers.Flip
	if f != FlipNone {
		tmp := image.NewNRGBA(nrgba.Rect)
		g := gift.New()
		if f == FlipX || f == FlipXY {
			g.Add(gift.FlipHorizontal())
		}
		if f == FlipY || f == FlipXY {
			g.Add(gift.FlipVertical())
		}
		g.Draw(tmp, nrgba)
		nrgba = tmp
	}
	return nrgba, nil
}

func (img *Image) Serialize() (string, error) {
	s, err := img.Layers.Serialize()
	if err != nil {
		return "", errors.Wrap(err, "Image.Serialize: failed to serialize")
	}
	return "L." + itoa(int(img.Layers.Flip)) + " " + s, nil
}

func (img *Image) Deserialize(s string) (bool, error) {
	m, err := img.Layers.Deserialize(s, img.PFV)
	if err != nil {
		return false, err
	}
	return m, nil
}

// TODO: faview selected item state

// ViewState holds per-image view settings for restoration
type ViewState struct {
	Zoom    float64 `json:"zoom,omitempty"`
	ScrollX float64 `json:"scrollX,omitempty"` // Relative position 0.0-1.0
	ScrollY float64 `json:"scrollY,omitempty"` // Relative position 0.0-1.0
}

type ProjectState struct {
	Version   int
	FilePath  string
	Layer     map[string]SerializedData
	PFV       PFVSerializedData
	ViewState *ViewState `json:"viewState,omitempty"` // May be nil for old data
}

func (img *Image) SerializeProject() *ProjectState {
	return &ProjectState{
		Version:  1,
		FilePath: *img.FilePath,
		Layer:    img.Layers.SerializeSafe(),
		PFV:      img.PFV.Serialize(),
	}
}

func (img *Image) DeserializeProject(state *ProjectState) (warn.Warning, error) {
	var wr warn.Warning
	if w, err := img.Layers.DeserializeSafe(state.Layer); err != nil {
		return wr, errors.Wrap(err, "Image.DeserializeProject: failed to deserialize")
	} else if w != nil {
		wr = append(wr, w...)
	}
	if w, err := img.PFV.Deserialize(state.PFV); err != nil {
		return wr, errors.Wrap(err, "Image.DeserializeProject: failed to deserialize")
	} else if w != nil {
		wr = append(wr, w...)
	}
	return wr, nil
}
