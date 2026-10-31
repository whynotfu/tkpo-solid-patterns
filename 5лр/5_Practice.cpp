// Лабораторная работа №5. Поведенческий паттерн Состояние.
// Вариант 2: Банкомат. Консольное приложение - клиент библиотеки 5_AtmState.h.

#include <iostream>
#include <string>
#include <random>
#include <clocale>
#include "5_AtmState.h"

using namespace std;
using namespace AtmSystem;

// Выполняет операцию и печатает ответ и новое состояние
void show(ATM& atm, const string& action, const string& answer) {
    cout << action << " -> " << answer << endl;
    cout << "   [состояние: " << atm.getStateName() << ", в банкомате "
         << atm.getCash() << " руб.]" << endl;
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

// Банк с одним счетом: PIN 1234, баланс 10000
void fillBank(Bank& bank) {
    bank.addAccount(1234, "Иванов", 10000);
}

void testInitialState(Tester& t) {
    Bank bank;
    ATM withMoney("A1", 5000, 0, bank, 1);
    ATM empty("A2", 0, 0, bank, 1);
    t.check(withMoney.getStateName() == "Ожидание", "Банкомат с деньгами начинает в Ожидании");
    t.check(empty.getStateName() == "Заблокирован", "Пустой банкомат начинает в состоянии Заблокирован");
}

void testIdle(Tester& t) {
    Bank bank;
    fillBank(bank);
    ATM atm("A1", 5000, 0, bank, 1);
    atm.withdraw(1000);
    t.check(atm.getStateName() == "Ожидание" && atm.getCash() == 5000, "В Ожидании снять деньги нельзя");
    atm.loadMoney(1000);
    t.check(atm.getStateName() == "Ожидание" && atm.getCash() == 6000, "В Ожидании можно загрузить деньги");
}

void testAuthentication(Tester& t) {
    Bank bank;
    fillBank(bank);
    ATM atm("A1", 5000, 0, bank, 1);

    atm.enterPin(1111);
    t.check(atm.getStateName() == "Аутентификация пользователя" && atm.getAttempts() == 1,
            "Неверный PIN: остаемся в Аутентификации");
    atm.enterPin(1234);
    t.check(atm.getStateName() == "Выполнение операций", "Верный PIN: переход к операциям");

    ATM atm2("A2", 5000, 0, bank, 1);
    atm2.enterPin(1);
    atm2.enterPin(2);
    atm2.enterPin(3);
    t.check(atm2.getStateName() == "Ожидание" && atm2.getAttempts() == 0,
            "Три неверных PIN: возврат в Ожидание");

    ATM atm3("A3", 5000, 0, bank, 1);
    atm3.enterPin(1111);
    atm3.finish();
    t.check(atm3.getStateName() == "Ожидание", "Отмена во время Аутентификации");
}

void testNoConnection(Tester& t) {
    Bank bank;
    fillBank(bank);
    // Вероятность отсутствия связи 100%
    ATM atm("A1", 5000, 1.0, bank, 1);
    string answer = atm.enterPin(1234);
    t.check(atm.getStateName() == "Ожидание" && answer.find("Нет связи") != string::npos,
            "Без связи с банком вход невозможен");
}

void testOperations(Tester& t) {
    Bank bank;
    fillBank(bank);
    ATM atm("A1", 5000, 0, bank, 1);
    atm.enterPin(1234);

    atm.withdraw(1000);
    t.check(atm.getCash() == 4000 && bank.findAccount(1234)->balance == 9000,
            "Снятие 1000 руб.: меньше и в банкомате, и на счете");
    atm.withdraw(150);
    t.check(atm.getCash() == 4000, "Сумма не кратная 100 не выдается");
    atm.withdraw(4500);
    t.check(atm.getCash() == 4000, "Нельзя снять больше, чем есть в банкомате");
    atm.loadMoney(1000);
    t.check(atm.getCash() == 4000, "Во время обслуживания клиента загрузка запрещена");
    atm.finish();
    t.check(atm.getStateName() == "Ожидание", "Завершение работы: возврат в Ожидание");
}

void testBalance(Tester& t) {
    Bank bank;
    bank.addAccount(1111, "Петров", 500);
    ATM atm("A1", 5000, 0, bank, 1);
    atm.enterPin(1111);
    atm.withdraw(1000);
    t.check(atm.getCash() == 5000 && bank.findAccount(1111)->balance == 500,
            "Нельзя снять больше, чем на счете");
}

void testBlocked(Tester& t) {
    Bank bank;
    fillBank(bank);
    ATM atm("A1", 2000, 0, bank, 1);
    atm.enterPin(1234);
    atm.withdraw(2000);
    t.check(atm.getStateName() == "Заблокирован" && atm.getCash() == 0,
            "Деньги закончились: банкомат заблокирован");

    atm.enterPin(1234);
    t.check(atm.getStateName() == "Заблокирован", "В заблокированном банкомате PIN не принимается");

    atm.loadMoney(10000);
    t.check(atm.getStateName() == "Ожидание" && atm.getCash() == 10000,
            "После загрузки денег банкомат разблокирован");
}

void runTests() {
    Tester t;
    cout << "\n===== Тестирование =====" << endl;
    testInitialState(t);
    testIdle(t);
    testAuthentication(t);
    testNoConnection(t);
    testOperations(t);
    testBalance(t);
    testBlocked(t);
    t.printResult();
}

// ======================= Демонстрация =======================

int main() {
    setlocale(LC_ALL, "Russian");

    Bank bank;
    bank.addAccount(1234, "Иванов", 10000);
    bank.addAccount(4321, "Петрова", 3000);

    // Вероятность отсутствия связи с банком - 10%
    random_device device;
    ATM atm("ATM-001", 5000, 0.1, bank, device());

    cout << "===== Демонстрация =====" << endl;
    cout << "Банкомат " << atm.getId() << ", состояние: " << atm.getStateName()
         << ", денег: " << atm.getCash() << " руб." << endl << endl;

    show(atm, "Снять 1000", atm.withdraw(1000));
    show(atm, "Ввести PIN 1111", atm.enterPin(1111));
    show(atm, "Ввести PIN 1234", atm.enterPin(1234));
    show(atm, "Снять 3000", atm.withdraw(3000));
    show(atm, "Завершить работу", atm.finish());

    show(atm, "Ввести PIN 4321", atm.enterPin(4321));
    show(atm, "Снять 2000", atm.withdraw(2000));
    show(atm, "Ввести PIN 4321", atm.enterPin(4321));
    show(atm, "Загрузить 20000", atm.loadMoney(20000));
    show(atm, "Ввести PIN 4321", atm.enterPin(4321));
    show(atm, "Снять 2000", atm.withdraw(2000));
    show(atm, "Завершить работу", atm.finish());

    runTests();
    return 0;
}
