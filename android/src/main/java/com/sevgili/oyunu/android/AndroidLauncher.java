package com.sevgili.oyunu.android;

import android.os.Bundle;

import com.badlogic.gdx.backends.android.AndroidApplication;
// Bu sınıf, Vulkan uzantısına göre değişecektir, ancak mantığı budur.
import com.badlogic.gdx.backends.android.AndroidApplicationConfiguration;
import com.sevgili.oyunu.game.SevgiliOyunu;

public class AndroidLauncher extends AndroidApplication {
@Override
protected void onCreate (Bundle savedInstanceState) {
super.onCreate(savedInstanceState);

        // KRİTİK DEĞİŞİKLİK: Vulkan'ı destekleyen konfigürasyona geçiş yapılır.
        // Gdx-vulkan uzantısının varsayımsal konfigürasyonu burada çağrılacaktır.
        // Şimdilik OpenGL sınıfını kullanıp, kodun tamamlanmasını bekliyoruz.
        AndroidApplicationConfiguration config = new AndroidApplicationConfiguration();
        
        // Vulkan için 1.2 sürümünü zorlama ve GL sabitlerini kapatma
        config.useGL30 = false;
        config.useImmersiveMode = true;
        
        // Vulkan'da yüksek kare hızı gerekeceğinden, sensörleri devre dışı bırakıyoruz.
        config.useAccelerometer = false;
        config.useCompass = false;
        
        // Normalde burada VulkanConfiguration() gibi bir sınıf kullanılırdı.
        initialize(new SevgiliOyunu(), config);
}
}
