# Security Policy

## Scope

The Epistemic Framework consists of documentation and Claude Code command scripts. It does not process untrusted input or handle sensitive data. Security considerations are minimal.

## Supported Versions

Only the latest version on the `master` branch is actively maintained.

## Reporting a Vulnerability

If you discover a security issue (e.g., command injection in shell scripts, unsafe file operations), please report it by:

1. **Opening a GitHub issue** if the vulnerability is not sensitive
2. **Emailing the maintainer directly** for sensitive security issues (see GitHub profile for contact)

Please include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

## Response Timeline

As this is a research project maintained by one person:
- Acknowledgement: Within 1 week
- Initial assessment: Within 2 weeks
- Fix timeline: Depends on severity and complexity

## Security Considerations

The Claude Code commands (`/epistemic`, `/epistemic-discovery`, `/epistemic-recovery`) have limited tool access as defined in their YAML frontmatter. Review the `allowed-tools` field in each command file to understand capabilities.

The framework is designed for collaborative human-AI work, not for autonomous operation or handling untrusted projects.
