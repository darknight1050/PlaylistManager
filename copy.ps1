& ./build.ps1
if ($useDebug.IsPresent) {
    & adb push build/debug/libplaylistmanager.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libplaylistmanager.so
} else {
    & adb push build/libplaylistmanager.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libplaylistmanager.so
}

& adb shell am force-stop com.beatgames.beatsaber
& adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity
if ($log.IsPresent) {
    & ./log.ps1
}