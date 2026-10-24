"""Fetches the public libmagic test corpus (file/file on GitHub, tests/*.testfile with
their .result descriptions) into a folder, for Udm.NativeTests.exe --sniff-corpus <folder>.
The corpus is test input only; UDM's signature table is written from format specifications.

    python -I tools/fetch-libmagic-corpus.py <folder>
"""
import json, pathlib, subprocess, sys, urllib.request

if len(sys.argv) != 2:
    sys.exit(__doc__)
target = pathlib.Path(sys.argv[1])
target.mkdir(parents=True, exist_ok=True)
with urllib.request.urlopen("https://api.github.com/repos/file/file/contents/tests", timeout=60) as response:
    listing = json.load(response)
wanted = [entry for entry in listing if entry["name"].endswith((".testfile", ".result"))]
args = ["curl", "-sL", "--parallel", "--parallel-max", "16"]
for entry in wanted:
    args += ["-o", str(target / entry["name"]), entry["download_url"]]
subprocess.run(args, check=True)
print("%d files in %s" % (len(wanted), target))
