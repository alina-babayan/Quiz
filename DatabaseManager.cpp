#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("quizsystem.db");
    if (!m_db.open()) {
        qWarning() << "Cannot open database:" << m_db.lastError().text();
    }
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) m_db.close();
}


void DatabaseManager::initDatabase() {
    QSqlQuery query;

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Users (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            display_name TEXT NOT NULL,
            password TEXT NOT NULL,
            role TEXT NOT NULL CHECK (role IN ('admin','user')),
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Quizzes (
            quiz_id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT,
            created_by INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (created_by) REFERENCES Users(user_id) ON DELETE CASCADE
        )
    )");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Questions (
            question_id INTEGER PRIMARY KEY AUTOINCREMENT,
            quiz_id INTEGER NOT NULL,
            text TEXT NOT NULL,
            question_type TEXT DEFAULT 'multiple_choice',
            FOREIGN KEY (quiz_id) REFERENCES Quizzes(quiz_id) ON DELETE CASCADE
        )
    )");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS Options (
            option_id INTEGER PRIMARY KEY AUTOINCREMENT,
            question_id INTEGER NOT NULL,
            text TEXT NOT NULL,
            is_correct INTEGER NOT NULL CHECK (is_correct IN (0,1)),
            FOREIGN KEY (question_id) REFERENCES Questions(question_id) ON DELETE CASCADE
        )
    )");

    query.exec(R"(
        CREATE TABLE IF NOT EXISTS UserQuizzes (
            participation_id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            quiz_id INTEGER NOT NULL,
            score INTEGER CHECK(score >=0 AND score<=100),
            completed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
            FOREIGN KEY (quiz_id) REFERENCES Quizzes(quiz_id) ON DELETE CASCADE,
            UNIQUE(user_id, quiz_id)
        )
    )");
}

QJsonArray DatabaseManager::getQuizzes() {
    QJsonArray arr;
    QSqlQuery query("SELECT quiz_id, title, description FROM Quizzes ORDER BY created_at DESC");
    while (query.next()) {
        QJsonObject obj;
        obj["id"] = query.value(0).toInt();
        obj["title"] = query.value(1).toString();
        obj["description"] = query.value(2).toString();
        arr.append(obj);
    }
    qDebug() << "Loaded" << arr.size() << "quizzes";
    return arr;
}

QJsonObject DatabaseManager::getQuiz(int quizId) {
    QJsonObject quizObj;
    QSqlQuery query;
    query.prepare("SELECT quiz_id, title, description FROM Quizzes WHERE quiz_id = ?");
    query.addBindValue(quizId);
    if(query.exec() && query.next()) {
        quizObj["id"] = query.value(0).toInt();
        quizObj["title"] = query.value(1).toString();
        quizObj["description"] = query.value(2).toString();

        QSqlQuery q2;
        q2.prepare("SELECT question_id, text FROM Questions WHERE quiz_id = ?");
        q2.addBindValue(quizId);
        QJsonArray questions;
        if(q2.exec()) {
            while(q2.next()) {
                QJsonObject qobj;
                int qid = q2.value(0).toInt();
                qobj["id"] = qid;
                qobj["text"] = q2.value(1).toString();

                QSqlQuery q3;
                q3.prepare("SELECT option_id, text, is_correct FROM Options WHERE question_id = ?");
                q3.addBindValue(qid);
                QJsonArray choices;
                if(q3.exec()) {
                    while(q3.next()) {
                        QJsonObject cobj;
                        cobj["id"] = q3.value(0).toInt();
                        cobj["text"] = q3.value(1).toString();
                        cobj["is_correct"] = q3.value(2).toInt();
                        choices.append(cobj);
                    }
                }
                qobj["choices"] = choices;
                questions.append(qobj);
            }
        }
        quizObj["questions"] = questions;
    }
    return quizObj;
}


void DatabaseManager::addQuestion(int quizId, const QString &text, const QString &type, int points) {
    QSqlQuery query;
    query.prepare("INSERT INTO Questions (quiz_id, text, question_type) VALUES (?, ?, ?)");
    query.addBindValue(quizId);
    query.addBindValue(text);
    query.addBindValue(type);

    if (!query.exec()) {
        qDebug() << "Failed to add question:" << query.lastError().text();
    } else {
        qDebug() << "Question added with ID:" << query.lastInsertId().toInt();
    }
}

void DatabaseManager::addChoice(int questionId, const QString &text, bool isCorrect) {
    QSqlQuery query;
    query.prepare("INSERT INTO Options (question_id, text, is_correct) VALUES (?, ?, ?)");
    query.addBindValue(questionId);
    query.addBindValue(text);
    query.addBindValue(isCorrect ? 1 : 0);

    if (!query.exec()) {
        qDebug() << "Failed to add option:" << query.lastError().text();
    } else {
        qDebug() << "Option added with ID:" << query.lastInsertId().toInt();
    }
}

