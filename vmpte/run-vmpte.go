package main

import (
	"bytes"
	"strings"
	"fmt"
	"flag"
	"os"
	"os/exec"
	"os/signal"
)

const (
	STATUS_FILE_PATH = "/tmp/vmpte"
)

type KernelConfig struct {
	MemoryOvercommit string
	TransHugePage    string
}

var (
	verbose *bool
	delay *string
	sleep *string
)

var (
	OrigKC	KernelConfig
	VmPTEKC = KernelConfig{"1", "madvise"}
)

func (p *KernelConfig) ReadKernelConfig() error {
	overcommit, err := os.ReadFile("/proc/sys/vm/overcommit_memory")
	if err != nil {
		return err
	}
	p.MemoryOvercommit = string(overcommit[:len(overcommit)-1])

	thp, err := os.ReadFile("/sys/kernel/mm/transparent_hugepage/enabled")
	if err != nil {
		return err
	}
	obra := bytes.Index(thp, []byte("["))
	cbra := bytes.Index(thp, []byte("]"))
	p.TransHugePage = string(thp[obra+1 : cbra])

	return nil
}

func (p *KernelConfig) WriteKernelConfig() error {
	err := os.WriteFile("/proc/sys/vm/overcommit_memory",
		[]byte(p.MemoryOvercommit), 0600)
	if err != nil {
		return err
	}
	err = os.WriteFile("/sys/kernel/mm/transparent_hugepage/enabled",
		[]byte(p.TransHugePage), 0600)
	if err != nil {
		return err
	}
	return nil
}

func (p *KernelConfig) String() string {
	var oc string

	if strings.Compare(p.MemoryOvercommit, "1") == 0 {
		oc = "enabled"
	} else {
		oc = "disabled"
	}
	return fmt.Sprintf("memory_overcommit=%s, thp=%s", oc, p.TransHugePage)
}

func run_and_collect_stats() error {
	cmd := exec.Command("./vmpte", "-d", *delay, "-s", *sleep)
	if cmd.Err != nil {
		fmt.Fprintf(os.Stderr, "%v\n", cmd.Err)
		return cmd.Err
	}
	err := cmd.Start()
	if err != nil {
		fmt.Fprintf(os.Stderr, "%v\n", err)
		return err
	}
	pid := cmd.Process.Pid

	fmt.Fprintf(os.Stderr, "Child process has pid %d\n", pid)

	err = cmd.Wait()
	if err != nil {
		fmt.Fprintf(os.Stderr, "vmpte failed: %v\n", err)
		return err
	}
	file, err := os.Open(STATUS_FILE_PATH)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Could not open status file: %v\n", err)
		return err
	}
	defer file.Close()

	procStat := make([]byte, 1024)
	_, err = file.Read(procStat)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Could not read status file\n")
		return err
	}
	ents := strings.Split(string(procStat), "\n")
	for _, ent := range ents {
		if strings.Contains(ent, "VmPTE") {
			fields := strings.Fields(ent)
			fmt.Printf("PageTables,%s,%s\n", fields[1], fields[2])
			break
		}
	}
	err = os.Remove(STATUS_FILE_PATH)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Could not unlink status file: %v\n", err)
		return err
	}
	return nil
}

func apply_vmpte_kernel_config() {
	err := OrigKC.ReadKernelConfig()
	if err != nil {
		panic(err)
	}
	if *verbose == true {
		fmt.Printf("memory_overcommit=%v\n", OrigKC.MemoryOvercommit)
		fmt.Printf("transparent_hugepages=%v\n", OrigKC.TransHugePage)
		fmt.Printf("Changing kernel config to %s\n", VmPTEKC.String())
	}
	err = VmPTEKC.WriteKernelConfig()
	if err != nil {
		panic(err)
	}
}

func revert_kernel_config() {
	if *verbose == true {
		fmt.Printf("Reverting kernel config to %s\n", OrigKC.String())
	}
	err := OrigKC.WriteKernelConfig()
	if err != nil {
		panic(err)
	}
}

func handle_signals() {
	c := make(chan os.Signal, 1)
	signal.Notify(c)
	for {
		<-c
		revert_kernel_config()
		os.Exit(1)
	 }
}

func main() {
	retval := 0

	verbose = flag.Bool("v", false, "Verbose")
	delay = flag.String("d", "0", "Inject us delay")
	sleep = flag.String("s", "0", "Sleep for ms before exiting")
	flag.Parse()

	if os.Geteuid() != 0 {
		fmt.Fprintf(os.Stderr, "Must run as root!\n")
		os.Exit(1)
	}

	go handle_signals()

	apply_vmpte_kernel_config()

	err := run_and_collect_stats()
	if err != nil {
		retval = 1
	}
	revert_kernel_config()
	os.Exit(retval)
}
