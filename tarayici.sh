#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

echo "🚀 Turbo hata.sh v2 — Android NDK + Gradle + XML tarayıcı/düzeltici"

ROOT_DIR="$(pwd)"
CPP_DIR="android/src/main/cpp"
CMAKE_FILE="$CPP_DIR/CMakeLists.txt"
MANIFEST="android/src/main/AndroidManifest.xml"
GRADLE_FILE="android/build.gradle"
LOG_DIR=".scanlog"
mkdir -p "$LOG_DIR"

AUTO_FIX="${1:-}"           # "düzelt" ise interaktif düzeltme
AUTO_YES="${AUTO_YES:-0}"   # 1 ise onaysız

CXX_LOG="$LOG_DIR/clang_errors.log"
XML_LOG="$LOG_DIR/xml_errors.log"
KURAL_LOG="$LOG_DIR/rules.log"
> "$CXX_LOG"; > "$XML_LOG"; > "$KURAL_LOG"

confirm() {
  local msg="$1"
  if [ "$AUTO_FIX" != "düzelt" ]; then return 1; fi
  if [ "$AUTO_YES" = "1" ]; then return 0; fi
  read -rp "🤖 $msg [y/N]: " ans
  [[ "${ans,,}" == "y" ]]
}

backup_and_patch() {
  local file="$1"
  local patch_content="$2"
  cp "$file" "$file.bak"
  printf "%s" "$patch_content" > "$file"
  echo "🧩 Patch uygulandı: $file (backup: $file.bak)"
  echo "------ DIFF ------"
  if command -v diff >/dev/null 2>&1; then
    diff -u "$file.bak" "$file" || true
  else
    echo "(diff yok)"
  fi
  echo "------------------"
}

find_ndk_include() {
  # Termux/Android NDK include path’lerini olabildiğince tarar
  local hits=""
  for cand in \
    "$HOME/ndk_includes" \
    "$HOME/Android/Sdk/ndk"/*/sources/android/native_app_glue \
    "$HOME/Android/Sdk/ndk"/* \
    "/data/data/com.termux/files/usr/include" \
    "/data/data/com.termux/files/usr/lib/clang"/*/include \
    "/data/data/com.termux/files/usr/include/android" \
    "/system/include" "/system/usr/include"
  do
    if [ -d "$cand" ] || [ -f "$cand/android_native_app_glue.h" ]; then
      hits="$hits $cand"
    fi
  done
  echo "$hits"
}

derive_cxxflags() {
  local flags="-std=c++17 -Wall -Wextra -fsyntax-only"
  local incs="$(find_ndk_include)"
  for p in $incs; do
    if [ -f "$p/android_native_app_glue.h" ] || [ -d "$p" ]; then
      flags="$flags -I$p"
    fi
  done
  # Yaygın NDK header’ları için tahmini ek yollar:
  flags="$flags -I$HOME/Android/Sdk/ndk/*/sources/android/native_app_glue"
  echo "$flags"
}

compile_check() {
  echo "🔍 [1/6] C++ derleme ve sentaks kontrolü (clang++)"
  local cxxflags; cxxflags="$(derive_cxxflags)"
  local CXX_FILES
  CXX_FILES=$(find "$CPP_DIR" -name "*.cpp" -o -name "*.cc" -o -name "*.cxx")
  if [ -z "$CXX_FILES" ]; then
    echo "ℹ️ C++ kaynak bulunamadı: $CPP_DIR"
    return
  fi

  for file in $CXX_FILES; do
    echo "🧪 Derleniyor: $file"
    # Sadece sentaks; link yok. Include path’ler ve defines eklenebilir.
    clang++ $cxxflags "$file" 2>>"$CXX_LOG" || true
    if grep -q "$file" "$CXX_LOG"; then
      echo "❌ Hata: $file"
      echo "--------------------------------------------------"
      nl -ba "$file" | sed 's/^/  /'
      echo "--------------------------------------------------"
      echo "🔥 clang++ çıktısı:"
      grep "$file" "$CXX_LOG" || true
      echo ""
    fi
  done

  if [ -s "$CXX_LOG" ]; then
    echo "⚠️ C++ hataları bulundu."
  else
    echo "✅ Tüm C++ dosyaları sentaks açısından temiz."
  fi
}

