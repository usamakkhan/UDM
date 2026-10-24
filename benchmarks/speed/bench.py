"""UDM vs IDM download benchmark harness.

Drives both download managers through their command lines against the loopback
origin in ``origin.py`` and against read-only public origins, and records wall
time, throughput, connection counts and segment behaviour per case.

Example::

    python benchmarks/speed/bench.py --udm release-native/UDM.exe --work C:/bench-work \
        --out benchmarks/speed/results/2026-10-18-before --clients udm,idm --cases single-8,single-16

See README.md for the case list and what each metric means. Nothing here uses
the personal UDM catalog: UDM runs with ``--instance-tag`` and a private
``--data-dir``. IDM is the installed application; its connection-count setting
is written to the registry for the duration of a run and restored afterwards.
"""
from __future__ import annotations
import argparse, hashlib, json, os, re, shutil, subprocess, sys, time, threading, winreg
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from origin import Origin, Options, expected_sha256  # noqa: E402

MiB = 1024 * 1024
IDM_EXE = r"C:\Program Files (x86)\Internet Download Manager\IDMan.exe"
IDM_KEY = r"Software\DownloadManager"
# IDM 6.43 reads the per-download connection count from MaxConnectionsNumber; ConnectionsNumber is
# the older name and is written too so both agree.
IDM_CONNECTIONS_VALUES = ("MaxConnectionsNumber", "ConnectionsNumber")

# ---------------------------------------------------------------------------
# Cases. ``server`` holds Origin options; ``connections`` is the client setting.
# ``kill_at`` kills the client once that fraction of the file was served and
# relaunches it (resume behaviour). ``public`` names a public URL instead.
# ---------------------------------------------------------------------------
CASES = {
    "single-8": dict(server=dict(size=384 * MiB, rate=8_000_000, latency_ms=20), connections=8),
    "single-16": dict(server=dict(size=384 * MiB, rate=8_000_000, latency_ms=20), connections=16),
    "uncapped-8": dict(server=dict(size=384 * MiB, rate=0, latency_ms=0), connections=8),
    "resume-8": dict(server=dict(size=384 * MiB, rate=8_000_000, latency_ms=20), connections=8, kill_at=0.45),
    "outage-8": dict(server=dict(size=384 * MiB, rate=8_000_000, latency_ms=20), connections=8, outage_at=0.45, outage_seconds=5),
    "drop-8": dict(server=dict(size=384 * MiB, rate=8_000_000, latency_ms=20, drop_every=8 * MiB), connections=8),
    "no-range-1": dict(server=dict(size=256 * MiB, rate=0, latency_ms=20, no_range=True), connections=8),
    "slow-start-2": dict(server=dict(size=256 * MiB, rate=8_000_000, latency_ms=20, max_conns=2), connections=8),
    "public-alpine-8": dict(public="https://dl-cdn.alpinelinux.org/alpine/v3.21/releases/x86_64/alpine-standard-3.21.3-x86_64.iso", connections=8),
    "public-kernel-8": dict(public="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.tar.xz", connections=8),
}


def log(*parts):
    print(time.strftime("%H:%M:%S"), *parts, flush=True)


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(4 * MiB), b""):
            h.update(block)
    return h.hexdigest()


def kill_tree(pid: int):
    subprocess.run(["taskkill", "/PID", str(pid), "/T", "/F"], capture_output=True)


def processes(name: str) -> list[int]:
    out = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {name}", "/FO", "CSV", "/NH"], capture_output=True, text=True).stdout
    return [int(row.split('","')[1]) for row in out.splitlines() if row.startswith('"')]


def rmtree(path: Path):
    for _ in range(20):
        try:
            if path.exists():
                shutil.rmtree(path)
            return
        except PermissionError:
            time.sleep(0.5)


