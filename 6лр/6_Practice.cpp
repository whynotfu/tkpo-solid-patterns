// Лабораторная работа №6. Паттерн MVC.
// Основа - ЛР5 (Банкомат, паттерн Состояние), библиотека 5_AtmState.h.
// Model      - AtmModel: банкомат + оповещение наблюдателей (паттерн Observer)
// View       - ScreenView (экран клиента), OperatorView (панель оператора)
// Controller - AtmController: разбирает команды пользователя и меняет модель

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <clocale>
#include "5_AtmState.h"

using namespace std;
using namespace AtmSystem;

// ======================= Наблюдатель =======================

class Observer {
public:
    virtual ~Observer() {}
    virtual void update() = 0;
};

class Observable {
public:
    void addObserver(Observer* observer) {
        observers.push_back(observer);
    }

    void removeObserver(Observer* observer) {
        observers.erase(remove(observers.begin(), observers.end(), observer), observers.end());
    }

    void notifyUpdate() {
        for (Observer* observer : observers) {
            observer->update();
        }
    }

private:
    vector<Observer*> observers;
};

// ======================= Модель =======================

// Вся логика работы с данными: банк, банкомат и журнал операций.
// После каждого изменения модель оповещает представления.
class AtmModel : public Observable {
public:
    AtmModel(const string& id, int cash, double noConnectionChance, unsigned int seed)
        : atm(id, cash, noConnectionChance, bank, seed) {}

    void addAccount(int pin, const string& owner, int balance) {
        bank.addAccount(pin, owner, balance);
    }

    void enterPin(int pin) { apply("Ввод PIN", atm.enterPin(pin)); }
    void withdraw(int amount) { apply("Снятие " + to_string(amount), atm.withdraw(amount)); }
    void finish() { apply("Завершение", atm.finish()); }
    void loadMoney(int amount) { apply("Загрузка " + to_string(amount), atm.loadMoney(amount)); }

    string getId() const { return atm.getId(); }
    int getCash() const { return atm.getCash(); }
    string getStateName() const { return atm.getStateName(); }
    string getLastMessage() const { return lastMessage; }
    const vector<string>& getHistory() const { return history; }

    // Имя и баланс клиента, который сейчас обслуживается ("" - клиента нет)
    string getClientName() {
        Account* account = atm.getAccount();
        return account == nullptr ? "" : account->owner;
    }

    int getClientBalance() {
        Account* account = atm.getAccount();
        return account == nullptr ? 0 : account->balance;
    }

private:
    Bank bank;
    ATM atm;
    string lastMessage;
    vector<string> history;

    void apply(const string& operation, const string& result) {
        lastMessage = result;
        history.push_back(operation + ": " + result);
        notifyUpdate();
    }
};

// ======================= Представления =======================

// Экран банкомата, который видит клиент
class ScreenView : public Observer {
public:
    ScreenView(AtmModel& model, ostream& out) : model(model), out(out) {
        model.addObserver(this);
    }

    ~ScreenView() { model.removeObserver(this); }

    void update() override { print(); }

    void print() {
        out << "+------------- ЭКРАН " << model.getId() << " -------------+" << endl;
        out << "| " << model.getLastMessage() << endl;
        string client = model.getClientName();
        if (!client.empty()) {
            out << "| Клиент: " << client << ", на счете " << model.getClientBalance() << " руб." << endl;
        }
        out << "+-----------------------------------------+" << endl;
    }

private:
    AtmModel& model;
    ostream& out;
};

// Служебная панель оператора: состояние и наличные в банкомате
class OperatorView : public Observer {
public:
    static const int LOW_CASH = 3000;

    OperatorView(AtmModel& model, ostream& out) : model(model), out(out) {
        model.addObserver(this);
    }

    ~OperatorView() { model.removeObserver(this); }

    void update() override { print(); }

    void print() {
        out << "  [оператор] состояние: " << model.getStateName()
            << " | наличных: " << model.getCash() << " руб."
            << " | операций: " << model.getHistory().size();
        if (model.getCash() < LOW_CASH) out << " | ВНИМАНИЕ: мало денег";
        out << endl;
    }

private:
    AtmModel& model;
    ostream& out;
};

// ======================= Контроллер =======================

// Принимает команды пользователя и вызывает методы модели.
// Сам ничего не выводит о данных - это делают представления.
class AtmController {
public:
    AtmController(AtmModel& model) : model(model) {}

