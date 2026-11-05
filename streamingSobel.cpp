// Threaded streaming Sobel benchmark. The benchmark loop intentionally does
// no video I/O, display I/O, allocation, or logging.
#include "sobel_filter.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string input = "4k_video.mp4";
    std::string output;
    int frameIndex = 0;
    int threads = 1;
    int iterations = 1;
    bool display = false;
};

bool parsePositive(const char* text, int& value) {
    try {
        const int parsed = std::stoi(text);
        if (parsed < 1) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseNonNegative(const char* text, int& value) {
    try {
        const int parsed = std::stoi(text);
        if (parsed < 0) return false;
        value = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseOptions(int argc, char* argv[], Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc) {
            options.input = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            options.output = argv[++i];
        } else if (arg == "--frame" && i + 1 < argc) {
            if (!parseNonNegative(argv[++i], options.frameIndex)) return false;
        } else if (arg == "--threads" && i + 1 < argc) {
            if (!parsePositive(argv[++i], options.threads)) return false;
        } else if (arg == "--iterations" && i + 1 < argc) {
            if (!parsePositive(argv[++i], options.iterations)) return false;
        } else if (arg == "--display") {
            options.display = true;
        } else if (arg == "--no-display") {
            options.display = false;
        } else if (arg[0] != '-' && i == 1) { // retain the former "<numThreads>" form
            if (!parsePositive(argv[i], options.threads)) return false;
        } else {
            return false;
        }
    }
    return true;
}

void printUsage(const char* program) {
    std::cerr << "Usage: " << program
              << " [--threads N] [--iterations N] [--input FILE] [--frame N] [--output FILE] [--no-display|--display]\n";
}

inline uchar gray(const cv::Vec3b& pixel) {
    return static_cast<uchar>((54 * pixel[2] + 183 * pixel[1] + 19 * pixel[0]) >> 8);
}

struct WorkerBuffers {
    explicit WorkerBuffers(int columns) : a(columns), b(columns), c(columns) {}
    std::vector<uchar> a, b, c;
};

class ThreadedStreamingSobel {
public:
    ThreadedStreamingSobel(const cv::Mat& input, int threadCount)
        : frame_(input), output_(cv::Mat::zeros(input.rows, input.cols, CV_8UC1)),
          threadCount_(threadCount), buffers_(), workers_() {
        buffers_.reserve(threadCount_);
        workers_.reserve(threadCount_);
        const int workRows = std::max(0, frame_.rows - 2);
        for (int i = 0; i < threadCount_; ++i) {
            buffers_.emplace_back(frame_.cols);
            const int start = 1 + i * workRows / threadCount_;
            const int end = 1 + (i + 1) * workRows / threadCount_;
            workers_.emplace_back(&ThreadedStreamingSobel::workerLoop, this, i, start, end);
        }
    }

    ~ThreadedStreamingSobel() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        start_.notify_all();
        for (std::thread& worker : workers_) worker.join();
    }

    // The code enclosed by perf stat: wake pre-created workers, then wait.
    void runOnce() {
        std::unique_lock<std::mutex> lock(mutex_);
        completed_ = 0;
        ++generation_;
        start_.notify_all();
        done_.wait(lock, [this] { return completed_ == threadCount_; });
    }

    const cv::Mat& output() const { return output_; }

private:
    void workerLoop(int workerId, int startRow, int endRow) {
        unsigned seenGeneration = 0;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                start_.wait(lock, [this, &seenGeneration] {
                    return stopping_ || generation_ != seenGeneration;
                });
                if (stopping_) return;
                seenGeneration = generation_;
            }
            processRows(startRow, endRow, buffers_[workerId]);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (++completed_ == threadCount_) done_.notify_one();
            }
        }
    }

    void processRows(int startRow, int endRow, WorkerBuffers& buffers) {
        const int first = std::max(startRow, 1);
        const int last = std::min(endRow, frame_.rows - 1); // exclusive
        const int columns = frame_.cols;
        if (first >= last || columns < 3) return;

        uchar* previous = buffers.a.data();
        uchar* current = buffers.b.data();
        uchar* next = buffers.c.data();
        const cv::Vec3b* rowAbove = frame_.ptr<cv::Vec3b>(first - 1);
        const cv::Vec3b* rowCurrent = frame_.ptr<cv::Vec3b>(first);
        for (int x = 0; x < columns; ++x) {
            previous[x] = gray(rowAbove[x]);
            current[x] = gray(rowCurrent[x]);
        }

        for (int y = first; y < last; ++y) {
            const cv::Vec3b* rowBelow = frame_.ptr<cv::Vec3b>(y + 1);
            for (int x = 0; x < columns; ++x) next[x] = gray(rowBelow[x]);

            uchar* out = output_.ptr<uchar>(y);
            for (int x = 1; x < columns - 1; ++x) {
                const int gx = -previous[x - 1] + previous[x + 1]
                               - 2 * current[x - 1] + 2 * current[x + 1]
                               - next[x - 1] + next[x + 1];
                const int gy = previous[x - 1] + 2 * previous[x] + previous[x + 1]
                               - next[x - 1] - 2 * next[x] - next[x + 1];
                out[x] = static_cast<uchar>(std::min(255, std::abs(gx) + std::abs(gy)));
            }
            uchar* old = previous;
            previous = current;
            current = next;
            next = old;
        }
    }

    const cv::Mat& frame_;
    cv::Mat output_;
    int threadCount_;
    std::vector<WorkerBuffers> buffers_;
    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable start_, done_;
    unsigned generation_ = 0;
    int completed_ = 0;
    bool stopping_ = false;
};

std::uint64_t checksum(const cv::Mat& image) {
    std::uint64_t total = 0;
    for (int y = 0; y < image.rows; ++y) {
        const uchar* row = image.ptr<uchar>(y);
        for (int x = 0; x < image.cols; ++x) total += row[x];
    }
    return total;
}

} // namespace

int main(int argc, char* argv[]) {
    Options options;
    if (!parseOptions(argc, argv, options)) {
        printUsage(argv[0]);
        return 1;
    }

    cv::VideoCapture video(options.input);
    cv::Mat frame;
    if (!video.isOpened()) {
        std::cerr << "Could not open " << options.input << "\n";
        return 1;
    }
    for (int index = 0; index <= options.frameIndex; ++index) {
        if (!video.read(frame) || frame.empty()) {
            std::cerr << "Could not load frame " << options.frameIndex << " from " << options.input << "\n";
            return 1;
        }
    }
    if (frame.empty()) {
        std::cerr << "Could not load a frame from " << options.input << "\n";
        return 1;
    }

    ThreadedStreamingSobel sobel(frame, options.threads); // allocations and worker creation occur here
    for (int iteration = 0; iteration < options.iterations; ++iteration) sobel.runOnce();

    const std::uint64_t resultChecksum = checksum(sobel.output());
    std::cout << "configuration=optimized"
              << " threads=" << options.threads
              << " frame=" << options.frameIndex
              << " iterations=" << options.iterations
              << " checksum=" << resultChecksum << '\n';

    // Optional correctness artifact: written only after all measured work.
    if (!options.output.empty() && !cv::imwrite(options.output, sobel.output())) {
        std::cerr << "Could not write " << options.output << "\n";
        return 1;
    }
    if (options.display) {
        cv::imshow("Sobel", sobel.output()); // deliberately after the benchmark loop
        cv::waitKey(0);
    }
    return 0;
}