class ConnectionSampler:
    """Samples established TCP connections of a PID tree with netstat (public origins)."""

    def __init__(self, pids: set[int]):
        self.pids, self.peak, self.samples, self.stop = pids, 0, [], False
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.thread.start()

    def run(self):
        while not self.stop:
            out = subprocess.run(["netstat", "-ano", "-p", "tcp"], capture_output=True, text=True).stdout
            count = 0
            for line in out.splitlines():
                cols = line.split()
                if len(cols) == 5 and cols[3] == "ESTABLISHED" and int(cols[4]) in self.pids and not cols[2].startswith("127."):
                    count += 1
            self.peak = max(self.peak, count)
            self.samples.append(count)
            time.sleep(1)

    def finish(self):
        self.stop = True
        self.thread.join(timeout=5)
        return {"peak_concurrent": self.peak, "samples": self.samples}


# ---------------------------------------------------------------------------
class UdmClient:
    name = "udm"

    def __init__(self, exe: Path, work: Path, settings: dict | None = None):
        self.exe, self.work = exe, work
        self.data = work / "udm-data"
        self.tag = "speed-bench"
        self.settings = settings or {}  # extra Settings merged into the private catalog (--udm-settings)

    def prepare(self, connections: int, folder: Path):
        self.kill()  # a stale benchmark instance would hold the data dir; the personal app has another tag
        rmtree(self.data)
        self.data.mkdir(parents=True)
        state = {"Schema": 1, "Settings": {
            "Connections": connections, "Parallel": 3, "DownloadFolder": str(folder), "CategoryFolders": False,
            "SuppressProgressDialog": True, "SuppressCompletionDialog": True, "Sound": False, "ShowTipsOnStartup": False,
            "ClipboardMonitor": False, "PrefetchFileInfo": False, "StopOnWebPage": False, "DuplicatePolicy": "Add",
            **self.settings,
        }, "Downloads": [], "Queues": []}
        (self.data / "state.json").write_text(json.dumps(state), encoding="utf-8")

    def start(self, url: str, folder: Path, name: str, resume: bool = False) -> subprocess.Popen:
        args = [str(self.exe), "--instance-tag", self.tag, "--data-dir", str(self.data)]
        args += ["/s"] if resume else ["/d", url, "/p", str(folder), "/f", name, "/n", "/q"]
        return subprocess.Popen(args, cwd=str(self.exe.parent))

    def state(self) -> dict | None:
        for _ in range(10):
            try:
                return json.loads((self.data / "state.json").read_text(encoding="utf-8"))
            except (OSError, ValueError):
                time.sleep(0.05)
        return None

    def job(self) -> dict | None:
        s = self.state()
        if s and s.get("Downloads"):
            return s["Downloads"][0]
        return None

    def done(self, output: Path, size: int) -> bool:
        j = self.job()
        return bool(j) and j.get("Status") == "Complete" and output.exists() and output.stat().st_size == size

    def failed(self) -> str | None:
        j = self.job()
        if j and j.get("Status") in ("Failed", "Error") or (j and j.get("Error")):
            return str(j.get("Error") or j.get("Status"))
        return None

    def details(self) -> dict:
        j = self.job() or {}
        segs = j.get("Segments") or []
        return {"status": j.get("Status"), "segments": len(segs), "dynamic_splits": j.get("DynamicSplits"),
                "transfer_seconds": j.get("TransferSeconds"), "received": j.get("Received"), "transferred": j.get("TransferredBytes"),
                "range_supported": j.get("RangeSupported"), "error": j.get("Error"), "http2_fallback": j.get("Http2FallbackError"),
                "finish_phases": j.get("FinishPhases"), "network_tail_seconds": j.get("NetworkTailSeconds"), "network_ticks": j.get("NetworkTicks"), "elapsed_seconds": j.get("ElapsedSeconds")}

    def kill(self, proc=None):
        if proc is not None:
            kill_tree(proc.pid)
        # only our tagged instance holds our data dir; the personal app has another tag
        script = ("Get-CimInstance Win32_Process -Filter \"Name='UDM.exe'\" | Where-Object { $_.CommandLine -like '*" + self.tag + "*' } | "
                  "ForEach-Object { Stop-Process -Id $_.ProcessId -Force }")
        subprocess.run(["powershell", "-NoProfile", "-Command", script], capture_output=True)


