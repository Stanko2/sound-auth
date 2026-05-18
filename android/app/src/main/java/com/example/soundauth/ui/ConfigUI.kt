package com.example.soundauth.ui

import ads_mobile_sdk.h6
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Divider
import androidx.compose.material3.DividerDefaults
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import com.example.soundauth.AppPrefs
import com.example.soundauth.SoundTransferWrapper

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SoundConfigScreen(prefs: AppPrefs) {
    var expanded by remember { mutableStateOf(false) }
    var modType by remember { mutableStateOf(prefs.modType) }

    Column(Modifier.padding(16.dp).verticalScroll(rememberScrollState())) {
        Text("Protocol Settings", style = MaterialTheme.typography.titleMedium)
        NumericField("FFT Size", prefs.fftSize) { prefs.fftSize = it }
        NumericField("Marker F1", prefs.markerF1) { prefs.markerF1 = it }
        NumericField("Marker F2", prefs.markerF2) { prefs.markerF2 = it }
        NumericField("Message chunk size", prefs.chunkSize) { prefs.chunkSize = it }

        HorizontalDivider(
            Modifier.padding(vertical = 12.dp),
            DividerDefaults.Thickness,
            DividerDefaults.color
        )

        Text("Modulation Strategy", style = MaterialTheme.typography.titleSmall)
        val options = listOf("2tone", "MFSK", "simple")
        ExposedDropdownMenuBox(
            expanded = expanded,
            onExpandedChange = { expanded = !expanded }
        ) {
            OutlinedTextField(
                value = modType!!,
                onValueChange = {},
                readOnly = true,
                label = { Text("Select Strategy") },
                trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
                modifier = Modifier.menuAnchor().fillMaxWidth()
            )
            ExposedDropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
                options.forEach { selection ->
                    DropdownMenuItem(
                        text = { Text(selection) },
                        onClick = {
                            modType = selection
                            prefs.modType = selection
                            expanded = false
                        }
                    )
                }
            }
        }

        // Conditional Fields
        when (modType) {
            "2tone" -> {
                NumericField("Start Frequency", prefs.startFreq) { prefs.startFreq = it }
                NumericField("Spacing", prefs.spacing) { prefs.spacing = it }
                NumericField("Bits Per Frame", prefs.bitsPerFrame) { prefs.bitsPerFrame = it }
            }
            "MFSK" -> {
                NumericField("Start Frequency", prefs.startFreq) { prefs.startFreq = it }
                NumericField("Spacing", prefs.spacing) { prefs.spacing = it }
                NumericField("Region Size", prefs.regionSize) { prefs.regionSize = it }
                NumericField("Num Regions", prefs.numRegions) { prefs.numRegions = it }
            }
            "simple" -> {
                NumericField("Freq 1", prefs.simpleF1) { prefs.simpleF1 = it }
                NumericField("Freq 2", prefs.simpleF2) { prefs.simpleF2 = it }
            }
        }
    }
}

@Composable
fun <T> NumericField(label: String, value: T, onUpdate: (T) -> Unit) {
    var text by remember(value) { mutableStateOf(value.toString()) }
    OutlinedTextField(
        value = text,
        onValueChange = {
            text = it
            when(value) {
                is Int -> it.toIntOrNull()?.let { v -> onUpdate(v as T) }
                is Float -> it.toFloatOrNull()?.let { v -> onUpdate(v as T) }
            }
        },
        label = { Text(label) },
        modifier = Modifier.fillMaxWidth(),
        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Decimal)
    )
}