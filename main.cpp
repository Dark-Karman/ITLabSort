#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

struct TimingRow {
    std::size_t n{};
    std::optional<double> bubble_seconds;
    std::optional<double> hoare_seconds;
};

std::vector<double> generate_random_array(std::size_t n, double min_value, double max_value) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<double> dist(min_value, max_value);

    std::vector<double> data(n);
    for (double &value : data) {
        value = dist(gen);
    }
    return data;
}

bool write_array_to_file(const std::string &filename, const std::vector<double> &data) {
    std::ofstream out(filename);
    if (!out) {
        return false;
    }

    out << std::setprecision(17);
    out << data.size() << '\n';
    for (const double value : data) {
        out << value << '\n';
    }
    return true;
}

std::vector<double> read_array_from_file(const std::string &filename) {
    std::ifstream in(filename);
    if (!in) {
        throw std::runtime_error("Не удалось открыть файл: " + filename);
    }

    std::size_t n = 0;
    in >> n;
    std::vector<double> data(n);
    for (std::size_t i = 0; i < n; ++i) {
        if (!(in >> data[i])) {
            throw std::runtime_error("Ошибка чтения элемента " + std::to_string(i) + " из файла " + filename);
        }
    }
    return data;
}

bool is_sorted_non_decreasing(const std::vector<double> &data) {
    for (std::size_t i = 1; i < data.size(); ++i) {
        if (data[i] < data[i - 1]) {
            return false;
        }
    }
    return true;
}

std::optional<double> bubble_sort_with_timing(std::vector<double> &data, double max_seconds) {
    const auto start = Clock::now();

    for (std::size_t i = 0; i < data.size(); ++i) {
        bool swapped = false;
        for (std::size_t j = 0; j + 1 < data.size() - i; ++j) {
            if (data[j] > data[j + 1]) {
                std::swap(data[j], data[j + 1]);
                swapped = true;
            }
        }

        const auto now = Clock::now();
        const double elapsed = std::chrono::duration<double>(now - start).count();
        if (elapsed > max_seconds) {
            return std::nullopt;
        }

        if (!swapped) {
            break;
        }
    }

    const auto end = Clock::now();
    return std::chrono::duration<double>(end - start).count();
}

std::size_t hoare_partition(std::vector<double> &data, std::size_t low, std::size_t high) {
    const double pivot = data[low + (high - low) / 2];
    std::size_t i = low;
    std::size_t j = high;

    while (true) {
        while (data[i] < pivot) {
            ++i;
        }
        while (data[j] > pivot) {
            --j;
        }
        if (i >= j) {
            return j;
        }
        std::swap(data[i], data[j]);
        ++i;
        --j;
    }
}

void quick_sort_hoare(std::vector<double> &data, std::size_t low, std::size_t high) {
    if (low >= high) {
        return;
    }

    const std::size_t split = hoare_partition(data, low, high);
    if (split > low) {
        quick_sort_hoare(data, low, split);
    }
    if (split + 1 < high) {
        quick_sort_hoare(data, split + 1, high);
    }
}

std::optional<double> hoare_sort_with_timing(std::vector<double> &data) {
    const auto start = Clock::now();
    if (!data.empty()) {
        quick_sort_hoare(data, 0, data.size() - 1);
    }
    const auto end = Clock::now();
    return std::chrono::duration<double>(end - start).count();
}