class IdmClient:
    name = "idm"

    def __init__(self, exe: Path, work: Path, set_connections: bool = True):
        self.exe, self.work, self.set_connections = exe, work, set_connections
        self.original = None

    def _read(self):
        saved = {}
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, IDM_KEY) as k:
            for name in IDM_CONNECTIONS_VALUES:
                try:
                    saved[name] = winreg.QueryValueEx(k, name)[0]
                except FileNotFoundError:
                    saved[name] = None
        return saved

    def _write(self, values):
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, IDM_KEY, 0, winreg.KEY_SET_VALUE) as k:
            for name in IDM_CONNECTIONS_VALUES:
                value = values.get(name) if isinstance(values, dict) else values
                if value is None:
                    try:
                        winreg.DeleteValue(k, name)
                    except FileNotFoundError:
                        pass
                else:
                    winreg.SetValueEx(k, name, 0, winreg.REG_DWORD, int(value))

    def prepare(self, connections: int, folder: Path):
        for pid in processes("IDMan.exe"):
            kill_tree(pid)
        time.sleep(0.5)
        if self.set_connections:
            if self.original is None:
                self.original = ("saved", self._read())
            self._write(connections)

    def restore(self):
        if self.original:
            self._write(self.original[1])
            self.original = None

    def start(self, url: str, folder: Path, name: str, resume: bool = False) -> subprocess.Popen:
        return subprocess.Popen([str(self.exe), "/d", url, "/p", str(folder), "/f", name, "/n", "/q"])

    def done(self, output: Path, size: int) -> bool:
        if not (output.exists() and output.stat().st_size == size):
            return False
        try:
            # IDM assembles into the destination; exclusive open succeeds once it released the file
            with open(output, "r+b"):
                return True
        except PermissionError:
            return False

    def failed(self):
        return None

    def details(self):
        return {}

    def kill(self, proc):
        for pid in processes("IDMan.exe"):
            kill_tree(pid)


# ---------------------------------------------------------------------------
def wait_until(predicate, timeout: float, poll: float = 0.2) -> bool:
    deadline = time.perf_counter() + timeout
    while time.perf_counter() < deadline:
        if predicate():
            return True
        time.sleep(poll)
    return False


