// Лабораторная работа №4. Библиотека классов для паттерна Фасад.
// Вариант 2: расчёт ежедневной нормы потребления килокалорий.

#pragma once

#include <string>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <map>
#include <cmath>

namespace Calories {

// ======================= Входные данные =======================

enum class Sex { Male, Female };
enum class Activity { Low, Medium, High };
enum class BodyKind { Asthenic, Normosthenic, Hypersthenic };

struct PersonData {
    double height;      // рост, см
    double weight;      // вес, кг
    int age;            // возраст, лет
    Sex sex;
    Activity activity;  // группа физической активности
};

// ======================= Классы подсистемы =======================

// Тип телосложения. Влияет на норму калорий и на идеальный вес.
class BodyType {
public:
    virtual ~BodyType() {}
    virtual std::string getName() const = 0;
    virtual double getCalorieFactor() const = 0;
    virtual double getWeightFactor() const = 0;
};

// Астеник: быстрый обмен веществ, легкий костяк
class Asthenic : public BodyType {
public:
    std::string getName() const override { return "астеник"; }
    double getCalorieFactor() const override { return 1.1; }
    double getWeightFactor() const override { return 0.9; }
};

class Normosthenic : public BodyType {
public:
    std::string getName() const override { return "нормостеник"; }
    double getCalorieFactor() const override { return 1.0; }
    double getWeightFactor() const override { return 1.0; }
};

// Гиперстеник: медленный обмен веществ, широкий костяк
class Hypersthenic : public BodyType {
public:
    std::string getName() const override { return "гиперстеник"; }
    double getCalorieFactor() const override { return 0.9; }
    double getWeightFactor() const override { return 1.1; }
};

// Проверка входных данных
class PersonValidator {
public:
    void validate(const PersonData& p) const {
        if (p.height < 50 || p.height > 250) throw std::invalid_argument("Рост должен быть от 50 до 250 см");
        if (p.weight < 20 || p.weight > 300) throw std::invalid_argument("Вес должен быть от 20 до 300 кг");
        if (p.age < 14 || p.age > 100) throw std::invalid_argument("Возраст должен быть от 14 до 100 лет");
    }
};

// Основной обмен по формуле Миффлина - Сан Жеора, ккал
class BasalMetabolism {
public:
    double calculate(const PersonData& p) const {
        double base = 10 * p.weight + 6.25 * p.height - 5 * p.age;
        return p.sex == Sex::Male ? base + 5 : base - 161;
    }
};

// Коэффициент физической активности
class ActivityFactor {
public:
    double getFactor(Activity a) const {
        switch (a) {
        case Activity::Low: return 1.2;
        case Activity::Medium: return 1.55;
        default: return 1.725;
        }
    }

    std::string getName(Activity a) const {
        switch (a) {
        case Activity::Low: return "низкая";
        case Activity::Medium: return "средняя";
        default: return "высокая";
        }
    }
};

// Идеальный вес по формуле Брока с поправкой на телосложение, кг
class IdealWeight {
public:
    double calculate(double height, const BodyType& body) const {
        double base;
        if (height < 165) base = height - 100;
        else if (height <= 175) base = height - 105;
        else base = height - 110;
        return base * body.getWeightFactor();
    }
};

// ======================= Фасад =======================

// Единая точка входа: клиент передает данные человека и тип телосложения,
// а фасад сам обращается к классам подсистемы.
class CalorieFacade {
public:
    CalorieFacade() {
        bodyTypes[BodyKind::Asthenic] = std::unique_ptr<BodyType>(new Asthenic());
        bodyTypes[BodyKind::Normosthenic] = std::unique_ptr<BodyType>(new Normosthenic());
        bodyTypes[BodyKind::Hypersthenic] = std::unique_ptr<BodyType>(new Hypersthenic());
    }

    // Суточная норма, ккал (округляется до целого)
    double dailyNorm(const PersonData& p, BodyKind kind) const {
        validator.validate(p);
        double norm = metabolism.calculate(p) * activity.getFactor(p.activity) *
                      bodyTypes.at(kind)->getCalorieFactor();
        return std::round(norm);
    }

    double idealWeight(const PersonData& p, BodyKind kind) const {
        validator.validate(p);
        return idealWeightCalc.calculate(p.height, *bodyTypes.at(kind));
    }

    std::string report(const PersonData& p, BodyKind kind) const {
        double norm = dailyNorm(p, kind);
        double ideal = idealWeight(p, kind);
        std::ostringstream out;
        out << (p.sex == Sex::Male ? "Мужчина" : "Женщина") << ", " << p.age << " лет, "
            << p.height << " см, " << p.weight << " кг, "
            << bodyTypes.at(kind)->getName() << ", активность " << activity.getName(p.activity) << "\n"
            << "  Основной обмен: " << std::round(metabolism.calculate(p)) << " ккал\n"
            << "  Суточная норма: " << norm << " ккал\n"
            << "  Идеальный вес: " << ideal << " кг\n"
            << "  Рекомендация: " << recommendation(p.weight, ideal, norm);
        return out.str();
    }

private:
    // Ссылки на классы подсистемы
    PersonValidator validator;
    BasalMetabolism metabolism;
    ActivityFactor activity;
    IdealWeight idealWeightCalc;
    std::map<BodyKind, std::unique_ptr<BodyType>> bodyTypes;

    std::string recommendation(double weight, double ideal, double norm) const {
        std::ostringstream out;
        if (weight > ideal * 1.1) {
            out << "для снижения веса около " << std::round(norm * 0.85) << " ккал";
        } else if (weight < ideal * 0.9) {
            out << "для набора веса около " << std::round(norm * 1.15) << " ккал";
        } else {
            out << "вес в норме, придерживайтесь " << norm << " ккал";
        }
        return out.str();
    }
};

}
