'use strict';
// Diagnostic observation only. Never replaces a function, argument or result.
// Records only paths containing the caller's unique owned-workload token.
const records = [], files = new Map(), mappings = new Map(), listeners = [];
let armed = false, dropped = 0, sequence = 0;
const limit = 12000;
const nativeResolvedSamples = [];
const nativeMetrics = { calls: 0, pathsResolved: 0, pathsMatched: 0, pathFailures: 0, nonDiskSkipped: 0, fileTypeFailures: 0 };
const kernel = Process.getModuleByName('kernel32.dll');
const counter = new NativeFunction(kernel.getExportByName('QueryPerformanceCounter'), 'bool', ['pointer'], { scheduling: 'exclusive' });
const frequency = new NativeFunction(kernel.getExportByName('QueryPerformanceFrequency'), 'bool', ['pointer']);
const clock = Memory.alloc(8);
if (!frequency(clock)) throw new Error('QueryPerformanceFrequency failed');
const hz = clock.readS64().toNumber();
function ticks() { if (!counter(clock)) throw new Error('QueryPerformanceCounter failed'); return clock.readS64().toNumber(); }
function emit(record) { if (records.length < limit) records.push(record); else ++dropped; }
function text(p, wide) { try { return (wide ? p.readUtf16String() : p.readAnsiString()).slice(0, 32760); } catch (_) { return ''; } }
function valid(h) { return !h.isNull() && !h.equals(ptr(-1)); }
function install(name, callbacks) {
  const address = kernel.getExportByName(name);
  listeners.push(Interceptor.attach(address, callbacks));
}
function snapshot() { return { frequency: hz, now: ticks(), dropped, nativeMetrics, nativeResolvedSamples, openFiles: files.size, openMappings: mappings.size, records }; }
rpc.exports = {
  identity() { return { pid: Process.id, image: Process.mainModule.path, arch: Process.arch, frequency: hz }; },
  start(token) {
    if (armed || !/^[a-z0-9]{24,64}$/.test(token)) throw new Error('A fresh unique workload token is required');
    armed = true;
    const matches = path => {
      const lower = path.toLowerCase();
      if (lower.includes(token)) return true;
      // Observed IDM layout truncates the owned filename to twenty hex digits.
      // Require its known cache root, exact nonce prefix, numbered directory,
      // and exact candidate body/log leaf; never match a generic cache path.
      const parts = lower.split('\\'), parent = parts[parts.length - 2], leaf = parts[parts.length - 1];
      const prefix = token.slice(0, 20);
      return lower.includes('\\idm\\dwnldata\\') &&
        new RegExp('^' + prefix + '_[0-9]+$').test(parent || '') &&
        (leaf === prefix + '.bin' || leaf === parent + '.log');
    };
    for (const wide of [false, true]) {
      const name = 'CreateFile' + (wide ? 'W' : 'A');
      install(name, {
        onEnter(args) {
          const path = text(args[0], wide);
          if (!matches(path)) return;
          this.entry = { api: name, id: ++sequence, begin: ticks(), tid: this.threadId,
            path, access: args[1].toUInt32(), share: args[2].toUInt32(),
            disposition: args[4].toUInt32(), flags: args[5].toUInt32() };
        },
        onLeave(result) {
          if (!this.entry) return;
          const entry = this.entry; entry.end = ticks(); entry.handle = result.toString();
          entry.ok = valid(result); entry.error = entry.ok ? 0 : this.lastError;
          if (entry.ok) files.set(entry.handle, entry.path);
          emit(entry);
        }
      });
      install('CreateFileMapping' + (wide ? 'W' : 'A'), {
        onEnter(args) {
          const path = files.get(args[0].toString()); if (!path) return;
          this.entry = { api: 'CreateFileMapping' + (wide ? 'W' : 'A'), id: ++sequence,
            begin: ticks(), tid: this.threadId, path, handle: args[0].toString(), protection: args[2].toUInt32() };
        },
        onLeave(result) {
          if (!this.entry) return;
          const entry = this.entry; entry.end = ticks(); entry.ok = !result.isNull();
          entry.mapping = result.toString(); if (entry.ok) mappings.set(entry.mapping, entry.path);
          emit(entry);
        }
      });
    }
    // Do not hook ReadFile: even an unrelated, untracked pipe read would retain
    // an onLeave callback until it returns and can prevent prompt script unload.
    for (const name of ['WriteFile', 'FlushFileBuffers']) install(name, {
      onEnter(args) {
        const path = files.get(args[0].toString()); if (!path) return;
        this.entry = { api: name, id: ++sequence, begin: ticks(), tid: this.threadId,
          path, handle: args[0].toString() };
        if (name !== 'FlushFileBuffers') {
          this.entry.requested = args[2].toUInt32(); this.entry.overlapped = !args[4].isNull();
          this.count = args[3];
        }
      },
      onLeave(result) {
        if (!this.entry) return;
        const entry = this.entry; entry.end = ticks(); entry.ok = result.toInt32() !== 0;
        entry.error = entry.ok ? 0 : this.lastError;
        if (entry.ok && this.count && !this.count.isNull()) {
          try { entry.completed = this.count.readU32(); } catch (_) { entry.countUnreadable = true; }
        }
        emit(entry);
      }
    });
    install('MapViewOfFile', {
      onEnter(args) {
        const path = mappings.get(args[0].toString()); if (!path) return;
        this.entry = { api: 'MapViewOfFile', id: ++sequence, begin: ticks(), tid: this.threadId,
          path, mapping: args[0].toString(), access: args[1].toUInt32() };
      },
      onLeave(result) {
        if (!this.entry) return;
        this.entry.end = ticks(); this.entry.ok = !result.isNull(); emit(this.entry);
      }
    });
    install('CloseHandle', {
      onEnter(args) {
        const handle = args[0].toString(), path = files.get(handle) || mappings.get(handle); if (!path) return;
        this.entry = { api: 'CloseHandle', id: ++sequence, begin: ticks(), tid: this.threadId, path, handle };
      },
      onLeave(result) {
        if (!this.entry) return;
        const entry = this.entry; entry.end = ticks(); entry.ok = result.toInt32() !== 0;
        if (entry.ok) { files.delete(entry.handle); mappings.delete(entry.handle); } emit(entry);
      }
    });

    // Resolve the live handle instead of assuming a Win32 open was observed.
    // Kernel copy helpers may issue native writes without calling WriteFile.
    const ntdll = Process.getModuleByName('ntdll.dll');
    const finalPath = new NativeFunction(kernel.getExportByName('GetFinalPathNameByHandleW'), 'uint32', ['pointer', 'pointer', 'uint32', 'uint32']);
    const fileType = new NativeFunction(kernel.getExportByName('GetFileType'), 'uint32', ['pointer']);
    function ownedPath(handle) {
      // Pipes, character devices and unknown handles are outside file attribution.
      // Do not issue pathname queries on unrelated IPC handles.
      const type = fileType(handle);
      if (type !== 1) {
        if (type === 0) ++nativeMetrics.fileTypeFailures;
        else ++nativeMetrics.nonDiskSkipped;
        return '';
      }
      // NativeFunction may release the JS lock; never share this output buffer.
      const pathBuffer = Memory.alloc(65536);
      const count = finalPath(handle, pathBuffer, 32768, 0);
      if (count === 0 || count >= 32768) { ++nativeMetrics.pathFailures; return ''; }
      ++nativeMetrics.pathsResolved;
      const path = pathBuffer.readUtf16String(count);
      if (!matches(path)) return '';
      ++nativeMetrics.pathsMatched;
      if (nativeResolvedSamples.length < 96 && !nativeResolvedSamples.includes(path)) nativeResolvedSamples.push(path);
      return path;
    }
    for (const name of ['NtWriteFile', 'NtFlushBuffersFile', 'NtFlushBuffersFileEx']) {
      const address = ntdll.findExportByName(name); if (!address) continue;
      listeners.push(Interceptor.attach(address, {
        onEnter(args) {
          ++nativeMetrics.calls;
          const error = this.lastError;
          let path = ''; try { path = ownedPath(args[0]); } finally { this.lastError = error; }
          if (!path) return;
          this.entry = { api: name, id: ++sequence, begin: ticks(), tid: this.threadId, path, handle: args[0].toString() };
          if (name === 'NtWriteFile') {
            this.entry.requested = args[6].toUInt32(); this.statusBlock = args[4];
            this.entry.bodyCandidate = path.toLowerCase().endsWith('.bin');
            if (!args[7].isNull()) try { this.entry.byteOffset = args[7].readS64().toString(); } catch (_) { this.entry.offsetUnreadable = true; }
          } else if (name === 'NtFlushBuffersFileEx') this.entry.flags = args[1].toUInt32();
        },
        onLeave(result) {
          if (!this.entry) return;
          const entry = this.entry; entry.end = ticks(); entry.status = result.toUInt32();
          entry.pending = entry.status === 0x103; entry.ok = result.toInt32() >= 0 && !entry.pending;
          // STATUS_PENDING is submission, never proof of completed bytes.
          if (entry.ok && this.statusBlock && !this.statusBlock.isNull()) {
            try { entry.completed = this.statusBlock.add(Process.pointerSize).readPointer().toUInt32(); }
            catch (_) { entry.countUnreadable = true; }
          }
          emit(entry);
        }
      }));
    }
    Interceptor.flush();
    return { armed: true, hooks: listeners.length, begin: ticks() };
  },
  snapshot,
  stop() { for (const listener of listeners) listener.detach(); armed = false; return snapshot(); }
};
