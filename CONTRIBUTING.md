# Contributing

This is a learning project, but it follows a real engineering workflow on
purpose. This document is the source of truth for *how* we make changes.

## Branch / PR / CI workflow

We never commit directly to `main`. Every change goes through a feature branch
and a pull request:

1. **Branch off `main`:**
   ```sh
   git switch main && git pull
   git switch -c feat/<short-topic>      # or fix/<topic>, docs/<topic>
   ```
2. **Make the change**, with its tests/docs, in focused commits.
3. **Verify locally before pushing** (see "Local checks" below).
4. **Push and open a PR:**
   ```sh
   git push -u origin feat/<short-topic>
   ```
   Then open the PR against `main` on GitHub.
5. **CI runs automatically** on the PR (see `.github/workflows/ci.yml`):
   firmware build, host unit tests, AddressSanitizer/UBSan, valgrind, and
   coverage. All checks must pass.
6. **Merge when green.** Keep `main` always releasable.

Branch name prefixes: `feat/`, `fix/`, `docs/`, `chore/`.

## Developer tooling

Beyond ESP-IDF, the linting/testing gate uses these tools. Install them once so
your local checks match CI.

| Tool | Purpose | Install (Arch) |
|------|---------|----------------|
| clang-format | C code formatting | `sudo pacman -S clang` |
| clang-tidy | Static analysis | `sudo pacman -S clang` |
| cppcheck | Static analysis (second opinion) | `sudo pacman -S cppcheck` |
| pre-commit | Runs the fast checks before each commit | `sudo pacman -S pre-commit` |
| doxygen | API docs | `sudo pacman -S doxygen` |
| valgrind, gcovr | Host memory check / coverage | `sudo pacman -S valgrind gcovr` |

> On non-Arch systems, use your package manager's equivalents; `clang-format`
> and `clang-tidy` ship in the LLVM/clang package. CI installs all of these in
> the `espressif/idf` container, so the versions there are the source of truth.

**Enable the pre-commit hooks (once per clone):**

```sh
pre-commit install          # installs the git hook
pre-commit run --all-files  # run every hook against the whole tree
```

After `pre-commit install`, the hygiene + clang-format + cppcheck hooks run
automatically on every `git commit`. (clang-tidy is not a pre-commit hook — it
needs a full build + compile database — so it runs in CI only.)

## Local checks (run before pushing)

```sh
# Firmware builds (warnings are errors for our code):
. "$IDF_PATH/export.sh" && idf.py build

# Host unit tests, sanitizers, valgrind, coverage:
make -C test_host test
make -C test_host asan
make -C test_host valgrind
make -C test_host coverage

# Lint gate (static analysis + formatting):
pre-commit run --all-files   # clang-format + cppcheck + hygiene
./tools/cppcheck.sh          # cppcheck alone
idf.py build && ./tools/clang-tidy.sh   # clang-tidy (needs the compile DB)

# API docs (optional locally; built in CI):
make docs
```

If all of these pass locally, CI will almost certainly pass too.

## Code standards

- **Warnings are errors** for first-party code (`-Wall -Wextra -Werror`, scoped
  to our components, not ESP-IDF's).
- **Keep logic out of hardware code.** Pure logic goes in `processor` (no IDF
  deps) so it stays host-testable. See
  [ADR-0001](docs/adr/0001-split-logic-from-hardware.md).
- **Format** with the repo `.clang-format`.

## Documentation standards

We use Doxygen (`/** ... */`) comments on the public API. The standard:

- **Every file** has an `@file` and a one-line `@brief`.
- **Every public function** (in a header) documents its `@param`s and
  `@return`, plus any important preconditions or ownership rules.
- **Do not over-document stubs.** A placeholder component that does not do
  anything yet gets only `@file`/`@brief` and a short note of what it *will*
  do. Full API docs are written when the real behavior lands — documenting
  empty stubs in detail just creates comments that go stale. (For example,
  `sampler`/`reporter`/`app_core` stay lightly documented until their
  FreeRTOS behavior is implemented.)
- Record significant decisions as an ADR under `docs/adr/` rather than burying
  the rationale in a commit message.

## Why this much process for a learning project?

Practicing the workflow *is* one of the goals. Doing it on a small, low-stakes
project builds the habits (small PRs, green CI before merge, decisions written
down) that matter on larger ones.
