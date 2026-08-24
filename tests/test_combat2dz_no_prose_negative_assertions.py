#!/usr/bin/env python3
from pathlib import Path
import ast
import re

root = Path(__file__).resolve().parents[1]
sh = (root/'termux_fix39_build.sh').read_text(errors='ignore')
work_tests = sorted(set(re.findall(r'python "\$ROOT/(tests/[^"]+\.py)" "\$WORK"', sh)))
problems=[]
for rel in work_tests:
    path=root/rel
    tree=ast.parse(path.read_text(errors='ignore'), filename=str(path))
    for node in ast.walk(tree):
        if not isinstance(node, ast.Assert):
            continue
        test=node.test
        if not (isinstance(test, ast.Compare) and len(test.ops)==1 and isinstance(test.ops[0], ast.NotIn)):
            continue
        left=test.left
        if not (isinstance(left, ast.Constant) and isinstance(left.value, str)):
            continue
        value=left.value.strip()
        # Prose/comment archaeology is not a semantic runtime invariant.
        # Code/symbol tokens (punctuation, identifiers, paths) remain allowed.
        words=value.split()
        code_punct=set('(){}[];=<>/._-&|*+!')
        if len(words) >= 4 and not any(ch in value for ch in code_punct):
            problems.append(f'{rel}:{getattr(node, "lineno", 0)} negative prose assertion: {value!r}')
assert not problems, '\n'.join(problems)
print(f'Combat2DZ post-integration prose-negative assertion sweep: PASS ({len(work_tests)} WORK tests)')
