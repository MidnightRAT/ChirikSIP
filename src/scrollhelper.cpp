#include "scrollhelper.h"

static const QString SCROLL_SEP = QStringLiteral(" *|* ");
static const int SCROLL_VISIBLE = 30;
static const int SCROLL_MARGIN = 20;
static const int SCROLL_INTERVAL_MS = 200;

ScrollHelper::ScrollHelper(QLabel *label, QObject *parent)
    : QObject(parent)
    , m_label(label)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(SCROLL_INTERVAL_MS);
    connect(m_timer, &QTimer::timeout, this, &ScrollHelper::onTick);
}

void ScrollHelper::setText(const QString &text)
{
    m_text = text;
    m_offset = 0;

    if (text.isEmpty()) {
        m_timer->stop();
        m_label->setText(QString());
        return;
    }

    QFontMetrics fm(m_label->font());
    int avail = m_label->width() - SCROLL_MARGIN;
    if (fm.horizontalAdvance(text) <= avail) {
        m_label->setText(text);
        m_timer->stop();
        return;
    }

    m_timer->start();
}

void ScrollHelper::stop()
{
    m_timer->stop();
    if (!m_text.isEmpty())
        m_label->setText(m_text);
}

void ScrollHelper::onTick()
{
    if (m_text.isEmpty()) {
        m_timer->stop();
        return;
    }

    QFontMetrics fm(m_label->font());
    int avail = m_label->width() - SCROLL_MARGIN;
    if (fm.horizontalAdvance(m_text) <= avail) {
        m_label->setText(m_text);
        m_timer->stop();
        return;
    }

    QString src = m_text + SCROLL_SEP;
    m_label->setText(src.mid(m_offset % src.length(), SCROLL_VISIBLE));
    m_offset++;
}
