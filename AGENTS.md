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

## Commits

- Commit only when the user asks. Write commit messages following `docs/commit_messages.md`.
- Before running `git commit`, show the user the list of files to be committed and the full commit
  message, and wait for their approval. Apply requested corrections and show the result again;
  commit only after the user confirms.

## Validation

- TODO
- run appropriate tests if test projects are available for covered source parts.

## Documentation

- TODO



