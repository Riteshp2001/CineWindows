<!-- graft:start -->
## Graft - repo context graph

This repo is indexed in `graft/`: small linked markdown nodes that explain each
system and carry exact file:line spans, kept in sync with the code through git.

For ANY task here - understanding how something works, finding where code lives,
or scoping a change - get context from the graph before grepping or opening
source files. Re-ask freely (it's cheap) and reuse literal identifiers you
already have (symbol, error string, file name) as the query. New to this repo?
Run `graft map` first - a token-budgeted orientation (dir clusters, hubs,
hotspots), no LLM, no key.

- Run `graft ask "<your question>" --source` → ranked nodes with the relevant
  code spans inlined (each hit's ≤8-line crux by default; `--full` for whole
  definitions when the crux isn't enough). Match the tool to the task shape:
  for understanding or editing, the top node IS the answer - cite its
  `covers:` file:line spans and edit straight from `--source`. For
  exhaustive tasks ("every occurrence / every caller of this pattern"), ranked
  results are top-N, not complete - run `graft grep "<literal>"` instead
  (exhaustive over indexed files, grouped by enclosing symbol), falling back
  to raw `grep -rn` only for unindexed files.
- `graft skeleton <file>` → every definition's signature + span, ~10× cheaper
  than reading the file; use it to skim an API surface.
- `graft callers <symbol>` gives precomputed, exact edges - who calls this.
  Add `--direction out` for what it calls, or `--depth N` to walk
  transitively for the full blast radius. For structural questions, skip
  ranking and use this directly.
- Or browse: `graft/INDEX.md` lists every node; follow the links.
- Monorepos and folders of multiple repos rank fairly across sub-projects -
  hits carry `[scope/]` labels naming which one they're from. Narrow with
  `graft ask "<task>" --in <scope>/` once you know where you're working.

If a returned span is truncated ("+N more lines"), open the file at that exact
range before finalizing. Only open source files when a node genuinely lacks a
needed detail, and then at the exact file:line the node points to - never
re-read whole files.

After big code changes, refresh the graph with `graft build` (deterministic,
no API key, $0).
<!-- graft:end -->

## Global Agent Instructions

- Never use the em dash character. Use a plain hyphen "-" instead.
- When writing commit messages, never add the agent name as a co-author.
- Never manually modify `CHANGELOG.md` files or files marked as auto-generated. Update the source or generation process instead.
- When making technical decisions, prioritize quality, simplicity, robustness, scalability, and long-term maintainability over development cost.
- For one-off or infrequent operational work, use the simplest direct end-to-end path. Do not add wrappers, control planes, policy layers, custom verifiers, or automation unless the direct path exposes a concrete blocker or repeated need that justifies it.
- For bug fixes, first reproduce the bug in an end-to-end setting that closely matches the end-user experience. Use that reproduction to guide the fix and verify the result.
- When end-to-end testing a product, inspect the UI carefully and pursue pixel-level quality. Fix clearly visible issues encountered along the way when they are within the affected surface and can be addressed without destabilizing the task.
- Apply the same engineering standard to lint failures, test failures, and test flakiness. Investigate and fix issues found during validation when they are actionable and within the affected surface.
- Before using dynamic workflows, ultra code, or any harness feature that immediately spawns a large swarm of subagents, explain the tradeoffs and ask the user for explicit approval.

### Maintaining This File

- Keep this file focused on knowledge useful to almost every future agent session in this project.
- Do not repeat what the codebase already shows. Point to the authoritative file or command instead.
- Prefer rewriting or pruning existing entries over appending new ones.
- Keep entries concise and preserve this standard when updating the file.
