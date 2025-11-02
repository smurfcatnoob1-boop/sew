#!/data/data/com.termux/files/usr/bin/bash

echo "🔍 [1/5] C++ Derleme ve Sentaks Kontrolü (clang++)"
CPP_DIR="android/src/main/cpp"
CXX_FILES=$(find "$CPP_DIR" -name "*.cpp")

mkdir -p .scanlog
CXX_LOG=".scanlog/clang_errors.log"
> "$CXX_LOG"

for file in $CXX_FILES; do
  echo "🧪 Derleniyor: $file"
  clang++ -std=c++17 -Wall -Wextra -fsyntax-only "$file" 2>>"$CXX_LOG"
  if [ $? -ne 0 ]; then
    echo "❌ HATA: $file"
    echo "↪ Dosya içeriği:"
    echo "--------------------------------------------------"
    nl -ba "$file" | sed 's/^/  /'
    echo "--------------------------------------------------"
    echo "🔥 clang++ çıktısı:"
    grep "$file" "$CXX_LOG"
    echo ""
  fi
done

if [ -s "$CXX_LOG" ]; then
  echo "⚠️ C++ hataları bulundu."
else
  echo "✅ Tüm C++ dosyaları sentaks açısından temiz."
fi

echo ""
echo "🔍 [2/5] CMakeLists.txt içinde kaynak kontrolü"
CMAKE_FILE="$CPP_DIR/CMakeLists.txt"
if [ -f "$CMAKE_FILE" ]; then
  if grep -q "main.cpp" "$CMAKE_FILE"; then
    echo "✅ main.cpp CMakeLists.txt içinde tanımlı."
  else
    echo "❌ main.cpp CMakeLists.txt içinde eksik!"
    echo "↪ Dosya içeriği:"
    echo "--------------------------------------------------"
    nl -ba "$CMAKE_FILE" | sed 's/^/  /'
    echo "--------------------------------------------------"
  fi
else
  echo "❌ CMakeLists.txt bulunamadı!"
fi

echo ""
echo "🔍 [3/5] XML Validasyon (strings.xml, styles.xml, manifest)"
pkg install libxml2 -y >/dev/null 2>&1
XML_FILES=$(find android/src/main/res -name "*.xml")
XML_FILES+=" android/src/main/AndroidManifest.xml"
XML_LOG=".scanlog/xml_errors.log"
> "$XML_LOG"

for xml in $XML_FILES; do
  echo "📄 Kontrol: $xml"
  xmllint --noout "$xml" 2>>"$XML_LOG"
  if [ $? -ne 0 ]; then
    echo "❌ XML hatası: $xml"
    echo "↪ Dosya içeriği:"
    echo "--------------------------------------------------"
    nl -ba "$xml" | sed 's/^/  /'
    echo "--------------------------------------------------"
    echo "🔥 xmllint çıktısı:"
    grep "$xml" "$XML_LOG"
    echo ""
  fi
done

if [ -s "$XML_LOG" ]; then
  echo "⚠️ XML hataları bulundu."
else
  echo "✅ Tüm XML dosyaları geçerli."
fi

echo ""
echo "🔍 [4/5] AndroidManifest.xml içeriği"
MANIFEST="android/src/main/AndroidManifest.xml"
if grep -q "package=" "$MANIFEST"; then
  echo "✅ package tanımı var."
else
  echo "❌ package tanımı eksik!"
  echo "↪ Dosya içeriği:"
  echo "--------------------------------------------------"
  nl -ba "$MANIFEST" | sed 's/^/  /'
  echo "--------------------------------------------------"
fi

if grep -q "<activity" "$MANIFEST"; then
  echo "✅ activity tanımı var."
else
  echo "❌ activity tanımı eksik!"
fi

echo ""
echo "🔍 [5/5] build.gradle kontrolü"
GRADLE_FILE="android/build.gradle"
if [ -f "$GRADLE_FILE" ]; then
  if grep -q "com.android.application" "$GRADLE_FILE"; then
    echo "✅ build.gradle içinde application plugin tanımlı."
  else
    echo "❌ build.gradle içinde plugin eksik!"
    echo "↪ Dosya içeriği:"
    echo "--------------------------------------------------"
    nl -ba "$GRADLE_FILE" | sed 's/^/  /'
    echo "--------------------------------------------------"
  fi
else
  echo "❌ build.gradle dosyası bulunamadı!"
fi
