/*
 * SpeedChanger.cpp
 *
 *  Created on: Aug 30, 2011
 *      Author: Selur
 */

#include "SpeedChanger.h"
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QTextCodec>
#include <iostream>
using namespace std;

SpeedChanger::SpeedChanger(const QString path, const QString content, const double delay,
                           const double speed, const double outputFps, const QString output) :
    m_path(path), m_content(content), m_output(output), m_guessInputFps(outputFps != 0),
        m_delay(delay), m_speed(speed), m_outputFps(outputFps)
{

}

// << "srt" << "ass" << "idx" << << "ttxt"
bool SpeedChanger::adjustSpeed()
{
  if (m_guessInputFps) {
    cerr << "guessing frame rate from input subtitles,.." << endl;
    bool success = false;
    if (m_path.endsWith("srt", Qt::CaseInsensitive)) {
      success = this->guessFpsFromSrt();
    } else if (m_path.endsWith("ass", Qt::CaseInsensitive)) {
      success = this->guessFpsFromAss();
    } else if (m_path.endsWith("idx", Qt::CaseInsensitive)) {
      success = this->guessFpsFromIdx();
    } else if (m_path.endsWith("ttx", Qt::CaseInsensitive)) {
      success = this->guessFpsFromTtx();
    }
    cerr << "-> calculated speed: " << m_speed << endl;
    if (!success) {
      return false;
    }
  }
  if (m_path.endsWith("srt", Qt::CaseInsensitive)) {
    this->adjustSrt();
  } else if (m_path.endsWith("ass", Qt::CaseInsensitive)) {
    this->adjustAss();
  } else if (m_path.endsWith("idx", Qt::CaseInsensitive)) {
    this->adjustIdx();
  } else if (m_path.endsWith("ttx", Qt::CaseInsensitive)) {
    this->adjustTtx();
  } else {
    cerr << qPrintable("unknown file type "+m_path) << endl;
    return false;
  }
  return true;
}

/**
 * saves the content of a QString 'text' into a file 'to'
 **/
void SpeedChanger::save()
{
  QString to = m_output;
  if (to.isEmpty()) {
    to = m_path;
    to = to.insert(to.lastIndexOf("."), "_adjusted");
  }

  QString text = m_content;
  if (text.isEmpty()) {
    cerr << qPrintable(QObject::tr("Failed: saveTextTo %1 called with empty text").arg(to)) << endl;
    return;
  }

  QFile file(to);
  file.remove();
  QByteArray encoding = "utf-8";
  if (m_path.endsWith("srt", Qt::CaseInsensitive)) {
    encoding = "windows-1258";
  }
  cerr << "writing file to " << qPrintable(to) << endl;

  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out.setCodec(QTextCodec::codecForName(encoding));
    out << text;
    if (file.exists()) {
      file.close();
      cerr << "finished!" << endl;
      return;
    }
  }
  cerr << qPrintable(QObject::tr("Failed to saveTextTo %1").arg(to)) << endl;
}

QString SpeedChanger::secondsToHMSZZZ(double seconds)
{
  if (seconds == 0) {
    return "00:00:00";
  }
  QString time = QString();
  int hrs = 0;
  if (seconds >= 3600) { //Stunden
    hrs = int(seconds) / 3600;
  } else if (seconds == 3600) {
    hrs = 1;
  }
  time += QString((hrs < 10) ? "0" : QString())+ QString::number(hrs);

  int min = 0;
  seconds = seconds - 3600 * hrs;
  if (seconds >= 60) { //Minuten
    min = int(seconds) / 60;
  }
  time += ":" + QString((min < 10) ? "0" : QString())+ QString::number(min);

  int sec = 0;
  seconds = seconds - 60 * min;
  if (seconds > 0) { //Sekunden
    sec = int(seconds);
  }
  time += ":";
  time += QString((sec < 10) ? "0" : QString());
  time += QString::number(sec);
  int milliseconds = int(1000 * (seconds - (sec * 1.0)) + 0.5);
  if (milliseconds == 0) {
    time += ".000";
    return time;
  }
  time += ".";
  if (milliseconds < 10) {
    time += "00";
  } else if (milliseconds < 100) {
    time += "0";
  }
  time += QString::number(milliseconds);
  return time;
}

double SpeedChanger::timeToSeconds(QTime time)
{
  return ((time.hour() * 60.0 + time.minute()) * 60.0 + time.second()) + (time.msec() / 1000.0);
}

/**
 * converts a time string to seconds
 **/
