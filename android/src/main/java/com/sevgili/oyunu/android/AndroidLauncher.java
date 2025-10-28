package com.sevgili.oyunu.android;

import android.os.Bundle;

import com.badlogic.gdx.backends.android.AndroidApplication;
import com.badlogic.gdx.backends.android.AndroidApplicationConfiguration;
// SON VE DOĞRU PAKET ADI: SevgiliOyunu sınıfı, 'game' paketinin içinde.
import com.sevgili.oyunu.game.SevgiliOyunu; 
import com.badlogic.gdx.utils.GdxNativesLoader;

public class AndroidLauncher extends AndroidApplication {
@Override
protected void onCreate (Bundle savedInstanceState) {
super.onCreate(savedInstanceState);

// Native kütüphanelerin yüklenmesini garanti eden kod (libgdx.so fix)
GdxNativesLoader.load();

AndroidApplicationConfiguration config = new AndroidApplicationConfiguration();
initialize(new SevgiliOyunu(), config);
}
}
