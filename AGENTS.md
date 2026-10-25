# EQ2 Engine agent instructions

This is the canonical policy for every coding agent working in this repository. Tool-specific
instruction files must import or point here instead of copying these rules. If guidance conflicts,
the user's current request wins, followed by this file, then the subsystem documentation.

## Agent prerequisites and human ownership

- **Agents must not create or submit pull requests, or reply to reviewer questions.** Prepare local
  changes and validation results for the human author instead.
- The human author must fully understand every submitted code change, including its behavior,
  evidence, compatibility constraints and validation. The human creates the pull request, answers
  review questions and takes responsibility for the submission; agent assistance does not transfer
  that responsibility.
  
## Mandatory preflight

Before editing:

1. Read this file completely.
2. Run `git status --short`. This is a shared, usually dirty worktree; assume every existing change
   belongs to the user or another agent.
3. Read `docs/building.md`, `docs/codestyle.md` and the relevant parts of `docs/architecture.md`.
4. Check whether another agent is modifying the same files. Partition work or coordinate before
   editing overlapping code.
5. At the start of each new task in a long-running session, and always right before committing,
   check whether this file or the docs you rely on changed since you read them
   (`git status --short -- AGENTS.md docs/`, `git log -1 --format=%h -- AGENTS.md docs/`).
   Re-read any that changed and follow the current version; other agents may update the rules
   in parallel sessions.

## Commits

- Commit only when the user asks. Write commit messages following `docs/commit_messages.md`.
- Before running `git commit`, prepare the commit following "Review before committing" in
  `docs/commit_messages.md` (check the index, stage only the task files or use
  `git commit --only` when others have staged files), show the user the file list and the full
  commit message, and commit only after the user confirms.

## Validation

- TODO
- run appropriate tests if test projects are available for covered source parts.

## Documentation

- Before working with an engine system, find its overview in the "Related documents" table of
  `docs/architecture.md` and read it to learn which systems, classes and entry points to use
  (for example `docs/physics_overview.md` for `shared/physics`). Check the code when the
  document and the code disagree.



