package ru.iu3.fclient;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import org.apache.commons.codec.DecoderException;
import org.apache.commons.codec.binary.Hex;

import ru.iu3.fclient.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity implements TransactionEvents {


    // Used to load the 'fclient' library on application startup.
    static {
        System.loadLibrary("fclient");
        System.loadLibrary("mbedcrypto");
    }

    private ActivityMainBinding binding;

    ActivityResultLauncher activityResultLauncher;

    public static byte[] stringToHex(String s)
    {
        byte[] hex;
        try
        {
            hex = Hex.decodeHex(s.toCharArray());
        }
        catch (DecoderException ex)
        {
            hex = null;
        }
        return hex;
    }

    // TransactionEvents Implementation
    private String pin;

    @Override
    public String enterPin(int ptc, String amount) {
        pin = new String();
        Intent it = new Intent(MainActivity.this, PinpadActivity.class);
        it.putExtra("ptc", ptc);
        it.putExtra("amount", amount);
        synchronized (MainActivity.this) {
            activityResultLauncher.launch(it);
            try {
                MainActivity.this.wait();
            } catch (Exception ex) {
                //todo: log error
            }
        }
        return pin;
    }

    @Override
    public void transactionResult(boolean result) {
        runOnUiThread(()-> {
            Toast.makeText(MainActivity.this, result ? "ok" : "failed", Toast.LENGTH_SHORT).show();
        });
    }


//    public void onButtonClick(View v) // Toast Test
//    {
//        Toast.makeText(this, "Hello", Toast.LENGTH_SHORT).show();
//    }

//    public void onButtonClick(View v) // encryption test
//    {
//        byte[] key = stringToHex("0123456789ABCDEF0123456789ABCDE0");
//        byte[] enc = encrypt(key, stringToHex("001002010050A0B102"));
//        byte[] dec = decrypt(key, enc);
//        String s = new String(Hex.encodeHex(dec)).toUpperCase();
//        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
//    }

    public void onButtonClick(View v)
    {
//        Intent it = new Intent(this, PinpadActivity.class);
//            startActivity(it);
//        activityResultLauncher.launch(it);
        byte[] trd = stringToHex("9F0206000000000100");
        transaction(trd);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        activityResultLauncher  = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                new ActivityResultCallback () {
                    @Override
                    public void onActivityResult(Object obj) {
                        if (obj instanceof ActivityResult) {
                            ActivityResult result = (ActivityResult) obj;
                            if (result.getResultCode() == Activity.RESULT_OK) {
                                Intent data = result.getData();
                                // обработка результата
//                                String pin = data.getStringExtra("pin");
//                                Toast.makeText(MainActivity.this, pin, Toast.LENGTH_SHORT).show();
                                pin = data.getStringExtra("pin");
                                synchronized (MainActivity.this) {
                                    MainActivity.this.notifyAll();
                                }
                            }
                        }
                    }

                    public void onActivityResult(ActivityResult result) {
                        if (result.getResultCode() == Activity.RESULT_OK) {
                            Intent data = result.getData();
                            // обработка результата
                            String pin = data.getStringExtra("pin");
                            Toast.makeText(MainActivity.this, pin, Toast.LENGTH_SHORT).show();
                        }
                    }
                });


//        int res = initRng();
//        byte[] v = randomBytes(16);
//
//        String testString = "Test String for encryption";
//        byte[] testByteArray = testString.getBytes();
//        byte[] encryptedByteArray = encrypt(v, testByteArray);
//        byte[] decryptedByteArray = decrypt(v, encryptedByteArray);
//        String decryptedString = new String(decryptedByteArray);
//
//        // Example of a call to a native method
//        TextView tv = findViewById(R.id.sample_text);
//        TextView encrypted_text = findViewById(R.id.encrypted);
//
//        encrypted_text.setText(stringFromJNI());
//        tv.setText(decryptedString);
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
    public native boolean transaction(byte[] trd);
}