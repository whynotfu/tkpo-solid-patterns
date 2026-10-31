// Лабораторная работа №4. Библиотека классов для паттерна Адаптер.
// Вариант 1: адаптер объектов. Адаптируемый класс - Баллон с газом.

#pragma once

#include <string>
#include <sstream>
#include <stdexcept>

namespace Gas {

// Молярная газовая постоянная, Дж/(моль*К)
const double R = 8.31;

// ======================= Адаптируемый класс =======================

// Баллон с газом. Атрибуты оформлены как свойства (get/set),
// аналог автоматических свойств C#.
// Уравнение Менделеева-Клапейрона: pV = (m / M) * R * T
class GasCylinder {
public:
    GasCylinder(double volume, double mass, double molar) {
        setVolume(volume);
        setMass(mass);
        setMolar(molar);
    }

    // Объём баллона, м3
    double getVolume() const { return volume; }
    void setVolume(double value) {
        if (value <= 0) throw std::invalid_argument("Объём должен быть больше нуля");
        volume = value;
    }

    // Масса газа, кг
    double getMass() const { return mass; }
    void setMass(double value) {
        if (value < 0) throw std::invalid_argument("Масса не может быть отрицательной");
        mass = value;
    }

    // Молярная масса газа, кг/моль
    double getMolar() const { return molar; }
    void setMolar(double value) {
        if (value <= 0) throw std::invalid_argument("Молярная масса должна быть больше нуля");
        molar = value;
    }

    // Давление в баллоне при температуре T (К), Па
    double getPressure(int T) const {
        return amountOfMatter() * R * T / volume;
    }

    // Количество вещества, моль
    double amountOfMatter() const {
        return mass / molar;
    }

    std::string toString() const {
        std::ostringstream out;
        out << "Объём = " << volume << " м3, масса = " << mass
            << " кг, молярная масса = " << molar << " кг/моль";
        return out.str();
    }

private:
    double volume;
    double mass;
    double molar;
};

// ======================= Целевой интерфейс =======================

// Интерфейс, который ожидает клиент
class ICylinder {
public:
    virtual ~ICylinder() {}
    // Изменение давления при начальной температуре T0 и изменении температуры dT
    virtual double calculateDp(int T0, int dT) const = 0;
    // Изменить массу газа на величину dm
    virtual void modifMass(double dm) = 0;
    // Строка с данными об объекте
    virtual std::string getData() const = 0;
};

// ======================= Адаптер объектов =======================

// Адаптер хранит ссылку на адаптируемый объект (композиция)
// и переводит вызовы целевого интерфейса в вызовы GasCylinder.
class CylinderAdapter : public ICylinder {
public:
    CylinderAdapter(GasCylinder& cylinder) : cylinder(cylinder) {}

    double calculateDp(int T0, int dT) const override {
        return cylinder.getPressure(T0 + dT) - cylinder.getPressure(T0);
    }

    void modifMass(double dm) override {
        cylinder.setMass(cylinder.getMass() + dm);
    }

    std::string getData() const override {
        std::ostringstream out;
        out << "Баллон с газом: " << cylinder.toString()
            << ", количество вещества = " << cylinder.amountOfMatter() << " моль";
        return out.str();
    }

private:
    GasCylinder& cylinder;
};

}
