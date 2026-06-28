#include "callnotification.h"
#include "sipclient.h"
#include "scrollhelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

CallNotification::CallNotification(SipClient *client, QWidget *parent)
    : QDialog(parent)
    , m_client(client)
{
    setWindowTitle("Incoming Call");
    setFixedSize(300, 180);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose, false);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QWidget *displayWidget = new QWidget(this);
    displayWidget->setStyleSheet("background: #1a1a2e; border: 1px solid #444; border-radius: 6px;");
    QVBoxLayout *displayLayout = new QVBoxLayout(displayWidget);
    displayLayout->setContentsMargins(10, 8, 10, 8);
    displayLayout->setSpacing(0);

    m_numberLabel = new QLabel(displayWidget);
    m_numberLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_numberLabel->setStyleSheet("color: #e0e0e0; font-size: 22px; font-weight: bold; background: transparent; font-family: 'Segment16A';");
    m_numberLabel->setMinimumHeight(36);
    displayLayout->addWidget(m_numberLabel);

    m_nameLabel = new QLabel(displayWidget);
    m_nameLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_nameLabel->setStyleSheet("color: #888; font-size: 12px; background: transparent; font-family: monospace;");
    m_nameLabel->setMinimumHeight(18);
    displayLayout->addWidget(m_nameLabel);

    mainLayout->addWidget(displayWidget);

    m_scrollHelper = new ScrollHelper(m_nameLabel, this);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    m_answerBtn = new QPushButton("Answer", this);
    m_answerBtn->setMinimumHeight(40);
    m_answerBtn->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: bold; "
        "background: #2e7d32; color: white; border: none; border-radius: 8px; }"
        "QPushButton:pressed { background: #1b5e20; }");

    m_rejectBtn = new QPushButton("Reject", this);
    m_rejectBtn->setMinimumHeight(40);
    m_rejectBtn->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: bold; "
        "background: #c62828; color: white; border: none; border-radius: 8px; }"
        "QPushButton:pressed { background: #8e0000; }");

    btnLayout->addWidget(m_answerBtn);
    btnLayout->addWidget(m_rejectBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_answerBtn, &QPushButton::clicked, this, &CallNotification::onAnswer);
    connect(m_rejectBtn, &QPushButton::clicked, this, &CallNotification::onReject);
}

void CallNotification::showNotification(const QString &number, const QString &name)
{
    m_numberLabel->setText(number);

    if (!name.isEmpty()) {
        m_scrollHelper->setText(name);
    } else {
        m_scrollHelper->stop();
        m_nameLabel->setText("-= UNKNOWN =-");
    }

    show();
    raise();
    activateWindow();
}

void CallNotification::hideNotification()
{
    m_scrollHelper->stop();
    hide();
}

void CallNotification::onAnswer()
{
    m_scrollHelper->stop();
    if (!m_client->answerCall()) {
        emit rejected();
    } else {
        emit accepted();
    }
    hide();
}

void CallNotification::onReject()
{
    m_scrollHelper->stop();
    m_client->hangup();
    emit rejected();
    hide();
}
