#!/bin/bash
set -e
cd ${0%/*}
while :; do
  while getopts i:o:-: arg; do
    case $arg in
      i)
        input="$OPTARG"
        echo Option $arg specified.
        ;;
      o)
        output="$OPTARG"
        echo $output
        echo Option $arg specified.
        ;;
      *)
        echo Unknown option: $OPTARG. 2 >/dev/null
        ;;
    esac
    [ ! -z "$input" ] && [ ! -z "$output" ] && break;
  done
  ((OPTIND++))
  [ $OPTIND -gt $# ] && break
done

if [[ ! -z "$input" ]]; then
  input="$(realpath "$input")"
  input="${input%/*}"
fi

if [[ ! -z "$output" ]]; then
  output="$(realpath "$output")"
fi
echo Input: $input
echo Output: $output

docker build -t videosubfinder:cuda -f run_cuda.Dockerfile .
if [[ "$input" = "$output" ]]; then
docker run -it --gpus all --rm -v "$input":"$input" videosubfinder:cuda /VideoSubFinderCli "$@"
else
docker run -it --gpus all --rm -v "$input":"$input" -v "$output":"$output" videosubfinder:cuda /VideoSubFinderCli "$@"
fi
