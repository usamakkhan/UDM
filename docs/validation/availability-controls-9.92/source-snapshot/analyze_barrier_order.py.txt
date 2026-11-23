"""Read-only paired analysis of the private same-binary publication control.

This checks diagnostic coverage and retains individual losses. It cannot admit
a production change or establish physical-device/power-loss durability.
"""
import argparse
import json
import statistics
from pathlib import Path

METRICS = ('firstAvailabilityTailSeconds', 'processExitWallSeconds',
           'processExitTailSeconds', 'durableWallSeconds', 'durableTailSeconds')


def analyze(data):
    trials = {}
    for row in data['results']:
        assert row['case'] == 'single-4m' and row['bytes'] == 4 * 1024 ** 2
        assert row['hashMatches'] and row['trackedProcesses'] >= 1
        trial = trials.setdefault(row['trial'], {})
        assert row['client'] in ('udm', 'baseline', 'idm') and row['client'] not in trial
        assert all(isinstance(row[k], (int, float)) and row[k] >= 0 for k in METRICS)
        trial[row['client']] = row
    assert trials, 'No paired control records'
    rows = []
    for number, trial in sorted(trials.items()):
        assert set(trial) == {'udm', 'baseline', 'idm'}, 'Incomplete paired trial'
        serial, overlap = trial['udm'], trial['baseline']
        assert serial['sourceExecutableSha256'] == overlap['sourceExecutableSha256'], 'Not the same binary'
        assert serial['launcherExitCode'] == overlap['launcherExitCode'] == 0
        assert serial['publicationPhases']['ControlBarrierOrder'] == 'serial'
        assert overlap['publicationPhases']['ControlBarrierOrder'] == 'overlap'
        assert serial['publicationPhases']['ControlBodyJoinBeforeIntentSeconds'] >= 0, 'Serial body join was not exercised'
        losses = [key for key in METRICS if serial[key] > overlap[key]]
        rows.append(dict(trial=number, serialLosses=losses,
                         serialMinusOverlapMs={key: (serial[key] - overlap[key]) * 1000 for key in METRICS},
                         serial={key: serial[key] for key in METRICS},
                         overlap={key: overlap[key] for key in METRICS},
                         idm={key: trial['idm'][key] for key in METRICS}))
    return dict(coverageValidated=True, trials=rows,
                trialsWithSerialLoss=sum(bool(row['serialLosses']) for row in rows),
                serialLossCounts={key: sum(key in row['serialLosses'] for row in rows) for key in METRICS},
                medianSerialMinusOverlapMs={key: statistics.median(row['serialMinusOverlapMs'][key] for row in rows) for key in METRICS},
                rejectedForObservedRegressions=any(row['serialLosses'] for row in rows),
                productionChangeAdmitted=False,
                scope='Private same-binary C: 4 MiB barrier-order diagnostic; normal-product repetition and recovery qualification remain separate gates.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = analyze(json.loads(args.input.read_text(encoding='utf-8-sig')))
    args.output.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'trials'}))
