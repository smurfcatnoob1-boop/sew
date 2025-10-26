#!/usr/bin/env sh
exec java -Dorg.gradle.appname=gradle -jar $(dirname "$0")/gradle/wrapper/gradle-wrapper.jar "$@"
