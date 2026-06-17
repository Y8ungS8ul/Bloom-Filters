#include "HyperLogLog.hpp"
#include <functional>

/**
* @details Реализация указанной функциональности для HyperlLogLog.hpp
*/

HyperLogLog::HyperLogLog(int p) 
    : p(p), m(1 << p), registers(m, 0) {}

void HyperLogLog::insert(const std::string& element) {
    // Используем стандартную хеш-функцию для строк
    size_t hash = std::hash<std::string>{}(element);
    
    // Индекс регистра: первые p бит
    int index = hash % m;
    
    // Количество ведущих нулей в оставшейся части
    int rho = getRho(hash);
    
    // Обновляем регистр, если нашли больше ведущих нулей
    if (rho > registers[index]) {
        registers[index] = rho;
    }
}

double HyperLogLog::estimate() const {
    // Корректирующий коэффициент
    double alpha = getAlpha(p);
    
    // Гармоническое среднее: сумма 2^(-rho)
    double sum = 0.0;
    for (size_t value : registers) {
        sum += std::pow(2.0, -static_cast<double>(value));
    }
    
    // Основная оценка
    double estimate = alpha * m * m / sum;
    
    // Корректировка для малых диапазонов (много нулевых регистров)
    if (estimate <= 2.5 * m) {
        size_t zeroCount = std::count(registers.begin(), registers.end(), 0);
        if (zeroCount != 0) {
            // Линейная подсчет для малых кардинальностей
            estimate = m * std::log(static_cast<double>(m) / zeroCount);
        }
    }
    
    return estimate;
}

void HyperLogLog::clear() {
    // Сбрасываем все регистры в 0
    std::fill(registers.begin(), registers.end(), 0);
}

int HyperLogLog::getRho(size_t hash) const {
    int rho = 1;
    
    // Считаем количество ведущих нулей
    while ((hash & 1) == 0) {
        rho++;
        hash >>= 1;
    }
    
    return rho;
}

double HyperLogLog::getAlpha(int p) const {
    // Предрасчитанные значения для малых p
    if (p == 4) return 0.673;
    if (p == 5) return 0.697;
    if (p == 6) return 0.709;
    
    // Общая формула для p >= 7
    return 0.7213 / (1.0 + 1.079 / m);
}