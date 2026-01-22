package ipc

import (
	"encoding/binary"
	"errors"
	"image"
	"io"
	"math"
	"os"
	"runtime"
	"sync"

	"psdtoolkit/ods"
)

func nrgbaToNBGRA(p []byte) {
	for i := 0; i < len(p); i += 4 {
		if p[i+3] > 0 {
			p[i+2], p[i+0] = p[i+0], p[i+2]
		}
	}
}

// copyWithOffsetBGRA copies src to dst with offset and NRGBA->NBGRA conversion in a single pass.
//
// GPU-side Flip Optimization:
// Flip processing is NOT done here - it's delegated to AviUtl's GPU-based flip filter
// (obj.effect("反転")) which runs on the GPU with essentially zero CPU cost.
// This eliminates the CPU overhead of pixel-by-pixel flip operations for large images.
//
// Offset Inversion for Flip:
// When flipX or flipY is enabled, the offset sign is inverted. This is because:
//   - Without flip: offset is applied first, then the image is displayed as-is
//   - With GPU flip: the image is flipped AFTER being positioned, so we need to
//     pre-invert the offset so that flip(offset(image)) == offset(flip(image))
//
// Example: if flipY is on and offsetY is +100, we use -100 so the final position
// after GPU flip matches what it would be if we did CPU flip with +100 offset.
//
// Uses parallel processing for performance.
func copyWithOffsetBGRA(dst, src *image.NRGBA, offsetX, offsetY int, flipX, flipY bool) {
	dstW, dstH := dst.Rect.Dx(), dst.Rect.Dy()
	srcW, srcH := src.Rect.Dx(), src.Rect.Dy()

	// Invert offset for flipped axes to maintain correct positioning after GPU flip
	// (see function comment for detailed explanation)
	if flipX {
		offsetX = -offsetX
	}
	if flipY {
		offsetY = -offsetY
	}

	numWorkers := runtime.NumCPU()
	rowsPerWorker := (dstH + numWorkers - 1) / numWorkers

	var wg sync.WaitGroup
	for w := 0; w < numWorkers; w++ {
		startY := w * rowsPerWorker
		endY := startY + rowsPerWorker
		if endY > dstH {
			endY = dstH
		}
		if startY >= dstH {
			break
		}

		wg.Add(1)
		go func(startY, endY int) {
			defer wg.Done()
			for dy := startY; dy < endY; dy++ {
				dstRowStart := (dy-dst.Rect.Min.Y)*dst.Stride - dst.Rect.Min.X*4
				for dx := 0; dx < dstW; dx++ {
					// Calculate source coordinates with offset
					sx := dx - offsetX
					sy := dy - offsetY

					// Bounds check
					if sx < 0 || sx >= srcW || sy < 0 || sy >= srcH {
						continue // Leave dst pixel as zero (transparent)
					}

					srcIdx := (sy-src.Rect.Min.Y)*src.Stride + (sx-src.Rect.Min.X)*4
					dstIdx := dstRowStart + dx*4

					// Copy with RGBA -> BGRA swap (only if alpha > 0)
					if src.Pix[srcIdx+3] > 0 {
						dst.Pix[dstIdx+0] = src.Pix[srcIdx+2] // B <- R
						dst.Pix[dstIdx+1] = src.Pix[srcIdx+1] // G <- G
						dst.Pix[dstIdx+2] = src.Pix[srcIdx+0] // R <- B
						dst.Pix[dstIdx+3] = src.Pix[srcIdx+3] // A <- A
					}
				}
			}
		}(startY, endY)
	}
	wg.Wait()
}

func readIDAndFilePath() (int, string, error) {
	id, err := readInt32()
	if err != nil {
		return 0, "", err
	}
	filePath, err := readString()
	if err != nil {
		return 0, "", err
	}
	ods.ODS("  ID: %d / FilePath: %s", id, filePath)
	return id, filePath, nil
}

func readUInt64() (uint64, error) {
	var l uint64
	if err := binary.Read(os.Stdin, binary.LittleEndian, &l); err != nil {
		return 0, err
	}
	return l, nil
}

func readUInt32() (int, error) {
	var l uint32
	if err := binary.Read(os.Stdin, binary.LittleEndian, &l); err != nil {
		return 0, err
	}
	return int(l), nil
}

func readInt32() (int, error) {
	var l uint32
	if err := binary.Read(os.Stdin, binary.LittleEndian, &l); err != nil {
		return 0, err
	}
	return int(int32(l)), nil
}

func readFloat32() (float32, error) {
	var l uint32
	if err := binary.Read(os.Stdin, binary.LittleEndian, &l); err != nil {
		return 0, err
	}
	return math.Float32frombits(l), nil
}

func readBool() (bool, error) {
	i, err := readInt32()
	if err != nil {
		return false, err
	}
	return i != 0, nil
}

func readString() (string, error) {
	l, err := readInt32()
	if err != nil {
		return "", err
	}

	buf := make([]byte, l)
	read, err := io.ReadFull(os.Stdin, buf)
	if err != nil {
		return "", err
	}
	if read != l {
		return "", errors.New("unexcepted read size")
	}
	return string(buf), nil
}

func writeUint64(i uint64) error {
	return binary.Write(os.Stdout, binary.LittleEndian, i)
}

func writeInt32(i int32) error {
	return binary.Write(os.Stdout, binary.LittleEndian, i)
}

func writeUint32(i uint32) error {
	return binary.Write(os.Stdout, binary.LittleEndian, i)
}

func writeFloat32(v float32) error {
	return binary.Write(os.Stdout, binary.LittleEndian, math.Float32bits(v))
}

func writeBool(v bool) error {
	if v {
		return writeInt32(1)
	}
	return writeInt32(0)
}

func writeString(s string) error {
	if err := writeInt32(int32(len(s))); err != nil {
		return err
	}
	if _, err := os.Stdout.WriteString(s); err != nil {
		return err
	}
	ods.ODS("  -> String(Len: %d)", len(s))
	return nil
}

func writeReply(err error) error {
	if err == nil {
		return writeUint32(0x80000000)
	}
	s := err.Error()
	if err := writeUint32(uint32(len(s)&0x7fffffff) | 0x80000000); err != nil {
		return err
	}
	if _, err := os.Stdout.WriteString(s); err != nil {
		return err
	}
	ods.ODS("  -> Error: %v", err)
	return nil
}

func writeBinary(b []byte) error {
	if err := writeInt32(int32(len(b))); err != nil {
		return err
	}
	if _, err := os.Stdout.Write(b); err != nil {
		return err
	}
	ods.ODS("  -> Binary(Len: %d)", len(b))
	return nil
}

func itoa(x int) string {
	if x < 10 {
		return string([]byte{byte(x + '0')})
	} else if x < 100 {
		return string([]byte{byte(x/10 + '0'), byte(x%10 + '0')})
	}

	var b [32]byte
	i := len(b) - 1
	for x > 9 {
		b[i] = byte(x%10 + '0')
		x /= 10
		i--
	}
	b[i] = byte(x + '0')
	return string(b[i:])
}
