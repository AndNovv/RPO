package ru.iu3.fclient;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import ru.iu3.fclient.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'fclient' library on application startup.
    static {
        System.loadLibrary("fclient");
        System.loadLibrary("mbedcrypto");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        int res = initRng();
        byte[] v = randomBytes(16);

        String testString = "Test String for encryption";
        byte[] testByteArray = testString.getBytes();
        byte[] encryptedByteArray = encrypt(v, testByteArray);
        byte[] decryptedByteArray = decrypt(v, encryptedByteArray);
        String decryptedString = new String(decryptedByteArray);

        // Example of a call to a native method
        TextView tv = findViewById(R.id.sample_text);
        TextView encrypted_text = findViewById(R.id.encrypted);

        encrypted_text.setText(stringFromJNI());
        tv.setText(decryptedString);
    }

    /**
     * A native method that is implemented by the 'fclient' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
    public static native int initRng();
    public static native byte[] randomBytes(int no);
    public static native  byte[] encrypt(byte[] key, byte[] data);
    public static native  byte[] decrypt(byte[] key, byte[] data);
}