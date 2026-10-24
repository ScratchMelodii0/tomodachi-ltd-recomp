# Keep all JNI-callable methods.
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep GameActivity subclass (referenced by AndroidManifest.xml).
-keep class com.tomodachi.recomp.** { *; }

# Keep enums intact.
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}
