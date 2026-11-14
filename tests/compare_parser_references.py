#!/usr/bin/env python3
"""Compare ASCII preprocessing against local NetTTS and VoxTalk sources.

References are read-only. Temporary host executables do not use either engine.
This compares FlexTalk marker output, not the Mac speech-command translation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

CASES = [
    "", " ", "Hello there.", "The time is 8:47 AM.",
    "On behalf of The Board, please wait.", "Now boarding.",
    "Please stand by.", "A reminder to all personnel.",
    "Good morning everyone.", "Good evening everyone.",
    "This automated train is arriving.", "This tram is departing.",
    "The Board is ready.", "the apple is on the table.",
    "Welcome to the Black Mesa Research Facility.",
    "At 08:00 hours, report to Sector C.", "The time is 12:05 PM.",
    "Meet at 3:15 am.", "It is 1300 hours.",
    "Temperature is 100 degrees.", "It is 23 degrees Celsius.",
    "123 456 789 100 101 999 1000.",
    "A B C D E F G.", "Sector A and Sector B.",
    "One, two; three: four.", "Stop! Wait? Go.",
    'He said "Hello." Then left.', "Wait -- please stand by.",
    "Alpha - beta and gamma or delta.",
    "This is a test of the emergency broadcast system.",
    "one\ttwo\nthree", "...", "?!", "don't can't won't",
]


def run(args, **kw):
    return subprocess.run(args, check=True, **kw)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--nettts", type=Path, required=True)
    parser.add_argument("--voxtalk", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sources = {"macvox": root / "src/vox_parser.c",
               "nettts": args.nettts / "src/vox_parser.cpp",
               "voxtalk": args.voxtalk / "src/vox_parser.cpp"}
    report = {"scope": "ASCII preprocessing only, both wrapper settings",
              "sha256": {k: hashlib.sha256(p.read_bytes()).hexdigest()
                         for k, p in sources.items()}, "differences": []}
    with tempfile.TemporaryDirectory(prefix="macvox-parity-") as temp:
        work = Path(temp)
        for name, source in sources.items():
            if name == "macvox":
                code = '''#include "vox_parser.h"
#include <stdio.h>
#include <stdlib.h>
int main(int n, char **v) {
    char *s = vox_process_buffer(v[1], atoi(v[2]));
    if (!s) return 2;
    fputs(s, stdout); free(s); return 0;
}
'''
                compiler = ["cc", "-DVOX_PARSER_STANDALONE"]
                suffix = ".c"
            else:
                expression = ("std::wstring w; for (unsigned char c : std::string(v[1])) w += c; "
                              "auto s = vox_process(w, std::atoi(v[2]) != 0); "
                              "for (wchar_t c : s) std::cout << char(c);") if name == "nettts" else (
                                  "std::cout << voxtalk::vox_process(v[1], std::atoi(v[2]) != 0);")
                code = '#include "vox_parser.hpp"\n#include <iostream>\n#include <cstdlib>\n' + (
                    'int main(int n, char **v) { ' + expression + ' return 0; }\n')
                compiler = ["c++", "-std=c++17"]
                suffix = ".cpp"
            driver = work / (name + suffix)
            driver.write_text(code)
            run(compiler + ["-O1", "-I", str(source.parent), str(driver),
                            str(source), "-o", str(work / name)])
        for text in CASES:
            for wrap in (0, 1):
                outputs = {}
                for name in sources:
                    result = subprocess.run([str(work / name), text, str(wrap)],
                                            capture_output=True)
                    if name == "macvox" and result.returncode == 2:
                        outputs[name] = None
                    else:
                        result.check_returncode()
                        outputs[name] = result.stdout.decode("ascii")
                if len(set(outputs.values())) != 1:
                    report["differences"].append({"input": text, "wrap": wrap,
                                                  "outputs": outputs})
    report["comparisons"] = len(CASES) * 2
    report["matching"] = report["comparisons"] - len(report["differences"])
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(f'{report["matching"]}/{report["comparisons"]} agree across all three parsers; '
          f'{len(report["differences"])} differ')
    return bool(report["differences"])


if __name__ == "__main__":
    raise SystemExit(main())
