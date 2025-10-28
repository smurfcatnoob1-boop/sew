package com.sevgili.oyunu.android;

import android.os.Bundle;

import com.badlogic.gdx.backends.android.AndroidApplication;
import com.badlogic.gdx.backends.android.AndroidApplicationConfiguration;
// KESİN ÇÖZÜM: Core modülünün paket adının doğru varsayımı: com.sevgili.oyunu.SevgiliOyunu'dan SevgiliOyunu'nu import et.
import com.sevgili.oyunu.SevgiliOyunu; 
import com.badlogic.gdx.utils.GdxNativesLoader;

public class AndroidLauncher extends AndroidApplication {
@Override
protected void onCreate (Bundle savedInstanceState) {
super.onCreate(savedInstanceState);

// Native kütüphanelerin yüklenmesini garanti eden kod (libgdx.so fix'i)
GdxNativesLoader.load();

AndroidApplicationConfiguration config = new AndroidApplicationConfiguration();
initialize(new SevgiliOyunu(), config);
}
}
