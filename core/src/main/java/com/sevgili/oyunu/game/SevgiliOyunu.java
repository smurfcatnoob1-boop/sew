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
import com.badlogic.gdx.assets.AssetManager; 
import com.badlogic.gdx.graphics.g3d.Model; 
import com.badlogic.gdx.graphics.g3d.ModelInstance; 
import com.badlogic.gdx.graphics.g3d.loader.GltfAssetLoader; // HATA VEREN SINIF
import com.badlogic.gdx.assets.loaders.resolvers.InternalFileHandleResolver; 
import java.util.HashMap;

public class SevgiliOyunu extends ApplicationAdapter {

    private PerspectiveCamera kamera;
    private ModelBatch modelBatch;
    private Environment environment;
    private AssetManager assets; 
    private ModelInstance karakterModeli; 
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
        environment = createEnvironment(); // Metot çağrısı

        // 3. Oyun Bileşenlerini Başlatma - KRİTİK: ASSET YÜKLEMEYİ ZORLAMA
        assets = new AssetManager();
        
        // KRİTİK EKLENTİ: GLTF ve GLB yükleyicilerini kaydetme (Bağımlılık eklenene kadar hata verecek)
        GltfAssetLoader gltfLoader = new GltfAssetLoader(new InternalFileHandleResolver());
        assets.setLoader(Model.class, ".gltf", gltfLoader);
        assets.setLoader(Model.class, ".glb", gltfLoader);
        
        String modelYolu = "kaan.gltf"; 

        try {
            assets.load(modelYolu, Model.class);
            assets.finishLoading();

            if (assets.isLoaded(modelYolu)) {
                Model model = assets.get(modelYolu, Model.class);
                karakterModeli = new ModelInstance(model);
                yerelKarakter = new KarakterErkek(0, 5f, 0f);
                karakterModeli.transform.translate(yerelKarakter.konum.x, yerelKarakter.konum.y, yerelKarakter.konum.z);
                Gdx.app.log("ASSET_LOAD_INFO", "Model (" + modelYolu + ") BAŞARILIYLA YÜKLENDİ. HATA ÇÖZÜLDÜ.");
            } else {
                 Gdx.app.error("ASSET_HATA_KRITIK", "Model yüklenemedi: " + modelYolu + " (AssetManager hatası)");
            }

        } catch (Exception e) {
            Gdx.app.error("ASSET_HATA_KRITIK", "Model YÜKLENEMEDİ! Yüklenen dosya: " + modelYolu + ". Detay: " + e.getMessage());
        }

        Gdx.app.log("OYUN", "Java/LibGDX 3D Oyun Başlatıldı.");
    }                                                           
    
    // YENİDEN EKLENEN KRİTİK METOT TANIMI
    private Environment createEnvironment() {                   
        Environment env = new Environment();
        env.set(new ColorAttribute(ColorAttribute.AmbientLight, 0.5f, 0.5f, 0.5f, 1f));
        env.add(new DirectionalLight().set(0.8f, 0.8f, 0.8f, -1f, -0.8f, -0.2f));
        return env;
    }

    @Override
    public void render() {
        // Çizim
        kamera.update(); 
        Gdx.gl.glClearColor(0.2f, 0.2f, 0.2f, 1f); 
        Gdx.gl.glClear(GL20.GL_COLOR_BUFFER_BIT | GL20.GL_DEPTH_BUFFER_BIT);

        // 3D Modelleme
        modelBatch.begin(kamera);

        if (karakterModeli != null) {
            modelBatch.render(karakterModeli, environment);
        }

        modelBatch.end();
    }

    @Override
    public void dispose() {
        if (modelBatch != null) modelBatch.dispose();
        if (assets != null) assets.dispose();
    }
}
