# Contributing to Epistemic Framework

Thank you for your interest in the Epistemic Framework!

## Project Status

This is a research project exploring how to externalise epistemic standards for human-AI collaboration. Development is driven by real-world use and iterative refinement.

## How to Contribute

### Reporting Issues

If something doesn't work or doesn't make sense, open an issue with:
- A clear, descriptive title
- What you were trying to do
- What happened vs what you expected
- Context (which command, what project type)

### Suggesting Improvements

Feature requests and improvements are welcome. Please open an issue describing:
- The problem or gap you've encountered
- How you think it should work
- Why it serves the framework's terminal goal (better human-AI collaboration)

### Pull Requests

Pull requests are welcome, especially for:
- Bug fixes in test scripts or commands
- Documentation improvements
- Additional test scenarios
- New pillar documents (if they capture a genuine epistemic principle)

Before submitting a large PR, open an issue first to discuss the approach.

**PR Guidelines:**
- All tests must pass (`tests/automated/test_*.sh`)
- Add tests for new functionality
- Write in the project's style (see `STYLE.md` for internal reference)
- Update documentation as needed
- Use British English

## Development Setup

```bash
# Clone the repository
git clone https://github.com/SonicField/epistemic-framework.git
cd epistemic-framework

# Install commands
./bin/install.sh

# Run tests
./tests/automated/test_install.sh
./tests/automated/test_epistemic_command.sh
./tests/automated/test_epistemic_discovery.sh
./tests/automated/test_epistemic_recovery.sh
```

## Testing Philosophy

Tests evaluate AI output using a second AI instance. The verdict file is the deterministic state of truth. See `tests/automated/test_epistemic_command.sh` for the pattern.

## Questions?

Open an issue for questions about the framework's design, usage, or philosophy.

## Licence

By contributing, you agree that your contributions will be licensed under the MIT License.
