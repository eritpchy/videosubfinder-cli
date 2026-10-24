#include "VideoSubFinderCli.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;
std::string g_ReportFileName;
static std::string g_DefStringForEmptySub = "sub duration: %sub_duration%";
static bool g_join_subs_and_correct_time = true;

static fs::path executablePath() {
#ifdef _WIN32
    std::vector<wchar_t> path(32768);
    auto size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!size || size == path.size()) throw std::runtime_error("Cannot locate executable");
    return fs::path(path.data());
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> path(size);
    if (_NSGetExecutablePath(path.data(), &size)) throw std::runtime_error("Cannot locate executable");
    return fs::canonical(path.data());
#else
    return fs::canonical("/proc/self/exe");
#endif
}

static std::string trim(const std::string& s) {
    auto first = s.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}

using Settings = std::map<std::string, std::string>;
template<class T> static void ReadProperty(const Settings& settings, T& value, const std::string& key) {
    auto it = settings.find(key);
    if (it == settings.end()) return;
    std::istringstream input(it->second);
    T parsed;
    if (!(input >> parsed) || !(input >> std::ws).eof())
        throw std::runtime_error("Invalid setting: " + key);
    value = parsed;
}
static void ReadProperty(const Settings& settings, std::string& value, const std::string& key) {
    auto it = settings.find(key);
    if (it != settings.end()) value = it->second;
}
static void ReadProperty(const Settings& settings, std::vector<std::string>& value, const std::string& key) {
    auto it = settings.find(key);
    if (it == settings.end()) return;
    value.clear();
    if (it->second == "none") return;
    std::istringstream input(it->second);
    std::string item;
    while (std::getline(input, item, ';')) if (!trim(item).empty()) value.push_back(trim(item));
}

#include "Settings.h"

static s64 parseTime(const std::string& value) {
    std::smatch match;
    if (!std::regex_match(value, match, std::regex(R"((\d+):(\d{1,2}):(\d{1,2}):(\d{1,3}))")))
        throw std::runtime_error("Time must use hour:min:sec:millisec");
    auto h = std::stoll(match[1]), m = std::stoll(match[2]);
    auto s = std::stoll(match[3]), ms = std::stoll(match[4]);
    if (h > 100000 || m > 59 || s > 59 || ms > 999) throw std::runtime_error("Time is out of range");
    return ((h * 60 + m) * 60 + s) * 1000 + ms;
}

static std::string srtTime(s64 ms) {
    char value[40];
    std::snprintf(value, sizeof(value), "%02lld:%02lld:%02lld,%03lld",
        ms / 3600000, ms / 60000 % 60, ms / 1000 % 60, ms % 1000);
    return value;
}

static void writeSubtitles(const fs::path& path) {
    std::vector<std::pair<s64, s64>> intervals;
    const std::regex pattern(R"((\d+)_(\d+)_(\d+)_(\d+)__(\d+)_(\d+)_(\d+)_(\d+)_.*)");
    for (const auto& name : g_file_names_vector) {
        std::smatch m;
        if (!std::regex_match(name, m, pattern)) throw std::runtime_error("Invalid frame timestamp: " + name);
        auto time = [&](int n) { return ((std::stoll(m[n]) * 60 + std::stoll(m[n+1])) * 60 + std::stoll(m[n+2])) * 1000 + std::stoll(m[n+3]); };
        intervals.emplace_back(time(1), time(5));
    }
    std::sort(intervals.begin(), intervals.end());
    if (!path.parent_path().empty()) fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write subtitle file: " + path.string());
    size_t index = 0;
    for (const auto& [begin, end] : intervals) {
        std::ostringstream duration;
        duration << (end - begin) / 1000 << ',';
        duration.width(3); duration.fill('0'); duration << (end - begin) % 1000;
        out << ++index << '\n' << srtTime(begin) << " --> " << srtTime(end) << '\n'
            << std::regex_replace(g_DefStringForEmptySub, std::regex("%sub_duration%"), duration.str()) << "\n\n";
    }
    if (!out) throw std::runtime_error("Failed to write subtitle file");
}

static void clearTextImages() {
    for (const auto& entry : fs::directory_iterator(fs::u8path(g_work_dir) / "RGBImages")) {
        if (!entry.is_regular_file()) continue;
        cv::Mat encoded;
        ReadFile(encoded, entry.path().u8string());
        auto image = cv::imdecode(encoded, cv::IMREAD_COLOR);
        if (image.empty()) throw std::runtime_error("Cannot read frame: " + entry.path().string());
        int w = image.cols, h = image.rows, W = w, H = h, xmin = 0, ymin = 0;
        auto stem = entry.path().stem().string();
        GetImInfo(stem, w, h, &W, &H, &xmin, nullptr, &ymin, nullptr, &stem);
        simple_buffer<u8> bgr(w*h*3), ff(w*h), sf(w*h), tf(w*h), ne(w*h), y(w*h), il;
        BGRMatToImage(image, w, h, bgr);
        if (GetTransformedImage(bgr, ff, sf, tf, ne, y, w, h, W, H, 0, w-1)) {
            simple_buffer<u8> cleared(w*h*g_scale*g_scale, static_cast<u8>(255));
            FindTextLines(bgr, cleared, tf, ff, ne, il, "/TXTImages/", stem, w, h, W, H, xmin, ymin);
        }
    }
}

