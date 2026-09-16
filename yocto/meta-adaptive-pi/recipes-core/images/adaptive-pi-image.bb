SUMMARY = "AdaptivePi development image for generic AArch64 QEMU"
DESCRIPTION = "Minimal AdaptivePi target image with SSH, C++ runtime, and debugging support."
LICENSE = "Apache-2.0"

require recipes-core/images/core-image-minimal.bb

IMAGE_FEATURES += " ssh-server-openssh"

IMAGE_INSTALL:append = " \
    gdbserver \
    libstdc++ \
"