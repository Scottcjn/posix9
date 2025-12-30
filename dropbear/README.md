# Dropbear SSH for Mac OS 9

This directory contains the port of Dropbear SSH server to Mac OS 9 using the POSIX9 compatibility layer.

## About Dropbear

Dropbear is a lightweight SSH server and client, designed for embedded systems. It's perfect for Mac OS 9 due to:
- Small footprint (~110KB binary)
- Minimal dependencies
- Configurable feature set
- BSD-style license

## Building

### Prerequisites

1. **Dropbear source** - Download from https://matt.ucc.asn.au/dropbear/dropbear.html
2. **POSIX9 library** - Build libposix9.a first
3. **Toolchain** - Either:
   - CodeWarrior Pro 8+ for PPC
   - Retro68 cross-compiler

### Steps

1. Download and extract Dropbear source:
   ```bash
   wget https://matt.ucc.asn.au/dropbear/releases/dropbear-2024.86.tar.bz2
   tar xf dropbear-2024.86.tar.bz2
   cd dropbear-2024.86
   ```

2. Apply OS 9 patches:
   ```bash
   patch -p1 < ../posix9/dropbear/patches/os9-compat.patch
   ```

3. Copy OS 9 configuration:
   ```bash
   cp ../posix9/dropbear/os9/localoptions.h .
   cp ../posix9/dropbear/os9/os9_platform.h .
   cp ../posix9/dropbear/os9/os9_platform.c .
   ```

4. Build with Retro68:
   ```bash
   mkdir build-os9 && cd build-os9
   cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/retro68.cmake \
            -DPOSIX9_DIR=/path/to/posix9
   make
   ```

### With CodeWarrior

Import the provided `dropbear_os9.mcp` project file. Ensure:
- Include paths point to POSIX9 headers
- Link with libposix9.a
- Target is PowerPC with CFM

## Features Enabled

| Feature | Status | Notes |
|---------|--------|-------|
| SSH server (sshd) | ✅ | Main functionality |
| Password auth | ✅ | Via Open Transport password file |
| Public key auth | ✅ | RSA/DSA/ECDSA |
| Shell access | ⚠️ | Launches MPW Shell or configured app |
| SCP | ✅ | File transfer |
| SFTP | ❌ | Disabled (too complex) |
| Port forwarding | ❌ | Disabled (no fork()) |
| X11 forwarding | ❌ | No X11 on OS 9 |

## Disabled Features

These are disabled in `localoptions.h` due to OS 9 limitations:

- **ENABLE_X11FWD** - No X11
- **ENABLE_AGENTFWD** - No Unix domain sockets
- **ENABLE_LOCALTCPFWD** - Requires fork()
- **ENABLE_REMOTETCPFWD** - Requires fork()
- **ENABLE_SVR_REMOTETCPFWD** - Requires fork()
- **ENABLE_CLI_**** - Client features (we only build server)

## Configuration

Edit `/System Folder/Preferences/dropbear/` files:

- `authorized_keys` - Public keys for key-based auth
- `dropbear_rsa_host_key` - Server RSA key
- `dropbear_dss_host_key` - Server DSA key
- `passwd` - User accounts (optional)

## First Run

1. Generate host keys (run once):
   ```
   dropbearkey -t rsa -f "Macintosh HD:System Folder:Preferences:dropbear:dropbear_rsa_host_key"
   ```

2. Start the server:
   - Double-click "Dropbear SSH" application
   - Or run from MPW: `dropbear -F -E`

3. Connect from another machine:
   ```bash
   ssh -o HostKeyAlgorithms=+ssh-rsa user@your-mac.local
   ```

## Shell Integration

When a user connects, Dropbear needs to run a shell. Options:

1. **MPW Shell** - Set `SHELL` env to MPW path
2. **ToolServer** - Lightweight command processor
3. **Custom app** - Your own command handler

Configure in Preferences:dropbear:config:
```
shell=/Applications/MPW Shell
```

## Security Notes

- No privilege separation (single user system)
- All users effectively have full access
- Use public key auth for security
- Consider firewall rules via Open Transport

## Troubleshooting

**"Connection refused"**
- Check if Dropbear is running
- Verify port 22 is not blocked

**"Host key verification failed"**
- Generate host keys first
- Or use `ssh -o StrictHostKeyChecking=no`

**Hangs after login**
- Shell not configured properly
- Check MPW Shell path

## License

Dropbear is MIT licensed. POSIX9 layer is MIT licensed.
