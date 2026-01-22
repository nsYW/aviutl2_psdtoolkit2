package ipc

import (
	"fmt"
	"os"
	"syscall"
	"unsafe"
)

var (
	kernel32                = syscall.NewLazyDLL("kernel32.dll")
	procOpenFileMappingW    = kernel32.NewProc("OpenFileMappingW")
	procMapViewOfFile       = kernel32.NewProc("MapViewOfFile")
	procUnmapViewOfFile     = kernel32.NewProc("UnmapViewOfFile")
	procGetCurrentProcessId = kernel32.NewProc("GetCurrentProcessId")
)

const (
	fileMapRead = 0x0004
)

// SharedMemory holds shared memory resources for zero-copy IPC
// C side creates the mapping, Go side opens and writes to it
type SharedMemory struct {
	hMapFile   syscall.Handle
	mappedPtr  unsafe.Pointer
	bufferSize int
	cPID       int // C side PID (parent process)
}

// NewSharedMemory creates a SharedMemory instance
// The actual mapping is opened on first use or when size changes
func NewSharedMemory(cPID int) *SharedMemory {
	return &SharedMemory{
		cPID: cPID,
	}
}

// openMapping opens the shared memory mapping created by C side
func (shm *SharedMemory) openMapping() error {
	// Close existing mapping if any
	shm.closeMapping()

	// Open shared memory created by C side using C's PID
	name := fmt.Sprintf("Local\\PSDTKit_Pixel_%d", shm.cPID)
	namePtr, _ := syscall.UTF16PtrFromString(name)

	ret, _, errno := procOpenFileMappingW.Call(
		fileMapRead|0x0002, // FILE_MAP_READ | FILE_MAP_WRITE
		0,
		uintptr(unsafe.Pointer(namePtr)),
	)
	if ret == 0 {
		return fmt.Errorf("OpenFileMappingW failed: %v", errno)
	}
	shm.hMapFile = syscall.Handle(ret)

	// Map the entire view (size 0 means entire mapping)
	ret, _, errno = procMapViewOfFile.Call(
		uintptr(shm.hMapFile),
		fileMapRead|0x0002, // FILE_MAP_READ | FILE_MAP_WRITE
		0, 0,
		0, // Map entire file
	)
	if ret == 0 {
		syscall.CloseHandle(shm.hMapFile)
		shm.hMapFile = 0
		return fmt.Errorf("MapViewOfFile failed: %v", errno)
	}
	shm.mappedPtr = unsafe.Pointer(ret)

	return nil
}

// closeMapping releases the current mapping
func (shm *SharedMemory) closeMapping() {
	if shm.mappedPtr != nil {
		procUnmapViewOfFile.Call(uintptr(shm.mappedPtr))
		shm.mappedPtr = nil
	}
	if shm.hMapFile != 0 {
		syscall.CloseHandle(shm.hMapFile)
		shm.hMapFile = 0
	}
	shm.bufferSize = 0
}

// EnsureOpen ensures the shared memory is opened
// If shmResized is true, reopens the mapping
func (shm *SharedMemory) EnsureOpen(shmResized bool) error {
	if shmResized || shm.mappedPtr == nil {
		return shm.openMapping()
	}
	return nil
}

// Close releases shared memory resources
func (shm *SharedMemory) Close() {
	shm.closeMapping()
}

// CPID returns the C side process ID
func (shm *SharedMemory) CPID() int {
	return shm.cPID
}

// GetBuffer returns the shared memory buffer as a byte slice
// Note: The actual size is determined by C side, we use maxLen for safety
func (shm *SharedMemory) GetBuffer(maxLen int) []byte {
	if shm.mappedPtr == nil || maxLen <= 0 {
		return nil
	}
	return unsafe.Slice((*byte)(shm.mappedPtr), maxLen)
}

// GetParentPID returns the parent process ID (C side)
func GetParentPID() int {
	// On Windows, we need to find the parent process
	// For simplicity, use environment variable or PPID
	// Since we're launched by C side, use PPID
	return os.Getppid()
}
