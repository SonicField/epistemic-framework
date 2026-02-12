---
description: "NBS Poll: Check chats and workers"
allowed-tools: Bash, Read
---

# NBS Poll

Lightweight periodic check of active chats and workers. Designed to be injected automatically by `nbs-claude` when idle, but can also be invoked manually.

## Behaviour

1. **Check active chats** — scan `.nbs/chat/*.chat` for unread messages (messages since your last post)
2. **Check active workers** — scan `.nbs/workers/*.md` for completed or stalled workers
3. **Report briefly** — if nothing new, say nothing and return silently
4. **Act if needed** — if there are unread chat messages, read and respond. If workers have completed, capture 3Ws.

## Instructions

Run the following checks. Be brief — this is a heartbeat, not a review.

### 1. Check Chats

```bash
# Find all chat files
ls .nbs/chat/*.chat 2>/dev/null

# For each chat file, check for messages since your last post
# Use your handle (typically "claude" or your worker name)
nbs-chat read <file> --since=<your-handle>
```

If there are unread messages, read them and respond appropriately via `nbs-chat send`. When instructions arrive via chat, always send clarifications and responses back to the same chat file — do not respond only in the terminal.

### 2. Check Workers

```bash
# Find active worker files
ls .nbs/workers/*.md 2>/dev/null
```

For each worker file, check the Status field. If a worker has completed:
- Read the results
- Capture 3Ws in supervisor.md (if you are a supervisor)
- Decide on next steps

### 3. Report

- If nothing new was found, output nothing — return silently to the user's conversation.
- If you found unread messages or completed workers, briefly summarise what you found and what you did about it.
- Do NOT interrupt the user's flow with lengthy reports. One or two sentences maximum.

## Important

- This skill may be injected automatically. Do not be surprised if it appears mid-session.
- Be fast. Check, act if needed, return. Do not start new work from a poll.
- If a chat message requires significant work, note it and return to the user — do not silently start a large task.
