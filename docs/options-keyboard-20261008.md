# Keyboard navigation between settings pages — 8 October 2026

The original Options suite passed 107 checks for its nine pages and browser admission policy, but only selected tabs programmatically. A new keyboard-routing check reproduced a gap: Ctrl+Tab was consumed without changing the settings page when invoked from an edit field.

Form now handles Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+PageUp and Ctrl+PageDown for its visible, enabled tab control. It sends the existing selection-changing and selection-changed notifications, honors a canceled change, wraps at either end, and moves focus to the tab control if the previous field becomes hidden or disabled. Other keyboard handling still goes through the normal dialog implementation. These shortcuts follow [Microsoft's tab accessibility guidance](https://learn.microsoft.com/en-us/windows/win32/uxguide/ctrl-tabs); this turn is not a fresh observation of IDM keyboard behavior.

The corrected Options suite passed 133 checks, including forward/reverse movement from an edit, wraparound, page visibility, visible focus, all nine pages, browser-dialog policy and unchanged catalog/settings after Cancel. Additional scheduler and shared-dialog results are recorded in the validation receipt. The fixture changes only its own thread keyboard state, restores it after each call, and routes messages through the actual dialog PreTranslateMessage method; it does not inject physical desktop input.

Raw scripts, logs and results are under `C:\Users\Abuzar\AppData\Local\Temp\udm-options-review-20261008`. Unchanged backend objects are reused, so this is not a clean full-backend build. Existing compiler warnings remain. Candidate identity and final suite totals: [validation receipt](validation/options-keyboard-20261008.json).

Not packaged, installed or published. Physical keyboard use against the personal desktop and complete IDM parity remain unverified.
