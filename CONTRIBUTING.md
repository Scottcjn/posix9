# Contributing

Thanks for helping improve POSIX9. This project targets classic Mac OS
environments, so compatibility and careful documentation are important.

## Local Setup

Clone the repository:

```bash
git clone https://github.com/Scottcjn/posix9.git
cd posix9
```

Review the README and existing scripts before changing behavior.

## Contribution Guidelines

- Preserve compatibility with the documented classic Mac targets.
- Avoid adding dependencies that are unavailable on the target systems.
- Keep scripts and instructions reversible where possible.
- Update documentation when changing setup, compatibility, or troubleshooting
  behavior.
- Prefer small pull requests focused on one compatibility or documentation goal.

## Validation

For documentation-only changes:

```bash
git diff --check
```

For code or script changes, include the exact command, host platform, and target
environment used for testing.

## Pull Request Checklist

- Summarize the compatibility impact.
- Include validation commands and environment details.
- Note any untested target hardware or OS version.
- Link the related issue or bounty, if applicable.