double SpeedChanger::timeToSeconds(QString value)
{
  QString tmp = value.trimmed();
  double dtime = tmp.toDouble();
  int index = tmp.indexOf(":");
  bool tripple = tmp.count(":") == 3;
  if (index == -1) {
    return 0;
  }
  if (index == 1) {
    tmp = "0" + tmp;
  }
  bool dot = true;
  index = tmp.indexOf(".");
  if (index == -1) {
    index = tmp.indexOf(",");
    dot = false;
  }
  if (index != -1) {
    int length = tmp.length();
    QTime time;
    if ((length - index) == 2) {
      if (tripple) {
        time = QTime::fromString(tmp, "hh:mm:ss:z");
      } else if (dot) {
        time = QTime::fromString(tmp, "hh:mm:ss.z");
      } else {
        time = QTime::fromString(tmp, "hh:mm:ss,z");
      }
    } else {
      if ((length - index) == 3) {
        tmp += "0";
      }
      if (tripple) {
        time = QTime::fromString(tmp, "hh:mm:ss:zzz");
      } else if (dot) {
        time = QTime::fromString(tmp, "hh:mm:ss.zzz");
      } else {
        time = QTime::fromString(tmp, "hh:mm:ss,zzz");
      }
    }
    dtime = timeToSeconds(time);
  } else {
    QTime time;
    if (dot) {
      time = QTime::fromString(tmp, "hh:mm:ss");
    } else {
      time = QTime::fromString(tmp, "hh:mm:ss");
    }
    dtime = (time.hour() * 60.0 + time.minute()) * 60.0 + time.second();
  }
  return dtime;
}

bool SpeedChanger::guessInputFps(QList<int> times)
{
  // 41ms / 42ms       <> 24 / 23.976fps
  // 40ms              <> 25fps
  // 33ms / 34ms       <> 30 / 29.976fps
  cerr << "checking times,.." << endl;
  int count24 = 0;
  int count25 = 0;
  int count30 = 0;
  int unknown = 0;

  int value;
  bool hit;
  for (int i = 0, c = times.count(); i < c; ++i) {
    value = times.at(i);
    hit = false;
    if (value % 41 == 0 || value % 42 == 0) {
      count24++;
      hit = true;
    }
    if (value % 40 == 0) {
      count25++;
      hit = true;
    }
    if (value % 34 == 0 || value % 33 == 0) {
      count30++;
      hit = true;
    }
    if (!hit) {
      unknown++;
    }
  }
  cerr << " counts: " << count24 << endl;
  cerr << "   23.976fps: " << count24 << endl;
  cerr << "   25fps: " << count25 << endl;
  cerr << "   29.976fps: " << count30 << endl;

  double fromFps;
  if (count24 >= count25 && count24 >= count30 && count24 >= unknown) {
    fromFps = 23.976;
  } else if (count25 >= count30 && count25 >= unknown) {
    fromFps = 25;
  } else if (count30 >= unknown) {
    fromFps = 30;
  } else {
    return false;
  }
  m_speed = m_outputFps / fromFps;

  return true;
}

bool SpeedChanger::guessFpsFromSrt()
{
  QList<int> times;
  QString line, from;
  QStringList lines = m_content.split("\n");
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();

    if (line.isEmpty() || !line.contains(" --> ")) {
      continue;
    }
    line = line.remove(line.indexOf(" --> "), line.size()).trimmed();
    //00:07:56,600
    line = line.remove(0, line.indexOf(",") + 1);
    times.append(line.toInt());
  }
  return this->guessInputFps(times);
}
bool SpeedChanger::guessFpsFromAss()
{
  QList<int> times;
  QString line;
  QStringList lines = m_content.split("\n"), elements;
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (line.isEmpty() || !line.startsWith("Dialogue:")) {
      continue;
    }
    line = line.remove(0, line.indexOf(":") + 1).trimmed();
    elements = line.split(",");
    if (elements.count() != 2) {
      continue;
    }
    line = elements.at(1);
    line = line.remove(0, line.indexOf(".") + 1);
    while (line.size() < 3) {
      line += "0";
    }
    times.append(line.toInt());
  }
  return this->guessInputFps(times);
}
bool SpeedChanger::guessFpsFromIdx()
{
  QList<int> times;

  QString line;
  QStringList lines = m_content.split("\n");
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (!line.startsWith("timestamp:")) {
      continue;
    }
    //timestamp: 00:00:19:280, filepos: 000002000
    line = line.remove(0, 11);
    line = line.remove(line.indexOf(","), line.size()).trimmed();
    line = line.remove(line.indexOf(":"), line.size()).trimmed();
    while (line.size() < 3) {
      line += "0";
    }
    times.append(line.toInt());
  }
  return this->guessInputFps(times);
}
bool SpeedChanger::guessFpsFromTtx()
{
  //<TextSample sampleTime="00:00:00.000" xml:space="preserve"></TextSample>
  QList<int> times;
  QString line;
  QStringList lines = m_content.split("\n");
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (line.isEmpty() || !line.startsWith("<TextSample")) {
      continue;
    }
    line = line.remove(0, line.indexOf(".") + 1);
    line = line.remove(line.indexOf("\""), line.length());
    while (line.size() < 3) {
      line += "0";
    }
    times.append(line.toInt());
  }
  return this->guessInputFps(times);
}

/**
 * 1
 * 00:00:14,680 --> 00:00:17,513
 * At the left we can see...
 */
