#include "callnotification.h"
#include "sipclient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontMetrics>
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

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(200);
    connect(m_scrollTimer, &QTimer::timeout, this, &CallNotification::onScrollTick);

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
        QFontMetrics fm(m_nameLabel->font());
        int avail = m_nameLabel->width() - 20;
        if (fm.horizontalAdvance(name) <= avail) {
            m_nameLabel->setText(name);
            m_scrollTimer->stop();
        } else {
            m_scrollText = name;
            m_scrollOffset = 0;
            m_scrollTimer->start();
        }
    } else {
        m_nameLabel->setText("-= UNKNOWN =-");
        m_scrollTimer->stop();
    }

    show();
    raise();
    activateWindow();
}

void CallNotification::hideNotification()
{
    m_scrollTimer->stop();
    hide();
}

void CallNotification::onAnswer()
{
    m_scrollTimer->stop();
    m_client->answerCall();
    emit accepted();
    hide();
}

void CallNotification::onReject()
{
    m_scrollTimer->stop();
    m_client->hangup();
    emit rejected();
    hide();
}

void CallNotification::onScrollTick()
{
    if (m_scrollText.isEmpty()) {
        m_scrollTimer->stop();
        return;
    }

    QFontMetrics fm(m_nameLabel->font());
    int avail = m_nameLabel->width() - 20;
    if (fm.horizontalAdvance(m_scrollText) <= avail) {
        m_nameLabel->setText(m_scrollText);
        m_scrollTimer->stop();
        return;
    }

    QString sep = " *|* ";
    QString src = m_scrollText + sep;
    m_nameLabel->setText(src.mid(m_scrollOffset % src.length(), 30));
    m_scrollOffset++;
}
