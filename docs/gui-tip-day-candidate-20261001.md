# Tip of the Day — staged candidate

Added Help > Tip of the Day at the reference menu position. The dialog uses the reference 253 by 164 dialog-unit size, Tahoma sizing through Form, and the observed positions and labels for the heading, tip text, startup checkbox, Next Tip and Close. It contains eight original UDM tips and persists the next tip index and startup preference. Close, the window close action and Escape use the same save path; a save error leaves the dialog open.

Normal foreground launches show tips unless opted out. Background, silent, incoming-address and recovery handoff launches do not. Startup defaults and rotation semantics are UDM implementation choices: resource inspection alone does not prove IDM's runtime semantics. The reference bitmap/panel artwork is not reproduced, and rendered equivalence remains unverified.

## Verification

The final rebuilt candidate passed 18 actual-app tip checks and 33 actual-app GUI/recovery regression checks. Tests cover all eight tips and wrapping, saving opt-out, foreground restart without tips, manual reopening, enabling startup tips, next-tip persistence, window-close persistence, background suppression, Help routes, menus, limiter settings and Recovery reopening. The broader 200 native checks from the prior Help acceptance predate this change and are not claimed as freshly rerun.

The first regression identified a real problem: Recovery's Open UDM action showed a tip modal over the main window. Suppressing tips for --wait-process handoffs fixed it; the final 33-check run passed. An earlier slow harness was stopped and corrected to query process ownership once per observation. These incomplete runs are not counted as passes.

Personal catalog and installed UDM/native-host/monitor hashes remain unchanged. The candidate is not installed. Full IDM parity remains unfinished, including remaining Help actions, visual equivalence, and the broader browser, engine and driver gaps in the research register.

[Acceptance](D:/UDM-Workspace/candidates/native-084-backup/control/tip-day-acceptance.json) · [Tip tests](D:/UDM-Workspace/candidates/native-084-backup/control/tip-day-app-results.json) · [GUI regression](D:/UDM-Workspace/candidates/native-084-backup/control/ui-tip-regression-final-app.json) · [Reference layout](D:/UDM-Workspace/candidates/native-084-backup/control/tip-reference-layout.json)
