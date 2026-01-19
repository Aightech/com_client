# Dependencies

This project uses git submodules for dependency management. This document explains how to work with them.

## Quick Start

After cloning, run:
```bash
./scripts/setup.sh
```

Or manually:
```bash
git submodule update --init --recursive
```

## Common Operations

### Cloning the repository (new checkout)
```bash
git clone --recursive <repo-url>
```

Or if already cloned without `--recursive`:
```bash
git clone <repo-url>
cd <repo>
git submodule update --init --recursive
```

### Updating after git pull
When you pull changes that update submodule references:
```bash
git pull
git submodule update --recursive
```

### Checking submodule status
```bash
git submodule status
```
- A `-` prefix means not initialized
- A `+` prefix means the checked-out commit differs from the index

## Dependency Tree

```
com_client
└── lib/
    └── strANSIseq (terminal ANSI formatting)
```

## For Maintainers

### Updating a submodule to latest
```bash
cd lib/strANSIseq
git checkout main
git pull
cd ../..
git add lib/strANSIseq
git commit -m "Update strANSIseq"
```

### Adding a new dependency
```bash
git submodule add <repo-url> lib/<name>
git commit -m "Add <name> dependency"
```

### Removing a dependency
```bash
git submodule deinit lib/<name>
git rm lib/<name>
rm -rf .git/modules/lib/<name>
git commit -m "Remove <name> dependency"
```

## Troubleshooting

**CMake error: "Submodule 'X' not initialized"**
```bash
git submodule update --init --recursive
```

**Submodule in detached HEAD state**
This is normal. Submodules point to specific commits, not branches.

**Accidentally committed wrong submodule reference**
```bash
git checkout HEAD -- lib/<name>
git submodule update --recursive
```
