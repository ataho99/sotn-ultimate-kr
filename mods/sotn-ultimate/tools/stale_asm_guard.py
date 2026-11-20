#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
# ninja does not track the .s files that INCLUDE_ASM pulls in: when patch_status_asm.py (or anything else)
# rewrites a .s after its object was built, the object stays stale.  Delete every object whose INCLUDE_ASM
# .s is newer than the .o so the next ninja run recompiles it.  Run from the tree root.
#   stale_asm_guard.py          delete stale objects
#   stale_asm_guard.py --check  list only (exit 1 if any)
import glob
import os
import re
import sys

pat = re.compile(r'INCLUDE_ASM\("([^"]+)",\s*(\w+)\)')
check = "--check" in sys.argv
stale = []
for src in glob.glob("src/**/*.c", recursive=True):
    text = open(src, errors="ignore").read()
    refs = pat.findall(text)
    if not refs:
        continue
    obj = "build/us/" + src + ".o"
    if not os.path.exists(obj):
        continue
    om = os.path.getmtime(obj)
    for d, n in refs:
        s = "asm/us/%s/%s.s" % (d, n)
        if os.path.exists(s) and os.path.getmtime(s) > om:
            stale.append((obj, s))
            break
for obj, s in stale:
    print("stale:", obj, "<-", s)
    if not check:
        os.remove(obj)
print("stale_asm_guard: %d stale object(s)%s" % (len(stale), "" if check else " removed"))
sys.exit(1 if (check and stale) else 0)
