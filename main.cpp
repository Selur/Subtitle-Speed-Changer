#include <QFile>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <iostream>
using namespace std;

#include "SpeedChanger.h"

void printUsage()
{
  cerr << "Welcome to 'SubtitleSpeedChanger'!!" << endl;
  cerr << " written by Selur" << endl;
  cerr
      << "subtitleSpeedChange --input \"Path to subtitle file\" --speed factor --delay delayMs --output \"Path to output file\""
      << endl;
  cerr
      << " i.e.: subtitleSpeedChange --input \"d:\\InputSubtitle.idx\" --speed 0.892 --delay 10 --output \"Path to output file\""
      << endl;
  cerr << "Side notes:" << endl;
  cerr << "  [*] --input \"Path to subtitle file\"" << endl;
  cerr << "        specifies the input file" << endl;
  cerr << "  [*] --speed" << endl;
  cerr << "        also accepts toFPS/fromFPS; and " << endl;
  cerr << "  [*] --outfps" << endl;
  cerr
      << "        alternative to --speed causes subtitleSpeedChange to guess the fps of the input subtitle and calculate the speed with the output to match the output fps"
      << endl;
}

void removeQuotes(QString &input)
{
  input = input.trimmed();
  if (input.startsWith("\"")) {
    input = input.remove(0, 1);
  }
  if (input.endsWith("\"")) {
    input = input.remove(input.size() - 1, 1);
  }
}

QString getExtension(QString filename)
{
  if (filename.isEmpty()) {
    return QString();
  }
  int index = filename.lastIndexOf(".");
  removeQuotes(filename);
  filename = filename.remove(0, index + 1);
  return filename.toLower();
}

bool checkExtension(const QString path)
{
  QStringList supportedExtensions;
  supportedExtensions << "srt" << "ass" << "idx" << "ttxt";
  QString extension = getExtension(path);
  if (!supportedExtensions.contains(extension, Qt::CaseSensitive)) {
    cerr
        << qPrintable("Error - Unknown/Unsupported extension ."+extension+" for '"+path+"' !") << endl;
    return false;
  }
  return true;
}

bool checkFile(const QString path)
{
  if (!QFile::exists(path)) {
    cerr << qPrintable("Error - There's no file named '"+path+"' !") << endl;
    return false;
  }
  return true;
}

bool checkPath(QString &path)
{
  removeQuotes(path);
  if (!checkExtension(path)) {
    return false;
  }
  if (!checkFile(path)) {
    return false;
  }
  return true;
}

QString readInputFile(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    cerr << qPrintable("Error - Couldn't open '"+path+"' in read only mode!") << endl;
    return QString();
  }
  cerr << "loading " << qPrintable(path) << " into memory,.." << endl;
  QString content = file.readAll();
  cerr << "-> finished loading " << qPrintable(path) << " into memory,.." << endl;
  return content;
}

QRegExp regExpFor(QString string)
{
  return QRegExp("*" + string + "*", Qt::CaseInsensitive, QRegExp::Wildcard);
}

//cerr << "subtitleSpeedChange --input \"Path to subtitle file\" --speed factor --delay delayMs --output \"Path to output file\"" << endl;
int main(int argc, char *argv[])
{
  cerr << "Checking input,.." << endl;
  QStringList input;
  for (int i = 0; i < argc; ++i) {
    input << argv[i];
  }
  int index = input.indexOf(regExpFor("--input"));
  if (index == -1) {
    cerr << "Error - No '--input' specified!" << endl;
    printUsage();
    return -1;
  }
  QString path = input.at(index + 1);
  if (!checkPath(path)) {
    return -1;
  }
  cerr << "Input: " << qPrintable(path) << endl;

  QString fileContent = readInputFile(path);
  if (fileContent.isEmpty()) {
    cerr << "Error - File is empty,.. nothing to do,.." << endl;
    return -1;
  }

  double speed = 1;
  index = input.indexOf(regExpFor("--speed"));
  if (index != -1) {
    QString tmp = input.at(index + 1);
    if (!tmp.contains("/")) {
      speed = tmp.toDouble();
    } else {
      QStringList list = tmp.split("/");
      speed = list.at(0).toDouble() / list.at(1).toDouble();
    }
    cerr << "Speed: " << speed << endl;
  }

  double outfps = 0;
  index = input.indexOf(regExpFor("--outfps"));
  if (index != -1) {
    outfps = input.at(index + 1).toDouble();
    cerr << "outfps: " << outfps << endl;
  }

  double delay = 0;
  index = input.indexOf(regExpFor("--delay"));
  if (index != -1) {
    delay = input.at(index + 1).toDouble();
    cerr << "Delay: " << delay << endl;
  }

  QString output = QString();
  index = input.indexOf(regExpFor("--output"));
  if (index != -1) {
    output = input.at(index + 1);
    cerr << "Output: " << qPrintable(output) << endl;
  }
  SpeedChanger speedChanger(path, fileContent, delay, speed, outfps, output);
  int ret = 0;
  if (!(speedChanger.adjustSpeed())) {
    ret = -1;
  }
  cerr << "finished,.." << endl;
  return ret;
}
