#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    QString server() const;
    QString username() const;
    QString password() const;

    void setServer(const QString &s);
    void setUsername(const QString &u);
    void setPassword(const QString &p);

private:
    QLineEdit *m_serverEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
};

#endif // SETTINGSDIALOG_H
