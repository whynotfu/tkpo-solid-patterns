// Лабораторная работа №5. Библиотека классов для паттерна Состояние.
// Вариант 2: Банкомат.
// Состояния: Ожидание, Аутентификация пользователя, Выполнение операций,
// Заблокирован (денег нет).

#pragma once

#include <string>
#include <map>
#include <random>

namespace AtmSystem {

// ======================= Банк =======================

// Счет клиента банка
struct Account {
    std::string owner;
    int balance;
};

// Банк хранит счета; ключ - PIN-код карты (упрощение для лабораторной)
class Bank {
public:
    void addAccount(int pin, const std::string& owner, int balance) {
        accounts[pin] = Account{ owner, balance };
    }

    Account* findAccount(int pin) {
        auto it = accounts.find(pin);
        return it == accounts.end() ? nullptr : &it->second;
    }

private:
    std::map<int, Account> accounts;
};

class ATM;

// ======================= Интерфейс Состояние =======================

// Каждая операция банкомата возвращает сообщение для пользователя
class IAtmState {
public:
    virtual ~IAtmState() {}
    virtual std::string getName() const = 0;
    virtual std::string enterPin(ATM& atm, int pin) = 0;
    virtual std::string withdraw(ATM& atm, int amount) = 0;
    virtual std::string finish(ATM& atm) = 0;
    virtual std::string loadMoney(ATM& atm, int amount) = 0;
};

// ======================= Конкретные состояния =======================

class IdleState : public IAtmState {
public:
    std::string getName() const override { return "Ожидание"; }
    std::string enterPin(ATM& atm, int pin) override;
    std::string withdraw(ATM& atm, int amount) override;
    std::string finish(ATM& atm) override;
    std::string loadMoney(ATM& atm, int amount) override;
};

class AuthenticationState : public IAtmState {
public:
    std::string getName() const override { return "Аутентификация пользователя"; }
    std::string enterPin(ATM& atm, int pin) override;
    std::string withdraw(ATM& atm, int amount) override;
    std::string finish(ATM& atm) override;
    std::string loadMoney(ATM& atm, int amount) override;
};

class OperationState : public IAtmState {
public:
    std::string getName() const override { return "Выполнение операций"; }
    std::string enterPin(ATM& atm, int pin) override;
    std::string withdraw(ATM& atm, int amount) override;
    std::string finish(ATM& atm) override;
    std::string loadMoney(ATM& atm, int amount) override;
};

class BlockedState : public IAtmState {
public:
    std::string getName() const override { return "Заблокирован"; }
    std::string enterPin(ATM& atm, int pin) override;
    std::string withdraw(ATM& atm, int amount) override;
    std::string finish(ATM& atm) override;
    std::string loadMoney(ATM& atm, int amount) override;
};

// ======================= Конечный автомат =======================

// Банкомат хранит текущее состояние и передает ему все операции.
// Объекты состояний создаются один раз и переключаются указателем.
class ATM {
public:
    static const int MAX_ATTEMPTS = 3;

    ATM(const std::string& id, int cash, double noConnectionChance, Bank& bank, unsigned int seed)
        : id(id), cash(cash), noConnectionChance(noConnectionChance), bank(bank),
          random(seed), account(nullptr), attempts(0) {
        state = cash > 0 ? (IAtmState*)&idle : (IAtmState*)&blocked;
    }

    // Операции, которые вызывает пользователь
    std::string enterPin(int pin) { return state->enterPin(*this, pin); }
    std::string withdraw(int amount) { return state->withdraw(*this, amount); }
    std::string finish() { return state->finish(*this); }
    std::string loadMoney(int amount) { return state->loadMoney(*this, amount); }

    std::string getId() const { return id; }
    int getCash() const { return cash; }
    std::string getStateName() const { return state->getName(); }

    // Методы ниже используют классы состояний
    void setState(IAtmState* newState) { state = newState; }
    IAtmState* getIdleState() { return &idle; }
    IAtmState* getAuthenticationState() { return &authentication; }
    IAtmState* getOperationState() { return &operation; }
    IAtmState* getBlockedState() { return &blocked; }

    void addCash(int amount) { cash += amount; }
    void takeCash(int amount) { cash -= amount; }

    Bank& getBank() { return bank; }
    Account* getAccount() { return account; }
    void setAccount(Account* value) { account = value; }

    int getAttempts() const { return attempts; }
    void setAttempts(int value) { attempts = value; }

