#!/usr/bin/env python3
"""Extract JSON verdict from evaluator response.

Handles markdown code blocks and finds the first valid JSON object
containing a "verdict" field.

Usage: extract_json.py <input_file>
Output: JSON on stdout, exit 0 on success, exit 1 on failure
"""

import sys
import re
import json


def extract_json(text: str) -> dict | None:
    """Extract JSON object containing 'verdict' from text."""
    # Strip markdown code blocks
    text = re.sub(r'```json\s*', '', text)
    text = re.sub(r'```\s*', '', text)

    # Find balanced braces containing "verdict"
    depth = 0
    start = -1

    for i, c in enumerate(text):
        if c == '{':
            if depth == 0:
                start = i
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0 and start >= 0:
                candidate = text[start:i+1]
                if '"verdict"' in candidate:
                    try:
                        return json.loads(candidate)
                    except json.JSONDecodeError:
                        pass
                start = -1

    return None


def main():
    if len(sys.argv) != 2:
        print("Usage: extract_json.py <input_file>", file=sys.stderr)
        sys.exit(2)

    try:
        with open(sys.argv[1], 'r') as f:
            text = f.read()
    except FileNotFoundError:
        print(f"File not found: {sys.argv[1]}", file=sys.stderr)
        sys.exit(2)

    result = extract_json(text)

    if result:
        print(json.dumps(result))
        sys.exit(0)
    else:
        print('{"error": "no valid JSON found"}')
        sys.exit(1)


if __name__ == '__main__':
    main()