static int runCli(const std::vector<std::string>& argv) {
    const std::map<std::string, std::string> aliases = {
        {"h","help"},{"c","clear_dirs"},{"r","run_search"},{"ces","create_empty_sub"},
        {"i","input_video"},{"o","output_dir"},{"gs","general_settings"},{"nthr","num_threads"},
        {"s","start_time"},{"e","end_time"},{"te","top"},{"be","bottom"},{"le","left"},{"re","right"},
        {"top_video_image_percent_end","top"},{"bottom_video_image_percent_end","bottom"},
        {"left_video_image_percent_end","left"},{"right_video_image_percent_end","right"},
        {"ovocv","open_video_opencv"},{"ovffmpeg","open_video_ffmpeg"},{"uc","use_cuda"},
        {"dsi","dont_save_images"},{"ccti","create_cleared_text_images"}};
    const std::vector<std::string> flags = {"help","version","verbose","clear_dirs","run_search", "dont_save_images", "create_cleared_text_images", "open_video_opencv", "open_video_ffmpeg", "use_cuda"};
    const std::vector<std::string> values = {"input_video","output_dir","general_settings","num_threads", "start_time","end_time","top","bottom","left","right","create_empty_sub"};
    Settings options;
    for (size_t i=1; i<argv.size(); ++i) {
        auto arg = argv[i];
        if (arg.empty() || arg[0]!='-') throw std::runtime_error("Unexpected argument: " + arg);
        arg.erase(0, arg.find_first_not_of('-'));
        auto equals = arg.find('=');
        auto key = arg.substr(0, equals);
        if (aliases.count(key)) key = aliases.at(key);
        if (std::find(flags.begin(), flags.end(), key)!=flags.end()) {
            if (equals!=std::string::npos) throw std::runtime_error("Flag does not take a value: " + key);
            options[key]="1";
        } else if (std::find(values.begin(), values.end(), key)!=values.end()) {
            if (equals!=std::string::npos) options[key]=arg.substr(equals+1);
            else if (i+1<argv.size()) options[key]=argv[++i];
            else throw std::runtime_error("Missing option value: " + key);
        } else throw std::runtime_error("Unknown option: " + key);
    }
    if (options.count("version")) { std::cout << "VideoSubFinderCli " << VSF_VERSION << '\n'; return 0; }
    if (options.count("help")) {
        std::cout << "VideoSubFinderCli " << VSF_VERSION << "\n"
            "Usage: VideoSubFinderCli -r -i VIDEO -o OUTPUT [options]\n"
            "  -c, --clear_dirs                 Clear generated image files\n"
            "  -r, --run_search                 Search for hardcoded subtitles\n"
            "  -ces, --create_empty_sub PATH    Write SRT intervals\n"
            "  -ccti, --create_cleared_text_images  Create TXTImages from RGBImages\n"
            "  -dsi, --dont_save_images         Write intervals without images\n"
            "  -gs, --general_settings PATH     Load settings/general.cfg\n"
            "  -nthr, --num_threads N           Search threads (2-64)\n"
            "  -s, --start_time H:M:S:MS        Start timestamp\n"
            "  -e, --end_time H:M:S:MS          End timestamp\n"
            "  -te, -be, -le, -re NUMBER        Crop fractions (0-1)\n"
            "  -ovffmpeg, --open_video_ffmpeg   Use OpenCV's FFmpeg backend\n"
            "  -ovocv, --open_video_opencv      Select the available OpenCV backend\n"
            "  -uc, --use_cuda                 Require CUDA support\n"
            "  --verbose --version -h --help\n";
        return 0;
    }
    if (!options.count("run_search") && !options.count("create_cleared_text_images"))
        throw std::runtime_error("Specify --run_search or --create_cleared_text_images");
    g_app_dir = executablePath().parent_path().u8string();
    auto settings = options.count("general_settings") ? fs::u8path(options.at("general_settings")) : fs::u8path(g_app_dir)/"settings/general.cfg";
    if (options.count("general_settings") || fs::exists(settings)) loadSettings(settings);
    auto output = fs::absolute(fs::u8path(options.count("output_dir") ? options.at("output_dir") : "."));
    fs::create_directories(output);
    g_work_dir = output.u8string();
    g_ReportFileName = (output/"report.log").u8string();
    for (const auto& folder : {"RGBImages","ISAImages","ILAImages","TXTImages","TXTImagesJoined","TestImages","TXTResults","DebugImages"}) {
        auto path = output/folder;
        if (fs::is_symlink(path)) throw std::runtime_error("Output image directory must not be a symbolic link");
        fs::create_directories(path);
        if (options.count("clear_dirs")) for (const auto& entry : fs::directory_iterator(path)) {
            auto extension = entry.path().extension().string();
            if (entry.is_regular_file() && (extension==".jpeg" || extension==".jpg" || extension==".png" || extension==".bmp")) fs::remove(entry.path());
        }
    }
    g_save_images = !options.count("dont_save_images");
    g_im_save_format = ".jpeg";
    ReadProperty(options, g_threads, "num_threads");
    if (g_threads!=-1 && (g_threads<2 || g_threads>64)) throw std::runtime_error("Thread count must be 2-64");
    if (g_DL<2 || g_DL>64 || g_scale<1 || g_scale>8) throw std::runtime_error("Invalid sub_frame_length or image scale");
    g_pViewRGBImage = [](simple_buffer<int>&,int,int) {};
    for (int i=0;i<2;++i) {
        g_pViewImage[i] = [](simple_buffer<int>&,int,int) {};
        g_pViewBGRImage[i] = [](simple_buffer<u8>&,int,int) {};
        g_pViewGreyscaleImage[i] = [](simple_buffer<u8>&,int,int) {};
    }
    g_color_ranges = GetColorRanges(g_use_filter_color);
    g_outline_color_ranges = GetColorRanges(g_use_outline_filter_color);
    if (options.count("use_cuda")) {
#ifndef USE_CUDA
        throw std::runtime_error("This binary was built without CUDA");
#else
        if (!InitCUDADevice()) throw std::runtime_error("CUDA device is unavailable");
        g_use_cuda_gpu = true;
#endif
    }
    if (options.count("run_search")) {
        if (!options.count("input_video")) throw std::runtime_error("Input video is required");
        auto input = fs::absolute(fs::u8path(options.at("input_video")));
        if (!fs::is_regular_file(input)) throw std::runtime_error("Input video does not exist: " + input.string());
        auto* video = GetOCVVideoObject();
        video->m_Dir = g_work_dir;
        int backend = options.count("open_video_opencv") ? cv::CAP_ANY : cv::CAP_FFMPEG;
        if (!video->OpenMovie(input.u8string(), nullptr, backend)) throw std::runtime_error("Cannot decode input video: " + input.string());
        double left=0,right=1,top=1,bottom=0;
        ReadProperty(options,left,"left"); ReadProperty(options,right,"right");
        ReadProperty(options,top,"top"); ReadProperty(options,bottom,"bottom");
        if (!std::isfinite(left+right+top+bottom) || left<0 || right>1 || left>=right || bottom<0 || top>1 || bottom>=top)
            throw std::runtime_error("Crop fractions must satisfy 0 <= left < right <= 1 and 0 <= bottom < top <= 1");
        s64 begin = options.count("start_time") ? parseTime(options.at("start_time")) : 0;
        s64 end = options.count("end_time") ? parseTime(options.at("end_time")) : video->m_Duration;
        if (end<=begin || begin>=video->m_Duration) throw std::runtime_error("Invalid video time interval");
        video->SetVideoWindowSettins(left,right,1-top,1-bottom);
        if (video->m_w<8 || video->m_h<8) throw std::runtime_error("Crop must be at least 8x8 pixels");
        if (!video->SetNullRender()) throw std::runtime_error("Cannot initialize decoder");
        const auto started=std::chrono::steady_clock::now();
        auto last=started-std::chrono::seconds(1);
        g_progress_callback = [begin,end,started,&last](s64 current,s64) {
            auto now=std::chrono::steady_clock::now();
            if (now-last<std::chrono::seconds(1)) return;
            last=now;
            std::cerr << "Progress: " << std::clamp(100*(current-begin)/(end-begin),0LL,99LL)
                << "% elapsed=" << std::chrono::duration_cast<std::chrono::seconds>(now-started).count() << "s\n";
        };
        g_file_names_vector.clear();
        g_RunSubSearch=1;
        FastSearchSubtitles(video,begin,end);
        g_progress_callback = {};
        video->CloseMovie();
        std::cerr << "Progress: 100%\n";
        if (options.count("create_empty_sub")) writeSubtitles(fs::u8path(options.at("create_empty_sub")));
    }
    if (options.count("create_cleared_text_images")) clearTextImages();
    std::cerr << "FINISH\n";
    return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
    try {
        std::vector<std::string> arguments;
        for (int i=0;i<argc;++i) {
            int size=WideCharToMultiByte(CP_UTF8,0,argv[i],-1,nullptr,0,nullptr,nullptr);
            if (!size) throw std::runtime_error("Cannot decode command line");
            std::string argument(size, '\0');
            WideCharToMultiByte(CP_UTF8,0,argv[i],-1,argument.data(),size,nullptr,nullptr);
            argument.pop_back();
            arguments.push_back(std::move(argument));
        }
        return runCli(arguments);
    } catch (const std::exception& error) { std::cerr << "ERROR: " << error.what() << '\n'; return 1; }
}
#else
int main(int argc, char** argv) {
    try { return runCli(std::vector<std::string>(argv,argv+argc)); }
    catch (const std::exception& error) { std::cerr << "ERROR: " << error.what() << '\n'; return 1; }
}
#endif
