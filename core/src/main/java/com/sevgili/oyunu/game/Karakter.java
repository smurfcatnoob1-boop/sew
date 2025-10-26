package com.sevgili.oyunu.game;

import com.badlogic.gdx.Gdx;
import com.badlogic.gdx.math.Vector3;
import java.util.Map;

// Kotlin'deki Karakter sınıfının Java karşılığı
public class Karakter {
    
    public final int kontrolId;
    public String aktifSkinIsmi;
    
    public Vector3 konum;
    public float hizX = 0f;
    public float hizY = 0f;
    public final float yerHizi = 5f;
    public final float zipGucu = 15f;
    
    private float animasyonTimer = 0f;
    private final float animasyonHizi = 0.15f;
    private final Map<String, Integer> animasyonKareleri = Map.of(
        "IDLE", 1, "RUN", 8, "JUMP", 4, "SOYUNMA", 10
    );
    
    public String durum = "IDLE"; 

    public Karakter(int kontrolId, String baslangicSkinIsmi, float startX, float startY) {
        this.kontrolId = kontrolId;
        this.aktifSkinIsmi = baslangicSkinIsmi;
        this.konum = new Vector3(startX, startY, 0f);
    }

    public int getCurrentFrame() {
        int uzunluk = animasyonKareleri.getOrDefault(durum, 1);
        int kareIndeksi = (int) (animasyonTimer % uzunluk);
        animasyonTimer += animasyonHizi;
        return kareIndeksi;
    }

    public void update(float yerCekimi, float zeminSeviyesi, float genislik) {
        hizY -= yerCekimi * Gdx.graphics.getDeltaTime();
        if (hizY < -20f) hizY = -20f;

        konum.x += hizX * Gdx.graphics.getDeltaTime(); 
        konum.y += hizY * Gdx.graphics.getDeltaTime();
        
        if (konum.y < zeminSeviyesi) {
            konum.y = zeminSeviyesi;
            hizY = 0f;
            if (durum.equals("JUMP")) durum = "IDLE";
        }
    }
}

class KarakterErkek extends Karakter {
    public KarakterErkek(int kontrolId, float startX, float startY) {
        super(kontrolId, "Kaan", startX, startY);
    }
}
