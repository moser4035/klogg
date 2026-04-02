# AGENTS.md

## Commit Message Assistant

When the user asks for a commit message, generate it from staged git changes (`git diff --cached`).

### Output format
Return plain text only.
Return exactly in this order:
1. One Conventional Commit subject line (max 72 chars)
2. Optional blank line
3. Optional body bullet list with key details

### Conventional Commits
Use this schema:
`type(scope): summary`

Allowed types:
- feat
- fix
- refactor
- perf
- docs
- test
- build
- ci
- chore
- revert

Rules:
- Subject in imperative mood
- Lowercase type
- No trailing period
- Keep subject concise and specific
- Prefer scope when clear (module/package/feature)

### Breaking changes
If applicable, use `!` in header and add a `BREAKING CHANGE:` note in the body.

### Jira / ticket integration
If a branch or staged diff indicates a ticket (e.g. `RDA-123`), prepend it to the subject summary:
`type(scope): RDA-123 short summary`
Do not invent ticket IDs.

### Language
Default language: English.
If the user explicitly requests German, output German text while keeping Conventional Commit syntax.

### Safety checks
Before finalizing, ensure:
- Message reflects staged changes only
- No unrelated assumptions
- No sensitive data

### If context is insufficient
If no staged changes exist, respond with:
`No staged changes found. Stage files first, then ask again.`
