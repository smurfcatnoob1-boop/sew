package com.sevgili.oyunu.game;

import java.util.HashMap;
import java.util.Map;

public class GrafikAyarlari {

    // Grafik Seviyeleri ve Ayarları
    public static final Map<String, Map<String, Object>> SEVIYELER = Map.of(
        "DUSUK", Map.of("doku_carpan", 0.5f, "shadows", false, "anti_aliasing", 1, "fps_limit", 30),
        "ORTA", Map.of("doku_carpan", 1.0f, "shadows", false, "anti_aliasing", 2, "fps_limit", 60),
        "YUKSEK", Map.of("doku_carpan", 1.5f, "shadows", true, "anti_aliasing", 4, "fps_limit", 60),
        "COK_YUKSEK", Map.of("doku_carpan", 2.0f, "shadows", true, "anti_aliasing", 8, "fps_limit", 120)
    );
    
    public static String seviyeAdi = "ORTA";

    public static Map<String, Object> mevcutAyarlar;

    static {
        mevcutAyarlar = SEVIYELER.get(seviyeAdi);
    }

    public static void setSeviye(String yeniSeviyeAdi) {
        String upperName = yeniSeviyeAdi.toUpperCase();
        if (SEVIYELER.containsKey(upperName)) {
            seviyeAdi = upperName;
            mevcutAyarlar = SEVIYELER.get(seviyeAdi);
            Gdx.app.log("AYAR", "Grafik seviyesi ayarlandı: " + seviyeAdi);
        } else {
            Gdx.app.log("UYARI", "Geçersiz grafik seviyesi: " + yeniSeviyeAdi);
        }
    }
}
