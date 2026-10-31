// Лабораторная работа №4. Структурные паттерны Адаптер и Фасад.
// Консольное приложение - клиент библиотек 4_GasAdapter.h и 4_CalorieFacade.h.
// Адаптер: вариант 1 (адаптер объектов, Баллон с газом).
// Фасад: вариант 2 (ежедневная норма килокалорий).

#include <iostream>
#include <string>
#include <cmath>
#include <clocale>
#include "4_GasAdapter.h"
#include "4_CalorieFacade.h"

using namespace std;
using namespace Gas;
using namespace Calories;

// ======================= Клиент адаптера =======================

// Клиент знает только целевой интерфейс ICylinder
void useCylinder(ICylinder& cylinder) {
    cout << cylinder.getData() << endl;
    cout << "Изменение давления при нагреве с 300 К на 50 К: "
         << cylinder.calculateDp(300, 50) << " Па" << endl;
    cylinder.modifMass(0.5);
    cout << "После добавления 0.5 кг газа:" << endl;
    cout << cylinder.getData() << endl;
    cout << "Изменение давления при нагреве с 300 К на 50 К: "
         << cylinder.calculateDp(300, 50) << " Па" << endl;
}

// ======================= Тестирование =======================

class Tester {
public:
    Tester() : passed(0), failed(0) {}

    void check(bool condition, const string& name) {
        if (condition) {
            passed++;
            cout << "  [OK]   " << name << endl;
        } else {
            failed++;
            cout << "  [FAIL] " << name << endl;
        }
    }

    void printResult() const {
        cout << "Пройдено: " << passed << ", провалено: " << failed << endl;
    }

private:
    int passed;
    int failed;
};

bool equal(double a, double b) { return fabs(a - b) < 1e-6; }

bool contains(const string& text, const string& part) {
    return text.find(part) != string::npos;
}

void testCylinder(Tester& t) {
    // 1 кг кислорода в баллоне 0.05 м3
    GasCylinder cylinder(0.05, 1, 0.032);
    t.check(equal(cylinder.amountOfMatter(), 31.25), "Количество вещества: 1 / 0.032 = 31.25 моль");
    t.check(equal(cylinder.getPressure(300), 1558125), "Давление при 300 К = 1558125 Па");

    bool thrown = false;
    try { GasCylinder bad(0, 1, 0.032); } catch (const invalid_argument&) { thrown = true; }
    t.check(thrown, "Баллон с нулевым объёмом не создается");
}

void testAdapter(Tester& t) {
    GasCylinder cylinder(0.05, 1, 0.032);
    CylinderAdapter adapter(cylinder);
    ICylinder& target = adapter;

    t.check(equal(target.calculateDp(300, 10), 51937.5), "calculateDp(300, 10) = 51937.5 Па");
    t.check(equal(target.calculateDp(300, 0), 0), "Без изменения температуры давление не меняется");
    t.check(target.calculateDp(300, -10) < 0, "При охлаждении давление падает");

    target.modifMass(1);
    t.check(equal(cylinder.getMass(), 2), "modifMass(+1) меняет массу адаптируемого баллона");
    target.modifMass(-0.5);
    t.check(equal(cylinder.getMass(), 1.5), "modifMass(-0.5) уменьшает массу");

    bool thrown = false;
    try { target.modifMass(-10); } catch (const invalid_argument&) { thrown = true; }
    t.check(thrown && equal(cylinder.getMass(), 1.5), "Нельзя сделать массу отрицательной");

    t.check(contains(target.getData(), "Баллон с газом") && contains(target.getData(), "моль"),
            "getData() возвращает данные баллона");
}

void testFacade(Tester& t) {
    CalorieFacade facade;
    PersonData man = { 180, 80, 30, Sex::Male, Activity::Low };
    PersonData woman = { 180, 80, 30, Sex::Female, Activity::Low };

    // Основной обмен мужчины: 10*80 + 6.25*180 - 5*30 + 5 = 1780
    t.check(equal(facade.dailyNorm(man, BodyKind::Normosthenic), 2136), "Норма нормостеника: 1780 * 1.2 = 2136");
    // Женщины: 1780 - 166 = 1614, 1614 * 1.2 = 1936.8
    t.check(equal(facade.dailyNorm(woman, BodyKind::Normosthenic), 1937), "Норма женщины меньше: 1937");

    t.check(facade.dailyNorm(man, BodyKind::Asthenic) > facade.dailyNorm(man, BodyKind::Normosthenic) &&
            facade.dailyNorm(man, BodyKind::Normosthenic) > facade.dailyNorm(man, BodyKind::Hypersthenic),
            "Астенику нужно больше калорий, гиперстенику меньше");

    PersonData active = man;
    active.activity = Activity::High;
    t.check(equal(facade.dailyNorm(active, BodyKind::Normosthenic), 3071), "Высокая активность: 1780 * 1.725 = 3071");

    t.check(equal(facade.idealWeight(man, BodyKind::Normosthenic), 70), "Идеальный вес при росте 180: 70 кг");
    t.check(equal(facade.idealWeight(man, BodyKind::Asthenic), 63), "Идеальный вес астеника: 63 кг");

    PersonData wrong = { 180, 80, 5, Sex::Male, Activity::Low };
    bool thrown = false;
    try { facade.dailyNorm(wrong, BodyKind::Normosthenic); } catch (const invalid_argument&) { thrown = true; }
    t.check(thrown, "Неверные данные отклоняются");

    t.check(contains(facade.report(man, BodyKind::Hypersthenic), "гиперстеник"), "Отчет фасада");
}

void runTests() {
    Tester t;
    cout << "\n===== Тестирование =====" << endl;
    testCylinder(t);
    testAdapter(t);
    testFacade(t);
    t.printResult();
}

// ======================= Демонстрация =======================

int main() {
    setlocale(LC_ALL, "Russian");

    cout << "===== Паттерн Адаптер =====" << endl;
    GasCylinder oxygen(0.05, 1, 0.032);   // кислород, M = 32 г/моль
    CylinderAdapter adapter(oxygen);
    useCylinder(adapter);

    cout << "\n===== Паттерн Фасад =====" << endl;
    CalorieFacade facade;
    PersonData anna = { 165, 52, 20, Sex::Female, Activity::Medium };
    PersonData ivan = { 182, 95, 35, Sex::Male, Activity::Low };
    PersonData oleg = { 175, 68, 25, Sex::Male, Activity::High };

    cout << facade.report(anna, BodyKind::Asthenic) << endl;
    cout << facade.report(ivan, BodyKind::Hypersthenic) << endl;
    cout << facade.report(oleg, BodyKind::Normosthenic) << endl;

    try {
        PersonData wrong = { 300, 70, 20, Sex::Male, Activity::Low };
        facade.dailyNorm(wrong, BodyKind::Normosthenic);
    } catch (const invalid_argument& e) {
        cout << "Ошибка: " << e.what() << endl;
    }

    runTests();
    return 0;
}
