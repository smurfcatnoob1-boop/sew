package com.sevgili.oyunu.android;

import android.os.Bundle;

import com.badlogic.gdx.backends.android.AndroidApplication;
import com.badlogic.gdx.backends.android.AndroidApplicationConfiguration;
import com.sevgili.oyunu.SevgiliOyunu; // Bu sınıfı bulması gerekiyor
import com.badlogic.gdx.utils.GdxNativesLoader;

public class AndroidLauncher extends AndroidApplication {
@Override
protected void onCreate (Bundle savedInstanceState) {
super.onCreate(savedInstanceState);

// Native kütüphanelerin yüklenmesini garanti eden son fix
GdxNativesLoader.load();

AndroidApplicationConfiguration config = new AndroidApplicationConfiguration();
initialize(new SevgiliOyunu(), config);
}
}
