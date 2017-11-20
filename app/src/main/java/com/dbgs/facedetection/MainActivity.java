package com.dbgs.facedetection;

import android.Manifest;
import android.app.ProgressDialog;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.dbgs.facedetection.interfaces.IPermission;
import com.tbruyelle.rxpermissions.RxPermissions;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;

import me.nereo.multi_image_selector.MultiImageSelectorActivity;
import rx.Subscriber;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback,IPermission{

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("face");
    }

    public native void loadModel(String detectModel);
    public native boolean process(Bitmap bitmap);
    private native void setSurface(Surface surface, int w, int h);
    public native void destroy();


    private SurfaceView surfaceView;
    private Bitmap bm;
    public static final int REQUEST_HEAD_IMAGE = 2000;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        findViewById(R.id.btn_take_img).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
             requestImages();
            }
        });
        // Example of a call to a native method
        surfaceView = (SurfaceView)findViewById(R.id.surfaceView);
        surfaceView.getHolder().addCallback(this);
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... voids) {
                File dir = new File(Environment.getExternalStorageDirectory(), "face");
                copyAssetsFile("haarcascade_frontalface_alt.xml", dir);
                File file = new File(dir, "haarcascade_frontalface_alt.xml");
                loadModel(file.getAbsolutePath());
                return null;
            }

            @Override
            protected void onPreExecute() {
                showLoading();
            }

            @Override
            protected void onPostExecute(Void aVoid) {
                dismissLoading();
            }
        }.execute();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        //分辨率
//        setSurface(surfaceHolder.getSurface(), 640, 480);
//        if (bm != null) {
//            process(bm);
//
//        }
        setSurface(holder.getSurface(), 640, 480);
        safeProcess();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        destroy();
        safeRecycled();
    }
    List<String> upoloadImgs;


    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
            if(requestCode == REQUEST_HEAD_IMAGE){
                if(resultCode == RESULT_OK){
                    if (upoloadImgs != null){
                        upoloadImgs.clear();
                        upoloadImgs = null;
                    }
                    upoloadImgs = data.getStringArrayListExtra(MultiImageSelectorActivity.EXTRA_RESULT);
                    if (upoloadImgs !=null &&upoloadImgs.size()>0){
                       String imagePath  = upoloadImgs.get(0);
                        if (!TextUtils.isEmpty(imagePath)) {
                            bm = toBitmap(imagePath);
                            safeProcess();
                        }
                    }

                }
            } else  {
                super.onActivityResult(requestCode, resultCode, data);
            }

    }

    public void safeProcess() {
        if (null != bm && !bm.isRecycled()) {
            process(bm);
        }
    }

    public void safeRecycled() {
        if (null != bm && !bm.isRecycled()) {
            bm.recycle();
        }
        bm = null;
    }
    private void copyAssetsFile(String name, File dir) {
        if (!dir.exists()) {
            dir.mkdirs();
        }

        File file = new File(dir, name);
        if (!file.exists()) {
            try {
                InputStream is = getAssets().open(name);
                FileOutputStream fos = new FileOutputStream(file);
                byte[] buffer = new byte[1028];
                int len;
                while ((len = is.read(buffer)) != -1) {
                    fos.write(buffer, 0, len);
                }
                fos.flush();
                fos.close();
                is.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private ProgressDialog pd;

    private void showLoading() {
        if (pd == null) {
            pd = new ProgressDialog(this);
            pd.setIndeterminate(true);
        }
        pd.show();
    }

    private void dismissLoading() {
        if (pd != null) {
            pd.dismiss();
        }
    }
    public static Bitmap toBitmap(String pathName) {
        if (TextUtils.isEmpty(pathName))
            return null;
        BitmapFactory.Options o = new BitmapFactory.Options();
        o.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(pathName, o);
        int width_tmp = o.outWidth, height_tmp = o.outHeight;
        int scale = 1;
        while (true) {
            if (width_tmp <= 640 && height_tmp <= 480) {
                break;
            }
            width_tmp /= 2;
            height_tmp /= 2;
            scale *= 2;
        }
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inSampleSize = scale;
        opts.outHeight = height_tmp;
        opts.outWidth = width_tmp;
        return BitmapFactory.decodeFile(pathName, opts);
    }

    void requestImages(){
        getPermission().request(Manifest.permission.CAMERA,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_EXTERNAL_STORAGE
        )
                .subscribe(new Subscriber<Boolean>() {
                    @Override
                    public void onCompleted() {

                    }

                    @Override
                    public void onError(Throwable e) {

                    }

                    @Override
                    public void onNext(Boolean aBoolean) {//AssessQuestionFragment
                        if (aBoolean){
                            Intent intent = new Intent(MainActivity.this, MultiImageSelectorActivity.class);
                            intent.putExtra(MultiImageSelectorActivity.EXTRA_SHOW_CAMERA, true);
                            intent.putExtra(MultiImageSelectorActivity.EXTRA_SELECT_MODE, MultiImageSelectorActivity.MODE_SINGLE);
                            startActivityForResult(intent, MainActivity.REQUEST_HEAD_IMAGE);
                        }else {
                            showCustomToast("请打开读写sd卡和调用相机的权限");
                        }

                    }
                });

    }
    RxPermissions rxPermissions ; // where this is an Activity instance
    @Override
    public RxPermissions getPermission() {
        if (rxPermissions == null){
            rxPermissions = new RxPermissions(this); // where this is an Activity instance
        }
        return rxPermissions;
    }

    void showCustomToast(String str){
        Toast.makeText(getApplicationContext(),str,Toast.LENGTH_SHORT).show();
    }
}