std::string format_seconds(const std::optional<double> &value) {
    if (!value.has_value()) {
        return "> лимита";
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << *value;
    return oss.str();
}

void print_and_save_table(const std::vector<TimingRow> &rows, const std::string &filename) {
    std::ofstream out(filename);
    if (!out) {
        throw std::runtime_error("Не удалось открыть файл таблицы: " + filename);
    }

    const std::string sep = "+------------+----------------------+----------------------+";
    const std::string header = "| N          | Bubble sort (s)      | Hoare sort (s)       |";

    auto print_line = [&](std::ostream &os, const TimingRow &row) {
        os << "| " << std::setw(10) << row.n
           << " | " << std::setw(20) << format_seconds(row.bubble_seconds)
           << " | " << std::setw(20) << format_seconds(row.hoare_seconds)
           << " |\n";
    };

    std::vector<std::reference_wrapper<std::ostream>> outputs = {std::cout, out};
    for (std::ostream &os : outputs) {
        os << sep << '\n';
        os << header << '\n';
        os << sep << '\n';
        for (const auto &row : rows) {
            print_line(os, row);
        }
        os << sep << '\n';
        os << "*Для Bubble sort установлен лимит времени 15 секунд на один запуск.\n";
    }
}

int main() {
    try {
        constexpr int variant_first_digit = 1;
        constexpr int variant_second_digit = 3;
        const double min_value = 10000.0 + variant_first_digit * 1000.0 + variant_second_digit * 10.0;
        const double max_value = 30000.0 + variant_first_digit * 1000.0 + 700.0 + variant_second_digit;

        const std::vector<std::size_t> check_sizes = {50};
        const std::vector<std::size_t> experiment_sizes = {1000, 10000, 100000, 500000, 1000000};
        const double bubble_time_limit_seconds = 15.0;

        std::cout << "Диапазон генерации: [" << min_value << ", " << max_value << "]\n";

        for (const std::size_t n : check_sizes) {
            const std::string source_file = "input_" + std::to_string(n) + ".txt";
            const auto random_data = generate_random_array(n, min_value, max_value);
            if (!write_array_to_file(source_file, random_data)) {
                throw std::runtime_error("Не удалось записать исходный массив в " + source_file);
            }

            auto data_bubble = read_array_from_file(source_file);
            auto bubble_time = bubble_sort_with_timing(data_bubble, bubble_time_limit_seconds);
            if (bubble_time.has_value()) {
                if (!is_sorted_non_decreasing(data_bubble)) {
                    throw std::runtime_error("Bubble sort выдал некорректный результат для N=50");
                }
                write_array_to_file("sorted_bubble_" + std::to_string(n) + ".txt", data_bubble);
            }

            auto data_hoare = read_array_from_file(source_file);
            auto hoare_time = hoare_sort_with_timing(data_hoare);
            if (!is_sorted_non_decreasing(data_hoare)) {
                throw std::runtime_error("Hoare sort выдал некорректный результат для N=50");
            }
            write_array_to_file("sorted_hoare_" + std::to_string(n) + ".txt", data_hoare);

            std::cout << "Проверка N=50: Bubble=" << format_seconds(bubble_time)
                      << " c, Hoare=" << format_seconds(hoare_time) << " c\n";
        }

        std::vector<TimingRow> table;
        table.reserve(experiment_sizes.size());

        for (const std::size_t n : experiment_sizes) {
            const std::string source_file = "input_" + std::to_string(n) + ".txt";
            const auto random_data = generate_random_array(n, min_value, max_value);
            if (!write_array_to_file(source_file, random_data)) {
                throw std::runtime_error("Не удалось записать исходный массив в " + source_file);
            }

            auto data_bubble = read_array_from_file(source_file);
            const auto bubble_time = bubble_sort_with_timing(data_bubble, bubble_time_limit_seconds);
            if (bubble_time.has_value()) {
                if (!is_sorted_non_decreasing(data_bubble)) {
                    throw std::runtime_error("Bubble sort выдал некорректный результат для N=" + std::to_string(n));
                }
                write_array_to_file("sorted_bubble_" + std::to_string(n) + ".txt", data_bubble);
            }

            auto data_hoare = read_array_from_file(source_file);
            const auto hoare_time = hoare_sort_with_timing(data_hoare);
            if (!is_sorted_non_decreasing(data_hoare)) {
                throw std::runtime_error("Hoare sort выдал некорректный результат для N=" + std::to_string(n));
            }
            write_array_to_file("sorted_hoare_" + std::to_string(n) + ".txt", data_hoare);

            table.push_back(TimingRow{n, bubble_time, hoare_time});
        }

        print_and_save_table(table, "timing_table.txt");
        std::cout << "Таблица сохранена в timing_table.txt\n";
    } catch (const std::exception &ex) {
        std::cerr << "Ошибка: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
