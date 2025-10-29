package com.sevgili.oyunu.game;

import com.badlogic.gdx.ApplicationAdapter;
import com.badlogic.gdx.Gdx;                                      
import com.badlogic.gdx.graphics.GL20;                            
import com.badlogic.gdx.graphics.Color; // Ekledik

// Test amaçlı geçici olarak sadeleştirilmiş sınıf
public class SevgiliOyunu extends ApplicationAdapter {

    @Override
    public void create() {
        Gdx.app.log("OYUN", "--- OYUN BAŞARILIYLA YÜKLENDİ ---");
    }                                                             

    @Override
    public void render() {
        // EKLE-BAŞLAT: Ekranı tamamen Kırmızıya boyayarak test et.
        Gdx.gl.glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // Kırmızı
        Gdx.gl.glClear(GL20.GL_COLOR_BUFFER_BIT | GL20.GL_DEPTH_BUFFER_BIT);

        // Hiçbir çizim kodu yok. Sadece kırmızı ekran.
    }

    @Override
    public void dispose() {
        // Boş
    }
}
