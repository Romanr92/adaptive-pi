# Publish a Yocto SDK

## Purpose

The AdaptivePi Yocto SDK is a generated release artifact. It is not committed to Git.

A published SDK lets CI reproduce the same AArch64 cross-build environment used locally, without rebuilding Yocto for every pull request.

## When to publish a new SDK

Publish a new SDK when one of these changes:

- Yocto/Poky version;
- target machine or tuning;
- target image package contents;
- SDK configuration;
- target toolchain configuration.

Normal C++ application changes reuse the SDK associated with the current platform release.

## Generate the SDK installer

Open the initialized Yocto build environment and run:

```bash
bitbake adaptive-pi-image -c populate_sdk
```

The installer is generated under:

```text
yocto/build/tmp/deploy/sdk/
```

## Create the SHA-256 checksum

From the repository root, run:

```Bash
scripts/yocto/create-sdk-checksum.sh \
  yocto/build/tmp/deploy/sdk/<sdk-installer>.sh
```

This creates a neighboring checksum file:

```text
<sdk-installer>.sh.sha256
```

The checksum contains only the installer filename. This makes it valid after CI downloads both files into its own temporary directory.

# Verify the checksum locally

```Bash
cd yocto/build/tmp/deploy/sdk
sha256sum -c <sdk-installer>.sh.sha256
```

The result must end with:

```Text
OK
```
Do not publish the installer if verification fails.

# Upload the release assets

For the GitHub release corresponding to this platform version:

1. Open the release and select Edit release.
2. Upload the SDK installer (`.sh`).
3. Upload the matching checksum (`.sh.sha256`).
4. Save the release.

Keep both files attached to the same release tag.

## CI usage

The pull-request AArch64 target-build gate will:

1. download the versioned SDK installer and checksum;
2. verify the SHA-256 checksum;
3. install the SDK in the CI workspace;
4. configure and cross-build the application;
5. validate that the resulting executable is an AArch64 ELF binary.

This gate proves that application changes remain cross-compilable for the AdaptivePi target. It does not yet run the application in QEMU.