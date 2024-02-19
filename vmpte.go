package main

import (
	"fmt"
	"os"
	"bytes"
	"strings"
)

type KernelConfig struct {
	MemoryOvercommit string
	TransHugePage string
}

func (p *KernelConfig) ReadKernelConfig() error {
	overcommit, err := os.ReadFile("/proc/sys/vm/overcommit_memory")
	if err != nil {
		return err
	}
	p.MemoryOvercommit = string(overcommit[:len(overcommit) - 1])

	thp, err := os.ReadFile("/sys/kernel/mm/transparent_hugepage/enabled")
	if err != nil {
		return err
	}
	obra := bytes.Index(thp, []byte("["))
	cbra := bytes.Index(thp, []byte("]"))
	p.TransHugePage = string(thp[obra + 1 : cbra])

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

func main() {
	OrigKC := KernelConfig{}
	VmPTEKC := KernelConfig{"1", "madvise"}

	if os.Geteuid() != 0 {
		fmt.Fprintf(os.Stderr, "Must run as root!\n")
		os.Exit(1)
	}
	err := OrigKC.ReadKernelConfig();
	if err != nil {
		panic(err)
	}
	fmt.Printf("memory_overcommit=%v\n", OrigKC.MemoryOvercommit)
	fmt.Printf("transparent_hugepages=%v\n", OrigKC.TransHugePage)

	fmt.Printf("Changing kernel config to %s\n", VmPTEKC.String())
	err = VmPTEKC.WriteKernelConfig()
	if err != nil {
		panic(err)
	}

	fmt.Printf("Reverting kernel config to %s\n", OrigKC.String())
	err = OrigKC.WriteKernelConfig()
	if err != nil {
		panic(err)
	}
}
