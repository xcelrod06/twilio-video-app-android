package com.twilio.video;

import android.support.test.filters.LargeTest;
import android.support.test.rule.ActivityTestRule;

import com.twilio.video.ui.MediaTestActivity;
import com.twilio.video.util.PermissionUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Arrays;

import static junit.framework.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

@RunWith(Parameterized.class)
@LargeTest
public class LocalAudioTrackAudioOptionsParameterizedTest {
    @Parameterized.Parameters(name = "echoCancellation: {0}, " +
            "autoGainControl: {1}, " +
            "noiseSuppression: {2}, " +
            "highpassFilter: {3}, " +
            "stereoSwapping: {4}, " +
            "audioJitterBufferFastAccerlerate: {5}, " +
            "typingDetection: {6}")
    public static Iterable<Object[]> data() {
        int numBooleanParameters = 7;
        int numAudioOptionsConfigurations = (int) Math.pow(2, numBooleanParameters);
        Object[][] testParams = new Object[numAudioOptionsConfigurations][numBooleanParameters];

        for (int i = 0x0 ; i < numAudioOptionsConfigurations ; i++) {
            Object[] audioOptions = new Object[numBooleanParameters];
            for (int j = 0 ; j < numBooleanParameters ; j++) {
                boolean enableAudioOption = (i & (0x1 << j)) == 0;
                audioOptions[j] = enableAudioOption;
            }
            testParams[i] = audioOptions;
        }

        return Arrays.asList(testParams);
    }

    private final AudioOptions audioOptions;
    @Rule public ActivityTestRule<MediaTestActivity> activityRule =
            new ActivityTestRule<>(MediaTestActivity.class);
    private MediaTestActivity mediaTestActivity;
    private LocalMedia localMedia;
    private LocalAudioTrack localAudioTrack;


    public LocalAudioTrackAudioOptionsParameterizedTest(boolean echoCancellation,
                                                        boolean autoGainControl,
                                                        boolean noiseSuppression,
                                                        boolean highpassFilter,
                                                        boolean stereoSwapping,
                                                        boolean audioJitterBufferFastAccelerate,
                                                        boolean typingDetection) {
        this.audioOptions = new AudioOptions.Builder()
                .echoCancellation(echoCancellation)
                .autoGainControl(autoGainControl)
                .noiseSuppression(noiseSuppression)
                .highpassFilter(highpassFilter)
                .stereoSwapping(stereoSwapping)
                .audioJitterBufferFastAccelerate(audioJitterBufferFastAccelerate)
                .typingDetection(typingDetection)
                .build();
    }

    @Before
    public void setup() {
        mediaTestActivity = activityRule.getActivity();
        PermissionUtils.allowPermissions(mediaTestActivity);
        localMedia = LocalMedia.create(mediaTestActivity);
    }

    @After
    public void teardown() {
        localMedia.removeAudioTrack(localAudioTrack);
        localMedia.release();
    }

    @Test
    public void canAddAudioTrackWithOptions() {
        localAudioTrack = localMedia.addAudioTrack(true, audioOptions);

        assertNotNull(localAudioTrack);
        assertTrue(localAudioTrack.isEnabled());
    }
}
