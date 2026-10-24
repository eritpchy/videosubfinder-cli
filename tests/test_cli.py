#!/usr/bin/env python3
"""Exercise the installed CLI with generated, independently timed subtitles."""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


def invoke(command, *, success=True, cwd=None):
    result = subprocess.run([str(x) for x in command], cwd=cwd, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=90)
    if (result.returncode == 0) != success:
        raise AssertionError(f"Exit {result.returncode}: {command}\n{result.stdout}")
    if re.search(r"AddressSanitizer|runtime error:", result.stdout):
        raise AssertionError(result.stdout)
    return result.stdout


def fixtures(root):
    supplied = os.environ.get('VSF_TEST_MEDIA')
    if supplied:
        for extension in ['mkv','mp4']:
            shutil.copy2(Path(supplied)/f'input video.{extension}', root/f'input video.{extension}')
    else:
        fonts = [Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'),
                 Path('/System/Library/Fonts/Supplemental/Arial.ttf'),
                 Path('C:/Windows/Fonts/arial.ttf')]
        font = next((p for p in fonts if p.is_file()), None)
        assert font, 'A system TrueType font is required for fixture generation'
        shutil.copy2(font, root/'font.ttf')
        filters = ','.join(
            f"drawtext=fontfile=font.ttf:text='{text}':fontsize=32:fontcolor=white:"
            f"borderw=2:bordercolor=black:x=(w-text_w)/2:y=h-65:enable='between(t,{start},{end})'"
            for text,start,end in [('HELLO WORLD',1,3),('SECOND LINE',4,6)])
        invoke(['ffmpeg','-v','error','-y','-f','lavfi','-i','color=c=0x1c2c42:s=640x360:r=25:d=7',
                '-vf',filters,'-c:v','ffv1','input video.mkv'],cwd=root)
        invoke(['ffmpeg','-v','error','-y','-i','input video.mkv','-c:v','mpeg4','-q:v','2',
                'input video.mp4'],cwd=root)
    return root/'input video.mkv', root/'input video.mp4'


def check_srt(path):
    text = path.read_text(encoding='utf-8')
    times = re.findall(r'(\d{2}):(\d{2}):(\d{2}),(\d{3}) --> (\d{2}):(\d{2}):(\d{2}),(\d{3})',text)
    intervals = [tuple(((int(parts[n])*60+int(parts[n+1]))*60+int(parts[n+2]))*1000+int(parts[n+3])
                       for n in [0,4]) for parts in times]
    assert len(intervals)>=2, text
    for start, end in intervals:
        assert 0<=start<end<=7100, intervals
    for start, end in [(1000,3000),(4000,6000)]:
        assert any(abs(a-start)<500 and abs(b-end)<500 for a,b in intervals), intervals
    return intervals


def main():
    binary = Path(sys.argv[1]).resolve()
    assert binary.is_file(), binary
    assert 'create_cleared_text_images' in invoke([binary,'--help'])
    assert re.search(r'VideoSubFinderCli \d+\.\d+\.\d+',invoke([binary,'--version']))
    root_dir = Path(os.environ.get('VSF_TEST_ROOT','build/test-work')).resolve()
    root_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='cli regression ',dir=root_dir) as temporary:
        root = Path(temporary)
        mkv, mp4 = fixtures(root)
        results = {}
        base = [binary,'-r','-i',mkv,'-o',root/'invalid']
        for extra in [['--unknown'],['-te','2'],['-le','0.8','-re','0.2'],['-te','nan'],
                      ['-s','bad'],['-s','0:60:00:000'],['-nthr','0'],['-nthr','2x'],['-e','0:00:00:000']]:
            invoke(base+extra,success=False,cwd=root)
        invoke([binary,'-r','-i',root/'missing.mkv','-o',root/'missing'],success=False,cwd=root)
        (root/'broken.mkv').write_bytes(b'not a video')
        invoke([binary,'-r','-i',root/'broken.mkv','-o',root/'broken'],success=False,cwd=root)
        (root/'bad.cfg').write_text('text_alignment = 99\n')
        invoke(base+['-gs',root/'bad.cfg'],success=False,cwd=root)
        for media in [mkv,mp4]:
            output = root/media.suffix[1:]
            subtitle = output/'subtitle.srt'
            log=invoke([binary,'-c','-r','-i',media,'-o',output,'-ces',subtitle,
                        '-te','0.4','-be','0','-nthr','2','-ovffmpeg'],cwd=root)
            assert 'Progress: 100%' in log and 'FINISH' in log, log
            results[media.suffix] = check_srt(subtitle)
            frames=list((output/'RGBImages').glob('*.jpeg'))
            assert frames, log
            assert all(':' not in f.name for f in frames), frames
            invoke([binary,'--create_cleared_text_images','-o',output],cwd=root)
            assert list((output/'TXTImages').glob('*.jpeg')), 'No cleared text images'
        output=root/'no images 字幕'
        output.mkdir()
        log=invoke([binary,'--run_search','--input_video',mkv,'--output_dir',output,
                    '--create_empty_sub',output/'sub.srt','--dont_save_images',
                    '--top_video_image_percent_end=0.4','--num_threads=2'],cwd=root)
        check_srt(output/'sub.srt')
        assert not list((output/'RGBImages').iterdir()), log
        # A relocated binary must use its own settings and retain caller-relative paths.
        unicode_video=root/'字幕 input.mkv'
        shutil.copy2(mkv,unicode_video)
        unicode_output=root/'Unicode 字幕'
        invoke([binary,'-r','-i',unicode_video,'-o',unicode_output,'-ces',unicode_output/'字幕.srt',
                '-dsi','-te','0.4','-nthr','2'],cwd=root)
        check_srt(unicode_output/'字幕.srt')
        relative=root/'relative'
        invoke([binary,'-r','-i',mkv.name,'-o','relative','-ces','relative/sub.srt',
                '-dsi','-te','0.4','-nthr','2'],cwd=root)
        check_srt(relative/'sub.srt')
        results['checks']='help, version, validation, MKV, MP4, SRT timing, frames, CCTI, no-images, relative paths'
        print(json.dumps(results,indent=2))


if __name__=='__main__':
    main()
