package com.sevgili.oyunu.android;

import android.os.Bundle;

import com.badlogic.gdx.backends.android.AndroidApplication;
import com.badlogic.gdx.backends.android.AndroidApplicationConfiguration;
// KESİN ÇÖZÜM: SevgiliOyunu sınıfını bulmak için wildcard (*) kullanıldı.
import com.sevgili.oyunu.*;
import com.badlogic.gdx.utils.GdxNativesLoader;

public class AndroidLauncher extends AndroidApplication {
@Override
protected void onCreate (Bundle savedInstanceState) {
super.onCreate(savedInstanceState);

// Native kütüphanelerin yüklenmesini garanti eden kod
GdxNativesLoader.load();

AndroidApplicationConfiguration config = new AndroidApplicationConfiguration();
// Artık SevgiliOyunu sınıfını bulması gerekiyor.
initialize(new SevgiliOyunu(), config);
}
}
