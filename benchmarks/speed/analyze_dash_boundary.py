"""Validate owned DASH boundary evidence without launching tools or deleting files."""
import argparse
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import re


def file_digest(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def analyze(root, require_media=False):
    return analyze_data(json.loads((root / 'results.json').read_text(encoding='utf-8-sig')),
                        root, require_media)


def analyze_data(data, root=None, require_media=False):
    assert data['investigationCompleted'] is True
    cases = data['cases']
    assert len(cases) == 3
    assert {row['case'] for row in cases} == {'packet-aligned', 'aac-crossing', 'both-crossing'}
    expected_boundaries = {'packet-aligned': Fraction('1.28'), 'aac-crossing': Fraction('1'),
                           'both-crossing': Fraction('1.01')}
    artifacts = []

    def artifact(name, digest):
        assert re.fullmatch(r'[a-zA-Z0-9._-]+', name) and name not in ('.', '..')
        assert re.fullmatch(r'[0-9a-f]{64}', digest)
        path = root / name if root is not None else None
        present = path is not None and path.is_file()
        if require_media:
            assert present, name
        if present:
            assert file_digest(path) == digest, name
        artifacts.append(dict(name=name, retained=present, rehashed=present))

    rows = []
    for row in cases:
        assert Fraction(row['boundary']) == expected_boundaries[row['case']]
        for key in ['expectedSamples', 'decodedSamples', 'expectedVideoFrames', 'actualVideoFrames']:
            assert type(row[key]) is int and row[key] > 0
        assert type(row['sampleDelta']) is int
        assert row['sampleDelta'] == row['decodedSamples'] - row['expectedSamples']
        assert Fraction(row['boundary']) * 48000 * 3 == row['expectedSamples']
        for key in ['audioExact', 'videoHashesAndPtsExact', 'allExact']:
            assert type(row[key]) is bool
        for key in ['expectedPcmSha256', 'actualPcmSha256']:
            assert re.fullmatch(r'[0-9a-f]{64}', row[key])
        assert row['audioExact'] == (row['expectedPcmSha256'] == row['actualPcmSha256'])
        if row['audioExact']:
            assert row['sampleDelta'] == 0
        if row['videoHashesAndPtsExact']:
            assert row['expectedVideoFrames'] == row['actualVideoFrames']
        assert row['allExact'] == (row['audioExact'] and row['videoHashesAndPtsExact'])
        assert len(row['clips']) == 3 and {clip['period'] for clip in row['clips']} == {0, 1, 2}
        for clip in row['clips']:
            assert clip['expectedSamples'] * 3 == row['expectedSamples']
            artifact(f"{row['case']}-{clip['period']}.mp4", clip['sha256'])
        artifact(row['case'] + '-joined.mp4', row['joinedSha256'])
        rows.append({key: row[key] for key in ['case', 'boundary', 'expectedSamples',
                                             'decodedSamples', 'sampleDelta', 'expectedVideoFrames',
                                             'actualVideoFrames', 'audioExact',
                                             'videoHashesAndPtsExact', 'allExact']})
    assert type(data['allCasesExact']) is bool
    assert data['allCasesExact'] == all(row['allExact'] for row in cases)
    continuous = data.get('continuousEncoderControl', False)
    assert type(continuous) is bool
    assert len(data['sourceFiles']) == (2 if continuous else 6)
    for source in data['sourceFiles']:
        artifact(source['name'], source['sha256'])
    return dict(evidenceValid=True, allCasesExact=data['allCasesExact'],
                continuousEncoderControl=continuous,
                packetClockQualified=data.get('videoClockFromContainerPackets', False),
                cases=rows, artifacts=artifacts,
                allMediaRehashed=all(row['rehashed'] for row in artifacts),
                nativeDownloadWorkflowProven=False, fullParityProven=False)


def join_native_guard(reports, guard):
    assert guard['investigationCompleted'] is True and guard['actualNativeDurationGuard'] is True
    expected = {name + '/' + row['case']: row['allExact']
                for name, report in reports.items() for row in report['cases']}
    assert len(guard['cases']) == len(expected)
    assert {row['case'] for row in guard['cases']} == set(expected)
    for row in guard['cases']:
        assert type(row['guardAccepted']) is bool
        assert row['decodedOracleExact'] == expected[row['case']]
    return dict(cases=len(expected), coarseAcceptedButOracleFailed=sum(
        row['guardAccepted'] and not row['decodedOracleExact'] for row in guard['cases']),
        allMediaExact=all(expected.values()), fullNativeRecordingWorkflow=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--require-media', action='store_true')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = analyze(args.root.resolve(), args.require_media)
    encoded = json.dumps(result, indent=2) + '\n'
    if args.output:
        if args.output.exists():
            parser.error('Use a new report path')
        args.output.write_text(encoded, encoding='utf-8', newline='\n')
    else:
        print(encoded, end='')