void SpeedChanger::adjustSrt()
{
  QString line, from, to;
  QStringList lines = m_content.split("\n"), fromAndTo;
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();

    if (line.isEmpty() || !line.contains(" --> ")) {
      continue;
    }
    fromAndTo = line.split(" --> ");
    if (fromAndTo.count() != 2) {
      continue;
    }
    from = fromAndTo.at(0);
    from = from.trimmed();
    to = fromAndTo.at(1);
    to = to.trimmed();
    from = this->secondsToHMSZZZ((this->timeToSeconds(from) * 1000.0 * m_speed + m_delay) / 1000.0);
    from = from.replace(".", ",");
    to = this->secondsToHMSZZZ((this->timeToSeconds(to) * 1000.0 * m_speed + m_delay) / 1000.0);
    to = to.replace(".", ",");
    line = lines.takeAt(i);
    lines.insert(i, from + " --> " + to);
  }
  m_content = lines.join("\n");
  this->save();
}

/**
 * Dialogue: 0,0:00:15.04,0:00:18.04,Default,,0000,0000,0000,,Auf der linken Seite sehen wir...
 * Dialogue: 0,0:00:18.75,0:00:20.33,Default,,0000,0000,0000,,Auf der rechten Seite sehen wir die...
 * Dialogue: 0,0:00:20.41,0:00:21.91,Default,,0000,0000,0000,,...die Enthaupter.
 * Dialogue: 0,0:00:22.00,0:00:24.62,Default,,0000,0000,0000,,Alles ist sicher.|Vollkommen sicher.
 */
void SpeedChanger::adjustAss()
{
  QString line, from, to;
  QStringList lines = m_content.split("\n"), elements;
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (line.isEmpty() || !line.startsWith("Dialogue:")) {
      continue;
    }
    line = line.remove(0, line.indexOf(":") + 1).trimmed();
    elements = line.split(",");
    from = elements.at(1);
    from = this->secondsToHMSZZZ((this->timeToSeconds(from) * 1000.0 * m_speed + m_delay) / 1000.0);
    to = elements.at(2);
    to = this->secondsToHMSZZZ((this->timeToSeconds(to) * 1000.0 * m_speed + m_delay) / 1000.0);
    line = lines.takeAt(i);
    line = line.replace(elements.at(1), from);
    line = line.replace(elements.at(2), to);
    lines.insert(i, line);
  }
  m_content = lines.join("\n");
  this->save();
}

/**
 * # Vob/Cell ID: 1, 1 (PTS: 0)
 * time offset: 0
 * timestamp: 00:00:14:400, filepos: 000000000
 * timestamp: 00:00:17:440, filepos: 000001000
 * timestamp: 00:00:19:280, filepos: 000002000
 * timestamp: 00:00:21:120, filepos: 000002800
 * timestamp: 00:00:23:600, filepos: 000003800
 */
void SpeedChanger::adjustIdx()
{
  QString line, oldTime, newTime;
  bool adjusted = false;
  double adjust = 0;
  QStringList lines = m_content.split("\n");
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (line.isEmpty()) {
      continue;
    }
    if (line.startsWith("time offset:") || line.startsWith("delay:")) {
      if (adjusted) {
        continue;
      }
      oldTime = lines.takeAt(i);
      lines.insert(i, "# " + oldTime);
      oldTime = oldTime.remove(0, oldTime.indexOf(":") + 1).trimmed();
      if (oldTime.contains(":")) {
        adjust = this->timeToSeconds(oldTime) * 1000.0;
      }
      m_delay += adjust;
      adjusted = true;
      continue;
    }
    if (!line.startsWith("timestamp:")) {
      continue;
    }
    oldTime = line;
    oldTime = oldTime.remove(0, oldTime.indexOf(":") + 1);
    oldTime = oldTime.remove(oldTime.indexOf(","), oldTime.length());
    oldTime = oldTime.trimmed();
    newTime = this->secondsToHMSZZZ(
        (this->timeToSeconds(oldTime) * 1000.0 * m_speed + m_delay) / 1000.0);
    newTime = newTime.replace(".", ":");
    line = lines.takeAt(i);
    line = line.replace(oldTime, newTime);
    lines.insert(i, line);
  }
  m_content = lines.join("\n");
  this->save();
}

//<TextSample sampleTime="00:00:00.000" xml:space="preserve"></TextSample>
void SpeedChanger::adjustTtx()
{
  QString line, oldTime, newTime;
  QStringList lines = m_content.split("\n");
  for (int i = 0, c = lines.count(); i < c; ++i) {
    line = lines.at(i).trimmed();
    if (line.isEmpty() || !line.startsWith("<TextSample")) {
      continue;
    }
    oldTime = line;
    oldTime = oldTime.remove(0, oldTime.indexOf("\"") + 1);
    oldTime = oldTime.remove(oldTime.indexOf("\""), oldTime.length());
    newTime = this->secondsToHMSZZZ(
        (this->timeToSeconds(oldTime) * 1000.0 * m_speed + m_delay) / 1000.0);
    line = lines.takeAt(i);
    line = line.replace(oldTime, newTime);
    lines.insert(i, line);
  }
  m_content = lines.join("\n");
  this->save();
}
