package com.sevgili.oyunu.android;

import android.os.Bundle;

import com.badlogic.gdx.backends.android.AndroidApplication;
import com.badlogic.gdx.backends.android.AndroidApplicationConfiguration;
import com.sevgili.oyunu.game.SevgiliOyunu;

public class AndroidLauncher extends AndroidApplication {
    @Override
    protected void onCreate (Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        AndroidApplicationConfiguration config = new AndroidApplicationConfiguration();
        
        // Yüksek performans/grafik ayarları
        config.useImmersiveMode = true;
        config.useGL30 = true; 
        config.numSamples = 4; // Anti-aliasing
        
        initialize(new SevgiliOyunu(), config);
    }
}
