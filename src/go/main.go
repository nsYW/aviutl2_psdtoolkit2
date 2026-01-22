package main

import (
	"context"
	"flag"
	"fmt"
	"runtime"

	// _ "net/http/pprof"

	"psdtoolkit/assets"
	"psdtoolkit/gc"
	"psdtoolkit/gui"
	"psdtoolkit/imgmgr/editing"
	"psdtoolkit/imgmgr/source"
	"psdtoolkit/ipc"
	"psdtoolkit/ods"
)

var gitTag string
var gitRevision string

func init() {
	runtime.LockOSThread()
}

type odsLogger struct{}

func (odsLogger) Println(v ...interface{}) {
	ods.ODS(fmt.Sprintln(v...))
}

func main() {
	// go http.ListenAndServe(":6060", nil)
	// psd.Debug = log.New(os.Stdout, "psd: ", log.Lshortfile)

	defer func() {
		if err := recover(); err != nil {
			ods.Recover(err)
		}
	}()

	flag.Parse()

	srcs := &source.Sources{Logger: odsLogger{}}

	// Create and start the Editing actor
	ed := editing.New(srcs)
	ctx, cancelEditing := context.WithCancel(context.Background())
	defer cancelEditing()
	go ed.Run(ctx)

	ipcm := ipc.New(srcs)
	g := gui.New(ed)

	ipcm.AddFile = g.AddFileSync
	ipcm.UpdateCurrentProjectPath = func(file string) error {
		srcs.ProjectPath = file
		return nil
	}
	ipcm.UpdateTagState = g.UpdateTagStateSync
	ipcm.ClearFiles = g.ClearFiles
	ipcm.GetWindowHandle = g.GetWindowHandle
	ipcm.Serialize = g.Serialize
	ipcm.Deserialize = g.Deserialize
	ipcm.GCing = g.Touch
	g.SendEditingImageState = ipcm.SendEditingImageState
	g.ExportFaviewSlider = ipcm.ExportFaviewSlider
	g.ExportLayerNames = ipcm.ExportLayerNames
	g.RenderScaled = ipcm.RenderScaled
	g.DropFiles = func(filenames []string) {
		if err := g.AddFile(extractPSDAndPFV(filenames), 0); err != nil {
			g.ReportError(err)
		}
	}

	exitCh := make(chan struct{})
	go ipcm.Main(exitCh)
	gcDone := gc.Start(exitCh)

	if err := g.Init(
		"PSDToolKit "+gitTag+" ( "+gitRevision+" )",
		assets.BG,
		assets.Symbols,
	); err != nil {
		ipcm.Abort(err)
		<-gcDone
		return
	}

	g.Main(exitCh)
	<-gcDone
}
