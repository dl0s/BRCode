import bb.cascades 1.4

TextArea {
    property bool scopeActive: true
    enabled: scopeActive
    textFormat: TextFormat.Plain
    inputMode: TextAreaInputMode.Default
    input.flags: TextInputFlag.AutoCorrectionOff | TextInputFlag.AutoCapitalizationOff | TextInputFlag.AutoPeriodOff | TextInputFlag.WordSubstitutionOff
}
