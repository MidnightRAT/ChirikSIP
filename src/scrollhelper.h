#ifndef SCROLLHELPER_H
#define SCROLLHELPER_H

#include <QLabel>
#include <QTimer>
#include <QFontMetrics>

class ScrollHelper : public QObject
{
    Q_OBJECT
public:
    explicit ScrollHelper(QLabel *label, QObject *parent = nullptr);

    void setText(const QString &text);
    void stop();

private slots:
    void onTick();

private:
    QLabel *m_label;
    QTimer *m_timer;
    QString m_text;
    int m_offset = 0;
};

#endif // SCROLLHELPER_H
