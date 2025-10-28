package com.sevgili.oyunu.android;

import android.os.Bundle;

import com.badlogic.gdx.backends.android.AndroidApplication;
import com.badlogic.gdx.backends.android.AndroidApplicationConfiguration;
import com.sevgili.oyunu.SevgiliOyunu;
// GdxNativesLoader kütüphanesini import etmeye gerek kalmaması için bu satırı sildik.
// import com.badlogic.gdx.utils.GdxNativesLoader; 

public class AndroidLauncher extends AndroidApplication {
@Override
protected void onCreate (Bundle savedInstanceState) {
// KOD GERİ DÖNÜŞ NOKTASI: GdxNativesLoader.load(); satırı buraya eklenecek.

// Eğer AndroidApplicationConfiguration sınıfını uzatıyorsanız, LibGDX bunu zaten yapar. 
// Ancak hata devam ettiği için, initialize() metodundan hemen önce bunu çağırarak YÜKLEMEYİ ZORLUYORUZ.

super.onCreate(savedInstanceState);

AndroidApplicationConfiguration config = new AndroidApplicationConfiguration();
// Hata devam ettiği için, LibGDX'in eski sürüm kurulumuna uygun bir initializer kullanıyoruz.
initialize(new SevgiliOyunu(), config);
}
}
