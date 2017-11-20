#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <opencv2/opencv.hpp>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#define LOG_TAG "native"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)


extern "C" {

using namespace cv;
using namespace std;
CascadeClassifier *faceClassifier;
ANativeWindow *nativeWindow;


void bitmap2Mat(JNIEnv *env, jobject bitmap, Mat &dst);



    JNIEXPORT void JNICALL
    Java_com_dbgs_facedetection_MainActivity_loadModel(JNIEnv *env, jobject instance,
                                                       jstring detectModel_) {
        const char *detectModel = env->GetStringUTFChars(detectModel_, 0);
        //获取一个Classifier model
        faceClassifier = new CascadeClassifier(detectModel);
        // TODO

        env->ReleaseStringUTFChars(detectModel_, detectModel);
    }

    JNIEXPORT jboolean JNICALL
    Java_com_dbgs_facedetection_MainActivity_process(JNIEnv *env, jobject instance, jobject bitmap) {

        // TODO
        int ret = 1;
        Mat src;

        bitmap2Mat(env, bitmap, src);
        imwrite("/sdcard/d.png",src);
        if (faceClassifier) {
            vector<Rect> faces;
            Mat grayMat;
            //图像灰度化
            cvtColor(src, grayMat, CV_BGR2GRAY);
            imwrite("/sdcard/huidu.png",grayMat);
            //直方图均衡化 增强对比效果
            equalizeHist(grayMat, grayMat);
            imwrite("/sdcard/e.png",grayMat);
            //识别，并将识别到的头部区域写入faces 向量中
            faceClassifier->detectMultiScale(grayMat, faces);
            grayMat.release();
            for (int i = 0; i < faces.size(); ++i) {
                Rect face = faces[i];
                // Scalar(0, 255, 255) 参数含义:矩形的颜色
                rectangle(src, face.tl(), face.br(), Scalar(0, 255, 255));
            }
        }
        if (!nativeWindow) {
            LOGI("native window null");
            ret = 0;
            goto end;
        }
        ANativeWindow_Buffer window_buffer;

        if (ANativeWindow_lock(nativeWindow, &window_buffer, 0)) {
            LOGI("native window lock fail");
            ret = 0;
            goto end;
        }
        imwrite("/sdcard/c.png",src);
        // 直接画了 不传到java了，使用原生的nativeWindow直接画
        cvtColor(src, src, CV_BGR2RGBA);
        imwrite("/sdcard/b.png",src);
        resize(src, src, Size(window_buffer.width, window_buffer.height));
        //todo window_buffer.height * window_buffer.width *4?
        memcpy(window_buffer.bits, src.data, window_buffer.height * window_buffer.width *4);
        ANativeWindow_unlockAndPost(nativeWindow);
        end:
        src.release();

        return ret;
    }

    JNIEXPORT void JNICALL
    Java_com_dbgs_facedetection_MainActivity_setSurface(JNIEnv *env, jobject instance, jobject surface,
                                                        jint w, jint h) {
        if(surface && w && h) {
            if (nativeWindow) {
                LOGI("release old native window");
                ANativeWindow_release(nativeWindow);
                nativeWindow = 0;
            }
            LOGI("new native window");
            nativeWindow = ANativeWindow_fromSurface(env, surface);
            if (nativeWindow) {
                //设置nativeWindow 的分辨率和 显示图像的格式
                ANativeWindow_setBuffersGeometry(nativeWindow, w, h, WINDOW_FORMAT_RGBA_8888);
            }
        } else {
            if (nativeWindow) {
                LOGI("release old native window");
                ANativeWindow_release(nativeWindow);
                nativeWindow = 0;
            }
        }
        // TODO

    }

    JNIEXPORT void JNICALL
    Java_com_dbgs_facedetection_MainActivity_destroy(JNIEnv *env, jobject instance) {

        // TODO
        if(faceClassifier) {
            delete faceClassifier;
            faceClassifier = 0;
        }
        if(nativeWindow) {
            ANativeWindow_release(nativeWindow);
            nativeWindow = 0;
        }
    }
void bitmap2Mat(JNIEnv *env, jobject bitmap, Mat &dst) {
#if 0
    AndroidBitmapInfo info;
    void *pixels = 0;
    //获取bitmap信息
    CV_Assert(AndroidBitmap_getInfo(env, bitmap, &info) >= 0);
    //必须是 rgba8888 rgb565
    CV_Assert(info.format == ANDROID_BITMAP_FORMAT_RGBA_8888);
    //lock 获得数据
    CV_Assert(AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0);
    CV_Assert(pixels);

    dst.create(info.height, info.width, CV_8UC3);
    LOGI("bitmap2Mat: RGBA_8888 bitmap -> Mat");
    Mat tmp;
    tmp = Mat(info.height, info.width, CV_8UC3, pixels);
    cvtColor(tmp, dst, COLOR_RGBA2BGR);
    tmp.release();
    AndroidBitmap_unlockPixels(env, bitmap);
#else
    AndroidBitmapInfo  info;
    void*              pixels = 0;


    try {
        LOGI("nBitmapToMat");
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        dst.create(info.height, info.width, CV_8UC4);
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            LOGI("nBitmapToMat: RGBA_8888 -> CV_8UC4");
            Mat tmp(info.height, info.width, CV_8UC4, pixels);
//            if(needUnPremultiplyAlpha) cvtColor(tmp, dst, COLOR_mRGBA2RGBA);
//            else tmp.copyTo(dst);
            tmp.copyTo(dst);
//            cvtColor(tmp, dst, COLOR_mRGBA2RGBA);
            cvtColor(dst, dst, COLOR_RGBA2BGR);
            tmp.release();
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            LOGI("nBitmapToMat: RGB_565 -> CV_8UC4");
            Mat tmp(info.height, info.width, CV_8UC2, pixels);
//            cvtColor(tmp, dst, COLOR_BGR5652RGBA);
            cvtColor(tmp, dst, COLOR_BGR5652BGR);
            tmp.release();
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGI("nBitmapToMat catched cv::Exception: %s", e.what());
        jclass je = env->FindClass("org/opencv/core/CvException");
        if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        LOGI("nBitmapToMat catched unknown exception (...)");
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
        return;
    }
#endif

}
}