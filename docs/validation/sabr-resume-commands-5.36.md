# Desktop recovery commands for held media

Resume in the main download list now opens the same media recovery dialog as the progress window. A mixed selection resumes ordinary downloads first and handles held media separately. Cancel stops further recovery dialogs in that command and preserves the held jobs and their checkpoints.

The native presentation fixture uses the real main list, progress window and media recovery dialog. It selects an ordinary loopback download and two held media jobs. It observes the generated recovery page without opening a personal browser, checks the saved playback position and cancels the dialog. It then activates the actual Refresh button in the progress window and checks the same recovery behavior.

All 150 native presentation checks passed. The ten new checks covered the mixed selection, one recovery dialog at a time, the saved playback position, the progress-window button and preservation of both held downloads after cancellation. Both staged 0.69.0 browser bundles passed 19 capture and 31 audio checks each. All five native executables, 83 recorded inputs and 95 staged browser files matched their hashes. [Build receipt](sabr-resume-commands-5.36-build.json)

The installed executables reported UDM 4.74 and IDM 6.43.10.2 during this pass. The recovery commands were exercised in the isolated 5.36 component. A paired installed comparison and cold exact-size timing remain open.

## Cold 360p handoff on the staged 5.35 build

An isolated Edge profile selected 360p on the public reference video without the additional playback warmup. The browser prepared the choice and received the native handoff response in 1,342 ms. The fixture bypassed File Info for this run, so the measurement excludes dialog paint and exact-size timing. UDM retained 2,051 bytes before the origin required browser attestation. The job remained on hold and published no file. The temporary host registration was removed. This verifies public 360p selection and admission and leaves its full transfer open.
