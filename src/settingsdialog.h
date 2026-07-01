#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
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
    int port() const;

    void setServer(const QString &s);
    void setUsername(const QString &u);
    void setPassword(const QString &p);
    void setPort(int p);

private:
    QLineEdit *m_serverEdit;
    QLineEdit *m_usernameEdit;
    QLineEdit *m_passwordEdit;
    QSpinBox *m_portEdit;
};

#endif // SETTINGSDIALOG_H
