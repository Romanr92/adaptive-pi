#!/usr/bin/env python3
"""Boot an extracted platform with host QEMU, without a Yocto build tree."""
import configparser
import os
from pathlib import Path
import shlex
import sys

root = Path(__file__).resolve().parent
config = configparser.ConfigParser(interpolation=None)
config.read(root / "adaptive-pi-image-qemuarm64.rootfs.qemuboot.conf")
c = config["config_bsp"]
args = ["qemu-system-aarch64", "-nographic", "-kernel", str(root / "Image")]
for key in ("qb_machine", "qb_cpu", "qb_mem", "qb_smp", "qb_rng", "qb_opt_append", "qb_serial_opt"):
    args.extend(shlex.split(c.get(key, "")))
rootfs = str(root / "adaptive-pi-image-qemuarm64.rootfs.ext4")
for argument in shlex.split(c["qb_rootfs_opt"]):
    args.append(argument.replace("@ROOTFS@", rootfs))
args.extend(["-netdev", "user,id=net0,hostfwd=tcp:127.0.0.1:2222-:22"])
args.extend(shlex.split(c["qb_network_device"].replace("@MAC@", "52:54:00:12:34:56")))
commandline = "root=/dev/vda rw ip=dhcp"
for console in c.get("serial_consoles", "").split():
    commandline += " console=" + console.split(";")[1]
commandline += " " + c.get("qb_kernel_cmdline_append", "")
args.extend(["-append", commandline])
try:
    os.execvp(args[0], args)
except OSError as error:
    sys.exit(f"Could not start host QEMU: {error}")
