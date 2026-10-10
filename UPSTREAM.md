# Upstream provenance

This repository is a **fork of musl libc**, carried as the source tree for the
Dunit OS libc port (see `PORTING.md`). It is not vendored snippets — it is the
full upstream tree, so upstream bug/security fixes can be pulled and rebased.

| | |
|---|---|
| Upstream project | musl libc |
| Upstream URL | `https://git.musl-libc.org/git/musl` (remote `upstream`) |
| License | MIT (see `COPYRIGHT`) |
| Base release | **v1.2.6** |
| Base commit | `b1efda5b91735e33635376ca11c9b497a1c66e39` |
| `git describe` | `v1.2.6-67-gb1efda5b` |
| Forked on | 2026-10-10 |

## Tracking upstream

```sh
git fetch upstream
git rebase upstream/master        # replay the Dunit delta on new upstream
```

Any version bump must pass the full libc test gate before it is accepted (see
`PORTING.md`). Keep OS-specific changes minimal and never reformat upstream files,
so the Dunit delta stays small and reviewable.
