# CI and security invariants

Do not weaken these without a deliberate decision — they protect a PUBLIC
repo that uses a self-hosted runner.

## CI gating (`.github/workflows/ci.yml`)

- The `on-target` job runs on a self-hosted ESP32 runner. It MUST stay gated
  to: `push` events, `main` branch, and `github.repository ==
  'AbbyStrebel/sensor-pipeline'`. Never make it run on `pull_request` or from
  forks — that would let untrusted code execute on the runner.
- Default workflow permission is `contents: read`. Only the tag-triggered
  `release` job may opt up to `contents: write`, scoped to that job alone.
  Do not grant write permissions workflow-wide.
- Keep third-party actions pinned to a commit SHA (e.g. the release action),
  not a floating tag.

## Secrets

- Never commit signing or private keys. `*.pem` / `*.key` are gitignored as a
  backstop, but the real rule is: keys live in CI secrets, not the repo.
- On-device secrets belong in encrypted NVS (Phase 5), never in source.
