#!/usr/bin/env python3
"""
Trim unreachable code from a Hack assembly file by computing reachability
from the bootstrap and `Sys.init` entry point. Keeps labels, instructions
and removes function bodies that are never reached.

Usage: trim_asm.py input.asm output.asm
"""
import sys
import re

if len(sys.argv) != 3:
    print("Usage: trim_asm.py input.asm output.asm", file=sys.stderr)
    sys.exit(2)

infile = sys.argv[1]
outfile = sys.argv[2]

with open(infile, 'r', encoding='utf-8') as f:
    lines = f.readlines()

label_re = re.compile(r'^\(([^)]+)\)')
goto_re = re.compile(r'^@([A-Za-z0-9_.$:]+)\s*$')
jmp_re = re.compile(r'0;JMP|JGT|JLT|JEQ|JNE|JGE|JLE')
call_ret_re = re.compile(r'^// call\s+([A-Za-z0-9_.:$]+)\s+\d+')

# Map label -> line index
labels = {}
for i, ln in enumerate(lines):
    m = label_re.match(ln.strip())
    if m:
        labels[m.group(1)] = i

# Build adjacency: from line index, what labels/gotos/calls it can transfer to
targets = {}
# Map return labels to their call sites
return_label_to_call = {}
for i, ln in enumerate(lines):
    s = ln.strip()
    if s.startswith('@'):
        m = goto_re.match(s)
        if m and i+1 < len(lines):
            # next line might be a jump
            nxt = lines[i+1].strip()
            if nxt.endswith('0;JMP') or 'J' in nxt:
                tgt = m.group(1)
                if tgt in labels:
                    targets.setdefault(i, set()).add(labels[tgt])
    # detect call comments emitted by the VM translator to find function names
    mc = call_ret_re.match(s)
    if mc:
        func = mc.group(1)
        if func in labels:
            targets.setdefault(i, set()).add(labels[func])
            # Find the return label that comes after this call
            # The return label is placed after the call sequence (can be 50+ lines later)
            # Look for a label matching pattern: func$ret.N
            for j in range(i+1, min(i+60, len(lines))):
                ret_m = label_re.match(lines[j].strip())
                if ret_m:
                    label_name = ret_m.group(1)
                    # Check if this is a return label for this function
                    # Pattern: functionName$ret.number
                    if func in label_name and '$ret.' in label_name:
                        ret_label = label_name
                        return_label_to_call[ret_label] = i
                        # Mark that when this call is made, the return label is also reachable
                        # (execution will continue there after the function returns)
                        targets.setdefault(i, set()).add(j)
                        break

# Also consider explicit -- entry points: bootstrap and Sys.init label
entry_labels = set()
if 'Sys.init' in labels:
    entry_labels.add(labels['Sys.init'])
# also keep any return labels that start from a call comment usage
for i, ln in enumerate(lines):
    if ln.strip().startswith('// Bootstrap Code'):
        entry_labels.add(i)
        break

# BFS over lines and labels: if a label is reached, mark from that line forward
visited_lines = set()
from collections import deque
q = deque()
for el in entry_labels:
    q.append(el)

while q:
    idx = q.popleft()
    if idx in visited_lines:
        continue
    visited_lines.add(idx)
    
    # Check if this is a return statement (jumps to R14, then A=M, then 0;JMP)
    s = lines[idx].strip() if idx < len(lines) else ""
    if s == "@R14" and idx+2 < len(lines):
        nxt1 = lines[idx+1].strip() if idx+1 < len(lines) else ""
        nxt2 = lines[idx+2].strip() if idx+2 < len(lines) else ""
        if nxt1 == "A=M" and nxt2 == "0;JMP":
            # This is a return statement - mark all return labels from visited calls as reachable
            for ret_label, call_site in return_label_to_call.items():
                if call_site in visited_lines and ret_label in labels:
                    ret_idx = labels[ret_label]
                    if ret_idx not in visited_lines:
                        q.append(ret_idx)
    
    # from this line, follow explicit targets
    for tgt in targets.get(idx, ()): 
        if tgt not in visited_lines:
            q.append(tgt)
    
    # also step to next line (fall-through) unless it's a jump instruction
    if idx+1 < len(lines):
        nxt = lines[idx+1].strip()
        # Check if next line is a label - if so, we can fall through to it
        if label_re.match(nxt):
            # It's a label, we can fall through
            q.append(idx+1)
        elif not re.search(r'0;JMP|;J', nxt):
            # Not a jump, can fall through
            q.append(idx+1)
        # If it is a jump, we don't fall through, but we might have already
        # added the target via the targets map above

# Write output keeping only visited lines and any labels referenced
with open(outfile, 'w', encoding='utf-8') as out:
    for i, ln in enumerate(lines):
        if i in visited_lines:
            out.write(ln)

print(f'Wrote trimmed asm to {outfile}. Lines kept: {len(visited_lines)}')