    // Есть ли связь с банком (случайное событие)
    bool isBankAvailable() {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(random) >= noConnectionChance;
    }

    // Завершение сеанса: сброс данных клиента и переход в нужное состояние
    void endSession() {
        account = nullptr;
        attempts = 0;
        state = cash > 0 ? (IAtmState*)&idle : (IAtmState*)&blocked;
    }

private:
    std::string id;
    int cash;
    double noConnectionChance;
    Bank& bank;
    std::mt19937 random;
    Account* account;
    int attempts;

    IdleState idle;
    AuthenticationState authentication;
    OperationState operation;
    BlockedState blocked;
    IAtmState* state;
};

// ======================= Реализация состояний =======================

// --- Ожидание ---

inline std::string IdleState::enterPin(ATM& atm, int pin) {
    atm.setState(atm.getAuthenticationState());
    return atm.enterPin(pin);
}

inline std::string IdleState::withdraw(ATM&, int) {
    return "Сначала введите PIN-код";
}

inline std::string IdleState::finish(ATM&) {
    return "Банкомат уже в режиме ожидания";
}

inline std::string IdleState::loadMoney(ATM& atm, int amount) {
    if (amount <= 0) return "Сумма загрузки должна быть больше нуля";
    atm.addCash(amount);
    return "Загружено " + std::to_string(amount) + " руб.";
}

// --- Аутентификация пользователя ---

inline std::string AuthenticationState::enterPin(ATM& atm, int pin) {
    if (!atm.isBankAvailable()) {
        atm.endSession();
        return "Нет связи с банком, попробуйте позже";
    }
    Account* account = atm.getBank().findAccount(pin);
    if (account != nullptr) {
        atm.setAccount(account);
        atm.setState(atm.getOperationState());
        return "PIN-код верный. Здравствуйте, " + account->owner;
    }
    atm.setAttempts(atm.getAttempts() + 1);
    if (atm.getAttempts() >= ATM::MAX_ATTEMPTS) {
        atm.endSession();
        return "Неверный PIN-код. Попытки исчерпаны, карта изъята";
    }
    return "Неверный PIN-код. Осталось попыток: " +
           std::to_string(ATM::MAX_ATTEMPTS - atm.getAttempts());
}

inline std::string AuthenticationState::withdraw(ATM&, int) {
    return "Сначала введите верный PIN-код";
}

inline std::string AuthenticationState::finish(ATM& atm) {
    atm.endSession();
    return "Операция отменена, заберите карту";
}

inline std::string AuthenticationState::loadMoney(ATM&, int) {
    return "Банкомат занят клиентом, загрузка невозможна";
}

// --- Выполнение операций ---

inline std::string OperationState::enterPin(ATM&, int) {
    return "PIN-код уже введен";
}

inline std::string OperationState::withdraw(ATM& atm, int amount) {
    if (amount <= 0 || amount % 100 != 0) return "Сумма должна быть кратна 100 руб.";
    if (amount > atm.getCash()) return "В банкомате недостаточно денег";
    if (amount > atm.getAccount()->balance) return "Недостаточно средств на счете";
    if (!atm.isBankAvailable()) return "Нет связи с банком, операция не выполнена";

    atm.getAccount()->balance -= amount;
    atm.takeCash(amount);
    std::string message = "Выдано " + std::to_string(amount) + " руб. Остаток на счете: " +
                          std::to_string(atm.getAccount()->balance) + " руб.";
    if (atm.getCash() == 0) {
        atm.endSession();
        message += " Деньги в банкомате закончились, банкомат заблокирован";
    }
    return message;
}

inline std::string OperationState::finish(ATM& atm) {
    atm.endSession();
    return "Работа завершена, заберите карту";
}

inline std::string OperationState::loadMoney(ATM&, int) {
    return "Банкомат занят клиентом, загрузка невозможна";
}

// --- Заблокирован (денег нет) ---

inline std::string BlockedState::enterPin(ATM&, int) {
    return "Банкомат заблокирован: нет денег";
}

inline std::string BlockedState::withdraw(ATM&, int) {
    return "Банкомат заблокирован: нет денег";
}

inline std::string BlockedState::finish(ATM&) {
    return "Банкомат заблокирован: нет денег";
}

inline std::string BlockedState::loadMoney(ATM& atm, int amount) {
    if (amount <= 0) return "Сумма загрузки должна быть больше нуля";
    atm.addCash(amount);
    atm.setState(atm.getIdleState());
    return "Загружено " + std::to_string(amount) + " руб. Банкомат разблокирован";
}

}
