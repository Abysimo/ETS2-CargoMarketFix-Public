# Repository role policy

DO NOT USE THIS REPOSITORY FOR NEW DEVELOPMENT.

NEW DEVELOPMENT BELONGS IN Abysimo/ETS2-CargoMarketFix.

Repository: `Abysimo/ETS2-CargoMarketFix-Public`

Role: **PUBLIC RELEASE / DISTRIBUTION SNAPSHOT** — not the development source of truth.

This repository is PUBLIC following explicit user approval. Do not change visibility, create tags or publish releases/assets without explicit authorization for the task.

## Allowed release work

This repository receives only user-approved release-ready source snapshots, compatibility additions already developed and validated privately, public README/documentation, release configuration, approved release tags/assets, and minimal public-safe maintenance tests.

Do not perform reverse engineering, exploratory compatibility work, diagnostic development, experimental hooks, raw research, new algorithm development, WPR/ETL investigation, temporary probes or unpublished fix development here. Begin such work in `Abysimo/ETS2-CargoMarketFix`, complete and validate it there, then obtain explicit user approval before promotion.

If a task does not specify a repository, default to the PRIVATE DEVELOPMENT repository, never this repository. This applies to regressions, new fixes, changed RVAs and ports for ETS2 1.58, 1.59, 1.60 or later versions.

## One-way promotion

`PRIVATE DEVELOPMENT: Abysimo/ETS2-CargoMarketFix`

↓ **USER APPROVAL + SANITIZED RELEASE PROMOTION**

`PUBLIC RELEASE SNAPSHOT: Abysimo/ETS2-CargoMarketFix-Public`

The private development repository remains canonical. Never assume PUBLIC -> PRIVATE synchronization. Do not merge or copy this repository back into development unless the user explicitly requests it.

## Compatibility release checklist

1. Develop the port or fix in the private repository.
2. Validate the exact new executable and release safety/behavior requirements privately. Do not infer support from a major/minor version number alone.
3. Obtain user approval for a sanitized release promotion.
4. Update this repository's supported-version table, README compatibility information, release notes, release package contents and checksums wherever maintained. Include only public-safe source, documentation, configuration and maintenance tests. Scan staged content for secrets, personal data and excluded research artifacts before pushing.
5. Publish the matching release, tags and assets only when explicitly approved. Source promotion alone is not permission to change visibility or publish.

Use SUPPORTED / STRUCTURALLY VALIDATED when exact identity, sites, native maintenance and fail-closed checks pass but the original large-map reproduction setup is unavailable. Do not request an unavailable physical test or block a compatibility release solely on that basis. Reserve SUPPORTED / PRACTICALLY TESTED for actual practical evidence. Future unsupported versions remain unsupported until separately validated.

Do not import private Git history, raw executable/disassembly/memory dumps, logs, ETL, SDK/game assets, saves/profiles or local backups. Keep local build/package outputs untracked. Documentation-only work must not change gameplay source, build metadata or release configuration; do not rebuild or start a research test campaign unnecessarily.

License status: **MIT License, explicitly approved by the user**. See LICENSE. Release review and explicit user publication approval remain required for each release. Do not change the selected license without an explicit user decision.
