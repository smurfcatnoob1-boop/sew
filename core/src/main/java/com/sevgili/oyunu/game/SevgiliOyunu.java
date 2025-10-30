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
import com.badlogic.gdx.graphics.g3d.loader.GltfAssetLoader; // KRİTİK İMPORT: GLTF/GLB Yükleyici
import com.badlogic.gdx.assets.loaders.resolvers.InternalFileHandleResolver; // KRİTİK İMPORT
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
        // ... (Grafik ve Kamera Kurulumları) ...

        modelBatch = new ModelBatch();
        environment = createEnvironment();

        // 3. Oyun Bileşenlerini Başlatma - KRİTİK: ASSET YÜKLEMEYİ ZORLAMA
        assets = new AssetManager();
        
        // KRİTİK EKLENTİ: GLTF ve GLB yükleyicilerini kaydetme
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
                // Karakteri oluştur ve model konumunu ayarla
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
    // ... (render ve dispose metotları aynı kalıyor) ...
}
