#pragma once

#include <Arduino.h>

class SemVer {
public:
  SemVer(const String& versionStr);
  String toString() const;

  bool operator>(const SemVer& other) const;
  bool operator<(const SemVer& other) const;
  bool operator==(const SemVer& other) const;
  bool operator!=(const SemVer& other) const;

protected:
  int major;
  int minor;
  int patch;
};

SemVer::SemVer(const String& versionStr) {
  int firstDot = versionStr.indexOf('.');
  int secondDot = versionStr.lastIndexOf('.');
  major = versionStr.substring(0, firstDot).toInt();
  minor = versionStr.substring(firstDot + 1, secondDot).toInt();
  patch = versionStr.substring(secondDot + 1).toInt();
}

String SemVer::toString() const {
  return String(major) + "." + String(minor) + "." + String(patch);
}

bool SemVer::operator>(const SemVer& other) const {
  if (major > other.major) return true;
  if (minor > other.minor) return true;
  if (patch > other.patch) return true;
  return false;
}

bool SemVer::operator<(const SemVer& other) const {
  if (major < other.major) return true;
  if (minor < other.minor) return true;
  if (patch < other.patch) return true;
  return false;
}

bool SemVer::operator==(const SemVer& other) const {
  if (major != other.major) return false;
  if (minor != other.minor) return false;
  if (patch != other.patch) return false;
  return true;
}

bool SemVer::operator!=(const SemVer& other) const {
  return !operator==(other);
}