cmake_check() {
  echo ""
  echo "🔍 [2/6] CMakeLists.txt kontrolü"
  if [ ! -f "$CMAKE_FILE" ]; then
    echo "❌ $CMAKE_FILE bulunamadı!"
    return
  fi

  local cmake_txt; cmake_txt="$(cat "$CMAKE_FILE")"

  if ! grep -q "add_library" "$CMAKE_FILE"; then
    echo "⚠️ add_library eksik görünüyor."
    local patch="cmake_minimum_required(VERSION 3.10)
project(native_app)

add_library(native_app SHARED
    main.cpp
)

find_library(log-lib log)
target_link_libraries(native_app
    android
    ${log-lib}
)
"
    if confirm "CMakeLists.txt için temel NDK şablonu ekleyelim mi?"; then
      backup_and_patch "$CMAKE_FILE" "$patch"
    fi
  fi

  if ! grep -q "android" "$CMAKE_FILE"; then
    echo "⚠️ target_link_libraries içinde 'android' yok; NDK native glue ve activity için gerekli olabilir."
  fi
  if ! grep -q "log" "$CMAKE_FILE"; then
    echo "⚠️ 'log' kütüphanesi yok; __android_log_print için gerekli."
  fi
}

xml_check() {
  echo ""
  echo "🔍 [3/6] XML validasyon (res ve manifest)"
  pkg install libxml2 -y >/dev/null 2>&1 || true

  local XML_FILES
  XML_FILES=$(find android/src/main/res -name "*.xml" 2>/dev/null || true)
  XML_FILES="$XML_FILES $MANIFEST"
  > "$XML_LOG"

  for xml in $XML_FILES; do
    [ -f "$xml" ] || continue
    echo "📄 Kontrol: $xml"
    xmllint --noout "$xml" 2>>"$XML_LOG" || true
    if grep -q "$xml" "$XML_LOG"; then
      echo "❌ XML hatası: $xml"
      nl -ba "$xml" | sed 's/^/  /'
      echo "🔥 xmllint çıktısı:"
      grep "$xml" "$XML_LOG" || true
      echo ""
    fi
  done

  if [ -s "$XML_LOG" ]; then
    echo "⚠️ XML hataları bulundu."
  else
    echo "✅ Tüm XML dosyaları geçerli."
  fi
}

manifest_check() {
  echo ""
  echo "🔍 [4/6] AndroidManifest.xml inceleniyor"
  if [ ! -f "$MANIFEST" ]; then
    echo "❌ Manifest bulunamadı: $MANIFEST"
    return
  fi
  local has_pkg has_act
  has_pkg=$(grep -c "package=" "$MANIFEST" || true)
  has_act=$(grep -c "<activity" "$MANIFEST" || true)
  if [ "$has_pkg" -gt 0 ]; then echo "✅ package tanımı var."; else echo "❌ package tanımı eksik!"; fi
  if [ "$has_act" -gt 0 ]; then echo "✅ activity tanımı var."; else echo "❌ activity tanımı eksik!"; fi

  # Native activity örnek öneri
  if ! grep -q "android.app.NativeActivity" "$MANIFEST"; then
    echo "ℹ️ NativeActivity kullanıyorsan, activity’nin name’i android.app.NativeActivity olmalı."
  fi
}

gradle_check() {
  echo ""
  echo "🔍 [5/6] build.gradle kontrolü"
  if [ ! -f "$GRADLE_FILE" ]; then
    echo "❌ build.gradle bulunamadı!"
    return
  fi
  local has_app has_ndk
  has_app=$(grep -c "com.android.application" "$GRADLE_FILE" || true)
  has_ndk=$(grep -c "externalNativeBuild" "$GRADLE_FILE" || true)

  if [ "$has_app" -gt 0 ]; then
    echo "✅ application plugin tanımlı."
  else
    echo "❌ application plugin eksik!"
  fi

  if [ "$has_ndk" -gt 0 ]; then
    echo "✅ externalNativeBuild/CMake yapılandırması var."
  else
    echo "⚠️ NDK/CMake entegrasyonu eksik olabilir (externalNativeBuild)."
  fi
}

