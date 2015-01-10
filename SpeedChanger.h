/*
 * SpeedChanger.h
 *
 *  Created on: Aug 30, 2011
 *      Author: Selur
 */

#ifndef SPEEDCHANGER_H_
#define SPEEDCHANGER_H_
#include <QString>
#include <QTime>
#include <QList>

class SpeedChanger
{
  public:
    SpeedChanger(const QString path, const QString content, const double delay, const double speed,
                 const double outputfps, const QString output);
    bool adjustSpeed();

  private:
    QString m_path, m_content, m_output;
    bool m_guessInputFps;
    double m_delay, m_speed, m_outputFps;
    void adjustSrt();
    void adjustAss();
    void adjustIdx();
    void adjustTtx();
    bool guessFpsFromSrt();
    bool guessFpsFromAss();
    bool guessFpsFromIdx();
    bool guessFpsFromTtx();
    bool guessInputFps(QList<int> times);

    void save();
    double timeToSeconds(QString value);
    double timeToSeconds(QTime time);
    QString secondsToHMSZZZ(double seconds);

};

#endif /* SPEEDCHANGER_H_ */
