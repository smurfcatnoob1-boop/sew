package com.sevgili.oyunu.game;

import com.badlogic.gdx.ApplicationAdapter;
import com.badlogic.gdx.Gdx;                                      
import com.badlogic.gdx.graphics.GL20;                            
import com.badlogic.gdx.graphics.PerspectiveCamera;
import com.badlogic.gdx.graphics.g3d.Environment;                 
import com.badlogic.gdx.graphics.g3d.ModelBatch;                  
import com.badlogic.gdx.graphics.g3d.attributes.ColorAttribute;
import com.badlogic.gdx.graphics.g3d.environment.DirectionalLight;
import com.badlogic.gdx.math.Vector3;
import java.util.HashMap;

// Kotlin'deki SevgiliOyunu sınıfının Java karşılığı
public class SevgiliOyunu extends ApplicationAdapter {

    private PerspectiveCamera kamera;
    private ModelBatch modelBatch;                                    
    private Environment environment;

    // Oyun Durumu
    private Karakter yerelKarakter;

    @Override
    public void create() {
        // 1. Grafik ve Performans Ayarları (Statik çağrıyı güvenli hale getiriyoruz)
        // Eğer GrafikAyarlari hatasız yüklenirse, bu satır çalışır.
        try {
            int antiAliasing = (int) GrafikAyarlari.mevcutAyarlar.get("anti_aliasing");
            Gdx.gl.glEnable(GL20.GL_DEPTH_TEST);
        } catch (Exception e) {
            Gdx.app.error("GRAFIK_HATA", "Grafik Ayarları yüklenemedi: " + e.getMessage());
        }

        // 2. Kamera ve Ortam Kurulumu                                    
        kamera = new PerspectiveCamera(67, Gdx.graphics.getWidth(), Gdx.graphics.getHeight());
        kamera.position.set(10f, 10f, 10f);
        kamera.lookAt(0f, 0f, 0f);
        kamera.near = 0.1f;
        kamera.far = 300f;
        kamera.update();

        modelBatch = new ModelBatch();
        environment = createEnvironment();

        // 3. Oyun Bileşenlerini Başlatma
        // yerelKarakter = new KarakterErkek(0, 5f, 0f); // Model yükleme hatası ihtimaline karşı hala yorum satırında kalmalı.

        Gdx.app.log("OYUN", "Java/LibGDX 3D Oyun Başlatıldı. Seviye: Harita Yüklendi.");
    }                                                             
    private Environment createEnvironment() {                             
        Environment env = new Environment();
        env.set(new ColorAttribute(ColorAttribute.AmbientLight, 0.5f, 0.5f, 0.5f, 1f));
        env.add(new DirectionalLight().set(0.8f, 0.8f, 0.8f, -1f, -0.8f, -0.2f));
        return env;
    }

    @Override
    public void render() {
        // Çizim
        Gdx.gl.glClearColor(0.2f, 0.2f, 0.2f, 1f); // Gri temizleme rengi
        Gdx.gl.glClear(GL20.GL_COLOR_BUFFER_BIT | GL20.GL_DEPTH_BUFFER_BIT);

        // ModelBatch'i çalıştırmadan önce Karakter sınıfının yüklenmesini bekleyelim
        // modelBatch.begin(kamera);
        // modelBatch.end();
        
        // Bu sadece gri ekranı gösterecektir.
    }

    @Override
    public void dispose() {
        if (modelBatch != null) modelBatch.dispose();
    }
}