rules_engine() {
  echo ""
  echo "🔍 [6/6] Kural motoru: hata imzaları → çözüm önerileri"
  # clang hata günlüğü üzerinden yaygın imzalar
  local log_content; log_content="$(cat "$CXX_LOG" 2>/dev/null || true)"

  # 1) android_native_app_glue.h bulunamadı
  if echo "$log_content" | grep -qi "android_native_app_glue\.h"; then
    echo "🔧 İpucu: android_native_app_glue.h bulunamıyor."
    echo "— Çözüm: NDK include path’ine native_app_glue dizinini ekleyin veya CMake’de include_directories kullanın."
    echo "— Örnek CMake: include_directories(\${ANDROID_NDK}/sources/android/native_app_glue)"
    echo "— Ek olarak: target_link_libraries(native_app android log)"
    echo "✍️ Düzeltme adımı: CMake’e include ve link ekleyelim."
    if [ -f "$CMAKE_FILE" ] && confirm "CMakeLists.txt’ye native_app_glue include path’i ve android/log linkleri ekleyelim mi?"; then
      cp "$CMAKE_FILE" "$CMAKE_FILE.bak"
      awk '
        BEGIN { added_inc=0; added_link=0 }
        { print }
        /add_library/ && added_inc==0 {
          print "include_directories(${ANDROID_NDK}/sources/android/native_app_glue)"
          added_inc=1
        }
        /target_link_libraries/ && added_link==0 {
          print "target_link_libraries(native_app android log)"
          added_link=1
        }
      ' "$CMAKE_FILE.bak" > "$CMAKE_FILE"
      echo "🧩 CMake include/link eklendi."
    fi
  fi

  # 2) ANativeActivity_onCreate undefined reference (link zamanı)
  if echo "$log_content" | grep -qi "ANativeActivity_onCreate"; then
    echo "🔧 İpucu: ANativeActivity_onCreate sembolü tanımsız."
    echo "— Sebep: NativeActivity için doğru imza veya link kütüphaneleri (android) eksik olabilir."
    echo "— Çözüm: C dosyanızda doğru imza ile tanımlayın ve android lib’ini linkleyin."
    echo "— CMake: target_link_libraries(native_app android log)"
  fi

  # 3) __android_log_print not found
  if echo "$log_content" | grep -qi "__android_log_print"; then
    echo "🔧 İpucu: __android_log_print sembolü bulunamadı."
    echo "— Çözüm: log kütüphanesini linkleyin."
    echo "— CMake: find_library(log-lib log); target_link_libraries(native_app ${log-lib})"
  fi

  # 4) JNI imza hataları
  if echo "$log_content" | grep -qi "JNI"; then
    echo "🔧 İpucu: JNI ile ilgili hata."
    echo "— Çözüm: Paket/ad alanı ve yöntem imzalarını (Java_com_package_Class_method) kontrol edin; NDK r21+ sürüm uyumluluğunu doğrulayın."
  fi

  # XML hataları için imza örneği
  local xml_content; xml_content="$(cat "$XML_LOG" 2>/dev/null || true)"
  if echo "$xml_content" | grep -qi "Opening"; then
    echo "🔧 İpucu: XML açılış/kapanış etiketi tutarsız."
    echo "— Çözüm: xmllint çıktısındaki satırı manuel düzeltin; istersen düzelt modunda otomatik kapatma eklenir."
  fi
}

main() {
  compile_check
  cmake_check
  xml_check
  manifest_check
  gradle_check
  rules_engine

  echo ""
  echo "📋 Özet:"
  [ -s "$CXX_LOG" ] && echo "• C++ hata günlüğü: $CXX_LOG" || echo "• C++ hatası yok."
  [ -s "$XML_LOG" ] && echo "• XML hata günlüğü: $XML_LOG" || echo "• XML hatası yok."
  echo "• Kural motoru raporu: $KURAL_LOG (konsola yazıldı)"

  echo ""
  if [ "$AUTO_FIX" = "düzelt" ]; then
    echo "🛠 İnteraktif düzeltme modu aktif."
  else
    echo "ℹ️ Otomatik düzeltme için: bash tarayici.sh düzelt  (AUTO_YES=1 onaysız)"
  fi
}

main "$@"
