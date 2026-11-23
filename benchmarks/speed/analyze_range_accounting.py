"""Read-only range accounting for owned compare-completion origin snapshots.

Server send counts prove bytes accepted by its socket, not client retention.
Snapshots are taken after the app lifetime but may precede server-handler teardown.
"""
from pathlib import Path
import argparse
import json


def coverage(intervals, size):
    """Return covered bytes, repeated sends and holes in half-open intervals."""
    assert isinstance(size, int) and size > 0
    events = {}
    total = 0
    for start, end in intervals:
        assert 0 <= start <= end <= size
        if start == end:
            continue
        events[start] = events.get(start, 0) + 1
        events[end] = events.get(end, 0) - 1
        total += end - start
    active = previous = unique = 0
    holes, overlap = [], []
    for point in sorted(set(events) | {0, size}):
        if point > previous:
            if active:
                unique += point - previous
                if active > 1:
                    overlap.append({'start': previous, 'endExclusive': point,
                                    'multiplicity': active})
            else:
                holes.append([previous, point])
        active += events.get(point, 0)
        assert active >= 0
        previous = point
    assert active == 0
    return {'sentBytes': total, 'uniqueBytes': unique,
            'repeatedBytes': total - unique, 'holes': holes, 'overlap': overlap}


def analyze(root):
    data = json.loads((root / 'results.json').read_text(encoding='utf-8-sig'))
    rows = []
    for result in data['results']:
        folder = root / f"{result['case']}-{result['client']}-{result['trial']}"
        origin = json.loads((folder / 'origin.json').read_text(encoding='utf-8-sig'))
        intervals, requests = [], []
        for connection in origin['detail']:
            for request in connection['requests']:
                sent = request['sent']
                assert isinstance(sent, int) and sent >= 0
                if sent:
                    assert request['method'] == 'GET' and request['status'] in (200, 206)
                    assert sent <= request['end'] - request['start'] + 1
                    intervals.append((request['start'], request['start'] + sent))
                requests.append({'connection': connection['id'], 'range': request['range'],
                                 'start': request['start'], 'declaredEnd': request['end'],
                                 'sent': sent, 'outcome': request['outcome'],
                                 'serverHandlerClosed': connection['closed'] is not None})
        account = coverage(intervals, result['bytes'])
        assert account['sentBytes'] == origin['bytes_sent'] == result['originBytes']
        assert len(requests) == origin['requests'] == result['requests']
        assert account['uniqueBytes'] == result['bytes'] and not account['holes']
        assert result['hashMatches']
        rows.append({'case': result['case'], 'client': result['client'],
                     'trial': result['trial'], **account, 'requests': requests,
                     'allServerHandlersClosedAtSnapshot': all(
                         c['closed'] is not None for c in origin['detail'])})
    return {'rows': rows, 'scope': 'Owned origin socket-send intervals at saved snapshot',
            'clientDiscardOrRetentionMeasured': False,
            'laterServerActivityMeasured': False,
            'rangeAccountingComplete': True}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = analyze(args.root.resolve())
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'runs': len(report['rows']), 'rangeAccountingComplete': True}))