def run_case(name: str, case: dict, client, work: Path, out_dir: Path, timeout: float) -> dict:
    folder = work / "downloads"
    rmtree(folder)
    folder.mkdir(parents=True)
    result = {"case": name, "client": client.name, "connections_setting": case["connections"]}
    origin = None
    if "public" in case:
        url = case["public"]
        size = None
        file_name = url.rsplit("/", 1)[1]
    else:
        opts = Options(**case["server"])
        origin = Origin(opts)
        url = origin.url + f"?run={name}-{client.name}-{int(time.time())}"
        size = opts.size
        file_name = f"{name}-{client.name}.bin"
        result["server"] = case["server"]
        result["expected_sha256"] = expected_sha256(opts.size, opts.seed)
    output = folder / file_name
    result["url"] = url
    client.prepare(case["connections"], folder)
    log(f"[{name}/{client.name}] start {url}")
    started = time.perf_counter()
    if origin is not None:
        result["launch_at"] = round(origin.now(), 3)  # origin-relative clock used by request_log/connection_log
    proc = client.start(url, folder, file_name)
    sampler = ConnectionSampler({proc.pid}) if origin is None else None
    phases = []
    try:
        if origin is not None and case.get("kill_at"):
            target = int(case["kill_at"] * size)
            reached = wait_until(lambda: origin.bytes_sent() >= target or client.failed(), timeout)
            kill_time = time.perf_counter()
            served_before_kill = origin.bytes_sent()
            client.kill(proc)
            time.sleep(1.0)
            served_at_kill = origin.bytes_sent()
            before = origin.snapshot()
            phases.append({"phase": "first-run", "reached_kill_point": reached, "seconds": kill_time - started,
                           "bytes_served": served_at_kill, "connections": before["connections"], "requests": before["requests"]})
            log(f"[{name}/{client.name}] killed after {served_before_kill / MiB:.0f} MiB served; relaunching")
            # UDM's part files and IDM's temp data stay in place: relaunch and measure the resume
            resume_started = time.perf_counter()
            if origin is not None:
                result["relaunch_at"] = round(origin.now(), 3)
            proc = client.start(url, folder, file_name, resume=True)
            started_resume = resume_started
            result["resume"] = True
        elif origin is not None and case.get("outage_at"):
            started_resume = None
            target = int(case["outage_at"] * size)
            wait_until(lambda: origin.bytes_sent() >= target or client.failed(), timeout)
            result["outage_at_seconds"] = round(time.perf_counter() - started, 3)
            result["outage_origin_at"] = round(origin.now(), 3)
            origin.outage(case["outage_seconds"])
            log(f"[{name}/{client.name}] outage of {case['outage_seconds']} s after {origin.bytes_sent() / MiB:.0f} MiB served")
        else:
            started_resume = None
        stable = {"size": -1, "since": 0.0}
        def finished():
            if size is not None:
                return client.done(output, size)
            # unknown size (public origin): UDM reports completion in its catalog; IDM creates the
            # destination file when it merges, so wait until it is closed and its size stopped changing
            if not output.exists():
                return False
            if client.name == "udm":
                return (client.job() or {}).get("Status") == "Complete"
            current = output.stat().st_size
            if current != stable["size"]:
                stable["size"], stable["since"] = current, time.perf_counter()
                return False
            if time.perf_counter() - stable["since"] < 2:
                return False
            try:
                with open(output, "r+b"):
                    return True
            except PermissionError:
                return False
        ok = wait_until(lambda: finished() or client.failed(), timeout)
        finished_at = time.perf_counter()
        result["completed"] = bool(ok) and not client.failed()
        result["client_error"] = client.failed()
        result["wall_seconds"] = round(finished_at - started, 3)
        if started_resume:
            result["resume_wall_seconds"] = round(finished_at - started_resume, 3)
        if output.exists():
            result["output_bytes"] = output.stat().st_size
            if result["completed"]:
                digest = sha256_file(output)
                result["sha256"] = digest
                if size is not None:
                    result["sha256_ok"] = digest == result["expected_sha256"]
                else:
                    size = output.stat().st_size
        if size and result.get("wall_seconds"):
            effective = result.get("resume_wall_seconds") or result["wall_seconds"]
            result["throughput_MBps"] = round(size / effective / 1e6, 2)
            result["size_bytes"] = size
        result["client"] = client.name
        result["client_details"] = client.details()
        if origin is not None:
            snap = origin.snapshot()
            detail = snap.pop("detail")
            # per-request summary (status, range start, bytes, outcome) without bulky per-byte data
            snap["request_log"] = [
                {"conn": c["id"], "at": round(r["at"], 3), "status": r["status"], "start": r["start"], "end": r["end"],
                 "sent": r["sent"], "outcome": r["outcome"], "finished": r["finished"] and round(r["finished"], 3)}
                for c in detail for r in c["requests"]]
            snap["connection_log"] = [{"id": c["id"], "opened": round(c["opened"], 3), "closed": c["closed"] and round(c["closed"], 3),
                                       "bytes": c["bytes"], "rejected": c["rejected"], "drops": c["drops"], "requests": len(c["requests"])} for c in detail]
            snap["wasted_bytes"] = snap["bytes_sent"] - (size or 0) if result.get("completed") else None
            result["origin"] = snap
            result["phases"] = phases
        if sampler:
            result["tcp"] = sampler.finish()
        # make sure the client process is gone (UDM /q exits itself; /s resume and IDM may linger)
        if client.name == "udm":
            if not wait_until(lambda: proc.poll() is not None, 15):
                client.kill(proc)
        else:
            wait_until(lambda: not processes("IDMan.exe"), 10)
            client.kill(proc)
    finally:
        if origin:
            origin.close()
    log(f"[{name}/{client.name}] completed={result.get('completed')} wall={result.get('wall_seconds')}s "
        f"thr={result.get('throughput_MBps')} MB/s conns={(result.get('origin') or result.get('tcp') or {}).get('connections', (result.get('tcp') or {}).get('peak_concurrent'))}")
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / f"{name}-{client.name}.json").write_text(json.dumps(result, indent=1), encoding="utf-8")
    rmtree(folder)
    return result


