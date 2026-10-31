// Лабораторная работа №8. Паттерн Декоратор.
// Основа - ЛР7 (вариант 11, «Журнал учителя», паттерн Стратегия).
// Объекты классов-наследников (Зачет, Экзамен) декорируются дополнительными
// свойствами: домашнее задание, пересдача, дистанционный формат.

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <clocale>

using namespace std;

// ======================= Ввод данных =======================

string readLine(istream& in, ostream& out, const string& prompt) {
    out << prompt;
    string line;
    getline(in, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

// Число можно ввести и через точку, и через запятую: "4.5" или "4,5".
// Разбор не зависит от setlocale (иначе "4.5" в русской локали читается как 4).
double readNumber(istream& in, ostream& out, const string& prompt) {
    string line = readLine(in, out, prompt);
    for (char& c : line) {
        if (c == ',') c = '.';
    }
    istringstream parser(line);
    parser.imbue(locale::classic());
    double value = 0;
    if (!(parser >> value)) return 0;
    return value;
}

// ======================= Компонент (абстрактный) =======================

// Общий интерфейс и для записей журнала, и для декораторов
class LessonComponent {
public:
    virtual ~LessonComponent() {}
    virtual LessonComponent* clone() const = 0;
    virtual string getType() const = 0;
    virtual vector<pair<string, string>> getFields() const = 0;
    virtual void input(istream& in, ostream& out) = 0;

    // Вывод данных
    void print(ostream& out) const {
        out << getType();
        for (const auto& field : getFields()) {
            out << " | " << field.first << ": " << field.second;
        }
        out << endl;
    }

    // Значение поля по названию ("" - поля нет)
    string getField(const string& name) const {
        for (const auto& field : getFields()) {
            if (field.first == name) return field.second;
        }
        return "";
    }
};

// ======================= Конкретный компонент: Урок =======================

class Lesson : public LessonComponent {
public:
    Lesson() : date("01.09.2026"), topic("Без темы"), className("1А") {
        liveCount++;
    }

    Lesson(const string& date, const string& topic, const string& className)
        : date(date), topic(topic), className(className) {
        liveCount++;
    }

    Lesson(const Lesson& other)
        : date(other.date), topic(other.topic), className(other.className) {
        liveCount++;
    }

    ~Lesson() override {
        liveCount--;
    }

    LessonComponent* clone() const override { return new Lesson(*this); }

    string getType() const override { return "Урок"; }

    vector<pair<string, string>> getFields() const override {
        return { { "Дата", date }, { "Тема", topic }, { "Класс", className } };
    }

    void input(istream& in, ostream& out) override {
        date = readLine(in, out, "Дата: ");
        topic = readLine(in, out, "Тема: ");
        className = readLine(in, out, "Класс: ");
    }

    string getDate() const { return date; }
    string getTopic() const { return topic; }
    string getClassName() const { return className; }

    // Сколько объектов Урок сейчас существует (для проверки деструкторов)
    static int getLiveCount() { return liveCount; }

protected:
    string date;
    string topic;
    string className;

private:
    static int liveCount;
};

int Lesson::liveCount = 0;

// ======================= Наследники Урока =======================

class Credit : public Lesson {
public:
    Credit() : Lesson(), passedCount(0) {}

    Credit(const string& date, const string& topic, const string& className, int passedCount)
        : Lesson(date, topic, className), passedCount(passedCount) {}

    Credit(const Credit& other) : Lesson(other), passedCount(other.passedCount) {}

    ~Credit() override {}

    LessonComponent* clone() const override { return new Credit(*this); }

    string getType() const override { return "Зачет"; }

    vector<pair<string, string>> getFields() const override {
        vector<pair<string, string>> fields = Lesson::getFields();
        fields.push_back({ "Сдали", to_string(passedCount) + " чел." });
        return fields;
    }

    void input(istream& in, ostream& out) override {
        Lesson::input(in, out);
        passedCount = (int)readNumber(in, out, "Сколько учеников сдали: ");
    }

    int getPassedCount() const { return passedCount; }

private:
    int passedCount;
};

class Exam : public Lesson {
public:
    Exam() : Lesson(), form("письменный"), averageGrade(0) {}

    Exam(const string& date, const string& topic, const string& className,
         const string& form, double averageGrade)
        : Lesson(date, topic, className), form(form), averageGrade(averageGrade) {}

    Exam(const Exam& other) : Lesson(other), form(other.form), averageGrade(other.averageGrade) {}

    ~Exam() override {}

    LessonComponent* clone() const override { return new Exam(*this); }

    string getType() const override { return "Экзамен"; }

    vector<pair<string, string>> getFields() const override {
        vector<pair<string, string>> fields = Lesson::getFields();
        ostringstream grade;
        grade << fixed << setprecision(1) << averageGrade;
        fields.push_back({ "Форма", form });
        fields.push_back({ "Средний балл", grade.str() });
        return fields;
    }

    void input(istream& in, ostream& out) override {
        Lesson::input(in, out);
        form = readLine(in, out, "Форма (устный/письменный): ");
        averageGrade = readNumber(in, out, "Средний балл: ");
    }

    string getForm() const { return form; }
    double getAverageGrade() const { return averageGrade; }

private:
    string form;
    double averageGrade;
};

// ======================= Декоратор (абстрактный) =======================

// Хранит обернутый компонент и по умолчанию передает ему все вызовы.
// Декоратор владеет обернутым объектом и удаляет его в деструкторе.
class LessonDecorator : public LessonComponent {
public:
    LessonDecorator(LessonComponent* wrapped) : wrapped(wrapped) {}

    // Копирование: копируется вся цепочка обернутых объектов
    LessonDecorator(const LessonDecorator& other) : wrapped(other.wrapped->clone()) {}

    LessonDecorator& operator=(const LessonDecorator&) = delete;

    ~LessonDecorator() override { delete wrapped; }

    string getType() const override { return wrapped->getType(); }
    vector<pair<string, string>> getFields() const override { return wrapped->getFields(); }
    void input(istream& in, ostream& out) override { wrapped->input(in, out); }

protected:
    LessonComponent* wrapped;
};

// ======================= Конкретные декораторы =======================

// Домашнее задание к уроку
class HomeworkDecorator : public LessonDecorator {
public:
    HomeworkDecorator(LessonComponent* wrapped, const string& homework)
        : LessonDecorator(wrapped), homework(homework) {}

    HomeworkDecorator(const HomeworkDecorator& other)
        : LessonDecorator(other), homework(other.homework) {}

    LessonComponent* clone() const override { return new HomeworkDecorator(*this); }

    // Новое поле добавляется после полей обернутого объекта
    vector<pair<string, string>> getFields() const override {
        vector<pair<string, string>> fields = wrapped->getFields();
        fields.push_back({ "Домашнее задание", homework });
        return fields;
    }

    void input(istream& in, ostream& out) override {
        wrapped->input(in, out);
        homework = readLine(in, out, "Домашнее задание: ");
    }

private:
    string homework;
};

// Пересдача зачета или экзамена
class RetakeDecorator : public LessonDecorator {
public:
    RetakeDecorator(LessonComponent* wrapped, const string& retakeDate, int studentsCount)
        : LessonDecorator(wrapped), retakeDate(retakeDate), studentsCount(studentsCount) {}

    RetakeDecorator(const RetakeDecorator& other)
        : LessonDecorator(other), retakeDate(other.retakeDate), studentsCount(other.studentsCount) {}

    LessonComponent* clone() const override { return new RetakeDecorator(*this); }

    // Меняет и тип записи, и добавляет поля (повторная пересдача не дублирует пометку)
    string getType() const override {
        string type = wrapped->getType();
        if (type.find("(с пересдачей)") != string::npos) return type;
        return type + " (с пересдачей)";
    }

    vector<pair<string, string>> getFields() const override {
        vector<pair<string, string>> fields = wrapped->getFields();
        fields.push_back({ "Пересдача", retakeDate });
        fields.push_back({ "На пересдачу", to_string(studentsCount) + " чел." });
        return fields;
    }

    void input(istream& in, ostream& out) override {
        wrapped->input(in, out);
        retakeDate = readLine(in, out, "Дата пересдачи: ");
        studentsCount = (int)readNumber(in, out, "Сколько учеников на пересдачу: ");
    }

private:
    string retakeDate;
    int studentsCount;
};

// Дистанционное проведение
class OnlineDecorator : public LessonDecorator {
public:
    OnlineDecorator(LessonComponent* wrapped, const string& link = "")
        : LessonDecorator(wrapped), link(link) {}

    OnlineDecorator(const OnlineDecorator& other)
        : LessonDecorator(other), link(other.link) {}

    LessonComponent* clone() const override { return new OnlineDecorator(*this); }

    // Пометка добавляется перед типом обернутого объекта
    string getType() const override { return "[онлайн] " + wrapped->getType(); }

    vector<pair<string, string>> getFields() const override {
        vector<pair<string, string>> fields = wrapped->getFields();
        fields.push_back({ "Ссылка", link });
        return fields;
    }

    void input(istream& in, ostream& out) override {
        wrapped->input(in, out);
        link = readLine(in, out, "Ссылка на занятие: ");
    }

private:
    string link;
};

// ======================= Стратегия (интерфейс) =======================

class Journal;

class IFormatStrategy {
public:
    virtual ~IFormatStrategy() {}
    virtual string getName() const = 0;
    virtual string format(const Journal& journal) const = 0;
};

// ======================= Основной класс: Журнал =======================

class Journal {
public:
    Journal() : subject("Без предмета"), teacher("Не указан"), strategy(nullptr) {}

    Journal(const string& subject, const string& teacher)
        : subject(subject), teacher(teacher), strategy(nullptr) {}

    // Глубокая копия: каждая запись (вместе с декораторами) копируется через clone()
    Journal(const Journal& other)
        : subject(other.subject), teacher(other.teacher), strategy(other.strategy) {
        for (LessonComponent* lesson : other.lessons) {
            lessons.push_back(lesson->clone());
        }
    }

    Journal& operator=(Journal other) {
        swap(subject, other.subject);
        swap(teacher, other.teacher);
        swap(strategy, other.strategy);
        swap(lessons, other.lessons);
        return *this;
    }

    ~Journal() {
        for (LessonComponent* lesson : lessons) {
            delete lesson;
        }
    }

    // Журнал становится владельцем переданной записи
    void addLesson(LessonComponent* lesson) { lessons.push_back(lesson); }

    // Оборачивает запись декоратором прямо в журнале.
    // wrap получает старую запись и возвращает декоратор, который ею владеет.
    void decorate(int index, const function<LessonComponent*(LessonComponent*)>& wrap) {
        lessons[index] = wrap(lessons[index]);
    }

    int getCount() const { return (int)lessons.size(); }
    const LessonComponent& getLesson(int index) const { return *lessons[index]; }

    string getSubject() const { return subject; }
    string getTeacher() const { return teacher; }

    void input(istream& in, ostream& out) {
        subject = readLine(in, out, "Предмет: ");
        teacher = readLine(in, out, "Учитель: ");
    }

    void print(ostream& out) const {
        out << "Журнал: " << subject << ", учитель: " << teacher
            << ", записей: " << lessons.size() << endl;
        for (int i = 0; i < (int)lessons.size(); i++) {
            out << "  " << i + 1 << ") ";
            lessons[i]->print(out);
        }
    }

    void setFormat(const IFormatStrategy* newStrategy) { strategy = newStrategy; }
    const IFormatStrategy* getFormat() const { return strategy; }

    string show() const {
        if (strategy == nullptr) return "Формат вывода не выбран\n";
        return strategy->format(*this);
    }

private:
    string subject;
    string teacher;
    vector<LessonComponent*> lessons;   // записи журнала: уроки и декорированные уроки
    const IFormatStrategy* strategy;
};

// ======================= Конкретные стратегии (из ЛР7) =======================

class TextFormat : public IFormatStrategy {
public:
    string getName() const override { return "text"; }

    string format(const Journal& journal) const override {
        ostringstream out;
        out << "Журнал: " << journal.getSubject() << " (учитель: " << journal.getTeacher() << ")\n";
        for (int i = 0; i < journal.getCount(); i++) {
            const LessonComponent& lesson = journal.getLesson(i);
            out << i + 1 << ". " << lesson.getType() << ":";
            for (const auto& field : lesson.getFields()) {
                out << " " << field.first << " - " << field.second << ";";
            }
            out << "\n";
        }
        return out.str();
    }
};

class HtmlFormat : public IFormatStrategy {
public:
    string getName() const override { return "html"; }

    string format(const Journal& journal) const override {
        ostringstream out;
        out << "<h1>" << escape(journal.getSubject()) << "</h1>\n";
        out << "<h2>Учитель: " << escape(journal.getTeacher()) << "</h2>\n";
        for (int i = 0; i < journal.getCount(); i++) {
            const LessonComponent& lesson = journal.getLesson(i);
            out << "<p><b>" << escape(lesson.getType()) << "</b>";
            for (const auto& field : lesson.getFields()) {
                out << " " << escape(field.first) << ": " << escape(field.second) << ";";
            }
            out << "</p>\n";
        }
        return out.str();
    }

private:
    static string escape(const string& s) {
        string result;
        for (char c : s) {
            if (c == '<') result += "&lt;";
            else if (c == '>') result += "&gt;";
            else if (c == '&') result += "&amp;";
            else result += c;
        }
        return result;
    }
};

class JsonFormat : public IFormatStrategy {
public:
    string getName() const override { return "json"; }

    string format(const Journal& journal) const override {
        ostringstream out;
        out << "{\n";
        out << "  \"subject\": \"" << escape(journal.getSubject()) << "\",\n";
        out << "  \"teacher\": \"" << escape(journal.getTeacher()) << "\",\n";
        out << "  \"lessons\": [\n";
        for (int i = 0; i < journal.getCount(); i++) {
            const LessonComponent& lesson = journal.getLesson(i);
            out << "    { \"type\": \"" << escape(lesson.getType()) << "\"";
            for (const auto& field : lesson.getFields()) {
                out << ", \"" << escape(field.first) << "\": \"" << escape(field.second) << "\"";
            }
            out << " }" << (i + 1 < journal.getCount() ? "," : "") << "\n";
        }
        out << "  ]\n}\n";
        return out.str();
    }

private:
    static string escape(const string& s) {
        string result;
        for (char c : s) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else result += c;
        }
        return result;
    }
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

bool contains(const string& text, const string& part) {
    return text.find(part) != string::npos;
}

Journal makeJournal() {
    Journal journal("Математика", "Иванова М.П.");
    journal.addLesson(new HomeworkDecorator(new Lesson("02.09.2026", "Дроби", "5А"), "№ 15, 16"));
    journal.addLesson(new RetakeDecorator(new Credit("20.10.2026", "Уравнения", "5А", 24), "27.10.2026", 3));
    journal.addLesson(new OnlineDecorator(
        new RetakeDecorator(new Exam("25.12.2026", "Итоги полугодия", "5А", "письменный", 4.2), "15.01.2027", 2),
        "https://meet.school.ru/5a"));
    return journal;
}

void testComponentsUnchanged(Tester& t) {
    Exam exam("25.12.2026", "Итоги", "9Б", "устный", 3.8);
    t.check(exam.getType() == "Экзамен" && exam.getFields().size() == 5,
            "Исходный объект без декораторов не изменился");
}

void testSingleDecorator(Tester& t) {
    Credit* credit = new Credit("20.10.2026", "Уравнения", "5А", 24);
    RetakeDecorator retake(credit, "27.10.2026", 3);
    t.check(retake.getType() == "Зачет (с пересдачей)", "Декоратор меняет тип записи");
    t.check(retake.getField("Сдали") == "24 чел." && retake.getField("Пересдача") == "27.10.2026",
            "Декоратор сохраняет поля объекта и добавляет свои");
    t.check(credit->getField("Пересдача") == "", "Обернутый объект не получил чужих полей");
}

void testChain(Tester& t) {
    OnlineDecorator decorated(
        new HomeworkDecorator(
            new RetakeDecorator(new Exam("25.12.2026", "Итоги", "5А", "письменный", 4.2), "15.01.2027", 2),
            "Повторить главу 4"),
        "https://meet.school.ru/5a");

    t.check(decorated.getType() == "[онлайн] Экзамен (с пересдачей)", "Цепочка из трех декораторов: тип");
    t.check(decorated.getField("Средний балл") == "4.2" && decorated.getField("Пересдача") == "15.01.2027" &&
            decorated.getField("Домашнее задание") == "Повторить главу 4" &&
            decorated.getField("Ссылка") == "https://meet.school.ru/5a",
            "Цепочка из трех декораторов: все поля на месте");
}

void testInput(Tester& t) {
    ostringstream silent;
    istringstream in("10.11.2026\nТреугольники\n7В\n25\nhttps://meet.school.ru/7v\n");
    OnlineDecorator online(new Credit());
    online.input(in, silent);
    t.check(online.getField("Тема") == "Треугольники" && online.getField("Сдали") == "25 чел." &&
            online.getField("Ссылка") == "https://meet.school.ru/7v",
            "Ввод данных через декоратор заполняет и объект, и декоратор");
}

void testJournalAndMemory(Tester& t) {
    int before = Lesson::getLiveCount();
    {
        Journal journal = makeJournal();
        Journal copy(journal);
        copy.decorate(1, [](LessonComponent* lesson) { return new HomeworkDecorator(lesson, "Задачи 1-5"); });

        t.check(copy.getLesson(1).getField("Домашнее задание") == "Задачи 1-5" &&
                journal.getLesson(1).getField("Домашнее задание") == "",
                "Декорирование записи в копии не меняет оригинал");
        t.check(&copy.getLesson(2) != &journal.getLesson(2) &&
                copy.getLesson(2).getType() == journal.getLesson(2).getType(),
                "Копия журнала глубоко копирует цепочки декораторов");
    }
    t.check(Lesson::getLiveCount() == before, "Деструкторы декораторов удалили все уроки");
}

void testStrategiesWithDecorators(Tester& t) {
    Journal journal = makeJournal();
    HtmlFormat html;
    JsonFormat json;

    journal.setFormat(&html);
    t.check(contains(journal.show(), "<b>[онлайн] Экзамен (с пересдачей)</b>"),
            "Стратегия html выводит декорированные записи без изменений в коде");
    journal.setFormat(&json);
    t.check(contains(journal.show(), "\"Домашнее задание\": \"№ 15, 16\""),
            "Стратегия json выводит поля декораторов");
}

void runTests() {
    Tester t;
    cout << "\n===== Тестирование =====" << endl;
    testComponentsUnchanged(t);
    testSingleDecorator(t);
    testChain(t);
    testInput(t);
    testJournalAndMemory(t);
    testStrategiesWithDecorators(t);
    t.printResult();
}

// ======================= Меню =======================

void addLessonFromUser(Journal& journal) {
    string type = readLine(cin, cout, "Тип записи (1 - урок, 2 - зачет, 3 - экзамен): ");
    LessonComponent* lesson;
    if (type == "2") lesson = new Credit();
    else if (type == "3") lesson = new Exam();
    else lesson = new Lesson();
    lesson->input(cin, cout);
    journal.addLesson(lesson);
    cout << "Запись добавлена" << endl;
}

void decorateFromUser(Journal& journal) {
    journal.print(cout);
    int index = (int)readNumber(cin, cout, "Номер записи: ") - 1;
    if (index < 0 || index >= journal.getCount()) {
        cout << "Нет такой записи" << endl;
        return;
    }
    string kind = readLine(cin, cout, "Добавить (1 - домашнее задание, 2 - пересдачу, 3 - онлайн): ");
    if (kind == "1") {
        string homework = readLine(cin, cout, "Домашнее задание: ");
        journal.decorate(index, [&](LessonComponent* l) { return new HomeworkDecorator(l, homework); });
    } else if (kind == "2") {
        string date = readLine(cin, cout, "Дата пересдачи: ");
        int count = (int)readNumber(cin, cout, "Сколько учеников на пересдачу: ");
        journal.decorate(index, [&](LessonComponent* l) { return new RetakeDecorator(l, date, count); });
    } else if (kind == "3") {
        string link = readLine(cin, cout, "Ссылка на занятие: ");
        journal.decorate(index, [&](LessonComponent* l) { return new OnlineDecorator(l, link); });
    } else {
        cout << "Неизвестный декоратор" << endl;
        return;
    }
    cout << "Запись дополнена" << endl;
}

void runMenu(Journal& journal) {
    TextFormat text;
    HtmlFormat html;
    JsonFormat json;
    journal.setFormat(&text);

    while (true) {
        cout << "\nТекущий формат: " << journal.getFormat()->getName() << endl;
        cout << "1 - показать журнал, 2 - сменить формат, 3 - добавить запись, "
                "4 - дополнить запись (декоратор), 0 - выход" << endl;
        string choice = readLine(cin, cout, "> ");
        if (!cin || choice == "0") break;

        if (choice == "1") {
            cout << journal.show();
        } else if (choice == "2") {
            string f = readLine(cin, cout, "Формат (1 - text, 2 - html, 3 - json): ");
            if (f == "1") journal.setFormat(&text);
            else if (f == "2") journal.setFormat(&html);
            else if (f == "3") journal.setFormat(&json);
            else cout << "Неизвестный формат" << endl;
        } else if (choice == "3") {
            addLessonFromUser(journal);
        } else if (choice == "4") {
            decorateFromUser(journal);
        } else {
            cout << "Неизвестная команда" << endl;
        }
    }
    journal.setFormat(nullptr);
}

// ======================= Демонстрация =======================

int main() {
    setlocale(LC_ALL, "Russian");

    cout << "===== Демонстрация =====" << endl;

    // Один и тот же экзамен без декораторов и с ними
    Exam* exam = new Exam("25.12.2026", "Итоги полугодия", "5А", "письменный", 4.2);
    cout << "--- Экзамен без декораторов ---" << endl;
    exam->print(cout);

    LessonComponent* decorated = new OnlineDecorator(
        new RetakeDecorator(exam, "15.01.2027", 2), "https://meet.school.ru/5a");
    cout << "--- Экзамен + пересдача + онлайн ---" << endl;
    decorated->print(cout);
    delete decorated;   // удалит всю цепочку, включая сам экзамен

    // Журнал из ЛР7 с декорированными записями
    Journal journal = makeJournal();
    journal.addLesson(new Credit("15.11.2026", "Геометрия", "5А", 22));
    journal.decorate(3, [](LessonComponent* l) { return new HomeworkDecorator(l, "Выучить теоремы"); });

    cout << "\n--- Журнал (метод print) ---" << endl;
    journal.print(cout);

    HtmlFormat html;
    journal.setFormat(&html);
    cout << "\n--- Журнал в формате html ---" << endl;
    cout << journal.show();
    journal.setFormat(nullptr);

    runTests();

    cout << "\n===== Меню =====" << endl;
    runMenu(journal);
    return 0;
}
