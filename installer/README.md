# POSIX9 SSH Server Installer for Mac OS 9

## Package Contents

```
POSIX9 SSH/
├── Dropbear SSH              # Main SSH server application
├── Dropbear Key Generator    # Tool to create host keys
├── Read Me                   # User documentation
├── Configuration/
│   ├── passwd.sample         # Sample password file
│   └── authorized_keys.sample
└── Source/                   # Optional: source code
    └── posix9-src.sit.hqx
```

## Creating the Installer

### Option 1: StuffIt Archive (Simplest)

1. Build the binaries with CodeWarrior or Retro68
2. Place in the `files/` directory
3. Use StuffIt Deluxe to create a .sit archive
4. Convert to .sit.hqx for web distribution

### Option 2: InstallerVISE

1. Install InstallerVISE on a Mac
2. Open `POSIX9_SSH.vct` template
3. Add built binaries
4. Build installer

### Option 3: Apple Installer

1. Use `PackageMaker` or script-based installer
2. Create `.smi` (Self-Mounting Image)

## Build Process

### Prerequisites

- CodeWarrior Pro 8+ OR Retro68 cross-compiler
- Mac OS 9.1+ SDK headers

### Building with Retro68

```bash
# On Linux/macOS host:
cd posix9
mkdir build && cd build

# Configure for PPC Mac OS 9
cmake .. -DCMAKE_TOOLCHAIN_FILE=$RETRO68/m68k-apple-macos/cmake/retro68-ppc.cmake

# Build
make

# Package
make package
```

### Building with CodeWarrior

1. Open `POSIX9_SSH.mcp`
2. Set target to "PPC Release"
3. Build (Cmd+M)
4. Find binary in `build/` folder

## Installation Script (MPW)

```mpw
# POSIX9 SSH Installer Script
# Run from MPW Shell

Set InstallDir "{SystemFolder}Extensions:POSIX9 SSH:"

# Create directories
NewFolder "{InstallDir}"
NewFolder "{SystemFolder}Preferences:dropbear:"

# Copy files
Duplicate -y "Dropbear SSH" "{InstallDir}"
Duplicate -y "Dropbear Key Generator" "{InstallDir}"

# Generate host keys
"{InstallDir}Dropbear Key Generator" -t rsa ∂
    -f "{SystemFolder}Preferences:dropbear:dropbear_rsa_host_key"

Echo "Installation complete!"
Echo "To start SSH server, run 'Dropbear SSH' from Extensions folder"
```

## File Locations After Install

| File | Location |
|------|----------|
| Dropbear SSH | System Folder:Extensions:POSIX9 SSH: |
| Host Keys | System Folder:Preferences:dropbear: |
| Password File | System Folder:Preferences:dropbear:passwd |
| Authorized Keys | System Folder:Preferences:dropbear:authorized_keys |
| Log File | System Folder:Preferences:dropbear:dropbear.log |

## Post-Installation

1. **Generate Host Keys** (first run only)
   - Double-click "Dropbear Key Generator"
   - Or run: `dropbearkey -t rsa -f ":System Folder:Preferences:dropbear:dropbear_rsa_host_key"`

2. **Configure Password** (optional)
   - Edit `:System Folder:Preferences:dropbear:passwd`
   - Format: `username:password` (one per line)
   - If no file exists, any password works for "root"

3. **Add SSH Keys** (recommended)
   - Add public keys to `:System Folder:Preferences:dropbear:authorized_keys`
   - One key per line, OpenSSH format

4. **Start the Server**
   - Double-click "Dropbear SSH"
   - Or add to Startup Items for auto-start

5. **Connect**
   ```bash
   ssh root@your-mac.local -p 22
   ```

## Startup Item

To start SSH automatically on boot:

1. Create an alias to "Dropbear SSH"
2. Place alias in `System Folder:Startup Items:`
3. Reboot

## Uninstallation

1. Quit Dropbear SSH
2. Delete `System Folder:Extensions:POSIX9 SSH:` folder
3. Delete `System Folder:Preferences:dropbear:` folder
4. Remove any Startup Items alias

## Troubleshooting

**Server won't start**
- Check Extensions Manager for conflicts
- Ensure Open Transport is enabled
- Check for port 22 conflicts

**Can't connect**
- Verify IP address: Apple Menu → Control Panels → TCP/IP
- Check firewall settings
- Try: `ssh -v root@ip-address`

**Authentication fails**
- Check passwd file format
- Try public key authentication
- Check dropbear.log for errors

## Support

- GitHub: https://github.com/Scottcjn/posix9
- Issues: https://github.com/Scottcjn/posix9/issues
