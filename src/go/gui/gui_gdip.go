//go:build gdip

package gui

import (
	"psdtoolkit/locale"
	"psdtoolkit/ods"

	"github.com/golang-ui/nuklear/nk"
	"github.com/golang-ui/nuklear/nk/w32"
)

// font wraps either a single GdipFont or a FontChain for fallback support.
type font struct {
	gdipFont  *nk.GdipFont  // Used for symbol font (single font)
	fontChain *nk.FontChain // Used for main font (font chain with fallback)
}

// Close releases font resources.
func (f *font) Close() {
	if f.fontChain != nil {
		f.fontChain.Close()
		f.fontChain = nil
	}
	if f.gdipFont != nil {
		f.gdipFont.Close()
		f.gdipFont = nil
	}
}

// Handle returns the nuklear user font handle.
func (f *font) Handle() *nk.UserFont {
	if f.fontChain != nil {
		return f.fontChain.Handle()
	}
	if f.gdipFont != nil {
		return f.gdipFont.Handle()
	}
	return nil
}

func (g *GUI) initFont(symbolFont []byte) error {
	// Create symbol font from embedded TTF (for icons)
	symbol, err := nk.NkCreateFontFromBytes(symbolFont, 14)
	if err != nil {
		return err
	}
	g.font.Symbol = &font{gdipFont: symbol}
	g.font.SymbolHandle = g.font.Symbol.Handle()

	// Create main font chain with system fonts based on locale
	fontList := locale.GetPreferredFontList()
	ods.ODS("locale: detected fonts: %v", fontList.UIFonts)

	fontChain := nk.NewFontChain(18)

	// Add system fonts from the locale-specific list
	for _, fontName := range fontList.UIFonts {
		if err := fontChain.AddSystemFont(fontName); err != nil {
			ods.ODS("locale: font %q not available: %v", fontName, err)
			continue
		}
		ods.ODS("locale: added font %q to chain", fontName)
	}

	// Always add Tahoma as the final fallback
	if err := fontChain.AddSystemFont("Tahoma"); err != nil {
		ods.ODS("locale: fallback font Tahoma not available: %v", err)
	} else {
		ods.ODS("locale: added fallback font Tahoma to chain")
	}

	g.font.Main = &font{fontChain: fontChain}
	g.font.MainHandle = g.font.Main.Handle()
	nk.NkStyleSetFont(g.context, g.font.MainHandle)
	return nil
}

func (g *GUI) freeFont() {
	if g.font.SymbolHandle != nil {
		g.font.SymbolHandle.Free()
	}
	if g.font.Symbol != nil {
		g.font.Symbol.Close()
	}
	if g.font.MainHandle != nil {
		g.font.MainHandle.Free()
	}
	if g.font.Main != nil {
		g.font.Main.Close()
	}
}

func (g *GUI) pollEvents() {
	w32.PollEvents()
}

func (g *GUI) terminate() {
	w32.Terminate()
}

type window struct {
	w *w32.Window
}

func newWindow(width, height int, title string) (*window, *nk.Context, error) {
	if err := w32.Init(); err != nil {
		return nil, nil, err
	}
	win, err := w32.CreateWindow(width, height, title, nil, nil)
	if err != nil {
		return nil, nil, err
	}
	ctx := nk.NkPlatformInit(win, nk.PlatformInstallCallbacks)
	return &window{w: win}, ctx, nil
}

func (w *window) Show()                 { w.w.Show() }
func (w *window) Hide()                 { w.w.Hide() }
func (w *window) GetSize() (int, int)   { return w.w.GetSize() }
func (w *window) SetShouldClose(v bool) { w.w.SetShouldClose(v) }
func (w *window) ShouldClose() bool     { return w.w.ShouldClose() }
func (w *window) NativeWindow() uintptr { return uintptr(w.w.Handle) }
func (w *window) SetDropCallback(fn func(w *window, filenames []string)) {
	w.w.SetDropCallback(func(_ *w32.Window, filenames []string) {
		fn(w, filenames)
	})
}

func (w *window) Render() {
	var clearColor nk.Color
	nk.NkPlatformRender(nk.AntiAliasingOff, clearColor)
}
