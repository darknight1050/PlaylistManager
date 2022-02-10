& ./build.ps1
& adb push build/libplaylistmanager.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libplaylistmanager.so
& adb shell am force-stop com.beatgames.beatsaber
& adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity