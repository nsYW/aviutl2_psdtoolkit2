package gui

import (
	"psdtoolkit/img"
)

func updateRenderedImage(g *GUI, im *img.Image) {
	if g.cancelRender != nil {
		g.cancelRender()
		g.cancelRender = nil
	}

	// Use the new SetImage which routes through RenderScaled callback
	// Thumbnail update is handled via onImageRendered callback set in Init
	g.mainView.SetImage(im)
}
