#ifndef TIMER_H
#define TIMER_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

class timer {
  public:
    explicit timer(int64_t total_units, int bar_width = 32)
      : total_units_(total_units),
        bar_width_(bar_width) {
      start();
    }

    void start() {
      start_ = std::chrono::steady_clock::now();
    }

    void update_progress(int64_t completed_units) {
      const auto now = std::chrono::steady_clock::now();
      const double elapsed_s = elapsed_seconds_since_start(now);
      const double frac =
        total_units_ > 0
          ? std::clamp(
                static_cast<double>(completed_units) / static_cast<double>(total_units_),
                0.0,
                1.0)
          : 1.0;
      print_progress_line(frac, elapsed_s);
    }

    void print_complete(const std::string& message) {
      const double total_s =
        elapsed_seconds_since_start(std::chrono::steady_clock::now());
      std::cerr << '\n' << message << " in " << format_duration_hms(total_s) << ".\n"
                << std::flush;
    }

    static std::string format_duration_hms(double seconds) {
      if (seconds < 0.0) {
        seconds = 0.0;
      }
      if (seconds < 60.0) {
        std::ostringstream os;
        os << std::fixed << std::setprecision(1) << seconds << 's';
        return os.str();
      }
      const auto whole = static_cast<int64_t>(seconds);
      const int h = static_cast<int>(whole / 3600);
      const int m = static_cast<int>((whole % 3600) / 60);
      const int s = static_cast<int>(whole % 60);
      std::ostringstream os;
      os << std::setfill('0');
      if (h > 0) {
        os << h << 'h' << std::setw(2) << m << 'm' << std::setw(2) << s << 's';
      } else {
        os << m << 'm' << std::setw(2) << s << 's';
      }
      return os.str();
    }

  private:
    static constexpr double eta_min_fraction = 0.01;

    int64_t total_units_;
    int bar_width_;
    std::chrono::steady_clock::time_point start_;

    double elapsed_seconds_since_start(
      std::chrono::steady_clock::time_point now) const {
      return std::chrono::duration<double>(now - start_).count();
    }

    void print_progress_line(double frac, double elapsed_s) const {
      int filled =
        static_cast<int>(std::round(frac * static_cast<double>(bar_width_)));
      filled = std::clamp(filled, 0, bar_width_);

      std::cerr << '\r' << '[';
      for (int k = 0; k < bar_width_; ++k) {
        std::cerr << (k < filled ? '#' : '-');
      }
      std::cerr << "] " << std::fixed << std::setprecision(1) << (100.0 * frac) << "%  "
                << format_duration_hms(elapsed_s) << " elapsed";
      if (frac >= eta_min_fraction && frac < 1.0) {
        const double eta_s = elapsed_s * (1.0 / frac - 1.0);
        std::cerr << "  ETA " << format_duration_hms(eta_s);
      }
      std::cerr << std::flush;
    }
};

#endif
