package com.sevgili.oyunu.game

// Kotlin Karşılığı: renderer.py içindeki GrafikAyarlari sınıfı
object GrafikAyarlari {
    
    // Grafik Seviyeleri ve Ayarları (OpenGL ES/Vulkan'a uygun çarpanlar)
    val SEVIYELER = mapOf(
        "DUSUK" to mapOf("doku_carpan" to 0.5f, "shadows" to false, "anti_aliasing" to 1, "fps_limit" to 30),
        "ORTA" to mapOf("doku_carpan" to 1.0f, "shadows" to false, "anti_aliasing" to 2, "fps_limit" to 60),
        "YUKSEK" to mapOf("doku_carpan" to 1.5f, "shadows" to true, "anti_aliasing" to 4, "fps_limit" to 60),
        "COK_YUKSEK" to mapOf("doku_carpan" to 2.0f, "shadows" to true, "anti_aliasing" to 8, "fps_limit" to 120) // En yüksek grafik kalitesi
    )
    
    var seviyeAdi: String = "ORTA"
        private set

    val mevcutAyarlar: Map<String, Any>
        get() = SEVIYELER[seviyeAdi] ?: SEVIYELER["ORTA"]!!

    fun setSeviye(yeniSeviyeAdi: String, donanimIzinVeriyor: Boolean = true): Boolean {
        val upperName = yeniSeviyeAdi.uppercase()
        if (upperName !in SEVIYELER) {
            println("UYARI: Geçersiz grafik seviyesi: $yeniSeviyeAdi")
            return false
        }

        if (!donanimIzinVeriyor && upperName in listOf("YUKSEK", "COK_YUKSEK")) {
            println("CIHAZ DESTEKLEMIYOR: Yüksek seviye seçilemedi.")
            return false
        }
            
        seviyeAdi = upperName
        println("Grafik seviyesi ayarlandı: $seviyeAdi (Doku: ${mevcutAyarlar["doku_carpan"]})")
        return true
    }
}
