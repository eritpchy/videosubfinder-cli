#!/usr/bin/env python3
"""Check lossless, line-delimited CLI records under concurrent output."""
from collections import Counter
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    count = 1500
    progress = [[f'Progress: {i % 100}% elapsed={worker * count + i}s'
                 for i in range(count)] for worker in range(2)]
    frames = [[f'Frame: 0_00_04_000__0_00_06_039_{worker}_{i}'
               for i in range(count)] for worker in range(2, 4)]
    reports = [[f'Video ended or cannot read new frames. worker={worker} record={i} '
                + ('x' * 8192 if i % 25 == 0 else '')
                for i in range(count)] for worker in range(4, 6)]
    producers = progress + frames + reports
    initial = 'Record framing regression'
    final = ['Progress: 100%', 'FINISH']
    expected = Counter([initial, *final] + [line for group in producers for line in group])
    expected_reports = Counter([initial] + [line for group in reports for line in group])
    binary = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix='vsf console records ') as temporary:
        report = Path(temporary) / 'report.log'
        for iteration in range(4):
            report.write_text('Stale report must be overwritten\n', encoding='utf-8')
            separate = iteration == 3
            result = subprocess.run([str(binary), str(report)], stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE if separate else subprocess.STDOUT,
                                    text=True, encoding='utf-8', timeout=30)
            assert result.returncode == 0, (result.returncode, result.stdout[-4000:], (result.stderr or '')[-4000:])
            assert result.stdout.endswith('\n'), 'Unterminated stdout record'
            if separate:
                assert result.stderr.endswith('\n'), 'Unterminated stderr record'
                assert Counter(result.stdout.splitlines()) == expected_reports
                assert result.stderr.splitlines()[-2:] == final
            else:
                assert result.stdout.splitlines()[0] == initial
                assert result.stdout.splitlines()[-2:] == final
            lines = (result.stdout + (result.stderr or '')).splitlines()
            assert Counter(lines) == expected, f'Lost, split, joined, or duplicated records (run {iteration})'
            positions = {line: index for index, line in enumerate(lines)}
            for group in producers:
                order = [positions[line] for line in group]
                assert order == sorted(order), 'Producer record order changed'
            assert Counter(report.read_text(encoding='utf-8').splitlines()) == expected_reports
    print('PASS: 36,000 concurrent records, merged/separate streams, report mirroring and ordering')


if __name__ == '__main__':
    main()
