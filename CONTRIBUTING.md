# Contributing to POSIX9

Thanks for contributing to POSIX9. The project targets Classic Mac OS through
Retro68, so portability and clear build notes are more important than modern
host assumptions.

## Setup

1. Fork the repository and create a branch:

   ```sh
   git checkout -b fix/short-description
   ```

2. Install or build Retro68 as described in `README.md` and `BUILDING.md`.
   The default scripts expect:

   ```sh
   export RETRO68_PREFIX="$HOME/Retro68/build/toolchain"
   ```

3. Build the PowerPC library when a Retro68 toolchain is available:

   ```sh
   ./build-ppc.sh
   ```

4. For 68K changes, run:

   ```sh
   ./build-68k.sh
   ```

5. To validate the sample program:

   ```sh
   cd test
   ./build-test.sh
   ```

## Pull Request Guidelines

- Keep changes focused on one subsystem, such as headers, Toolbox shims,
  Dropbear integration, installer files, or build scripts.
- State which target you built: PowerPC, 68K, or both.
- Include the Retro68 version or commit when reporting build behavior.
- Update `README.md` or `BUILDING.md` if commands, paths, or supported
  platforms change.
- Do not include generated `build-*` directories or compiled Classic Mac OS
  artifacts.

## Code Style

- Keep C interfaces small and compatible with Classic Mac OS constraints.
- Prefer explicit platform checks over assumptions about host Unix features.
- Maintain existing naming and header organization.
- Shell scripts should fail clearly when `RETRO68_PREFIX` or required tools are
  missing.
- Avoid changes that require runtime services unavailable on Classic Mac OS
  unless they are guarded and documented.

## Validation Checklist

- [ ] Relevant build script was run or the hardware/toolchain limitation is
  documented in the PR.
- [ ] Test program build was attempted for library changes.
- [ ] Documentation reflects new setup or compatibility requirements.
- [ ] Generated files and local toolchain paths are not committed.
- [ ] The PR explains the Classic Mac OS target affected by the change.
