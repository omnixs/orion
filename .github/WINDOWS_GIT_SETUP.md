# Windows Git Configuration for Line Endings

## Problem
Windows uses CRLF (`\r\n`) for line endings, while Linux/macOS use LF (`\n`). This causes issues:
- Entire files show as changed in PRs when only line endings differ
- Makes code review difficult
- Can cause build issues across platforms

## Solution
Configure Git on Windows to automatically convert line endings.

---

## One-Time Setup on Windows

### Option 1: Global Configuration (Recommended)

Run these commands in PowerShell or Git Bash **once** on your Windows machine:

```powershell
# Configure Git to checkout files with LF, commit with LF
git config --global core.autocrlf input

# Ensure files are normalized in the repo
git config --global core.eol lf
```

**What this does:**
- `core.autocrlf input` - Converts CRLF → LF when committing (but keeps LF when checking out)
- `core.eol lf` - Uses LF line endings in the working directory

### Option 2: Per-Repository Configuration

If you prefer to configure only for this repository:

```powershell
cd C:\path\to\orion
git config core.autocrlf input
git config core.eol lf
```

---

## Verify Configuration

Check your current settings:

```powershell
git config --global core.autocrlf
git config --global core.eol
```

Expected output:
```
input
lf
```

---

## Re-normalize Existing Files (After Configuration)

If you already have files checked out with CRLF:

```powershell
# Remove all files from Git's index (keeps them in working directory)
git rm --cached -r .

# Re-add them with the new line ending settings
git add .

# Check what changed (should show line ending normalization)
git diff --cached

# If everything looks good, reset (don't commit these changes)
git reset
```

Alternatively, just delete and re-clone the repository:

```powershell
cd C:\path\to\parent
rm -r -force orion
git clone https://github.com/omnixs/orion.git
cd orion
git config core.autocrlf input
git config core.eol lf
```

---

## Visual Studio Code Configuration

Add to your VS Code `settings.json` (File → Preferences → Settings → search for "eol"):

```json
{
    "files.eol": "\n"
}
```

Or use the VS Code UI:
1. Open Settings (Ctrl+,)
2. Search for "End of Line"
3. Set to `\n` (LF)

---

## How .gitattributes Helps

The `.gitattributes` file in the repository root enforces line endings **regardless of your local Git config**:

```gitattributes
# All text files use LF
* text=auto eol=lf

# Source files explicitly use LF
*.cpp text eol=lf
*.hpp text eol=lf
```

This means:
- ✅ Even if you forget to configure Git, `.gitattributes` ensures LF in commits
- ✅ Works for all contributors automatically
- ✅ Prevents accidental CRLF commits

---

## Troubleshooting

### "I still see CRLF warnings"

```powershell
# Check if files have CRLF locally
git ls-files --eol

# Should show "i/lf w/lf" for most files
# If you see "i/crlf w/crlf", re-normalize:
git add --renormalize .
```

### "Git says 'LF will be replaced by CRLF'"

This warning appears when `core.autocrlf` is set to `true` (the Windows default). Change it:

```powershell
git config --global core.autocrlf input
```

### "How do I check line endings in a file?"

PowerShell:
```powershell
(Get-Content file.cpp -Raw) -match "`r`n"  # True if CRLF, False if LF
```

Git Bash:
```bash
file file.cpp  # Shows "CRLF" or "LF"
```

---

## Summary

**For all Windows developers:**

1. **One-time setup:**
   ```powershell
   git config --global core.autocrlf input
   git config --global core.eol lf
   ```

2. **VS Code setting:**
   ```json
   "files.eol": "\n"
   ```

3. **Verification:**
   ```powershell
   git config --global --list | Select-String "autocrlf|eol"
   ```

With these settings, you'll never introduce Windows line endings again! 🎉

---

## References

- [Git Documentation - gitattributes](https://git-scm.com/docs/gitattributes)
- [Git Configuration - core.autocrlf](https://git-scm.com/book/en/v2/Customizing-Git-Git-Configuration)
- [GitHub - Dealing with line endings](https://docs.github.com/en/get-started/getting-started-with-git/configuring-git-to-handle-line-endings)
