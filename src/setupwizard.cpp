#include "setupwizard.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QFile>

SetupWizard::SetupWizard(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("ChirikSIP Setup");
    setFixedSize(400, 320);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_stack = new QStackedWidget(this);
    setupPages();
    mainLayout->addWidget(m_stack);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_backBtn = new QPushButton("Back", this);
    m_nextBtn = new QPushButton("Next", this);
    m_backBtn->setEnabled(false);
    btnLayout->addWidget(m_backBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_nextBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_nextBtn, &QPushButton::clicked, this, &SetupWizard::onNext);
    connect(m_backBtn, &QPushButton::clicked, this, &SetupWizard::onBack);
}

bool SetupWizard::isFirstRun()
{
    return !QFile::exists(QSettings().fileName());
}

void SetupWizard::setupPages()
{
    // Page 0: Welcome
    QWidget *welcome = new QWidget(this);
    QVBoxLayout *wLayout = new QVBoxLayout(welcome);
    QLabel *title = new QLabel("<h2>Welcome to ChirikSIP</h2>", welcome);
    title->setAlignment(Qt::AlignCenter);
    QLabel *desc = new QLabel(
        "This wizard will help you configure your SIP account.\n\n"
        "You will need:\n"
        "- SIP server address\n"
        "- Username\n"
        "- Password", welcome);
    desc->setAlignment(Qt::AlignLeft);
    desc->setStyleSheet("font-size: 13px; padding: 10px;");
    wLayout->addSpacing(20);
    wLayout->addWidget(title);
    wLayout->addWidget(desc);
    wLayout->addStretch();
    m_stack->addWidget(welcome);

    // Page 1: Server
    QWidget *serverPage = new QWidget(this);
    QFormLayout *sLayout = new QFormLayout(serverPage);
    QLabel *sTitle = new QLabel("<h3>SIP Server</h3>", serverPage);
    sLayout->addRow(sTitle);
    m_serverEdit = new QLineEdit(serverPage);
    m_serverEdit->setPlaceholderText("sip.example.com");
    sLayout->addRow("Server:", m_serverEdit);
    sLayout->addRow("", new QLabel("Enter your SIP server address.\nExample: voip.provider.com", serverPage));
    m_stack->addWidget(serverPage);

    // Page 2: Credentials
    QWidget *credPage = new QWidget(this);
    QFormLayout *cLayout = new QFormLayout(credPage);
    QLabel *cTitle = new QLabel("<h3>Account</h3>", credPage);
    cLayout->addRow(cTitle);
    m_usernameEdit = new QLineEdit(credPage);
    m_usernameEdit->setPlaceholderText("username");
    cLayout->addRow("User:", m_usernameEdit);
    m_passwordEdit = new QLineEdit(credPage);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("password");
    cLayout->addRow("Pass:", m_passwordEdit);
    m_stack->addWidget(credPage);

    // Page 3: Finish
    QWidget *finish = new QWidget(this);
    QVBoxLayout *fLayout = new QVBoxLayout(finish);
    QLabel *fTitle = new QLabel("<h2>Setup Complete</h2>", finish);
    fTitle->setAlignment(Qt::AlignCenter);
    QLabel *fDesc = new QLabel(
        "Your SIP account is configured.\n\n"
        "Press Finish to start using ChirikSIP.", finish);
    fDesc->setAlignment(Qt::AlignCenter);
    fDesc->setStyleSheet("font-size: 13px; padding: 10px;");
    fLayout->addSpacing(20);
    fLayout->addWidget(fTitle);
    fLayout->addWidget(fDesc);
    fLayout->addStretch();
    m_stack->addWidget(finish);

    m_pageCount = m_stack->count();
}

void SetupWizard::onNext()
{
    if (m_currentPage == 1 && m_serverEdit->text().trimmed().isEmpty()) {
        m_serverEdit->setFocus();
        return;
    }
    if (m_currentPage == 2 && m_usernameEdit->text().trimmed().isEmpty()) {
        m_usernameEdit->setFocus();
        return;
    }

    if (m_currentPage == m_pageCount - 2) {
        accept();
        return;
    }

    m_currentPage++;
    m_stack->setCurrentIndex(m_currentPage);
    m_backBtn->setEnabled(m_currentPage > 0);
}

void SetupWizard::onBack()
{
    if (m_currentPage > 0) {
        m_currentPage--;
        m_stack->setCurrentIndex(m_currentPage);
        m_backBtn->setEnabled(m_currentPage > 0);
        m_nextBtn->setText("Next");
    }
}

QString SetupWizard::server() const { return m_serverEdit->text().trimmed(); }
QString SetupWizard::username() const { return m_usernameEdit->text().trimmed(); }
QString SetupWizard::password() const { return m_passwordEdit->text(); }
