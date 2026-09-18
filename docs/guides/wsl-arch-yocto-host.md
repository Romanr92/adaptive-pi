# Arch Linux WSL2 Yocto host on a non-C drive

Use this guide to install the **official Arch Linux WSL distribution** directly
onto `D:\` (or another non-system drive), then prepare it for the
[Fresh Arch Linux to Yocto/QEMU setup](fresh-arch-yocto-build.md).

The AdaptivePi checkout and all Yocto build data live in the Linux filesystem
inside the WSL virtual disk, not on a mounted Windows drive.

> Windows keeps the WSL platform and some system files on its system drive.
> This guide moves the large Arch Linux WSL virtual disk and optional WSL swap
> file off `C:\`.

## 1. Install and update WSL2

On Windows 11, enable CPU virtualization in UEFI/BIOS. Open **PowerShell as
Administrator** and install WSL without a default distribution:

```powershell
wsl --install --no-distribution
```

Restart if prompted. Then, still in an elevated PowerShell, update and inspect
the currently available official distributions:

```powershell
wsl --update
wsl --set-default-version 2
wsl --list --online
```

The list must include the official `archlinux` entry. If it does not, run
`wsl --update`, restart Windows, and check again before proceeding.

## 2. Install official Arch Linux directly on D:

Install the official image at the requested location. The `--location` value is
the permanent storage directory for the distribution's virtual disk:

```powershell
New-Item -ItemType Directory -Force -Path D:\WSL\distros
wsl --install --distribution archlinux --location D:\WSL\distros\AdaptivePi-Arch
```

After installation, the distribution starts automatically as `root`. Run the
following in that Arch shell, then return to PowerShell:

```bash
exit
```

Now verify that Arch uses WSL2:

```powershell
wsl --list --verbose
```

The output must show `archlinux` with version `2`. The large `ext4.vhdx` must
be below `D:\WSL\distros\AdaptivePi-Arch`.

If Windows reports that `--location` is unsupported, update WSL with
`wsl --update` and restart Windows. Do not fall back to a third-party Arch
image or manually-built bootstrap archive for this project.

## 3. Configure optional WSL resources

The resource limits below are optional. Configure them after the first Arch
launch and before starting a Yocto build.

Create the non-C directories for WSL swap and the distribution location if they
do not already exist:

```powershell
New-Item -ItemType Directory -Force -Path D:\WSL\distros, D:\WSL\swap
```

To create `%UserProfile%\.wslconfig` directly from PowerShell, run:

```powershell
$wslConfig = Join-Path $env:USERPROFILE '.wslconfig'
@'
[wsl2]
memory=16GB
processors=4
swap=8GB
swapFile=D:\\WSL\\swap\\swap.vhdx
nestedVirtualization=true

[experimental]
# Reclaim unused memory back to Windows dynamically.
autoMemoryReclaim=dropcache
'@ | Set-Content -Path $wslConfig -Encoding ascii

Get-Content $wslConfig
```

This is a starting allocation for a 32 GiB Windows host. To create or edit the
file manually instead, run `notepad $wslConfig`, paste the same content, save,
and close Notepad.

| Windows RAM | WSL memory | WSL processors | WSL swap |
|---|---:|---:|---:|
| 16 GiB | 10 GiB | 2 | 8 GiB |
| 32 GiB | 16–20 GiB | 4 | 8–16 GiB |
| 64 GiB or more | 32 GiB | 8 | 16 GiB |

Do not assign all host RAM or logical processors to WSL: Windows needs
resources while Yocto builds. Apply the file before the Yocto build:

### Worked examples

**Intel Core i9-14900KF with 64 GiB RAM**: begin with 32 GiB WSL memory,
8 processors, and 16 GiB swap. The host has 32 logical CPU threads, but this
leaves capacity for Windows and avoids memory pressure during a Yocto build.

```ini
[wsl2]
memory=32GB
processors=8
swap=16GB
swapFile=D:\\WSL\\swap\\swap.vhdx
nestedVirtualization=true

[experimental]
autoMemoryReclaim=dropcache
```

**ASUS TUF A18 FA808U with 32 GiB RAM**: begin with 16 GiB WSL memory,
4 processors, and 8 GiB swap. If this laptop has 64 GiB RAM instead, use the
64 GiB profile above; if it has 16 GiB, use the 16 GiB row in the table.

```ini
[wsl2]
memory=16GB
processors=4
swap=8GB
swapFile=D:\\WSL\\swap\\swap.vhdx
nestedVirtualization=true

[experimental]
autoMemoryReclaim=dropcache
```

```powershell
wsl --shutdown
```

## 4. Initialize Arch Linux

Start the new official Arch distribution as root:

```powershell
wsl -d archlinux -u root
```

In Arch, initialize package signing, update the system, and install the minimal
administration tools.

> [!IMPORTANT]
> Replace every `<selected_username>` below with the Linux username you chose.
> The angle brackets are only a visible placeholder; do not type them in the
> commands.

```bash
pacman-key --init
pacman-key --populate archlinux
pacman -Syu --noconfirm
pacman -S --needed sudo nano

passwd root
useradd -m -G wheel -s /bin/bash <selected_username>
passwd <selected_username>
EDITOR=nano visudo -f /etc/sudoers.d/10-wheel
```

Add the following one line in the opened sudoers file, save, and exit:

```sudoers
%wheel ALL=(ALL:ALL) ALL
```

Then configure the UTF-8 locale required by BitBake. Open the existing locale
configuration and remove the leading `#` only from `en_US.UTF-8 UTF-8`:

```bash
chmod 440 /etc/sudoers.d/10-wheel
nano /etc/locale.gen
```

In `nano`, use `Ctrl+W` to find `en_US.UTF-8 UTF-8`, remove its leading `#`,
then save with `Ctrl+O`, press Enter, and exit with `Ctrl+X` before running
`locale-gen`.

```bash
locale-gen
printf 'LANG=en_US.UTF-8\n' > /etc/locale.conf
exit
```

## 5. Set the default user and begin AdaptivePi setup

Back in PowerShell, set the normal user as default and restart WSL:

```powershell
wsl -d archlinux -u root -- sh -c "printf '[user]\ndefault=<selected_username>\n' > /etc/wsl.conf"
wsl --shutdown
wsl -d archlinux
```

In the new Arch shell, verify the environment and create the Linux-native
workspace:

```bash
whoami
locale
sudo -v
mkdir -p ~/workspace
```

Expected results are the selected username from `whoami`, `en_US.UTF-8` for
`LANG`, and working `sudo`.

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
| `archlinux` is not in `wsl --list --online` | Update WSL, restart Windows, then check the online list again. |
| `--location` is unsupported | Update WSL, restart Windows, and retry. |
| BitBake cannot find `en_US.UTF-8` | Run `locale -a`, repeat the locale commands in section 4 if necessary, then open a new WSL shell. |
| Yocto is unexpectedly slow | Confirm the checkout is under `~/workspace`, not below `/mnt/c` or `/mnt/d`. |
