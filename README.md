# videosubfinder-cli

## Install dependencies
- macOS
```bash
  brew install wxwidgets@3.2 opencv@4 ffmpeg tbb
```
- Ubuntu 20.04

    [Build/Docker/cuda/run_cuda.Dockerfile](Build/Docker/cuda/run_cuda.Dockerfile)

NOTE: The static version does not require installation dependencies!

## Build from sources
- macOS
    
    [.github/workflows/build-darwin-x64.yml](.github/workflows/build-darwin-x64.yml)


- Ubuntu 20.04
    
    [Build/Docker/cpu/build.sh](Build/Docker/cpu/build.sh)


## Usage
```bash
Usage: VideoSubFinderCli [-h] [--verbose] [-c] [-r] [-ces <str>] [-i <str>] [-ovocv] 
  [-ovffmpeg] [-uc] [-dsi] [-s <str>] [-e <str>] [-te <double>] [-be <double>] 
  [-le <double>] [-re <double>] [-o <str>] [-nthr <num>] [-h]
-h, --help                                    	show this help message
--verbose                                     	generate verbose log messages
-c, --clear_dirs                              	Clear Folders (remove all images), performed before any other steps
-r, --run_search                              	Run Search (find frames with hardcoded text (hardsub) on video)
-ces, --create_empty_sub=<str>                	Create Empty Sub With Provided Output File Name (*.srt)
-i, --input_video=<str>                       	input video file
-ovocv, --open_video_opencv                   	open video by OpenCV (default)
-ovffmpeg, --open_video_ffmpeg                	open video by FFMPEG
-uc, --use_cuda                               	use cuda
-dsi, --dont_save_images                      	Don't save images
-s, --start_time=<str>                        	start time, default = 0:00:00:000 (in format hour:min:sec:milisec)
-e, --end_time=<str>                          	end time, default = video length
-te, --top_video_image_percent_end=<double>   	top video image percent offset from image bottom, can be in range [0.0,1.0], default = 1.0
-be, --bottom_video_image_percent_end=<double>	bottom video image percent offset from image bottom, can be in range [0.0,1.0], default = 0.0
-le, --left_video_image_percent_end=<double>  	left video image percent end, can be in range [0.0,1.0], default = 0.0
-re, --right_video_image_percent_end=<double> 	right video image percent end, can be in range [0.0,1.0], default = 1.0
-o, --output_dir=<str>                        	output dir (root directory where results will be stored)
-nthr, --num_threads=<num>                    	number of threads used for Run Search
-h, --help                                    	show this help message

Example of usage:
chmod +x ./VideoSubFinderCli.run
./VideoSubFinderCli.run -c -r -i "./test_video.mp4"  -o "./ResultsDir" -te 0.5 -be 0.1 -le 0.1 -re 0.9 -s 0:00:10:300 -e 0:00:13:100
```

## Donate
- Bitcoin: bc1q9t4t6g42hge2stvu96rzr6kn7ynq52al5uv0pg