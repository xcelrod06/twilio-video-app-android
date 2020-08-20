package com.twilio.video.app.integrationTest

import androidx.test.ext.junit.rules.activityScenarioRule
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.MediumTest
import com.twilio.video.app.R
import com.twilio.video.app.screen.assertSettingsTitleIsVisible
import com.twilio.video.app.ui.settings.SettingsActivity
import com.twilio.video.app.util.getString
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
@MediumTest
@IntegrationTest
class SettingsTest {

    @get:Rule
    var scenario = activityScenarioRule<SettingsActivity>()

    @Test
    fun configuration_change_should_not_crash_app() {
        val title = getString(R.string.settings_title)
        assertSettingsTitleIsVisible(title)
        scenario.scenario.recreate()
        assertSettingsTitleIsVisible(title)
    }
}
