#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <QObject>
#include <QSqlDatabase>
#include <QJsonArray>
#include <QJsonObject>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    Q_INVOKABLE void initDatabase();

    // Quiz functions
    Q_INVOKABLE QJsonArray getQuizzes();
    Q_INVOKABLE QJsonObject getQuiz(int quizId);
    //Q_INVOKABLE void addQuiz(const QString &title, const QString &description);
    Q_INVOKABLE void addQuestion(int quizId, const QString &text, const QString &type, int points);
    Q_INVOKABLE void addChoice(int questionId, const QString &text, bool isCorrect);
    Q_INVOKABLE bool addUser(const QString &username,
                             const QString &displayName,
                             const QString &password,
                             const QString &role);
    Q_INVOKABLE bool checkUser(const QString &username,
                               const QString &password);
    // Attempt / grading
    Q_INVOKABLE double submitAttempt(int quizId, const QJsonArray &answers, int userId);
    Q_INVOKABLE QString getUserRole(const QString &username);
    Q_INVOKABLE int addQuiz(const QString &title, const QString &description);
private:
    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H
