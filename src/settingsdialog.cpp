#include "settingsdialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QApplication>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    setMinimumWidth(300);
    setWindowIcon(QApplication::windowIcon());

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();
    m_serverEdit = new QLineEdit(this);
    m_serverEdit->setPlaceholderText("sip.example.com");
    form->addRow("Server:", m_serverEdit);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("username");
    form->addRow("User:", m_usernameEdit);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("password");
    form->addRow("Pass:", m_passwordEdit);

    mainLayout->addLayout(form);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

QString SettingsDialog::server() const { return m_serverEdit->text().trimmed(); }
QString SettingsDialog::username() const { return m_usernameEdit->text().trimmed(); }
QString SettingsDialog::password() const { return m_passwordEdit->text(); }

void SettingsDialog::setServer(const QString &s) { m_serverEdit->setText(s); }
void SettingsDialog::setUsername(const QString &u) { m_usernameEdit->setText(u); }
void SettingsDialog::setPassword(const QString &p) { m_passwordEdit->setText(p); }
