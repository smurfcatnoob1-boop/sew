plugins {
    id("com.android.application")
    id("com.badlogic.gdx.backends.gdx-platform")
    kotlin("android")
}

android {
    namespace = "com.sevgili.oyunu.android"
    compileSdk = 34
    defaultConfig {
        applicationId = "com.sevgili.oyunu.android"
        minSdk = 24
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }
}

dependencies {
    implementation(project(":core"))
    implementation("com.badlogicgames.gdx:gdx-backend-android:1.12.1")
    // Gerekli LibGDX uzantıları buraya eklenir
}

// LibGDX için Asset (Kaynak) taşıma görevi
tasks.register<Copy>("copyAssets") {
    from("assets") 
    into("$buildDir/intermediates/assets/debug") 
}