def summarize(results: list[dict]) -> str:
    rows = ["| case | client | done | wall s | MB/s | conns | requests | rejected | drops | wasted MiB | sha ok | note |",
            "|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---|"]
    for r in results:
        o = r.get("origin") or {}
        tcp = r.get("tcp") or {}
        conns = o.get("connections", tcp.get("peak_concurrent", ""))
        wasted = o.get("wasted_bytes")
        d = r.get("client_details") or {}
        note = r.get("client_error") or ""
        if r["client"] == "udm" and d.get("segments") is not None:
            note += f" segments={d.get('segments')} splits={d.get('dynamic_splits')}"
        rows.append(f"| {r['case']} | {r['client']} | {r.get('completed')} | {r.get('resume_wall_seconds') or r.get('wall_seconds')} | {r.get('throughput_MBps', '')} | {conns} | "
                    f"{o.get('requests', '')} | {o.get('rejected_connections', '')} | {o.get('drops', '')} | {'' if wasted is None else round(wasted / MiB, 1)} | {r.get('sha256_ok', '')} | {note.strip()} |")
    return "\n".join(rows)


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--udm", type=Path, required=True, help="UDM.exe to benchmark")
    p.add_argument("--idm", type=Path, default=Path(IDM_EXE))
    p.add_argument("--udm-settings", default="{}", help="JSON object merged into UDM's private Settings, e.g. {\"UseHttp3\": false}")
    p.add_argument("--work", type=Path, required=True, help="scratch folder for data dirs and downloads")
    p.add_argument("--out", type=Path, required=True, help="result folder (one JSON per case and client)")
    p.add_argument("--cases", default=",".join(CASES), help="comma-separated case names")
    p.add_argument("--clients", default="udm,idm")
    p.add_argument("--repeat", type=int, default=1)
    p.add_argument("--timeout", type=float, default=600)
    p.add_argument("--no-idm-registry", action="store_true", help="do not write IDM's connection count to the registry")
    a = p.parse_args(argv)
    a.work.mkdir(parents=True, exist_ok=True)
    clients = []
    for c in a.clients.split(","):
        if c == "udm":
            clients.append(UdmClient(a.udm.resolve(), a.work, json.loads(a.udm_settings)))
        elif c == "idm":
            clients.append(IdmClient(a.idm, a.work, set_connections=not a.no_idm_registry))
    results = []
    try:
        for name in a.cases.split(","):
            case = CASES[name]
            for i in range(a.repeat):
                for client in clients:
                    results.append(run_case(name, case, client, a.work, a.out, a.timeout))
    finally:
        for client in clients:
            if isinstance(client, IdmClient):
                client.restore()
        if results:
            # the summary covers every result file in the output folder, so partial reruns merge
            results = [json.loads(f.read_text(encoding="utf-8")) for f in sorted(a.out.glob("*.json"))]
            results.sort(key=lambda r: (list(CASES).index(r["case"]) if r["case"] in CASES else 99, r["client"]))
            table = summarize(results)
            (a.out / "summary.md").write_text(table + "\n", encoding="utf-8")
            print(table)


if __name__ == "__main__":
    main()
