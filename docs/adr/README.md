# Architecture Decision Records (ADRs)

An ADR captures a single significant decision: the context, the choice made, and
its consequences. They are immutable once accepted — if a decision changes, we
add a new ADR that supersedes the old one rather than editing history. This lets
future-you (and contributors) understand *why* the project is the way it is.

Format: see [`0000-template.md`](0000-template.md).

## Index

| ADR | Title | Status |
|-----|-------|--------|
| [0001](0001-split-logic-from-hardware.md) | Split pure logic from hardware code | Accepted |
| [0002](0002-native-gcc-for-host-coverage.md) | Use native gcc (not the IDF linux target) for host tests & coverage | Accepted |