double DatabaseManager::submitAttempt(int quizId, const QJsonArray &answers, int userId) {
    QSqlQuery query;
    query.prepare("INSERT INTO Attempt (user_id, quiz_id, total_score) VALUES (?, ?, 0)");
    query.addBindValue(userId);
    query.addBindValue(quizId);
    query.exec();
    int attemptId = query.lastInsertId().toInt();

    double totalScore = 0;

    for(const QJsonValue &val : answers) {
        QJsonObject obj = val.toObject();
        int questionId = obj["questionId"].toInt();
        QJsonValue answerVal = obj["answer"];

        QSqlQuery q2;
        q2.prepare("SELECT type, points FROM Question WHERE question_id=?");
        q2.addBindValue(questionId);
        if(q2.exec() && q2.next()) {
            QString qtype = q2.value(0).toString();
            int points = q2.value(1).toInt();
            double awarded = 0;

            if(qtype == "single") {
                int choiceId = answerVal.toInt();
                QSqlQuery q3;
                q3.prepare("SELECT is_correct FROM Choice WHERE choice_id=?");
                q3.addBindValue(choiceId);
                if(q3.exec() && q3.next() && q3.value(0).toInt()==1) awarded = points;

                QSqlQuery qins;
                qins.prepare("INSERT INTO Answer (attempt_id, question_id, choice_id) VALUES (?, ?, ?)");
                qins.addBindValue(attemptId);
                qins.addBindValue(questionId);
                qins.addBindValue(choiceId);
                qins.exec();

            } else if(qtype == "multiple") {
                QJsonArray choiceIds = answerVal.toArray();
                QSqlQuery qcheck("SELECT choice_id, is_correct FROM Choice WHERE question_id="+QString::number(questionId));
                QMap<int,int> correctMap;
                while(qcheck.next()) correctMap[qcheck.value(0).toInt()] = qcheck.value(1).toInt();

                int correctCount=0;
                int totalCorrect=0;
                for(auto key : correctMap.keys()) if(correctMap[key]==1) totalCorrect++;
                for(auto ch : choiceIds){
                    int cid = ch.toInt();
                    if(correctMap.value(cid,0)==1) correctCount++;
                    else correctCount -= 0.5;
                    QSqlQuery qins;
                    qins.prepare("INSERT INTO Answer (attempt_id, question_id, choice_id) VALUES (?, ?, ?)");
                    qins.addBindValue(attemptId);
                    qins.addBindValue(questionId);
                    qins.addBindValue(cid);
                    qins.exec();
                }
                awarded = points * std::max(0.0, double(correctCount)/double(totalCorrect));

            } else if(qtype == "text") {
                QString textAnswer = answerVal.toString();
                QSqlQuery qins;
                qins.prepare("INSERT INTO Answer (attempt_id, question_id, text_answer) VALUES (?, ?, ?)");
                qins.addBindValue(attemptId);
                qins.addBindValue(questionId);
                qins.addBindValue(textAnswer);
                qins.exec();
                awarded = 0;
            }

            totalScore += awarded;
        }
    }

    QSqlQuery qupd;
    qupd.prepare("UPDATE Attempt SET total_score=? WHERE attempt_id=?");
    qupd.addBindValue(totalScore);
    qupd.addBindValue(attemptId);
    qupd.exec();

    return totalScore;
}
bool DatabaseManager::addUser(const QString &username,
                              const QString &displayName,
                              const QString &password,
                              const QString &role)
{
    QSqlQuery query;
    query.prepare("INSERT INTO Users (username, display_name, role, password) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(displayName);
    query.addBindValue(role);
    query.addBindValue(password);

    if(!query.exec()) {
        qWarning() << "Add user failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::checkUser(const QString &username,
                                const QString &password)
{
    QSqlQuery query;
    query.prepare("SELECT user_id FROM Users WHERE username=? AND password=?");
    query.addBindValue(username);
    query.addBindValue(password);

    if(query.exec() && query.next()) return true;
    return false;
}
QString DatabaseManager::getUserRole(const QString &username) {
    QSqlQuery query;
    query.prepare("SELECT role FROM Users WHERE username = ?");
    query.addBindValue(username);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}
int DatabaseManager::addQuiz(const QString &title, const QString &description) {
    QSqlQuery query;
    query.prepare("INSERT INTO Quizzes (title, description, created_by) VALUES (?, ?, 1)");
    query.addBindValue(title);
    query.addBindValue(description);

    if (query.exec()) {
        int quizId = query.lastInsertId().toInt();
        qDebug() << "Quiz created with ID:" << quizId;
        return quizId;
    }
    qDebug() << "Failed to create quiz:" << query.lastError().text();
    return -1;
}
