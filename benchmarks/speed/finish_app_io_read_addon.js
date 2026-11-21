'use strict';
// Concatenated after finish_user_io_agent.js. Read observation is safe only
// under the validated leave-loaded-through-target-exit lifecycle: untracked
// synchronous pipe reads may still have pending Interceptor return frames.
const startWithoutReads = rpc.exports.start;
rpc.exports.start = function (token) {
  const state = startWithoutReads(token);
  install('ReadFile', {
    onEnter(args) {
      const path = files.get(args[0].toString());
      if (!path) return;
      this.entry = { api: 'ReadFile', id: ++sequence, begin: ticks(), tid: this.threadId,
        path, handle: args[0].toString(), requested: args[2].toUInt32(),
        overlapped: !args[4].isNull() };
      this.count = args[3];
    },
    onLeave(result) {
      if (!this.entry) return;
      const entry = this.entry;
      entry.end = ticks(); entry.ok = result.toInt32() !== 0;
      entry.error = entry.ok ? 0 : this.lastError;
      entry.pending = !entry.ok && entry.error === 997;
      if (entry.ok) {
        try { entry.completed = this.count.readU32(); }
        catch (_) { entry.countUnreadable = true; }
      }
      emit(entry);
    }
  });
  Interceptor.flush();
  return { ...state, hooks: listeners.length, readsObserved: true,
    lifecycle: 'snapshot without detach, leave loaded until normal target exit' };
};
rpc.exports.stop = function () {
  throw new Error('Use snapshot and normal target exit; detaching live read hooks is not qualified');
};