    // Команды: pin <код>, take <сумма>, end, load <сумма>
    // Возвращает false, если команда не распознана
    bool execute(const string& command) {
        istringstream in(command);
        string name;
        in >> name;
        int value = 0;

        if (name == "pin" && in >> value) {
            model.enterPin(value);
        } else if (name == "take" && in >> value) {
            model.withdraw(value);
        } else if (name == "end") {
            model.finish();
        } else if (name == "load" && in >> value) {
            model.loadMoney(value);
        } else {
            return false;
        }
        return true;
    }

    // Интерактивный режим: читает команды до "exit" или конца ввода
    void run(istream& in, ostream& out) {
        out << "Команды: pin <код>, take <сумма>, end, load <сумма>, exit" << endl;
        string line;
        while (out << "> " && getline(in, line)) {
            if (line == "exit") break;
            if (line.empty()) continue;
            if (!execute(line)) out << "Неизвестная команда" << endl;
        }
        out << endl;
    }

private:
    AtmModel& model;
};

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

// Наблюдатель для тестов: считает оповещения
class CountingObserver : public Observer {
public:
    CountingObserver() : count(0) {}
    void update() override { count++; }
    int count;
};

bool contains(const string& text, const string& part) {
    return text.find(part) != string::npos;
}

void testModel(Tester& t) {
    AtmModel model("T1", 5000, 0, 1);
    model.addAccount(1234, "Иванов", 10000);
    CountingObserver counter;
    model.addObserver(&counter);

    model.enterPin(1234);
    t.check(counter.count == 1, "Модель оповещает наблюдателя после изменения");
    t.check(model.getClientName() == "Иванов", "Модель хранит текущего клиента");
    model.withdraw(1000);
    t.check(model.getCash() == 4000 && model.getClientBalance() == 9000, "Модель выдает деньги");
    t.check(model.getHistory().size() == 2, "Модель ведет журнал операций");

    model.removeObserver(&counter);
    model.finish();
    t.check(counter.count == 2, "Отписанный наблюдатель не получает оповещений");
}

void testViews(Tester& t) {
    AtmModel model("T1", 2000, 0, 1);
    model.addAccount(1234, "Иванов", 10000);
    ostringstream screenOut, operatorOut;
    ScreenView screen(model, screenOut);
    OperatorView panel(model, operatorOut);

    model.enterPin(1234);
    t.check(contains(screenOut.str(), "PIN-код верный") && contains(screenOut.str(), "Иванов"),
            "Экран показывает сообщение и клиента");
    t.check(contains(operatorOut.str(), "Выполнение операций"),
            "Панель оператора показывает состояние");

    model.withdraw(1500);
    t.check(contains(operatorOut.str(), "ВНИМАНИЕ: мало денег"),
            "Панель оператора предупреждает о нехватке денег");
}

void testController(Tester& t) {
    AtmModel model("T1", 5000, 0, 1);
    model.addAccount(1234, "Иванов", 10000);
    AtmController controller(model);

    t.check(controller.execute("pin 1234") && model.getStateName() == "Выполнение операций",
            "Команда pin меняет состояние модели");
    t.check(controller.execute("take 2000") && model.getCash() == 3000, "Команда take снимает деньги");
    t.check(controller.execute("end") && model.getStateName() == "Ожидание", "Команда end завершает работу");
    t.check(controller.execute("load 1000") && model.getCash() == 4000, "Команда load загружает деньги");
    t.check(!controller.execute("fly 10") && !controller.execute("take"),
            "Неизвестные и неполные команды отклоняются");

    istringstream input("pin 1234\ntake 500\nexit\ntake 500\n");
    ostringstream output;
    controller.run(input, output);
    t.check(model.getCash() == 3500, "Интерактивный режим останавливается на exit");
}

void runTests() {
    Tester t;
    cout << "\n===== Тестирование =====" << endl;
    testModel(t);
    testViews(t);
    testController(t);
    t.printResult();
}

// ======================= Демонстрация =======================

int main() {
    setlocale(LC_ALL, "Russian");

    random_device device;
    AtmModel model("ATM-001", 5000, 0.1, device());
    model.addAccount(1234, "Иванов", 10000);
    model.addAccount(4321, "Петрова", 3000);

    ScreenView screen(model, cout);
    OperatorView panel(model, cout);
    AtmController controller(model);

    cout << "===== Демонстрация =====" << endl;
    vector<string> script = { "take 1000", "pin 1234", "take 3000", "end", "pin 4321", "take 2000", "load 20000" };
    for (const string& command : script) {
        cout << "\n> " << command << endl;
        controller.execute(command);
    }

    runTests();

    cout << "\n===== Интерактивный режим =====" << endl;
    cout << "PIN-коды: 1234 (Иванов), 4321 (Петрова)" << endl;
    controller.run(cin, cout);
    return 0;
}
