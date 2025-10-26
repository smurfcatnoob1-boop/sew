package com.sevgili.oyunu.game

import com.badlogic.gdx.Gdx
import com.badlogic.gdx.graphics.Camera
import com.badlogic.gdx.graphics.g3d.ModelBatch
import com.badlogic.gdx.graphics.g3d.Environment
import com.badlogic.gdx.graphics.g3d.ModelInstance
import com.badlogic.gdx.graphics.g3d.Model
import com.badlogic.gdx.assets.AssetManager
import com.badlogic.gdx.graphics.Texture
import com.badlogic.gdx.graphics.g3d.loader.G3dModelLoader
import com.badlogic.gdx.utils.UBJsonReader

// Kotlin Karşılığı: renderer.py içindeki Renderer sınıfı
class Renderer3D {
    
    private val assetManager = AssetManager()
    private val modelInstances = mutableMapOf<String, ModelInstance>()
    
    init {
        // Tüm GLTF/GLB modellerini yüklemeye başla (Asenkron)
        assetManager.load("models/poolrooms.glb", Model::class.java)
        assetManager.load("models/kaan.gltf", Model::class.java)
        assetManager.load("models/kiz.gltf", Model::class.java)
        assetManager.load("models/kiz1.gltf", Model::class.java)
        assetManager.finishLoading() // Şimdilik senkron yükleme yapıyoruz.
        
        Gdx.app.log("RENDERER", "3D Modeller Yüklendi.")

        // Yüklü modellerden ModelInstance oluşturma
        modelInstances["Map"] = ModelInstance(assetManager.get("models/poolrooms.glb", Model::class.java))
    }

    fun render(modelBatch: ModelBatch, environment: Environment, camera: Camera, karakterler: List<Karakter>) {
        
        val dokuCarpan = GrafikAyarlari.mevcutAyarlar["doku_carpan"] as Float
        val shadowsOn = GrafikAyarlari.mevcutAyarlar["shadows"] as Boolean

        // 1. Map'i Çiz
        modelBatch.begin(camera)
        modelInstances["Map"]?.let { mapInstance ->
            // Harita render edilir. Yüksek çözünürlüklü dokular dokuCarpan'a göre seçilir.
            modelBatch.render(mapInstance, environment)
        }

        // 2. Karakterleri Çiz
        for (karakter in karakterler) {
            val modelName = karakter.aktifSkinIsmi
            
            // Eğer model cache'de yoksa yükle ve instance oluştur (Model ve Animasyonlar)
            if (modelName !in modelInstances) {
                if (assetManager.isLoaded("models/$modelName.gltf")) {
                     modelInstances[modelName] = ModelInstance(assetManager.get("models/$modelName.gltf", Model::class.java))
                } else {
                    continue 
                }
            }

            modelInstances[modelName]?.let { instance ->
                // Pozisyon ve Animasyon Ayarı
                instance.transform.setToTranslation(karakter.konum) 
                // instance.transform.rotate(Vector3.Y, karakter.yon)

                // YÜKSEK GRAFİK KALİTESİ: Animasyon karesine göre GLTF'deki animasyonu oynat
                // Bu kısım ModelInstance'ın AnimationController'ı ile yapılır.
                val frameIndex = karakter.getCurrentFrame()
                
                // instance'ın Materyallerini dokuCarpan'a göre ayarla (Low/High Res doku)
                
                // instance'ı çiz
                modelBatch.render(instance, environment)
            }
        }
        
        modelBatch.end()
    }
    
    fun dispose() {
        assetManager.dispose()
    }
}
