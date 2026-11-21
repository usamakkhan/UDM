"""Compare normal and private ordered UDM builds through actual process exit.

Each output is flushed and independently hashed before its owned payload is
removed. Catalogs and complete origin/accounting records remain for inspection.
No installation, registry changes, GUI automation or process termination.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import statistics
import subprocess
import time
import uuid

from origin import Origin, Options, expected_sha256

spec = importlib.util.spec_from_file_location("completion_utils", Path(__file__).with_name("compare-completion.py"))
utils = importlib.util.module_from_spec(spec)
spec.loader.exec_module(utils)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("normal", "ordered", "c-work", "d-work", "out"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--normal-sha256", required=True)
    parser.add_argument("--ordered-sha256", required=True)
    parser.add_argument("--trials", type=int, default=3)
    parser.add_argument("--protect", action="append", type=Path, default=[])
    args = parser.parse_args()
    assert 2 <= args.trials <= 5
    executables = {"normal": args.normal.resolve(), "ordered": args.ordered.resolve()}
    identities = {"normal": args.normal_sha256.upper(), "ordered": args.ordered_sha256.upper()}
    assert all(utils.sha(path) == identities[name] for name, path in executables.items())
    protected = {str(path.resolve()): utils.sha(path) for path in args.protect}
    work = {"C": args.c_work.resolve(), "D": args.d_work.resolve()}
    assert work["C"].drive.upper() == "C:" and work["D"].drive.upper() == "D:"
    assert not args.out.exists() and all(not path.exists() for path in work.values())
    args.out.mkdir(parents=True)
    for path in work.values():
        path.mkdir(parents=True)
        (path / ".udm-ordered-comparison").write_text("owned native comparison", encoding="utf-8")
    cases = [("single-4m", 4 * 1024**2, 1), ("single-64m", 64 * 1024**2, 1),
             ("split-64m", 64 * 1024**2, 8)]
    rows = []
    utils.save(args.out / "inputs-private.json", {
        "executables": {name: str(path) for name, path in executables.items()},
        "sha256": identities, "protectedFiles": protected,
        "work": {drive: str(path) for drive, path in work.items()},
        "harnessSha256": utils.sha(Path(__file__)),
        "utilitySha256": utils.sha(Path(__file__).with_name("compare-completion.py")),
        "originSha256": utils.sha(Path(__file__).with_name("origin.py")),
    })
    for trial in range(1, args.trials + 1):
        for drive_index, (drive, parent) in enumerate(work.items()):
            for case_index, (case, size, connections) in enumerate(cases):
                # Reverse first client across trials, cases and drives. Each
                # pair uses identical settings and a fresh origin/catalog.
                order = ["normal", "ordered"] if (trial + drive_index + case_index) % 2 else ["ordered", "normal"]
                for client in order:
                    label = f"{drive}-{case}-{trial}-{client}"
                    folder = parent / label
                    folder.mkdir()
                    destination, data = folder / "files", folder / "catalog"
                    destination.mkdir(); data.mkdir()
                    output, state = destination / "sample.bin", data / "state.json"
                    assert len(str(output)) < 260
                    utils.save(state, {"Schema": 1, "Settings": {
                        "Connections": connections, "Parallel": 1, "DownloadFolder": str(destination),
                        "CategoryFolders": False, "SuppressProgressDialog": True,
                        "SuppressCompletionDialog": True, "Sound": False, "ShowTipsOnStartup": False,
                        "ClipboardMonitor": False, "PrefetchFileInfo": False, "StopOnWebPage": False,
                        "DuplicatePolicy": "Numbered", "CloseToTray": False, "UseHttp3": False,
                    }, "Downloads": [], "Queues": []})
                    origin = Origin(Options(size=size, rate=8_000_000, latency_ms=20,
                                            no_range=connections == 1,
                                            accept_ranges="none" if connections == 1 else None,
                                            seed=20261113))
                    process = None
                    try:
                        executable = executables[client]
                        assert utils.sha(executable) == identities[client]
                        command = [str(executable), "--instance-tag", "ordered-compare-" + uuid.uuid4().hex[:12],
                                   "--data-dir", str(data), "/d", origin.url, "/p", str(destination),
                                   "/f", output.name, "/n", "/q"]
                        clock, started = utils.anchor(), time.perf_counter()
                        process = subprocess.Popen(command, cwd=executable.parent)
                        utils.save(args.out / "current-private.json", {"label": label, "pid": process.pid,
                                                                   "folder": str(folder), "command": command})
                        first = completed = None
                        while time.perf_counter() - started < 120:
                            if first is None and output.exists() and output.stat().st_size == size and utils.available(output):
                                first = time.perf_counter()
                            exited = utils.exit_ticks(int(process._handle))
                            if exited is not None:
                                completed = clock[0] + (exited - clock[1]) / 1e7
                                break
                            time.sleep(0.002)
                        if completed is None:
                            raise TimeoutError("Inspect recorded live process before retry; no process was terminated")
                        process.wait(timeout=3)
                        assert process.returncode == 0
                        after = utils.anchor()
                        drift = (after[0] - clock[0]) - (after[1] - clock[1]) / 1e7
                        assert abs(drift) < 0.005
                        snapshot = origin.snapshot()
                        last = origin.epoch + max(request["last_body_sent"] for connection in snapshot["detail"]
                                                  for request in connection["requests"]
                                                  if request.get("last_body_sent") is not None)
                        assert completed >= last and output.stat().st_size == size and utils.available(output)
                        durable, operations = utils.durable_output(output)
                        digest = expected_sha256(size, 20261113).upper()
                        assert utils.sha(output) == digest
                        job = utils.load(state)["Downloads"][0]
                        assert job["Status"] == "Complete" and job["Sha256"].upper() == digest
                        assert job["Received"] == size and job["FileName"] == output.name
                        ordered = job.get("OrderedBodyWrites", {})
                        if client == "ordered" and connections > 1:
                            assert ordered.get("Used") and ordered["Complete"]
                            assert ordered["AcceptedBytes"] == ordered["WrittenBytes"] == size
                        else:
                            assert not ordered.get("Used", False)
                        assert not list((data / "publications").glob("*.json"))
                        assert all(utils.sha(Path(path)) == digest for path, digest in protected.items())
                        result = {"drive": drive, "case": case, "trial": trial, "client": client,
                                  "bytes": size, "configuredConnections": connections,
                                  "processExitTailSeconds": completed - last,
                                  "processExitWallSeconds": completed - started,
                                  "durableTailSeconds": durable - last, "durableWallSeconds": durable - started,
                                  "firstAvailabilityTailSeconds": first - last if first else None,
                                  "postExitToDurableSeconds": durable - completed, "postExitOperations": operations,
                                  "clockMappingErrorBoundSeconds": clock[2] / 2, "clockDriftSeconds": drift,
                                  "requests": snapshot["requests"], "originBytes": snapshot["bytes_sent"],
                                  "sha256": digest, "hashMatches": True, "normalProcessExit": True,
                                  "sourceExecutableSha256": identities[client], "orderedBodyWrites": ordered,
                                  "finishPhases": job.get("FinishPhases"),
                                  "publicationPhases": job.get("PublicationPhases"),
                                  "prefinishWriteback": job.get("PrefinishWriteback"),
                                  "protectedFilesUnchanged": True, "instrumented": False}
                        utils.save(args.out / (label + ".json"), {"result": result, "origin": snapshot, "job": job})
                        # Only this exact, verified output is removed. Preserve
                        # metadata and any unexpected file for separate review.
                        assert output.resolve().parent == destination.resolve() and output.stat().st_nlink == 1
                        for path in (output, *output.parents):
                            assert not path.lstat().st_file_attributes & 0x400
                            if path == parent: break
                        output.unlink()
                        result["payloadRemoved"] = True
                        rows.append(result)
                        utils.save(args.out / "results-private.json", {"results": rows})
                        print(json.dumps({"label": label, "exitTailMs": round((completed - last) * 1000, 3),
                                          "durableTailMs": round((durable - last) * 1000, 3),
                                          "wallMs": round((completed - started) * 1000, 3), "payloadRemoved": True}), flush=True)
                    except BaseException as error:
                        utils.save(args.out / "failure-private.json", {"label": label, "error": str(error),
                            "pid": process.pid if process else None, "exitCode": process.poll() if process else None,
                            "origin": origin.snapshot(), "outputExists": output.exists()})
                        raise
                    finally:
                        origin.close()
    summary = []
    for drive in work:
        for case, _, _ in cases:
            selected = {client: [row for row in rows if row["drive"] == drive and row["case"] == case
                                 and row["client"] == client] for client in executables}
            medians = {client: {metric: statistics.median(row[metric] for row in group) * 1000
                               for metric in ("processExitTailSeconds", "durableTailSeconds",
                                              "processExitWallSeconds", "durableWallSeconds")}
                       for client, group in selected.items()}
            summary.append({"drive": drive, "case": case, "medianMilliseconds": medians,
                            "orderedNotSlowerExitMedian": medians["ordered"]["processExitTailSeconds"] <= medians["normal"]["processExitTailSeconds"],
                            "orderedNotSlowerDurableMedian": medians["ordered"]["durableTailSeconds"] <= medians["normal"]["durableTailSeconds"]})
    assert len(rows) == args.trials * len(work) * len(cases) * len(executables)
    utils.save(args.out / "terminal-private.json", {"passed": True, "transfers": len(rows), "summary": summary,
        "allPayloadsRemoved": all(row["payloadRemoved"] for row in rows), "pairedIdmComparison": False,
        "installedAppChanged": False, "orderedWritesDefaultEnabled": False,
        "scope": "Normal versus gated ordered UDM builds; actual exit includes application threads, not unrelated system services."})
    print(json.dumps({"transfers": len(rows), "summary": summary}), flush=True)


if __name__ == "__main__":
    main()
