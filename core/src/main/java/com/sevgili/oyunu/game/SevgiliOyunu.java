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
import com.badlogic.gdx.assets.AssetManager; // KRİTİK EKLENTİ
import com.badlogic.gdx.graphics.g3d.Model; // KRİTİK EKLENTİ
import com.badlogic.gdx.graphics.g3d.ModelInstance; // KRİTİK EKLENTİ
import java.util.HashMap;

public class SevgiliOyunu extends ApplicationAdapter {

    private PerspectiveCamera kamera;
    private ModelBatch modelBatch;
    private Environment environment;
    private AssetManager assets; // Asset yöneticisi
    private ModelInstance karakterModeli; // Yüklenen modelin örneği

    // Oyun Durumu
    private Karakter yerelKarakter;

    @Override
    public void create() {
        // 1. Grafik ve Performans Ayarları (Güvenlik kodları korunuyor)
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

        // 3. Oyun Bileşenlerini Başlatma - KRİTİK: ASSET YÜKLEMEYİ ZORLAMA
        assets = new AssetManager();
        String modelYolu = "kaan.gltf"; 

        try {
            assets.load(modelYolu, Model.class);
            // Yükleme tamamlanana kadar bekle (Hata varsa burada patlar)
            assets.finishLoading();

            if (assets.isLoaded(modelYolu)) {
                Model model = assets.get(modelYolu, Model.class);
                karakterModeli = new ModelInstance(model);
                yerelKarakter = new KarakterErkek(0, 5f, 0f);
                Gdx.app.log("ASSET_LOAD_INFO", "Model (" + modelYolu + ") BAŞARILIYLA YÜKLENDİ.");
            } else {
                 Gdx.app.error("ASSET_HATA_KRITIK", "Model yüklenemedi: " + modelYolu + " (AssetManager hatası)");
            }

        } catch (Exception e) {
            // Yükleme sırasında fırlayan hatayı loga yazdır.
            Gdx.app.error("ASSET_HATA_KRITIK", "Model YÜKLENEMEDİ! Yüklenen dosya: " + modelYolu + ". Detay: " + e.getMessage());
        }

        Gdx.app.log("OYUN", "Java/LibGDX 3D Oyun Başlatıldı.");
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

        // 3D Modelleme
        kamera.update(); // Kamera render döngüsünde güncellenmelidir.
        modelBatch.begin(kamera);

        // KRİTİK EKLENTİ: Yüklenen modeli çizmeye zorla.
        if (karakterModeli != null) {
            modelBatch.render(karakterModeli, environment);
        }

        modelBatch.end();
    }

    @Override
    public void dispose() {
        if (modelBatch != null) modelBatch.dispose();
        if (assets != null) assets.dispose(); // Assets'ı temizle
    }
}
