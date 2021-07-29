& ./build.ps1
& adb push libs/arm64-v8a/libplaylistmanager.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libplaylistmanager.so
& adb shell am force-stop com.beatgames.beatsaber
& adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity