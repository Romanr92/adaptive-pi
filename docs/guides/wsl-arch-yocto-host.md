# Arch Linux WSL2 Yocto host on a non-C drive

Use this guide to place the large Arch WSL2 distribution disk on `D:\` (or
another non-system drive), then prepare it for the
[Fresh Arch Linux to Yocto/QEMU setup](fresh-arch-yocto-build.md).

The repository and all Yocto build data live in the Linux filesystem inside the
WSL virtual disk, not on a mounted Windows drive.

> Windows still keeps the WSL platform and some system files on its system
> drive. This guide moves the large, growable Linux `ext4.vhdx` and optional WSL
> swap file off `C:\`.

## 1. Install WSL2

On Windows 11, enable CPU virtualization in UEFI/BIOS. Open **PowerShell as
Administrator** and run:

```powershell
wsl --install --no-distribution
```

Restart if prompted. Then, still in an elevated PowerShell:

```powershell
wsl --update
wsl --set-default-version 2
wsl --status
```

The status output must show default version `2`. `--no-distribution` prevents
WSL from installing a default Linux distribution to its default location.

## 2. Configure WSL resources and storage

Create the non-C directories:

```powershell
New-Item -ItemType Directory -Force -Path D:\WSL\distros, D:\WSL\swap, D:\WSL\import
```

Create `%UserProfile%\.wslconfig` with a starting allocation appropriate to a
32 GiB Windows host:

```ini
[wsl2]
memory=16GB
processors=4
swap=8GB
swapFile=D:\\WSL\\swap\\swap.vhdx
```

Use the table to choose an initial allocation. Do not assign all host RAM or
logical processors to WSL: Windows needs resources while Yocto builds.

| Windows RAM | WSL memory | WSL processors | WSL swap |
|---|---:|---:|---:|
| 16 GiB | 10 GiB | 2 | 8 GiB |
| 32 GiB | 16–20 GiB | 4 | 8–16 GiB |
| 64 GiB or more | 32 GiB | 8 | 16 GiB |

Apply the configuration:

```powershell
wsl --shutdown
```

## 3. Download and verify the official Arch bootstrap archive

Install 7-Zip, then download the current Arch bootstrap archive and its
checksum list:

```powershell
winget install --id 7zip.7zip --exact

$import = 'D:\WSL\import'
$name = 'archlinux-bootstrap-x86_64.tar.zst'
$bootstrap = Join-Path $import $name

Invoke-WebRequest -Uri "https://geo.mirror.pkgbuild.com/iso/latest/$name" -OutFile $bootstrap
Invoke-WebRequest -Uri 'https://geo.mirror.pkgbuild.com/iso/latest/sha256sums.txt' -OutFile (Join-Path $import 'sha256sums.txt')

$expected = (Select-String -Path (Join-Path $import 'sha256sums.txt') -Pattern "^([0-9a-f]{64})\s+\*?$name$").Matches.Groups[1].Value.ToLower()
$actual = (Get-FileHash -Algorithm SHA256 $bootstrap).Hash.ToLower()
if (-not $expected -or $actual -ne $expected) {
  throw 'Arch bootstrap SHA-256 verification failed. Do not import this archive.'
}
'Arch bootstrap SHA-256 verified.'
```

Do not skip the checksum check. It validates the exact archive downloaded for
this installation.

## 4. Create the rootfs tarball and import it to D:

Create a WSL-importable tarball from the verified archive:

```powershell
$sevenZip = "$env:ProgramFiles\7-Zip\7z.exe"
$stage = 'D:\WSL\import\arch-bootstrap'
$rootfs = 'D:\WSL\import\archlinux-wsl-rootfs.tar'

Remove-Item -Recurse -Force $stage -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stage | Out-Null
& $sevenZip x $bootstrap "-o$stage"
& $sevenZip x (Join-Path $stage 'archlinux-bootstrap-x86_64.tar') "-o$stage"
tar.exe -C (Join-Path $stage 'root.x86_64') -cf $rootfs .
if (-not (Test-Path $rootfs)) { throw 'WSL root filesystem tarball was not created.' }
```

Import it. The second argument is the permanent location of the distribution
disk:

```powershell
wsl --import AdaptivePi-Arch `
  D:\WSL\distros\AdaptivePi-Arch `
  D:\WSL\import\archlinux-wsl-rootfs.tar `
  --version 2
wsl --list --verbose
```

`AdaptivePi-Arch` must appear with version `2`. Its `ext4.vhdx` is stored under
`D:\WSL\distros\AdaptivePi-Arch`.

## 5. Initialize Arch Linux

Start the imported distribution as root:

```powershell
wsl -d AdaptivePi-Arch -u root
```

In Arch, initialize package keys and create a normal development user. Replace
`adaptivepi` with your preferred Linux username if needed.

```bash
pacman-key --init
pacman-key --populate archlinux
pacman -Syu --needed sudo vi

useradd -m -G wheel -s /bin/bash adaptivepi
passwd adaptivepi
EDITOR=vi visudo -f /etc/sudoers.d/10-wheel
```

Add the following one line in the opened sudoers file, save, and exit:

```sudoers
%wheel ALL=(ALL:ALL) ALL
```

Then configure the UTF-8 locale required by BitBake:

```bash
chmod 440 /etc/sudoers.d/10-wheel
sed -i 's/^#en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen
locale-gen
printf 'LANG=en_US.UTF-8\n' > /etc/locale.conf
exit
```

## 6. Set the default user and begin AdaptivePi setup

Back in PowerShell, set the normal user as default and restart WSL:

```powershell
wsl -d AdaptivePi-Arch -u root -- sh -c "printf '[user]\ndefault=adaptivepi\n' > /etc/wsl.conf"
wsl --shutdown
wsl -d AdaptivePi-Arch
```

In the new Arch shell, verify the environment and create the Linux-native
workspace:

```bash
whoami
locale
sudo -v
mkdir -p ~/workspace
```

Expected results are `adaptivepi` from `whoami`, `en_US.UTF-8` for `LANG`, and
working `sudo`.

Do **not** store the Yocto checkout under `/mnt/c` or `/mnt/d`: those are
mounted NTFS filesystems and are slower with different permission semantics.
Use `~/workspace/adaptive-pi`, which is stored in the `D:\`-backed WSL disk.

Continue at section 3, **Install all required host tools**, in the
[Fresh Arch Linux to Yocto/QEMU setup](fresh-arch-yocto-build.md). Run all
Yocto, QEMU, CMake, and Git commands in the Arch WSL shell. In VS Code, use
**Remote - WSL** to open the repository inside WSL.

## Troubleshooting

| Symptom | Resolution |
|---|---|
| `wsl --install` reports virtualization is disabled | Enable CPU virtualization in UEFI/BIOS and reboot. |
| `wsl --import` fails | Confirm that the rootfs tar exists, WSL2 is enabled, and `D:\` is NTFS with enough free space. |
| `pacman-key --init` is slow | Let the initial key generation complete; do not interrupt it. |
| BitBake cannot find `en_US.UTF-8` | Run `locale -a`, repeat the locale commands in section 5 if necessary, then open a new WSL shell. |
| Yocto is unexpectedly slow | Confirm the checkout is under `~/workspace`, not below `/mnt/c` or `/mnt/d`. |
